/* This file is part of the KDE project
   Copyright (C) 2012-2017 Jarosław Staniek <staniek@kde.org>

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

#include "KexiTestHandler.h"
#include <QCoreApplication>

class KexiTestHandler::Private
{
public:
    Private() {}
    QList<QCommandLineOption> extraOptions;
};

KexiTestHandler::KexiTestHandler()
 : d(new Private)
{
}

KexiTestHandler::~KexiTestHandler()
{
    delete d;
}

QList<QCommandLineOption> KexiTestHandler::extraOptions() const
{
    return d->extraOptions;
}

void KexiTestHandler::addExtraOption(const QCommandLineOption &option)
{
    d->extraOptions.append(option);
}

void KexiTestHandler::removeOwnOptions(QStringList *args)
{
    for(const QCommandLineOption &extraOption : d->extraOptions) {
        for(const QString &name : extraOption.names()) {
            args->removeOne("--" + name);
        }
    }
}
