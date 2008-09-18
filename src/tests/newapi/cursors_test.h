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

#ifndef CURSORS_TEST_H
#define CURSORS_TEST_H

int tablesTest();

int cursorsTest()
{
    if (!conn->databaseExists(db_name)) {
        if (tablesTest() != 0)
            return 1;
        kDebug() << "DB created & filled";
    }

    if (!conn->useDatabase(db_name)) {
        conn->debugError();
        return 1;
    }

    KexiDB::Cursor *cursor = conn->executeQuery("select * from persons", cursor_options);  //KexiDB::Cursor::Buffered );
    kDebug() << "executeQuery() = " << !!cursor;
    if (!cursor)
        return 1;

    kDebug() << "Cursor::moveLast() ---------------------";
    kDebug() << "-- Cursor::moveLast() == " << cursor->moveLast();
    cursor->moveLast();
    kDebug() << "Cursor::moveFirst() ---------------------";
    kDebug() << "-- Cursor::moveFirst() == " << cursor->moveFirst();

    /*  kDebug()<<"Cursor::moveNext() == "<<cursor->moveNext();
      kDebug()<<"Cursor::moveNext() == "<<cursor->moveNext();
      kDebug()<<"Cursor::moveNext() == "<<cursor->moveNext();
      kDebug()<<"Cursor::moveNext() == "<<cursor->moveNext();
      kDebug()<<"Cursor::eof() == "<<cursor->eof();*/
    conn->deleteCursor(cursor);

    return 0;
}

#endif

