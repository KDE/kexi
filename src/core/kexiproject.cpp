/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2003-20198 Jarosław Staniek <staniek@kde.org>

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

#include "kexiproject.h"
#include "kexiprojectdata.h"
#include "kexipartmanager.h"
#include "kexipartitem.h"
#include "kexipartinfo.h"
#include "kexipart.h"
#include "KexiWindow.h"
#include "KexiWindowData.h"
#include "kexi.h"
#include "kexiblobbuffer.h"
#include "kexiguimsghandler.h"
#include <kexiutils/utils.h>
#include <KexiIcon.h>

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

#include <KLocalizedString>
#include <KStandardGuiItem>
#include <KMessageBox>

#include <KDbConnection>
#include <KDbConnectionOptions>
#include <KDbCursor>
#include <KDbDriverManager>
#include <KDbParser>
#include <KDbMessageHandler>
#include <KDbProperties>
#include <KDbTransactionGuard>

#include <assert.h>

//! @return a real plugin ID for @a pluginId and @a partMime
//! for compatibility with Kexi 1.x
static QString realPluginId(const QString &pluginId, const QString &partMime)
{
    if (pluginId.startsWith(QLatin1String("http://"))) {
        // for compatibility with Kexi 1.x
        // part mime was used at the time
        return QLatin1String("org.kexi-project.")
               + QString(partMime).remove("kexi/");
    }
    return pluginId;
}

class Q_DECL_HIDDEN KexiProject::Private
{
public:
    explicit Private(KexiProject *qq)
            : q(qq)
            , connection(0)
            , data(0)
            , tempPartItemID_Counter(-1)
            , sqlParser(0)
            , versionMajor(0)
            , versionMinor(0)
            , privateIDCounter(0)
            , itemsRetrieved(false)
    {
    }
    ~Private() {
        delete data;
        data = 0;
        delete sqlParser;
        foreach(KexiPart::ItemDict* dict, itemDicts) {
            qDeleteAll(*dict);
            dict->clear();
        }
        qDeleteAll(itemDicts);
        qDeleteAll(unstoredItems);
        unstoredItems.clear();
    }

    void savePluginId(const QString& pluginId, int typeId)
    {
        if (!typeIds.contains(pluginId) && !pluginIdsForTypeIds.contains(typeId)) {
            typeIds.insert(pluginId, typeId);
            pluginIdsForTypeIds.insert(typeId, pluginId);
        }
//! @todo what to do with extra plugin IDs for the same type ID or extra type ID name for the plugin ID?
    }

    //! @return user name for the current project
    //! @todo the name is taken from connection but it also can be specified otherwise
    //!       if the same connection data is shared by multiple users. This will be especially
    //!       true for 3-tier architectures.
    QString userName() const
    {
        QString name = connection->data().userName();
        return name.isNull() ? "" : name;
    }

    bool setNameOrCaption(KexiPart::Item* item,
                          const QString* _newName,
                          const QString* _newCaption)
    {
        q->clearResult();
        if (data->userMode()) {
            return false;
        }

        KexiUtils::WaitCursor wait;
        QString newName;
        if (_newName) {
            newName = _newName->trimmed();
            KDbMessageTitleSetter ts(q);
            if (newName.isEmpty()) {
                q->m_result = KDbResult(xi18n("Could not set empty name for this object."));
                return false;
            }
            if (q->itemForPluginId(item->pluginId(), newName) != 0) {
                q->m_result = KDbResult(
                    xi18nc("@info",
                           "Could not use this name. Object <resource>%1</resource> already exists.",
                           newName));
                return false;
            }
        }
        QString newCaption;
        if (_newCaption) {
            newCaption = _newCaption->trimmed();
        }

        KDbMessageTitleSetter et(q,
                                xi18nc("@info",
                                       "Could not rename object <resource>%1</resource>.", item->name()));
        if (!q->checkWritable())
            return false;
        KexiPart::Part *part = q->findPartFor(*item);
        if (!part)
            return false;
        KDbTransactionGuard tg(connection);
        if (!tg.transaction().isActive()) {
            q->m_result = connection->result();
            return false;
        }
        if (_newName) {
            if (!part->rename(item, newName)) {
                q->m_result = KDbResult(part->lastOperationStatus().description);
                q->m_result.setMessageTitle(part->lastOperationStatus().message);
                return false;
            }
            if (!connection->executeSql(KDbEscapedString("UPDATE kexi__objects SET o_name=%1 WHERE o_id=%2")
                    .arg(connection->escapeString(newName))
                    .arg(connection->driver()->valueToSql(KDbField::Integer, item->identifier()))))
            {
                q->m_result = connection->result();
                return false;
            }
        }
        if (_newCaption) {
            if (!connection->executeSql(KDbEscapedString("UPDATE kexi__objects SET o_caption=%1 WHERE o_id=%2")
                    .arg(connection->escapeString(newCaption))
                    .arg(connection->driver()->valueToSql(KDbField::Integer, item->identifier()))))
            {
                q->m_result = connection->result();
                return false;
            }
        }
        if (!tg.commit()) {
            q->m_result = connection->result();
            return false;
        }
        QString oldName(item->name());
        if (_newName) {
            item->setName(newName);
            emit q->itemRenamed(*item, oldName);
        }
        QString oldCaption(item->caption());
        if (_newCaption) {
            item->setCaption(newCaption);
            emit q->itemCaptionChanged(*item, oldCaption);
        }
        return true;
    }

    KexiProject *q;
    //! @todo KEXI3 use equivalent of QPointer<KDbConnection>
    KDbConnection* connection;
    //! @todo KEXI3 use equivalent of QPointer<KexiProjectData> or make KexiProjectData implicitly shared like KDbConnectionData
    KexiProjectData *data;
    QString error_title;
    KexiPart::MissingPartsList missingParts;

    QHash<QString, int> typeIds;
    QHash<int, QString> pluginIdsForTypeIds;
    //! a cache for item() method, indexed by plugin IDs
    QHash<QString, KexiPart::ItemDict*> itemDicts;
    QSet<KexiPart::Item*> unstoredItems;
    //! helper for getting unique
    //! temporary identifiers for unstored items
    int tempPartItemID_Counter;
    KDbParser* sqlParser;
    int versionMajor;
    int versionMinor;
    int privateIDCounter; //!< counter: ID for private "document" like Relations window
    bool itemsRetrieved;
};

//---------------------------

