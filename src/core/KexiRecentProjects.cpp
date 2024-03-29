/* This file is part of the KDE project
   Copyright (C) 2011-2014 Jarosław Staniek <staniek@kde.org>

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

#include "KexiRecentProjects.h"
#include "kexidbshortcutfile.h"
#include "kexidbconnectionset.h"
#include <kexi.h>

#include <KDbDriverManager>
#include <KDbDriverMetaData>
#include <KDbConnection>
#include <KDbMessageHandler>

#include <QDebug>
#include <QDir>
#include <QStandardPaths>

//#define KexiRecentProjects_DEBUG

//! @internal
class Q_DECL_HIDDEN KexiRecentProjects::Private
{
public:
    explicit Private(KexiRecentProjects *qq)
        : handler(0), q(qq), loaded(false)
    {
    }
    ~Private()
    {
        qDeleteAll(toDelete);
    }
    void load();
    bool add(KexiProjectData *data, const QString& existingShortcutPath,
             bool deleteDuplicate = false);

    KDbMessageHandler* handler;
    QMap<KexiProjectData*, QString> shortcutPaths;
private:
    KexiRecentProjects *q;
    bool loaded;
    QString path;
    QMap<QString, KexiProjectData*> projectsForKey;
    QSet<KexiProjectData*> toDelete;
};

void KexiRecentProjects::Private::load()
{
    if (loaded)
        return;
    if (!Kexi::isKexiInstance()) {
        // Do not show the list of documents if this is not really Kexi but a test app based on Kexi
        return;
    }
    loaded = true;
#ifdef KexiRecentProjects_DEBUG
    qDebug() << "wait..";
    sleep(2);
#endif
    path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
            + "/kexi/recent_projects/";
    QDir dir(path);
    if (!dir.mkpath(path)) {
        q->m_result.setMessage(xi18n("Could not create folder <filename>%1</filename> for "
                                     "storing recent projects information.", path));
        return;
    }
    if (!dir.exists() || !dir.isReadable()) {
        return;
    }
    const QStringList shortcuts
        = dir.entryList(QStringList() << QLatin1String("*.kexis"),
                        QDir::Files | QDir::NoSymLinks | QDir::Readable | QDir::CaseSensitive
                            | QDir::Hidden // Hidden too because there can be names starting with
                                           // dot or hidden for without a clear reason
        );
#ifdef KexiRecentProjects_DEBUG
    qDebug() << shortcuts;
#endif
    foreach (const QString& shortcutPath, shortcuts) {
#ifdef KexiRecentProjects_DEBUG
        qDebug() << shortcutPath;
#endif
        KexiProjectData *data = new KexiProjectData;
        bool ok = data->load(path + shortcutPath);
#ifdef KexiRecentProjects_DEBUG
        qDebug() << "result:" << ok;
#endif
        if (ok) {
            add(data, path + shortcutPath, true /*deleteDuplicate*/);
        }
        else {
            q->m_result = data->result();
            delete data;
        }
    }
}

static QString key(const KexiProjectData& data)
{
    return KexiDBConnectionSet::key(*data.connectionData())
        + ',' + data.databaseName();
}

