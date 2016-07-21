/* This file is part of the KDE project
   Copyright (C) 2016 Jarosław Staniek <staniek@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#define QT_ONLY
#include <QApplication>
#include <QIcon>
#include <iostream>
#include "../../src/main/KexiRegisterResource_p.h"

//! @internal runtime check for existence of breeze icons
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QString errorMessage;
    const char *file
    = "The icon resource is invalid, please build repository "
      "from https://quickgit.kde.org/?p=breeze-icons.git (with -DBINARY_ICONS_RESOURCE=ON) "
      "or install \"breeze-icons package\" that delivers the file.\nIssue: ";
    if (!registerGlobalBreezeIconsResource(&errorMessage)) {
        std::cerr << file << " " << errorMessage.toStdString() << std::endl;
        return 1;
    }
    setupBreezeIconTheme();
    const char *iconName = "document-save-as";
    const QIcon icon(QIcon::fromTheme(iconName));
    if (icon.themeName() != "breeze") {
        std::cerr << file << "resource's theme is \"" << icon.themeName().toStdString()
                  << "\", expected: \"breeze\"" << std::endl;
        return 1;
    }
    QString sizes;
    for(const QSize &size : icon.availableSizes()) {
        sizes += QString::number(size.width()) + ',';
    }
    sizes.chop(1);
    const char *expectedSizes = "16,22,24,32";
    if (sizes != expectedSizes) {
        std::cerr << file << "\"" << iconName << "\" icon's available sizes are \"" << sizes.toStdString()
                  << "\", expected \"" << expectedSizes << "\"" << std::endl;
        return 1;
    }
    return 0;
}
