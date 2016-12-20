/* This file is part of the KDE project
   Copyright (C) 2004 Adam Pigg <adam@piggz.co.uk>
   Copyright (C) 2004 Jarosław Staniek <staniek@kde.org>
   Copyright (C) 2005 Martin Ellis <kde@martinellis.co.uk>

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

#include "keximigratedata.h"

using namespace KexiMigration;

class Q_DECL_HIDDEN Data::Private
{
public:
    Private() {}
    KexiProjectData *destinationProjectData = nullptr;
};

Data::Data()
    : source(nullptr)
    , d(new Private)
{
}

Data::~Data()
{
    delete d;
}

KexiProjectData* Data::destinationProjectData()
{
    return d->destinationProjectData;
}

const KexiProjectData* Data::destinationProjectData() const
{
    return d->destinationProjectData;
}

void Data::setDestinationProjectData(KexiProjectData* destinationProjectData)
{
    if (d->destinationProjectData && d->destinationProjectData != destinationProjectData) {
        delete d->destinationProjectData;
    }
    d->destinationProjectData = destinationProjectData;
}

QString Data::sourceDatabaseInfoString() const
{
    return source ? KexiProjectData::infoString(sourceName, *source).toString(Kuit::PlainText)
                  : QString();
}
