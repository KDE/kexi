/* This file is part of the KDE project
   Copyright (C) 2012 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXILINKBUTTON_H
#define KEXILINKBUTTON_H

#include "kexiutils_export.h"

#include <QPushButton>

class KGuiItem;

//! A flat icon-based button without background that behaves like a link
class KEXIUTILS_EXPORT KexiLinkButton : public QPushButton
{
    Q_OBJECT
public:
    explicit KexiLinkButton(QWidget* parent = nullptr);

    explicit KexiLinkButton(const QIcon &icon, QWidget* parent = nullptr);

    explicit KexiLinkButton(const KGuiItem &item, QWidget *parent = 0);

    explicit KexiLinkButton(const QPixmap &pixmap, QWidget* parent = nullptr);

    virtual ~KexiLinkButton();

    /*! If true, foreground color of the current palette is always used for painting
        the button's icon. This is done by replacing color.
        The foreground color is QPalette::Text by default, and can be changed
        using setForegroundRole().
        The icon is expected to be monochrome.
        Works well also after palette change.
        False by default. */
    void setUsesForegroundColor(bool set);

    /*! @return true if foreground color of the current palette is always used for painting
        the button's icon. */
    bool usesForegroundColor() const;

    void setIcon(const QIcon &icon);

protected:
    virtual void changeEvent(QEvent* event) override;

private:
    void init();
    void updateIcon(const QIcon &icon);

    class Private;
    Private * const d;
};

#endif
