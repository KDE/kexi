/* This file is part of the KDE project
   Copyright (C) 2005-2011 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIDBSHORTCUTFILE_H
#define KEXIDBSHORTCUTFILE_H

#include <QString>

#include "kexicore_export.h"

namespace KexiDB
{
class ConnectionData;
}

/*! Loads and saves information for a "shortcut to a connection" file containing
 connection information with database name (i.e. ProjectData).
 This is implementation for handling .KEXIS files.
 See http://www.kexi-project.org/wiki/wikiview/index.php@KexiMimeTypes_DataSaving_Loading.html
*/
class KEXICORE_EXPORT KexiDBShortcutFile
{
public:
    /*! Creates a new object for \a fileName. */
    explicit KexiDBShortcutFile(const QString& fileName);

    ~KexiDBShortcutFile();

    //! \return filename provided on this object's construction. */
    QString fileName() const;

protected:
    class Private;
    Private * const d;
};

/*! Loads and saves information for a "shortcut" file containing
 connection information (i.e. KexiDB::ConnectionData).
 This is implementation for handling .KEXIC files.
 See http://www.kexi-project.org/wiki/wikiview/index.php@KexiMimeTypes_DataSaving_Loading.html
*/
class KEXICORE_EXPORT KexiDBConnShortcutFile : protected KexiDBShortcutFile
{
public:
    /*! Creates a new object for \a fileName. */
    explicit KexiDBConnShortcutFile(const QString& fileName);

    ~KexiDBConnShortcutFile();

    /*! Loads connection data into \a data.
     \a groupKey, if provided will be set to a group key,
     so you can later use it in saveConnectionData().
     \return true on success. */
    bool loadConnectionData(KexiDB::ConnectionData& data, QString* groupKey = 0);

    /*! Saves connection data \a data to a shortcut file.
     If \a storePassword is true, password will be saved in the file,
     even if data.savePassword is false.
     Existing data is merged with new data. \a groupKey is reused, if specified.
     If \a overwriteFirstGroup is true (the default) first found group will be overwritten
     instead of creating of a new unique group. This mode is usable for updating .kexic files
     containing single connection data, what's used for storing connections repository.
     \return true on success. */
    bool saveConnectionData(const KexiDB::ConnectionData& data,
                            bool savePassword, QString* groupKey = 0, bool overwriteFirstGroup = true);

    //! \return filename provided on this object's construction. */
    QString fileName() const {
        return KexiDBShortcutFile::fileName();
    }

protected:
};

#endif
