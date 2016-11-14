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

#ifndef KEXI_MIGRATE_H
#define KEXI_MIGRATE_H

#include "keximigratedata.h"
#include "KexiVersion.h"

#include <KDbConnection>
#include <KDbSqlRecord>
#include <KDbTableSchema>

#include <QVariantList>

class KDbConnectionProxy;

namespace Kexi
{
class ObjectStatus;
}
class KexiMigratePluginMetaData;

/*! KexiMigration implementation version.
 It is altered after every change:
 - major number is increased after incompatible change ofthe plugin interface/behavior
 - minor number is increased after minor changes
 @note Do not use these constants to get library version information in external code.
       Use KexiMigratePluginMetaData::*version() functions instead.
*/
#define KEXI_MIGRATION_VERSION_MAJOR KEXI_STABLE_VERSION_MAJOR
#define KEXI_MIGRATION_VERSION_MINOR KEXI_STABLE_VERSION_MINOR

/*!
 * \namespace KexiMigration
 * \brief Framework for importing databases into native KDb databases.
 */
namespace KexiMigration
{

//! \return KexiMigration library version info
KEXIMIGRATE_EXPORT KDbVersionInfo version();

//! @short A base class for a migrate plugin that imports native databases into Kexi projects
/*! A generic API for importing schema and data from an existing
database into a new Kexi project. Can be also used for importing native Kexi databases.

Basic idea is this:
-# User selects an existing DB and new project (file or server based)
-# User specifies whether to import structure and data or structure only.
-# Import tool connects to db
-# Checks if it is already a kexi project (not implemented yet)
-# If not, then read structure and construct new project
-# Ask user what to do if column type is not supported

See kexi/doc/dev/kexi_import.txt for more info.
*/
class KEXIMIGRATE_EXPORT KexiMigrate : public QObject, public KDbResultable
{
    Q_OBJECT

public:
    virtual ~KexiMigrate();

    //! Info about the driver's plugin
    const KexiMigratePluginMetaData* metaData() const;

//! @todo Remove this! KexiMigrate should be usable for multiple concurrent migrations!
    KexiMigration::Data* data();

//! @todo Remove this! KexiMigrate should be usable for multiple concurrent migrations!
    //! Data Setup.  Requires two connection objects, a name and a bool
    void setData(KexiMigration::Data* migrateData);

    /*! Checks whether the destination database exists.
     For file-based dest. projects, we've already asked about overwriting
     existing project but for server-based projects it's better to ask user.
     This method should be called before performImport() or performExport().

     \return true if no connection-related errors occurred.
     \a acceptingNeeded is set to true if destination database exists.
     In this case you should ask about accepting database overwriting.
     Used in ImportWizard::import(). */
    bool checkIfDestinationDatabaseOverwritingNeedsAccepting(Kexi::ObjectStatus* result,
            bool* acceptingNeeded);

    /*! Checks if the source- and the destination databases are identical.
    \return true if they are identical else false. */
    bool isSourceAndDestinationDataSourceTheSame() const;

    //! Connects, perform an import operation, and disconnects
    bool performImport(Kexi::ObjectStatus* result = 0);

    //! Perform an export operation
    bool performExport(Kexi::ObjectStatus* result = 0);

    //! Returns true if the migration driver supports progress updates.
    inline bool progressSupported() {
        return drv_progressSupported();
    }

//! @todo This is copied from KDbDriver. One day it will be merged with KDb.
    //! \return property value for \a propertyName available for this driver.
    //! If there's no such property defined for driver, Null QVariant value is returned.
    virtual QVariant propertyValue(const QByteArray& propertyName);

//! @todo This is copied from KDbDriver. One day it will be merged with KDb.
    void setPropertyValue(const QByteArray& propertyName, const QVariant& value);

//! @todo This is copied from KDbDriver. One day it will be merged with KDb.
    //! \return translated property caption for \a propertyName.
    //! If there's no such property defined for driver, empty string value is returned.
    QString propertyCaption(const QByteArray& propertyName) const;

    //! @todo This is copied from KDbDriver. One day it will be merged with KDb.
    //! Set translated property caption for \a propertyName.
    void setPropertyCaption(const QByteArray& propertyName, const QString &caption);

//! @todo This is copied from KDbDriver. One day it will be merged with KDb.
    //! \return a list of property names available for this driver.
    QList<QByteArray> propertyNames() const;

