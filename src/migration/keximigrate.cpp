/* This file is part of the KDE project
   Copyright (C) 2004 Adam Pigg <adam@piggz.co.uk>
   Copyright (C) 2004-2006 Jaroslaw Staniek <js@iidea.pl>
   Copyright (C) 2005 Martin Ellis <martin.ellis@kdemail.net>

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

#include "keximigrate.h"

#include <kdebug.h>
#include <kinputdialog.h>
#include <kapplication.h>

#include <kexiutils/identifier.h>
#include <core/kexi.h>
#include <core/kexiproject.h>
#include <kexidb/drivermanager.h>

using namespace KexiDB;
using namespace KexiMigration;

KexiMigrate::KexiMigrate(QObject *parent, const char *name,
  const QStringList&) 
  : QObject( parent, name )
  , m_migrateData(0)
  , m_destPrj(0)
{
	m_kexiDBCompatibleTableSchemasToRemoveFromMemoryAfterImport.setAutoDelete(true);
}

//! Used for computing progress: 
//! let's assume that each table creation costs the same as inserting 20 rows
#define NUM_OF_ROWS_PER_CREATE_TABLE 20


//=============================================================================
// Migration parameters
void KexiMigrate::setData(KexiMigration::Data* migrateData)
{
	m_migrateData = migrateData;
}

//=============================================================================
// Destructor
KexiMigrate::~KexiMigrate()
{
	delete m_destPrj;
}

bool KexiMigrate::checkIfDestinationDatabaseOverwritingNeedsAccepting(Kexi::ObjectStatus* result, 
	bool& acceptingNeeded)
{
	acceptingNeeded = false;
	if (result)
		result->clearStatus();

	KexiDB::DriverManager drvManager;
	KexiDB::Driver *destDriver = drvManager.driver(
		m_migrateData->destination->connectionData()->driverName);
	if (!destDriver) {
		result->setStatus(&drvManager,
			i18n("Could not create database \"%1\".")
			.arg(m_migrateData->destination->databaseName()));
		return false;
	}

	// For file-based dest. projects, we've already asked about overwriting 
	// existing project but for server-based projects we need to ask now.
	if (destDriver->isFileDriver())
		return true; //nothing to check
	KexiDB::Connection *tmpConn 
		= destDriver->createConnection( *m_migrateData->destination->connectionData() );
	if (!tmpConn || destDriver->error() || !tmpConn->connect()) {
		delete tmpConn;
		return true;
	}
	if (tmpConn->databaseExists( m_migrateData->destination->databaseName() )) {
		acceptingNeeded = true;
	}
	tmpConn->disconnect();
	delete tmpConn;
	return true;
}

bool KexiMigrate::isSourceAndDestinationDataSourceTheSame() const
{
	KexiDB::ConnectionData* sourcedata = m_migrateData->source;
	KexiDB::ConnectionData* destinationdata = m_migrateData->destination->connectionData();
	return (
		sourcedata && destinationdata &&
		m_migrateData->sourceName == m_migrateData->destination->databaseName() && // same database name
		sourcedata->driverName == destinationdata->driverName && // same driver
		sourcedata->hostName == destinationdata->hostName && // same host
		sourcedata->fileName() == destinationdata->fileName() && // same filename
		sourcedata->dbPath() == destinationdata->dbPath() && // same database path
		sourcedata->dbFileName() == destinationdata->dbFileName() // same database filename
	);
}

//=============================================================================
// Perform Import operation
bool KexiMigrate::performImport(Kexi::ObjectStatus* result)
{
	if (result)
		result->clearStatus();

	KexiDB::DriverManager drvManager;
	KexiDB::Driver *destDriver = drvManager.driver(
		m_migrateData->destination->connectionData()->driverName);
	if (!destDriver) {
		result->setStatus(&drvManager,
			i18n("Could not create database \"%1\".")
			.arg(m_migrateData->destination->databaseName()));
		return false;
	}

	QStringList tables;

	// Step 1 - connect
	kdDebug() << "KexiMigrate::performImport() CONNECTING..." << endl;
	if (!drv_connect()) {
		kdDebug() << "Couldnt connect to database server" << endl;
		if (result)
			result->setStatus(i18n("Could not connect to data source \"%1\".")
				.arg(m_migrateData->source->serverInfoString()), "");
		return false;
	}

	// Step 2 - get table names
	kdDebug() << "KexiMigrate::performImport() GETTING TABLENAMES..." << endl;
	if (!tableNames(tables)) {
		kdDebug() << "Couldnt get list of tables" << endl;
		if (result)
			result->setStatus(
				i18n("Could not get a list of table names for data source \"%1\".")
					.arg(m_migrateData->source->serverInfoString()), "");
		return false;
	}

	// Check if there are any tables
	if (tables.isEmpty()) {
		kdDebug() << "There were no tables to import" << endl;
		if (result)
			result->setStatus(
				i18n("No tables to import found in data source \"%1\".")
					.arg(m_migrateData->source->serverInfoString()), "");
		return false;
	}

	// Step 3 - Read table schemas
	tables.sort();
	m_tableSchemas.clear();
	if (!destDriver) {
		result->setStatus(&drvManager);
		return false;
	}
	const bool kexi__objects_exists = tables.find("kexi__objects")!=tables.end();
	QStringList kexiDBTables;
	if (kexi__objects_exists) {
		tristate res = drv_queryStringListFromSQL(
			QString::fromLatin1("SELECT o_name FROM kexi__objects WHERE o_type=%1")
		.arg((int)KexiDB::TableObjectType), 0, kexiDBTables, -1);
		if (res == true) {
			// prepend KexiDB-compatible tables to 'tables' list, so we'll copy KexiDB-compatible tables first,
			// to make sure existing IDs will not be in conflict with IDs newly generated for non-KexiDB tables
			kexiDBTables.sort();
			foreach(QStringList::ConstIterator, it, kexiDBTables)
				tables.remove( *it );
//kdDebug() << "KexiDB-compat tables: " << kexiDBTables << endl;
//kdDebug() << "non-KexiDB tables: " << tables << endl;
		}
	}

//	uint i=0;
	// -- read table schemas and create them in memory (only for non-KexiDB-compat tables)
	foreach (QStringList::ConstIterator, it, tables) {
		if (destDriver->isSystemObjectName( *it ) //"kexi__objects", etc.
			|| (*it).lower().startsWith("kexi__")) //tables at KexiProject level, e.g. "kexi__blobs"
			continue;
		// this is a non-KexiDB table: generate schema from native data source
		const QString tableName( KexiUtils::string2Identifier(*it) );
		KexiDB::TableSchema *tableSchema = new KexiDB::TableSchema(tableName);
		tableSchema->setCaption( *it ); //caption is equal to the original name

		if (!drv_readTableSchema(*it, *tableSchema)) {
			delete tableSchema;
			if (result)
				result->setStatus(
					i18n("Could not import project from data source \"%1\". Error reading table \"%2\".")
					.arg(m_migrateData->source->serverInfoString()).arg(tableName), "");
			return false;
		}
		//yeah, got a table
		//Add it to list of tables which we will create if all goes well
		m_tableSchemas.append(tableSchema);
	}

	// Step 4 - Create a new database as we have all required info
	// - create copies of KexiDB-compat tables
	// - create copies of non-KexiDB tables
	delete m_destPrj;
	m_destPrj = new KexiProject(m_migrateData->destination, 
		result ? (KexiDB::MessageHandler*)*result : 0);
	bool ok = true == m_destPrj->create(true /*forceOverwrite*/);

	KexiDB::Connection *destConn = 0;

	if (ok)
		ok = (destConn = m_destPrj->dbConnection());

	KexiDB::Transaction trans;
	if (ok) {
		trans = destConn->beginTransaction();
		if (trans.isNull()) {
			ok = false;
			if (result)
				result->setStatus(destConn,
					i18n("Could not create database \"%1\".")
					.arg(m_migrateData->destination->databaseName()));
			//later destConn->dropDatabase(m_migrateData->destination->databaseName());
			//don't delete prj, otherwise eror message will be deleted		delete prj;
			//later return m_destPrj;
		}
	}
	
	if (ok) {
		if (drv_progressSupported())
			progressInitialise();

		// Step 5 - Create the copies of KexiDB-compat tables in memory (to maintain the same IDs)
		m_kexiDBCompatibleTableSchemasToRemoveFromMemoryAfterImport.clear();
		foreach (QStringList::ConstIterator, it, kexiDBTables) {
			//load the schema from kexi__objects and kexi__fields
			TableSchema *t = new TableSchema();
			RowData data;
			bool firstRecord = true;
			if (true == drv_fetchRecordFromSQL(
					QString("SELECT o_id, o_type, o_name, o_caption, o_desc FROM kexi__objects "
					"WHERE o_name='%1' AND o_type=%1").arg(*it).arg((int)KexiDB::TableObjectType), 
					data, firstRecord)
				&& destConn->setupObjectSchemaData( data, *t ))
			{
//! @todo to reuse Connection::setupTableSchema()'s statement somehow...
				//load schema for every field and add it
				firstRecord = true;
				QString sql(
					QString::fromLatin1("SELECT t_id, f_type, f_name, f_length, f_precision, f_constraints, "
						"f_options, f_default, f_order, f_caption, f_help"
						" FROM kexi__fields WHERE t_id=%1 ORDER BY f_order").arg(t->id()) );
				while (ok) {
					tristate res = drv_fetchRecordFromSQL(sql, data, firstRecord);
					if (res != true) {
						if (false == res)
							ok = false;
						break;
					}
					KexiDB::Field* f = destConn->setupField( data );
					if (f)
						t->addField(f);
					else
						ok = false;
				}
				if (ok)
					ok = destConn->drv_createTable(*t);
				if (ok)
					m_kexiDBCompatibleTableSchemasToRemoveFromMemoryAfterImport.append(t);
			}
			if (!ok)
				delete t;
		}
	}

	// Step 6 - Copy kexi__objects NOW because we'll soon create new objects with new IDs (3.)...
	if (ok) {
		if (kexi__objects_exists)
			ok = drv_copyTable("kexi__objects", destConn, destConn->tableSchema("kexi__objects"));
	}

	// Step 7 - Create the non-KexiDB-compatible tables: new IDs will be assigned to them
	if (ok) {
		KexiDB::TableSchema *ts;
		for (QPtrListIterator<TableSchema> it (m_tableSchemas); (ts = it.current()); ++it) {
			ok = destConn->createTable( ts );
			if (!ok) {
				kdDebug() << "Failed to create a table " << ts->name() << endl;
				destConn->debugError();
				if (result)
					result->setStatus(destConn,
						i18n("Could not create database \"%1\".")
						.arg(m_migrateData->destination->databaseName()));
				m_tableSchemas.remove(ts);
				break;
			}
			updateProgress((Q_ULLONG)NUM_OF_ROWS_PER_CREATE_TABLE);
		}
	}

	if (ok)
		ok = destConn->commitTransaction(trans);

	if (ok) {
		//add compatible tables to the list, so data will be copied, if needed
		if (m_migrateData->keepData) {
			for(QPtrListIterator<TableSchema> it (m_kexiDBCompatibleTableSchemasToRemoveFromMemoryAfterImport);
				it.current(); ++it)
			{
				m_tableSchemas.append(it.current());
			}
		}
		else
			m_tableSchemas.clear();
	}

	if (ok) {
		if (m_destPrj->error()) {
			ok = false;
			if (result)
				result->setStatus(m_destPrj,
					i18n("Could not import project from data source \"%1\".")
					.arg(m_migrateData->source->serverInfoString()));
		}
	}

	// Step 8 - Copy data if asked to
	if (ok) {
		trans = destConn->beginTransaction();
		ok = !trans.isNull();
	}
	if (ok) {
		if (m_migrateData->keepData) {
//! @todo check detailed "copy forms/blobs/tables" flags here when we add them
			// Copy data for "kexi__objectdata" as well, if available in the source db
			if (tables.find("kexi__objectdata")!=tables.end())
				m_tableSchemas.append(destConn->tableSchema("kexi__objectdata")); 
			// Copy data for "kexi__blobs" as well, if available in the source db
			if (tables.find("kexi__blobs")!=tables.end())
				m_tableSchemas.append(destConn->tableSchema("kexi__blobs")); 
			// Copy data for "kexi__fields" as well, if available in the source db
			if (tables.find("kexi__fields")!=tables.end())
				m_tableSchemas.append(destConn->tableSchema("kexi__fields")); 
		}

		for(QPtrListIterator<TableSchema> ts(m_tableSchemas); ok && ts.current() != 0 ; ++ts)
		{
			const QString tname( ts.current()->name().lower() );
			if (destConn->driver()->isSystemObjectName( tname )
//! @todo what if these two tables are not compatible with tables created in destination db
//!       because newer db format was used?
				&& tname!="kexi__objectdata" //copy this too
				&& tname!="kexi__blobs" //copy this too
				&& tname!="kexi__fields" //copy this too
			)
			{
				kdDebug() << "Do not copy data for system table: " << tname << endl;
//! @todo copy kexi__db contents!
				continue;
			}
			kdDebug() << "Copying data for table: " << tname << endl;
			QString originalTableName;
			if (kexiDBTables.find(tname)==kexiDBTables.end())
				//caption is equal to the original name
				originalTableName = ts.current()->caption().isEmpty() ? tname : ts.current()->caption();
			else
				originalTableName = tname;
			ok = drv_copyTable(originalTableName, destConn, ts.current());
			if (!ok) {
				kdDebug() << "Failed to copy table " << tname << endl;
				if (result)
					result->setStatus(destConn,
						i18n("Could not copy table \"%1\" to destination database.").arg(tname));
				break;
			}
		}//for
	}

	// Done.
	if (ok)
		ok = destConn->commitTransaction(trans);

	if (ok)
		ok = drv_disconnect();

	m_kexiDBCompatibleTableSchemasToRemoveFromMemoryAfterImport.clear();

	if (ok) {
		if (destConn)
			ok = destConn->disconnect();
		return ok;
	}

	// Finally: error handling
	if (result && result->error())
		result->setStatus(destConn,
			i18n("Could not import data from data source \"%1\".")
				.arg(m_migrateData->source->serverInfoString()));
	if (destConn) {
		destConn->debugError();
		destConn->rollbackTransaction(trans);
	}
	drv_disconnect();
	if (destConn) {
		destConn->disconnect();
		destConn->dropDatabase(m_migrateData->destination->databaseName());
	}
	return false;
}
//=============================================================================

