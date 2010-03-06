/*
* Kexi Report Plugin
* Copyright (C) 2007-2009 by Adam Pigg (adam@piggz.co.uk)
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "keximigratereportdata.h"
#include <kdebug.h>

//!Connect to an external data source
//!connStr is in the form driver|connection_string|table
KexiMigrateReportData::KexiMigrateReportData(const QString & connStr)
{
    QStringList extConn = connStr.split('|');

    if (extConn.size() == 3) {
        KexiMigration::MigrateManager mm;

        m_KexiMigrate = mm.driver(extConn[0]);
        KexiDB::ConnectionData cd;
        KexiMigration::Data dat;
        cd.setFileName(extConn[1]);
        dat.source = &cd;
        m_KexiMigrate->setData(&dat);
        m_valid = m_KexiMigrate->connectSource();
        QStringList names;

        if (m_valid) {
            m_valid = m_KexiMigrate->readTableSchema(extConn[2], m_TableSchema);
        }
        if (m_valid) {
            m_schema = new KexiDB::TableOrQuerySchema(m_TableSchema);
        }
        m_valid = m_KexiMigrate->tableNames(names);
        if (m_valid && names.contains(extConn[2])) {
            m_valid = m_KexiMigrate->readFromTable(extConn[2]);
        }
    }
}

KexiMigrateReportData::~KexiMigrateReportData()
{
    delete m_KexiMigrate;
    m_KexiMigrate = 0;
}

uint KexiMigrateReportData::fieldNumber(const QString &fld) const
{
    KexiDB::QueryColumnInfo::Vector flds;

    uint x = -1;

    if (m_schema) {
        flds = m_schema->columns();

        for (int i = 0; i < flds.size() ; ++i) {
            if (fld.toLower() == flds[i]->aliasOrName().toLower()) {
                x = i;
            }
        }
    }
    return x;
}

QStringList KexiMigrateReportData::fieldNames() const
{
    KexiDB::QueryColumnInfo::Vector flds;
    QStringList names;

    if (m_schema) {
        flds = m_schema->columns();

        for (int i = 0; i < flds.size() ; ++i) {
            names << flds[i]->field->name();
        }
    }
    return names;
}

QVariant KexiMigrateReportData::value(unsigned int i) const
{
    if (!m_valid)
        return QVariant();

    return m_KexiMigrate->value(i);

}

QVariant KexiMigrateReportData::value(const QString &fld) const
{
    if (!m_valid)
        return QVariant();

    int i = fieldNumber(fld);

    return m_KexiMigrate->value(i);
}

bool KexiMigrateReportData::moveNext()
{
    if (!m_valid)
        return false;

    return m_KexiMigrate->moveNext();

}

bool KexiMigrateReportData::movePrevious()
{
    if (!m_valid)
        return false;

    return m_KexiMigrate->movePrevious();
}

bool KexiMigrateReportData::moveFirst()
{
    if (!m_valid)
        return false;

    return m_KexiMigrate->moveFirst();

}

bool KexiMigrateReportData::moveLast()
{
    if (!m_valid)
        return false;

    return m_KexiMigrate->moveLast();
}

long KexiMigrateReportData::at() const
{
    return 0;
}

long KexiMigrateReportData::recordCount() const
{
    return 1;
}