bool KexiRecentProjects::Private::add(KexiProjectData *newData,
                                      const QString& existingShortcutPath,
                                      bool deleteDuplicate)
{
    //qDebug() << *newData;
    KexiProjectData::List list(q->list()); // also loads it
    if (list.contains(newData))
        return true;

    // find similar project data
    QString newDataKey = key(*newData);
#ifdef KexiRecentProjects_DEBUG
    qDebug() << "path:" << path << "newDataKey:" << newDataKey;
    qDebug() << "projectsForKey.keys():" << projectsForKey.keys();
    qDebug() << "shortcutPaths.values():" << shortcutPaths.values();
#endif
    KexiProjectData* existingData = projectsForKey.value(newDataKey);
    QString shortcutPath = existingShortcutPath;
    if (existingData && existingData->lastOpened() < newData->lastOpened()) {
        if (q->takeProjectDataInternal(existingData)) {
            // this data will be replaced by similar
#ifdef KexiRecentProjects_DEBUG
            qDebug() << "Existing data replaced by new similar:"
                        << "\nexisting:" << *existingData
                        << "\nnew:" << *newData;
#endif
            if (deleteDuplicate) {
                QString fileToRemove(shortcutPaths.value(existingData));

                delete existingData;
#ifdef KexiRecentProjects_DEBUG
                qDebug() << "Removing unnecessary file shortcut:" << fileToRemove;
#endif
                if (!QFile::remove(fileToRemove)) {
                    qWarning() << "Failed to remove unnecessary recent file shortuct:"
                               << fileToRemove;
                }
            }
            else { // cannot be deleted now, remember
                toDelete.insert(existingData);
            }
        }
        if (shortcutPath.isEmpty()) {
            shortcutPath = shortcutPaths.value(existingData); // reuse this fileName
        }
        projectsForKey.remove(newDataKey);
        shortcutPaths.remove(existingData);
    }
    else { // no existing data or existing is newer
        if (existingData && existingData->lastOpened() >= newData->lastOpened()) {
            // the new data is older than existing
            // this data is replaced by similar
#ifdef KexiRecentProjects_DEBUG
            qDebug() << "New data is older than existing - removing new:"
                        << "\nexisting:" << *existingData
                        << "\nnew:" << *newData;
#endif
            if (deleteDuplicate) {
                delete newData;
#ifdef KexiRecentProjects_DEBUG
                qDebug() << "Removing unnecessary file shortcut:" << existingShortcutPath;
#endif
                if (!QFile::remove(existingShortcutPath)) {
                    qWarning() << "Failed to remove unnecessary recent file shortuct:"
                               << existingShortcutPath;
                }
            }
            else { // cannot be deleted now, remember
                toDelete.insert(newData);
            }
            return true;
        }
        else {
#ifdef KexiRecentProjects_DEBUG
            qDebug() << "New data:" << *newData;
#endif
        }
        if (shortcutPath.isEmpty()) {
            KDbConnectionData conn = *newData->connectionData();
            KDbDriverManager manager;
            const KDbDriverMetaData *metaData = manager.driverMetaData(conn.driverId());
            if (!metaData) {
                q->m_result = manager.result();
                return false;
            }
            if (metaData->isFileBased()) {
                shortcutPath = QFileInfo(newData->databaseName()).fileName();
                QFileInfo fi(shortcutPath);
                if (!fi.suffix().isEmpty()) {
                    shortcutPath.chop(fi.suffix().length() + 1);
                }
            } else {
                shortcutPath = newData->databaseName();
                if (!conn.hostName().isEmpty()) {
                    shortcutPath += '_' + conn.hostName();
                }
            }
            if (shortcutPath.startsWith('.')) {
                shortcutPath.prepend('_');
            }
            shortcutPath = path + shortcutPath;
            int suffixNumber = 0;
            QString suffixNumberString;
            forever { // add "_{number}" to ensure uniqueness
                if (!QFile::exists(shortcutPath + suffixNumberString + QLatin1String(".kexis")))
                    break;
                suffixNumber++;
                suffixNumberString = QString("_%1").arg(suffixNumber);
            }
            shortcutPath += (suffixNumberString + QLatin1String(".kexis"));
        }
    }
    projectsForKey.insert(newDataKey, newData);
    shortcutPaths.insert(newData, shortcutPath);
    q->addProjectDataInternal(newData);

#ifdef KexiRecentProjects_DEBUG
    qDebug() << "existingShortcutPath:" << existingShortcutPath;
    qDebug() << "shortcutPath:" << shortcutPath;
#endif
    bool result = true;
    if (existingShortcutPath.isEmpty()) {
        result = newData->save(shortcutPath, false /* !savePassword */);
    }
#ifdef KexiRecentProjects_DEBUG
    qDebug() << "result:" << result;
#endif
    return result;
}

KexiRecentProjects::KexiRecentProjects(KDbMessageHandler* handler)
    : KexiProjectSet()
    , d(new Private(this))
{
    d->handler = handler;
}

KexiRecentProjects::~KexiRecentProjects()
{
    delete d;
}

void KexiRecentProjects::addProjectData(const KexiProjectData &data)
{
    if (!Kexi::isKexiInstance()) {
        // Do not update the list of documents if this is not really Kexi but a test app based on Kexi
        return;
    }
    d->add(new KexiProjectData(data), QString() /*save new shortcut*/);
}

void KexiRecentProjects::addProjectDataInternal(KexiProjectData *data)
{
    KexiProjectSet::addProjectData(data);
}

KexiProjectData* KexiRecentProjects::takeProjectDataInternal(KexiProjectData *data)
{
    return KexiProjectSet::takeProjectData(data);
}

KexiProjectData::List KexiRecentProjects::list() const
{
    d->load();
    return KexiProjectSet::list();
}

QString KexiRecentProjects::shortcutPath(const KexiProjectData& data) const
{
    return d->shortcutPaths.value(const_cast<KexiProjectData*>(&data));
}
