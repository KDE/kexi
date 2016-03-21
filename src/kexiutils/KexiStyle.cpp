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
#include <QModelIndex>
#include <QPainter>
#include <QStyleOptionViewItem>

namespace KexiStyle {

KEXIUTILS_EXPORT void setupFrame(QFrame *frame)
{
    if (frame) {
        frame->setFrameStyle(QFrame::NoFrame);
    }
}

KEXIUTILS_EXPORT void setupModeSelector(QFrame *selector)
{
    KexiStyle::setupFrame(selector);
    selector->setFont(KexiUtils::smallestReadableFont());
    selector->setPalette(KexiStyle::alternativePalette(selector->palette()));
}

KEXIUTILS_EXPORT void overpaintModeSelector(QWidget *widget, QPainter *painter,
                                            const QRect &selectedRect)
{
    // draw gradient
    painter->save();
    int w = widget->fontMetrics().height() * 3 / 2;
    painter->translate(widget->width() - w, 0);
    QLinearGradient grad(0, 0, w, 0);
    QColor c(widget->palette().base().color());
    c.setAlpha(0);
    grad.setColorAt(0, c);
    c.setAlpha(15);
    grad.setColorAt(0.5, c);
    grad.setColorAt(1.0, QColor(0, 0, 0, 50));
    painter->fillRect(0, 0, w, widget->height(), QBrush(grad));
    painter->restore();

    // draw: /|
    //       \|
    if (!selectedRect.isNull()) {
        painter->save();
        w = selectedRect.height() / 10;
        if (w % 2 == 0) {
            ++w;
        }
        painter->translate(selectedRect.x() + selectedRect.width() - w,
                           selectedRect.y() + (selectedRect.height() - w * 2) / 2 - 0.5);
        QPolygon polygon;
        polygon << QPoint(w, 0) << QPoint(w, w * 2) << QPoint(0, w);
        painter->setPen(QPen(Qt::NoPen));
        painter->setBrush(KexiUtils::charcoalGrey());
        painter->drawPolygon(polygon);
        painter->restore();
    }
}

KEXIUTILS_EXPORT void overpaintModeSelectorItem(QPainter *painter,
                                                const QStyleOptionViewItem &option,
                                                const QModelIndex &index)
{
    Q_UNUSED(painter)
    Q_UNUSED(option)
    Q_UNUSED(index)
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
