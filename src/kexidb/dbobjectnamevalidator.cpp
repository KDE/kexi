/* This file is part of the KDE project
   Copyright (C) 2004-2005 Jarosław Staniek <staniek@kde.org>

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

#include "dbobjectnamevalidator.h"

#include "driver.h"

using namespace KexiDB;
using namespace KexiUtils;

ObjectNameValidator::ObjectNameValidator(
    KexiDB::Driver *drv, QObject * parent)
        : Validator(parent)
        , m_drv(drv)
{
}

ObjectNameValidator::~ObjectNameValidator()
{
}

Validator::Result ObjectNameValidator::internalCheck(
    const QString & /*valueName*/, const QVariant& v,
    QString &message, QString &details)
{

    if (m_drv.isNull() ? !KexiDB::Driver::isKexiDBSystemObjectName(v.toString())
            : !m_drv->isSystemObjectName(v.toString()))
        return Validator::Ok;
    message = i18n("You cannot use name \"%1\" for your object.\n"
                   "It is reserved for internal Kexi objects. Please choose another name.",
                   v.toString());
    details = i18n("Names of internal Kexi objects are starting with \"kexi__\".");
    return Validator::Error;
}
