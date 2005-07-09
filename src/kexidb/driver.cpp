/* This file is part of the KDE project
   Copyright (C) 2003-2004 Jaroslaw Staniek <js@iidea.pl>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <kexidb/driver.h>
#include <kexidb/driver_p.h>
#include <kexidb/drivermanager.h>
#include <kexidb/drivermanager_p.h>
#include "error.h"
#include "drivermanager.h"
#include "connection.h"
#include "connectiondata.h"

#include <qfileinfo.h>

#include <klocale.h>
#include <kdebug.h>

#include <assert.h>

using namespace KexiDB;

/*! used when we do not have Driver instance yet,
 or when we cannot get one */
QValueVector<QString> dflt_typeNames;


//---------------------------------------------


DriverBehaviour::DriverBehaviour()
	: UNSIGNED_TYPE_KEYWORD("UNSIGNED")
	, AUTO_INCREMENT_FIELD_OPTION("AUTO_INCREMENT")
	, AUTO_INCREMENT_PK_FIELD_OPTION("AUTO_INCREMENT PRIMARY KEY")
	, SPECIAL_AUTO_INCREMENT_DEF(false)
	, AUTO_INCREMENT_REQUIRES_PK(false)
	, ROW_ID_FIELD_RETURNS_LAST_AUTOINCREMENTED_VALUE(false)
	, QUOTATION_MARKS_FOR_IDENTIFIER('"')
	, USING_DATABASE_REQUIRED_TO_CONNECT(true)
	, _1ST_ROW_READ_AHEAD_REQUIRED_TO_KNOW_IF_THE_RESULT_IS_EMPTY(false)
	, SELECT_1_SUBQUERY_SUPPORTED(false)
	, SQL_KEYWORDS(0)
{

}

//---------------------------------------------

Driver::Driver( QObject *parent, const char *name, const QStringList & )
	: QObject( parent, name )
	, Object()
	, beh( new DriverBehaviour() )
	, d( new DriverPrivate() )
{
	d->connections.setAutoDelete(false);
	//TODO: reasonable size
	d->connections.resize(101);
	d->typeNames.resize(Field::LastType + 1);

	d->initKexiKeywords();
}


Driver::~Driver()
{
	DriverManagerInternal::self()->aboutDelete( this );
//	KexiDBDbg << "Driver::~Driver()" << endl;
	QPtrDictIterator<Connection> it( d->connections );
	Connection *conn;
	while ( (conn = it.toFirst()) ) {
		delete conn;
	}
	delete beh;
	delete d;
//	KexiDBDbg << "Driver::~Driver() ok" << endl;
}

bool Driver::isValid()
{
	clearError();
	if (KexiDB::versionMajor() != versionMajor()
		|| KexiDB::versionMinor() != versionMinor())
	{
		setError(ERR_INCOMPAT_DRIVER_VERSION,
		i18n("Incompatible database driver's \"%1\" version: found version %2, expected version %3.")
		.arg(name())
		.arg(QString("%1.%2").arg(versionMajor()).arg(versionMinor()))
		.arg(QString("%1.%2").arg(KexiDB::versionMajor()).arg(KexiDB::versionMinor())));
		return false;
	}

	QString inv_impl = i18n("Invalid database driver's \"%1\" implementation:\n").arg(name());
	QString not_init = i18n("Value of \"%1\" is not initialized for the driver.");
	if (beh->ROW_ID_FIELD_NAME.isEmpty()) {
		setError(ERR_INVALID_DRIVER_IMPL, inv_impl + not_init.arg("DriverBehaviour::ROW_ID_FIELD_NAME"));
		return false;
	}

	return true;
}

const QPtrList<Connection> Driver::connectionsList() const
{
	QPtrList<Connection> clist;
	QPtrDictIterator<Connection> it( d->connections );
	for( ; it.current(); ++it )
		clist.append( &(*it) );
	return clist;
}

QString Driver::fileDBDriverMimeType() const
{ return d->fileDBDriverMimeType; }

QString Driver::defaultFileBasedDriverMimeType()
{ return QString::fromLatin1("application/x-kexiproject-sqlite3"); }

QString Driver::defaultFileBasedDriverName()
{
	DriverManager dm;
	return dm.lookupByMime(Driver::defaultFileBasedDriverMimeType()).lower();
}

const KService* Driver::service() const
{ return d->service; }

bool Driver::isFileDriver() const
{ return d->isFileDriver; }

int Driver::features() const
{ return d->features; }

bool Driver::transactionsSupported() const
{ return d->features & (SingleTransactions | MultipleTransactions); }

QString Driver::sqlTypeName(int id_t, int /*p*/) const
{
	if (id_t==Field::Null)
		return "Null";
	return d->typeNames[id_t];
}

Connection *Driver::createConnection( ConnectionData &conn_data )
{
	clearError();
	if (!isValid())
		return 0;

	if (d->isFileDriver) {
		if (conn_data.fileName().isEmpty()) {
			setError(ERR_MISSING_DB_LOCATION, i18n("File name expected for file-based database driver.") );
			return 0;
		}
	}
//	Connection *conn = new Connection( this, conn_data );
	Connection *conn = drv_createConnection( conn_data );
	conn_data.driverName = name();
	d->connections.insert( conn, conn );
	return conn;
}

