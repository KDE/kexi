/* This file is part of the KDE project
   Copyright (C) 2009 Sharan Rao <sharanrao@gmail.com>

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

#ifndef ODBC_SQLQUERY_UNIT_H
#define ODBC_SQLQUERY_UNIT_H

#include "odbcqueryunit.h"

namespace KexiDB
{

class ODBCSQLQueryUnit : public ODBCQueryUnit
{
 Q_OBJECT

 public:
    ODBCSQLQueryUnit(QObject* parent);

    virtual SQLRETURN execute();

};

}

#endif
