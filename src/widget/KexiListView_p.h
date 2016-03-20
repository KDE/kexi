/* This file is part of the KDE project

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

#include <QAbstractItemDelegate>

class KPageListViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT

public:
    KPageListViewDelegate(QObject *parent = 0);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;

private:
    void drawFocus(QPainter *, const QStyleOptionViewItem &, const QRect &) const;
};

class SelectionModel : public QItemSelectionModel
{
    Q_OBJECT

public:
    SelectionModel(QAbstractItemModel *model, QObject *parent);

public Q_SLOTS:
    void clear() Q_DECL_OVERRIDE;
    void select(const QModelIndex &index, QItemSelectionModel::SelectionFlags command) Q_DECL_OVERRIDE;
    void select(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command) Q_DECL_OVERRIDE;
};
