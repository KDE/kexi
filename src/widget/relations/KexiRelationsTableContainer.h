/* This file is part of the KDE project
   Copyright (C) 2002, 2003 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2003-2007 Jarosław Staniek <staniek@kde.org>

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

#ifndef KexiRelationsTableContainer_H
#define KexiRelationsTableContainer_H

#include "kexirelationsview_export.h"

#include <widget/fields/KexiFieldListView.h>

#include <QFrame>
#include <QStringList>

class KexiRelationsScrollArea;
class KexiRelationViewTable;
class KexiRelationViewTableContainerHeader;

namespace KexiDB
{
class TableOrQuerySchema;
}

//! @short Provides a frame displaying single table or query in relation view.
class KEXIRELATIONSVIEW_EXPORT KexiRelationsTableContainer : public QFrame
{
    Q_OBJECT

public:
    KexiRelationsTableContainer(
        QWidget* parent,
        KexiRelationsScrollArea *scrollArea,
        KexiDB::TableOrQuerySchema *schema);

    virtual ~KexiRelationsTableContainer();

    int globalY(const QString &field);

    KexiDB::TableOrQuerySchema* schema() const;

    int right() const {
        return x() + width() - 1;
    }

    int bottom() const {
        return y() + height() - 1;
    }

    /*! \return list of selected field names. */
    QStringList selectedFieldNames() const;

Q_SIGNALS:
    void moved(KexiRelationsTableContainer *);
    void endDrag();
    void gotFocus();
    void contextMenuRequest(const QPoint& pos);
    void fieldsDoubleClicked(KexiDB::TableOrQuerySchema& tableOrQuery, const QStringList& fieldNames);

public Q_SLOTS:
    void setFocus();
    void unsetFocus();

protected Q_SLOTS:
    void moved();
    void slotContextMenu(const QPoint& p);
    void slotFieldsDoubleClicked(const QModelIndex &idx);

    friend class KexiRelationViewTableContainerHeader;

protected:
    virtual void focusInEvent(QFocusEvent* event);
    virtual void focusOutEvent(QFocusEvent* event);

private:
    class Private;
    Private* const d;
};

#endif
