/*  Shanti Keyboard
 *  Copyright (C) 2026 Hasan Muhammad
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "FcitxIntegration.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTimer>
#include <QWidget>

namespace {
constexpr auto kAppId = "org.shanti.ShantiFcitx5";
constexpr auto kMarker = "Shanti Flatpak autostart integration.";
constexpr auto kDaemonBusName = "org.fcitx.Fcitx5";
constexpr auto kDaemonBin = "/app/bin/fcitx5";
bool gPromptSuppressedUntilHealthy = false;

struct Paths {
    QString fcitxConfigDir;
    QString profile;
    QString autostart;
};

Paths paths() {
    const QString home = QDir::homePath();
    Paths result;
    result.fcitxConfigDir = QDir::cleanPath(home + "/.config/fcitx5");
    result.profile = result.fcitxConfigDir + "/profile";
    result.autostart = QDir::cleanPath(home + "/.config/autostart/" + kAppId + ".desktop");
    return result;
}

QString autostartEntry() {
    return QStringLiteral(
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Name=Shanti Keyboard (Fcitx5)\n"
        "Comment=Starts the Shanti Keyboard input method engine for Fcitx5 at login\n"
        "# %1\n"
        "Exec=flatpak run --user --command=fcitx5 org.shanti.ShantiFcitx5 -d\n"
        "X-GNOME-Autostart-enabled=true\n"
    ).arg(kMarker);
}

QString defaultProfile() {
    // Mirrors the shape fcitx5 itself writes (a comment line above every
    // key) so a generated file blends in with daemon-managed ones.
    return QStringLiteral(
        "[Groups/0]\n"
        "# Group Name\n"
        "Name=Default\n"
        "# Layout\n"
        "Default Layout=us\n"
        "# Default Input Method\n"
        "DefaultIM=shanti\n"
        "\n"
        "[Groups/0/Items/0]\n"
        "# Name\n"
        "Name=keyboard-us\n"
        "# Layout\n"
        "Layout=\n"
        "\n"
        "[Groups/0/Items/1]\n"
        "# Name\n"
        "Name=shanti\n"
        "# Layout\n"
        "Layout=\n"
        "\n"
        "[GroupOrder]\n"
        "0=Default\n"
    );
}

bool fileContainsMarker(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;
    return file.readAll().contains(kMarker);
}

bool daemonRunning() {
    auto *iface = QDBusConnection::sessionBus().interface();
    return iface && iface->isServiceRegistered(kDaemonBusName);
}

bool autostartPresent() {
    return QFileInfo(paths().autostart).isFile();
}

// Shanti counts as listed when some line is exactly its input-method
// name ("Name=shanti"), not a substring of another IM's name.
bool profileListsShanti(const QString &contents) {
    static const QRegularExpression linePattern(QStringLiteral("^\\s*Name\\s*=\\s*shanti\\s*$"));
    const QStringList lines = contents.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        if (linePattern.match(line).hasMatch())
            return true;
    }
    return false;
}

bool readProfile(QString *contents, QString *error) {
    const Paths p = paths();
    QFile file(p.profile);
    if (!file.exists()) {
        *contents = QString();
        return true;
    }
    if (!file.open(QIODevice::ReadOnly) || file.size() > 4 * 1024 * 1024) {
        *error = QString("Could not read the Fcitx5 profile at %1.").arg(p.profile);
        return false;
    }
    *contents = QString::fromUtf8(file.readAll());
    return true;
}

bool profileHasShanti() {
    QString contents;
    QString error;
    if (!readProfile(&contents, &error))
        return false;
    return profileListsShanti(contents);
}

// A Fcitx5 profile is a shared daemon-owned file, not an exclusively
// managed one; edits are always additive and never rewrite existing text.
bool writeProfileAtomic(const QString &contents, QString *error) {
    const Paths p = paths();
    if (!QDir().mkpath(p.fcitxConfigDir)) {
        *error = QString("Could not create directory: %1").arg(p.fcitxConfigDir);
        return false;
    }
    QSaveFile file(p.profile);
    const QByteArray data = contents.toUtf8();
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) ||
        file.write(data) != data.size() || !file.commit()) {
        *error = QString("Could not write: %1").arg(p.profile);
        return false;
    }
    return true;
}

// Inserts a "[Groups/<g>/Items/<N>]" block for Shanti after the last item
// block of the profile (group 0 preferred), never touching DefaultIM or
// any other line. Returns false when the profile has no recognizable
// groups; then fcitx5-configtool is the safer path.
bool insertImIntoProfile(QString &contents, QString *error) {
    const QRegularExpression itemHeader(QStringLiteral("^\\[Groups/(\\d+)/Items/(\\d+)\\]\\s*$"));
    const QRegularExpression groupHeader(QStringLiteral("^\\[Groups/(\\d+)\\]\\s*$"));

    int group = 0;
    int maxItem = -1;
    int anchor = -1; // line index of the item heading the new block follows

    {
        QStringList lines = contents.split(QLatin1Char('\n'));
        for (int pass = 0; pass < 2 && anchor < 0; ++pass) {
            group = 0;
            maxItem = -1;
            for (int i = 0; i < lines.size(); ++i) {
                const QRegularExpressionMatch match = itemHeader.match(lines.at(i));
                if (!match.hasMatch())
                    continue;
                const int g = match.captured(1).toInt();
                if (pass == 0 && g != 0)
                    continue;
                const int index = match.captured(2).toInt();
                if (index > maxItem) {
                    maxItem = index;
                    anchor = i;
                    group = g;
                }
            }
        }

        if (anchor < 0) {
            // A group that exists but has no items yet: insert right
            // before the section that follows its header.
            for (int i = 0; i < lines.size(); ++i) {
                const QRegularExpressionMatch match = groupHeader.match(lines.at(i));
                if (match.hasMatch()) {
                    group = match.captured(1).toInt();
                    anchor = i;
                    break;
                }
            }
        }
        if (anchor < 0) {
            *error = QString("The Fcitx5 profile at %1 has no recognisable input method groups; "
                             "configure it with fcitx5-configtool instead.")
                         .arg(paths().profile);
            return false;
        }

        // Insertion point: first section heading after the anchor, or EOF.
        int insertAt = lines.size();
        for (int i = anchor + 1; i < lines.size(); ++i) {
            const QString &line = lines.at(i);
            if (line.startsWith(QLatin1Char('[')) && line.trimmed().endsWith(QLatin1Char(']'))) {
                insertAt = i;
                break;
            }
        }

        QStringList block;
        if (insertAt > 0 && !lines.at(insertAt - 1).isEmpty())
            block << QString(); // blank separator, matching daemon formatting
        block << QString("[Groups/%1/Items/%2]").arg(group).arg(maxItem + 1)
              << "# Name"
              << "Name=shanti"
              << "# Layout"
              << "Layout=" << QString();

        for (int i = 0; i < block.size(); ++i)
            lines.insert(insertAt + i, block.at(i));

        QString result = lines.join(QLatin1Char('\n'));
        if (!result.endsWith(QLatin1Char('\n')))
            result += QLatin1Char('\n');
        contents = result;
    }
    return true;
}

bool writeAutostartEntry(QString *error) {
    const Paths p = paths();
    if (!QDir().mkpath(QFileInfo(p.autostart).absolutePath())) {
        *error = QString("Could not create directory: %1").arg(QFileInfo(p.autostart).absolutePath());
        return false;
    }
    QSaveFile file(p.autostart);
    const QByteArray data = autostartEntry().toUtf8();
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) ||
        file.write(data) != data.size() || !file.commit()) {
        *error = QString("Could not write: %1").arg(p.autostart);
        return false;
    }
    return true;
}

bool startDaemon(QString *error) {
    if (!QProcess::startDetached(QString(kDaemonBin), {QStringLiteral("-d")})) {
        *error = "Could not start the Fcitx5 daemon.";
        return false;
    }
    return true;
}

bool restartDaemon(QString *error) {
    QDBusInterface controller(kDaemonBusName, "/controller",
                              "org.fcitx.Fcitx.Controller1", QDBusConnection::sessionBus());
    if (!controller.isValid()) {
        *error = "Could not reach the running Fcitx5 daemon to apply the changes.";
        return false;
    }
    const QDBusMessage reply = controller.call("Restart");
    if (reply.type() == QDBusMessage::ErrorMessage) {
        *error = reply.errorMessage();
        return false;
    }
    return true;
}

bool setupNeeded() {
    return !autostartPresent() || !profileHasShanti() || !daemonRunning();
}

bool hasManagedFiles() {
    const Paths p = paths();
    return fileContainsMarker(p.autostart);
}

bool installIntegration(QString *error) {
    const Paths p = paths();
    const QString home = QDir::homePath();
    if (home.contains(QLatin1Char('\n')) || home.contains(QLatin1Char('\r'))) {
        *error = "This home directory path cannot be safely represented in the Fcitx5 configuration.";
        return false;
    }
    if (QFileInfo(p.autostart).exists() && !fileContainsMarker(p.autostart)) {
        *error = QString("Refusing to replace an existing unmanaged autostart file: %1").arg(p.autostart);
        return false;
    }

    QString profileContents;
    if (!readProfile(&profileContents, error))
        return false;

    bool profileChanged = false;
    if (profileContents.trimmed().isEmpty()) {
        // Fresh machine: claim the profile with Shanti as the default IM.
        profileContents = defaultProfile();
        profileChanged = true;
    } else if (!profileListsShanti(profileContents)) {
        if (!insertImIntoProfile(profileContents, error))
            return false;
        profileChanged = true;
    }
    if (profileChanged && !writeProfileAtomic(profileContents, error))
        return false;

    if (!writeAutostartEntry(error))
        return false;

    if (daemonRunning()) {
        // The daemon re-reads its profile on restart; without a restart
        // it only knows the IMs it loaded at startup.
        if (!restartDaemon(error))
            return false;
    } else if (!startDaemon(error)) {
        return false;
    }

    gPromptSuppressedUntilHealthy = false;
    return true;
}

bool removeIntegration(QString *error) {
    const Paths p = paths();
    // The profile is deliberately left untouched: it belongs to the
    // user's Fcitx setup, and the IM entry stays available there.
    if (QFileInfo(p.autostart).exists() && !fileContainsMarker(p.autostart)) {
        *error = QString("Refusing to remove an autostart file that is not managed by Shanti: %1").arg(p.autostart);
        return false;
    }
    if (!QFile::remove(p.autostart)) {
        *error = QString("Could not remove: %1").arg(p.autostart);
        return false;
    }
    gPromptSuppressedUntilHealthy = true;
    return true;
}

void showSetupError(QWidget *parent, const QString &title, const QString &error) {
    Q_UNUSED(parent);
    QMessageBox::critical(parent, title, error);
}

void showSetupPrompt(QWidget *parent) {
    if (!setupNeeded())
        return;

    QMessageBox box(QMessageBox::Information, "Set up Shanti for Fcitx5",
                    "The Fcitx5 daemon does not currently have Shanti registered as an input method.\n\n"
                    "Set up integration for this user? This adds Shanti to the Fcitx5 input method list, "
                    "creates a login autostart entry, and starts the Fcitx5 daemon.",
                    QMessageBox::NoButton, parent);
    QPushButton *setup = box.addButton("Set up integration", QMessageBox::AcceptRole);
    box.addButton("Later", QMessageBox::RejectRole);
    box.setDefaultButton(setup);
    box.exec();
    if (box.clickedButton() != setup)
        return;

    QString error;
    if (!installIntegration(&error)) {
        showSetupError(parent, "Could not set up Shanti", error);
        return;
    }
    QMessageBox::information(parent, "Shanti is registered",
                             "The Fcitx5 daemon is running with Shanti's input method. Switch input methods "
                             "(usually Ctrl+Space) and select Shanti, then start typing.");
}

void showManagementPrompt(QWidget *parent) {
    if (!FcitxIntegration::isApplicable())
        return;
    if (setupNeeded()) {
        showSetupPrompt(parent);
        return;
    }

    QMessageBox box(QMessageBox::Question, "Fcitx5 integration",
                    "Shanti is registered with Fcitx5 for this user. Remove that integration?",
                    QMessageBox::NoButton, parent);
    QPushButton *remove = box.addButton("Remove integration", QMessageBox::DestructiveRole);
    QPushButton *keep = box.addButton("Keep integration", QMessageBox::RejectRole);
    box.setDefaultButton(keep);
    box.exec();
    if (box.clickedButton() != remove)
        return;

    QString error;
    if (!removeIntegration(&error)) {
        QMessageBox::critical(parent, "Could not remove Shanti integration", error);
        return;
    }
    QMessageBox::information(parent, "Shanti integration removed",
                             "The login autostart entry was removed. Shanti remains in the Fcitx5 input method list.");
}

void showSetupOrRepair(QWidget *parent) {
    QMessageBox box(QMessageBox::Question, "Set up Shanti for Fcitx5",
                    "Set up or repair this user's Fcitx5 integration? This updates the Fcitx5 profile and the "
                    "login autostart entry, and restarts the Fcitx5 daemon.",
                    QMessageBox::Cancel, parent);
    QPushButton *setup = box.addButton("Set up / repair", QMessageBox::AcceptRole);
    box.setDefaultButton(setup);
    box.exec();
    if (box.clickedButton() != setup)
        return;

    QString error;
    if (!installIntegration(&error)) {
        showSetupError(parent, "Could not set up Shanti", error);
        return;
    }
    QMessageBox::information(parent, "Shanti is registered",
                             "The Fcitx5 daemon is running with Shanti's input method. Switch input methods "
                             "(usually Ctrl+Space) and select Shanti, then start typing.");
}

void showRemoveConfirmation(QWidget *parent) {
    QMessageBox box(QMessageBox::Warning, "Remove Fcitx5 integration",
                    "Remove Shanti's managed login autostart entry for this user? Shanti stays in the "
                    "Fcitx5 input method list.",
                    QMessageBox::Cancel, parent);
    QPushButton *remove = box.addButton("Remove integration", QMessageBox::DestructiveRole);
    box.setDefaultButton(QMessageBox::Cancel);
    box.exec();
    if (box.clickedButton() != remove)
        return;

    QString error;
    if (!removeIntegration(&error)) {
        QMessageBox::critical(parent, "Could not remove Shanti integration", error);
        return;
    }
    QMessageBox::information(parent, "Shanti integration removed",
                             "The login autostart entry was removed. Shanti remains in the Fcitx5 input method list.");
}
}

namespace FcitxIntegration {
bool isApplicable() {
    const auto env = QProcessEnvironment::systemEnvironment();
    return env.value("FLATPAK_ID") == kAppId;
}

QString statusText() {
    if (!isApplicable())
        return {};
    QStringList missing;
    if (!autostartPresent())
        missing << QStringLiteral("login autostart");
    if (!profileHasShanti())
        missing << QStringLiteral("Fcitx5 input method entry");
    if (!daemonRunning())
        missing << QStringLiteral("running daemon");
    if (!missing.isEmpty())
        return QStringLiteral("Not fully configured. Missing: %1.")
            .arg(missing.join(QStringLiteral(", ")));
    return QStringLiteral("Configured. The Fcitx5 daemon is running.");
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
        QMessageBox::information(parent, "Shanti integration",
                                 "No app-managed Fcitx5 autostart entry was found to remove.");
}
}
