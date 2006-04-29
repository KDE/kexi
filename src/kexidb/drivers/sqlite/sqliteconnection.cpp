/* This file is part of the KDE project
   Copyright (C) 2003-2006 Jaroslaw Staniek <js@iidea.pl>

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

#include "sqliteconnection.h"
#include "sqliteconnection_p.h"
#include "sqlitecursor.h"
#include "sqlitepreparedstatement.h"

#include "sqlite.h"

#ifndef SQLITE2
# include "kexisql.h" //for isReadOnly()
#endif

#include <kexidb/driver.h>
#include <kexidb/cursor.h>
#include <kexidb/error.h>

#include <qfile.h>
#include <qdir.h>

#include <kgenericfactory.h>
#include <kdebug.h>

//remove debug
#undef KexiDBDrvDbg
#define KexiDBDrvDbg if (0) kdDebug()

using namespace KexiDB;

SQLiteConnectionInternal::SQLiteConnectionInternal(Connection *connection)
 : ConnectionInternal(connection)
 , data(0)
 , data_owned(true)
 , errmsg_p(0)
 , res(SQLITE_OK)
 , temp_st(0x10000)
#ifdef SQLITE3
 , result_name(0)
#endif
{
}

SQLiteConnectionInternal::~SQLiteConnectionInternal() 
{
	if (data_owned && data) {
		free( data ); 
		data = 0;
	}
//sqlite_freemem does this	if (errmsg) {
//		free( errmsg );
//		errmsg = 0;
//	}
}

void SQLiteConnectionInternal::storeResult()
{
	if (errmsg_p) {
		errmsg = errmsg_p;
		sqlite_free(errmsg_p);
		errmsg_p = 0;
	}
#ifdef SQLITE3
	errmsg = (data && res!=SQLITE_OK) ? sqlite3_errmsg(data) : 0;
#endif
}

/*! Used by driver */
SQLiteConnection::SQLiteConnection( Driver *driver, ConnectionData &conn_data )
	: Connection( driver, conn_data )
	,d(new SQLiteConnectionInternal(this))
{
}

SQLiteConnection::~SQLiteConnection()
{
	KexiDBDrvDbg << "SQLiteConnection::~SQLiteConnection()" << endl;
	//disconnect if was connected
//	disconnect();
	destroy();
	delete d;
	KexiDBDrvDbg << "SQLiteConnection::~SQLiteConnection() ok" << endl;
}

bool SQLiteConnection::drv_connect()
{
	KexiDBDrvDbg << "SQLiteConnection::connect()" << endl;
	return true;
}

bool SQLiteConnection::drv_disconnect()
{
	KexiDBDrvDbg << "SQLiteConnection::disconnect()" << endl;
	return true;
}

bool SQLiteConnection::drv_getDatabasesList( QStringList &list )
{
	//this is one-db-per-file database
	list.append( m_data->fileName() ); //more consistent than dbFileName() ?
	return true;
}

bool SQLiteConnection::drv_containsTable( const QString &tableName )
{
	bool success;
	return resultExists(QString("select name from sqlite_master where type='table' and name LIKE %1")
		.arg(driver()->escapeString(tableName)), success) && success;
}

bool SQLiteConnection::drv_getTablesList( QStringList &list )
{
	KexiDB::Cursor *cursor;
	m_sql = "select lower(name) from sqlite_master where type='table'";
	if (!(cursor = executeQuery( m_sql ))) {
		KexiDBWarn << "Connection::drv_getTablesList(): !executeQuery()" << endl;
		return false;
	}
	list.clear();
	cursor->moveFirst();
	while (!cursor->eof() && !cursor->error()) {
		list += cursor->value(0).toString();
		cursor->moveNext();
	}
	if (cursor->error()) {
		deleteCursor(cursor);
		return false;
	}
	return deleteCursor(cursor);
}

bool SQLiteConnection::drv_createDatabase( const QString &dbName )
{
	// SQLite creates a new db is it does not exist
	return drv_useDatabase(dbName);
#if 0
	d->data = sqlite_open( QFile::encodeName( m_data->fileName() ), 0/*mode: unused*/, 
		&d->errmsg_p );
	d->storeResult();
	return d->data != 0;
#endif
}