    //! Extension of existing API to provide generic row access to external data for ImportTableWizard.
    //! @todo refactor
    bool connectSource(Kexi::ObjectStatus* result);

    //! Extension of existing API to close connection for ImportTableWizard.
    //! @todo refactor
    bool disconnectSource();

    //! Get table names in source database (driver specific)
    bool tableNames(QStringList *tablenames);

    //! Read schema for a given table (driver specific)
    bool readTableSchema(const QString& originalName, KDbTableSchema *tableSchema);

    //! Starts reading data from the source dataset's table
    KDbSqlResult* readFromTable(const QString& tableName);

    //!Move to the next row
    bool moveNext();

    //!Move to the previous row
    bool movePrevious();

    //!Move to the next row
    bool moveFirst();

    //!Move to the previous row
    bool moveLast();

    //!Read the data at the given row/field
    QVariant value(int i);

Q_SIGNALS:
    void progressPercent(int percent);

protected:
    //! Used by MigrateManager.
    explicit KexiMigrate(QObject *parent, const QVariantList &args = QVariantList());

    /*! Used by the migration driver manager to set metaData for just loaded driver. */
    void setMetaData(const KexiMigratePluginMetaData *metaData);

    //! Creates connection to source database and connects.
    //! If it is a database supported by low-level routines of KDb, sourceConnection() will
    //! be available afterwards.
    //! If not, connectInternal() can still return true but sourceConnection() will return
    //! @c nullptr.
    //! @see drv_createConnection() drv_connect()
    bool connectInternal(Kexi::ObjectStatus* result);

    //! Disconnects from source database. If it is a database supported by low-level routines
    //! of KDb, destroys the connection object pointed by sourceConnection() too.
    //! @return true on success.
    //! @see drv_disconnect() connectInternal()
    bool disconnectInternal();

    //! @return connection to source database if it is a database supported by low-level
    //! routines of KDb. In other cases such as importing from a TSV file, this function
    //! returns @c nullptr.
    KDbConnectionProxy* sourceConnection();

    //! Migration drivers that use low-level routines KDb to access the source database
    //! should create and return a driver-specific KDbConnection object that handles
    //! connection to the source database.
    //! Migration drivers that use custom data sources (not KDb-compatible) should return
    //! @c nullptr.
    //! @note KexiMigrate::m_result should be set to a result of the operation.
    virtual KDbConnection* drv_createConnection() = 0;

    //! Connects to source database using (driver specific).
    //! Default implementation calls sourceConnection()->drv_connect() and if it succeeds,
    //! it calls sourceConnection()->drv_useDatabase(data()->sourceName), then returns
    //! the result. If this is enough for connecting for a migration driver, there is no need
    //! to reimplement drv_connect().
    //! If sourceConnection() is @c nullptr (custom types of sources),
    //! default implementation just returns @c false. In this case drv_connect() should
    //! be implemented.
    virtual bool drv_connect();

    //! Disconnect from source database (driver specific).
    //! If the source database is supported by low-level routines KDb,
    //! KDbConnection::disconnect() is called for this connection.
    //! For other types of sources @c false is returned so in these cases this method
    //! should be reimplemented.
    virtual bool drv_disconnect();

    //! Get table names in source database (driver specific)
    /*! @return List of table names available for this connection.
     The names are in lower case. The method should return true only if there was no
     error on getting database names list from the server. */
    virtual bool drv_tableNames(QStringList *tablenames) = 0;

    //! Read schema for a given table (driver specific)
    virtual bool drv_readTableSchema(
        const QString& originalName, KDbTableSchema *tableSchema) = 0;

    /*! Fetches maximum number from table \a tableName, column \a columnName
     into \a result. On success true is returned. If there is no records in the table,
     \a result is set to 0 and true is returned.
     - Note 1: implement only if the database can already contain kexidb__* tables
       (so e.g. keximdb driver doea not need this).
     - Note 2: default implementation uses drv_querySingleStringFromSQL()
       with "SELECT MAX(columName) FROM tableName" statement, assuming SQL-compliant
       backend.
    */
    virtual bool drv_queryMaxNumber(const QString& tableName,
                                    const QString& columnName, int *result);