KexiProject::KexiProject(const KexiProjectData& pdata, KDbMessageHandler* handler)
        : QObject(), KDbObject(), KDbResultable()
        , d(new Private(this))
{
    d->data = new KexiProjectData(pdata);
    setMessageHandler(handler);
}

KexiProject::KexiProject(const KexiProjectData& pdata, KDbMessageHandler* handler,
                         KDbConnection* conn)
        : QObject(), KDbObject(), KDbResultable()
        , d(new Private(this))
{
    d->data = new KexiProjectData(pdata);
    setMessageHandler(handler);
    if (*d->data->connectionData() == d->connection->data())
        d->connection = conn;
    else
        qWarning() << "passed connection's data ("
            << conn->data().toUserVisibleString() << ") is not compatible with project's conn. data ("
            << d->data->connectionData()->toUserVisibleString() << ")";
}

KexiProject::~KexiProject()
{
    closeConnection();
    delete d;
}

KDbConnection *KexiProject::dbConnection() const
{
    return d->connection;
}

KexiProjectData* KexiProject::data() const
{
    return d->data;
}

int KexiProject::versionMajor() const
{
    return d->versionMajor;
}

int KexiProject::versionMinor() const
{
    return d->versionMinor;
}

tristate
KexiProject::open(bool *incompatibleWithKexi)
{
    Q_ASSERT(incompatibleWithKexi);
    KDbMessageGuard mg(this);
    return openInternal(incompatibleWithKexi);
}

tristate
KexiProject::open()
{
    KDbMessageGuard mg(this);
    return openInternal(0);
}

tristate
KexiProject::openInternal(bool *incompatibleWithKexi)
{
    if (!Kexi::partManager().infoList()) {
        m_result = Kexi::partManager().result();
        return cancelled;
    }
    if (incompatibleWithKexi)
        *incompatibleWithKexi = false;
    //qDebug() << d->data->databaseName() << d->data->connectionData()->driverId();
    KDbMessageTitleSetter et(this,
                             xi18nc("@info",
                                    "Could not open project <resource>%1</resource>.",
                                    d->data->databaseName()));

    if (!d->data->connectionData()->databaseName().isEmpty()) {
        QFileInfo finfo(d->data->connectionData()->databaseName());
        if (!finfo.exists()) {
            KMessageBox::error(nullptr, xi18nc("@info", "Could not open project. "
                                         "The project file <filename>%1</filename> does not exist.",
                                         QDir::toNativeSeparators(finfo.absoluteFilePath())),
                                         xi18nc("@title:window", "Could Not Open File"));
            return cancelled;
        }
        if (!d->data->isReadOnly() && !finfo.isWritable()) {
            if (KexiProject::askForOpeningNonWritableFileAsReadOnly(0, finfo)) {
                d->data->setReadOnly(true);
            }
            else {
                return cancelled;
            }
        }
    }

    if (!createConnection()) {
        qWarning() << "!createConnection()";
        return false;
    }
    bool cancel = false;
    if (!d->connection->useDatabase(d->data->databaseName(), true, &cancel)) {
        m_result = d->connection->result();
        if (cancel) {
            return cancelled;
        }
        qWarning() << "!d->connection->useDatabase() "
                   << d->data->databaseName() << d->data->connectionData()->driverId();

        if (d->connection->result().code() == ERR_NO_DB_PROPERTY) {
//<temp>
//! @todo this is temporary workaround as we have no import driver for SQLite
            if (/*supported?*/ !d->data->connectionData()->driverId().contains("sqlite")) {
//</temp>
                if (incompatibleWithKexi)
                    *incompatibleWithKexi = true;
            } else {
                KDbMessageTitleSetter et(this,
                    xi18nc("@info (don't add tags around %1, it's done already)",
                           "Database project %1 does not "
                           "appear to have been created using Kexi and cannot be opened. "
                           "It is an SQLite file created using other tools.",
                           KexiUtils::localizedStringToHtmlSubstring(d->data->infoString())));
                m_result = d->connection->result();
            }
            closeConnectionInternal();
            return false;
        }

        m_result = d->connection->result();
        closeConnectionInternal();
        return false;
    }

    if (!initProject())
        return false;

    return createInternalStructures(/*insideTransaction*/true);
}

tristate
KexiProject::create(bool forceOverwrite)
{
    KDbMessageGuard mg(this);
    KDbMessageTitleSetter et(this,
                             xi18nc("@info",
                                    "Could not create project <resource>%1</resource>.",
                                    d->data->databaseName()));

    if (!createConnection())
        return false;
    if (!checkWritable())
        return false;
    if (d->connection->databaseExists(d->data->databaseName())) {
        if (!forceOverwrite)
            return cancelled;
        if (!d->connection->dropDatabase(d->data->databaseName())) {
            m_result = d->connection->result();
            closeConnectionInternal();
            return false;
        }
        //qDebug() << "--- DB '" << d->data->databaseName() << "' dropped ---";
    }
    if (!d->connection->createDatabase(d->data->databaseName())) {
        m_result = d->connection->result();
        closeConnectionInternal();
        return false;
    }
    //qDebug() << "--- DB '" << d->data->databaseName() << "' created ---";
    // and now: open
    if (!d->connection->useDatabase(d->data->databaseName())) {
        qWarning() << "--- DB '" << d->data->databaseName() << "' USE ERROR ---";
        m_result = d->connection->result();
        closeConnectionInternal();
        return false;
    }
    //qDebug() << "--- DB '" << d->data->databaseName() << "' used ---";

    //<add some data>
    KDbTransaction trans = d->connection->beginTransaction();
    if (trans.isNull())
        return false;

    if (!createInternalStructures(/*!insideTransaction*/false))
        return false;

    //add some metadata
//! @todo put more props. todo - creator, created date, etc. (also to KexiProjectData)
    KDbProperties props = d->connection->databaseProperties();
    if (!props.setValue("kexiproject_major_ver", d->versionMajor)
            || !props.setCaption("kexiproject_major_ver", xi18n("Project major version"))
            || !props.setValue("kexiproject_minor_ver", d->versionMinor)
            || !props.setCaption("kexiproject_minor_ver", xi18n("Project minor version"))
            || !props.setValue("project_caption", d->data->caption())
            || !props.setCaption("project_caption", xi18n("Project caption"))
            || !props.setValue("project_desc", d->data->description())
            || !props.setCaption("project_desc", xi18n("Project description")))
    {
        m_result = props.result();
        return false;
    }

    if (trans.isActive() && !d->connection->commitTransaction(trans))
        return false;
    //</add some metadata>

    if (!Kexi::partManager().infoList()) {
        m_result = Kexi::partManager().result();
        return cancelled;
    }
    return initProject();
}

