/* This file is part of the KDE project
   Copyright (C) 2003-2006 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIDB_CONN_SQLITE_H
#define KEXIDB_CONN_SQLITE_H

#include <qstringlist.h>

#include <kexidb/connection.h>

/*!
 */

namespace KexiDB
{

class SQLiteConnectionInternal;
class Driver;

//! sqlite-specific connection
class SQLiteConnection : public Connection
{
    Q_OBJECT

public:
    virtual ~SQLiteConnection();

    virtual Cursor* prepareQuery(const QString& statement, uint cursor_options = 0);
    virtual Cursor* prepareQuery(QuerySchema& query, uint cursor_options = 0);

//#ifndef SQLITE2 //TEMP IFDEF!
    virtual PreparedStatement::Ptr prepareStatement(PreparedStatement::StatementType type,
            FieldList& fields);
//#endif
    /*! Reimplemented to provide real read-only flag of the connection */
    virtual bool isReadOnly() const;

protected:
    /*! Used by driver */
    SQLiteConnection(Driver *driver, ConnectionData &conn_data);

    virtual bool drv_connect(KexiDB::ServerVersionInfo& version);
    virtual bool drv_disconnect();
    virtual bool drv_getDatabasesList(QStringList &list);

//TODO: move this somewhere to low level class (MIGRATION?)
    virtual bool drv_getTablesList(QStringList &list);

//TODO: move this somewhere to low level class (MIGRATION?)
    virtual bool drv_containsTable(const QString &tableName);

    /*! Creates new database using connection. Note: Do not pass \a dbName
      arg because for file-based engine (that has one database per connection)
      it is defined during connection. */
    virtual bool drv_createDatabase(const QString &dbName = QString());

    /*! Opens existing database using connection. Do not pass \a dbName
      arg because for file-based engine (that has one database per connection)
      it is defined during connection. If you pass it,
      database file name will be changed. */
    virtual bool drv_useDatabase(const QString &dbName = QString(), bool *cancelled = 0,
                                 MessageHandler* msgHandler = 0);

    virtual bool drv_closeDatabase();

    /*! Drops database from the server using connection.
      After drop, database shouldn't be accessible
      anymore, so database file is just removed. See note from drv_useDatabase(). */
    virtual bool drv_dropDatabase(const QString &dbName = QString());

    //virtual bool drv_createTable( const KexiDB::Table& table );

    virtual bool drv_executeSQL(const QString& statement);
//  virtual bool drv_executeQuery( const QString& statement );

    virtual quint64 drv_lastInsertRowID();

    virtual int serverResult();
    virtual QString serverResultName();
    virtual QString serverErrorMsg();
    virtual void drv_clearServerResult();
    virtual tristate drv_changeFieldProperty(TableSchema &table, Field& field,
            const QString& propertyName, const QVariant& value);

#ifdef SQLITE2
    /*! Alters table's described \a tableSchema name to \a newName.
     This implementation is ineffective but works.
     - creates a copy of the table
     - copies all rows
     - drops old table.
     All the above should be performed within single transaction.
     \return true on success.
     More advanced server backends implement this using "ALTER TABLE .. RENAME TO".
    */
    virtual bool drv_alterTableName(TableSchema& tableSchema, const QString& newName, bool replace = false);
#endif

    //! for drv_changeFieldProperty()
    tristate changeFieldType(TableSchema &table, Field& field, Field::Type type);

    SQLiteConnectionInternal* d;

    friend class SQLiteDriver;
    friend class SQLiteCursor;
};

}

#endif
