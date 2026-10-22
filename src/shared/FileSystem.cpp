/*
 *  Shanti Keyboard
 *  Copyright (C) 2016 Muhammad Mominul Huque <mominul2082@gmail.com>
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include "FileSystem.h"
#include "Log.h"

UserFolders *gUserFolders = nullptr;

#ifdef Q_OS_LINUX
namespace {
QString linuxDataDirectory() {
    const QString bundledDirectory =
        QDir::cleanPath(QCoreApplication::applicationDirPath() +
                        "/../share/shanti-keyboard");
    if (QDir(bundledDirectory).exists()) {
        return bundledDirectory;
    }

    return QDir::cleanPath(
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
        "/shanti-keyboard");
}

// Copy `src` into `dst` recursively (files already present in `dst` are kept).
bool copyDirRecursively(const QString &srcPath, const QString &dstPath) {
    QDir dst(dstPath);
    if (!dst.mkpath(dstPath)) return false;
    const QDir src(srcPath);
    const auto entries = src.entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);
    for (const QFileInfo &entry : entries) {
        if (entry.isDir()) {
            if (!copyDirRecursively(entry.absoluteFilePath(),
                                    dst.filePath(entry.fileName()))) {
                return false;
            }
        } else {
            const QString target = dst.filePath(entry.fileName());
            if (QFile::exists(target)) continue;
            if (!QFile::copy(entry.absoluteFilePath(), target)) return false;
        }
    }
    return true;
}

// One-time rebrand migration: copy the legacy openbangla-keyboard locations
// to the shanti names when the new ones are missing. Legacy files are kept
// untouched (the uninstaller cleans both names).
void migrateLegacyPaths() {
    const QString genericData =
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    const QString legacyData = genericData + "/openbangla-keyboard";
    const QString dataDir = genericData + "/shanti-keyboard";
    if (QDir(legacyData).exists() && !QDir(dataDir).exists()) {
        copyDirRecursively(legacyData, dataDir);
    }

    const QString configHome =
        QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    const QString legacyStore = configHome + "/OpenBangla/Keyboard.conf";
    const QString storeDir = configHome + "/Shanti";
    const QString store = storeDir + "/Shanti.conf";
    if (QFile::exists(legacyStore) && !QFile::exists(store)) {
        if (QDir().mkpath(storeDir)) {
            QFile::copy(legacyStore, store);
        }
    }
}
}
#endif

void UserFolders::setupLinux() {
    migrateLegacyPaths();
    path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/shanti-keyboard";
    // Create our folder in the user specific data folder
    dir.mkpath(path);
    // Create user specific layouts folder
    dir.mkpath(path + "/layouts/");
}

void UserFolders::setupMacOS() {
    path = dir.homePath() + "/Library/Containers/org.shanti.inputmethod.keyboard/Data/Library/Application Support/org.shanti.inputmethod.keyboard";
    // Create our folder in the user specific data folder
    bool ret = dir.mkpath(path);

    // Create user specific layouts folder
    dir.mkpath(path + "/layouts/");

    LOG_DEBUG("Shanti IME directory present: %d\n", ret);

    QString resourcePath = QCoreApplication::applicationDirPath() + "/../Resources/";
    QDir resourceDir =  QDir(resourcePath);

    LOG_DEBUG("Shanti user folder: %s\n", path.toStdString().c_str());
    LOG_DEBUG("Shanti GUI path: %s\n", resourceDir.absolutePath().toUtf8().constData());

    QDir layoutsDir = QDir(resourceDir.filePath("layouts"));
    QDir userLayoutsDir = QDir(path + "/layouts/");

    QStringList files = layoutsDir.entryList(QDir::Files);

    for (const QString &file : files) {
        // Migrate data files to user specific folder on first run
        bool ret = migrateFile(file, layoutsDir, userLayoutsDir, false);
        LOG_DEBUG("Migrating layout %s: %d\n", file.toStdString().c_str(), ret);
    }
}

void UserFolders::setupWindows() {
    path = qEnvironmentVariable("LOCALAPPDATA") + "/Shanti";
    // Create our folder in the user specific data folder
    dir.mkpath(path);
    // Create user specific layouts folder
    dir.mkpath(path + "/layouts/");
}

QString LayoutsFilePath() {
#ifdef Q_OS_MACOS
    return gUserFolders->dataPath() + "/layouts/";
#elif defined(Q_OS_WIN)
    return QCoreApplication::applicationDirPath() + "/layouts";
#else
    return linuxDataDirectory() + "/layouts";
#endif
}

QString AvroPhoneticLayoutPath() {
#ifdef Q_OS_MACOS
    return gUserFolders->dataPath() + "/layouts/avrophonetic.json";
#elif defined(Q_OS_WIN)
    return QCoreApplication::applicationDirPath() + "/layouts/avrophonetic.json";
#else
    return linuxDataDirectory() + "/layouts/avrophonetic.json";
#endif
}

QString AutoCorrectFilePath() {
#ifdef Q_OS_MACOS
    return QCoreApplication::applicationDirPath() + "/../Resources/data/autocorrect.json";
#elif defined(Q_OS_WIN)
    return QCoreApplication::applicationDirPath() + "/data/autocorrect.json";
#else
    return linuxDataDirectory() + "/data/autocorrect.json";
#endif
}

#ifdef Q_OS_LINUX
    QString DatabasePath() {
        return linuxDataDirectory() + "/data";
    }

    QString DictionaryPath() {
        return linuxDataDirectory() + "/data/dictionary.json";
    }

    QString SuffixDictPath() {
        return linuxDataDirectory() + "/data/suffix.json";
    }

    QString RegexDictPath() {
        return linuxDataDirectory() + "/data/regex.json";
    }
#endif


/// Copy the `fileName` from `src` to `dst`.
/// 
/// overwrite: overwrite the file if it already exists in the destination.
/// 
/// return true if the file was copied successfully, false otherwise.
bool migrateFile(const QString &fileName, const QDir &src, const QDir &dst, bool overwrite) {
    QString srcFile = src.filePath(fileName);
    QString dstFile = dst.filePath(fileName);

    if(QFile::exists(srcFile) && QFile::exists(dstFile)) {
        if(!overwrite) {
            return false;
        }
        QFile::remove(dstFile);
    }

    return QFile::copy(srcFile, dstFile);
}
