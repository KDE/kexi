/* This file is part of the KDE project
   Copyright (C) 2016 Jarosław Staniek <staniek@kde.org>

   Forked from kwidgetsaddons/src/kpageview_p.h:
   Copyright (C) 2006 Tobias Koenig (tokoe@kde.org)
   Copyright (C) 2007 Rafael Fernández López (ereslibre@kde.org)

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

#ifndef KEXILISTVIEW_H
#define KEXILISTVIEW_H

#include <QListView>

#include "kexiextwidgets_export.h"

class KEXIEXTWIDGETS_EXPORT KexiListView : public QListView
{
    Q_OBJECT

public:
    explicit KexiListView(QWidget *parent = 0);

    virtual ~KexiListView();

    void setModel(QAbstractItemModel *model) override;

private Q_SLOTS:
    void updateWidth();
};

#endif
