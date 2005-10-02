/* This file is part of the KDE project
   Copyright (C) 2004 Jaroslaw Staniek <js@iidea.pl>

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

#include "validator.h"

using namespace KexiUtils;

Validator::Validator(QObject * parent, const char * name)
: QValidator(parent,name)
, m_acceptsEmptyValue(false)
{
}

Validator::~Validator()
{
}

Validator::Result Validator::check(const QString &valueName, const QVariant& v, 
	QString &message, QString &details)
{
	if (v.isNull() || v.type()==QVariant::String && v.toString().isEmpty()) {
		if (!m_acceptsEmptyValue) {
			message = Validator::msgColumnNotEmpty().arg(valueName);
			return Error;
		}
		return Ok;
	}
	return internalCheck(valueName, v, message, details);
}

Validator::Result Validator::internalCheck(const QString & /*valueName*/, 
	const QVariant& /*v*/, QString & /*message*/, QString & /*details*/)
{
	return Error;
}

QValidator::State Validator::validate ( QString & , int & ) const
{
	return QValidator::Acceptable;
}

//-----------------------------------------------------------

MultiValidator::MultiValidator(QObject* parent, const char * name)
 : Validator(parent, name)
{
	m_ownedSubValidators.setAutoDelete(true);
}

MultiValidator::MultiValidator(Validator *validator, 
	QObject * parent, const char * name)
 : Validator(parent, name)
{
	addSubvalidator(validator);
}


void MultiValidator::addSubvalidator( Validator* validator, bool owned )
{
	if (!validator)
		return;
	m_subValidators.append(validator);
	if (owned && !validator->parent())
		m_ownedSubValidators.append(validator);
}

QValidator::State MultiValidator::validate( QString & input, int & pos ) const
{
	if (m_subValidators.isEmpty())
		return Invalid;
	State s;
	QValueList<Validator*>::const_iterator it;
	for ( it=m_subValidators.constBegin(); it!=m_subValidators.constEnd(); ++it) {
		s = (*it)->validate(input, pos);
		if (s==Intermediate || s==Invalid)
			return s;
	}
	return Acceptable;
}

void MultiValidator::fixup ( QString & input ) const
{
	QValueList<Validator*>::const_iterator it;
	for ( it=m_subValidators.constBegin(); it!=m_subValidators.constEnd(); ++it) {
		(*it)->fixup(input);
	}
}

Validator::Result MultiValidator::internalCheck(
	const QString &valueName, const QVariant& v, 
	QString &message, QString &details)
{
	if (m_subValidators.isEmpty())
		return Error;
	Result r;
	bool warning = false;
	QValueList<Validator*>::const_iterator it;
	for ( it=m_subValidators.begin(); it!=m_subValidators.end(); ++it) {
		r = (*it)->internalCheck(valueName, v, message, details);
		if (r==Error)
			return Error;
		if (r==Warning)
			warning = true;
	}
	return warning ? Warning : Ok;
}

