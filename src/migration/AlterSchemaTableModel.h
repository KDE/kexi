/* This file is part of the KDE project
   Copyright (C) 2009 Adam Pigg <adam@piggz.co.uk>
   Copyright (C) 2009-2016 Jarosław Staniek <staniek@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef ALTERSCHEMATABLEMODEL_H
#define ALTERSCHEMATABLEMODEL_H

#include <QModelIndex>
#include <QList>

#include <KDbRecordData>

class KDbTableSchema;

class AlterSchemaTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit AlterSchemaTableModel(QObject* parent = nullptr);
    ~AlterSchemaTableModel();

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    void setSchema(KDbTableSchema *schema);
    void setData(QList<KDbRecordData*> *data);
    void setRowCount(int i);
private:
    //! Reimplemented just to avoid 'hidden' warnings
    bool setData(const QModelIndex & index, const QVariant & value,
                 int role = Qt::EditRole) override
    {
        return QAbstractTableModel::setData(index, value, role);
    }

    KDbTableSchema *m_schema;
    QList<KDbRecordData*> *m_data; //!< Small amount of data to display to user
    int m_recordCount;
};

#endif // ALTERSCHEMATABLEMODEL_H
