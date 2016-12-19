/* This file is part of the KDE project
   Copyright (C) 2004 Adam Pigg <adam@piggz.co.uk>
   Copyright (C) 2004-2016 Jarosław Staniek <staniek@kde.org>
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
#include <core/kexi.h>
#include <core/kexiproject.h>

#include <KDbConnectionProxy>
#include <KDbDriverManager>
#include <KDbDriverMetaData>
#include <KDbProperties>
#include <KDbRecordData>
#include <KDbSqlResult>
#include <KDbVersionInfo>

#include <QInputDialog>
#include <QMutableListIterator>

using namespace KexiMigration;

class KexiMigrate::Private
{
public:
    Private()
      : metaData(nullptr)
      , migrateData(nullptr)
      , sourceConnection(nullptr)
    {
    }

    ~Private()
    {
        qDeleteAll(kexiDBCompatibleTableSchemasToRemoveFromMemoryAfterImport);
        kexiDBCompatibleTableSchemasToRemoveFromMemoryAfterImport.clear();
        delete migrateData;
    }

    QString couldNotCreateDatabaseErrorMessage() const
    {
        return xi18nc("@info", "Could not create database <resource>%1</resource>.",
                      migrateData->destinationProjectData()->databaseName());
    }

    //! Info about the driver's plugin
    const KexiMigratePluginMetaData *metaData;

    //! @todo Remove this! KexiMigrate should be usable for multiple concurrent migrations!
    //! Migrate Options
    KexiMigration::Data* migrateData;

    /*! Driver properties dictionary (indexed by name),
     useful for presenting properties to the user.
     Set available properties here in driver implementation. */
    QMap<QByteArray, QVariant> properties;

    /*! i18n'd captions for properties. You do not need
     to set predefined properties' caption in driver implementation
     -it's done automatically. */
    QMap<QByteArray, QString> propertyCaptions;

    //! KDb driver. For instance, it is used for escaping identifiers
    QPointer<KDbDriver> kexiDBDriver;

    /* private */

    //! Table schemas from source DB
    QList<KDbTableSchema*> tableSchemas;

    QList<KDbTableSchema*> kexiDBCompatibleTableSchemasToRemoveFromMemoryAfterImport;

    KDbConnectionProxy* sourceConnection;

    //! Size of migration job
    quint64 progressTotal;

    //! Amount of migration job complete
    quint64 progressDone;

    //! Don't recalculate progress done until this value is reached.
    quint64 progressNextReport;

};

KexiMigrate::KexiMigrate(QObject *parent, const QVariantList&)
        : QObject(parent)
        , KDbResultable()
        , d(new Private)
{
}

//! Used for computing progress:
//! let's assume that each table creation costs the same as inserting 20 rows
#define NUM_OF_ROWS_PER_CREATE_TABLE 20


//=============================================================================
// Migration parameters
KexiMigration::Data* KexiMigrate::data()
{
    return d->migrateData;
}

void KexiMigrate::setData(KexiMigration::Data* migrateData)
{
    if (d->migrateData && d->migrateData != migrateData) {
        delete d->migrateData;
    }
    d->migrateData = migrateData;
}

//=============================================================================
// Destructor
KexiMigrate::~KexiMigrate()
{
    disconnectInternal();
    delete d;
}

const KexiMigratePluginMetaData* KexiMigrate::metaData() const
{
    return d->metaData;
}

void KexiMigrate::setMetaData(const KexiMigratePluginMetaData *metaData)
{
    d->metaData = metaData;
    //! @todo KEXI3 d->initInternalProperties();
}

KDbConnectionProxy* KexiMigrate::sourceConnection()
{
    return d->sourceConnection;
}

