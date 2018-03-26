/***************************************************************************
 * kexidbparser.cpp
 * This file is part of the KDE project
 * copyright (C)2004-2006 by Sebastian Sauer (mail@dipe.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 ***************************************************************************/


#include "kexidbparser.h"
#include "kexidbschema.h"
#include "kexidbconnection.h"

using namespace Scripting;

KexiDBParser::KexiDBParser(KexiDBConnection* connection, KDbParser* parser, bool owner)
        : QObject(connection)
        , m_connection(connection)
        , m_parser(parser)
        , m_owner(owner)
{
    setObjectName("KexiDBParser");
}

KexiDBParser::~KexiDBParser()
{
    if (m_owner)
        delete m_parser;
}

bool KexiDBParser::parse(const QString& sql)
{
    return m_parser->parse(KDbEscapedString(sql));
}
void KexiDBParser::clear()
{
    m_parser->reset();
}
QString KexiDBParser::operation() const
{
    return m_parser->statementTypeString();
}

QObject* KexiDBParser::table()
{
    KDbTableSchema* t = m_parser->table();
    return t ? new KexiDBTableSchema(this, t, false) : 0;
}

QObject* KexiDBParser::query()
{
    KDbQuerySchema* q = m_parser->query();
    return q ? new KexiDBQuerySchema(this, q, false) : 0;
}

QObject* KexiDBParser::connection()
{
    return m_connection;
}
QString KexiDBParser::statement() const
{
    return m_parser->statement().toString();
}

QString KexiDBParser::errorType() const
{
    return m_parser->error().type();
}
QString KexiDBParser::errorMsg() const
{
    return m_parser->error().message();
}
int KexiDBParser::errorAt() const
{
    return m_parser->error().position();
}