bool KexiMigrate::performExport(Kexi::ObjectStatus* result)
{
	if (result)
		result->clearStatus();

	//! @todo performExport

	return false;
}

//=============================================================================
// Functions for getting table data
bool KexiMigrate::tableNames(QStringList & tn)
{
	//! @todo Cache list of table names
	kdDebug() << "Reading list of tables..." << endl;
	return drv_tableNames(tn);
}

//=============================================================================
// Progress functions
bool KexiMigrate::progressInitialise() {
	Q_ULLONG sum = 0, size;
	emit progressPercent(0);

  //! @todo Don't copy table names here
	QStringList tables;
	if(!tableNames(tables))
		return false;

	// 1) Get the number of rows/bytes to import
	int tableNumber = 1;
	for(QStringList::Iterator it = tables.begin();
	    it != tables.end(); ++it, tableNumber++)
	{
		if(drv_getTableSize(*it, size)) {
			kdDebug() << "KexiMigrate::progressInitialise() - table: " << *it 
			          << "size: " << (ulong)size << endl;
			sum += size;
			emit progressPercent(tableNumber * 5 /* 5% */ / tables.count());
		} else {
			return false;
		}
	}

	kdDebug() << "KexiMigrate::progressInitialise() - job size: " << (ulong)sum << endl;
	m_progressTotal = sum;
	m_progressTotal += tables.count() * NUM_OF_ROWS_PER_CREATE_TABLE;
	m_progressTotal = m_progressTotal * 105 / 100; //add 5 percent for above task 1)
	m_progressNextReport = sum / 100;
	m_progressDone = m_progressTotal * 5 / 100; //5 perecent already done in task 1)
	return true;
}


