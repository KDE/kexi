/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2003-2016 Jarosław Staniek <staniek@kde.org>

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

#include "kexiprojectdata.h"
#include <kexiutils/utils.h>
#include <kexi_global.h>

#include <KDbUtils>
#include <KDbDriverManager>
#include <KDbConnectionData>
#include <KDbDriverMetaData>

#include <KConfig>
#include <KConfigGroup>

#include <QDir>
#include <QStringList>
#include <QDebug>

//! Version of the KexiProjectData format.
#define KEXIPROJECTDATA_FORMAT 3
/* CHANGELOG:
 v1: initial version
 v2: "encryptedPassword" field added.
     For backward compatibility, it is not used if the connection data has been loaded from
     a file saved with version 1. In such cases unencrypted "password" field is used.
 v3: "name" for shortcuts to file-based databases is a full file path.
     If the file is within the user's home directory, the dir is replaced with $HOME,
     e.g. name=$HOME/mydb.kexi. Not compatible with earlier versions but in these
     versions only filename was stored so the file was generally inaccessible anyway.
     "lastOpened" field added of type date/time (ISO format).
*/

//! @internal
class KexiProjectDataPrivate
{
public:
    KexiProjectDataPrivate()
            : userMode(false)
            , readOnly(false) {}

    KDbConnectionData connData;
    QDateTime lastOpened;
    bool userMode;
    bool readOnly;
};

//---------------------------------------

KexiProjectData::KexiProjectData()
        : QObject(0)
        , KDbObject()
        , formatVersion(0)
        , d(new KexiProjectDataPrivate())
{
    setObjectName("KexiProjectData");
}

KexiProjectData::KexiProjectData(
    const KDbConnectionData &cdata, const QString& dbname, const QString& caption)
        : QObject(0)
        , KDbObject()
        , formatVersion(0)
        , d(new KexiProjectDataPrivate())
{
    setObjectName("KexiProjectData");
    d->connData = cdata;
    setDatabaseName(cdata.databaseName().isEmpty() ? dbname : cdata.databaseName());
    setCaption(caption);
}

KexiProjectData::KexiProjectData(const KexiProjectData& pdata)
        : QObject(0)
        , KDbObject()
        , KDbResultable(pdata)
        , d(new KexiProjectDataPrivate())
{
    setObjectName("KexiProjectData");
    *this = pdata;
    autoopenObjects = pdata.autoopenObjects;
}

KexiProjectData::~KexiProjectData()
{
    delete d;
}

KexiProjectData& KexiProjectData::operator=(const KexiProjectData & pdata)
{
    static_cast<KDbObject&>(*this) = static_cast<const KDbObject&>(pdata);
    //deep copy
    autoopenObjects = pdata.autoopenObjects;
    formatVersion = pdata.formatVersion;
    *d = *pdata.d;
    return *this;
}

KDbConnectionData* KexiProjectData::connectionData()
{
    return &d->connData;
}

const KDbConnectionData* KexiProjectData::connectionData() const
{
    return &d->connData;
}

QString KexiProjectData::databaseName() const
{
    return KDbObject::name();
}

void KexiProjectData::setDatabaseName(const QString& dbName)
{
    //qDebug() << dbName;
    //qDebug() << *this;
    KDbObject::setName(dbName);
}

bool KexiProjectData::userMode() const
{
    return d->userMode;
}

QDateTime KexiProjectData::lastOpened() const
{
    return d->lastOpened;
}

void KexiProjectData::setLastOpened(const QDateTime& lastOpened)
{
    d->lastOpened = lastOpened;

}
QString KexiProjectData::description() const
{
    return KDbObject::description();
}

void KexiProjectData::setDescription(const QString& desc)
{
    return KDbObject::setDescription(desc);
}

// static
KLocalizedString KexiProjectData::infoString(const QString &databaseName,
                                    const KDbConnectionData &data)
{
    if (data.databaseName().isEmpty()) {
        //server-based
        return kxi18nc("@info database connection",
                       "<resource>%1</resource> (connection <resource>%2</resource>)")
                .subs(databaseName).subs(data.toUserVisibleString());
    }
    //file-based
    return kxi18nc("@info database name",
                   "<resource>%1</resource>").subs(data.databaseName());
}

KLocalizedString KexiProjectData::infoString() const
{
    return infoString(databaseName(), d->connData);
}

void KexiProjectData::setReadOnly(bool set)
{
    d->readOnly = set;
}

bool KexiProjectData::isReadOnly() const
{
    return d->readOnly;
}

