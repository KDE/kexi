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

#ifndef KEXIMODESELECTOR_P_H
#define KEXIMODESELECTOR_P_H

#include <widget/KexiListView.h>

class QPainter;

//! @internal A single global mode
class KexiModeData
{
public:
    KexiModeData() : enabled(true) {}
    QString name;
    QIcon icon;
    bool enabled;
};

//! @internal A model for KexiModeSelector, each item has name and icon
class KexiModeSelectorModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit KexiModeSelectorModel(QObject *parent = 0);

    ~KexiModeSelectorModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const  Q_DECL_OVERRIDE;

    QVariant data(const QModelIndex &index, int role) const  Q_DECL_OVERRIDE;

    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;

    QModelIndex index(int row, int column, const QModelIndex& parent) const Q_DECL_OVERRIDE;

    QList<KexiModeData*> modes;
};

//! @internal A delegate for items of KexiModeSelector
class KexiModeSelectorDelegate : public KexiListViewDelegate
{
    Q_OBJECT

public:
    explicit KexiModeSelectorDelegate(QObject *parent = 0);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const Q_DECL_OVERRIDE;
};

#endif // KEXIMODESELECTOR_P_H
