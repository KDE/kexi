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
#else
#include <KConfigGroup>
#include <KSharedConfig>
#include <KMessageBox>
#endif

//! @todo Support other themes
const QString supportedIconTheme = QLatin1String("breeze");

//! @return true if @a path is readable
static bool fileReadable(const QString &path)
{
    return !path.isEmpty() && QFileInfo(path).isReadable();
}

#ifdef Q_OS_WIN
#define KPATH_SEPARATOR ';'
#else
#define KPATH_SEPARATOR ':'
#endif

//! @brief Used for a workaround: locations for QStandardPaths::AppDataLocation end with app name.
//! If this is not an expected app but for example a test app, replace
//! the subdir name with app name so we can find resource file(s).
static QStringList correctStandardLocations(const QString &privateName,
                                     QStandardPaths::StandardLocation location,
                                     const QString &extraLocation)
{
    QStringList result;
    QStringList standardLocations(QStandardPaths::standardLocations(location));
    if (!extraLocation.isEmpty()) {
        standardLocations.append(extraLocation);
    }
    if (privateName.isEmpty()) {
        result = standardLocations;
    } else {
        QRegularExpression re(QLatin1Char('/') + QCoreApplication::applicationName() + QLatin1Char('$'));
        for(const QString &dir : standardLocations) {
            if (dir.indexOf(re) != -1) {
                QString realDir(dir);
                realDir.replace(re, QLatin1Char('/') + privateName);
                result.append(realDir);
            }
        }
    }
    return result;
}

static QString locateFile(const QString &privateName,
                          const QString& path, QStandardPaths::StandardLocation location,
                          const QString &extraLocation, QStringList *triedLocations)
{
    Q_ASSERT(triedLocations);
    const QString subdirPath = QFileInfo(path).dir().path();
    {
        // Priority #1: This makes the app portable and working without installation, from the build dir
        const QString dataDir = QCoreApplication::applicationDirPath() + QStringLiteral("/data/") + privateName;
        triedLocations->append(QDir::cleanPath(dataDir + '/' + subdirPath));
        const QString dataFile = QFileInfo(dataDir + '/' + path).canonicalFilePath();
        if (fileReadable(dataFile)) {
            return dataFile;
        }
    }
    // Priority #2: Let QStandardPaths handle this, it will look for app local stuff
    const QStringList correctedStandardLocations(correctStandardLocations(privateName, location, extraLocation));
    for (const QString &dir : correctedStandardLocations) {
        triedLocations->append(QDir::cleanPath(dir + '/' + subdirPath));
        const QString dataFile = QFileInfo(dir + QLatin1Char('/') + path).canonicalFilePath();
        if (fileReadable(dataFile)) {
            return dataFile;
        }
    }
    // Priority #3: Try in PATH subdirs, useful for running apps from the build dir, without installing
    for(const QByteArray &pathDir : qgetenv("PATH").split(KPATH_SEPARATOR)) {
        const QString dataDir = QFile::decodeName(pathDir) + QStringLiteral("/data/");
        triedLocations->append(QDir::cleanPath(dataDir + '/' + subdirPath));
        const QString dataDirFromPath = QFileInfo(dataDir + '/' + path).canonicalFilePath();
        if (fileReadable(dataDirFromPath)) {
            return dataDirFromPath;
        }
    }
    return QString();
}

#ifndef KEXI_SKIP_REGISTERRESOURCE

#ifdef KEXI_BASE_PATH
const QString BASE_PATH(KEXI_BASE_PATH);
#else
const QString BASE_PATH(QCoreApplication::applicationName());
#endif

static bool registerResource(const QString& path, QStandardPaths::StandardLocation location,
                             const QString &resourceRoot, const QString &extraLocation,
                             KLocalizedString *errorMessage, KLocalizedString *detailsErrorMessage)
{
    QStringList triedLocations;
    const QString privateName = location == QStandardPaths::GenericDataLocation
        ? QString() : BASE_PATH;
    const QString fullPath = locateFile(privateName, path, location, extraLocation, &triedLocations);
    if (fullPath.isEmpty()
        || !QResource::registerResource(fullPath, resourceRoot))
    {
        const QString triedLocationsString = QLocale().createSeparatedList(triedLocations);
#ifdef QT_ONLY
        *errorMessage = QString("Could not open icon resource file %1.").arg(path);
        *detailsErrorMessage = QString("Tried to find in %1.").arg(triedLocationsString);
#else
        *errorMessage = kxi18nc("@info",
            "<para>Could not open icon resource file <filename>%1</filename>.</para>"
            "<para><application>%2</application> will not start. "
            "Please check if it is properly installed.</para>")
            .subs(QFileInfo(path).fileName()).subs(QApplication::applicationDisplayName());
        *detailsErrorMessage = kxi18nc("@info Tried to find files in <dir list>",
                                       "Tried to find in %1.").subs(triedLocationsString);
#endif
        return false;
    }
    *errorMessage = KLocalizedString();
    *detailsErrorMessage = KLocalizedString();
    return true;
}
#endif // !KEXI_SKIP_REGISTERRESOURCE

