/* This file is part of the KDE project
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

#ifndef KEXI_MIGRATE_MANAGER_P_H
#define KEXI_MIGRATE_MANAGER_P_H

#include <QMap>

#include <KDbObject>
#include <KDbResult>

class KexiMigratePluginMetaData;

namespace KexiMigration
{

//! Internal class of the migrate manager.
class MigrateManagerInternal : public QObject, public KDbResultable
{
    Q_OBJECT
public:
    /*! Used by self() */
    MigrateManagerInternal();

    ~MigrateManagerInternal();

    QStringList driverIds();

    /*! Tries to load migrate driver @a id.
      @return driver, or @c nullptr on error (then error result is also set) */
    KexiMigrate* driver(const QString& id);

    const KexiMigratePluginMetaData* driverMetaData(const QString &id);

    //! @return all migration driver IDs that support mimetype @a mimeType
    QStringList driverIdsForMimeType(const QString &mimeType);

    //! @return all migration driver IDs that support source KDb database driver ID @a sourceDriverId
    QStringList driverIdsForSourceDriver(const QString &sourceDriverId);

    //! @return user-visible message describing problem with lookup of migration drivers
    QStringList possibleProblemsMessage() const;

    //! @return list of file MIME types that are supported by migration drivers
    QStringList supportedFileMimeTypes();

    //! @return list of KDb driver IDs supported that are supported by migration drivers
    QStringList supportedSourceDriverIds();

protected Q_SLOTS:
    /*! Used to destroy all drivers on QApplication quit, so even if there are
     manager's static instances that are destroyed on program
     "static destruction", drivers are not kept after QApplication death.
    */
    void slotAppQuits();

protected:
    bool lookupDrivers();

    void clear();

    QMultiMap<QString, KexiMigratePluginMetaData*> m_metadata_by_mimetype;
    QMultiMap<QString, KexiMigratePluginMetaData*> m_metadataBySourceDrivers;
    QMap<QString, KexiMigratePluginMetaData*> m_driversMetaData; //!< used to store driver metadata
    QMap<QString, KexiMigrate*> m_drivers; //!< for owning drivers
    QStringList m_possibleProblems;
    bool m_lookupDriversNeeded;
};
}

#endif