bool KexiProjectData::load(const QString& fileName, QString* _groupKey)
{
    //! @todo how about readOnly arg?
    KConfig config(fileName, KConfig::SimpleConfig);
    KConfigGroup cg = config.group("File Information");
    int _formatVersion = cg.readEntry("version", KEXIPROJECTDATA_FORMAT);

    QString groupKey;
    if (!_groupKey || _groupKey->isEmpty()) {
        QStringList groups(config.groupList());
        foreach(const QString &s, groups) {
            if (s.toLower() != "file information") {
                groupKey = s;
                break;
            }
        }
        if (groupKey.isEmpty()) {
            m_result = KDbResult(xi18n("File <filename>%1</filename> contains no connection information.",
                                       fileName));
            return false;
        }
        if (_groupKey)
            *_groupKey = groupKey;
    } else {
        if (!config.hasGroup(*_groupKey)) {
            m_result = KDbResult(xi18n("File <filename>%1</filename> does not contain group <resource>%2</resource>.",
                                       fileName, *_groupKey));
            return false;
        }
        groupKey = *_groupKey;
    }

    cg = config.group(groupKey);
    QString type(cg.readEntry("type", "database").toLower());

    bool isDatabaseShortcut;
    if (type == "database") {
        isDatabaseShortcut = true;
    } else if (type == "connection") {
        isDatabaseShortcut = false;
    } else {
        m_result = KDbResult(xi18n("Invalid value <resource>%1</resource> type specified in group "
                                   "<resource>%2</resource> of file <filename>%3</filename>.",
                                   type, groupKey, fileName));
        return false;
    }

    const QString driverName = cg.readEntry("engine").toLower();
    if (driverName.isEmpty()) {
        m_result = KDbResult(xi18n("No valid \"engine\" field specified in group <resource>%1</resource> "
                                   "of file <filename>%2</filename>.", groupKey, fileName));
        return false;
    }

    // verification OK, now applying the values:
    // -- "engine" is backward compatible simple name, not a driver ID
    d->connData.setDriverId(QString::fromLatin1("org.kde.kdb.") + driverName);
    formatVersion = _formatVersion;
    d->connData.setHostName(cg.readEntry("server")); //empty allowed, means localhost
    if (isDatabaseShortcut) {
        KDbDriverManager driverManager;
        const KDbDriverMetaData *driverMetaData = driverManager.driverMetaData(d->connData.driverId());
        if (!driverMetaData) {
             m_result = driverManager.result();
            return false;
        }
        const bool fileBased = driverMetaData->isFileBased()
                && driverMetaData->id() == d->connData.driverId();
        setCaption(cg.readEntry("caption"));
        setDescription(cg.readEntry("comment"));
        d->connData.setCaption(QString()); /* connection name is not specified... */
        d->connData.setDescription(QString());
        if (fileBased) {
            QString fn(cg.readEntry("name"));
            if (!fn.isEmpty()) {
                const QString homeVar("$HOME");
                if (fn.startsWith(homeVar)) {
                    QString home(QDir::homePath());
                    if (home.endsWith('/')) {
                        home.chop(1);
                    }
                    fn = home + fn.mid(homeVar.length());
                }
                d->connData.setDatabaseName(fn);
                setDatabaseName(d->connData.databaseName());
            }
        }
        else {
            setDatabaseName(cg.readEntry("name"));
        }
    } else { // connection
        d->connData.setDatabaseName(QString());
        setCaption(QString());
        d->connData.setCaption(cg.readEntry("caption"));
        setDescription(QString());
        d->connData.setDescription(cg.readEntry("comment"));
        setDatabaseName(QString());   /* db name is not specified... */
    }
    d->connData.setPort(cg.readEntry("port", 0));
    d->connData.setUseLocalSocketFile(cg.readEntry("useLocalSocketFile", true));
    d->connData.setLocalSocketFileName(cg.readEntry("localSocketFile"));
    d->connData.setSavePassword(cg.hasKey("password") || cg.hasKey("encryptedPassword"));
    if (formatVersion >= 2) {
        QString password = cg.readEntry("encryptedPassword");
        if (!KDbUtils::simpleDecrypt(&password)) {
            qWarning() << "Encrypted password in connection file" << fileName << "cannot be decrypted so won't be used. "
                          "Delete \"encryptedPassword\" line or correct it to fix this issue.";
            password.clear();
        }
        d->connData.setPassword(password);
    }
    if (!cg.hasKey("encryptedPassword")) {//no "encryptedPassword", for compatibility
        //UNSAFE
        d->connData.setPassword(cg.readEntry("password"));
    }
    d->connData.setUserName(cg.readEntry("user"));
    QString lastOpenedStr(cg.readEntry("lastOpened"));
    if (!lastOpenedStr.isEmpty()) {
        QDateTime lastOpened(QDateTime::fromString(lastOpenedStr, Qt::ISODate));
        if (lastOpened.isValid()) {
            setLastOpened(lastOpened);
        }
    }
    /*! @todo add "options=", eg. as string list? */
    return true;
}

//! @return a simple driver name (as used by Kexi <= 2.x) for KDb's driver ID
//! or empty string if matching name not found.
//! Example: "sqlite" name is returned for "org.kde.kdb.sqlite" ID.
static QString driverIdToKexi2DriverName(const QString &driverId)
{
    QString prefix = "org.kde.kdb.";
    if (!driverId.startsWith(prefix)) {
        return QString();
    }
    QString suffix = driverId.mid(prefix.length());
    if (suffix == "sqlite"
        || suffix == "mysql"
        || suffix == "postgresql"
        || suffix == "xbase"
        || suffix == "sybase")
    {
        return suffix;
    }
    return QString();
}

