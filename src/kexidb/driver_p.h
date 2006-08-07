/* This file is part of the KDE project
   Copyright (C) 2003-2004 Jaroslaw Staniek <js@iidea.pl>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef KEXIDB_DRIVER_P_H
#define KEXIDB_DRIVER_P_H

#ifndef __KEXIDB__
# error "Do not include: this is KexiDB internal file"
#endif

#include <qstring.h>
#include <qvariant.h>
#include <qmap.h>
#include <qptrdict.h>
#include <qasciidict.h>
#include <qvaluevector.h>
#include <kgenericfactory.h>

#include "connection.h"
#include "admin.h"

class KService;

namespace KexiDB {

/*! Detailed definition of driver's default behaviour.
 Note for driver developers: 
 Change these defaults in you Driver subclass
 constructor, if needed.
*/
class KEXI_DB_EXPORT DriverBehaviour
{
	public:
		DriverBehaviour();
			
	//! "UNSIGNED" by default
	QString UNSIGNED_TYPE_KEYWORD;
	
	//! "AUTO_INCREMENT" by default, used as add-in word to field definition
	//! May be also used as full definition if SPECIAL_AUTO_INCREMENT_DEF is true.
	QString AUTO_INCREMENT_FIELD_OPTION;

	//! "AUTO_INCREMENT PRIMARY KEY" by default, used as add-in word to field definition
	//! May be also used as full definition if SPECIAL_AUTO_INCREMENT_DEF is true.
	QString AUTO_INCREMENT_PK_FIELD_OPTION;

	//! "" by default, used as type string for autoinc. field definition
	//! pgsql defines it as "SERIAL", sqlite defines it as "INTEGER"
	QString AUTO_INCREMENT_TYPE;

	/*! True if autoincrement field has special definition 
	 e.g. like "INTEGER PRIMARY KEY" for SQLite.
	 Special definition string should be stored in AUTO_INCREMENT_FIELD_OPTION.
	 False by default. */
	bool SPECIAL_AUTO_INCREMENT_DEF : 1;

	/*! True if autoincrement requires field to be declared as primary key.
	 This is true for SQLite. False by default. */
	bool AUTO_INCREMENT_REQUIRES_PK : 1;

	/*! Name of a field (or built-in function) with autoincremented unique value,
	 typically returned by Connection::drv_lastInsertRowID().

	 Examples:
	 - PostgreSQL and SQLite engines use 'OID' field 
	 - MySQL uses LAST_INSERT_ID() built-in function
	*/
	QString ROW_ID_FIELD_NAME;
	
	/*! True if the value (fetched from field or function,
	 defined by ROW_ID_FIELD_NAME member) is EXACTLY the value of autoincremented field,
	 not an implicit (internal) row number. Default value is false.
	 
	 Examples:
	 - PostgreSQL and SQLite engines have this flag set to false ('OID' field has 
	    it's own implicit value)
	 - MySQL engine has this flag set to true (LAST_INSERT_ID() returns real value 
	 of last autoincremented field). 

	 Notes:
	 If it's false, we have a convenient way for identifying row even when there's 
	 no primary key defined. So, as '_ROWID' column in MySQL is really 
	 just a synonym for the primary key, this engine needs to have primary keys always 
	 defined if we want to use interactive editing features like row updating and deleting.
	*/
	bool ROW_ID_FIELD_RETURNS_LAST_AUTOINCREMENTED_VALUE : 1;

	/*! Name of any (e.g. first found) database for this connection that 
	 typically always exists. This can be not set if we want to do some magic checking
	 what database name is availabe by reimplementing
	 Connection::anyAvailableDatabaseName().
	 Example: for PostgreSQL this is "template1".
	 
	 \sa Connection::SetAvailableDatabaseName()
	*/
	QString ALWAYS_AVAILABLE_DATABASE_NAME;
	
	/*! Quotation marks used for escaping identifier (see Driver::escapeIdentifier()).
	 Default value is '"'. Change it for your driver.
	*/
	QChar QUOTATION_MARKS_FOR_IDENTIFIER;
	
	/*! True if using database is requied to perform real connection.
	 This is true for may engines, e.g. for PostgreSQL, where connections 
	 string should contain a database name.
	 This flag is unused for file-based db drivers,
	 by default set to true and used for all other db drivers.
	*/
	bool USING_DATABASE_REQUIRED_TO_CONNECT : 1;

	/*! True if before we know whether the fetched result of executed query
	 is empty or not, we need to fetch first record. Particularly, it's true for SQLite.
	 The flag is used in Cursor::open(). By default this flag is false. */
	bool _1ST_ROW_READ_AHEAD_REQUIRED_TO_KNOW_IF_THE_RESULT_IS_EMPTY : 1;

	/*! True if "SELECT 1 from (subquery)" is supported. False by default.
	 Used in Connection::resultExists() for optimization. It's set to true for SQLite driver. */
	bool SELECT_1_SUBQUERY_SUPPORTED : 1;

