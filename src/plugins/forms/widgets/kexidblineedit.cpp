/* This file is part of the KDE project
   Copyright (C) 2005 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2004-2005 Jaroslaw Staniek <js@iidea.pl>

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

#include "kexidblineedit.h"

#include <knumvalidator.h>
#include <kdatetbl.h>
#include <kexiutils/utils.h>
#include <kexidb/queryschema.h>
#include <kexiutils/utils.h>

KexiDBLineEdit::KexiDBLineEdit(QWidget *parent, const char *name)
 : KLineEdit(parent, name)
 , KexiDBTextWidgetInterface()
 , KexiFormDataItemInterface()
// , m_autonumberDisplayParameters(0)
 , m_slotTextChanged_enabled(true)
{
	connect(this, SIGNAL(textChanged(const QString&)), this, SLOT(slotTextChanged(const QString&)));
}

KexiDBLineEdit::~KexiDBLineEdit()
{
//	delete m_autonumberDisplayParameters;
}

void KexiDBLineEdit::setInvalidState( const QString& displayText )
{
	KLineEdit::setReadOnly(true);
//! @todo move this to KexiDataItemInterface::setInvalidStateInternal() ?
	if (focusPolicy() & TabFocus)
		setFocusPolicy(QWidget::ClickFocus);
	setText(displayText);
}

void KexiDBLineEdit::setValueInternal(const QVariant& add, bool removeOld)
{
	if (m_columnInfo && m_columnInfo->field->type()==KexiDB::Field::Boolean) {
//! @todo temporary solution for booleans!
		setText( add.toBool() ? "1" : "0" );
	}
	else {
		if (removeOld)
			setText( add.toString() );
		else
			setText( m_origValue.toString() + add.toString() );
	}



	if (removeOld && m_columnInfo) {
		const KexiDB::Field::Type t = m_columnInfo->field->type();
		if (t == KexiDB::Field::Boolean) {
			//! @todo temporary solution for booleans!
			setText( add.toBool() ? "1" : "0" );
			return;
		}
	}
	
	m_slotTextChanged_enabled = false;

	if (removeOld)
		setText( add.toString() );
	else
		setText( m_origValue.toString() + add.toString() );
	setCursorPosition(0); //ok?

	m_slotTextChanged_enabled = true;
}

QVariant KexiDBLineEdit::value()
{
	const KexiDB::Field::Type t = m_columnInfo->field->type();
	switch (t) {
	case KexiDB::Field::Text:
	case KexiDB::Field::LongText:
		return text();
	case KexiDB::Field::Byte:
	case KexiDB::Field::ShortInteger:
		return text().toShort();
//! @todo uint, etc?
	case KexiDB::Field::Integer:
		return text().toInt();
	case KexiDB::Field::BigInteger:
		return text().toLongLong();
	case KexiDB::Field::Boolean:
		//! @todo temporary solution for booleans!
		return text() == "1" ? QVariant(true,1) : QVariant(false,0);
/*	case KexiDB::Field::Date:
		return dateFormatter()->stringToVariant( text() );
	case KexiDB::Field::Time:
		return timeFormatter()->stringToVariant( text() );
	case KexiDB::Field::DateTime:
		return stringToDateTime(*dateFormatter(), *timeFormatter(), text());*/
	case KexiDB::Field::Float:
		return text().toFloat();
	case KexiDB::Field::Double:
		return text().toDouble();
	}
//! @todo more data types!

	return text();
}

void KexiDBLineEdit::slotTextChanged(const QString&)
{
	if (!m_slotTextChanged_enabled)
		return;
	signalValueChanged();
}

bool KexiDBLineEdit::valueIsNull()
{
	return valueIsEmpty(); //ok??? text().isNull();
}

bool KexiDBLineEdit::valueIsEmpty()
{
	if (text().isEmpty())
		return true;

	return text().isEmpty();
}

bool KexiDBLineEdit::isReadOnly() const
{
	return KLineEdit::isReadOnly();
}

QWidget* KexiDBLineEdit::widget()
{
	return this;
}

bool KexiDBLineEdit::cursorAtStart()
{
	return cursorPosition()==0;
}

bool KexiDBLineEdit::cursorAtEnd()
{
	return cursorPosition()==(int)text().length();
}

void KexiDBLineEdit::clear()
{
	KLineEdit::clear();
}

void KexiDBLineEdit::setColumnInfo(KexiDB::QueryColumnInfo* cinfo)
{
	KexiFormDataItemInterface::setColumnInfo(cinfo);
	if (!cinfo)
		return;
//! @todo merge this code with KexiTableEdit code!
//! @todo set maximum length validator
//! @todo handle input mask (via QLineEdit::setInputMask()
	const KexiDB::Field::Type t = cinfo->field->type();
	if (cinfo->field->isIntegerType()) {
		QValidator *validator = 0;
		const bool u = cinfo->field->isUnsigned();
		int bottom, top;
		if (t==KexiDB::Field::Byte) {
			bottom = u ? 0 : -0x80;
			top = u ? 0xff : 0x7f;
		}
		else if (t==KexiDB::Field::ShortInteger) {
			bottom = u ? 0 : -0x8000;
			top = u ? 0xffff : 0x7fff;
		}
		else if (t==KexiDB::Field::Integer) {
			bottom = u ? 0 : -0x7fffffff-1;
			top = u ? 0xffffffff : 0x7fffffff;
		}
		else if (t==KexiDB::Field::BigInteger) {
/*! @todo couldn't work with KIntValidator: implement lonlong validator!
			bottom = u ? 0 : -0x7fffffffffffffff;
			top = u ? 0xffffffffffffffff : 127;*/
			validator = new KIntValidator(this);
		}

		if (!validator)
			validator = new KIntValidator(bottom, top, this);
		setValidator( validator );
	}
	else if (cinfo->field->isFPNumericType()) {
		QValidator *validator;
		if (t==KexiDB::Field::Float) {
			if (cinfo->field->isUnsigned()) //ok?
				validator = new KDoubleValidator(0, 3.4e+38, cinfo->field->scale(), this);
			else
				validator = new KDoubleValidator(this);
		}
		else {//double
			if (cinfo->field->isUnsigned()) //ok?
				validator = new KDoubleValidator(0, 1.7e+308, cinfo->field->scale(), this);
			else
				validator = new KDoubleValidator(this);
		}
		setValidator( validator );
	}
	else if (t==KexiDB::Field::Date) {
//! @todo use KDateWidget
		QValidator *validator = new KDateValidator(this);
		setValidator( validator );
	}
	else if (t==KexiDB::Field::Time) {
//! @todo use KTimeWidget
		setInputMask("00:00:00");
	}
	else if (t==KexiDB::Field::Boolean) {
//! @todo temporary solution for booleans!
		QValidator *validator = new KIntValidator(0, 1, this);
		setValidator( validator );
	}

	KexiDBTextWidgetInterface::setColumnInfo(cinfo, this);
}

void KexiDBLineEdit::paintEvent ( QPaintEvent *pe )
{
	KLineEdit::paintEvent( pe );
	KexiDBTextWidgetInterface::paintEvent( this, text().isEmpty(), alignment(), hasFocus() );
}

bool KexiDBLineEdit::event( QEvent * e )
{
	const bool ret = KLineEdit::event( e );
	KexiDBTextWidgetInterface::event(e, this, text().isEmpty());
	return ret;
}

#include "kexidblineedit.moc"
