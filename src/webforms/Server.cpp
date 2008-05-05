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

#include <QDir>

#include <KDebug>
#include <KUniqueApplication>

#include <shttpd.h>

namespace KexiWebForms {

    Server::Server(ServerConfig* serverConfig) {
        ctx = NULL;
        kDebug() << "Initializing HTTP server...";
        ctx = shttpd_init();
        if (ctx == NULL) {
            kError() << "HTTP Server not correctly initialized, aborting";
            exit(1);
        }

        if (config == NULL) {
            kError() << "Internal error, can't retrieve configuration data!";
            exit(1);
        } else {
            config = serverConfig;
        }
    }

    Server::~Server() {
        if (ctx != NULL)
            shttpd_fini(ctx);
    }

    bool Server::run() {
        if (ctx != NULL) {
            kDebug() << "Setting to listen on port " << config->ports;
            shttpd_set_option(ctx, "ports", config->ports.toStdString().c_str());

            if (QDir(config->webRoot).exists()) {
                kDebug() << "Webroot is " << config->webRoot;
                shttpd_set_option(ctx, "root", config->webRoot.toStdString().c_str());
            } else {
                kError() << "Webroot does not exist! Aborting";
                exit(1);
            }

            // SSL certificate
            if (config->https != NULL) {
                if (config->certPath != NULL) {
                    if (QFile(config->certPath).exists()) {
                        shttpd_set_option(ctx, "ssl_cert", config->certPath.toStdString().c_str());
                    } else {
                        kError() << "Certificate file does not exist! Aborting";
                        exit(1);
                    }
                }
            }
            
            // Do not show directory listings by default
            if (config->dirList) {
                kDebug() << "Enabling directory listing...";
                shttpd_set_option(ctx, "dir_list", "1");
            } else {
                shttpd_set_option(ctx, "dir_list", "0");
            }
            
            for (;;) {
                shttpd_poll(ctx, 1000);
                KUniqueApplication::processEvents();
            }
            
            return true;
        } else if (ctx == NULL) {
            kError() << "Internal error, SHTTPD engine was not initialized correctly";
            return false;
        } else { 
            kError() << "Unknown error";
            return false;
        }
    }

    void Server::registerHandler(const char* handler, void(*f)(shttpd_arg*)) {
        if (f != NULL) {
            kDebug() << "Registering handler for: " << handler;
            shttpd_register_uri(ctx, handler, f, NULL);
        }
    }

    ServerConfig* Server::getConfig() {
        if (config != NULL)
            return config;
        else {
            kError() << "Configuration data can't be loaded";
            kError() << "Internal error, aborting";
            exit(1);
        }
    }
}
