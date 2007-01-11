/* This file is part of the KDE project
   Copyright (C) 2002 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2003-2007 Jaroslaw Staniek <js@iidea.pl>

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

#include "kexiinputtableedit.h"

#include <qregexp.h>
#include <qevent.h>
#include <qlayout.h>
#include <qtimer.h>
#include <qpainter.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <qtooltip.h>

#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kcompletionbox.h>
#include <knumvalidator.h>
#include <kexiutils/longlongvalidator.h>
#include <kexidb/field.h>
#include <kexidb/fieldvalidator.h>

//! @internal
class MyLineEdit : public KLineEdit
{
	public:
		MyLineEdit(QWidget *parent, const char *name) : KLineEdit(parent,name)
		{}
	protected:
		virtual void drawFrame ( QPainter * p ) {
			p->setPen( QPen( colorGroup().text() ) );
			QRect r = rect();
			p->moveTo( r.topLeft() );
			p->lineTo( r.topRight() );
			p->lineTo( r.bottomRight() );
			p->lineTo( r.bottomLeft() );
			if (pos().x() == 0) //draw left side only when it is @ the edge
				p->lineTo( r.topLeft() );
		}
};

//======================================================

KexiInputTableEdit::KexiInputTableEdit(KexiTableViewColumn &column, QWidget *parent)
 : KexiTableEdit(column, parent)
{
	setName("KexiInputTableEdit");
//	m_type = f.type(); //copied because the rest of code uses m_type
//	m_field = &f;
//	m_origValue = value;//original value
	init();
}

KexiInputTableEdit::~KexiInputTableEdit()
{
}

void KexiInputTableEdit::init()
{
//	kdDebug() << "KexiInputTableEdit: m_origValue.typeName()==" << m_origValue.typeName() << endl;
//	kdDebug() << "KexiInputTableEdit: type== " << field()->typeName() << endl;
//	kdDebug() << "KexiInputTableEdit: displayed type== " << displayedField()->typeName() << endl;

	m_textFormatter.setField( field() );

	//init settings
	m_decsym = KGlobal::locale()->decimalSymbol();
	if (m_decsym.isEmpty())
		m_decsym=".";//default

	const bool align_right = displayedField()->isNumericType();

	if (!align_right) {
		//create layer for internal editor
		QHBoxLayout *lyr = new QHBoxLayout(this);
		lyr->addSpacing(4);
		lyr->setAutoAdd(true);
	}

	//create internal editor
	m_lineedit = new MyLineEdit(this, "KexiInputTableEdit-KLineEdit");
	setViewWidget(m_lineedit);
	if (align_right)
		m_lineedit->setAlignment(AlignRight);
//	m_cview->setFrame(false);
//	m_cview->setFrameStyle( QFrame::Plain | QFrame::Box );
//	m_cview->setLineWidth( 1 );
	m_calculatedCell = false;

#if 0 //js TODO
	connect(m_cview->completionBox(), SIGNAL(activated(const QString &)),
	 this, SLOT(completed(const QString &)));
	connect(m_cview->completionBox(), SIGNAL(highlighted(const QString &)),
	 this, SLOT(completed(const QString &)));
	 m_cview->completionBox()->setTabHandling(true);
#endif

}

void KexiInputTableEdit::setValueInternal(const QVariant& add, bool removeOld)
{
	QString text( m_textFormatter.valueToText(removeOld ? QVariant() : m_origValue, add.toString()) );
	if (text.isEmpty()) {
		if (m_origValue.toString().isEmpty()) {
			//we have to set NULL initial value:
			m_lineedit->setText(QString::null);
		}
	}
	else {
		m_lineedit->setText(text);
	}

#if 0
//move to end is better by default
		m_cview->selectAll();
#else
//js TODO: by default we're moving to the end of editor, ADD OPTION allowing "select all chars"
		m_lineedit->end(false);
#endif

	if (!m_lineedit->validator()) {
		QValidator *validator = new KexiDB::FieldValidator(
			*field(), m_lineedit, "KexiInputTableEdit-validator");
		m_lineedit->setValidator( validator );
	}
}

#if 0
//moved to KexiTextFormatter
QString KexiInputTableEdit::valueToText(KexiDB::Field* field, const QVariant& value, const QString& add)
{
	QString text; //result

	if (field->isFPNumericType()) {
//! @todo precision!
//! @todo support 'g' format
		text = QString::number(value.toDouble(), 'f', 
			QMAX(field->visibleDecimalPlaces(), 10)); //<-- 10 is quite good maximum for fractional digits 
													  //! @todo add command line settings?
		if (value.toDouble() == 0.0) {
			text = add.isEmpty() ? "0" : add; //eat 0
		}
		else {
//! @todo (js): get decimal places settings here...
			QStringList sl = QStringList::split(".", text);
			if (text.isEmpty()) {
				//nothing
			}
			else if (sl.count()==2) {
//				kdDebug() << "sl.count()=="<<sl.count()<< " " <<sl[0] << " | " << sl[1] << endl;
				const QString sl1 = sl[1];
				int pos = sl1.length()-1;
				if (pos>=1) {
					for (;pos>=0 && sl1[pos]=='0';pos--)
						;
					pos++;
				}
				if (pos>0)
					text = sl[0] + m_decsym + sl1.left(pos);
				else
					text = sl[0]; //no decimal point
			}
			text += add;
		}
/*moved to KexiDB::FieldValidator
		if (setValidator && !m_lineedit->validator()) {
			QValidator *validator = new KDoubleValidator(m_lineedit);
			m_lineedit->setValidator( validator );
		}*/
	}
	else {
		text = value.toString();
		if (field->isIntegerType()) {
			if (value.toInt() == 0) {
				text = add; //eat 0
			}
			else {
				text += add;
			}
/*moved to KexiDB::FieldValidator
//! @todo implement ranges here!
			if (setValidator && !m_lineedit->validator()) {
				QValidator *validator;
				if (KexiDB::Field::BigInteger == field()->type()) {
//! @todo use field->isUnsigned() for KexiUtils::ULongLongValidator
					validator = new KexiUtils::LongLongValidator(m_lineedit); 
				}
				else {
					validator = new KIntValidator(m_lineedit);
				}
				m_lineedit->setValidator( validator );
			}*/
		}
		else {//default: text
			text += add;
		}
	}

	return text;
}
#endif