bool KexiProject::createInternalStructures(bool insideTransaction)
{
    KDbTransactionGuard tg;
    if (insideTransaction) {
        tg.setTransaction(d->connection->beginTransaction());
        if (tg.transaction().isNull())
            return false;
    }

    //Get information about kexiproject version.
    //kexiproject version is a version of data layer above kexidb layer.
    KDbProperties props = d->connection->databaseProperties();
    bool ok;
    int storedMajorVersion = props.value("kexiproject_major_ver").toInt(&ok);
    if (!ok)
        storedMajorVersion = 0;
    int storedMinorVersion = props.value("kexiproject_minor_ver").toInt(&ok);
    if (!ok)
        storedMinorVersion = 1;

    const tristate containsKexi__blobsTable = d->connection->containsTable("kexi__blobs");
    if (~containsKexi__blobsTable) {
        return false;
    }
    int dummy;
    bool contains_o_folder_id = false;
    if (true == containsKexi__blobsTable) {
        const tristate res = d->connection->querySingleNumber(
                KDbEscapedString("SELECT COUNT(o_folder_id) FROM kexi__blobs"), &dummy, 0,
                KDbConnection::QueryRecordOptions(KDbConnection::QueryRecordOption::Default)
                    & ~KDbConnection::QueryRecordOptions(KDbConnection::QueryRecordOption::AddLimitTo1));
        if (res == false) {
            m_result = d->connection->result();
        }
        else if (res == true) {
            contains_o_folder_id = true;
        }
    }
    bool add_folder_id_column = false;

//! @todo what about read-only db access?
    if (storedMajorVersion <= 0) {
        d->versionMajor = KEXIPROJECT_VERSION_MAJOR;
        d->versionMinor = KEXIPROJECT_VERSION_MINOR;
        //For compatibility for projects created before Kexi 1.0 beta 1:
        //1. no kexiproject_major_ver and kexiproject_minor_ver -> add them
        if (!d->connection->options()->isReadOnly()) {
            if (!props.setValue("kexiproject_major_ver", d->versionMajor)
                    || !props.setCaption("kexiproject_major_ver", xi18n("Project major version"))
                    || !props.setValue("kexiproject_minor_ver", d->versionMinor)
                    || !props.setCaption("kexiproject_minor_ver", xi18n("Project minor version"))) {
                return false;
            }
        }

        if (true == containsKexi__blobsTable) {
//! @todo what to do for readonly connections? Should we alter kexi__blobs in memory?
            if (!d->connection->options()->isReadOnly()) {
                if (!contains_o_folder_id) {
                    add_folder_id_column = true;
                }
            }
        }
    }
    if (storedMajorVersion != d->versionMajor || storedMajorVersion != d->versionMinor) {
        //! @todo version differs: should we change something?
        d->versionMajor = storedMajorVersion;
        d->versionMinor = storedMinorVersion;
    }

    QScopedPointer<KDbInternalTableSchema> t_blobs(new KDbInternalTableSchema("kexi__blobs"));
    t_blobs->addField(new KDbField("o_id", KDbField::Integer,
                                        KDbField::PrimaryKey | KDbField::AutoInc, KDbField::Unsigned));
    t_blobs->addField(new KDbField("o_data", KDbField::BLOB));
    t_blobs->addField(new KDbField("o_name", KDbField::Text));
    t_blobs->addField(new KDbField("o_caption", KDbField::Text));
    t_blobs->addField(new KDbField("o_mime", KDbField::Text, KDbField::NotNull));
    t_blobs->addField(new KDbField("o_folder_id",
                                KDbField::Integer, 0, KDbField::Unsigned) //references kexi__gallery_folders.f_id
              //If null, the BLOB only points to virtual "All" folder
              //WILL BE USED in Kexi >=2.0
             );

    //*** create global BLOB container, if not present
    if (true == containsKexi__blobsTable) {
        if (add_folder_id_column && !d->connection->options()->isReadOnly()) {
            // 2. "kexi__blobs" table contains no "o_folder_id" column -> add it
            //    (by copying table to avoid data loss)
            QScopedPointer<KDbInternalTableSchema> kexi__blobsCopy(
                new KDbInternalTableSchema(*t_blobs)); //won't be not needed - will be physically renamed to kexi_blobs

            kexi__blobsCopy->setName("kexi__blobs__copy");
            if (!d->connection->createTable(kexi__blobsCopy.data(),
                    KDbConnection::CreateTableOption::Default | KDbConnection::CreateTableOption::DropDestination))
            {

                m_result = d->connection->result();
                return false;
            }
            KDbInternalTableSchema *ts = kexi__blobsCopy.take(); // createTable() took ownerhip of kexi__blobsCopy
            // 2.1 copy data (insert 0's into o_folder_id column)
            if (!d->connection->executeSql(KDbEscapedString(
                    "INSERT INTO kexi__blobs (o_data, o_name, o_caption, o_mime, o_folder_id) "
                    "SELECT o_data, o_name, o_caption, o_mime, 0 FROM kexi__blobs"))
                // 2.2 remove the original kexi__blobs
                || !d->connection->executeSql(
                       KDbEscapedString("DROP TABLE kexi__blobs")) // lowlevel
                // 2.3 rename the copy back into kexi__blobs
                || !d->connection->alterTableName(
                       ts, "kexi__blobs",
                       KDbConnection::AlterTableNameOptions(KDbConnection::AlterTableNameOption::Default)
                           & ~KDbConnection::AlterTableNameOptions(KDbConnection::AlterTableNameOption::DropDestination)))
            {
                //(no need to drop the copy, ROLLBACK will drop it)
                m_result = d->connection->result();
                return false;
            }
        }
        //! just insert this schema, proper table exists
        d->connection->createTable(t_blobs.take());
    } else {
        if (!d->connection->options()->isReadOnly()) {
            if (!d->connection->createTable(t_blobs.data(),
                KDbConnection::CreateTableOption::Default | KDbConnection::CreateTableOption::DropDestination))
            {
                m_result = d->connection->result();
                return false;
            }
            (void)t_blobs.take(); // createTable() took ownerhip of t_blobs
        }
    }

    //Store default part information.
    //Information for other parts (forms, reports...) are created on demand in KexiWindow::storeNewData()
    const tristate containsKexi__partsTable = d->connection->containsTable("kexi__parts");
    if (~containsKexi__partsTable) {
        return false;
    }
    QScopedPointer<KDbInternalTableSchema> t_parts(new KDbInternalTableSchema("kexi__parts"));
    t_parts->addField(
        new KDbField("p_id", KDbField::Integer, KDbField::PrimaryKey | KDbField::AutoInc, KDbField::Unsigned)
    );
    t_parts->addField(new KDbField("p_name", KDbField::Text));
    t_parts->addField(new KDbField("p_mime", KDbField::Text));
    t_parts->addField(new KDbField("p_url", KDbField::Text));

    if (true == containsKexi__partsTable) {
        //! just insert this schema
        d->connection->createTable(t_parts.take());
    } else {
        if (!d->connection->options()->isReadOnly()) {
            bool partsTableOk = d->connection->createTable(t_parts.data(),
                KDbConnection::CreateTableOption::Default | KDbConnection::CreateTableOption::DropDestination);
            if (!partsTableOk) {
                m_result = d->connection->result();
                return false;
            }
            KDbInternalTableSchema *ts = t_parts.take(); // createTable() took ownerhip of t_parts
            QScopedPointer<KDbFieldList> fl(ts->subList("p_id", "p_name", "p_mime", "p_url"));
#define INSERT_RECORD(typeId, groupName, name) \
            if (partsTableOk) { \
                partsTableOk = d->connection->insertRecord(fl.data(), QVariant(int(KexiPart::typeId)), \
                    QVariant(groupName), \
                    QVariant("kexi/" name), QVariant("org.kexi-project." name)); \
                if (partsTableOk) { \
                    d->savePluginId("org.kexi-project." name, int(KexiPart::typeId)); \
                } \
            }

            INSERT_RECORD(TableObjectType, "Tables", "table")
            INSERT_RECORD(QueryObjectType, "Queries", "query")
            INSERT_RECORD(FormObjectType, "Forms", "form")
            INSERT_RECORD(ReportObjectType, "Reports", "report")
            INSERT_RECORD(ScriptObjectType, "Scripts", "script")
            INSERT_RECORD(WebObjectType, "Web pages", "web")
            INSERT_RECORD(MacroObjectType, "Macros", "macro")
#undef INSERT_RECORD
            if (!partsTableOk) {
                m_result = d->connection->result();
                // note: kexi__parts object still exists because createTable() succeeded
                return false;
            }
        }
    }

    // User data storage
    const tristate containsKexi__userdataTable = d->connection->containsTable("kexi__userdata");
    if (~containsKexi__userdataTable) {
        return false;
    }
    QScopedPointer<KDbInternalTableSchema> t_userdata(new KDbInternalTableSchema("kexi__userdata"));
    t_userdata->addField(new KDbField("d_user", KDbField::Text, KDbField::NotNull));
    t_userdata->addField(new KDbField("o_id", KDbField::Integer, KDbField::NotNull, KDbField::Unsigned));
    t_userdata->addField(new KDbField("d_sub_id", KDbField::Text, KDbField::NotNull | KDbField::NotEmpty));
    t_userdata->addField(new KDbField("d_data", KDbField::LongText));

    if (true == containsKexi__userdataTable) {
        d->connection->createTable(t_userdata.take());
    }
    else if (!d->connection->options()->isReadOnly()) {
        if (!d->connection->createTable(t_userdata.data(),
            KDbConnection::CreateTableOption::Default | KDbConnection::CreateTableOption::DropDestination))
        {
            m_result = d->connection->result();
            return false;
        }
        (void)t_userdata.take(); // createTable() took ownerhip of t_userdata
    }

    if (insideTransaction) {
        if (tg.transaction().isActive() && !tg.commit()) {
            m_result = d->connection->result();
            return false;
        }
    }
    return true;
}

