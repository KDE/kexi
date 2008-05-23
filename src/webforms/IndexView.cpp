/* This file is part of the KDE project

   (C) Copyright 2008 by Lorenzo Villani <lvillani@binaryhelix.net>
   Time-stamp: <2008-05-23 19:14:12 lorenzo>

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

#include <string>
#include <sstream>
#include <google/template.h>

#include "Server.h"
#include "IndexView.h"
#include "DataProvider.h"
#include "HTTPStream.h"

namespace KexiWebForms {

    namespace IndexView {

        void show(Request* req) {
            HTTPStream stream(req);

            google::TemplateDictionary dict("index");
            dict.SetValue("TITLE", gConnection->data()->fileName().toLatin1().constData());

            std::ostringstream tables;
            // FIXME: Beware of temporary objects!
            for (int i = 0; i < gConnection->tableNames().size(); ++i)
                tables << "<li><a href=\"/table/" << gConnection->tableNames().at(i).toLatin1().constData()
                       << "\">" << gConnection->tableNames().at(i).toLatin1().constData() << "</a></li>";
            dict.SetValue("TABLES", tables.str());

            // FIXME: That's horrible
            std::ostringstream file;
            file << Server::instance()->config()->webRoot.toLatin1().constData() << "/index.tpl";

            google::Template* tpl = google::Template::GetTemplate(file.str(), google::DO_NOT_STRIP);
            std::string output;
            tpl->Expand(&output, &dict);

            stream << output << webend;
        }

    }

}
