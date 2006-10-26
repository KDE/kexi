/* This file is part of the KDE project
   Copyright (C) 2006 Jaroslaw Staniek <js@iidea.pl>

   Based on KIntValidator code by Glen Parker <glenebob@nwlink.com>

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

#include "longlongvalidator.h"

#include <qwidget.h>

using namespace KexiUtils;

LongLongValidator::LongLongValidator( QWidget * parent, int base, const char * name )
 : QValidator(parent, name)
 , m_min(0), m_max(0)
{
	setBase(base);
}

LongLongValidator::LongLongValidator( Q_LLONG bottom, Q_LLONG top, QWidget * parent, int base, const char * name )
  : QValidator(parent, name)
{
	setBase(base);
	setRange( bottom, top );
}

LongLongValidator::~LongLongValidator()
{
}

QValidator::State LongLongValidator::validate( QString &str, int & ) const
{
	bool ok;
	Q_LLONG val = 0;
	QString newStr;

	newStr = str.stripWhiteSpace();
	if (m_base > 10)
		newStr = newStr.upper();

	if (newStr == QString::fromLatin1("-")) {// a special case
		if ((m_min || m_max) && m_min >= 0)
			ok = false;
		else
			return QValidator::Acceptable;
	}
	else if (!newStr.isEmpty())
		val = newStr.toLongLong(&ok, m_base);
	else {
		val = 0;
		ok = true;
	}

	if (! ok)
		return QValidator::Invalid;

	if ((! m_min && ! m_max) || (val >= m_min && val <= m_max))
		return QValidator::Acceptable;

	if (m_max && m_min >= 0 && val < 0)
		return QValidator::Invalid;

	return QValidator::Valid;
}

void LongLongValidator::fixup( QString &str ) const
{
	int dummy;
	Q_LLONG val;
	QValidator::State state;

	state = validate(str, dummy);

	if (state == QValidator::Invalid || state == QValidator::Acceptable)
		return;

	if (! m_min && ! m_max)
		return;

	val = str.toLongLong(0, m_base);

	if (val < m_min)
		val = m_min;
	if (val > m_max)
		val = m_max;

	str.setNum(val, m_base);
}

void LongLongValidator::setRange( Q_LLONG bottom, Q_LLONG top )
{
	m_min = bottom;
	m_max = top;

	if (m_max < m_min)
		m_max = m_min;
}

void LongLongValidator::setBase( int base )
{
	m_base = base;
	if (m_base < 2)
		m_base = 2;
	if (m_base > 36)
		m_base = 36;
}

Q_LLONG LongLongValidator::bottom() const
{
	return m_min;
}

Q_LLONG LongLongValidator::top() const
{
	return m_max;
}

int LongLongValidator::base() const
{
	return m_base;
}