#ifndef KEXI_SKIP_SETUPBREEZEICONTHEME

inline bool registerGlobalBreezeIconsResource(KLocalizedString *errorMessage,
                                              KLocalizedString *detailsErrorMessage)
{
    QString extraLocation;
#ifdef CMAKE_INSTALL_FULL_ICONDIR
    extraLocation = QDir::fromNativeSeparators(QFile::decodeName(CMAKE_INSTALL_FULL_ICONDIR));
    if (extraLocation.endsWith("/icons")) {
        extraLocation.chop(QLatin1String("/icons").size());
    }
#endif
    return registerResource("icons/breeze/breeze-icons.rcc", QStandardPaths::GenericDataLocation,
                            QStringLiteral("/icons/breeze"), extraLocation, errorMessage,
                            detailsErrorMessage);
}

//! Tell Qt about the theme
inline void setupBreezeIconTheme()
{
#ifdef QT_GUI_LIB
    QIcon::setThemeSearchPaths(QStringList() << QStringLiteral(":/icons"));
    QIcon::setThemeName(QStringLiteral("breeze"));
#endif
}
#endif // !KEXI_SETUPBREEZEICONTHEME

#ifndef KEXI_SKIP_REGISTERICONSRESOURCE
/*! @brief Registers icons resource file
 * @param privateName Name to be used instead of application name for resource lookup
 * @param path Relative path to the resource file
 * @param location Standard file location to use for file lookup
 * @param resourceRoot A resource root for QResource::registerResource()
 * @param extraLocation Extra location
 * @param errorMessage On failure it is set to a brief error message.
 * @param detailedErrorMessage On failure it is set to a detailed error message.
 * other for warning
 */
static bool registerIconsResource(const QString &privateName, const QString& path,
                             QStandardPaths::StandardLocation location,
                             const QString &resourceRoot, const QString &extraLocation,
                             QString *errorMessage, QString *detailedErrorMessage)
{
    QStringList triedLocations;
    const QString fullPath = locateFile(privateName, path, location, extraLocation, &triedLocations);
    if (fullPath.isEmpty() || !QFileInfo(fullPath).isReadable()
        || !QResource::registerResource(fullPath, resourceRoot))
    {
        const QString triedLocationsString = QLocale().createSeparatedList(triedLocations);
#ifdef QT_ONLY
        *errorMessage
            = QString("Could not open icon resource file \"%1\". Please check if application "
                      "is properly installed.").arg(path);
        *detailedErrorMessage = QString("Tried to find in %1.").arg(triedLocationsString);
#else
        *errorMessage = xi18nc(
            "@info", "Could not open icon resource file <filename>%1</filename>. "
                     "Please check if <application>%2</application> is properly installed.",
            QFileInfo(path).fileName(), QApplication::applicationDisplayName());
        *detailedErrorMessage = xi18nc("@info Tried to find files in <dir list>",
                                       "Tried to find in %1.", triedLocationsString);
#endif
        return false;
    }
    *errorMessage = QString();
    *detailedErrorMessage = QString();
    return true;
}
#endif // !KEXI_SKIP_SETUPBREEZEICONTHEME

#if !defined QT_ONLY  && !defined KEXI_SKIP_SETUPPRIVATEICONSRESOURCE
/*! @brief Sets up a private icon resource file
 * @return @c false on failure and sets error message. Does not warn or exit on failure.
 * @param privateName Name to be used instead of application name for resource lookup
 * @param path Relative path to the resource file
 * @param themeName Icon theme to use. It affects filename.
 * @param errorMessage On failure it is set to a brief error message
 * @param detailedErrorMessage On failure it is set to a detailed error message
 * other for warning
 * @param prefix Resource path prefix. The default is useful for library-global resource,
 * other values is useful for plugins.
 */
