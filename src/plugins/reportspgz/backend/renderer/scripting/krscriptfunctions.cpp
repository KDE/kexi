/*
 * Kexi Report Plugin
 * Copyright (C) 2007-2008 by Adam Pigg (adam@piggz.co.uk)                  
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * Please contact info@openmfg.com with any questions on this license.
 */
#include "krscriptfunctions.h"
#include <kexidb/cursor.h>
#include <kdebug.h>

KRScriptFunctions::KRScriptFunctions(const KexiDB::Cursor *c)
{
	_curs = c;
	_conn = _curs->connection();;
}


KRScriptFunctions::~KRScriptFunctions()
{
}

void KRScriptFunctions::setSource(const QString &s)
{
	_source = s;	
}

void KRScriptFunctions::setWhere(const QString&w)
{
	_where = w;
}

qreal KRScriptFunctions::math(const QString &function, const QString &field)
{
	qreal ret;
	QString sql;
	
	sql = "SELECT " + function +"(" + field + ") FROM " + _source;
	
	if (!_where.isEmpty())
	{
		sql += " WHERE(" + _where + ")";
	}
	
	kDebug() << sql << endl;
	KexiDB::Cursor *curs= _conn->executeQuery(sql);
	
	if (curs)
	{
		ret = curs->value(0).toDouble();
	}
	else
	{
		ret = 0.0;
	}
	delete curs;
	return ret;
}

qreal KRScriptFunctions::sum(const QString &field)
{
	return math("SUM", field);
}

qreal KRScriptFunctions::avg(const QString &field)
{
	return math("AVG", field);
}

qreal KRScriptFunctions::min(const QString &field)
{
	return math("MIN", field);
}

qreal KRScriptFunctions::max(const QString &field)
{
	return math("MAX", field);
}

qreal KRScriptFunctions::count(const QString &field)
{
	return math("COUNT", field);
}

QVariant KRScriptFunctions::value(const QString &field)
{
	QVariant val;
	if (!_curs)
	{
		kDebug() << "No cursor to get value of field " << field << endl;
		return val;
	}
	
	
	KexiDB::QueryColumnInfo::Vector flds = _curs->query()->fieldsExpanded();
	for ( int i = 0; i < flds.size() ; ++i )
	{
		
		if (flds[i]->aliasOrName().toLower() == field.toLower())
		{
			val = const_cast<KexiDB::Cursor*>(_curs)->value(i);
		}
	}
	kDebug() << "Value of " << field <<  " is " << val << endl;
	
	return val;
}

