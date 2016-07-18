/* This file is part of the KDE project
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

#ifndef KEXI_MIGRATION_MNGR_H
#define KEXI_MIGRATION_MNGR_H

#include <QObject>
#include <QMap>

#include <kservice.h>

#include <core/KexiMigrateManagerInterface.h>

#include "keximigrate.h"

namespace KexiMigration
{

class MigrateManagerInternal;

//! @short Migration library management, for finding and loading migration drivers.
class KEXIMIGRATE_EXPORT MigrateManager : public QObject,
                                          public KexiMigrateManagerInterface
{
public:
    MigrateManager();

    virtual ~MigrateManager();

    //! @return result of the recent operation.
    KDbResult result() const;

    //! @return KDbResultable object for the recent operation.
    //! It adds serverResultName() in addition to the result().
    const KDbResultable* resultable() const;

    /*! Tries to load db driver with identifier @a id.
      The id is case insensitive.
      \return db driver, or @c nullptr on error (then error message is also set) */
    KexiMigrate* driver(const QString &id);

    /*! returns list of available drivers IDs.
      That drivers can be loaded by first use of driver() method. */
    const QStringList driverIds();

    /*! @return list of driver IDs for @a mimeType mime type.
     Empty list is returned if no driver has been found.
     Works only with drivers of file-based databases such as SQLite.
     The lookup is case insensitive. */
    QStringList driverIdsForMimeType(const QString &mimeType);

//! @todo copied from KDbDriverManager, merge it.
    /*! HTML information about possible problems encountered.
     It's displayed in 'details' section, if an error encountered.
     Currently it contains a list of incompatible migration drivers. */
    QString possibleProblemsMessage() const;

    //! @return list of file MIME types that are supported by migration drivers
    QStringList supportedFileMimeTypes() Q_DECL_OVERRIDE;
};

} //namespace KexiMigrate

#endif