static bool setupPrivateIconsResource(const QString &privateName, const QString& path,
                               const QString &themeName,
                               QString *errorMessage, QString *detailedErrorMessage,
                               const QString &prefix = QLatin1String(":/icons"))
{
    // Register application's resource first to have priority over the theme.
    // Some icons may exists in both resources.
    if (!registerIconsResource(privateName, path,
                          QStandardPaths::AppDataLocation,
                          QString(), QString(), errorMessage, detailedErrorMessage))
    {
        return false;
    }
    bool changeTheme = false;
#ifdef QT_GUI_LIB
    QIcon::setThemeSearchPaths(QStringList() << prefix << QIcon::themeSearchPaths());
    changeTheme = 0 != QIcon::themeName().compare(themeName, Qt::CaseInsensitive);
    if (changeTheme) {
        QIcon::setThemeName(themeName);
    }
#endif

    KConfigGroup cg(KSharedConfig::openConfig(), "Icons");
    changeTheme = changeTheme || 0 != cg.readEntry("Theme", QString()).compare(themeName, Qt::CaseInsensitive);
    // tell KIconLoader an co. about the theme
    if (changeTheme) {
        cg.writeEntry("Theme", themeName);
        cg.sync();
    }
    return true;
}

/*! @brief Sets up a private icon resource file
 * @return @c false on failure and sets error message.
 * @param privateName Name to be used instead of application name for resource lookup
 * @param path Relative path to the resource file
 * @param themeName Icon theme to use. It affects filename.
 * @param errorMessage On failure it is set to a brief error message.
 * @param detailedErrorMessage On failure it is set to a detailed error message.
 * other for warning
 * @param prefix Resource path prefix. The default is useful for library-global resource,
 * other values is useful for plugins.
 */
static bool setupPrivateIconsResourceWithMessage(const QString &privateName, const QString& path,
                                          const QString &themeName,
                                          QString *errorMessage, QString *detailedErrorMessage,
                                          const QString &prefix = QLatin1String(":/icons"))
{
    if (!setupPrivateIconsResource(privateName, path, themeName,
                                   errorMessage, detailedErrorMessage, prefix))
    {
        if (detailedErrorMessage->isEmpty()) {
            KMessageBox::error(nullptr, *errorMessage);
        } else {
            KMessageBox::detailedError(nullptr, *errorMessage, *detailedErrorMessage);
        }
        return false;
    }
    return true;
}

/*! @overload setupPrivateIconsResourceWithMessage(QString &privateName, const QString& path,
                                          const QString &themeName,
                                          QString *errorMessage, QString *detailedErrorMessage,
                                          const QString &prefix = QLatin1String(":/icons"))
    Uses default theme name.
 */
static bool setupPrivateIconsResourceWithMessage(const QString &privateName, const QString& path,
                                          QString *errorMessage, QString *detailedErrorMessage,
                                          const QString &prefix = QLatin1String(":/icons"))
{
    return setupPrivateIconsResourceWithMessage(privateName, path, supportedIconTheme,
                                                errorMessage, detailedErrorMessage, prefix);
}

/*! @brief Sets up a private icon resource file
 * Warns on failure and returns @c false.
 * @param privateName Name to be used instead of application name for resource lookup
 * @param path Relative path to the resource file
 * @param messageType Type of message to use on error, QtFatalMsg for fatal exit and any
 * other for warning
 * @param prefix Resource path prefix. The default is useful for library-global resource,
 * other values is useful for plugins.
 */
static bool setupPrivateIconsResourceWithMessage(const QString &privateName, const QString& path,
                                          QtMsgType messageType,
                                          const QString &prefix = QLatin1String(":/icons"))
{
    QString errorMessage;
    QString detailedErrorMessage;
    if (!setupPrivateIconsResourceWithMessage(privateName, path,
                                              &errorMessage, &detailedErrorMessage, prefix)) {
        if (messageType == QtFatalMsg) {
            qFatal("%s %s", qPrintable(errorMessage), qPrintable(detailedErrorMessage));
        } else {
            qWarning() << qPrintable(errorMessage) << qPrintable(detailedErrorMessage);
        }
        return false;
    }
    return true;
}

#endif // !QT_ONLY && !KEXI_SKIP_SETUPPRIVATEICONSRESOURCE
