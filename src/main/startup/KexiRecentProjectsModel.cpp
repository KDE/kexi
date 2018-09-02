/* This file is part of the KDE project
   Copyright (C) 2011-2018 Jarosław Staniek <staniek@kde.org>

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

#include "KexiRecentProjectsModel.h"
#include <kexi.h>
#include <core/KexiRecentProjects.h>
#include <core/kexiprojectdata.h>
#include <KexiIcon.h>

#include <KDbUtils>
#include <KDbDriverManager>
#include <KDbDriverMetaData>

#include <KLocalizedString>

#include <QFileInfo>
#include <QDebug>

KexiRecentProjectsModel::KexiRecentProjectsModel(
    const KexiRecentProjects& projects, QObject *parent)
 : QAbstractListModel(parent), m_projects(&projects)
{
}

int KexiRecentProjectsModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return m_projects->list().count();
}

QModelIndex KexiRecentProjectsModel::index(int row, int column,
                                           const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    if (row < 0 || row >= m_projects->list().count())
        return QModelIndex();
    return createIndex(row, column, (void*)m_projects->list().at(row));
}

//! @return "opened x minutes ago" string or similar
static QString openedString(const QDateTime& opened)
{
    //qDebug() << opened;
    const QDateTime cur(QDateTime::currentDateTime()); // date/time comparison will take care about timezones
    if (!opened.isValid() || opened >= cur)
        return QString();

    const int days = opened.daysTo(cur);
    if (days <= 1 && opened.secsTo(cur) < 24*60*60) {
        const int minutes = opened.secsTo(cur) / 60;
        const int hours = minutes / 60;
        if (hours < 1) {
            if (minutes == 0) {
                return xi18n("Opened less than minute ago");
            }
            return xi18np("Opened 1 minute ago", "Opened %1 minutes ago", minutes);
        }
        return xi18np("Opened 1 hour ago", "Opened %1 hours ago", hours);
    }
    if (days < 30) {
        return xi18np("Opened yesterday", "Opened %1 days ago", days);
    }
    if (days < 365) {
        return xi18np("Opened over a month ago", "Opened %1 months ago", days / 30);
    }
    return xi18np("Opened one year ago", "Opened %1 years ago", days / 365);
}

QVariant KexiRecentProjectsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    KexiProjectData *pdata = static_cast<KexiProjectData*>(index.internalPointer());
    bool fileBased = !pdata->connectionData()->databaseName().isEmpty();
    switch (role) {
    case Qt::DisplayRole: {
        //! @todo add support for imported entries, e.g. MS Access
        QStringList result;
        if (fileBased) {
            const QString caption = pdata->caption().trimmed();
            if (!caption.isEmpty()) {
                result << caption;
            }
            // Filename is a good replacement for caption but append it always since
            // sometimes captions can be misleading and not in par with that the file really
            // contains. Moreover it's currently it is not possible to edit the caption afterwards
            // for the users.
            result << QFileInfo(pdata->connectionData()->databaseName()).fileName();
        }
        else {
            const QString name = pdata->captionOrName();
            if (!name.isEmpty()) {
                result << name;
            }
            QString serverInfo = pdata->connectionData()->toUserVisibleString(
                KDbConnectionData::UserVisibleStringOption::None);
            // friendly message:
            if (serverInfo == "localhost") {
                result << xi18n("on local server");
            }
            else {
                result << xi18nc("@info", "on <resource>%1</resource> server", serverInfo);
            }
        }
        const QString opened(openedString(pdata->lastOpened()));
        if (!opened.isEmpty()) {
            result << opened;
        }
        return result.join('\n');
    }
    case Qt::ToolTipRole: {
        //! @todo add support for imported entries, e.g. MS Access
        QStringList result;
        if (fileBased) {
            result << xi18nc("@info File database <file>", "File database <filename>%1</filename>",
                             pdata->connectionData()->databaseName());
        } else {
            KDbDriverManager manager;
            const KDbDriverMetaData *driverMetaData = manager.driverMetaData(pdata->connectionData()->driverId());
            if (!driverMetaData) {
                result << xi18n("database");
            }
            result << xi18nc("<type> database, e.g. PostgreSQL database, MySQL database", "%1 database",
                          driverMetaData->name());
        }
        const QDateTime opened(pdata->lastOpened());
        if (!opened.isNull()) { // for precision
            result << xi18n("Last opened on %1", QLocale().toString(opened));
        }
        return QStringLiteral("<p>%1</p>").arg(result.join(QStringLiteral("</p><p>")));
    }
    case Qt::DecorationRole: {
        //! @todo show icon specific to given database or mimetype
        if (fileBased) {
            return Kexi::defaultFileBasedDriverIcon();
        }
        else {
            return Kexi::serverIcon();
        }
    }
    /*case KCategorizedSortFilterProxyModel::CategorySortRole: {
        int index = m_categoryNameIndex.value(info->category);
        if (index >= 0 && index < m_templateCategories.count()) {
            QVariantList list;
            list << index << info->caption;
            return list;
        }
        return QVariantList();
    }
    case KCategorizedSortFilterProxyModel::CategoryDisplayRole: {
        int index = m_categoryNameIndex.value(info->category);
        if (index >= 0 && index < m_templateCategories.count()) {
            KexiTemplateCategoryInfo category = m_templateCategories.value(index);
            return category.caption;
        }
        return QVariant();
    }*/
    case NameRole:
        return pdata->databaseName();
    /*case CategoryRole:
        return info->category;*/
    default:
        break;
    }
    return QVariant();
}

Qt::ItemFlags KexiRecentProjectsModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags f;
    if (index.isValid()) {
        f |= (Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    }
    return f;
}

// ----

KexiRecentProjectsProxyModel::KexiRecentProjectsProxyModel(QObject *parent)
 : KCategorizedSortFilterProxyModel(parent)
{
}

bool KexiRecentProjectsProxyModel::subSortLessThan(
    const QModelIndex& left, const QModelIndex& right) const
{
    KexiProjectData *pdataLeft = static_cast<KexiProjectData*>(left.internalPointer());
    KexiProjectData *pdataRight = static_cast<KexiProjectData*>(right.internalPointer());
    //qDebug() << *pdataLeft << *pdataRight;
    return pdataLeft->lastOpened() < pdataRight->lastOpened();
}

