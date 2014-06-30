/* This file is part of the KDE project
   Daniel Molkentin <molkentin@kde.org>
   Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2003-2014 Jarosław Staniek <staniek@kde.org>

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

#include <core/KexiMainWindowIface.h>

#include <ktrader.h>
#include <kdebug.h>
#include <klocale.h>
#include <kservice.h>

#include <assert.h>

#include <QApplication>

//remove debug
#undef KexiDBDbg
#define KexiDBDbg if (0) kDebug()

using namespace KexiMigration;

MigrateManagerInternal* MigrateManagerInternal::s_self = 0L;

/*! @todo
 Temporary, needed because MigrateManagerInternal::m_drivers is autodeleted
 drivers currently own KexiMigrate::Data members so these are destroyed when
 last MigrateManager instance is deleted. Remove this hack when
 KexiMigrate is split into Driver and Connection. */
MigrateManager __manager;

MigrateManagerInternal::MigrateManagerInternal() /* protected */
        : QObject(0)
        , Object()
        , m_refCount(0)
        , lookupDriversNeeded(true)
{
    setObjectName("KexiMigrate::MigrateManagerInternal");
    m_serverResultNum = 0;

}

MigrateManagerInternal::~MigrateManagerInternal()
{
    KexiDBDbg;
    qDeleteAll(m_drivers);
    m_drivers.clear();
    if (s_self == this)
        s_self = 0;
    KexiDBDbg << "ok";
}

void MigrateManagerInternal::slotAppQuits()
{
    if (KexiMainWindowIface::global() && KexiMainWindowIface::global()->thisWidget()
        && KexiMainWindowIface::global()->thisWidget()->isVisible())
    {
        return; //what a hack! - we give up when app is still there
    }
    KexiDBDbg << "let's clear drivers...";
    m_drivers.clear();
    qDeleteAll(m_drivers);
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
//! @todo for Qt-only version check for KComponentData wrapper
//  KexiDBWarn << "cannot work without KComponentData (KGlobal::mainComponent()==0)!";
//  setError("Driver Manager cannot work without KComponentData (KGlobal::mainComponent()==0)!");

    lookupDriversNeeded = false;
    clearError();
    KService::List tlist = KServiceTypeTrader::self()->query("Kexi/MigrationDriver");
    KService::List::ConstIterator it(tlist.constBegin());
    for (; it != tlist.constEnd(); ++it) {
        KService::Ptr ptr = (*it);
        QString srv_name = ptr->property("X-Kexi-MigrationDriverName").toString();
        if (srv_name.isEmpty()) {
            KexiDBWarn << "X-Kexi-MigrationDriverName must be set for migration driver"
            << ptr->property("Name").toString() << "service!\n -- skipped!";
            continue;
        }
        if (m_services_lcase.contains(srv_name.toLower())) {
            continue;
        }

//! @todo could be merged. Copied from KexiDB::DriverManager.
//<COPIED>
        QString srv_ver_str = ptr->property("X-Kexi-KexiMigrationVersion").toString();
        QStringList lst(srv_ver_str.split('.'));
        int minor_ver, major_ver;
        bool ok = (lst.count() == 2);
        if (ok)
            major_ver = lst[0].toUInt(&ok);
        if (ok)
            minor_ver = lst[1].toUInt(&ok);
        if (!ok) {
            KexiDBWarn << "problem with detecting" << srv_name.toLower() << "driver's version -- skipping it!";
            possibleProblems += QString("\"%1\" migration driver has unrecognized version; "
                                        "required driver version is \"%2.%3\"")
                                .arg(srv_name.toLower())
                                .arg(KexiMigration::version().major).arg(KexiMigration::version().minor);
            continue;
        }
        if (!KexiMigration::version().matches(major_ver, minor_ver)) {
            KexiDBWarn << QString("'%1' driver"
                                  " has version '%2' but required migration driver version is '%3.%4'\n"
                                  " -- skipping this driver!").arg(srv_name.toLower()).arg(srv_ver_str)
            .arg(KexiMigration::version().major).arg(KexiMigration::version().minor);
            possibleProblems += QString("\"%1\" migration driver has version \"%2\" "
                                        "but required driver version is \"%3.%4\"")
                                .arg(srv_name.toLower()).arg(srv_ver_str)
                                .arg(KexiMigration::version().major).arg(KexiMigration::version().minor);
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
                    KexiDBWarn << "more than one driver for" << mime << "mime type!";
                }
            }
        }
        m_services.insert(srv_name, ptr);
        m_services_lcase.insert(srv_name.toLower(), ptr);
        KexiDBDbg << "registered driver:" << ptr->name() << '(' << ptr->library() << ")";
    }

    if (tlist.isEmpty()) {
        setError(ERR_DRIVERMANAGER, i18n("Could not find any import/export database drivers."));
        return false;
    }
    return true;
}

