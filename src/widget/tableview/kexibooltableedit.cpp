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

#include "kexibooltableedit.h"

#include <kexidb/field.h>

#include <qpainter.h>
#include <qapplication.h>
#include <qclipboard.h>

#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>
#include <kglobalsettings.h>


KexiBoolTableEdit::KexiBoolTableEdit(KexiTableViewColumn &column, QScrollView *parent)
 : KexiTableEdit(column, parent, "KexiBoolTableEdit")
{
	init();
}

KexiBoolTableEdit::~KexiBoolTableEdit()
{
}

void KexiBoolTableEdit::init()
{
	kdDebug() << "KexiBoolTableEdit: m_origValue.typeName()==" << m_origValue.typeName() << endl;
	kdDebug() << "KexiBoolTableEdit: type== " << field()->typeName() << endl;

	m_hasFocusableWidget = false;
}
	
void KexiBoolTableEdit::setValueInternal(const QVariant& /*add*/, bool /*removeOld*/)
{
	m_currentValue = m_origValue;
	//nothing to do more...
}

/*bool KexiBoolTableEdit::valueChanged()
{
	if (m_lineedit->text()!=m_origText)
		return true;
	return KexiTableEdit::valueChanged();
}*/

bool KexiBoolTableEdit::valueIsNull()
{
	return m_currentValue.isNull();
}

bool KexiBoolTableEdit::valueIsEmpty()
{
	return m_currentValue.isNull();
}

QVariant KexiBoolTableEdit::value()
{
//	ok = true;
	return m_currentValue;
}

void KexiBoolTableEdit::clear()
{
	m_currentValue = QVariant();
}

bool KexiBoolTableEdit::cursorAtStart()
{
	return true;
}

bool KexiBoolTableEdit::cursorAtEnd()
{
	return true;
}

/*QSize KexiBoolTableEdit::totalSize()
{
	if (!m_lineedit)
		return size();
	return m_lineedit->size();
}*/

void KexiBoolTableEdit::setupContents( QPainter *p, bool /*focused*/, const QVariant& val, 
	QString &/*txt*/, int &/*align*/, int &/*x*/, int &y_offset, int &w, int &h  )
{
#ifdef Q_WS_WIN
//	x = 1;
	y_offset = -1;
#else
//	x = 1;
	y_offset = 0;
#endif
	int s = QMAX(h - 5, 12);
	s = QMIN( h-3, s );
	s = QMIN( w-3, s );//avoid too large box
	QRect r( QMAX( w/2 - s/2, 0 ) , h/2 - s/2 /*- 1*/, s, s);
	if (val.isNull() && !field()->isNotNull())
		p->setPen(QPen(palette().disabled().text(), 1));
	else
		p->setPen(QPen(colorGroup().text(), 1));
	p->drawRect(r);
	if (val.isNull() && !field()->isNotNull()) {
		p->drawText( r, Qt::AlignCenter, "?" );
	}
	else if (val.toBool()) {
		p->drawLine(r.x(), r.y(), r.right(), r.bottom());
		p->drawLine(r.x(), r.bottom(), r.right(), r.y());
//			p->drawLine(r.x() + 2, r.y() + 2, r.right() - 1, r.bottom() - 1);
//			p->drawLine(r.x() + 2, r.bottom() - 2, r.right() - 1, r.y() + 1);
	}

	
}

void KexiBoolTableEdit::clickedOnContents()
{
	m_currentValue = QVariant( !m_currentValue.toBool(), 0 );
}

void KexiBoolTableEdit::handleAction(const QString& actionName)
{
	if (actionName=="edit_paste") {
		emit editRequested();
		bool ok;
		const int value = qApp->clipboard()->text( QClipboard::Clipboard ).toInt(&ok);
		if (ok) {
			m_currentValue = (value==0) ? QVariant(false, 0) : QVariant(true, 1);
		}
		else {
			m_currentValue = field()->isNotNull() 
				? QVariant(0, false)/*0 instead of NULL - handle case when null is not allowed*/
				: QVariant();
		}
		repaintRelatedCell();
	}
	else if (actionName=="edit_cut") {
		emit editRequested();
//! @todo handle defaultValue...
		m_currentValue = field()->isNotNull() 
			? QVariant(0, false)/*0 instead of NULL - handle case when null is not allowed*/
			: QVariant();
		handleCopyAction(m_origValue);
		repaintRelatedCell();
	}
}

void KexiBoolTableEdit::handleCopyAction(const QVariant& value)
{
	if (value.type()==QVariant::Bool)
		qApp->clipboard()->setText(value.toBool() ? "1" : "0");
	else
		qApp->clipboard()->setText(QString::null);
}

//======================================================

KexiBoolEditorFactoryItem::KexiBoolEditorFactoryItem()
{
}

KexiBoolEditorFactoryItem::~KexiBoolEditorFactoryItem()
{
}

KexiTableEdit* KexiBoolEditorFactoryItem::createEditor(
	KexiTableViewColumn &column, QScrollView* parent)
{
	return new KexiBoolTableEdit(column, parent);
}

#include "kexibooltableedit.moc"