    /*! Fetches single string at column \a columnNumber for each record from result obtained
     by running \a sqlStatement. \a numRecords can be specified to limit number of records read.
     If \a numRecords is -1, all records are loaded.
     On success the result is stored in \a stringList and true is returned.
     \return cancelled if there are no records available.
     - Note: implement only if the database can already contain kexidb__* tables
      (so e.g. keximdb driver does not need this). */
//! @todo SQL-dependent!
    virtual tristate drv_queryStringListFromSQL(
        const KDbEscapedString& sqlStatement, int columnNumber, QStringList *stringList,
        int numRecords = -1)
    {
        Q_UNUSED(sqlStatement); Q_UNUSED(columnNumber); Q_UNUSED(stringList);
        Q_UNUSED(numRecords);
        return cancelled;
    }

    /*! Fetches single string at column \a columnNumber from result obtained
     by running \a sqlStatement.
     On success the result is stored in \a string and true is returned.
     \return cancelled if there are no records available.
     This implementation uses drv_queryStringListFromSQL() with numRecords == 1. */
//! @todo SQL-dependent!
    virtual tristate drv_querySingleStringFromSQL(const KDbEscapedString& sqlStatement,
            int columnNumber, QString *string);

    //! A functor for filtering records
    //! @see drv_copyTable()
    class RecordFilter {
    public:
        RecordFilter() {}
        virtual ~RecordFilter() {}
        virtual bool operator() (const KDbSqlRecord &record) const = 0;
        virtual bool operator() (const QList<QVariant> &record) const = 0;
    };

    //! Copy a table from source DB to target DB (driver specific)
    //! - create copies of KDb tables
    //! - create copies of non-KDb tables
    virtual bool drv_copyTable(const QString& srcTable, KDbConnection *destConn,
                               KDbTableSchema* dstTable,
                               const RecordFilter *recordFilter = nullptr) = 0;

    virtual bool drv_progressSupported() {
        return false;
    }

    /*! \return the size of a table to be imported, or 0 if not supported
      Finds the size of the named table, in order to provide feedback on
      migration progress.

      The units of the return type are deliberately unspecified.  Migration
      drivers may return the number of records in the table, or the size in
      bytes, etc.  Units should be chosen in order that the driver can
      return the size in the fastest way possible (e.g. migration from CSV
      files should use file size to avoid counting the number of rows, and
      migration from MDB files should return the number of rows as this is
      stored within the file).

      Obviously, the driver should use the same units when reporting
      migration progress.

      \return size of the specified table
    */
    virtual bool drv_getTableSize(const QString&, quint64*) {
        return false;
    }

    void updateProgress(quint64 step = 1ULL);

//! @todo user should be asked ONCE using a convenient wizard's page, not a popup dialog
    //! Prompt user to select a field type for unrecognized fields
    KDbField::Type userType(const QString& fname);

    virtual QString drv_escapeIdentifier(const QString& str) const;

    //Extended API
    //! Position the source dataset at the start of a table
    virtual KDbSqlResult* drv_readFromTable(const QString & tableName) {
      Q_UNUSED(tableName);
      return nullptr;
    }

    //! Move to the next row
    virtual bool drv_moveNext() { return false; }

    //! Move to the previous row
    virtual bool drv_movePrevious() { return false; }

    //! Move to the next row
    virtual bool drv_moveFirst() { return false; }

    //! Move to the previous row
    virtual bool drv_moveLast() { return false; }

    //! Read the data at the given row/field
    virtual QVariant drv_value(int i) { Q_UNUSED(i); return QVariant(); };

    //! @return Database driver for this migration.
    KDbDriver *driver();

    //! Sets database driver for this migration.
    void setDriver(KDbDriver *driver);

private:
    /*! Estimate size of migration job
     Calls drv_getTableSize for each table to be copied.
     \return sum of the size of all tables to be copied.
    */
    bool progressInitialise();

    //! Perform an import operation. It is assumed that source connection is established.
    //! @see performImport()
    bool performImportInternal(Kexi::ObjectStatus* result);

    //! Reads schema for table @a tableName from kexi__objects and kexi__fields.
    //! On success:
    //! - copies one record from the original kexi__objects table to the destination
    //!   database's table kexi__objects
    //! - copies all related records from the original kexi__fields table to the destination
    //!   database's table kexi__fields
    bool importTable(const QString& tableName, KDbConnectionProxy *destConn);

    class Private;
    Private * const d;

    friend class MigrateManager;
    friend class MigrateManagerInternal;
};

} //namespace KexiMigration

#endif