bool KexiProjectData::save(const QString& fileName, bool savePassword,
                           QString* _groupKey, bool overwriteFirstGroup)
{
    //! @todo how about readOnly arg?
    KConfig config(fileName, KConfig::SimpleConfig);
    KConfigGroup cg = config.group("File Information");

    int realFormatVersion = formatVersion;
    if (realFormatVersion == 0) /* 0 means "default version"*/
        realFormatVersion = KEXIPROJECTDATA_FORMAT;
    cg.writeEntry("version", realFormatVersion);

    const bool thisIsConnectionData = databaseName().isEmpty();

    //use or find a nonempty group key
    QString groupKey;
    if (_groupKey && !_groupKey->isEmpty()) {
        groupKey = *_groupKey;
    } else {
        QString groupPrefix;
        const QStringList groups(config.groupList());
        if (overwriteFirstGroup && !groups.isEmpty()) {
            foreach(const QString &s, groups) {
                if (s.toLower() != "file information") {
                    groupKey = s;
                    break;
                }
            }
        }

        if (groupKey.isEmpty()) {
            //find a new unique name
            if (thisIsConnectionData)
                groupPrefix = "Connection%1"; //do not i18n!
            else
                groupPrefix = "Database%1"; //do not i18n!

            int number = 1;
            while (config.hasGroup(groupPrefix.arg(number))) //a new group key couldn't exist
                number++;
            groupKey = groupPrefix.arg(number);
        }
        if (_groupKey) //return this one (generated or found)
            *_groupKey = groupKey;
    }

    config.group(groupKey).deleteGroup();
    cg = config.group(groupKey);
    KDbDriverManager manager;
    const KDbDriverMetaData *metaData = manager.driverMetaData(d->connData.driverId());
    if (!metaData) {
        m_result = manager.result();
        return false;
    }
    const bool fileBased = metaData->isFileBased();
    if (thisIsConnectionData) {
        cg.writeEntry("type", "connection");
        if (!d->connData.caption().isEmpty())
            cg.writeEntry("caption", d->connData.caption());
        if (!d->connData.description().isEmpty())
            cg.writeEntry("comment", d->connData.description());
    } else { //database
        cg.writeEntry("type", "database");
        if (!caption().isEmpty())
            cg.writeEntry("caption", caption());
        if (fileBased) {
            QString fn(d->connData.databaseName());
            if (!QDir::homePath().isEmpty() && fn.startsWith(QDir::homePath())) {
                // replace prefix if == $HOME
                fn = fn.mid(QDir::homePath().length());
                if (!fn.startsWith('/'))
                    fn.prepend('/');
                fn.prepend(QLatin1String("$HOME"));
            }
            cg.writeEntry("name", fn);
        }
        else { // server-based
            cg.writeEntry("name", databaseName());
        }
        if (!description().isEmpty())
            cg.writeEntry("comment", description());
    }

    QString engine = driverIdToKexi2DriverName(d->connData.driverId());
    if (engine.isEmpty()) {
        engine = d->connData.driverId();
    }
    cg.writeEntry("engine", engine);
    if (!fileBased) {
        if (!d->connData.hostName().isEmpty())
            cg.writeEntry("server", d->connData.hostName());

        if (d->connData.port() != 0)
            cg.writeEntry("port", int(d->connData.port()));
        cg.writeEntry("useLocalSocketFile", d->connData.useLocalSocketFile());
        if (!d->connData.localSocketFileName().isEmpty())
            cg.writeEntry("localSocketFile", d->connData.localSocketFileName());
        if (!d->connData.userName().isEmpty())
            cg.writeEntry("user", d->connData.userName());
    }

    if (savePassword || d->connData.savePassword()) {
        if (realFormatVersion < 2) {
            cg.writeEntry("password", d->connData.password());
        } else {
            QString encryptedPassword = d->connData.password();
            KDbUtils::simpleCrypt(&encryptedPassword);
            cg.writeEntry("encryptedPassword", encryptedPassword);
            encryptedPassword.fill(' '); //for security
        }
    }

    if (lastOpened().isValid()) {
        cg.writeEntry("lastOpened", lastOpened().toString(Qt::ISODate));
    }

    /*! @todo add "options=", eg. as string list? */
    if (!cg.sync()) {
        m_result = KDbResult("Could not write project file.");
        return false;
    }
    return true;
}

KEXICORE_EXPORT QDebug operator<<(QDebug dbg, const KexiProjectData& data)
{
    dbg.space() << "KexiProjectData" << "databaseName=" << data.databaseName()
        << "lastOpened=" << data.lastOpened() << "description=" << data.description()
        << "connectionData=(";
    if (data.connectionData()) {
        dbg.nospace() << *data.connectionData();
    }
    dbg.nospace() << ")";
    return dbg.space();
}