void KexiInputTableEdit::paintEvent ( QPaintEvent * /*e*/ )
{
	QPainter p(this);
	p.setPen( QPen( colorGroup().text() ) );
	p.drawRect( rect() );
}

void
KexiInputTableEdit::setRestrictedCompletion()
{
#if 0 //js TODO
kdDebug() << "KexiInputTableEdit::setRestrictedCompletion()" << endl;
//	KLineEdit *content = static_cast<KLineEdit*>(m_view);
	if(m_cview->text().isEmpty())
		return;

	kdDebug() << "KexiInputTableEdit::setRestrictedCompletion(): something to do" << endl;

	m_cview->useGlobalKeyBindings();

	QStringList newC;
	QStringList::ConstIterator it, end( m_comp.constEnd() );
	for( it = m_comp.constBegin(); it != end; ++it)
	{
		if((*it).startsWith(m_cview->text()))
			newC.append(*it);
	}
	m_cview->setCompletedItems(newC);
#endif
}

void
KexiInputTableEdit::completed(const QString &s)
{
//	kdDebug() << "KexiInputTableEdit::completed(): " << s << endl;
	m_lineedit->setText(s);
}

bool KexiInputTableEdit::valueChanged()
{
	//not needed? if (m_lineedit->text()!=m_origValue.toString())
	//not needed? 	return true;
	return KexiTableEdit::valueChanged();
}

bool KexiInputTableEdit::valueIsNull()
{
	return m_lineedit->text().isNull();
}