	/*! Keywords that need to be escaped for the driver.  Set this before calling
	    Driver::initSQLKeywords. */
	const char** SQL_KEYWORDS;
};

/*! Private driver's data members. Available for implementation. */
class DriverPrivate
{
	public:
		DriverPrivate();
		virtual ~DriverPrivate();

		QPtrDict<Connection> connections;

//(js)now QObject::name() is reused:
//		/*! The name equal to the service name (X-Kexi-DriverName) 
//		 stored in given service .desktop file. Set this in subclasses. */
//		QString m_driverName;
		
		/*! Name of MIME type of files handled by this driver 
		 if it is a file-based database's driver 
		 (equal X-Kexi-FileDBDriverMime service property) */
		QString fileDBDriverMimeType;

		/*! Info about the driver as a service. */
		KService *service;

		/*! Internal constant flag: Set this in subclass if driver is a file driver */
		bool isFileDriver : 1;

		/*! Internal constant flag: Set this in subclass if after successful
		 drv_createDatabased() database is in opened state (as after useDatabase()). 
		 For most engines this is not true. */
		bool isDBOpenedAfterCreate : 1;

		/*! List of system objects names, eg. build-in system tables that 
		 cannot be used by user, and in most cases user even shouldn't see these.
		 The list contents is driver dependent (by default is empty) 
		 - fill this in subclass ctor. */
//		QStringList m_systemObjectNames;

		/*! List of system fields names, build-in system fields that cannot be used by user,
		 and in most cases user even shouldn't see these.
		 The list contents is driver dependent (by default is empty) - fill this in subclass ctor. */
//		QStringList m_systemFieldNames;

		/*! Features (like transactions, etc.) supported by this driver
		 (sum of selected  Features enum items). 
		 This member should be filled in driver implementation's constructor 
		 (by default m_features==NoFeatures). */
		int features;

		//! real type names for this engine
		QValueVector<QString> typeNames;

		/*! Driver properties dictionary (indexed by name), 
		 useful for presenting properties to the user. 
		 Set available properties here in driver implementation. */
		QMap<QCString,QVariant> properties;

		/*! i18n'd captions for properties. You do not need 
		 to set predefined properties' caption in driver implementation 
		 -it's done automatically. */
		QMap<QCString,QString> propertyCaptions;

		/*! Provides a number of database administration tools for the driver. */
		AdminTools *adminTools;

	/*! Kexi SQL keywords that need to be escaped if used as an identifier (e.g.
	    for a table or column name).  These keywords will be escaped by the
	    front-end, even if they are not recognised by the backend to provide
	    UI consistency and to allow DB migration without changing the queries.
	    \sa DriverPrivate::initKexiKeywords(), KexiDB::kexiSQLKeywords.
	*/
	static QAsciiDict<bool>* kexiSQLDict;
	static const char *kexiSQLKeywords[];
	
	/*! Driver-specific SQL keywords that need to be escaped if used as an
	    identifier (e.g. for a table or column name) that aren't also Kexi SQL 
	    keywords.  These don't neccesarily need to be escaped when displayed by
	    the front-end, because they won't confuse the parser.  However, they do
	    need to be escaped before sending to the DB-backend which will have
	    it's own parser. 
	
	    \sa DriverBehaviour::SQL_KEYWORDS.
	*/	
	QAsciiDict<bool>* driverSQLDict;
	
	/*! Initialise the dictionary of Kexi SQL keywords used for escaping. */
	void initKexiKeywords();
	/*! Initialise the dictionary of driver-specific keywords used for escaping.
	    \a hashSize is the number of buckets to use in the dictionary.
	    \sa Driver::initSQLKeywords(). */
	void initDriverKeywords(const char* keywords[], int hashSize);
	
	protected:
		/*! Used by driver manager to initialize properties taken using internal
	      driver flags. */
		void initInternalProperties();
	
	private:
		void initKeywords(const char* keywords[], QAsciiDict<bool>& dict);
	
	friend class DriverManagerInternal;
};

// escaping types for Driver::escapeBLOBInternal()
#define BLOB_ESCAPING_TYPE_USE_X     0 //!< escaping like X'abcd0', used by sqlite
#define BLOB_ESCAPING_TYPE_USE_0x    1 //!< escaping like 0xabcd0, used by mysql
#define BLOB_ESCAPING_TYPE_USE_OCTAL 2 //!< escaping like 'abcd\\000', used by pgsql

class KEXI_DB_EXPORT AdminTools::Private
{
	public:
		Private();
		~Private();
};

}

//! Driver's static version information (implementation), 
//! with KLibFactory symbol declaration.
#define KEXIDB_DRIVER_INFO( class_name, internal_name ) \
	DatabaseVersionInfo class_name::version() const { return KEXIDB_VERSION; } \
	K_EXPORT_COMPONENT_FACTORY(kexidb_ ## internal_name ## driver, KGenericFactory<KexiDB::class_name>( "kexidb_" #internal_name ))

#endif