bool KexiMigrate::checkIfDestinationDatabaseOverwritingNeedsAccepting(Kexi::ObjectStatus* result,
        bool *acceptingNeeded)
{
    Q_ASSERT(acceptingNeeded);
    *acceptingNeeded = false;
    if (result)
        result->clearStatus();

    KDbDriverManager drvManager;
    KDbDriver *destDriver = drvManager.driver(
                                d->migrateData->destinationProjectData()->connectionData()->driverId());
    if (!destDriver) {
        result->setStatus(drvManager.resultable(), d->couldNotCreateDatabaseErrorMessage());
        return false;
    }

    // For file-based dest. projects, we've already asked about overwriting
    // existing project but for server-based projects we need to ask now.
    if (destDriver->metaData()->isFileBased()) {
        return true; //nothing to check
    }
    QScopedPointer<KDbConnection> tmpConn(
        destDriver->createConnection(*d->migrateData->destinationProjectData()->connectionData()));
    if (!tmpConn || destDriver->result().isError() || !tmpConn->connect()) {
        m_result = destDriver->result();
        return true;
    }
    if (tmpConn->databaseExists(d->migrateData->destinationProjectData()->databaseName())) {
        *acceptingNeeded = true;
    }
    tmpConn->disconnect();
    return true;
}

bool KexiMigrate::isSourceAndDestinationDataSourceTheSame() const
{
    KDbConnectionData* sourcedata = d->migrateData->source;
    KDbConnectionData* destinationdata = d->migrateData->destinationProjectData()->connectionData();
    return
        sourcedata && destinationdata &&
        d->migrateData->sourceName == d->migrateData->destinationProjectData()->databaseName() && // same database name
        sourcedata->driverId() == destinationdata->driverId()&& // same driver
        sourcedata->hostName() == destinationdata->hostName() && // same host
        sourcedata->databaseName() == destinationdata->databaseName(); // same database name/filename
}

bool KexiMigrate::connectInternal(Kexi::ObjectStatus* result)
{
    Q_ASSERT(!d->sourceConnection);
    KDbConnection* conn = drv_createConnection();
    bool ok = !this->result().isError();
    if (ok) { // note: conn == nullptr does not mean failure
        if (conn) {
            d->sourceConnection = new KDbConnectionProxy(conn);
        }
        ok = drv_connect();
    }
    if (ok) {
        return true;
    }
    delete d->sourceConnection; // should not exist but do it for sanity
    d->sourceConnection = nullptr;
    QString message(xi18n("Could not connect to database %1.",
                          d->migrateData->sourceDatabaseInfoString()));
    qWarning() << message;
    if (result) {
        result->setStatus(this, message);
    }
    return false;
}

bool KexiMigrate::drv_connect()
{
    if (!d->sourceConnection) {
        return false;
    }
    if (!d->sourceConnection->drv_connect()
        || !d->sourceConnection->drv_useDatabase(data()->sourceName))
    {
        m_result = d->sourceConnection->result();
        return false;
    }
    return true;
}

bool KexiMigrate::disconnectInternal()
{
    const bool ok = drv_disconnect();
    if (!ok) {
        if (!m_result.isError()) {
            if (d->sourceConnection) {
                m_result = d->sourceConnection->result();
            }
        }
    }
    delete d->sourceConnection;
    d->sourceConnection = 0;
    return ok;
}

bool KexiMigrate::drv_disconnect()
{
    if (d->sourceConnection) {
        return d->sourceConnection->disconnect();
    }
    return false;
}