bool
KexiProject::createConnection()
{
    clearResult();
    KDbMessageGuard mg(this);
    if (d->connection) {
        return true;
    }

    KDbMessageTitleSetter et(this);
    KDbDriver *driver = Kexi::driverManager().driver(d->data->connectionData()->driverId());
    if (!driver) {
        m_result = Kexi::driverManager().result();
        return false;
    }

    KDbConnectionOptions connectionOptions;
    if (d->data->isReadOnly()) {
        connectionOptions.setReadOnly(true);
    }
    d->connection = driver->createConnection(*d->data->connectionData(), connectionOptions);
    if (!d->connection) {
        m_result = driver->result();
        qWarning() << "error create connection:" << m_result;
        return false;
    }

    if (!d->connection->connect()) {
        m_result = d->connection->result();
        qWarning() << "error connecting:" << m_result;
        delete d->connection; //this will also clear connection for BLOB buffer
        d->connection = 0;
        return false;
    }

    //re-init BLOB buffer
//! @todo won't work for subsequent connection
    KexiBLOBBuffer::setConnection(d->connection);
    return true;
}

bool KexiProject::closeConnectionInternal()
{
    if (!m_result.isError()) {
        clearResult();
    }
    if (!d->connection) {
        return true;
    }
    if (!d->connection->disconnect()) {
        if (!m_result.isError()) {
            m_result = d->connection->result();
        }
        return false;
    }

    delete d->connection; //this will also clear connection for BLOB buffer
    d->connection = 0;
    return true;
}

bool KexiProject::closeConnection()
{
    clearResult();
    KDbMessageGuard mg(this);
    if (!d->connection)
        return true;

    if (!d->connection->disconnect()) {
        m_result = d->connection->result();
        return false;
    }

    delete d->connection; //this will also clear connection for BLOB buffer
    d->connection = 0;
    return true;
}

bool
KexiProject::initProject()
{
    //qDebug() << "checking project parts...";
    if (!checkProject()) {
        return false;
    }

// !@todo put more props. todo - creator, created date, etc. (also to KexiProjectData)
    KDbProperties props = d->connection->databaseProperties();
    QString str(props.value("project_caption").toString());
    if (!str.isEmpty())
        d->data->setCaption(str);
    str = props.value("project_desc").toString();
    if (!str.isEmpty())
        d->data->setDescription(str);

    return true;
}

bool
KexiProject::isConnected()
{
    if (d->connection && d->connection->isDatabaseUsed())
        return true;

    return false;
}

