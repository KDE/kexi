/* This file is part of the KDE project
   Copyright (C) 2016 Jarosław Staniek <staniek@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QResource>
#include <QStandardPaths>
#include <QDebug>

#ifdef QT_ONLY
#define KLocalizedString QString
#endif

//! @return true if @a path is readable
static bool fileReadable(const QString &path)
{
    return !path.isEmpty() && QFileInfo(path).isReadable();
}

static QString locateFile(const QString& path, QStandardPaths::StandardLocation location,
                          const QString &extraLocation)
{
    // let QStandardPaths handle this, it will look for app local stuff
    QString fullPath = QFileInfo(
        QStandardPaths::locate(location, path)).canonicalFilePath();
    if (fileReadable(fullPath)) {
        return fullPath;
    }

    // Try extra location
    fullPath = QFileInfo(extraLocation + '/' + path).canonicalFilePath();
    if (fileReadable(fullPath)) {
        return fullPath;
    }

    // A workaround: locations for QStandardPaths::AppDataLocation end with app name.
    // If this is not a kexi app but a test app such as GlobalSearchTest, replace
    // the subdir name with "kexi" so we can find Kexi file(s).
    QRegularExpression re(QLatin1Char('/') + QCoreApplication::applicationName() + '$');
    QStringList standardLocations(QStandardPaths::standardLocations(location));
    if (!extraLocation.isEmpty()) {
        standardLocations.append(extraLocation);
    }
    for(const QString &dir : standardLocations) {
        if (dir.indexOf(re) != -1) {
            QString realDir(dir);
            realDir.replace(re, QLatin1String("/kexi"));
            fullPath = realDir + '/' + path;
            if (fileReadable(fullPath)) {
                break;
            }
        }
    }
    return fullPath;
}

static bool registerResource(const QString& path, QStandardPaths::StandardLocation location,
                             const QString &resourceRoot, const QString &extraLocation,
                             KLocalizedString *errorMessage, KLocalizedString *detailsErrorMessage)
{
    const QString fullPath = locateFile(path, location, extraLocation);
    if (fullPath.isEmpty() || !QFileInfo(fullPath).isReadable()
        || !QResource::registerResource(fullPath, resourceRoot))
    {
        QStringList triedLocations(QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation));
        if (!extraLocation.isEmpty()) {
            triedLocations.append(extraLocation);
        }
        QString triedLocationsString;
        for(const QString &triedLocation : triedLocations) {
            KLocalizedString string;
            if (triedLocationsString.isEmpty()) {
                triedLocationsString = QDir::toNativeSeparators(triedLocation);
            } else {
#ifdef QT_ONLY
                triedLocationsString += QString::fromLatin1(", %1").arg(
                    QDir::toNativeSeparators(triedLocation));
#else
                triedLocationsString = xi18n("%1, %2", triedLocationsString,
                                             QDir::toNativeSeparators(triedLocation));
#endif
            }
        }
#ifdef QT_ONLY
        *errorMessage = QString("Could not open icon resource file %1.").arg(path);
        *detailsErrorMessage = QString("Tried to find in %1.").arg(triedLocationsString);
#else
        *errorMessage = kxi18nc("@info",
            "<para>Could not open icon resource file <filename>%1</filename>.</para>"
            "<para><application>Kexi</application> will not start. "
            "Please check if <application>Kexi</application> is properly installed.</para>")
            .subs(QFileInfo(path).fileName());
        *detailsErrorMessage = kxi18nc("@info Tried to find files in <dir list>",
                                       "Tried to find in %1.").subs(triedLocationsString);
#endif
        return false;
    }
    *errorMessage = KLocalizedString();
    *detailsErrorMessage = KLocalizedString();
    return true;
}

static bool registerGlobalBreezeIconsResource(KLocalizedString *errorMessage,
                                              KLocalizedString *detailsErrorMessage)
{
    QString extraLocation;
#ifdef CMAKE_INSTALL_FULL_ICONDIR
    extraLocation = QDir::fromNativeSeparators(QFile::decodeName(CMAKE_INSTALL_FULL_ICONDIR));
    if (extraLocation.endsWith("/icons")) {
        extraLocation.chop(QLatin1String("/icons").size());
    }
#elif defined(Q_OS_WIN)
    extraLocation = QCoreApplication::applicationDirPath() + QStringLiteral("/data");
#endif
    return registerResource("icons/breeze/breeze-icons.rcc", QStandardPaths::GenericDataLocation,
                            QStringLiteral("/icons/breeze"), extraLocation, errorMessage,
                            detailsErrorMessage);
}

//! Tell Qt about the theme
static void setupBreezeIconTheme()
{
    QIcon::setThemeSearchPaths(QStringList() << QStringLiteral(":/icons"));
    QIcon::setThemeName(QStringLiteral("breeze"));
}
