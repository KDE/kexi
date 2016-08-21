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

#ifndef KEXIMYSQLMIGRATE_H
#define KEXIMYSQLMIGRATE_H

#include <keximigrate.h>

namespace KexiMigration
{

class MysqlMigrate : public KexiMigrate
{
    Q_OBJECT

public:
    explicit MysqlMigrate(QObject *parent, const QVariantList& args = QVariantList());
    virtual ~MysqlMigrate();

protected:
    //! Driver specific function to return table names
    bool drv_tableNames(QStringList *tablenames) Q_DECL_OVERRIDE;

    //! Driver specific implementation to read a table schema
    bool drv_readTableSchema(
        const QString& originalName, KDbTableSchema *tableSchema) Q_DECL_OVERRIDE;

    //! Driver specific connection creation
    KDbConnection* drv_createConnection() Q_DECL_OVERRIDE;

    /*! Fetches single string at column \a columnNumber for each record from result obtained
     by running \a sqlStatement. \a numRecords can be specified to limit number of records read.
     If \a numRecords is -1, all records are loaded.
     @see KexiMigrate::drv_queryStringListFromSQL() */
    tristate drv_queryStringListFromSQL(
        const KDbEscapedString& sqlStatement, int columnNumber,
        QStringList *stringList, int numRecords = -1) Q_DECL_OVERRIDE;

    //! Copy a table from source DB to target DB (driver specific)
    bool drv_copyTable(const QString& srcTable,
                       KDbConnection *destConn, KDbTableSchema* dstTable,
                       const RecordFilter *recordFilter = nullptr) Q_DECL_OVERRIDE;

    virtual bool drv_progressSupported() {
        return true;
    }

    virtual bool drv_getTableSize(const QString& table, quint64* size);

//! @todo move this somewhere to low level class (MIGRATION?) virtual bool drv_getTablesList( QStringList &list );
//! @todo move this somewhere to low level class (MIGRATION?) virtual bool drv_containsTable( const QString &tableName );

    //Extended API
    //! Starts reading data from the source dataset's table
    KDbSqlResult* drv_readFromTable(const QString & tableName) Q_DECL_OVERRIDE;
};
}

#endif
