/* This file is part of the KDE project
   Daniel Molkentin <molkentin@kde.org>
   Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2003-2004 Jarosław Staniek <staniek@kde.org>

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

#include "migratemanager.h"
#include "migratemanager_p.h"
#include "keximigrate.h"

#include <klibloader.h>
#include <kparts/componentfactory.h>
#include <ktrader.h>
#include <kdebug.h>
#include <klocale.h>
#include <kservice.h>

#include <assert.h>

#include <qapplication.h>

//remove debug
#undef KexiDBDbg
#define KexiDBDbg if (0) kDebug()

using namespace KexiMigration;

MigrateManagerInternal* MigrateManagerInternal::s_self = 0L;

/*! @todo
 Temporary, needed because MigrateManagerInternal::m_drivers is autodeleted
 drivers currently own KexiMigrate::Data members so these are destroyed when
 last MigrateManager instance is deleted. Remove this hack when
 KexiMigrate is splitted into Driver and Connection. */
MigrateManager __manager;

MigrateManagerInternal::MigrateManagerInternal() /* protected */
        : QObject(0)
        , Object()
        , m_drivers(17, false)
        , m_refCount(0)
        , lookupDriversNeeded(true)
{
    setObjectName("KexiMigrate::MigrateManagerInternal");
    m_drivers.setAutoDelete(true);
    m_serverResultNum = 0;

}

MigrateManagerInternal::~MigrateManagerInternal()
{
    KexiDBDbg << "MigrateManagerInternal::~MigrateManagerInternal()";
    m_drivers.clear();
    if (s_self == this)
        s_self = 0;
    KexiDBDbg << "MigrateManagerInternal::~MigrateManagerInternal() ok";
}

void MigrateManagerInternal::slotAppQuits()
{
    if (qApp->mainWidget() && qApp->mainWidget()->isVisible())
        return; //what a hack! - we give up when app is still there
    KexiDBDbg << "MigrateManagerInternal::slotAppQuits(): let's clear drivers...";
    m_drivers.clear();
}

MigrateManagerInternal *MigrateManagerInternal::self()
{
    if (!s_self)
        s_self = new MigrateManagerInternal();

    return s_self;
}

bool MigrateManagerInternal::lookupDrivers()
{
    if (!lookupDriversNeeded)
        return true;

    if (qApp) {
        connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(slotAppQuits()));
    }
//TODO: for QT-only version check for KComponentData wrapper
//  KexiDBWarn << "DriverManagerInternal::lookupDrivers(): cannot work without KComponentData (KGlobal::mainComponent()==0)!";
//  setError("Driver Manager cannot work without KComponentData (KGlobal::mainComponent()==0)!");

    lookupDriversNeeded = false;
    clearError();
    KService::List tlist = KServiceTypeTrader::self()->query("Kexi/MigrationDriver");
    KService::List::ConstIterator it(tlist.constBegin());
    for (; it != tlist.constEnd(); ++it) {
        KService::Ptr ptr = (*it);
        QString srv_name = ptr->property("X-Kexi-MigrationDriverName").toString();
        if (srv_name.isEmpty()) {
            KexiDBWarn << "MigrateManagerInternal::lookupDrivers(): "
            "X-Kexi-MigrationDriverName must be set for migration driver \""
            << ptr->property("Name").toString() << "\" service!\n -- skipped!";
            continue;
        }
        if (m_services_lcase.contains(srv_name.toLower())) {
            continue;
        }

//! @todo could be merged. Copied from KexiDB::DriverManager.
//<COPIED>
        QString srv_ver_str = ptr->property("X-Kexi-KexiMigrationVersion").toString();
        QStringList lst(srv_ver_str.split("."));
        int minor_ver, major_ver;
        bool ok = (lst.count() == 2);
        if (ok)
            major_ver = lst[0].toUInt(&ok);
        if (ok)
            minor_ver = lst[1].toUInt(&ok);
        if (!ok) {
            KexiDBWarn << "MigrateManagerInternal::lookupDrivers(): problem with detecting '"
            << srv_name.toLower() << "' driver's version -- skipping it!";
            possibleProblems += QString("\"%1\" migration driver has unrecognized version; "
                                        "required driver version is \"%2.%3\"")
                                .arg(srv_name.toLower())
                                .arg(KexiMigration::versionMajor()).arg(KexiMigration::versionMinor());
            continue;
        }
        if (major_ver != KexiMigration::versionMajor() || minor_ver != KexiMigration::versionMinor()) {
            KexiDBWarn << QString("MigrateManagerInternal::lookupDrivers(): '%1' driver"
                                  " has version '%2' but required migration driver version is '%3.%4'\n"
                                  " -- skipping this driver!").arg(srv_name.toLower()).arg(srv_ver_str)
            .arg(KexiMigration::versionMajor()).arg(KexiMigration::versionMinor());
            possibleProblems += QString("\"%1\" migration driver has version \"%2\" "
                                        "but required driver version is \"%3.%4\"")
                                .arg(srv_name.toLower()).arg(srv_ver_str)
                                .arg(KexiMigration::versionMajor()).arg(KexiMigration::versionMinor());
            continue;
        }
//</COPIED>

        QString mime = ptr->property("X-Kexi-FileDBDriverMime").toString().toLower();
        QString drvType = ptr->property("X-Kexi-MigrationDriverType").toString().toLower();
        if (drvType == "file") {
            if (!mime.isEmpty()) {
                if (!m_services_by_mimetype.contains(mime)) {
                    m_services_by_mimetype.insert(mime, ptr);
                } else {
                    KexiDBWarn << "MigrateManagerInternal::lookupDrivers(): more than one driver for '"
                    << mime << "' mime type!";
                }
            }
        }
        m_services.insert(srv_name, ptr);
        m_services_lcase.insert(srv_name.toLower(), ptr);
        KexiDBDbg << "MigrateManager::lookupDrivers(): registered driver: " << ptr->name()
        << "(" << ptr->library() << ")";
    }

    if (tlist.isEmpty()) {
        setError(ERR_DRIVERMANAGER, i18n("Could not find any import/export database drivers."));
        return false;
    }
    return true;
}

