/***************************************************************************
 * kexidbdriver.cpp
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

#include "kexidbdriver.h"
#include "kexidbconnection.h"
#include "kexidbconnectiondata.h"

#include <KDbConnection>
#include <KDbDriverMetaData>

using namespace Scripting;

KexiDBDriver::KexiDBDriver(QObject* parent, KDbDriver* driver)
        : QObject(parent)
        , m_driver(driver)
{
    setObjectName("KexiDBDriver");
}

KexiDBDriver::~KexiDBDriver()
{
}

bool KexiDBDriver::isValid()
{
    return !m_driver->result().isError();
}

QString KexiDBDriver::escapeString(const QString& s)
{
    return m_driver->escapeString(KDbEscapedString(s).toString()).toString();
}
bool KexiDBDriver::isFileDriver()
{
    return m_driver->metaData()->isFileBased();
}
QStringList KexiDBDriver::fileDBDriverMimeTypes()
{
    return m_driver->metaData()->mimeTypes();
}
bool KexiDBDriver::isSystemObjectName(const QString& name)
{
    return m_driver->isSystemObjectName(name);
}
bool KexiDBDriver::isSystemDatabaseName(const QString& name)
{
    return m_driver->isSystemDatabaseName(name);
}
bool KexiDBDriver::isSystemFieldName(const QString& name)
{
    return m_driver->isSystemFieldName(name);
}
QString KexiDBDriver::valueToSql(const QString& fieldtype, const QVariant& value)
{
    return m_driver->valueToSql(KDbField::typeForString(fieldtype), value).toString();
}

QObject* KexiDBDriver::createConnection(QObject* data)
{
    KexiDBConnectionData* d = dynamic_cast<KexiDBConnectionData*>(data);
    return d ? new KexiDBConnection(m_driver->createConnection(*d->data())) : 0;
}

int KexiDBDriver::connectionCount()
{
    return m_driver->connections().count();
}

//! @todo
/*QObject* KexiDBDriver::connection(int index) {
    QSet<KDbConnection*> list = m_driver->connectionsList();
    return (index < list.count()) ? list.at(index) : 0;
}*/

