#include "FlatpakIbusIntegration.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QSaveFile>
#include <QTimer>
#include <QUrl>
#include <QWidget>
#include <QXmlStreamWriter>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusVariant>

#include "PlatformConfig.h"

namespace {
constexpr auto kAppId = "org.openbangla.Shanti.IBus";
constexpr auto kUnit = "org.freedesktop.IBus.session.GNOME.service";
constexpr auto kMarker = "Managed by OpenBangla Keyboard Shanti Flatpak integration.";
constexpr auto kComponentName = "org.freedesktop.IBus.OpenBangla";
constexpr auto kEngineName = "OpenBangla";
bool gPromptSuppressedUntilHealthy = false;

struct Paths {
    QString dataHome;
    QString configHome;
    QString component;
    QString launcher;
    QString dropIn;
};

Paths paths() {
    const QString home = QDir::homePath();
    Paths result;
    result.dataHome = QDir::cleanPath(home + "/.local/share");
    result.configHome = QDir::cleanPath(home + "/.config");
    result.component = result.dataHome + "/ibus/component/shanti-flatpak-openbangla.xml";
    result.launcher = result.dataHome + "/shanti-ibus/ibus-flatpak-host-wrapper.sh";
    result.dropIn = result.configHome + "/systemd/user/" + kUnit + ".d/90-shanti-flatpak-ibus.conf";
    return result;
}

QString systemdEscape(QString value) {
    value.replace('%', "%%");
    value.replace('\\', "\\\\");
    value.replace('"', "\\\"");
    return value;
}

bool hasMarker(const QString &path) {
    QFileInfo info(path);
    if (info.isSymLink() || !info.isFile())
        return false;
    QFile file(path);
    return file.open(QIODevice::ReadOnly) && file.readAll().contains(kMarker);
}

QByteArray componentXml(const QString &launcher) {
    QByteArray xml;
    QXmlStreamWriter writer(&xml);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeComment(kMarker);
    writer.writeStartElement("component");
    writer.writeTextElement("name", kComponentName);
    writer.writeTextElement("description", "OpenBangla Keyboard (Flatpak engine)");
    QString shellQuotedLauncher = launcher;
    shellQuotedLauncher.replace("'", "'\\''");
    writer.writeTextElement("exec", "'" + shellQuotedLauncher + "'");
    writer.writeTextElement("version", QCoreApplication::applicationVersion());
    writer.writeTextElement("author", "OpenBangla Keyboard Shanti");
    writer.writeTextElement("license", "GPL-3.0-or-later");
    writer.writeTextElement("homepage", "https://github.com/hasanmuammar/OpenBangla-Keyboard-Shanti");
    writer.writeTextElement("textdomain", "openbangla");
    writer.writeStartElement("engines");
    writer.writeStartElement("engine");
    writer.writeTextElement("name", kEngineName);
    writer.writeTextElement("longname", "OpenBangla Keyboard");
    writer.writeTextElement("description", "OpenBangla Bengali input method running in Flatpak");
    writer.writeTextElement("language", "bn");
    writer.writeTextElement("license", "GPL-3.0-or-later");
    writer.writeTextElement("author", "OpenBangla Keyboard Shanti");
    writer.writeTextElement("layout", "default");
    writer.writeTextElement("rank", "80");
    writer.writeTextElement("symbol", "অ");
    writer.writeEndElement();
    writer.writeEndElement();
    writer.writeEndElement();
    writer.writeEndDocument();
    return xml;
}

QByteArray hostWrapper() {
    return QString::fromUtf8(R"SH(#!/usr/bin/env bash
# Managed by OpenBangla Keyboard Shanti Flatpak integration.
set -euo pipefail

fail() { printf 'Shanti IBus integration: %s\n' "$*" >&2; exit 1; }
command -v flatpak >/dev/null 2>&1 || fail 'flatpak command not found'
command -v ibus >/dev/null 2>&1 || fail 'ibus command not found'

ibus_address="$(ibus address)" || fail 'could not get the host IBus address'
[[ "$ibus_address" == unix:path=* ]] || fail 'unsupported host IBus address'
ibus_socket="${ibus_address#unix:path=}"
ibus_socket="${ibus_socket%%,*}"
[[ -n "$ibus_socket" && "$ibus_socket" == /* ]] || fail 'invalid host IBus socket path'

host_config_home="${XDG_CONFIG_HOME:-$HOME/.config}"
config_dir="$host_config_home/OpenBangla"
flatpak_args=("--filesystem=$ibus_socket:ro" "--env=IBUS_ADDRESS=$ibus_address" "--env=XDG_CONFIG_HOME=$host_config_home")
if [[ -d "$config_dir" ]]; then
    flatpak_args+=("--filesystem=$config_dir:ro")
fi

flatpak_pid=""
cleanup() {
    local status=$?
    trap - EXIT INT TERM HUP
    if [[ -n "$flatpak_pid" ]]; then
        if kill -0 -- "-$flatpak_pid" 2>/dev/null; then
            kill -TERM -- "-$flatpak_pid" 2>/dev/null || true
            for _ in {1..20}; do
                kill -0 -- "-$flatpak_pid" 2>/dev/null || break
                sleep 0.1
            done
            kill -KILL -- "-$flatpak_pid" 2>/dev/null || true
        fi
        wait "$flatpak_pid" 2>/dev/null || true
    fi
    exit "$status"
}
trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
set -m

flatpak run "${flatpak_args[@]}" \
    --talk-name=org.freedesktop.IBus \
    --own-name=org.freedesktop.IBus.OpenBangla \
    --command=/app/libexec/ibus-engine-openbangla \
    org.openbangla.Shanti.IBus --ibus "$@" &
flatpak_pid=$!

set +e
wait "$flatpak_pid"
status=$?
set -e
exit "$status"
)SH").toUtf8();
}

QByteArray serviceDropIn(const Paths &p) {
    const QString componentDir = QFileInfo(p.component).absolutePath();
    const QString componentPath = componentDir + ":/usr/share/ibus/component";
    return QString("# %1\n[Service]\nEnvironment=\"IBUS_COMPONENT_PATH=%2\"\nExecStart=\nExecStart=/usr/bin/ibus-daemon --panel disable --cache=refresh\n")
        .arg(kMarker, systemdEscape(componentPath)).toUtf8();
}

bool atomicWrite(const QString &path, const QByteArray &contents, QFileDevice::Permissions permissions, QString *error) {
    QFileInfo info(path);
    if (info.exists() || info.isSymLink()) {
        if (!hasMarker(path)) {
            *error = QString("Refusing to replace an existing unmanaged file: %1").arg(path);
            return false;
        }
    }
    if (!QDir().mkpath(info.absolutePath())) {
        *error = QString("Could not create directory: %1").arg(info.absolutePath());
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(contents) != contents.size() || !file.commit()) {
        *error = QString("Could not write: %1").arg(path);
        return false;
    }
    QFile::setPermissions(path, permissions);
    return true;
}

bool systemdCall(const QString &method, const QVariantList &arguments, QString *error) {
    QDBusInterface manager("org.freedesktop.systemd1", "/org/freedesktop/systemd1",
                           "org.freedesktop.systemd1.Manager", QDBusConnection::sessionBus());
    if (!manager.isValid()) {
        *error = "Could not connect to the GNOME user service manager.";
        return false;
    }
    QDBusMessage reply = manager.callWithArgumentList(QDBus::Block, method, arguments);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        *error = reply.errorMessage();
        return false;
    }
    return true;
}

bool serviceActive(bool *active) {
    QDBusInterface manager("org.freedesktop.systemd1", "/org/freedesktop/systemd1",
                           "org.freedesktop.systemd1.Manager", QDBusConnection::sessionBus());
    if (!manager.isValid())
        return false;
    QDBusReply<QDBusObjectPath> unit = manager.call("GetUnit", QString(kUnit));
    if (!unit.isValid())
        return false;
    QDBusInterface properties("org.freedesktop.systemd1", unit.value().path(),
                               "org.freedesktop.DBus.Properties", QDBusConnection::sessionBus());
    QDBusReply<QDBusVariant> state = properties.call("Get", "org.freedesktop.systemd1.Unit", "ActiveState");
    if (!state.isValid())
        return false;
    *active = state.value().variant().toString() == "active";
    return true;
}

bool setupNeeded() {
    const Paths p = paths();
    if (!hasMarker(p.component) || !hasMarker(p.launcher) || !hasMarker(p.dropIn) || !QFileInfo(p.launcher).isExecutable())
        return true;
    QFile dropIn(p.dropIn);
    if (!dropIn.open(QIODevice::ReadOnly) || !dropIn.readAll().contains(QFileInfo(p.component).absolutePath().toUtf8()))
        return true;
    bool active = false;
    return !serviceActive(&active) || !active;
}

bool hasManagedFiles() {
    const Paths p = paths();
    return hasMarker(p.component) || hasMarker(p.launcher) || hasMarker(p.dropIn);
}

bool hasConflictingDropIn(const Paths &p, QString *conflict) {
    const QDir dir(QFileInfo(p.dropIn).absolutePath());
    const QStringList files = dir.entryList({"*.conf"}, QDir::Files | QDir::NoSymLinks);
    for (const QString &name : files) {
        const QString candidate = dir.filePath(name);
        if (candidate == p.dropIn || hasMarker(candidate))
            continue;
        QFile file(candidate);
        if (!file.open(QIODevice::ReadOnly))
            continue;
        const QByteArray contents = file.readAll();
        if (contents.contains("IBUS_COMPONENT_PATH") || contents.contains("ExecStart=")) {
            *conflict = candidate;
            return true;
        }
    }
    return false;
}

bool moveDropInToBackup(const QString &path, QString *backupPath, QString *error) {
    const QString base = path + ".openbangla-backup";
    QString candidate = base;
    for (int suffix = 2; QFileInfo(candidate).exists(); ++suffix)
        candidate = base + QString("-%1").arg(suffix);

    QFile file(path);
    if (!file.rename(candidate)) {
        *error = QString("Could not preserve the previous Shanti configuration at %1.").arg(candidate);
        return false;
    }
    *backupPath = candidate;
    return true;
}

void restoreLegacyDropIn(const QString &backupPath, const QString &originalPath) {
    if (!backupPath.isEmpty() && !QFileInfo(originalPath).exists())
        QFile::rename(backupPath, originalPath);
}

bool installIntegration(QString *error, QString *conflictPath = nullptr) {
    const Paths p = paths();
    if (p.dataHome.contains(':') || p.dataHome.contains('\n') || p.dataHome.contains('\r') ||
        p.configHome.contains('\n') || p.configHome.contains('\r')) {
        *error = "This home directory path cannot be safely represented in the IBus or systemd configuration.";
        return false;
    }

    bool active = false;
    if (!serviceActive(&active)) {
        *error = "The GNOME IBus user service was not found or is unavailable in this session.";
        return false;
    }

    if ((QFileInfo(p.dropIn).exists() || QFileInfo(p.dropIn).isSymLink()) && !hasMarker(p.dropIn)) {
        if (conflictPath)
            *conflictPath = p.dropIn;
        *error = QString("An existing IBus service override may conflict with this setup: %1").arg(p.dropIn);
        return false;
    }

    QString conflict;
    if (hasConflictingDropIn(p, &conflict)) {
        if (conflictPath)
            *conflictPath = conflict;
        *error = QString("A different IBus service override may conflict with this setup: %1").arg(conflict);
        return false;
    }

    const QFileDevice::Permissions executable = QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
        QFileDevice::ReadGroup | QFileDevice::ExeGroup | QFileDevice::ReadOther | QFileDevice::ExeOther;
    const QFileDevice::Permissions readable = QFileDevice::ReadOwner | QFileDevice::WriteOwner |
        QFileDevice::ReadGroup | QFileDevice::ReadOther;

    if (!atomicWrite(p.component, componentXml(p.launcher), readable, error) ||
        !atomicWrite(p.launcher, hostWrapper(), executable, error) ||
        !atomicWrite(p.dropIn, serviceDropIn(p), readable, error))
        return false;

    if (!systemdCall("Reload", {}, error) ||
        !systemdCall("RestartUnit", {QString(kUnit), QString("replace")}, error))
        return false;
    gPromptSuppressedUntilHealthy = false;
    return true;
}

void showSetupError(QWidget *parent, const QString &title, const QString &error, const QString &conflictPath) {
    QString message = error;
    if (!conflictPath.isEmpty()) {
        message += "\n\nSelect Open override folder to review this file. If it is an old Shanti setup you want to replace, use Back up and continue to retry setup. Do not change overrides you still use.";
    }

    QMessageBox box(QMessageBox::Critical, title, message, QMessageBox::NoButton, parent);
    QPushButton *openFolder = nullptr;
    if (!conflictPath.isEmpty())
        openFolder = box.addButton("Open override folder", QMessageBox::ActionRole);
    box.addButton(QMessageBox::Ok);
    box.setDefaultButton(QMessageBox::Ok);
    box.exec();

    if (box.clickedButton() == openFolder &&
        !QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(conflictPath).absolutePath()))) {
        QMessageBox::warning(parent, title,
                             QString("Could not open the folder. You can find the conflicting file here:\n%1")
                                 .arg(QFileInfo(conflictPath).absolutePath()));
    }
}

void showSetupFailure(QWidget *parent, const QString &error, const QString &conflictPath) {
    if (!conflictPath.isEmpty()) {
        QMessageBox box(QMessageBox::Question, "Update OpenBangla integration",
                        QString("An IBus service override is blocking setup:\n%1\n\nIf this is an old Shanti setup you want to replace, choose Back up and continue. The file will be preserved as a backup before the current integration is installed. If you do not recognize it, cancel and leave it unchanged.")
                            .arg(conflictPath),
                        QMessageBox::NoButton, parent);
        QPushButton *update = box.addButton("Back up and continue", QMessageBox::AcceptRole);
        QPushButton *openFolder = box.addButton("Open override folder", QMessageBox::ActionRole);
        box.addButton("Cancel", QMessageBox::RejectRole);
        box.setDefaultButton(update);
        box.exec();
        if (box.clickedButton() == openFolder) {
            if (!QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(conflictPath).absolutePath())))
                QMessageBox::warning(parent, "Could not open override folder", QFileInfo(conflictPath).absolutePath());
            return;
        }
        if (box.clickedButton() != update)
            return;

        QString backupPath;
        QString backupError;
        if (!moveDropInToBackup(conflictPath, &backupPath, &backupError)) {
            QMessageBox::critical(parent, "Could not update OpenBangla integration", backupError);
            return;
        }

        QString installError;
        QString nextConflict;
        if (!installIntegration(&installError, &nextConflict)) {
            restoreLegacyDropIn(backupPath, conflictPath);
            showSetupError(parent, "Could not update OpenBangla integration", installError, nextConflict);
            return;
        }

        QMessageBox::information(parent, "OpenBangla integration updated",
                                 QString("The current GNOME IBus integration is set up. The previous configuration was saved as:\n%1")
                                     .arg(backupPath));
        return;
    }
    showSetupError(parent, "Could not set up OpenBangla", error, conflictPath);
}

bool removeIntegration(QString *error) {
    const Paths p = paths();
    const QStringList ownedFiles = {p.component, p.launcher, p.dropIn};
    for (const QString &path : ownedFiles) {
        if ((QFileInfo(path).exists() || QFileInfo(path).isSymLink()) && !hasMarker(path)) {
            *error = QString("Refusing to remove a file that is not managed by OpenBangla: %1").arg(path);
            return false;
        }
    }
    for (const QString &path : ownedFiles) {
        if (QFileInfo::exists(path) && !QFile::remove(path)) {
            *error = QString("Could not remove: %1").arg(path);
            return false;
        }
    }
    if (!systemdCall("Reload", {}, error) ||
        !systemdCall("RestartUnit", {QString(kUnit), QString("replace")}, error))
        return false;
    gPromptSuppressedUntilHealthy = true;
    return true;
}

void showSetupPrompt(QWidget *parent) {
    if (!setupNeeded())
        return;

    QMessageBox box(QMessageBox::Information, "Set up OpenBangla for GNOME",
                    "GNOME IBus does not currently have OpenBangla registered as an input source.\n\n"
                    "Set up integration for this user? This adds an IBus component, a Flatpak launcher, "
                    "and a user-level override for GNOME's IBus service.",
                    QMessageBox::NoButton, parent);
    QPushButton *setup = box.addButton("Set up integration", QMessageBox::AcceptRole);
    box.addButton("Later", QMessageBox::RejectRole);
    box.setDefaultButton(setup);
    box.exec();
    if (box.clickedButton() != setup)
        return;

    QString error;
    QString conflictPath;
    if (!installIntegration(&error, &conflictPath)) {
        showSetupFailure(parent, error, conflictPath);
        return;
    }
    QMessageBox::information(parent, "OpenBangla is registered",
                             "GNOME's IBus service has been restarted. Open Settings → Keyboard → Input Sources, "
                             "choose Add Input Source, and select OpenBangla.");
}

void showManagementPrompt(QWidget *parent) {
    if (!FlatpakIbusIntegration::isApplicable())
        return;
    if (setupNeeded()) {
        showSetupPrompt(parent);
        return;
    }

    QMessageBox box(QMessageBox::Question, "GNOME IBus integration",
                    "OpenBangla is registered with GNOME IBus for this user. Remove that registration?",
                    QMessageBox::NoButton, parent);
    QPushButton *remove = box.addButton("Remove integration", QMessageBox::DestructiveRole);
    QPushButton *keep = box.addButton("Keep integration", QMessageBox::RejectRole);
    box.setDefaultButton(keep);
    box.exec();
    if (box.clickedButton() != remove)
        return;

    QString error;
    if (!removeIntegration(&error)) {
        QMessageBox::critical(parent, "Could not remove OpenBangla integration", error);
        return;
    }
    QMessageBox::information(parent, "OpenBangla integration removed",
                             "The user-level IBus registration was removed and GNOME's IBus service restarted.");
}

void showSetupOrRepair(QWidget *parent) {
    QMessageBox box(QMessageBox::Question, "Set up OpenBangla for GNOME",
                    "Set up or repair this user's GNOME IBus registration? This writes the managed IBus files and restarts GNOME's IBus service.",
                    QMessageBox::Cancel, parent);
    QPushButton *setup = box.addButton("Set up / repair", QMessageBox::AcceptRole);
    box.setDefaultButton(setup);
    box.exec();
    if (box.clickedButton() != setup)
        return;

    QString error;
    QString conflictPath;
    if (!installIntegration(&error, &conflictPath)) {
        showSetupFailure(parent, error, conflictPath);
        return;
    }
    QMessageBox::information(parent, "OpenBangla is registered",
                             "GNOME's IBus service has been restarted. Open Settings → Keyboard → Input Sources and select OpenBangla.");
}

void showRemoveConfirmation(QWidget *parent) {
    QMessageBox box(QMessageBox::Warning, "Remove GNOME IBus integration",
                    "Remove OpenBangla's managed GNOME IBus registration for this user? GNOME's IBus service will be restarted.",
                    QMessageBox::Cancel, parent);
    QPushButton *remove = box.addButton("Remove integration", QMessageBox::DestructiveRole);
    box.setDefaultButton(QMessageBox::Cancel);
    box.exec();
    if (box.clickedButton() != remove)
        return;

    QString error;
    if (!removeIntegration(&error)) {
        QMessageBox::critical(parent, "Could not remove OpenBangla integration", error);
        return;
    }
    QMessageBox::information(parent, "OpenBangla integration removed",
                             "The user-level IBus registration was removed and GNOME's IBus service restarted.");
}
}

namespace FlatpakIbusIntegration {
bool isApplicable() {
    const auto env = QProcessEnvironment::systemEnvironment();
    return env.value("FLATPAK_ID") == kAppId && detectDesktopEnvironment() == DesktopEnvironment::GNOME;
}

QString statusText() {
    if (!isApplicable())
        return {};
    if (setupNeeded())
        return "Not configured, or the GNOME IBus service is unavailable.";
    return "Configured. The GNOME IBus service is active.";
}

bool canRemove() {
    return isApplicable() && hasManagedFiles();
}

void checkAndPrompt(QWidget *parent) {
    if (!isApplicable())
        return;
    bool previouslyHealthy = !setupNeeded();
    QTimer::singleShot(0, parent, [parent]() { showSetupPrompt(parent); });
    auto *timer = new QTimer(parent);
    timer->setInterval(5 * 60 * 1000);
    QObject::connect(timer, &QTimer::timeout, parent, [parent, previouslyHealthy]() mutable {
        const bool healthy = !setupNeeded();
        if (healthy)
            gPromptSuppressedUntilHealthy = false;
        if (previouslyHealthy && !healthy && !gPromptSuppressedUntilHealthy)
            showSetupPrompt(parent);
        previouslyHealthy = healthy;
    });
    timer->start();
}

void showManagementDialog(QWidget *parent) {
    showManagementPrompt(parent);
}

void setupOrRepair(QWidget *parent) {
    if (isApplicable())
        showSetupOrRepair(parent);
}

void remove(QWidget *parent) {
    if (canRemove())
        showRemoveConfirmation(parent);
    else if (isApplicable())
        QMessageBox::information(parent, "OpenBangla integration",
                                 "No app-managed GNOME IBus files were found to remove.");
}
}
