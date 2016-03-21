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

#ifndef KEXISTYLE_H
#define KEXISTYLE_H

#include <kexiutils_export.h>

class QFont;
class QFrame;
class QPalette;
class QWidget;

//! Application style.
//! @todo make it configurable?
namespace KexiStyle
{
    //! Setup style for @a frame. By flat style is set (QFrame::NoFrame).
    KEXIUTILS_EXPORT void setupFrame(QFrame *frame);

    //! Setup style for the global mode selector widget (KexiModeSelector).
    //! By default setupFrame() is called to set flat style, minimal fonts are set
    //! and alternativePalette().
    KEXIUTILS_EXPORT void setupModeSelector(QFrame *selector);

    //! @return alternative palette based on @a palette.
    //! By default it is dark one based on Breeze colors.
    KEXIUTILS_EXPORT QPalette alternativePalette(const QPalette &palette);

    //! @return palette dedicated for side bars, based on @a palette.
    //! By default it equal to alternativePalette().
    KEXIUTILS_EXPORT QPalette sidebarsPalette(const QPalette &palette);

    //! Sets sidebarsPalette(). If it's nonstandard palette,
    //! QWidget::setAutoFillBackground(true) is also called for the widget.
    KEXIUTILS_EXPORT void setSidebarsPalette(QWidget *widget);

    //! @return font @a font adjusted to make it a title font.
    //! By default capitalization is set for the font.
    KEXIUTILS_EXPORT QFont titleFont(const QFont &font);
}

#endif // KEXISTYLE_H
