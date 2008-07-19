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


#include <kexidb/cursor.h>
#include <kexidb/connection.h>
#include <kexidb/tableschema.h>
#include <kexidb/queryschema.h>
#include <kexidb/roweditbuffer.h>

#include <pion/net/PionUser.hpp>
#include <pion/net/HTTPAuth.hpp>

#include "Permission.h"
#include "DataProvider.h"

#include "Authenticator.h"

namespace KexiWebForms {
namespace Auth {
    Authenticator* Authenticator::m_instance = 0;
    
    void Authenticator::init(pion::net::HTTPAuthPtr p) {
        if (!m_instance)
            m_instance = new Authenticator(p);
        m_instance->loadStore();
    }

    Authenticator* Authenticator::getInstance() {
        if (m_instance)
            return m_instance;
        else // ouch!
            return NULL;
    }

    // fictional loadStore, returns a fixed list of users
    bool Authenticator::loadStore() {
        KexiDB::TableSchema* table = gConnection->tableSchema("kexi__users");
        
        if (!table) {
            // the table doesn't exist, create it
            KexiDB::TableSchema* kexi__users = new KexiDB::TableSchema("kexi__users");
            KexiDB::Field* id = new KexiDB::Field("id", KexiDB::Field::Integer);
            id->setAutoIncrement(true);
            kexi__users->insertField(0, id);
            KexiDB::Field* name = new KexiDB::Field("name", KexiDB::Field::Text);
            kexi__users->insertField(1, name);
            KexiDB::Field* password = new KexiDB::Field("password", KexiDB::Field::Text);
            kexi__users->insertField(2, password);
            KexiDB::Field* create = new KexiDB::Field("p_create", KexiDB::Field::Boolean);
            kexi__users->insertField(3, create);
            KexiDB::Field* read = new KexiDB::Field("p_read", KexiDB::Field::Boolean);
            kexi__users->insertField(4, read);
            KexiDB::Field* update = new KexiDB::Field("p_update", KexiDB::Field::Boolean);
            kexi__users->insertField(5, update);
            KexiDB::Field* fdelete = new KexiDB::Field("p_delete", KexiDB::Field::Boolean);
            kexi__users->insertField(6, fdelete);
            KexiDB::Field* fquery = new KexiDB::Field("p_query", KexiDB::Field::Boolean);
            kexi__users->insertField(7, fquery);

            if (!gConnection->createTable(kexi__users)) {
                // Table was not created, fatal error
                kError() << "Failed to create system table kexi__users" << endl;
                kError() << "Error string: " << gConnection->errorMsg() << endl;
                /*delete name;
                delete password;
                delete create;
                delete read;
                delete update;
                delete fdelete;
                delete fquery;*/
                delete kexi__users;
                return false;
            } else {
                // Table was created, create two standard accounts
                KexiDB::QuerySchema query(*kexi__users);
                KexiDB::Cursor* cursor = gConnection->prepareQuery(query);
                KexiDB::RecordData recordData(kexi__users->fieldCount());
                KexiDB::RowEditBuffer editBuffer(true);
                // root
                QVariant vtrue(true);
                QVariant vfalse(false);
                kDebug() << "Creating user root with password root" << endl;
                QVariant user_root("root");
                QVariant password_root("root");
                editBuffer.insert(*query.columnInfo(name->name()), user_root);
                editBuffer.insert(*query.columnInfo(password->name()), password_root);
                editBuffer.insert(*query.columnInfo(create->name()), vtrue);
                editBuffer.insert(*query.columnInfo(read->name()), vtrue);
                editBuffer.insert(*query.columnInfo(update->name()), vtrue);
                editBuffer.insert(*query.columnInfo(fdelete->name()), vtrue);
                editBuffer.insert(*query.columnInfo(fquery->name()), vtrue);
                kDebug() << "Registering user within database" << endl;
                if (cursor->insertRow(recordData, editBuffer)) {
                    kDebug() << "Succeeded" << endl;
                    User* u = new User("root", "root");
                    m_users.append(*u);
                    m_auth->addUser(u->name().toUtf8().constData(), u->password().toUtf8().constData());
                } else {
                    kError() << "An error occurred" << endl;
                    return false;
                }

                // anonymous
                kDebug() << "Creating user anonymous with password guest" << endl;
                QVariant user_anonymous("anonymous");
                QVariant password_anonymous("guest");
                editBuffer.insert(*query.columnInfo(name->name()), user_anonymous);
                editBuffer.insert(*query.columnInfo(password->name()), password_anonymous);
                editBuffer.insert(*query.columnInfo(create->name()), vfalse);
                editBuffer.insert(*query.columnInfo(read->name()), vfalse);
                editBuffer.insert(*query.columnInfo(update->name()), vfalse);
                editBuffer.insert(*query.columnInfo(fdelete->name()), vfalse);
                editBuffer.insert(*query.columnInfo(fquery->name()), vfalse);
                if (cursor->insertRow(recordData, editBuffer)) {
                    kDebug() << "Succeeded" << endl;
                    User* u = new User("anonymous", "guest");
                    m_users.append(*u);
                    m_auth->addUser(u->name().toUtf8().constData(), u->password().toUtf8().constData());
                } else {
                    kError() << "An error occurred" << endl;
                    return false;
                }
                
            }
        } else {
            // load stuff from the store, create appropriated User objects, store them within
            // Authenticator
        }

        return true;
        
        /*User* u = new User("anonymous", "guest");
        m_users.append(*u);
        m_auth->addUser(u->name().toUtf8().constData(), u->password().toUtf8().constData());
        
        u = new User("root", "root");
        u->addPermission(CREATE);
        u->addPermission(READ);
        u->addPermission(UPDATE);
        u->addPermission(DELETE);
        m_users.append(*u);
        m_auth->addUser(u->name().toUtf8().constData(), u->password().toUtf8().constData());

        return true;*/
    }
    
    User Authenticator::authenticate(const char* name, const char* password) {
        for (int i = 0; i < m_users.size(); ++i) {
            User u = m_users.at(i);
            if ((u.name() == name) && (u.password() == password)) {
                return u;
            }
        }
        return User("anonymous", "guest");
    }

    User Authenticator::authenticate(const std::string& name, const std::string& password) {
        return authenticate(name.c_str(), password.c_str());
    }

    User Authenticator::authenticate(pion::net::PionUserPtr p) {
        return authenticate(p->getUsername(), p->getPassword());
    }

}
}
