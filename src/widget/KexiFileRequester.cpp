/* This file is part of the KDE project
   Copyright (C) 2016 Jarosław Staniek <staniek@kde.org>

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

#include "KexiFileRequester.h"
#include <KexiFileFilters.h>
#include <config-kexi.h>

#include <KFileWidget>
#include <KUrlRequester>
#include <QDebug>
#include <QStandardPaths>
#include <QVBoxLayout>

//! @internal
class Q_DECL_HIDDEN KexiFileRequester::Private : public QObject
{
    Q_OBJECT
public:
    Private()
    {
    }
    ~Private()
    {
    }
    void updateFilter()
    {
        requester->setFilter(filters.toString(KexiFileFilters::KUrlRequesterFormat));
    }

public Q_SLOTS:
    void updateUrl(const QUrl &url)
    {
        QString path = url.toDisplayString(QUrl::RemoveScheme | QUrl::PreferLocalFile);
#ifdef Q_OS_WIN
        if (path.startsWith('/')) {
            path = path.mid(1);
        }
#endif
        path = QDir::toNativeSeparators(path);
    //        qDebug() << url << url.isLocalFile() << url.toDisplayString() << QDir::toNativeSeparators(url.path())
    //                 << url.toDisplayString(QUrl::RemoveScheme | QUrl::PreferLocalFile)
    //                 << QDir::toNativeSeparators(url.toString(QUrl::RemoveScheme | QUrl::PreferLocalFile));
//        qDebug() <<
//                    QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
//                                              QLatin1String("kservices5"), QStandardPaths::LocateDirectory);
//        qDebug() <<
//                    QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QString(),
//                                              QStandardPaths::LocateDirectory);
        requester->setStartDir(url);
#ifdef Q_OS_WIN
        requester->fileDialog()->setDirectoryUrl(url);
#endif
        requester->setText(path);
    }

public:
    KUrlRequester *requester;
    KexiFileFilters filters;
};

KexiFileRequester::KexiFileRequester(const QUrl &url, QWidget *parent)
    : QWidget(parent), d(new Private)
{
    QVBoxLayout *lyr = new QVBoxLayout(this);
    d->requester = new KUrlRequester(url, this);
    d->updateUrl(url);
    //qDebug() << d->requester->url() << d->requester->startDir();
    connect(d->requester, &KUrlRequester::urlSelected, d, &KexiFileRequester::Private::updateUrl);
    lyr->addWidget(d->requester);
}

KexiFileRequester::~KexiFileRequester()
{
    delete d;
}

QUrl KexiFileRequester::url() const
{
    return d->requester->url();
}

void KexiFileRequester::setUrl(const QUrl &url)
{
    d->requester->setUrl(url);
    d->updateUrl(url);
}

void KexiFileRequester::setFileMode(KexiFileFilters::Mode mode)
{
    KFile::Modes fileModes = KFile::File | KFile::LocalOnly;
    if (mode == KexiFileFilters::Opening || mode == KexiFileFilters::CustomOpening) {
        fileModes |= KFile::ExistingOnly;
    }
    d->filters.setMode(mode);
    d->requester->setMode(fileModes);
    d->updateFilter();
}

QStringList KexiFileRequester::additionalMimeTypes() const
{
    return d->filters.additionalMimeTypes();
}

QStringList KexiFileRequester::excludedMimeTypes() const
{
    return d->filters.excludedMimeTypes();
}

QString KexiFileRequester::defaultFilter() const
{
    return d->filters.defaultFilter();
}

void KexiFileRequester::setExcludedMimeTypes(const QStringList &mimeTypes)
{
    d->filters.setExcludedMimeTypes(mimeTypes);
    d->updateFilter();
}

void KexiFileRequester::setAdditionalMimeTypes(const QStringList &mimeTypes)
{
    d->filters.setAdditionalMimeTypes(mimeTypes);
    d->updateFilter();
}

void KexiFileRequester::setDefaultFilter(const QString &filter)
{
    d->filters.setDefaultFilter(filter);
}

#include "KexiFileRequester.moc"
