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

#ifdef Q_OS_WIN
# define KDEWIN_FCNTL_H // avoid redef.
# define KDEWIN_SYS_STAT_H // avoid redef.
# define KDEWIN_MATH_H // avoid redef.
# define KDEWIN_STDIO_H // avoid redef.
# include <../include/stdio.h>
# include <../include/math.h>
#endif

#include "mysqlmigrate.h"
#include <migration/keximigratedata.h>
#include <kexi.h>

#include <KDb>
#include <KDbConnectionProxy>
#include <KDbCursor>
#include <KDbDriverManager>
#include <KDbField>
#include <KDbSqlResult>
#include <KDbUtils>

#include <QString>
#include <QRegExp>
#include <QVariant>
#include <QList>
#include <QDebug>

#ifdef Q_OS_WIN
# undef _WIN32_WINNT // avoid redef.
#endif
#include <mysql_version.h>
#include <mysql.h>
#define BOOL bool

using namespace KexiMigration;

/* This is the implementation for the MySQL specific import routines. */
KEXI_PLUGIN_FACTORY(MySQLMigrate, "keximigrate_mysql.json")

/* ************************************************************************** */
//! Constructor (needed for trading interface)
MySQLMigrate::MySQLMigrate(QObject *parent, const QVariantList& args) :
        KexiMigrate(parent, args)
{
}

/* ************************************************************************** */
//! Destructor
MySQLMigrate::~MySQLMigrate()
{
}

KDbConnection* MySQLMigrate::drv_createConnection()
{
    KDbDriverManager manager;
    KDbDriver *driver = manager.driver("org.kde.kdb.mysql");
    if (!driver) {
        m_result = manager.result();
        return nullptr;
    }
    KDbConnection *c = driver->createConnection(*data()->source);
    m_result = c ? KDbResult() : driver->result();
    return c;
}

/* ************************************************************************** */
/*! Get the types and properties for each column. */
bool MySQLMigrate::drv_readTableSchema(
    const QString& originalName, KDbTableSchema *tableSchema)
{
//! @todo IDEA: ask for user input for captions

    //Perform a query on the table to get some data
    KDbEscapedString sql = KDbEscapedString("SELECT * FROM %1 LIMIT 0")
            .arg(sourceConnection()->escapeIdentifier(tableSchema->name()));
    if (!sourceConnection()->executeSQL(sql)) {
        return false;
    }
    QScopedPointer<KDbSqlResult> result(sourceConnection()->useSqlResult());
    if (!result) {
        qWarning() << "null result";
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

bool MySQLMigrate::drv_tableNames(QStringList *tableNames)
{
    if (!sourceConnection()->executeSQL(KDbEscapedString("SHOW TABLES"))) {
        return false;
    }
    QScopedPointer<KDbSqlResult> result(sourceConnection()->useSqlResult());
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

/*! Fetches single string at field \a fieldIndex for each record from result obtained
 by running \a sqlStatement.
 On success the result is stored in \a stringList and true is returned.
 \return cancelled if there are no records available. */
tristate MySQLMigrate::drv_queryStringListFromSQL(
    const KDbEscapedString& sqlStatement, int fieldIndex, QStringList *stringList, int numRecords)
{
    if (!sourceConnection()->executeSQL(sqlStatement)) {
        return false;
    }
    QScopedPointer<KDbSqlResult> result(sourceConnection()->useSqlResult());
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

//! Copy MySQL table to a KDb table
bool MySQLMigrate::drv_copyTable(const QString& srcTable, KDbConnection *destConn,
                                 KDbTableSchema* dstTable,
                                 const RecordFilter *recordFilter)
{
    if (!sourceConnection()->executeSQL(KDbEscapedString("SELECT * FROM %1")
                          .arg(sourceConnection()->escapeIdentifier(srcTable))))
    {
        //! Can't get the data but we can accept it
        return true;
    }
    QScopedPointer<KDbSqlResult> result(sourceConnection()->useSqlResult());
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

bool MySQLMigrate::drv_getTableSize(const QString& table, quint64 *size)
{
    Q_ASSERT(size);
    if (!sourceConnection()->executeSQL(KDbEscapedString("SELECT COUNT(*) FROM %1")
            .arg(sourceConnection()->escapeIdentifier(table))))
    {
        return false;
    }
    QScopedPointer<KDbSqlResult> result(sourceConnection()->useSqlResult());
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

KDbSqlResult* MySQLMigrate::drv_readFromTable(const QString& tableName)
{
    if (!sourceConnection()->executeSQL(KDbEscapedString("SELECT * FROM %1")
        .arg(sourceConnection()->escapeIdentifier(tableName))))
    {
        m_result = sourceConnection()->result();
        qWarning() << m_result;
        return nullptr;
    }
    QScopedPointer<KDbSqlResult> result(sourceConnection()->useSqlResult());
    if (!result || result->lastResult().isError()) {
        return false;
    }
    return result.take();
}

#include "mysqlmigrate.moc"