KexiMigrate* MigrateManagerInternal::driver(const QString& name)
{
    if (!lookupDrivers())
        return 0;

    clearError();
    KexiDBDbg << "MigrationrManagerInternal::migrationDriver(): loading " << name;

    KexiMigrate *drv = name.isEmpty() ? 0 : m_drivers.find(name.toLatin1());
    if (drv)
        return drv; //cached

    if (!m_services_lcase.contains(name.toLower())) {
        setError(ERR_DRIVERMANAGER,
                 i18n("Could not find import/export database driver \"%1\".", name));
        return 0;
    }

    KService::Ptr ptr = *(m_services_lcase.find(name.toLower()));
    QString srv_name = ptr->property("X-Kexi-MigrationDriverName").toString();

    KexiDBDbg << "MigrateManagerInternal::driver(): library: " << ptr->library();
    drv = KService::createInstance<KexiMigrate>(ptr,
            this, QStringList(), &m_serverResultNum);
    if (!drv) {
        setError(ERR_DRIVERMANAGER,
                 i18n("Could not load import/export database driver \"%1\".", name));
        if (m_componentLoadingErrors.isEmpty()) {//fill errtable on demand
            m_componentLoadingErrors[KLibLoader::ErrNoServiceFound] = "ErrNoServiceFound";
            m_componentLoadingErrors[KLibLoader::ErrServiceProvidesNoLibrary] = "ErrServiceProvidesNoLibrary";
            m_componentLoadingErrors[KLibLoader::ErrNoLibrary] = "ErrNoLibrary";
            m_componentLoadingErrors[KLibLoader::ErrNoFactory] = "ErrNoFactory";
            m_componentLoadingErrors[KLibLoader::ErrNoComponent] = "ErrNoComponent";
        }
        m_serverResultName = m_componentLoadingErrors[m_serverResultNum];
        return 0;
    }
    KexiDBDbg << "MigrateManagerInternal::driver(): loading succeed: " << name;
    KexiDBDbg << "drv=" << (long)drv;

// drv->setName(srv_name.toLatin1());
// drv->d->service = ptr; //store info
// drv->d->fileDBDriverMimeType = ptr->property("X-Kexi-FileDBDriverMime").toString();
// drv->d->initInternalProperties();

    if (!drv->isValid()) {
        setError(drv);
        delete drv;
        return 0;
    }

    drv->setObjectName(srv_name);
    m_drivers.insert(name.toLatin1(), drv); //cache it
    return drv;
}

