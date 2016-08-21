/* This file is part of the KDE project
   Copyright (C) 2004 Martin Ellis <m.a.ellis@ncl.ac.uk>
   Copyright (C) 2006-2016 Jarosław Staniek <staniek@kde.org>

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

#include "mysqlmigrate.h"
#include <kexi.h>

/* This is the implementation for the MySQL specific import routines. */
KEXI_PLUGIN_FACTORY(MysqlMigrate, "keximigrate_mysql.json")

MysqlMigrate::MysqlMigrate(QObject *parent, const QVariantList& args)
    : KexiSqlMigrate(QLatin1String("org.kde.kdb.mysql"), parent, args)
{
}

MysqlMigrate::~MysqlMigrate()
{
}

#include "mysqlmigrate.moc"
