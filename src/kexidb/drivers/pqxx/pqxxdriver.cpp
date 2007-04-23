/* This file is part of the KDE project
   Copyright (C) 2003 Adam Pigg <adam@piggz.co.uk>

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

#include <kexidb/connection.h>
#include <kexidb/drivermanager.h>
#include <kexidb/driver_p.h>
#include <kexidb/utils.h>
#include "pqxxdriver.h"
#include "pqxxconnection.h"
#include <string>

#include <kdebug.h>

using namespace KexiDB;

KEXIDB_DRIVER_INFO( pqxxSqlDriver, pqxxsql )

//==================================================================================
//
pqxxSqlDriver::pqxxSqlDriver( QObject *parent, const char *name, const QStringList &args )
	: Driver( parent, name, args )
{
	d->isFileDriver = false;
	d->features = SingleTransactions | CursorForward | CursorBackward;
//! @todo enable this when kexidb supports multiple: d->features = MultipleTransactions | CursorForward | CursorBackward;

	beh->UNSIGNED_TYPE_KEYWORD = "";
	beh->ROW_ID_FIELD_NAME = "oid";
	beh->SPECIAL_AUTO_INCREMENT_DEF = false;
	beh->AUTO_INCREMENT_TYPE = "SERIAL";
	beh->AUTO_INCREMENT_FIELD_OPTION = "";
	beh->AUTO_INCREMENT_PK_FIELD_OPTION = "PRIMARY KEY";
	beh->ALWAYS_AVAILABLE_DATABASE_NAME = "template1";
	beh->QUOTATION_MARKS_FOR_IDENTIFIER = '"';
	beh->SQL_KEYWORDS = keywords;
	initSQLKeywords(233);

	//predefined properties
	d->properties["client_library_version"] = "";//TODO
	d->properties["default_server_encoding"] = ""; //TODO

	d->typeNames[Field::Byte]="SMALLINT";
	d->typeNames[Field::ShortInteger]="SMALLINT";
	d->typeNames[Field::Integer]="INTEGER";
	d->typeNames[Field::BigInteger]="BIGINT";
	d->typeNames[Field::Boolean]="BOOLEAN";
	d->typeNames[Field::Date]="DATE";
	d->typeNames[Field::DateTime]="TIMESTAMP";
	d->typeNames[Field::Time]="TIME";
	d->typeNames[Field::Float]="REAL";
	d->typeNames[Field::Double]="DOUBLE PRECISION";
	d->typeNames[Field::Text]="CHARACTER VARYING";
	d->typeNames[Field::LongText]="TEXT";
	d->typeNames[Field::BLOB]="BYTEA";
}

//==================================================================================
//Override the default implementation to allow for NUMERIC type natively
QString pqxxSqlDriver::sqlTypeName(int id_t, int p) const
{ 
	if (id_t==Field::Null)
		return "NULL";
	if (id_t==Field::Float || id_t==Field::Double)
	{
		if (p>0)
		{
			return "NUMERIC";
		}
		else
		{
			return d->typeNames[id_t];
		}
	}
	else
	{
		return d->typeNames[id_t];
	}
}

//==================================================================================
//
pqxxSqlDriver::~pqxxSqlDriver()
{
//	delete d;
}

//==================================================================================
//
KexiDB::Connection*
pqxxSqlDriver::drv_createConnection( ConnectionData &conn_data )
{
	return new pqxxSqlConnection( this, conn_data );
}

//==================================================================================
//
bool pqxxSqlDriver::isSystemObjectName( const QString& n ) const
{
	return Driver::isSystemObjectName(n);
}

//==================================================================================
//
bool pqxxSqlDriver::drv_isSystemFieldName( const QString& ) const
{
	return false;
}

//==================================================================================
//
bool pqxxSqlDriver::isSystemDatabaseName( const QString& n ) const
{
	return n.lower()=="template1" || n.lower()=="template0";
}

//==================================================================================
//
QString pqxxSqlDriver::escapeString( const QString& str) const
{
    return QString::fromLatin1("'")
    	+ QString::fromAscii( pqxx::sqlesc(std::string(str.utf8())).c_str() )
    	+ QString::fromLatin1("'");
}

//==================================================================================
//
QCString pqxxSqlDriver::escapeString( const QCString& str) const
{
    return QCString("'")
    	+ QCString( pqxx::sqlesc(QString(str).ascii()).c_str() )
    	+ QCString("'");
}

//==================================================================================
//
QString pqxxSqlDriver::drv_escapeIdentifier( const QString& str) const {
	return QString(str).replace( '"', "\"\"" );
}

//==================================================================================
//
QCString pqxxSqlDriver::drv_escapeIdentifier( const QCString& str) const {
	return QCString(str).replace( '"', "\"\"" );
}

//==================================================================================
//
QString pqxxSqlDriver::escapeBLOB(const QByteArray& array) const
{
	return KexiDB::escapeBLOB(array, KexiDB::BLOBEscapeOctal);
}

QString pqxxSqlDriver::valueToSQL( uint ftype, const QVariant& v ) const
{
	if (ftype==Field::Boolean) {
		// use SQL compliant TRUE or FALSE as described here
		// http://www.postgresql.org/docs/8.0/interactive/datatype-boolean.html
		// 1 or 0 does not work
		return v.toInt()==0 ? QString::fromLatin1("FALSE") : QString::fromLatin1("TRUE");
	}
	return Driver::valueToSQL(ftype, v);
}


#include "pqxxdriver.moc"