KexiPart::ItemDict*
KexiProject::items(KexiPart::Info *i)
{
    clearResult();
    KDbMessageGuard mg(this);
    if (!i || !isConnected())
        return 0;

    //trying in cache...
    KexiPart::ItemDict *dict = d->itemDicts.value(i->id());
    if (dict)
        return dict;
    if (d->itemsRetrieved)
        return 0;
    if (!retrieveItems())
        return 0;
    return items(i); // try again
}

bool KexiProject::retrieveItems()
{
    d->itemsRetrieved = true;
    KDbCursor *cursor = d->connection->executeQuery(
        KDbEscapedString("SELECT o_id, o_name, o_caption, o_type FROM kexi__objects ORDER BY o_type"));
    if (!cursor) {
        m_result = d->connection->result();
        return 0;
    }

    int recentTypeId = -1000;
    QString pluginId;
    KexiPart::ItemDict *dict = 0;
    QSet<QString> tableNamesSet;
    for (cursor->moveFirst(); !cursor->eof(); cursor->moveNext()) {
        bool ok;
        const int typeId = cursor->value(3).toInt(&ok);
        if (!ok || typeId <= 0) {
            qInfo() << "object of unknown type id" << cursor->value(3) << "id=" << cursor->value(0)
                    << "name=" <<  cursor->value(1);
            continue;
        }
        if (recentTypeId == typeId) {
            if (pluginId.isEmpty()) { // still the same unknown plugin ID
                continue;
            }
        }
        else {
            // a new type ID: create another plugin items dict if it's an ID for a known type
            recentTypeId = typeId;
            pluginId = pluginIdForTypeId(typeId);
            if (pluginId.isEmpty())
                continue;
            dict = new KexiPart::ItemDict();
            d->itemDicts.insert(pluginId, dict);
            if (typeId == KDb::TableObjectType) {
                // Starting to load table names: initialize.
                // This list since 3.2 does not contain names without physical tables so we can
                // catch these cases below.
                const QStringList tableNames(d->connection->tableNames(false /*public*/, &ok));
                if (!ok) {
                    m_result = KDbResult(ERR_OBJECT_NOT_FOUND, xi18n("Could not load list of tables."));
                    qDeleteAll(d->itemDicts);
                    return false;
                }
                for (const QString &name : tableNames) {
                    tableNamesSet.insert(name.toLower());
                }
            }
        }
        const int ident = cursor->value(0).toInt(&ok);
        const QString objName(cursor->value(1).toString());
        if (!ok || ident <= 0 || !KDb::isIdentifier(objName))
        {
            continue; // invalid ID or invalid name
        }
        if (typeId == KDb::TableObjectType) {
            if (d->connection->isInternalTableSchema(objName)) {
                qInfo() << "table" << objName << "id=" << ident << "is internal, skipping";
                continue;
            }
            if (!tableNamesSet.contains(objName.toLower())) {
                qInfo() << "table" << objName << "id=" << ident
                        << "does not correspondent with physical table";
                continue;
            }
        }
        KexiPart::Item *it = new KexiPart::Item();
        it->setIdentifier(ident);
        it->setPluginId(pluginId);
        it->setName(objName);
        it->setCaption(cursor->value(2).toString());
        dict->insert(it->identifier(), it);
    }

    d->connection->deleteCursor(cursor);
    return true;
}

int KexiProject::typeIdForPluginId(const QString &pluginId) const
{
    return d->typeIds.value(pluginId, -1);
}

QString KexiProject::pluginIdForTypeId(int typeId) const
{
    return d->pluginIdsForTypeIds.value(typeId);
}

//static
KDbTableOrQuerySchema::Type KexiProject::pluginIdToTableOrQueryType(
        const QString &pluginId, bool *ok)
{
    Q_ASSERT(ok);
    KDbTableOrQuerySchema::Type result;
    if (pluginId == QStringLiteral("org.kexi-project.table")) {
        result = KDbTableOrQuerySchema::Type::Table;
        *ok = true;
    } else if (pluginId == QStringLiteral("org.kexi-project.query")) {
        result = KDbTableOrQuerySchema::Type::Query;
        *ok = true;
    } else {
        result = KDbTableOrQuerySchema::Type::Table;
        *ok = false;
    }
    return result;
}

KexiPart::ItemDict*
KexiProject::itemsForPluginId(const QString &pluginId)
{
    KDbMessageGuard mg(this);
    KexiPart::Info *info = Kexi::partManager().infoForPluginId(pluginId);
    if (!info) {
        m_result = Kexi::partManager().result();
        return 0;
    }
    return items(info);
}

void
KexiProject::getSortedItems(KexiPart::ItemList* list, KexiPart::Info *i)
{
    Q_ASSERT(list);
    list->clear();
    KexiPart::ItemDict* dict = items(i);
    if (!dict)
        return;
    foreach(KexiPart::Item *item, *dict) {
        list->append(item);
    }
}

void
KexiProject::getSortedItemsForPluginId(KexiPart::ItemList *list, const QString &pluginId)
{
    Q_ASSERT(list);
    KexiPart::Info *info = Kexi::partManager().infoForPluginId(pluginId);
    if (!info) {
        m_result = Kexi::partManager().result();
        return;
    }
    getSortedItems(list, info);
}

void
KexiProject::addStoredItem(KexiPart::Info *info, KexiPart::Item *item)
{
    if (!info || !item)
        return;
    KexiPart::ItemDict *dict = items(info);
    item->setNeverSaved(false);
    d->unstoredItems.remove(item); //no longer unstored

    // are we replacing previous item?
    KexiPart::Item *prevItem = dict->take(item->identifier());
    if (prevItem) {
        emit itemRemoved(*prevItem);
    }

    dict->insert(item->identifier(), item);
    //let's update e.g. navigator
    emit newItemStored(item);
}

KexiPart::Item*
KexiProject::itemForPluginId(const QString &pluginId, const QString &name)
{
    KexiPart::ItemDict *dict = itemsForPluginId(pluginId);
    if (!dict) {
        qWarning() << "no part class=" << pluginId;
        return 0;
    }
    foreach(KexiPart::Item *item, *dict) {
        if (item->name() == name)
            return item;
    }
    return 0;
}

KexiPart::Item*
KexiProject::item(KexiPart::Info *i, const QString &name)
{
    KexiPart::ItemDict *dict = items(i);
    if (!dict)
        return 0;
    foreach(KexiPart::Item* item, *dict) {
        if (item->name() == name)
            return item;
    }
    return 0;
}

