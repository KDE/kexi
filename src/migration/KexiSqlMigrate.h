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

#ifndef KEXISQLMIGRATE_H
#define KEXISQLMIGRATE_H

#include <keximigrate.h>

//! @short A specialized class for a migrate plugin that imports native SQL databases into Kexi projects
//! Compared to more generic KexiMigrate, KexiSqlMigrate implements needed methods using
//! low level APIs of KDb. This is used for example by MySQL and PostgreSQL migration plugins.
class KEXIMIGRATE_EXPORT KexiSqlMigrate : public KexiMigration::KexiMigrate
{
    Q_OBJECT

public:
    //! Constructs a migrate plugin that uses low level APIs of KDb driver @a kdbDriverId.
    //! @a kdbDriverId must not be empty.
    explicit KexiSqlMigrate(const QString &kdbDriverId, QObject *parent,
                            const QVariantList& args = QVariantList());

    virtual ~KexiSqlMigrate();

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
     @see KexiMigrate::drv_queryStringListFromSql() */
    tristate drv_queryStringListFromSql(
        const KDbEscapedString& sqlStatement, int columnNumber,
        QStringList *stringList, int numRecords = -1) Q_DECL_OVERRIDE;

    //! Copy a table from source DB to target DB (driver specific)
    bool drv_copyTable(const QString& srcTable,
                       KDbConnection *destConn, KDbTableSchema* dstTable,
                       const RecordFilter *recordFilter = nullptr) Q_DECL_OVERRIDE;

    bool drv_progressSupported() Q_DECL_OVERRIDE {
        return true;
    }

    bool drv_getTableSize(const QString& table, quint64* size) Q_DECL_OVERRIDE;

//! @todo move this somewhere to low level class (MIGRATION?) virtual bool drv_getTablesList( QStringList &list );
//! @todo move this somewhere to low level class (MIGRATION?) virtual bool drv_containsTable( const QString &tableName );

    //Extended API
    //! Starts reading data from the source dataset's table
    QSharedPointer<KDbSqlResult> drv_readFromTable(const QString & tableName) Q_DECL_OVERRIDE;

    const QString m_kdbDriverId;

    //! Used by drv_tableNames, should be filled in constructor of a subclass
    KDbEscapedString m_tableNamesSql;
};

#endif