void KexiMigrate::updateProgress(Q_ULLONG step) {
	m_progressDone += step;
	if (m_progressDone >= m_progressNextReport) {
		int percent = (m_progressDone+1) * 100 / m_progressTotal;
		m_progressNextReport = ((percent + 1) * m_progressTotal) / 100;
		kdDebug() << "KexiMigrate::updateProgress(): " << (ulong)m_progressDone << "/"
		          << (ulong)m_progressTotal << " (" << percent << "%) next report at " 
		          << (ulong)m_progressNextReport << endl;
		emit progressPercent(percent);
	}
}

//=============================================================================
// Prompt the user to choose a field type
KexiDB::Field::Type KexiMigrate::userType(const QString& fname)
{
	KInputDialog *dlg;
	QStringList  types;
	QString res;

	types << "Byte";
	types << "Short Integer";
	types << "Integer";
	types << "Big Integer";
	types << "Boolean";
	types << "Date";
	types << "Date Time";
	types << "Time";
	types << "Float";
	types << "Double";
	types << "Text";
	types << "Long Text";
	types << "Binary Large Object";

	res = dlg->getItem( i18n("Field Type"),
	                    i18n("The data type for %1 could not be determined. "
				 "Please select one of the following data "
				 "types").arg(fname),
                      types, 0, false);

//! @todo use QMap<QCString, KexiDB::Field::Type> here!
	if (res == *types.at(0))
		return KexiDB::Field::Byte;
	else if (res == *types.at(1))
		return KexiDB::Field::ShortInteger;
	else if (res == *types.at(2))
		return KexiDB::Field::Integer;
	else if (res == *types.at(3))
		return KexiDB::Field::BigInteger;
	else if (res == *types.at(4))
		return KexiDB::Field::Boolean;
	else if (res == *types.at(5))
		return KexiDB::Field::Date;
	else if (res == *types.at(6))
		return KexiDB::Field::DateTime;
	else if (res == *types.at(7))
		return KexiDB::Field::Time;
	else if (res == *types.at(8))
		return KexiDB::Field::Float;
	else if (res == *types.at(9))
		return KexiDB::Field::Double;
	else if (res == *types.at(10))
		return KexiDB::Field::Text;
	else if (res == *types.at(11))
		return KexiDB::Field::LongText;
	else if (res == *types.at(12))
		return KexiDB::Field::BLOB;
	else
		return KexiDB::Field::Text;
}

