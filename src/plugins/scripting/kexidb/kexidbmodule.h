/***************************************************************************
 * kexidbmodule.h
 * This file is part of the KDE project
 * copyright (C)2004-2006 by Sebastian Sauer (mail@dipe.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 ***************************************************************************/

#ifndef SCRIPTING_KEXIDBMODULE_H
#define SCRIPTING_KEXIDBMODULE_H

#include <qstring.h>
#include <qvariant.h>
#include <qobject.h>

#include <kexidb/drivermanager.h>

namespace Scripting
{

// Forward declarations.
class KexiDBDriver;
class KexiDBConnectionData;
class KexiDBField;
class KexiDBTableSchema;
class KexiDBQuerySchema;

/**
 * The KexiDBModule class provides the main entry point to deal with
 * the KexiDB functionality.
 */
class KexiDBModule : public QObject
{
    Q_OBJECT
public:
    explicit KexiDBModule(QObject* parent = 0);
    virtual ~KexiDBModule();

public slots:

    /** Returns the version number the KexiDB module defines. */
    int version();

    /** Returns a list with available drivernames. */
    const QStringList driverNames();

    /** Return the to the defined \p drivername matching \a KexiDBDriver object. */
    QObject* driver(const QString& drivername);

    /** Return the to the defined mimetype-string matching drivername. */
    const QString lookupByMime(const QString& mimetype);

    /** Return the matching mimetype for the defined file. */
    const QString mimeForFile(const QString& filename);

    /** Return a new \a KexiDBConnectionData object. */
    QObject* createConnectionData();

    /** Create and return a \a KexiDBConnectionData object. Fill the content of the
    KexiDBConnectionData object with the defined file as. The file could be e.g.
    a *.kexi file or a *.kexis file. */
    QObject* createConnectionDataByFile(const QString& filename);

    /** Return a new \a KexiDBField object. */
    QObject* field();

    /** Return a new \a KexiDBTableSchema object. */
    QObject* tableSchema(const QString& tablename);

    /** Return a new \a KexiDBQuerySchema object. */
    QObject* querySchema();

private Q_SLOTS:

    // Wraps a KexiDB::Connection into a KexiDBConnection
    QObject* connectionWrapper(QObject* connection);

private:
    ::KexiDB::DriverManager m_drivermanager;
};

}

#endif