KexiPart::Item*
KexiProject::item(int identifier)
{
    foreach(KexiPart::ItemDict *dict, d->itemDicts) {
        KexiPart::Item *item = dict->value(identifier);
        if (item)
            return item;
    }
    return 0;
}

KexiPart::Part *KexiProject::findPartFor(const KexiPart::Item& item)
{
    clearResult();
    KDbMessageGuard mg(this);
    KDbMessageTitleSetter et(this);
    KexiPart::Part *part = Kexi::partManager().partForPluginId(item.pluginId());
    if (!part) {
        qWarning() << "!part:" << item.pluginId();
        m_result = Kexi::partManager().result();
    }
    return part;
}

KexiWindow* KexiProject::openObject(QWidget* parent, KexiPart::Item *item,
                                    Kexi::ViewMode viewMode, QMap<QString, QVariant>* staticObjectArgs)
{
    clearResult();
    KDbMessageGuard mg(this);
    if (viewMode != Kexi::DataViewMode && data()->userMode())
        return 0;

    KDbMessageTitleSetter et(this);
    KexiPart::Part *part = findPartFor(*item);
    if (!part)
        return 0;
    KexiWindow *window  = part->openInstance(parent, item, viewMode, staticObjectArgs);
    if (!window) {
        if (part->lastOperationStatus().error()) {
            m_result = KDbResult(xi18nc("@info",
                                        "Opening object <resource>%1</resource> failed.\n%2 %3", item->name())
                                 .arg(part->lastOperationStatus().message)
                                 .arg(part->lastOperationStatus().description)
                                 .replace("(I18N_ARGUMENT_MISSING)", " ")); // a hack until there's other solution
        }
        return 0;
    }
    return window;
}

KexiWindow* KexiProject::openObject(QWidget* parent, const QString &pluginId,
                                    const QString& name, Kexi::ViewMode viewMode)
{
    KexiPart::Item *it = itemForPluginId(pluginId, name);
    return it ? openObject(parent, it, viewMode) : 0;
}

bool KexiProject::checkWritable()
{
    if (!d->connection->options()->isReadOnly())
        return true;
    m_result = KDbResult(xi18n("This project is opened as read only."));
    return false;
}

bool KexiProject::removeObject(KexiPart::Item *item)
{
    Q_ASSERT(item);
    clearResult();
    if (data()->userMode())
        return false;

    KDbMessageTitleSetter et(this);
    if (!checkWritable())
        return false;
    KexiPart::Part *part = findPartFor(*item);
    if (!part)
        return false;
    if (!item->neverSaved() && !part->remove(item)) {
        //! @todo check for errors
        return false;
    }
    if (!item->neverSaved()) {
        KDbTransactionGuard tg(d->connection);
        if (!tg.transaction().isActive()) {
            m_result = d->connection->result();
            return false;
        }
        if (!d->connection->removeObject(item->identifier())) {
            m_result = d->connection->result();
            return false;
        }
        if (!removeUserDataBlock(item->identifier())) {
            m_result = KDbResult(ERR_DELETE_SERVER_ERROR, xi18n("Could not delete object's user data."));
            return false;
        }
        if (!tg.commit()) {
            m_result = d->connection->result();
            return false;
        }
    }
    emit itemRemoved(*item);

    //now: remove this item from cache
    if (part->info()) {
        KexiPart::ItemDict *dict = d->itemDicts.value(part->info()->pluginId());
        if (!(dict && dict->remove(item->identifier())))
            d->unstoredItems.remove(item);//remove temp.
    }
    return true;
}


bool KexiProject::renameObject(KexiPart::Item *item, const QString& newName)
{
    KDbMessageGuard mg(this);
    return d->setNameOrCaption(item, &newName, 0);
}

bool KexiProject::setObjectCaption(KexiPart::Item *item, const QString& newCaption)
{
    KDbMessageGuard mg(this);
    return d->setNameOrCaption(item, 0, &newCaption);
}

KexiPart::Item* KexiProject::createPartItem(KexiPart::Info *info, const QString& suggestedCaption)
{
    clearResult();
    KDbMessageGuard mg(this);
    if (data()->userMode())
        return 0;

    KDbMessageTitleSetter et(this);
    KexiPart::Part *part = Kexi::partManager().part(info);
    if (!part) {
        m_result = Kexi::partManager().result();
        return 0;
    }

    KexiPart::ItemDict *dict = items(info);
    if (!dict) {
        dict = new KexiPart::ItemDict();
        d->itemDicts.insert(info->pluginId(), dict);
    }
    QSet<QString> storedItemNames;
    foreach(KexiPart::Item* item, *dict) {
        storedItemNames.insert(item->name());
    }

    QSet<QString> unstoredItemNames;
    foreach(KexiPart::Item* item, d->unstoredItems) {
        unstoredItemNames.insert(item->name());
    }

    //find new, unique default name for this item
    int n;
    QString new_name;
    QString base_name;
    if (suggestedCaption.isEmpty()) {
        n = 1;
        base_name = part->instanceName();
    } else {
        n = 0; //means: try not to add 'n'
        base_name = KDb::stringToIdentifier(suggestedCaption).toLower();
    }
    base_name = KDb::stringToIdentifier(base_name).toLower();
    do {
        new_name = base_name;
        if (n >= 1)
            new_name += QString::number(n);
        if (storedItemNames.contains(new_name)) {
            n++;
            continue; //stored exists!
        }
        if (!unstoredItemNames.contains(new_name))
            break; //unstored doesn't exist
        n++;
    } while (n < 1000/*sanity*/);

    if (n >= 1000)
        return 0;

    QString new_caption(suggestedCaption.isEmpty()
        ? part->info()->name() : suggestedCaption);
    if (n >= 1)
        new_caption += QString::number(n);

    KexiPart::Item *item = new KexiPart::Item();
    item->setIdentifier(--d->tempPartItemID_Counter);  //temporary
    item->setPluginId(info->pluginId());
    item->setName(new_name);
    item->setCaption(new_caption);
    item->setNeverSaved(true);
    d->unstoredItems.insert(item);
    return item;
}

KexiPart::Item* KexiProject::createPartItem(KexiPart::Part *part, const QString& suggestedCaption)
{
    Q_ASSERT(part);
    return createPartItem(part->info(), suggestedCaption);
}

void KexiProject::deleteUnstoredItem(KexiPart::Item *item)
{
    if (!item)
        return;
    d->unstoredItems.remove(item);
    delete item;
}