QVariant KexiMigrate::propertyValue( const QCString& propName )
{
	return m_properties[propName.lower()];
}

QString KexiMigrate::propertyCaption( const QCString& propName ) const
{
	return m_propertyCaptions[propName.lower()];
}

void KexiMigrate::setPropertyValue( const QCString& propName, const QVariant& value )
{
	m_properties[propName.lower()] = value;
}

QValueList<QCString> KexiMigrate::propertyNames() const
{
	QValueList<QCString> names = m_properties.keys();
	qHeapSort(names);
	return names;
}

bool KexiMigrate::isValid()
{
	if (KexiMigration::versionMajor() != versionMajor()
		|| KexiMigration::versionMinor() != versionMinor())
	{
		setError(ERR_INCOMPAT_DRIVER_VERSION,
		i18n("Incompatible migration driver's \"%1\" version: found version %2, expected version %3.")
		.arg(name())
		.arg(QString("%1.%2").arg(versionMajor()).arg(versionMinor()))
		.arg(QString("%1.%2").arg(KexiMigration::versionMajor()).arg(KexiMigration::versionMinor())));
		return false;
	}
	return true;
}

bool KexiMigrate::drv_queryMaxNumber(const QString& tableName, 
	const QString& columnName, int& result)
{
	QString string;
	tristate r = drv_querySingleStringFromSQL(
		QString::fromLatin1("SELECT MAX(%1) FROM %2").arg(drv_escapeIdentifier(columnName))
		.arg(drv_escapeIdentifier(tableName)), 0, string);
	if (r == false)
		return false;
	if (~r) {
		result = 0;
		return true;
	}
	bool ok;
	int tmpResult = string.toInt(&ok);
	if (ok)
		result = tmpResult;
	return ok;
}

tristate KexiMigrate::drv_querySingleStringFromSQL(
	const QString& sqlStatement, uint columnNumber, QString& string)
{
	QStringList stringList;
	const tristate res = drv_queryStringListFromSQL(sqlStatement, columnNumber, stringList, 1);
	if (true == res)
		string = stringList.first();
	return res;
}

#include "keximigrate.moc"
