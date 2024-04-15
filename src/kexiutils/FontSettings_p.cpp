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

#include "FontSettings_p.h"
#include <KConfigGroup>

FontSettingsData::FontSettingsData()
{
    for (int i = 0; i < FontTypesCount; ++i) {
        m_fonts[i] = 0;
    }
}

FontSettingsData::~FontSettingsData()
{
    for (int i = 0; i < FontTypesCount; ++i) {
        delete m_fonts[i];
    }
}

static const char GeneralId[] =      "General";
static const char DefaultFont[] =    "Noto Sans";

static const FontData DefaultFontData[FontSettingsData::FontTypesCount] = {
    { GeneralId, "font",                 DefaultFont,  10, -1, QFont::SansSerif },
    { GeneralId, "fixed",                "Oxygen Mono",  9, -1, QFont::Monospace },
    { GeneralId, "toolBarFont",          DefaultFont,  9, -1, QFont::SansSerif },
    { GeneralId, "menuFont",             DefaultFont,  10, -1, QFont::SansSerif },
    { "WM",      "activeFont",           DefaultFont,  10, -1, QFont::SansSerif },
    { GeneralId, "taskbarFont",          DefaultFont,  10, -1, QFont::SansSerif },
    { GeneralId, "smallestReadableFont", DefaultFont,  8, -1, QFont::SansSerif }
};

QFont FontSettingsData::font(FontTypes fontType)
{
    QFont *cachedFont = m_fonts[fontType];
    if (!cachedFont) {
        const FontData &fontData = DefaultFontData[fontType];
        cachedFont = new QFont(QString::fromLatin1(fontData.FontName), fontData.Size, fontData.Weight);
        cachedFont->setStyleHint(fontData.StyleHint);
        if (!m_kdeGlobals) {
            m_kdeGlobals = KSharedConfig::openConfig(QStringLiteral("kdeglobals"), KConfig::NoGlobals);
        }
        const KConfigGroup configGroup(m_kdeGlobals, fontData.ConfigGroupKey);
        QString fontInfo = configGroup.readEntry(fontData.ConfigKey, QString());

        //If we have serialized information for this font, restore it
        //NOTE: We are not using KConfig directly because we can't call QFont::QFont from here
        if (!fontInfo.isEmpty()) {
            cachedFont->fromString(fontInfo);
        }
        m_fonts[fontType] = cachedFont;
    }
    return *cachedFont;
}
