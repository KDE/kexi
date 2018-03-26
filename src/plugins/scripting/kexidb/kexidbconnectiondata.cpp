/***************************************************************************
 * kexidbconnectiondata.cpp
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

#include "kexidbconnectiondata.h"


using namespace Scripting;

KexiDBConnectionData::KexiDBConnectionData() : QObject() {
    m_data = new KDbConnectionData();
}

KexiDBConnectionData::KexiDBConnectionData(QObject* parent, KDbConnectionData* data)
        : QObject(parent)
        , m_data(data)
{
    setObjectName("KexiDBConnectionData");
}

KexiDBConnectionData::~KexiDBConnectionData()
{
}

const QString KexiDBConnectionData::caption() const
{
    return m_data->caption();
}
void KexiDBConnectionData::setCaption(const QString& name)
{
    m_data->setCaption(name);
}

const QString KexiDBConnectionData::description() const
{
    return m_data->description();
}
void KexiDBConnectionData::setDescription(const QString& desc)
{
    m_data->setDescription(desc);
}

const QString KexiDBConnectionData::driverName() const
{
    return m_data->driverId();
}
void KexiDBConnectionData::setDriverName(const QString& driver)
{
    m_data->setDriverId(driver);
}

bool KexiDBConnectionData::localSocketFileUsed() const
{
    return m_data->useLocalSocketFile();
}
void KexiDBConnectionData::setLocalSocketFileUsed(bool used)
{
    m_data->setUseLocalSocketFile(used);
}
const QString KexiDBConnectionData::localSocketFileName() const
{
    return m_data->localSocketFileName();
}
void KexiDBConnectionData::setLocalSocketFileName(const QString& socketfilename)
{
    m_data->setLocalSocketFileName(socketfilename);
}

void KexiDBConnectionData::setDatabaseName(const QString& dbname)
{
    m_data->setDatabaseName(dbname);
}

const QString KexiDBConnectionData::hostName() const
{
    return m_data->hostName();
}
void KexiDBConnectionData::setHostName(const QString& hostname)
{
    m_data->setHostName(hostname);
}

int KexiDBConnectionData::port() const
{
    return m_data->port();
}
void KexiDBConnectionData::setPort(int p)
{
    m_data->setPort(p);
}

const QString KexiDBConnectionData::password() const
{
    return m_data->password();
}
void KexiDBConnectionData::setPassword(const QString& passwd)
{
    m_data->setPassword(passwd);
}

const QString KexiDBConnectionData::userName() const
{
    return m_data->userName();
}
void KexiDBConnectionData::setUserName(const QString& username)
{
    m_data->setUserName(username);
}
const QString KexiDBConnectionData::databaseName() const
{
    return m_data->databaseName();
}
const QString KexiDBConnectionData::toUserVisibleString() const
{
    return m_data->toUserVisibleString();
}

