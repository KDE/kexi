/* This file is part of the KDE project
   Copyright (C) 2005-2006 Jarosław Staniek <staniek@kde.org>

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

#include "kexidisplayutils.h"
#include <kexiutils/utils.h>

#include <QPixmap>
#include <QPainter>
#include <QImage>
#include <QWidget>

#include <KColorScheme>
#include <KLocalizedString>

//! A color for displaying default values or autonumbers
#define SPECIAL_TEXT_COLOR KColorScheme(QPalette::Active, KColorScheme::View).foreground(KColorScheme::LinkText).color()

Q_GLOBAL_STATIC(QPixmap, KexiDisplayUtils_autonum)

static void initDisplayUtilsImages()
{
    /*! @warning not reentrant! */
    QImage autonum(":/kexi-autonumber");
    KexiUtils::replaceColors(&autonum, SPECIAL_TEXT_COLOR);
    *KexiDisplayUtils_autonum = QPixmap::fromImage(autonum);
}

//-----------------

KexiDisplayUtils::DisplayParameters::DisplayParameters()
{
}

KexiDisplayUtils::DisplayParameters::DisplayParameters(const QWidget *w)
{
//! @todo check!
    textColor = w->palette().foreground().color();

    selectedTextColor = w->palette().highlightedText().color();
    font = w->font();
}

static QString autonumberText()
{
    return xi18nc("Autonumber, make it as short as possible", "(auto)");
}

void KexiDisplayUtils::initDisplayForAutonumberSign(DisplayParameters* par, const QWidget *widget)
{
    Q_ASSERT(par);
    initDisplayUtilsImages();

    par->textColor = SPECIAL_TEXT_COLOR;
    par->selectedTextColor = SPECIAL_TEXT_COLOR; //hmm, unused anyway
    par->font = widget->font();
    par->font.setItalic(true);
    QFontMetrics fm(par->font);
    par->textWidth = fm.width(autonumberText());
    par->textHeight = fm.height();
}

void KexiDisplayUtils::initDisplayForDefaultValue(DisplayParameters* par, const QWidget *widget)
{
    par->textColor = SPECIAL_TEXT_COLOR;
//! @todo check!
    par->selectedTextColor = widget->palette().highlightedText().color();
    par->font = widget->font();
    par->font.setItalic(true);
}

void KexiDisplayUtils::paintAutonumberSign(const DisplayParameters& par, QPainter* painter,
        int x, int y, int width, int height, Qt::Alignment alignment, bool overrideColor)
{
    painter->save();

    painter->setFont(par.font);
    if (!overrideColor)
        painter->setPen(par.textColor);

    if (!(alignment & Qt::AlignVertical_Mask))
        alignment |= Qt::AlignVCenter;
    if (!(alignment & Qt::AlignHorizontal_Mask))
        alignment |= Qt::AlignLeft;

    int y_pixmap_pos = 0;
    if (alignment & Qt::AlignVCenter) {
        y_pixmap_pos = qMax(0, y + 1 + (height - KexiDisplayUtils_autonum->height()) / 2);
    } else if (alignment & Qt::AlignTop) {
        y_pixmap_pos = y + qMax(0, (par.textHeight - KexiDisplayUtils_autonum->height()) / 2);
    } else if (alignment & Qt::AlignBottom) {
        y_pixmap_pos = y + 1 + height - KexiDisplayUtils_autonum->height()
                       - qMax(0, (par.textHeight - KexiDisplayUtils_autonum->height()) / 2);
    }

    if (alignment & (Qt::AlignLeft | Qt::AlignJustify)) {
        if (!overrideColor) {
            painter->drawPixmap(x, y_pixmap_pos, *KexiDisplayUtils_autonum);
            x += (KexiDisplayUtils_autonum->width() + 4);
        }
    } else if (alignment & Qt::AlignRight) {
        if (!overrideColor) {
            painter->drawPixmap(x + width - par.textWidth - KexiDisplayUtils_autonum->width() - 4,
                                y_pixmap_pos, *KexiDisplayUtils_autonum);
        }
    } else if (alignment & Qt::AlignCenter) {
        //! @todo
        if (!overrideColor)
            painter->drawPixmap(x + (width - par.textWidth) / 2 - KexiDisplayUtils_autonum->width() - 4,
                                y_pixmap_pos, *KexiDisplayUtils_autonum);
    }
    painter->drawText(x, y, width, height, alignment, autonumberText());
    painter->restore();
}

