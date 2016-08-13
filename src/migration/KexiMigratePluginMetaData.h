/* This file is part of the KDE project
   Copyright (C) 2016 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIMIGRATEPLUGINMETADATA_H
#define KEXIMIGRATEPLUGINMETADATA_H

#include <KexiPluginMetaData.h>

#include "keximigrate_export.h"

namespace KexiMigration {
class MigrateManagerInternal;
}

//! Provides information about a single Kexi Migration driver plugin
class KEXIMIGRATE_EXPORT KexiMigratePluginMetaData : public KexiPluginMetaData
{
public:
    ~KexiMigratePluginMetaData();

protected:
    explicit KexiMigratePluginMetaData(const QPluginLoader &loader);

    //! @return true if the driver is for file-based database sources such as MS Access or SQLite.
    /*! Defined by a "X-Kexi-FileBased" field in .json information files. */
    bool isFileBased() const;

    //! @return list of KDb driver IDs supported for source databases by this migration driver.
    //! Defined by a "X-Kexi-SupportedSourceDrivers" field in .json information files.
    //! @todo This is infrlxible: the value assumes the source database is connected using
    //! KDb drivers. For now it's used mostly for server sources.
    QStringList supportedSourceDrivers() const;

    friend class KexiMigration::MigrateManagerInternal;

private:
    Q_DISABLE_COPY(KexiMigratePluginMetaData)
    class Private;
    Private * const d;
};

#endif