bool KexiMigrate::importTable(const QString& tableName, KDbConnectionProxy *destConn)
{
    QScopedPointer<KDbTableSchema> t(new KDbTableSchema());
    KDbEscapedString sqlStatement = KDbEscapedString(
        "SELECT o_id, o_type, o_name, o_caption, o_desc "
        "FROM kexi__objects WHERE o_name=%1 AND o_type=%2")
            .arg(d->sourceConnection->escapeString(tableName))
            .arg(int(KDb::TableObjectType));
    QScopedPointer<KDbRecordData> record;
    {
        QScopedPointer<KDbSqlResult> result(d->sourceConnection->executeSQL(sqlStatement));
        if (!result) {
            m_result = d->sourceConnection->result();
            return false;
        }
        record.reset(result->fetchRecordData());
        if (!record) {
            return !result->lastResult().isError();
        }
        if (!destConn->setupObjectData(*record, t.data())) {
            m_result = d->sourceConnection->result();
            return false;
        }
    }
    sqlStatement
        = KDbEscapedString("SELECT t_id, f_type, f_name, f_length, f_precision, f_constraints, "
                           "f_options, f_default, f_order, f_caption, f_help"
                           " FROM kexi__fields WHERE t_id=%1 ORDER BY f_order").arg(t->id());
    QVector<QList<QVariant>> fieldRecords;
    {
        QScopedPointer<KDbSqlResult> fieldsResult(d->sourceConnection->executeSQL(sqlStatement));
        if (!fieldsResult) {
            m_result = d->sourceConnection->result();
            return false;
        }
        Q_FOREVER {
            QScopedPointer<KDbRecordData> fieldsRecord(fieldsResult->fetchRecordData());
            if (!fieldsRecord) {
                if (!fieldsResult->lastResult().isError()) {
                    break;
                }
                m_result = fieldsResult->lastResult();
                return false;
            }
            QScopedPointer<KDbField> f(destConn->setupField(*fieldsRecord));
            if (!f) {
                return false;
            }
            QString testName(f->name());
            int i = 1;
            Q_FOREVER { // try to find unique name
                if (!t->field(testName)) {
                    break;
                }
                ++i;
                testName = f->name() + QString::number(i);
            }
            if (testName != f->name()) {
                f->setName(testName);
                if (!f->caption().isEmpty()) {
                    f->setCaption(QString::fromLatin1("%1 %2").arg(f->caption()).arg(i));
                }
            }
            if (!t->addField(f.data())) {
                return false;
            }
            f.take();
            fieldRecords.append(fieldsRecord->toList());
        }
    }
    if (!destConn->drv_createTable(*t)) {
        return false;
    }
    KDbTableSchema *kexi__objectsTable = destConn->tableSchema("kexi__objects");
    KDbTableSchema *kexi__fieldsTable = destConn->tableSchema("kexi__fields");
    if (!kexi__objectsTable || !kexi__fieldsTable) {
        return false;
    }
    // copy the kexi__objects record
    if (!destConn->insertRecord(kexi__objectsTable, record->toList())) {
        return false;
    }
    // copy the kexi__fields records
    for (const QList<QVariant> &fieldRecordData : fieldRecords) {
        if (!destConn->insertRecord(kexi__fieldsTable, fieldRecordData)) {
            return false;
        }
    }
    d->kexiDBCompatibleTableSchemasToRemoveFromMemoryAfterImport.append(t.take());
    return true;
}

bool KexiMigrate::performImport(Kexi::ObjectStatus* result)
{
    if (result)
        result->clearStatus();

    // Step 1 - connect
    qDebug() << "CONNECTING...";
    if (!connectInternal(result)) {
        return false;
    }

    // "Real" steps
    bool ok = performImportInternal(result);

    if (!disconnectInternal()) {
        ok = false;
    }
    return ok;
}

