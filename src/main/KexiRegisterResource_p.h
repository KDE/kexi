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

#include <QFileInfo>
#include <QResource>
#include <QStandardPaths>
#include <QDebug>

#ifdef QT_ONLY
#define KLocalizedString QString
#endif

static bool registerResource(const QString& path, QStandardPaths::StandardLocation location,
                             const QString &resourceRoot, KLocalizedString *errorMessage)
{
    // let QStandardPaths handle this, it will look for app local stuff
    const QString fullPath = QFileInfo(
        QStandardPaths::locate(location, path)).canonicalFilePath();
    if (fullPath.isEmpty() || !QFileInfo(fullPath).isReadable()
        || !QResource::registerResource(fullPath, resourceRoot))
    {
#ifdef QT_ONLY
        *errorMessage = QString("Could not open icon resource file %1.").arg(path);
#else
        *errorMessage = kxi18nc("@info",
            "<para>Could not open icon resource file <filename>%1</filename>.</para>"
            "<para><application>Kexi</application> will not start. "
            "Please check if <application>Kexi</application> is properly installed.</para>")
            .subs(QFileInfo(path).fileName());
#endif
        return false;
    }
    *errorMessage = KLocalizedString();
    return true;
}

static bool registerGlobalBreezeIconsResource(KLocalizedString *errorMessage)
{
    return registerResource("icons/breeze/breeze-icons.rcc", QStandardPaths::GenericDataLocation,
                            QStringLiteral("/icons/breeze"), errorMessage);
}

//! Tell Qt about the theme
static void setupBreezeIconTheme()
{
    QIcon::setThemeSearchPaths(QStringList() << QStringLiteral(":/icons"));
    QIcon::setThemeName(QStringLiteral("breeze"));
}
