/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
   Copyright (C) 2007 David Faure <faure@kde.org>
   Copyright (C) 2015 Jarosław Staniek <staniek@kde.org>

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

#include "KexiJsonTrader.h"
#include "utils.h"

#include <QDebug>
#include <QList>
#include <QPluginLoader>
#include <QJsonObject>
#include <QJsonArray>
#include <QDirIterator>
#include <QDir>
#include <QCoreApplication>

class Q_DECL_HIDDEN KexiJsonTrader::Private
{
public:
    Private(const QString &subDir) : m_subDir(subDir)
    {
    }

    QStringList pluginPaths()
    {
        if (!m_pluginPathFound) {
            QStringList searchDirs;
            searchDirs += QCoreApplication::libraryPaths();
            for (const QString &dir : searchDirs) {
                const QString possiblePath = dir + QLatin1Char('/') + m_subDir;
                if (QDir(possiblePath).exists()) {
                    m_pluginPaths += possiblePath;
                }
            }
            m_pluginPathFound = true;
        }
        return m_pluginPaths;
    }

private:
    QString m_subDir;
    QStringList m_pluginPaths;
    bool m_pluginPathFound = false;
};

// ---

KexiJsonTrader::KexiJsonTrader(const QString& subDir)
    : d(new Private(subDir))
{
}

KexiJsonTrader::~KexiJsonTrader()
{
    delete d;
}

//! @return true if at least one service type from @a serviceTypeNames exists in @a foundServiceTypes
static bool supportsAtLeastServiceType(const QStringList &foundServiceTypes,
                                       const QStringList &serviceTypeNames)
{
    foreach(const QString &serviceTypeName, serviceTypeNames) {
        if (foundServiceTypes.contains(serviceTypeName)) {
            return true;
        }
    }
    return false;
}

//static
QJsonObject KexiJsonTrader::metaDataObjectForPluginLoader(const QPluginLoader &pluginLoader)
{
    return pluginLoader.metaData().value(QLatin1String("MetaData")).toObject();
}

//static
QJsonObject KexiJsonTrader::rootObjectForPluginLoader(const QPluginLoader &pluginLoader)
{
    QJsonObject json = metaDataObjectForPluginLoader(pluginLoader);
    if (json.isEmpty()) {
        return QJsonObject();
    }
    return json.value(QLatin1String("KPlugin")).toObject();
}

//! Checks loader @a loader
static bool checkLoader(QPluginLoader *loader, const QStringList &servicetypes,
                        const QString &mimetype)
{
    const QJsonObject pluginData = KexiJsonTrader::rootObjectForPluginLoader(*loader);
    if (pluginData.isEmpty()) {
        //qDebug() << dirIter.filePath() << "has no json!";
        return false;
    }
    const QJsonArray foundServiceTypesAray = pluginData.value(QLatin1String("ServiceTypes")).toArray();
    if (foundServiceTypesAray.isEmpty()) {
        qWarning() << "No ServiceTypes defined for plugin" << loader->fileName() << "-- skipping!";
        return false;
    }
    QStringList foundServiceTypes = KexiUtils::convertTypesUsingMethod<QVariant, QString, &QVariant::toString>(foundServiceTypesAray.toVariantList());
    if (!supportsAtLeastServiceType(foundServiceTypes, servicetypes)) {
        return false;
    }

    if (!mimetype.isEmpty()) {
        QJsonObject json = KexiJsonTrader::metaDataObjectForPluginLoader(*loader);
        QStringList mimeTypes = json.value(QLatin1String("X-KDE-ExtraNativeMimeTypes"))
                .toString().split(QLatin1Char(','));
        mimeTypes += json.value(QLatin1String("MimeType")).toString().split(QLatin1Char(';'));
        mimeTypes += json.value(QLatin1String("X-KDE-NativeMimeType")).toString();
        if (! mimeTypes.contains(mimetype)) {
            return false;
        }
    }
    return true;
}

static QList<QPluginLoader *> findPlugins(const QString &path, const QStringList &servicetypes,
                                          const QString &mimetype)
{
    QList<QPluginLoader*> list;
    QDirIterator dirIter(path,
                         /* QDirIterator::Subdirectories -- Since 3.0.1: Don't look into subdirs
                                                            because there may be 3.x dirs from
                                                            future Kexi versions. We will look
                                                            into subdirs since 3.1 again. */
                         QDirIterator::FollowSymlinks);
    while (dirIter.hasNext()) {
        dirIter.next();
        if (dirIter.fileInfo().isFile()) {
            QPluginLoader *loader = new QPluginLoader(dirIter.filePath());
            if (checkLoader(loader, servicetypes, mimetype)) {
                list.append(loader);
            } else {
                delete loader;
            }
        }
    }
    return list;
}

QList<QPluginLoader *> KexiJsonTrader::query(const QStringList &servicetypes,
                                             const QString &mimetype)
{
    QList<QPluginLoader *> list;
    foreach(const QString &path, d->pluginPaths()) {
        list += findPlugins(path, servicetypes, mimetype);
    }
    return list;
}

QList<QPluginLoader *> KexiJsonTrader::query(const QString &servicetype, const QString &mimetype)
{
    QStringList servicetypes;
    servicetypes << servicetype;
    return query(servicetypes, mimetype);
}

QStringList KexiJsonTrader::pluginPaths() const
{
    return d->pluginPaths();
}
