/* This file is part of the KDE project
   Copyright (C) 2003 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIDB_SQLITECONN_P_H
#define KEXIDB_SQLITECONN_P_H

#include <kexidb/connection_p.h>

#include "sqlite.h"

//for compatibility
#ifdef _SQLITE3_H_
# define SQLITE3
typedef sqlite3 sqlite_struct;
# define sqlite_free sqlite3_free
# define sqlite_close sqlite3_close
# define sqlite_exec sqlite3_exec
# define sqlite_last_insert_rowid sqlite3_last_insert_rowid
# define sqlite_error_string sqlite3_last_insert_row_id
# define sqlite_libversion sqlite3_libversion
# define sqlite_libencoding sqlite3_libencoding
#else
# ifndef SQLITE2
#  define SQLITE2
# endif
typedef struct sqlite sqlite_struct;
# define sqlite_free sqlite_freemem
#endif

namespace KexiDB
{

/*! Internal SQLite connection data. Also used inside SQLiteCursor. */
class SQLiteConnectionInternal : public ConnectionInternal
{
public:
    SQLiteConnectionInternal(Connection* connection);
    virtual ~SQLiteConnectionInternal();

    //! stores last result's message
    virtual void storeResult();

    sqlite_struct *data;
    bool data_owned; //!< true if data pointer should be freed on destruction
    QString errmsg; //<! server-specific message of last operation
    char *errmsg_p; //<! temporary: server-specific message of last operation
    int res; //<! result code of last operation on server

    QByteArray temp_st;
#ifdef SQLITE3
    const char *result_name;
#endif
};

}

#endif
