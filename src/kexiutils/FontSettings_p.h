/* This file is part of the KDE project
   Copyright (C) 2003-2016 Jarosław Staniek <staniek@kde.org>

   Contains code from kfontsettingsdata.cpp:
   Copyright (C) 2000, 2006 David Faure <faure@kde.org>
   Copyright 2008 Friedrich W. H. Kossebau <kossebau@kde.org>
   Copyright 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef KEXIUTILS_FONTSETTINGS_P_H
#define KEXIUTILS_FONTSETTINGS_P_H

#include <QFont>
#include <KSharedConfig>

//! @internal For KexiUtils::smallestReadableFont(), etc.
struct FontData {
    const char *ConfigGroupKey;
    const char *ConfigKey;
    const char *FontName;
    int Size;
    int Weight;
    QFont::StyleHint StyleHint;
};

//! @internal For KexiUtils::smallestReadableFont(), etc.
class FontSettingsData // : public QObject
{
    //Q_OBJECT
public:
    // if adding a new type here also add an entry to DefaultFontData
    enum FontTypes {
        GeneralFont = 0,
        FixedFont,
        ToolbarFont,
        MenuFont,
        WindowTitleFont,
        TaskbarFont,
        SmallestReadableFont,
        FontTypesCount
    };

public:
    FontSettingsData();
    ~FontSettingsData();

//public Q_SLOTS:
//    void dropFontSettingsCache();

//private Q_SLOTS:
//    void delayedDBusConnects();

public: // access, is not const due to caching
    QFont font(FontTypes fontType);

private:
    QFont *m_fonts[FontTypesCount];
    KSharedConfigPtr m_kdeGlobals;
};

#endif