bool KexiInputTableEdit::valueIsEmpty()
{
	return !m_lineedit->text().isNull() && m_lineedit->text().isEmpty();
}

QVariant KexiInputTableEdit::value()
{
	if (field()->isFPNumericType()) {//==KexiDB::Field::Double || m_type==KexiDB::Field::Float) {
		//! js @todo PRESERVE PRECISION!
		QString txt = m_lineedit->text();
		if (m_decsym!=".")
			txt = txt.replace(m_decsym,".");//convert back
		bool ok;
		const double result = txt.toDouble(&ok);
		return ok ? QVariant(result) : QVariant();
	}
	else if (field()->isIntegerType()) {
//! @todo check constraints
		bool ok;
		if (KexiDB::Field::BigInteger == field()->type()) {
			if (field()->isUnsigned()) {
				const Q_ULLONG result = m_lineedit->text().toULongLong(&ok);
				return ok ? QVariant(result) : QVariant();
			}
			else {
				const Q_LLONG result = m_lineedit->text().toLongLong(&ok);
				return ok ? QVariant(result) : QVariant();
			}
		}
		if (KexiDB::Field::Integer == field()->type()) {
			if (field()->isUnsigned()) {
				const uint result = m_lineedit->text().toUInt(&ok);
				return ok ? QVariant(result) : QVariant();
			}
		}
		//default: signed int
		const int result = m_lineedit->text().toInt(&ok);
		return ok ? QVariant(result) : QVariant();
	}
	//default: text
	return m_lineedit->text();
}

void
KexiInputTableEdit::clear()
{
	m_lineedit->clear();
}

bool KexiInputTableEdit::cursorAtStart()
{
	return m_lineedit->cursorPosition()==0;
}

bool KexiInputTableEdit::cursorAtEnd()
{
	return m_lineedit->cursorPosition()==(int)m_lineedit->text().length();
}

QSize KexiInputTableEdit::totalSize()
{
	if (!m_lineedit)
		return size();
	return m_lineedit->size();
}

void KexiInputTableEdit::handleCopyAction(const QVariant& value, const QVariant& visibleValue)
{
	Q_UNUSED(visibleValue);
//! @todo handle rich text?
	qApp->clipboard()->setText( m_textFormatter.valueToText(value, QString::null) );
}

void KexiInputTableEdit::handleAction(const QString& actionName)
{
	const bool alreadyVisible = m_lineedit->isVisible();

	if (actionName=="edit_paste") {
		if (!alreadyVisible) { //paste as the entire text if the cell was not in edit mode
			emit editRequested();
			m_lineedit->clear();
		}
		m_lineedit->paste();
	}
	else if (actionName=="edit_cut") {
//! @todo handle rich text?
		if (!alreadyVisible) { //cut the entire text if the cell was not in edit mode
			emit editRequested();
			m_lineedit->selectAll();
		}
		m_lineedit->cut();
	}
}

bool KexiInputTableEdit::showToolTipIfNeeded(const QVariant& value, const QRect& rect, 
	const QFontMetrics& fm, bool focused)
{
	QString text( value.type()==QVariant::String ? value.toString()
		: m_textFormatter.valueToText(value, QString::null) );
	QRect internalRect(rect);
	internalRect.setLeft(rect.x()+leftMargin());
	internalRect.setWidth(internalRect.width()-rightMargin(focused)-2*3);
	kexidbg << rect << " " << internalRect << " " << fm.width(text) << endl;
	return fm.width(text) > internalRect.width();
}

void KexiInputTableEdit::moveCursorToEnd()
{
	m_lineedit->end(false/*!mark*/);
}

void KexiInputTableEdit::moveCursorToStart()
{
	m_lineedit->home(false/*!mark*/);
}

void KexiInputTableEdit::selectAll()
{
	m_lineedit->selectAll();
}

KEXI_CELLEDITOR_FACTORY_ITEM_IMPL(KexiInputEditorFactoryItem, KexiInputTableEdit)

#include "kexiinputtableedit.moc"
