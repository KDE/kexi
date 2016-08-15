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

#include "AlterSchemaTableModel.h"

#include <KDbTableSchema>

#include <QDebug>

const int RECORDS_FOR_PREVIEW = 3;

AlterSchemaTableModel::AlterSchemaTableModel(QObject* parent)
    : QAbstractTableModel(parent)
    , m_schema(nullptr)
    , m_data(nullptr)
    , m_recordCount(RECORDS_FOR_PREVIEW)
{
}

AlterSchemaTableModel::~AlterSchemaTableModel()
{
    delete m_data;
}

QVariant AlterSchemaTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.column() >= (int)m_schema->fieldCount())
        return QVariant();

    if (role == Qt::DisplayRole) {
        if (m_data->length() > index.row()) {
            const KDbRecordData* r(m_data->value(index.row()));
            return r->value(index.column());
        }
    }
    return QVariant();
}

QVariant AlterSchemaTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        if (m_schema) {
            KDbField *fld = m_schema->field(section);
            if (fld)
                return m_schema->field(section)->captionOrName();
        }
        return QString("Column %1").arg(section);
    }
    return QString("Record %1").arg(section + 1);
}

int AlterSchemaTableModel::columnCount ( const QModelIndex& parent ) const
{
    Q_UNUSED(parent);
    if (m_schema) {
        return m_schema->fieldCount();
    }
    return 0;
}

int AlterSchemaTableModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return m_recordCount;
}

void AlterSchemaTableModel::setSchema(KDbTableSchema *schema)
{
    m_schema = schema;
    if (!m_schema) {
        return;
    }
    beginInsertColumns(QModelIndex(), 0, m_schema->fieldCount() - 1);
    endInsertColumns();

    emit layoutChanged();
}

void AlterSchemaTableModel::setData(QList<KDbRecordData*> *data)
{
    m_data = data;
}

void AlterSchemaTableModel::setRowCount(int i)
{
    if (i != m_recordCount) {
        m_recordCount = i;
        emit layoutChanged();
    }
}
