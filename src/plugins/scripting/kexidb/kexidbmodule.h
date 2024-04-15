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

#include <QString>
#include <QObject>

#include <KDbDriverManager>
#include <KDbConnection>

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
 * the KDb functionality.
 */
class KexiDBModule : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE explicit KexiDBModule(QObject* parent = nullptr);
    virtual ~KexiDBModule();

    /** Returns the version number the KexiDB module defines. */
    Q_INVOKABLE int version();

    /** Returns a list with available drivernames. */
    Q_INVOKABLE QStringList driverNames();

    /** Return the to the defined \p drivername matching \a KexiDBDriver object. */
    Q_INVOKABLE QObject* driver(const QString& drivername);

    /** Return the to the defined mimetype-string matching drivername. */
    Q_INVOKABLE QString lookupByMime(const QString& mimetype);

    /** Return the matching mimetype for the defined file. */
    Q_INVOKABLE QString mimeForFile(const QString& filename);

    /** Return a new \a KexiDBConnectionData object. */
    Q_INVOKABLE QObject* createConnectionData();

    /** Create and return a \a KexiDBConnectionData object. Fill the content of the
    KexiDBConnectionData object with the defined file as. The file could be e.g.
    a *.kexi file or a *.kexis file. */
    Q_INVOKABLE QObject* createConnectionDataByFile(const QString& filename);

    /** Return a new \a KexiDBField object. */
    Q_INVOKABLE QObject* field();

    /** Return a new \a KexiDBTableSchema object. */
    Q_INVOKABLE QObject* tableSchema(const QString& tablename);

    /** Return a new \a KexiDBQuerySchema object. */
    Q_INVOKABLE QObject* querySchema();

private Q_SLOTS:
    //! Wraps a KDbConnection into a KexiDBConnection
    QObject* connectionWrapper(KDbConnection* connection);

private:
    KDbDriverManager m_drivermanager;

};

}

#endif

