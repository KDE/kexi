/* This file is part of the KDE project
   Copyright (C) 2003   Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2003   Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2003-2014 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIDATATABLESCROLLAREA_H
#define KEXIDATATABLESCROLLAREA_H

#include "KexiTableScrollArea.h"

class KDbCursor;

/**
 * Database-aware table widget.
 */
class KEXIDATATABLE_EXPORT KexiDataTableScrollArea : public KexiTableScrollArea
{
    Q_OBJECT

public:
    /**
     * Creates a blank widget
     */
    explicit KexiDataTableScrollArea(QWidget *parent);

    /*! Creates a table widget and fills it using data from \a cursor.
     Cursor will be opened (with open()) if it is not yet opened.
     Cursor must be defined on query schema, not raw statement (see KDbConnection::prepareQuery()
     and KDbConnection::executeQuery()), otherwise the table view remain not filled with data.
     Cursor \a cursor will not be owned by this object.
     */
    KexiDataTableScrollArea(QWidget *parent, KDbCursor *cursor);

    ~KexiDataTableScrollArea();

    using KexiTableScrollArea::setData;

    /*! Fills table view with data using \a cursor. \return true on success.
     Cursor \a cursor will not be owned by this object. */
    bool setData(KDbCursor *cursor);

    /*! \return cursor used as data source for this table view,
     or NULL if no valid cursor is defined. */
    KDbCursor *cursor() {
        return m_cursor;
    }

    /**
     * @returns the number of records in the data set, (if data set is present)
     * @note not all of the records have to be processed
     */
    virtual int recordCount() const override {
        return m_data ? m_data->count() : 0;
    }

#ifdef KEXI_TABLE_PRINT_SUPPORT
//  virtual void print(KPrinter &printer);
#endif

protected:
    void init();

private:
    //db stuff
    KDbCursor *m_cursor;
};

#endif
