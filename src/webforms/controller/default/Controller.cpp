/*
 * This file is part of the KDE project
 *
 * (C) Copyright 2008 by Lorenzo Villani <lvillani@binaryhelix.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */


#include <QUrl>
#include <QFile>
#include <QString>
#include <QByteArray>
#include <QStringList>

#include <KDebug>
#include <KCmdLineArgs>

#include <google/template.h>

#include <pion/net/PionUser.hpp>
#include <pion/net/HTTPTypes.hpp>
#include <pion/net/WebService.hpp>
#include <pion/net/HTTPResponseWriter.hpp>

#include "../../view/default/Index.h"
#include "../../view/default/Create.h"
#include "../../view/default/Read.h"
#include "../../view/default/Update.h"
#include "../../view/default/Delete.h"
#include "../../view/default/Query.h"

#include "../../auth/User.h"
#include "../../auth/Permission.h"
#include "../../auth/Authenticator.h"

#include "Controller.h"

namespace KexiWebForms {

Controller::Controller() {
    m_index = new View::Index("index.tpl");
    m_create = new View::Create("create.tpl");
    m_read = new View::Read("read.tpl");
    m_update = new View::Update("update.tpl");
    m_delete = new View::Delete("delete.tpl");
    m_query = new View::Query("query.tpl");

    // Template directory
    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
    // Set template root directory equal to root directory
    google::Template::SetTemplateRootDirectory(QFile::encodeName(args->getOption("webroot")).constData());
}

Controller::~Controller() {
    delete m_index;
    delete m_create;
    delete m_read;
    delete m_update;
    delete m_delete;
    delete m_query;
}

void Controller::operator()(pion::net::HTTPRequestPtr& request, pion::net::TCPConnectionPtr& tcp_conn) {
    pion::net::HTTPResponseWriterPtr writer(pion::net::HTTPResponseWriter::create(tcp_conn, *request,
                                            boost::bind(&pion::net::TCPConnection::finish, tcp_conn)));

    // Authentication data
    pion::net::PionUserPtr userPtr;
    Auth::User u;
    if (request->getUser()) {
        userPtr = pion::net::PionUserPtr(request->getUser());
        u = Auth::Authenticator::getInstance()->authenticate(userPtr);
    }

    // Request URI handling & dispatch
    QStringList requestURI(QString(request->getOriginalResource().c_str()).split('/'));
    requestURI.removeFirst();

    QString action(requestURI.at(0));
    requestURI.removeFirst();

    QHash<QString, QString> data;

    // Convert all the stuff from hash_multimap and put it in data
    typedef pion::net::HTTPTypes::QueryParams::const_iterator SDIterator;
    pion::net::HTTPTypes::QueryParams params(request->getQueryParams());

    /*
     * The request hash is filled before we fill the needed value for
     * dispatched requests, this ensures that no forms values overwrites
     * our needed data but it's not good, in fact you can't use
     * kwebforms__table kwebforms__pkey kwebforms__pkeyValue kwebforms__query
     * as field names
     */
    for (SDIterator it = params.begin(); it != params.end(); ++it) {
        data[QUrl::fromPercentEncoding(QByteArray(it->first.c_str()))] =
            QUrl::fromPercentEncoding(QByteArray(it->second.c_str()));
    }

    kDebug() << "ACTION :" << action;
    kDebug() << "PARAMETERS COUNT: " << requestURI.count();

    bool malformedRequest = true;
    if (action == "") {
        if (requestURI.count() == 0) {
            malformedRequest = false;
            m_index->view(data, writer);
        }
    } else if (action == "create") {
        if ((requestURI.count() == 1) && u.can(Auth::CREATE)) {
            data["kwebforms__table"] = requestURI.at(0);
            m_create->view(data, writer);
            malformedRequest = false;
        }
    } else if (action == "read") {
        if ((requestURI.count() == 1) && u.can(Auth::READ)) {
            data["kwebforms__table"] = requestURI.at(0);
            m_read->view(data, writer);
            malformedRequest = false;
        }
    } else if (action == "update") {
        if ((requestURI.count() == 3) && u.can(Auth::UPDATE)) {
            data["kwebforms__table"] = requestURI.at(0);
            data["kwebforms__pkey"] = requestURI.at(1);
            data["kwebforms__pkeyValue"] = requestURI.at(2);
            m_update->view(data, writer);
            malformedRequest = false;
        }
    } else if (action == "delete") {
        if ((requestURI.count() == 3) && u.can(Auth::DELETE)) {
            data["kwebforms__table"] = requestURI.at(0);
            data["kwebforms__pkey"] = requestURI.at(1);
            data["kwebforms__pkeyValue"] = requestURI.at(2);
            m_delete->view(data, writer);
            malformedRequest = false;
        }
    } else if (action == "query") {
        if ((requestURI.count() == 1) /*&& u.can(Auth::QUERY)*/) {
            data["kwebforms__query"] = requestURI.at(0);
            m_query->view(data, writer);
            malformedRequest = false;
        }
    }

    if (malformedRequest)
        writer->writeNoCopy("<h1>Malformed Request</h1>");

    writer->writeNoCopy(pion::net::HTTPTypes::STRING_CRLF);
    writer->writeNoCopy(pion::net::HTTPTypes::STRING_CRLF);
    writer->send();
}

}