KexiMigrate* MigrateManagerInternal::driver(const QString& name)
{
    if (!lookupDrivers()) {
        kWarning() << "lookupDrivers failed";
        return 0;
    }

    clearError();
    KexiDBDbg << "loading" << name;

    KexiMigrate *drv = name.isEmpty() ? 0 : m_drivers.value(name.toLatin1().toLower());
    if (drv)
        return drv; //cached

    if (!m_services_lcase.contains(name.toLower())) {
        setError(ERR_DRIVERMANAGER,
                 i18n("Could not find import/export database driver \"%1\".", name));
        return 0;
    }

    KService::Ptr ptr = *(m_services_lcase.find(name.toLower()));
    QString srv_name = ptr->property("X-Kexi-MigrationDriverName").toString();

    KexiDBDbg << "library:" << ptr->library();

    KPluginLoader loader(ptr->library());
    const uint foundMajor = (loader.pluginVersion() >> 16) & 0xff;
    const uint foundMinor = (loader.pluginVersion() >> 8) & 0xff;
    if (!KexiMigration::version().matches(foundMajor, foundMinor)) {
        setError(ERR_INCOMPAT_DRIVER_VERSION,
                 i18n(
                     "Incompatible migration driver's \"%1\" version: found version %2, expected version %3.",
                     name,
                     QString("%1.%2").arg(foundMajor).arg(foundMinor),
                     QString("%1.%2").arg(KexiMigration::version().major).arg(KexiMigration::version().minor))
                );
        return 0;
    }

    KPluginFactory *factory = loader.factory();
    if (factory)
        drv = factory->create<KexiMigrate>(this);

    if (!drv) {
        setError(ERR_DRIVERMANAGER,
                 i18n("Could not load import/export database driver \"%1\".", name));
        return 0;
    }
    KexiDBDbg << "loading succeeded:" << name;
    KexiDBDbg << "drv=" << (long)drv;

    drv->setObjectName(srv_name);
    m_drivers.insert(name.toLatin1().toLower(), drv); //cache it
    return drv;
}

void MigrateManagerInternal::incRefCount()
{
    m_refCount++;
    KexiDBDbg << m_refCount;
}

void MigrateManagerInternal::decRefCount()
{
    m_refCount--;
    KexiDBDbg << m_refCount;
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
}

MigrateManager::~MigrateManager()
{
    KexiDBDbg;
    d_int->decRefCount();
    if (d_int->m_refCount == 0) {
        delete d_int;
    }
    KexiDBDbg << "ok";
}


const QStringList MigrateManager::driverNames()
{
    clearError();
    if (!d_int->lookupDrivers()) {
        kWarning() << "lookupDrivers failed";
        return QStringList();
    }
    if (d_int->m_services.isEmpty()) {
        kWarning() << "MigrateManager::ServicesMap is empty";
        return QStringList();
    }
    if (d_int->error()) {
        kWarning() << "Error:" << d_int->errorMsg();
        return QStringList();
    }
    return d_int->m_services.keys();
}

QString MigrateManager::driverForMimeType(const QString &mimeType)
{
    clearError();
    if (!d_int->lookupDrivers()) {
        kWarning() << "lookupDrivers failed";
        setError(d_int);
        return 0;
    }

    KService::Ptr ptr = d_int->m_services_by_mimetype[mimeType.toLower()];
    if (!ptr) {
        kWarning() << "No such mimetype" << mimeType;
        return QString();
    }

    return ptr->property("X-Kexi-MigrationDriverName").toString();
}

KexiMigrate* MigrateManager::driver(const QString& name)
{
    clearError();
    KexiMigrate *drv = d_int->driver(name);
    if (d_int->error()) {
        kWarning() << "Error:" << name << d_int->errorMsg();
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

QList<QString> MigrateManager::supportedFileMimeTypes()
{
    clearError();
    if (!d_int->lookupDrivers()) {
        kWarning() << "lookupDrivers failed";
        return QStringList();
    }
    return d_int->m_services_by_mimetype.keys();
}

#include "migratemanager_p.moc"
