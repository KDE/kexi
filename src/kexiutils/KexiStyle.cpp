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

#include "KexiStyle.h"
#include "utils.h"
#include <QFrame>

namespace KexiStyle {

KEXIUTILS_EXPORT void setupFrame(QFrame *frame)
{
    frame->setFrameStyle(QFrame::NoFrame);
}

KEXIUTILS_EXPORT void setupModeSelector(QFrame *selector)
{
    KexiStyle::setupFrame(selector);
    selector->setFont(KexiUtils::smallestReadableFont());
    selector->setPalette(KexiStyle::alternativePalette(selector->palette()));
}

KEXIUTILS_EXPORT QPalette alternativePalette(const QPalette &palette)
{
    QPalette p(palette);
    p.setColor(QPalette::Window, KexiUtils::charcoalGrey());
    p.setColor(QPalette::Base, KexiUtils::shadeBlack());
    p.setColor(QPalette::Button, KexiUtils::shadeBlack());
    p.setColor(QPalette::AlternateBase, KexiUtils::shadeBlackLighter());
    p.setColor(QPalette::WindowText, KexiUtils::paperWhite());
    p.setColor(QPalette::ButtonText, KexiUtils::paperWhite());
    p.setColor(QPalette::Text, KexiUtils::paperWhite());
    return p;
}

KEXIUTILS_EXPORT QPalette sidebarsPalette(const QPalette &palette)
{
    return alternativePalette(palette);
}

KEXIUTILS_EXPORT void setSidebarsPalette(QWidget *widget)
{
    widget->setPalette(sidebarsPalette(widget->palette()));
    widget->setAutoFillBackground(true);
}

KEXIUTILS_EXPORT QFont titleFont(const QFont &font)
{
    QFont newFont(font);
    newFont.setCapitalization(QFont::AllUppercase);
    return newFont;
}

}