KDbParser* KexiProject::sqlParser()
{
    if (!d->sqlParser) {
        if (!d->connection)
            return 0;
        d->sqlParser = new KDbParser(d->connection);
    }
    return d->sqlParser;
}

const char warningNoUndo[] = I18N_NOOP2("warning", "Entire project's data and design will be deleted.");

/*static*/
KexiProject*
KexiProject::createBlankProject(bool *cancelled, const KexiProjectData& data,
                                KDbMessageHandler* handler)
{
    Q_ASSERT(cancelled);
    *cancelled = false;
    KexiProject *prj = new KexiProject(data, handler);

    tristate res = prj->create(false);
    if (~res) {
//! @todo move to KexiMessageHandler
        if (KMessageBox::PrimaryAction != KMessageBox::warningTwoActions(nullptr,
            xi18nc("@info (don't add tags around %1, it's done already)",
                   "<para>The project %1 already exists.</para>"
                   "<para>Do you want to replace it with a new, blank one?</para>"
                   "<para><warning>%2</warning></para>",
                   KexiUtils::localizedStringToHtmlSubstring(prj->data()->infoString()),
                   xi18n(warningNoUndo)),
                QString(), KGuiItem(xi18nc("@action:button", "Replace")), KStandardGuiItem::cancel()))
//! @todo add toUserVisibleString() for server-based prj
        {
            delete prj;
            *cancelled = true;
            return 0;
        }
        res = prj->create(true/*overwrite*/);
    }
    if (res != true) {
        delete prj;
        return 0;
    }
    //qDebug() << "new project created --- ";
//! @todo Kexi::recentProjects().addProjectData( data );

    return prj;
}

/*static*/
tristate KexiProject::dropProject(const KexiProjectData& data,
                                  KDbMessageHandler* handler, bool dontAsk)
{
    if (!dontAsk && KMessageBox::PrimaryAction != KMessageBox::questionTwoActions(nullptr,
            xi18nc("@info",
                   "<para>Do you want to delete the project <resource>%1</resource>?</para>"
                   "<para><warning>%2</warning></para>",
                   static_cast<const KDbObject*>(&data)->name(),
                   i18n(warningNoUndo)),
                 QString(), KGuiItem(xi18nc("@action:button", "Delete Project"), koIconName("edit-delete")),
                 KStandardGuiItem::cancel(), QString(),
                 KMessageBox::Notify | KMessageBox::Dangerous))
    {
        return cancelled;
    }

    KexiProject prj(data, handler);
    if (!prj.open())
        return false;

    if (prj.dbConnection()->options()->isReadOnly()) {
        handler->showErrorMessage(
            KDbMessageHandler::Error,
            xi18n("Could not delete this project. Database connection for this project has been opened as read only."));
        return false;
    }

    KDbMessageGuard mg(prj.dbConnection()->result(), handler);
    return prj.dbConnection()->dropDatabase();
}

bool KexiProject::checkProject(const QString& singlePluginId)
{
    clearResult();

//! @todo catch errors!
    if (!d->connection->isDatabaseUsed()) {
        m_result = d->connection->result();
        return false;
    }
    const tristate containsKexi__partsTable = d->connection->containsTable("kexi__parts");
    if (~containsKexi__partsTable) {
        return false;
    }
    if (true == containsKexi__partsTable) { // check if kexi__parts exists, if missing, createInternalStructures() will create it
        KDbEscapedString sql = KDbEscapedString("SELECT p_id, p_name, p_mime, p_url FROM kexi__parts ORDER BY p_id");
        if (!singlePluginId.isEmpty()) {
            sql.append(KDbEscapedString(" WHERE p_url=%1").arg(d->connection->escapeString(singlePluginId)));
        }
        KDbCursor *cursor = d->connection->executeQuery(sql);
        if (!cursor) {
            m_result = d->connection->result();
            return false;
        }

        bool saved = false;
        for (cursor->moveFirst(); !cursor->eof(); cursor->moveNext()) {
            const QString partMime(cursor->value(2).toString());
            QString pluginId(cursor->value(3).toString());
            pluginId = realPluginId(pluginId, partMime);
            if (pluginId == QLatin1String("uk.co.piggz.report")) { // compatibility
                pluginId = QLatin1String("org.kexi-project.report");
            }
            KexiPart::Info *info = Kexi::partManager().infoForPluginId(pluginId);
            bool ok;
            const int typeId = cursor->value(0).toInt(&ok);
            if (!ok || typeId <= 0) {
                qWarning() << "Invalid type ID" << typeId << "; part with ID" << pluginId << "will not be used";
            }
            if (info && ok && typeId > 0) {
                d->savePluginId(pluginId, typeId);
                saved = true;
            }
            else {
                KexiPart::MissingPart m;
                m.name = cursor->value(1).toString();
                m.id = pluginId;
                d->missingParts.append(m);
            }
        }

        d->connection->deleteCursor(cursor);
        if (!saved && !singlePluginId.isEmpty()) {
            return false; // failure is single part class was not found
        }
    }
    return true;
}

int KexiProject::generatePrivateID()
{
    return --d->privateIDCounter;
}

bool KexiProject::createIdForPart(const KexiPart::Info& info)
{
    KDbMessageGuard mg(this);
    int typeId = typeIdForPluginId(info.pluginId());
    if (typeId > 0) {
        return true;
    }
    // try again, perhaps the id is already created
    if (checkProject(info.pluginId())) {
        return true;
    }

    // Find first available custom part ID by taking the greatest
    // existing custom ID (if it exists) and adding 1.
    typeId = int(KexiPart::UserObjectType);
    tristate success = d->connection->querySingleNumber(KDbEscapedString("SELECT max(p_id) FROM kexi__parts"), &typeId);
    if (!success) {
        // Couldn't read part id's from the kexi__parts table
        m_result = d->connection->result();
        return false;
    } else {
        // Got a maximum part ID, or there were no parts
        typeId = typeId + 1;
        typeId = qMax(typeId, (int)KexiPart::UserObjectType);
    }

    //this part's ID is not stored within kexi__parts:
    KDbTableSchema *ts =
        d->connection->tableSchema("kexi__parts");
    if (!ts) {
        m_result = d->connection->result();
        return false;
    }
    QScopedPointer<KDbFieldList> fl(ts->subList("p_id", "p_name", "p_mime", "p_url"));
    //qDebug() << "fieldlist:" << (fl ? *fl : QString());
    if (!fl)
        return false;

    //qDebug() << info.ptr()->untranslatedGenericName();
//  QStringList sl = part()->info()->ptr()->propertyNames();
//  for (QStringList::ConstIterator it=sl.constBegin();it!=sl.constEnd();++it)
   //qDebug() << *it << part()->info()->ptr()->property(*it).toString();
    if (!d->connection->insertRecord(
                fl.data(),
                QVariant(typeId),
                QVariant(info.untranslatedGroupName()),
                QVariant(QString::fromLatin1("kexi/") + info.typeName()/*ok?*/),
                QVariant(info.id() /*always ok?*/)))
    {
        m_result = d->connection->result();
        return false;
    }

    //qDebug() << "insert success!";
    d->savePluginId(info.id(), typeId);
    //qDebug() << "new id is:" << p_id;
    return true;
}

