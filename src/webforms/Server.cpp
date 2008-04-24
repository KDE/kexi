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

#include "Server.h"
#include "ServerConfig.h"

#include <KDebug>

#include <shttpd.h>

namespace KexiWebForms {
    Server::~Server() {
        if (ctx != NULL)
            shttpd_fini(ctx);
    }

    void Server::run(ServerConfig& serverConfig) {
        kDebug() << "Initializing server...";

        ctx = shttpd_init();
        shttpd_set_option(ctx, "ports", serverConfig.ports.toStdString().c_str());
        shttpd_set_option(ctx, "root", serverConfig.webRoot.toStdString().c_str());

        // Do not show directory listings by default
        shttpd_set_option(ctx, "dir_list", "0");

        kDebug() << "Listening...";
        
        for (;;)
            shttpd_poll(ctx, 1000);
    }
}
