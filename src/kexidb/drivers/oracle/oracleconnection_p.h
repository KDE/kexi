/* This file is part of the KDE project
   Copyright (C) 2008 Julia Sanchez-Simon <hithwen@gmail.com>
   Copyright (C) 2008 Miguel Angel Aragüez-Rey

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

#ifndef KEXIDB_ORACLECLIENT_P_H
#define KEXIDB_ORACLECLIENT_P_H
	#include <qstring.h>
	#include <kexidb/connection_p.h>
	#include <oci.h>
	#include <occi.h>
	#include <vector>
//typedef struct st_mysql MYSQL;
	#undef bool

#ifdef ORACLEMIGRATE_H
#define NAMESPACE KexiMigration
	#else
#define NAMESPACE KexiDB
	#endif

namespace KexiDB {
    class ConnectionData;
}
namespace NAMESPACE {
//! Internal Oracle connection data.
/*! Provides a low-level API for accessing Oracle databases, that can
    be shared by any module that needs direct access to the underlying
    database.  Used by the KexiDB driver.
 */
class OracleConnectionInternal : public KexiDB::ConnectionInternal
{
	public:
		OracleConnectionInternal(KexiDB::Connection* connection);
		virtual ~OracleConnectionInternal();

		//! Connects to a Oracle database
		bool db_connect(const KexiDB::ConnectionData& data);

		//! Disconnects from the database
		bool db_disconnect();

		//! Selects a database that is about to be used
		bool useDatabase(const QString &dbName = QString::null);
		
		//! Execute SQL statement on the database
		bool executeSQL( const QString& statement );

		//! Stores last operation's result
		virtual void storeResult();

		//! Escapes a table, database or column name
		QString escapeIdentifier(const QString& str) const;
		QString getServerVersion();
		
    oracle::occi::Environment *env;
		oracle::occi::ResultSet  	*rs;
  	oracle::occi::Connection  *oraconn;
  	oracle::occi::Statement  	*stmt;
		QString errmsg; //!< server-specific message of last operation
		int errno; //!< result code of last operation on server

		private:
		oracle::occi::SQLException ea;
		
		
        
};


//! Internal Oracle cursor data.
/*! Provides a low-level abstraction for iterating over Oracle result sets. */
class OracleCursorData : public OracleConnectionInternal
{
	public:
		OracleCursorData(KexiDB::Connection* connection);
		virtual ~OracleCursorData();
		std::vector<unsigned long> lengths;
		unsigned long numRows;
      
};

}
#endif
