/* This file is part of the KDE project
   Copyright (C) 2002 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2003 Joseph Wenninger<jowenn@kde.org>
   Copyright (C) 2004 Jarosław Staniek <staniek@kde.org>

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

#ifndef MYSQLCONNECTION_H
#define MYSQLCONNECTION_H

#include <qstringlist.h>

#include <kexidb/connection.h>
#include "mysqlcursor.h"

namespace KexiDB
{

class MySqlConnectionInternal;

/*!
 * Should override kexiDB/kexiDB
 * all other members are done by the
 * base class.
 */
class MySqlConnection : public Connection
{
    Q_OBJECT

public:
    virtual ~MySqlConnection();

    virtual Cursor* prepareQuery(const QString& statement = QString(), uint cursor_options = 0);
    virtual Cursor* prepareQuery(QuerySchema& query, uint cursor_options = 0);

    virtual PreparedStatement::Ptr prepareStatement(PreparedStatement::StatementType type,
            FieldList& fields);

protected:

    /*! Used by driver */
    MySqlConnection(Driver *driver, ConnectionData &conn_data);

    virtual bool drv_connect(KexiDB::ServerVersionInfo& version);
    virtual bool drv_disconnect();
    virtual bool drv_getDatabasesList(QStringList &list);
    //! reimplemented using "SHOW DATABASES LIKE..." because MySQL stores db names in lower case.
    virtual bool drv_databaseExists(const QString &dbName, bool ignoreErrors = true);
    virtual bool drv_createDatabase(const QString &dbName = QString());
    virtual bool drv_useDatabase(const QString &dbName = QString(), bool *cancelled = 0,
                                 MessageHandler* msgHandler = 0);
    virtual bool drv_closeDatabase();
    virtual bool drv_dropDatabase(const QString &dbName = QString());
    virtual bool drv_executeSQL(const QString& statement);
    virtual quint64 drv_lastInsertRowID();

    virtual int serverResult();
    virtual QString serverResultName();
    virtual QString serverErrorMsg();
    virtual void drv_clearServerResult();

//TODO: move this somewhere to low level class (MIGRATION?)
    virtual bool drv_getTablesList(QStringList &list);
//TODO: move this somewhere to low level class (MIGRATION?)
    virtual bool drv_containsTable(const QString &tableName);

    MySqlConnectionInternal* d;

    friend class MySqlDriver;
    friend class MySqlCursor;
};

}

#endif
