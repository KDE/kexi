/* This file is part of the KDE project
   Daniel Molkentin <molkentin@kde.org>
   Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2003-2016 Jarosław Staniek <staniek@kde.org>

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
#include "KexiMigratePluginMetaData.h"
#include <core/KexiMainWindowIface.h>
#include <KexiJsonTrader.h>
#include <config-kexi.h>

#include <KDbVersionInfo>

#include <KLocalizedString>

#include <QDebug>
#include <QApplication>

#include <assert.h>

#ifdef KEXI_MIGRATEMANAGER_DEBUG
#define KexiMigrateManagerDebug qDebug()
#else
#define KexiMigrateManagerDebug while (0) qDebug()
#endif

using namespace KexiMigration;

Q_GLOBAL_STATIC(MigrateManagerInternal, s_self)
Q_GLOBAL_STATIC_WITH_ARGS(KexiJsonTrader, KexiMigrateTrader_instance, (KEXI_BASE_NAME_LOWER "/migrate"))


MigrateManagerInternal::MigrateManagerInternal()
    : m_lookupDriversNeeded(true)
{
}

MigrateManagerInternal::~MigrateManagerInternal()
{
    KexiMigrateManagerDebug;
    clear();
    KexiMigrateManagerDebug << "ok";
}

void MigrateManagerInternal::clear()
{
    KexiMigrateManagerDebug << "Clearing drivers...";
    qDeleteAll(m_drivers);
    m_drivers.clear();
    qDeleteAll(m_driversMetaData);
    m_driversMetaData.clear();
}

void MigrateManagerInternal::slotAppQuits()
{
    if (qApp && !qApp->topLevelWidgets().isEmpty()
            && qApp->topLevelWidgets().first()->isVisible()) {
        return; //what a hack! - we give up when app is still there
    }
    clear();
}

bool MigrateManagerInternal::lookupDrivers()
{
    if (!m_lookupDriversNeeded)
        return true;

    if (qApp) {
        connect(qApp, &QApplication::aboutToQuit, this, &MigrateManagerInternal::slotAppQuits);
    }

    m_lookupDriversNeeded = false;
    clearResult();

    QList<QPluginLoader*> offers
            = KexiMigrateTrader_instance->query(QLatin1String("Kexi/MigrationDriver"));
    const QString expectedVersion = QString::fromLatin1("%1.%2")
            .arg(KexiMigration::version().major()).arg(KexiMigration::version().minor());
    for(const QPluginLoader *loader : offers) {
        QScopedPointer<KexiMigratePluginMetaData> metaData(new KexiMigratePluginMetaData(*loader));
        if (m_driversMetaData.contains(metaData->id())) {
            qWarning() << "Migration driver with ID" << metaData->id() << "already found at"
                         << m_driversMetaData.value(metaData->id())->fileName()
                         << "-- skipping another at" << metaData->fileName();
            continue;
        }
//! @todo Similar version check could be merged with KDbDriverManager
        if (metaData->version() != expectedVersion) {
            qWarning() << QString("Migration driver '%1' (%2) has version '%3' but "
                                  "KexiMigration library requires version '%4'\n"
                                  " -- skipping this driver!")
                          .arg(metaData->id()).arg(metaData->fileName())
                          .arg(metaData->version())
                          .arg(expectedVersion);
            m_possibleProblems += QString("Migration driver \"%1\" (%2) has version \"%3\" "
                                        "but required version is \"%4\"")
                          .arg(metaData->id()).arg(metaData->fileName())
                          .arg(metaData->version())
                          .arg(expectedVersion);
            continue;
        }
        foreach (const QString& mimeType, metaData->mimeTypes()) {
            m_metadata_by_mimetype.insertMulti(mimeType, metaData.data());
        }
        foreach (const QString& sourceDriverId, metaData->supportedSourceDrivers()) {
            m_metadataBySourceDrivers.insertMulti(sourceDriverId, metaData.data());
        }
        m_driversMetaData.insert(metaData->id(), metaData.data());
        KexiMigrateManagerDebug << "registered driver" << metaData->id() << '(' << metaData->fileName() << ")";
        metaData.take();
    }

    if (m_driversMetaData.isEmpty()) {
        m_result = KDbResult(ERR_DRIVERMANAGER, xi18n("Could not find any migration database drivers."));
        return false;
    }
    return true;
}

QStringList MigrateManagerInternal::driverIds()
{
    if (!lookupDrivers()) {
        return QStringList();
    }
    if (m_driversMetaData.isEmpty() && result().isError()) {
        return QStringList();
    }
    return m_driversMetaData.keys();
}

const KexiMigratePluginMetaData* MigrateManagerInternal::driverMetaData(const QString &id)
{
    if (!lookupDrivers()) {
        return 0;
    }
    const KexiMigratePluginMetaData *metaData = m_driversMetaData.value(id.toLower());
    if (!metaData || m_result.isError()) {
        m_result = KDbResult(ERR_DRIVERMANAGER,
                             tr("Could not find migration driver \"%1\".").arg(id));
    }
    return metaData;
}

QStringList MigrateManagerInternal::driverIdsForMimeType(const QString &mimeType)
{
    if (!lookupDrivers()) {
        return QStringList();
    }
    const QList<KexiMigratePluginMetaData*> metaDatas(m_metadata_by_mimetype.values(mimeType.toLower()));
    QStringList result;
    for (const KexiMigratePluginMetaData* metaData : metaDatas) {
        result.append(metaData->id());
    }
    return result;
}

QStringList MigrateManagerInternal::driverIdsForSourceDriver(const QString &sourceDriverId)
{
    if (!lookupDrivers()) {
        return QStringList();
    }
    QStringList result;
    for (const KexiMigratePluginMetaData* metaData : m_metadataBySourceDrivers.values(sourceDriverId.toLower())) {
        result.append(metaData->id());
    }
    return result;
}

KexiMigrate* MigrateManagerInternal::driver(const QString& id)
{
    if (!lookupDrivers()) {
        qWarning() << "lookupDrivers failed";
        return 0;
    }

    clearResult();
    KexiMigrateManagerDebug << "loading" << id;

    KexiMigrate *driver = m_drivers.value(id.toLatin1().toLower());
    if (driver) {
        return driver; //cached
    }

    if (!m_driversMetaData.contains(id.toLower())) {
        m_result = KDbResult(ERR_OBJECT_NOT_FOUND,
                             tr("Could not find migration driver \"%1\".").arg(id));
        return 0;
    }

    const KexiMigratePluginMetaData *metaData = m_driversMetaData.value(id.toLower());
    KPluginFactory *factory = qobject_cast<KPluginFactory*>(metaData->instantiate());
    if (!factory) {
        m_result = KDbResult(ERR_CANNOT_LOAD_OBJECT,
                             tr("Could not load migration driver's plugin file \"%1\".")
                                .arg(metaData->fileName()));
        QPluginLoader loader(metaData->fileName()); // use this to get the message
        (void)loader.load();
        m_result.setServerMessage(loader.errorString());
        qWarning() << m_result.message() << m_result.serverMessage();
        return 0;
    }
    driver = factory->create<KexiMigrate>();
    if (!driver) {
        m_result = KDbResult(ERR_CANNOT_LOAD_OBJECT,
                             tr("Could not open migration driver \"%1\" from plugin file \"%2\".")
                                .arg(metaData->id())
                                .arg(metaData->fileName()));
        qWarning() << m_result.message();
        return 0;
    }
    driver->setMetaData(metaData);
    m_drivers.insert(id.toLower(), driver);
    return driver;
}

QStringList MigrateManagerInternal::possibleProblemsMessage() const
{
    return m_possibleProblems;
}

QStringList MigrateManagerInternal::supportedFileMimeTypes()
{
    if (!lookupDrivers()) {
        qWarning() << "lookupDrivers failed";
        return QStringList();
    }
    return m_metadata_by_mimetype.uniqueKeys();
}

QStringList MigrateManagerInternal::supportedSourceDriverIds()
{
    if (!lookupDrivers()) {
        qWarning() << "lookupDrivers failed";
        return QStringList();
    }
    return m_metadataBySourceDrivers.uniqueKeys();
}

// ---------------------------
// --- DriverManager impl. ---
// ---------------------------

MigrateManager::MigrateManager()
        : QObject(0)
{
    setObjectName("KexiMigrate::MigrateManager");
}

MigrateManager::~MigrateManager()
{
    KexiMigrateManagerDebug;
}

QStringList MigrateManager::driverIdsForMimeType(const QString &mimeType)
{
    return s_self->driverIdsForMimeType(mimeType);
}

QStringList MigrateManager::driverIdsForSourceDriver(const QString &sourceDriverId)
{
    return s_self->driverIdsForSourceDriver(sourceDriverId);
}

KexiMigrate* MigrateManager::driver(const QString &id)
{
    return s_self->driver(id);
}

QString MigrateManager::possibleProblemsMessage() const
{
    if (s_self->possibleProblemsMessage().isEmpty())
        return QString();
    QString str = "<ul>";
    for(const QString &message : s_self->possibleProblemsMessage()) {
        str += (QString::fromLatin1("<li>") + message + QString::fromLatin1("</li>"));
    }
    str += "</ul>";
    return str;
}

QStringList MigrateManager::supportedFileMimeTypes()
{
    return s_self->supportedFileMimeTypes();
}

QStringList MigrateManager::supportedSourceDriverIds()
{
    return s_self->supportedSourceDriverIds();
}

KDbResult MigrateManager::result() const
{
    return s_self->result();
}

const KDbResultable* MigrateManager::resultable() const
{
    return s_self;
}