void MigrateManagerInternal::incRefCount()
{
    m_refCount++;
    KexiDBDbg << "MigrateManagerInternal::incRefCount(): " << m_refCount;
}

void MigrateManagerInternal::decRefCount()
{
    m_refCount--;
    KexiDBDbg << "MigrateManagerInternal::decRefCount(): " << m_refCount;
// if (m_refCount<1) {
//  KexiDBDbg<<"KexiDB::DriverManagerInternal::decRefCount(): reached m_refCount<1 -->deletelater()";
//  s_self=0;
//  deleteLater();
// }
}

// ---------------------------
// --- DriverManager impl. ---
// ---------------------------

MigrateManager::MigrateManager()
        : QObject(0)
        , Object()
        , d_int(MigrateManagerInternal::self())
{
    setObjectName("KexiMigrate::MigrateManager");
    d_int->incRefCount();
// if ( !s_self )
//  s_self = this;
// lookupDrivers();
}

MigrateManager::~MigrateManager()
{
    KexiDBDbg << "MigrateManager::~MigrateManager()";
    /* Connection *conn;
      for ( conn = m_connections.first(); conn ; conn = m_connections.next() ) {
        conn->disconnect();
        conn->m_driver = 0; //don't let the connection touch our driver now
        m_connections.remove();
        delete conn;
      }*/

    d_int->decRefCount();
    if (d_int->m_refCount == 0) {
        //delete internal drv manager!
        delete d_int;
    }
// if ( s_self == this )
    //s_self = 0;
    KexiDBDbg << "MigrateManager::~MigrateManager() ok";
}


const QStringList MigrateManager::driverNames()
{
    if (!d_int->lookupDrivers()) {
        kDebug() << "MigrateManager::driverNames() lookupDrivers failed";
        return QStringList();
    }

    if (d_int->m_services.isEmpty()) {
        kDebug() << "MigrateManager::driverNames() MigrateManager::ServicesMap is empty";
        return QStringList();
    }

    if (d_int->error()) {
        kDebug() << "MigrateManager::driverNames() Error: " << d_int->errorMsg();
        return QStringList();
    }

    return d_int->m_services.keys();
}

QString MigrateManager::driverForMimeType(const QString &mimeType)
{
    if (!d_int->lookupDrivers()) {
        kDebug() << "MigrateManager::driverForMimeType() lookupDrivers() failed";
        setError(d_int);
        return 0;
    }

    KService::Ptr ptr = d_int->m_services_by_mimetype[mimeType.toLower()];
    if (!ptr) {
        kDebug() << QString("MigrateManager::driverForMimeType(%1) No such mimetype").arg(mimeType);
        return QString();
    }

    return ptr->property("X-Kexi-MigrationDriverName").toString();
}

KexiMigrate* MigrateManager::driver(const QString& name)
{
    KexiMigrate *drv = d_int->driver(name);
    if (d_int->error()) {
        kDebug() << QString("MigrateManager::driver(%1) Error: %2")
        .arg(name).arg(d_int->errorMsg());
        setError(d_int);
    }
    return drv;
}

QString MigrateManager::serverErrorMsg()
{
    return d_int->m_serverErrMsg;
}

int MigrateManager::serverResult()
{
    return d_int->m_serverResultNum;
}

QString MigrateManager::serverResultName()
{
    return d_int->m_serverResultName;
}

void MigrateManager::drv_clearServerResult()
{
    d_int->m_serverErrMsg.clear();
    d_int->m_serverResultNum = 0;
    d_int->m_serverResultName.clear();
}

QString MigrateManager::possibleProblemsInfoMsg() const
{
    if (d_int->possibleProblems.isEmpty())
        return QString();
    QString str;
    str.reserve(1024);
    str = "<ul>";
    for (QStringList::ConstIterator it = d_int->possibleProblems.constBegin();
            it != d_int->possibleProblems.constEnd(); ++it) {
        str += (QString::fromLatin1("<li>") + *it + QString::fromLatin1("</li>"));
    }
    str += "</ul>";
    return str;
}

//------------------------

int KexiMigration::versionMajor()
{
    return KEXI_MIGRATION_VERSION_MAJOR;
}

int KexiMigration::versionMinor()
{
    return KEXI_MIGRATION_VERSION_MINOR;
}

#include "migratemanager_p.moc"
