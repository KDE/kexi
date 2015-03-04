/* This file is part of the KDE project
   Copyright (C) 2004-2014 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXICOMBOBOXPOPUP_H
#define KEXICOMBOBOXPOPUP_H

#include <QFrame>

class KexiComboBoxPopupPrivate;
class KexiTableScrollArea;
namespace KexiDB
{
class Field;
class RecordData;
class TableViewColumn;
class TableViewData;
}
class QEvent;

//! Internal class for displaying popup table view
class KexiComboBoxPopup : public QFrame
{
    Q_OBJECT
public:
//! @todo js: more ctors!
    /*! Constructor for creating a popup using definition from \a column.
     If the column is lookup column, it's definition is used to display
     one or more column within the popup. Otherwise column.field() is used
     to display single-column data. */
    KexiComboBoxPopup(QWidget* parent, KexiDB::TableViewColumn &column);

    /*! Alternative constructor supporting lookup fields and enum hints. */
    KexiComboBoxPopup(QWidget* parent, KexiDB::Field &field);

    virtual ~KexiComboBoxPopup();

    KexiTableScrollArea* tableView();

    /*! Sets maximum number of rows for this popup. */
    void setMaxRows(int r);

    /*! \return maximum number of rows for this popup. */
    int maxRows() const;

    /*! Default maximum number of rows for KexiComboBoxPopup objects. */
    static const int defaultMaxRows;

Q_SIGNALS:
    void rowAccepted(KexiDB::RecordData *record, int row);
    void cancelled();
    void hidden();

public Q_SLOTS:
    virtual void resize(int w, int h);
    void updateSize(int minWidth = 0);

protected Q_SLOTS:
    void slotTVItemAccepted(KexiDB::RecordData *record, int row, int col);
    void slotDataReloadRequested();

protected:
    void init();

    virtual bool eventFilter(QObject *o, QEvent *e);

    //! The main function for setting data; data can be set either by passing \a column or \a field.
    //! The second case is used for lookup
    void setData(KexiDB::TableViewColumn *column, KexiDB::Field *field);

    //! used by setData()
    void setDataInternal(KexiDB::TableViewData *data, bool owner = true);   //!< helper

    KexiComboBoxPopupPrivate * const d;

    friend class KexiComboBoxTableEdit;
};

#endif