Connection* Driver::removeConnection( Connection *conn )
{
	clearError();
	return d->connections.take( conn );
}

QString Driver::defaultSQLTypeName(int id_t)
{
	if (id_t==Field::Null)
		return "Null";
	if (dflt_typeNames.isEmpty()) {
		dflt_typeNames.resize(Field::LastType + 1);
		dflt_typeNames[Field::Byte]="Byte";
		dflt_typeNames[Field::ShortInteger]="ShortInteger";
		dflt_typeNames[Field::Integer]="Integer";
		dflt_typeNames[Field::BigInteger]="BigInteger";
		dflt_typeNames[Field::Boolean]="Boolean";
		dflt_typeNames[Field::Date]="Date";
		dflt_typeNames[Field::DateTime]="DateTime";
		dflt_typeNames[Field::Time]="Time";
		dflt_typeNames[Field::Float]="Float";
		dflt_typeNames[Field::Double]="Double";
		dflt_typeNames[Field::Text]="Text";
		dflt_typeNames[Field::LongText]="Text";
		dflt_typeNames[Field::BLOB]="BLOB";
	}
	return dflt_typeNames[id_t];
}

bool Driver::isSystemObjectName( const QString& n ) const
{
	return Driver::isKexiDBSystemObjectName(n);
}

bool Driver::isKexiDBSystemObjectName( const QString& n )
{
	return n.lower().startsWith("kexi__");
}

bool Driver::isSystemFieldName( const QString& n ) const
{
	if (!beh->ROW_ID_FIELD_NAME.isEmpty() && n.lower()==beh->ROW_ID_FIELD_NAME.lower())
		return true;
	return drv_isSystemFieldName(n);
}

QString Driver::valueToSQL( uint ftype, const QVariant& v ) const
{
	if (v.isNull())
		return "NULL";
	switch (ftype) {
		case Field::Byte:
		case Field::ShortInteger:
		case Field::Integer:
		case Field::Float:
		case Field::Double:
		case Field::BigInteger:
			return v.toString();
//TODO: here special encoding method needed
		case Field::Boolean:
			return QString::number(v.toInt()?1:0); //0 or 1
		case Field::Time:
			return QString("\'")+v.toTime().toString(Qt::ISODate)+"\'";
		case Field::Date:
			return QString("\'")+v.toDate().toString(Qt::ISODate)+"\'";
		case Field::DateTime:
			return dateTimeToSQL( v.toDateTime() );
		case Field::Text:
		case Field::LongText: {
			QString s = v.toString();
			return escapeString(s); //QString("'")+s.replace( '"', "\\\"" ) + "'";
		}
		case Field::BLOB: {
//TODO: here special encoding method needed
			QString s = v.toString();
			return escapeString(s); //QString("'")+v.toString()+"'";
		}
		case Field::InvalidType:
			return "!INVALIDTYPE!";
		default:
			KexiDBDbg << "Driver::valueToSQL(): UNKNOWN!" << endl;
			return QString::null;
	}
	return QString::null;
}

QVariant Driver::propertyValue( const QCString& propName ) const
{
	return d->properties[propName.lower()];
}

QString Driver::propertyCaption( const QCString& propName ) const
{
	return d->propertyCaptions[propName.lower()];
}

QValueList<QCString> Driver::propertyNames() const
{
	QValueList<QCString> names = d->properties.keys();
	qHeapSort(names);
	return names;
}

QString Driver::escapeIdentifier(const QString& str, int options) const
{
	QCString cstr = str.latin1();
	return QString(escapeIdentifier(cstr, options));
}

QCString Driver::escapeIdentifier(const QCString& str, int options) const
{
	bool needOuterQuotes = false;

// Need to use quotes if ...
// ... we have been told to, or ...
	if(options & EscapeAlways)
		needOuterQuotes = true;

// ... or if the driver does not have a list of keywords,
	else if(!d->driverSQLDict)
		needOuterQuotes = true;

// ... or if it's a keyword in Kexi's SQL dialect,
	else if(d->kexiSQLDict->find(str))
		needOuterQuotes = true;

// ... or if it's a keyword in the backends SQL dialect,
// (have already checked !d->driverSQLDict)
	else if((options & EscapeDriver) && d->driverSQLDict->find(str))
		needOuterQuotes = true;

// ... or if the identifier has a space in it...
  else if(str.find(' ') != -1)
		needOuterQuotes = true;

	if(needOuterQuotes && (options & EscapeKexi)) {
		const char quote = '"';
		return quote + QCString(str).replace( quote, "\"\"" ) + quote;
	}
	else if (needOuterQuotes) {
		const char quote = beh->QUOTATION_MARKS_FOR_IDENTIFIER.latin1();
		return quote + drv_escapeIdentifier(str) + quote;
	} else {
		return drv_escapeIdentifier(str);
	}
}

void Driver::initSQLKeywords(int hashSize) {

	if(!d->driverSQLDict && beh->SQL_KEYWORDS != 0) {
	  d->initDriverKeywords(beh->SQL_KEYWORDS, hashSize);
	}
}

#include "driver.moc"

