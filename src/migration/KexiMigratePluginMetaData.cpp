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

#include "KexiMigratePluginMetaData.h"
#include <KexiJsonTrader.h>
#include <QPluginLoader>
#include <QVariant>
#include <QDebug>

static bool isTrue(const KexiMigratePluginMetaData *metaData, const char* fieldName)
{
    return 0 == metaData->value(QLatin1String(fieldName))
                .compare(QLatin1String("true"), Qt::CaseInsensitive);
}

class Q_DECL_HIDDEN KexiMigratePluginMetaData::Private
{
public:
    explicit Private(const KexiMigratePluginMetaData *metaData, const QPluginLoader &pluginLoader)
        : isFileBased(isTrue(metaData, "X-Kexi-FileBased"))
        , supportedSourceDrivers(metaData->readStringList(
                                    KexiJsonTrader::metaDataObjectForPluginLoader(pluginLoader),
                                    QLatin1String("X-Kexi-SupportedSourceDrivers")))
    {
    }
    const bool isFileBased;
    const QStringList supportedSourceDrivers;
};

KexiMigratePluginMetaData::KexiMigratePluginMetaData(const QPluginLoader &loader)
    : KexiPluginMetaData(loader), d(new Private(this, loader))
{
}

KexiMigratePluginMetaData::~KexiMigratePluginMetaData()
{
    delete d;
}

QStringList KexiMigratePluginMetaData::supportedSourceDrivers() const
{
    return d->supportedSourceDrivers;
}