bool KexiMigrate::performImportInternal(Kexi::ObjectStatus* result)
{
    // Step 1 - destination driver
    KDbDriverManager drvManager;
    KDbDriver *destDriver = drvManager.driver(
                                     d->migrateData->destinationProjectData()->connectionData()->driverId());
    if (!destDriver) {
        result->setStatus(drvManager.resultable(), d->couldNotCreateDatabaseErrorMessage());
        return false;
    }

    // Step 2 - get table names
    qDebug() << "GETTING TABLENAMES...";
    QStringList tables;
    if (!tableNames(&tables)) {
        qWarning() << "Couldn't get list of tables";
        if (result)
            result->setStatus(
                xi18n("Could not get a list of table names for database %1.",
                      d->migrateData->sourceDatabaseInfoString()), QString());
        return false;
    }

    // Check if there are any tables
    if (tables.isEmpty()) {
        qDebug() << "There were no tables to import";
        if (result)
            result->setStatus(
                xi18n("No tables have been found in database %1.",
                      d->migrateData->sourceDatabaseInfoString()), QString());
        return false;
    }

    // Step 3 - Read KDb-compatible table schemas
    tables.sort();
    d->tableSchemas.clear();
    const bool kexi__objects_exists = tables.contains("kexi__objects");
    QStringList kexiDBTables;
    if (kexi__objects_exists) {
        tristate res = drv_queryStringListFromSQL(
                           KDbEscapedString("SELECT o_name FROM kexi__objects WHERE o_type=%1")
                           .arg(int(KDb::TableObjectType)), 0, &kexiDBTables, -1);
        if (res == true) {
            // Skip KDb-compatible schemas that have no physical tables
            QMutableListIterator<QString> kdbTablesIt(kexiDBTables);
            while (kdbTablesIt.hasNext()) {
                if (true != d->sourceConnection->resultExists(KDbEscapedString("SELECT * FROM %1")
                        .arg(sourceConnection()->escapeIdentifier(kdbTablesIt.next()))))
                {
                    qDebug() << "KDb table does not exist:" << kdbTablesIt.value();
                    kdbTablesIt.remove();
                }
            }
            // Separate KDb-compatible tables from the KDb-incompatible tables
            // so KDb-compatible tables can be later deeply copied without altering their IDs.
            kexiDBTables.sort();
            const QSet<QString> kdbTablesSet(kexiDBTables.toSet());
            QMutableListIterator<QString> tablesIt(tables);
            while (tablesIt.hasNext()) {
                if (kdbTablesSet.contains(tablesIt.next())) {
                    tablesIt.remove();
                }
            }
        //qDebug() << "KDb-compatible tables: " << kexiDBTables;
        //qDebug() << "non-KDb tables: " << tables;
        }
    }

    // -- read non-KDb-compatible tables schemas and create them in memory
    QMap<QString, QString> nativeNames;
    foreach(const QString& tableCaption, tables) {
        if (destDriver->isSystemObjectName(tableCaption)
            || KDbDriver::isKDbSystemObjectName(tableCaption) // "kexi__objects", etc.
               // other "kexi__*" tables at KexiProject level, e.g. "kexi__blobs"
            || tableCaption.startsWith(QLatin1String("kexi__"), Qt::CaseInsensitive))
        {
            continue;
        }
        // this is a non-KDb table: generate schema from native data source
        const QString tableIdentifier(KDb::stringToIdentifier(tableCaption.toLower()));
        nativeNames.insert(tableIdentifier, tableCaption);
        QScopedPointer<KDbTableSchema> tableSchema(new KDbTableSchema(tableIdentifier));
        tableSchema->setCaption(tableCaption);   //caption is equal to the original name

        if (!drv_readTableSchema(tableCaption, tableSchema.data())) {
            if (result)
                result->setStatus(
                    xi18nc("@info",
                           "Could not import project from database %1. Error reading table <resource>%2</resource>.",
                           d->migrateData->sourceDatabaseInfoString(), tableCaption), QString());
            return false;
        }
        //yeah, got a table
        //Add it to list of tables which we will create if all goes well
        d->tableSchemas.append(tableSchema.take());
    }

    // Step 4 - Create a new database as we have all required info
    KexiProject destProject(
        *d->migrateData->destinationProjectData(), result ? (KDbMessageHandler*)*result : 0);
    bool ok = true == destProject.create(true /*forceOverwrite*/)
              && destProject.dbConnection();

    QScopedPointer<KDbConnectionProxy> destConn;

    if (ok) {
        destConn.reset(new KDbConnectionProxy(destProject.dbConnection()));
        destConn->setParentConnectionIsOwned(false);
    }

    KDbTransaction trans;
    if (ok) {
        trans = destConn->beginTransaction();
        if (trans.isNull()) {
            ok = false;
            if (result) {
                result->setStatus(destConn->parentConnection(), d->couldNotCreateDatabaseErrorMessage());
            }
        }
    }

    if (ok) {
        if (drv_progressSupported()) {
            ok = progressInitialise();
        }
    }

    if (ok) {
        // Step 5 - Create the copies of KDb-compatible tables in memory (to maintain the same IDs)
        // Step 6.1 - Copy kexi__objects NOW because we'll soon create new objects with new IDs
        // Step 6.2 - Copy kexi__fields
        d->kexiDBCompatibleTableSchemasToRemoveFromMemoryAfterImport.clear();
        foreach(const QString& tableName, kexiDBTables) {
            if (!importTable(tableName, destConn.data())) {
                if (!m_result.isError()) {
                    m_result.setCode();
                }
                m_result.prependMessage(
                    xi18nc("@info", "Could not import table <resource>%1</resource>.", tableName));
                return false;
            }
        }
    }

    // Step 7 - Create non-KDb-compatible tables: new IDs will be assigned to them
    if (ok) {
        foreach(KDbTableSchema* ts, d->tableSchemas) {
            ok = destConn->createTable(ts);
            if (!ok) {
                qWarning() << "Failed to create a table " << ts->name();
                qWarning() << destConn->result();
                if (result) {
                    result->setStatus(destConn->parentConnection()->result(), nullptr,
                                      d->couldNotCreateDatabaseErrorMessage());
                }
                d->tableSchemas.removeAt(d->tableSchemas.indexOf(ts));
                break;
            }
            updateProgress((qulonglong)NUM_OF_ROWS_PER_CREATE_TABLE);
        }
    }

    if (ok)
        ok = destConn->commitTransaction(trans);

    if (ok) {
        //add KDb-compatible tables to the list, so data will be copied, if needed
        if (d->migrateData->keepData) {
            foreach(KDbTableSchema* table,
                    d->kexiDBCompatibleTableSchemasToRemoveFromMemoryAfterImport) {
                d->tableSchemas.append(table);
            }
        } else
            d->tableSchemas.clear();
    }

    if (ok) {
        if (destProject.result().isError()) {
            ok = false;
            if (result)
                result->setStatus(destProject.result(), nullptr,
                                  xi18n("Could not import project from data source %1.",
                                       d->migrateData->sourceDatabaseInfoString()));
        }
    }

    // Step 8 - Copy data if asked to
    if (ok) {
        trans = destConn->beginTransaction();
        ok = !trans.isNull();
    }
    if (ok) {
        if (d->migrateData->keepData) {
//! @todo check detailed "copy forms/blobs/tables" flags here when we add them
//! @todo don't copy kexi__objectdata and kexi__userdata for tables that do not exist
            // Copy data for "kexi__objectdata" as well, if available in the source db
            if (tables.contains("kexi__objectdata"))
                d->tableSchemas.append(destConn->tableSchema("kexi__objectdata"));
            // Copy data for "kexi__blobs" as well, if available in the source db
            if (tables.contains("kexi__blobs"))
                d->tableSchemas.append(destConn->tableSchema("kexi__blobs"));
        }

        foreach(KDbTableSchema *ts, d->tableSchemas) {
            if (!ok)
                break;
            if ((destConn->driver()->isSystemObjectName(ts->name())
                 || KDbDriver::isKDbSystemObjectName(ts->name()))
//! @todo what if these two tables are not compatible with tables created in destination db
//!       because newer db format was used?
                    && ts->name() != "kexi__objectdata" //copy this too
                    && ts->name() != "kexi__blobs" //copy this too
                    && ts->name() != "kexi__userdata" //copy this too
               )
            {
                qDebug() << "Won't copy data to system table" << ts->name();
//! @todo copy kexi__db contents!
                continue;
            }
            QString tsName = nativeNames.value(ts->name());
            qDebug() << "Copying data for table: " << tsName;
            if (tsName.isEmpty()) {
                tsName = ts->name();
            }
            ok = drv_copyTable(tsName, destConn->parentConnection(), ts);
            if (!ok) {
                qWarning() << "Failed to copy table " << tsName;
                if (result)
                    result->setStatus(destConn->parentConnection()->result(), nullptr,
                                      xi18nc("@info",
                                             "Could not copy table <resource>%1</resource> to destination database.", tsName));
                break;
            }
        }//for
    }

    // Done.
    if (ok)
        ok = destConn->commitTransaction(trans);

    d->kexiDBCompatibleTableSchemasToRemoveFromMemoryAfterImport.clear();

    if (ok) {
        if (destConn)
            ok = destConn->disconnect();
        return ok;
    }

    // Finally: error handling
    if (result && result->error())
        result->setStatus(destConn->parentConnection()->result(), nullptr,
                          xi18n("Could not import data from data source %1.",
                               d->migrateData->sourceDatabaseInfoString()));
    if (destConn) {
        qWarning() << destConn->result();
        destConn->rollbackTransaction(trans);
        destConn->disconnect();
        destConn->dropDatabase(d->migrateData->destinationProjectData()->databaseName());
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
// Progress functions
bool KexiMigrate::progressInitialise()
{
    emit progressPercent(0);

    //! @todo Don't copy table names here
    QStringList tables;
    if (!tableNames(&tables))
        return false;

    // 1) Get the number of rows/bytes to import
    int tableNumber = 1;
    quint64 sum = 0;
    foreach(const QString& tableName, tables) {
        quint64 size;
        if (drv_getTableSize(tableName, &size)) {
            qDebug() << "table:" << tableName << "size: " << (ulong)size;
            sum += size;
            emit progressPercent(tableNumber * 5 /* 5% */ / tables.count());
            tableNumber++;
        } else {
            return false;
        }
    }

    qDebug() << "job size:" << sum;
    d->progressTotal = sum;
    d->progressTotal += tables.count() * NUM_OF_ROWS_PER_CREATE_TABLE;
    d->progressTotal = d->progressTotal * 105 / 100; //add 5 percent for above task 1)
    d->progressNextReport = sum / 100;
    d->progressDone = d->progressTotal * 5 / 100; //5 perecent already done in task 1)
    return true;
}


void KexiMigrate::updateProgress(qulonglong step)
{
    d->progressDone += step;
    if (d->progressTotal > 0 && d->progressDone >= d->progressNextReport) {
        int percent = (d->progressDone + 1) * 100 / d->progressTotal;
        d->progressNextReport = ((percent + 1) * d->progressTotal) / 100;
        qDebug() << (ulong)d->progressDone << "/"
            << (ulong)d->progressTotal << " (" << percent << "%) next report at"
            << (ulong)d->progressNextReport;
        emit progressPercent(percent);
    }
}

//=============================================================================
// Prompt the user to choose a field type
KDbField::Type KexiMigrate::userType(const QString& fname)
{
    const QStringList typeNames(KDbField::typeNames());
    bool ok;
    const QString res
        = QInputDialog::getItem(nullptr, xi18nc("@title:window", "Field Type"),
            xi18nc("@info",
                   "The data type for field <resource>%1</resource> could not be determined. "
                   "Please select one of the following data types.", fname),
            typeNames, 0, false/* !editable */, &ok);

    if (!ok || res.isEmpty())
//! @todo OK?
        return KDbField::Text;

    return KDb::intToFieldType(int(KDbField::FirstType) + typeNames.indexOf(res));
}

QString KexiMigrate::drv_escapeIdentifier(const QString& str) const
{
    return d->kexiDBDriver ? d->kexiDBDriver->escapeIdentifier(str) : str;
}

KDbDriver *KexiMigrate::driver()
{
    return d->kexiDBDriver;
}

void KexiMigrate::setDriver(KDbDriver *driver)
{
    d->kexiDBDriver = driver;
}

QVariant KexiMigrate::propertyValue(const QByteArray& propertyName)
{
    return d->properties.value(propertyName.toLower());
}

QString KexiMigrate::propertyCaption(const QByteArray& propertyName) const
{
    return d->propertyCaptions.value(propertyName.toLower());
}

void KexiMigrate::setPropertyValue(const QByteArray& propertyName, const QVariant& value)
{
    d->properties.insert(propertyName.toLower(), value);
}

void KexiMigrate::setPropertyCaption(const QByteArray& propertyName, const QString &caption)
{
    d->propertyCaptions.insert(propertyName.toLower(), caption);
}

QList<QByteArray> KexiMigrate::propertyNames() const
{
    QList<QByteArray> names = d->properties.keys();
    qSort(names);
    return names;
}

/* moved to MigrateManagerInternal::driver():
bool KexiMigrate::isValid()
{
    if (KexiMigration::versionMajor() != versionMajor()
            || KexiMigration::versionMinor() != versionMinor()) {
        setError(ERR_INCOMPAT_DRIVER_VERSION,
                 xi18n(
                     "Incompatible migration driver's \"%1\" version: found version %2, expected version %3.",
                     objectName(),
                     QString("%1.%2").arg(versionMajor()).arg(versionMinor()),
                     QString("%1.%2").arg(KexiMigration::versionMajor()).arg(KexiMigration::versionMinor()))
                );
        return false;
    }
    return true;
}
*/

bool KexiMigrate::drv_queryMaxNumber(const QString& tableName,
                                     const QString& columnName, int *result)
{
    QString string;
    tristate r = drv_querySingleStringFromSQL(
                     KDbEscapedString("SELECT MAX(%1) FROM %2")
                     .arg(drv_escapeIdentifier(columnName))
                     .arg(drv_escapeIdentifier(tableName)), 0, &string);
    if (r == false)
        return false;
    if (~r) {
        result = 0;
        return true;
    }
    bool ok;
    int tmpResult = string.toInt(&ok);
    if (ok)
        *result = tmpResult;
    return ok;
}

tristate KexiMigrate::drv_querySingleStringFromSQL(
    const KDbEscapedString& sqlStatement, int columnNumber, QString *string)
{
    QStringList stringList;
    const tristate res = drv_queryStringListFromSQL(sqlStatement, columnNumber, &stringList, 1);
    if (true == res)
        *string = stringList.first();
    return res;
}

bool KexiMigrate::connectSource(Kexi::ObjectStatus* result)
{
    return connectInternal(result);
}

bool KexiMigrate::disconnectSource()
{
    return disconnectInternal();
}

bool KexiMigrate::readTableSchema(const QString& originalName, KDbTableSchema *tableSchema)
{
  return drv_readTableSchema(originalName, tableSchema);
}

bool KexiMigrate::tableNames(QStringList *tn)
{
    //! @todo Cache list of table names
    qDebug() << "Reading list of tables...";
    tn->clear();
    return drv_tableNames(tn);
}

KDbSqlResult* KexiMigrate::readFromTable(const QString & tableName)
{
  return drv_readFromTable(tableName);
}

bool KexiMigrate::moveNext()
{
  return drv_moveNext();
}

bool KexiMigrate::movePrevious()
{
  return drv_movePrevious();
}

bool KexiMigrate::moveFirst()
{
  return drv_moveFirst();
}

bool KexiMigrate::moveLast()
{
  return drv_moveLast();
}

QVariant KexiMigrate::value(int i)
{
  return drv_value(i);
}

//------------------------

KDbVersionInfo KexiMigration::version()
{
    return KDbVersionInfo(KEXI_MIGRATION_VERSION_MAJOR, KEXI_MIGRATION_VERSION_MINOR, 0);
}

