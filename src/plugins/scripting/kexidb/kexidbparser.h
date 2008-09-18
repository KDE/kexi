/***************************************************************************
 * kexidbparser.h
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

#ifndef SCRIPTING_KEXIDBPARSER_H
#define SCRIPTING_KEXIDBPARSER_H

#include <qstring.h>
#include <qobject.h>
#include <qpointer.h>

#include <kexidb/drivermanager.h>
#include <kexidb/parser/parser.h>

namespace Scripting
{

// Forward declaration.
class KexiDBConnection;
class KexiDBTableSchema;
class KexiDBQuerySchema;

/**
* The KexiDBParser could be used to parse SQL-statements.
*
* Example (in Python) ;
* @code
* # First we need a parser object.
* parser = connection.parser()
* # Parse a SQL-statement.
* parser.parse("SELECT * from table1")
* # The operation could be e.g. SELECT or INSERT.
* if parser.operation() == 'Error':
*     raise parser.errorMsg()
* # Print some feedback.
* print "Successfully parsed the SQL-statement %s" % parser.statement()
* @endcode
*/
class KexiDBParser : public QObject
{
    Q_OBJECT
public:
    KexiDBParser(KexiDBConnection* connection, ::KexiDB::Parser* parser, bool owner);
    virtual ~KexiDBParser();

public slots:

    /** Clears previous results and runs the parser on the SQL statement passed as an argument. */
    bool parse(const QString& sql);
    /** Clears parsing results. */
    void clear();
    /** Returns the resulting operation. */
    const QString operation();

    /** Returns the \a KexiDBTableSchema object on a CREATE TABLE operation. */
    QObject* table();
    /** Returns the \a KexiDBQuerySchema object on a SELECT operation. */
    QObject* query();
    /** Returns the \a KexiDBConnection object pointing to the used database connection. */
    QObject* connection();
    /** Returns the SQL query statement. */
    const QString statement();

    /** Returns the type string of the last error. */
    const QString errorType();
    /** Returns the message of the last error. */
    const QString errorMsg();
    /** Returns the position where the last error occurred. */
    int errorAt();

private:
    QPointer<KexiDBConnection> m_connection;
    ::KexiDB::Parser* m_parser;
    bool m_owner;
};

}

#endif

