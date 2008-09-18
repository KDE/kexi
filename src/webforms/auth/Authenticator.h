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

#ifndef KEXIWEBFORMS_AUTH_AUTHENTICATOR_H
#define KEXIWEBFORMS_AUTH_AUTHENTICATOR_H

#include <string>

#include <QList>

#include <pion/net/PionUser.hpp>
#include <pion/net/HTTPAuth.hpp>

#include "User.h"

namespace KexiWebForms {
namespace Auth {

/**
 * @brief Authenticator
 *
 * This service is responsible to show a the welcome page and a list of
 * tables and queries in the database
 */
class Authenticator {
public:
    static void init(pion::net::HTTPAuthPtr);
    static Authenticator* getInstance();

    virtual ~Authenticator() {}

    /**
     * This method tries to load the user database, if nothing is found
     * it creates a new table and fills it with two user accounts:
     * - root: with password "root" and has access to all functionalities
     * - anonymous: with password "guest" and has no access
     */
    bool loadStore();

    /**
     * Authenticate the user based on user name and password
     * @param char* username
     * @param char* password
     */
    User authenticate(const char*, const char*);

    /**
     * Authenticate the user based on user name and password
     * @param std::string& username
     * @param std::string& password
     */
    User authenticate(const std::string&, const std::string&);

    /**
     * Authenticate the user based on PionUserPtr structure
     * @param pion::net::PionUserPtr a boost::shared_ptr<PionUser> object
     * @todo: Decouple this class from the pion library
     */
    User authenticate(pion::net::PionUserPtr);

protected:
    /** ctor */
    Authenticator(pion::net::HTTPAuthPtr auth) : m_auth(auth) {}

private:
    static Authenticator* m_instance;
    pion::net::HTTPAuthPtr m_auth;
    QList<User> m_users;
};

} // end namespace Auth
} // end namespace KexiWebForms

#endif /* KEXIWEBFORMS_AUTH_AUTHENTICATOR_H */