bool SQLiteConnection::drv_useDatabase( const QString &dbName, bool *cancelled, 
	MessageHandler* msgHandler )
{
	Q_UNUSED(dbName);
#ifndef KEXI_FUTURE_FEATURES
	Q_UNUSED(cancelled);
	Q_UNUSED(msgHandler);
#endif
//	KexiDBDrvDbg << "drv_useDatabase(): " << m_data->fileName() << endl;
#ifdef SQLITE2
	d->data = sqlite_open( QFile::encodeName( m_data->fileName() ), 0/*mode: unused*/, 
		&d->errmsg_p );
	d->storeResult();
	return d->data != 0;
#else //SQLITE3
	//TODO: perhaps allow to use sqlite3_open16() as well for SQLite ~ 3.3 ?
//! @todo add option (command line or in kexirc?)
	int exclusiveFlag = Connection::isReadOnly() ? SQLITE_OPEN_READONLY : SQLITE_OPEN_WRITE_LOCKED; // <-- shared read + (if !r/o): exclusive write
//! @todo add option
	int allowReadonly = 1;
# ifdef KEXI_FUTURE_FEATURES
	const bool wasReadOnly = Connection::isReadOnly();
# endif

	d->res = sqlite3_open( 
		//m_data->fileName().ucs2(), //utf16
		QFile::encodeName( m_data->fileName() ), 
		&d->data,
		exclusiveFlag,
		allowReadonly /* If 1 and locking fails, try opening in read-only mode */
	);
	d->storeResult();

#ifdef KEXI_FUTURE_FEATURES
	if (d->res == SQLITE_OK && cancelled && !wasReadOnly && allowReadonly && isReadOnly()) {
		//opened as read only, ask
		if (KMessageBox::Continue != 
			askQuestion( 
			futureI18n("Do you want to open file \"%1\" as read-only?")
				.arg(QDir::convertSeparators(m_data->fileName()))
			+ "\n\n"
			+ i18n("The file is probably already open on this or another computer.") + " "
			+ i18n("Could not gain exclusive access for writing the file."),
			KMessageBox::WarningContinueCancel, KMessageBox::Continue, 
			KGuiItem(futureI18n("Open As Read-Only"), "fileopen"), KStdGuiItem::cancel(),
			"askBeforeOpeningFileReadOnly", KMessageBox::Notify, msgHandler ))
		{
			clearError();
			if (!drv_closeDatabase())
				return false;
			*cancelled = true;
			return false;
		}
	}
#endif

	if (d->res == SQLITE_CANTOPEN_WITH_LOCKED_READWRITE) {
		setError(ERR_ACCESS_RIGHTS, 
		i18n("The file is probably already open on this or another computer.")+"\n\n"
		+ i18n("Could not gain exclusive access for reading and writing the file.") + " "
		+ i18n("Check the file's permissions and whether it is already opened and locked by another application."));
	}
	else if (d->res == SQLITE_CANTOPEN_WITH_LOCKED_WRITE) {
		setError(ERR_ACCESS_RIGHTS, 
		i18n("The file is probably already open on this or another computer.")+"\n\n"
		+ i18n("Could not gain exclusive access for writing the file.") + " "
		+ i18n("Check the file's permissions and whether it is already opened and locked by another application."));
	}
	return d->res == SQLITE_OK;
#endif
}

bool SQLiteConnection::drv_closeDatabase()
{
	if (!d->data)
		return false;

#ifdef SQLITE2
	sqlite_close(d->data);
	d->data = 0;
	return true;
#else
	const int res = sqlite_close(d->data);
	if (SQLITE_OK == res) {
		d->data = 0;
		return true;
	}
	if (SQLITE_BUSY==res) {
		setError(ERR_OTHER);
	}
	return false;
#endif
}

bool SQLiteConnection::drv_dropDatabase( const QString &dbName )
{
	if (QFile(m_data->fileName()).exists() && !QDir().remove(m_data->fileName())) {
		setError(ERR_ACCESS_RIGHTS, i18n("Could not remove file \"%1\".")
			.arg(QDir::convertSeparators(dbName)) + " "
			+ i18n("Check the file's permissions and whether it is already opened and locked by another application."));
		return false;
	}
	return true;
}

//CursorData* SQLiteConnection::drv_createCursor( const QString& statement )
Cursor* SQLiteConnection::prepareQuery( const QString& statement, uint cursor_options )
{
	return new SQLiteCursor( this, statement, cursor_options );
}

Cursor* SQLiteConnection::prepareQuery( QuerySchema& query, uint cursor_options )
{
	return new SQLiteCursor( this, query, cursor_options );
}

bool SQLiteConnection::drv_executeSQL( const QString& statement )
{
//	KexiDBDrvDbg << "SQLiteConnection::drv_executeSQL(" << statement << ")" <<endl;
//	QCString st(statement.length()*2);
//	st = escapeString( statement.local8Bit() ); //?
#ifdef SQLITE_UTF8
	d->temp_st = statement.utf8();
#else
	d->temp_st = statement.local8Bit(); //latin1 only
#endif

	d->res = sqlite_exec( 
		d->data, 
		(const char*)d->temp_st, 
		0/*callback*/, 
		0,
		&d->errmsg_p );
	d->storeResult();
	return d->res==SQLITE_OK;
}

Q_ULLONG SQLiteConnection::drv_lastInsertRowID()
{
	return (Q_ULLONG)sqlite_last_insert_rowid(d->data);
}

int SQLiteConnection::serverResult()
{
	return d->res==0 ? Connection::serverResult() : d->res;
}

QString SQLiteConnection::serverResultName()
{
	QString r = 
#ifdef SQLITE2
		QString::fromLatin1( sqlite_error_string(d->res) );
#else //SQLITE3
		QString::null; //fromLatin1( d->result_name );
#endif
	return r.isEmpty() ? Connection::serverResultName() : r;
}

void SQLiteConnection::drv_clearServerResult()
{
	if (!d)
		return;
	d->res = SQLITE_OK;
#ifdef SQLITE2
	d->errmsg_p = 0;
#else
//	d->result_name = 0;
#endif
}

QString SQLiteConnection::serverErrorMsg()
{
	return d->errmsg.isEmpty() ? Connection::serverErrorMsg() : d->errmsg;
}

PreparedStatement::Ptr SQLiteConnection::prepareStatement(PreparedStatement::StatementType type, 
	FieldList& fields)
{
//#ifndef SQLITE2 //TEMP IFDEF!
	return new SQLitePreparedStatement(type, *d, fields);
//#endif
}

bool SQLiteConnection::isReadOnly() const
{
#ifdef SQLITE2
	return Connection::isReadOnly();
#else
	return d->data ? sqlite3_is_readonly(d->data) : false;
#endif
}

#include "sqliteconnection.moc"