KexiPart::MissingPartsList KexiProject::missingParts() const
{
    return d->missingParts;
}

static bool checkObjectId(const char* method, int objectID)
{
    if (objectID <= 0) {
        qWarning() << method <<  ": Invalid objectID" << objectID;
        return false;
    }
    return true;
}

tristate KexiProject::loadUserDataBlock(int objectID, const QString& dataID, QString *dataString)
{
    KDbMessageGuard mg(this);
    if (!checkObjectId("loadUserDataBlock", objectID)) {
        return false;
    }
    if (!d->connection->querySingleString(
               KDbEscapedString("SELECT d_data FROM kexi__userdata WHERE o_id=%1 AND ")
                .arg(d->connection->driver()->valueToSql(KDbField::Integer, objectID))
                + KDb::sqlWhere(d->connection->driver(), KDbField::Text, "d_user", d->userName())
                + " AND " + KDb::sqlWhere(d->connection->driver(), KDbField::Text, "d_sub_id", dataID),
               dataString))
    {
        m_result = d->connection->result();
        return false;
    }
    return true;
}

bool KexiProject::storeUserDataBlock(int objectID, const QString& dataID, const QString &dataString)
{
    KDbMessageGuard mg(this);
    if (!checkObjectId("storeUserDataBlock", objectID)) {
        return false;
    }
    KDbEscapedString sql
            = KDbEscapedString("SELECT kexi__userdata.o_id FROM kexi__userdata WHERE o_id=%1").arg(objectID);
    KDbEscapedString sql_sub
            = KDb::sqlWhere(d->connection->driver(), KDbField::Text, "d_user", d->userName())
              + " AND " + KDb::sqlWhere(d->connection->driver(), KDbField::Text, "d_sub_id", dataID);

    const tristate result = d->connection->resultExists(sql + " AND " + sql_sub);
    if (~result) {
        m_result = d->connection->result();
        return false;
    }
    if (result == true) {
        if (!d->connection->executeSql(
            KDbEscapedString("UPDATE kexi__userdata SET d_data="
                + d->connection->driver()->valueToSql(KDbField::LongText, dataString)
                + " WHERE o_id=" + QString::number(objectID) + " AND " + sql_sub)))
        {
            m_result = d->connection->result();
            return false;
        }
        return true;
    }
    if (!d->connection->executeSql(
               KDbEscapedString("INSERT INTO kexi__userdata (d_user, o_id, d_sub_id, d_data) VALUES (")
               + d->connection->driver()->valueToSql(KDbField::Text, d->userName())
               + ", " + QString::number(objectID)
               + ", " + d->connection->driver()->valueToSql(KDbField::Text, dataID)
               + ", " + d->connection->driver()->valueToSql(KDbField::LongText, dataString)
               + ")"))
    {
        m_result = d->connection->result();
        return false;
    }
    return true;
}

bool KexiProject::copyUserDataBlock(int sourceObjectID, int destObjectID, const QString &dataID)
{
    KDbMessageGuard mg(this);
    if (!checkObjectId("storeUserDataBlock(sourceObjectID)", sourceObjectID)) {
        return false;
    }
    if (!checkObjectId("storeUserDataBlock(destObjectID)", destObjectID)) {
        return false;
    }
    if (sourceObjectID == destObjectID)
        return true;
    if (!removeUserDataBlock(destObjectID, dataID)) // remove before copying
        return false;
    KDbEscapedString sql
        = KDbEscapedString("INSERT INTO kexi__userdata SELECT t.d_user, %2, t.d_sub_id, t.d_data "
                           "FROM kexi__userdata AS t WHERE d_user=%1 AND o_id=%3")
                         .arg(d->connection->escapeString(d->userName()))
                         .arg(d->connection->driver()->valueToSql(KDbField::Integer, destObjectID))
                         .arg(d->connection->driver()->valueToSql(KDbField::Integer, sourceObjectID));
    if (!dataID.isEmpty()) {
        sql += " AND " + KDb::sqlWhere(d->connection->driver(), KDbField::Text, "d_sub_id", dataID);
    }
    if (!d->connection->executeSql(sql)) {
        m_result = d->connection->result();
        return false;
    }
    return true;
}

bool KexiProject::removeUserDataBlock(int objectID, const QString& dataID)
{
    KDbMessageGuard mg(this);
    if (!checkObjectId("removeUserDataBlock", objectID)) {
        return false;
    }
    if (dataID.isEmpty()) {
        if (!KDb::deleteRecords(d->connection, "kexi__userdata",
                               "o_id", KDbField::Integer, objectID,
                               "d_user", KDbField::Text, d->userName()))
        {
            m_result = d->connection->result();
            return false;
        } else if (!KDb::deleteRecords(d->connection, "kexi__userdata",
                               "o_id", KDbField::Integer, objectID,
                               "d_user", KDbField::Text, d->userName(),
                               "d_sub_id", KDbField::Text, dataID))
        {
            m_result = d->connection->result();
            return false;
        }
    }
    return true;
}

// static
bool KexiProject::askForOpeningNonWritableFileAsReadOnly(QWidget *parent, const QFileInfo &finfo)
{
    KGuiItem openItem(KStandardGuiItem::open());
    openItem.setText(xi18n("Open As Read Only"));
    return KMessageBox::PrimaryAction == KMessageBox::questionTwoActions(
            parent, xi18nc("@info",
                          "<para>Could not open file <filename>%1</filename> for reading and writing.</para>"
                          "<para>Do you want to open the file as read only?</para>",
                          QDir::toNativeSeparators(finfo.filePath())),
                    xi18nc("@title:window", "Could Not Open File" ),
                    openItem, KStandardGuiItem::cancel(), QString());
}
