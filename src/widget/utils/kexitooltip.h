/* This file is part of the KDE project
   Copyright (C) 2006 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXITOOLTIP_H
#define KEXITOOLTIP_H

#include "kexiguiutils_export.h"
#include <kexi_global.h>

#include <QWidget>
#include <QVariant>

//! \brief A tooltip that can display rich content
class KEXIGUIUTILS_EXPORT KexiToolTip : public QWidget
{
    Q_OBJECT
public:
    explicit KexiToolTip(const QVariant& value, QWidget* parent = nullptr);
    virtual ~KexiToolTip();

    virtual QSize sizeHint() const override;

public Q_SLOTS:
    virtual void show();

protected:
    virtual void paintEvent(QPaintEvent *pev) override;
    virtual void drawFrame(QPainter& p);
    virtual void drawContents(QPainter& p);

private:
    class Private;
    Private * const d;
};

#endif
