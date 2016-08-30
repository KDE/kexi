/* This file is part of the KDE project
   Copyright (C) 2004 Martin Ellis <m.a.ellis@ncl.ac.uk>
   Copyright (C) 2006-2016 Jarosław Staniek <staniek@kde.org>

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

#include "KexiSqlMigrate.h"
#include <keximigratedata.h>
#include <kexi.h>

#include <KDbDriverManager>
#include <KDbConnectionProxy>
#include <KDbSqlResult>

KexiSqlMigrate::KexiSqlMigrate(const QString &kdbDriverId, QObject *parent,
                               const QVariantList& args)
        : KexiMigration::KexiMigrate(parent, args)
        , m_kdbDriverId(kdbDriverId)
{
    Q_ASSERT(!m_kdbDriverId.isEmpty());
}

KexiSqlMigrate::~KexiSqlMigrate()
{
}

KDbConnection* KexiSqlMigrate::drv_createConnection()
{
    KDbDriverManager manager;
    KDbDriver *driver = manager.driver(m_kdbDriverId);
    if (!driver) {
        m_result = manager.result();
        return nullptr;
    }
    KDbConnection *c = driver->createConnection(*data()->source);
    m_result = c ? KDbResult() : driver->result();
    return c;
}

bool KexiSqlMigrate::drv_readTableSchema(
    const QString& originalName, KDbTableSchema *tableSchema)
{
//! @todo IDEA: ask for user input for captions

    //Perform a query on the table to get some data
    KDbEscapedString sql = KDbEscapedString("SELECT * FROM %1 LIMIT 0")
            .arg(sourceConnection()->escapeIdentifier(tableSchema->name()));
    QScopedPointer<KDbSqlResult> result(sourceConnection()->executeSQL(sql));
    if (!result) {
        return false;
    }

    bool ok = true;
    const int fieldsCount = result->fieldsCount();
    for (int i = 0; i < fieldsCount; i++) {
        KDbField *field = result->createField(originalName, i);
        if (field->type() == KDbField::InvalidType) {
            field->setType(userType(originalName + '.' + field->name()));
        }
        if (!tableSchema->addField(field)) {
            delete field;
            tableSchema->clear();
            ok = false;
            break;
        }
    }
    return ok;
}

bool KexiSqlMigrate::drv_tableNames(QStringList *tableNames)
{
    QScopedPointer<KDbSqlResult> result(sourceConnection()->executeSQL(m_tableNamesSql));
    if (!result || result->fieldsCount() < 1) {
        return false;
    }
    Q_FOREVER {
        QScopedPointer<KDbSqlRecord> record(result->fetchRecord());
        if (!record) {
            if (result->lastResult().isError()) {
                return false;
            }
            break;
        }
        tableNames->append(record->stringValue(0));
    }
    return true;
}

tristate KexiSqlMigrate::drv_queryStringListFromSQL(
    const KDbEscapedString& sqlStatement, int fieldIndex, QStringList *stringList, int numRecords)
{
    QScopedPointer<KDbSqlResult> result(sourceConnection()->executeSQL(sqlStatement));
    if (!result) {
        return true;
    }
    if (result->fieldsCount() < (fieldIndex+1)) {
        qWarning() << sqlStatement << ": fieldIndex too large ("
                   << fieldIndex << "), expected 0.." << result->fieldsCount() - 1;
        return false;
    }
    for (int i = 0; numRecords == -1 || i < numRecords; i++) {
        QScopedPointer<KDbSqlRecord> record(result->fetchRecord());
        if (!record) {
            if (numRecords != -1 || result->lastResult().isError()) {
                return false;
            }
            return true;
        }
        stringList->append(record->stringValue(fieldIndex));
    }
    return true;
}

bool KexiSqlMigrate::drv_copyTable(const QString& srcTable, KDbConnection *destConn,
                                 KDbTableSchema* dstTable,
                                 const RecordFilter *recordFilter)
{
    QScopedPointer<KDbSqlResult> result(
        sourceConnection()->executeSQL(KDbEscapedString("SELECT * FROM %1")
            .arg(sourceConnection()->escapeIdentifier(srcTable))));
    if (!result) {
        return false;
    }
    const KDbQueryColumnInfo::Vector fieldsExpanded(dstTable->query()->fieldsExpanded());
    const int numFields = qMin(fieldsExpanded.count(), result->fieldsCount());
    Q_FOREVER {
        QScopedPointer<KDbSqlRecord> record(result->fetchRecord());
        if (!record) {
            if (!result->lastResult().isError()) {
                break;
            }
            return false;
        }
        bool filterUsed = false;
        if (recordFilter) {
            if ((*recordFilter)(*record)) {
                filterUsed = true;
            } else {
                continue;
            }
        }
        QList<QVariant> vals;
        for(int i = 0; i < numFields; ++i) {
            const KDbSqlString s(record->cstringValue(i));
            vals.append(KDb::cstringToVariant(
                            s.string, fieldsExpanded.at(i)->field->type(), 0, s.length));
        }
        updateProgress();
        if (recordFilter && !filterUsed) {
            if (!(*recordFilter)(vals)) {
                continue;
            }
        }
        if (!destConn->insertRecord(dstTable, vals)) {
            return false;
        }
    }
    /*! @todo Check that wasn't an error, rather than end of result set */
    return true;
}

bool KexiSqlMigrate::drv_getTableSize(const QString& table, quint64 *size)
{
    Q_ASSERT(size);
    QScopedPointer<KDbSqlResult> result(
        sourceConnection()->executeSQL(KDbEscapedString("SELECT COUNT(*) FROM %1")
                                       .arg(sourceConnection()->escapeIdentifier(table))));
    if (!result) {
        return false;
    }
    QScopedPointer<KDbSqlRecord> record(result->fetchRecord());
    if (!result || result->fieldsCount() == 0) {
        return false;
    }
    bool ok;
    quint64 value = record->toByteArray(0).toULongLong(&ok);
    if (!ok) {
        value = -1;
    }
    *size = value;
    return ok;
}

KDbSqlResult* KexiSqlMigrate::drv_readFromTable(const QString& tableName)
{
    QScopedPointer<KDbSqlResult> result(sourceConnection()->executeSQL(KDbEscapedString("SELECT * FROM %1")
        .arg(sourceConnection()->escapeIdentifier(tableName))));
    if (!result || result->lastResult().isError()) {
        m_result = sourceConnection()->result();
        qWarning() << m_result;
        return nullptr;
    }
    return result.take();
}
