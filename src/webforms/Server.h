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

#ifndef KEXI_WEBFORMS_SERVER_H
#define KEXI_WEBFORMS_SERVER_H

#include "Request.h"
#include "ServerConfig.h"

struct shttpd_ctx;

namespace KexiWebForms {

    class Server {
    public:
        ~Server();
        
        static Server* instance();

        bool init(ServerConfig*);
        bool run();

        // FIXME: Do not expose shttpd data structures
        void registerHandler(const char*, void(*f)(RequestData*));
        ServerConfig* config() const;
    protected:
        Server();
    private:
        static Server* m_instance;
        bool m_initialized;
        ServerConfig* m_config;
        shttpd_ctx* m_ctx;
    };

}

#endif
