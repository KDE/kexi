/* This file is part of the KDE project

   (C) Copyright 2008 by Lorenzo Villani <lvillani@binaryhelix.net>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <KDebug>

#include "User.h"

namespace KexiWebForms {
namespace Auth {

void User::addPermission(Permission p) {
    m_perms.append(p);
}

QString User::name() const {
    return m_name;
}

QString User::password() const {
    return m_password;
}

QList<Permission> User::permissions() const {
    return m_perms;
}

bool User::can(Permission p) {
    for (int i = 0; i < m_perms.size(); ++i) {
        if (m_perms.at(i) == p) {
            kDebug() << "User " << name() << " authorized";
            return true;
        }
    }
    kDebug() << "User " << name() << " NOT authorized";
    return false;
}

}
}
