/* This file is part of the KDE project
   Copyright (C) 2003-2004 Jarosław Staniek <staniek@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef PARSER_TEST_H
#define PARSER_TEST_H

int parserTest(const QString &st, const QStringList &params)
{
    int r = 0;

    if (!conn->useDatabase(db_name)) {
        conn->debugError();
        return 1;
    }

    KexiDB::Parser parser(conn);

    const bool ok = parser.parse(st);
    KexiDB::QuerySchema *q = parser.query();
    QList<QVariant> variantParams;
    foreach(const QString param, params)
    variantParams.append(param.toLocal8Bit());
    if (ok && q) {
        cout << q->debugString().toLatin1().constData() << '\n';
        cout << "-STATEMENT:\n" << conn->selectStatement(*q, variantParams).toLatin1().data() << '\n';
    } else {
        KexiDB::ParserError err = parser.error();
        kDebug() << QString("Error = %1\ntype = %2\nat = %3").arg(err.error())
        .arg(err.type()).arg(err.at());
        r = 1;
    }
    delete q;
    q = 0;


    if (!conn->closeDatabase()) {
        conn->debugError();
        return 1;
    }

    return r;
}

#endif

