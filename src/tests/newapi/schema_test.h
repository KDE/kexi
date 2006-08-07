/* This file is part of the KDE project
   Copyright (C) 2003-2004 Jaroslaw Staniek <js@iidea.pl>

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

#ifndef SCHEMA_TEST_H
#define SCHEMA_TEST_H

int schemaTest()
{
	if (!conn->useDatabase( db_name )) {
		kdDebug() << conn->errorMsg() << endl;
		return 1;
	}

	KexiDB::TableSchema *t = conn->tableSchema( "persons" );
	if (t)
		t->debug();
	else
		kdDebug() << "!persons" << endl;
	t = conn->tableSchema( "cars" );
	if (t)
		t->debug();
	else
		kdDebug() << "!cars" << endl;
/*
// some tests	
	{
		KexiDB::Field::ListIterator iter = t->fieldsIterator();
		KexiDB::Field::List *lst = t->fields();
		lst->clear();
		for (;iter.current();++iter) {
			kdDebug() << "FIELD=" << iter.current()->name() << endl;
//			iter.current()->setName("   ");
		}
	}*/
	return 0;
}

#endif

