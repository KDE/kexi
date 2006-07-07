/* This file is part of the KDE project
   Copyright (C) 2005 Jaroslaw Staniek <js@iidea.pl>

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

#include "kexidataiteminterface.h"

#include <kdebug.h>

KexiDataItemChangesListener::KexiDataItemChangesListener()
{
}

KexiDataItemChangesListener::~KexiDataItemChangesListener()
{
}

//-----------------------------------------------

KexiDataItemInterface::KexiDataItemInterface()
 : m_listener(0)
 , m_parentDataItemInterface(0)
 , m_hasFocusableWidget(true)
 , m_disable_signalValueChanged(false)
 , m_acceptEditorAfterDeleteContents(false)
{
}

KexiDataItemInterface::~KexiDataItemInterface()
{
}

void KexiDataItemInterface::setValue(const QVariant& value, const QVariant& add, bool removeOld)
{
	m_disable_signalValueChanged = true; //to prevent emmiting valueChanged()
//needed?	clear();
	m_origValue = value;
	setValueInternal(add, removeOld);
	m_disable_signalValueChanged = false;
}

void KexiDataItemInterface::signalValueChanged()
{
	if (m_disable_signalValueChanged || isReadOnly())
		return;
	if (m_parentDataItemInterface) {
		m_parentDataItemInterface->signalValueChanged();
		return;
	}
	if (m_listener)
		m_listener->valueChanged(this);
}

bool KexiDataItemInterface::valueChanged()
{
//	bool ok;
//	kdDebug() << m_origValue.toString() << " ? " << value(ok).toString() << endl;
//	return (m_origValue != value(ok)) && ok;
	kdDebug() << "KexiDataItemInterface::valueChanged(): " << m_origValue.toString() << " ? " << value().toString() << endl;
	return m_origValue != value();
}

/*
void KexiDataItemInterface::setValue(const QVariant& value)
{
	m_disable_signalValueChanged = true; //to prevent emmiting valueChanged()
	setValueInternal( value );
	m_disable_signalValueChanged = false;
}*/

void KexiDataItemInterface::installListener(KexiDataItemChangesListener* listener)
{
	m_listener = listener;
}

void KexiDataItemInterface::showFocus( const QRect& r, bool readOnly )
{
	Q_UNUSED(r);
	Q_UNUSED(readOnly);
}

void KexiDataItemInterface::hideFocus()
{
}

void KexiDataItemInterface::clickedOnContents()
{
}

bool KexiDataItemInterface::valueIsValid()
{
	return true;
}
