/* This file is part of the KDE project
   Copyright (C) 2002 Peter Simonsson <psn@linux.se>
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

#include <qlayout.h>
#include <qstyle.h>
#include <qwindowsstyle.h>
#include <qpainter.h>

#include "kexicomboboxbase.h"
#include <widget/utils/kexicomboboxdropdownbutton.h>
#include "kexicomboboxpopup.h"
#include "kexitableview.h"
#include "kexitableitem.h"
#include "kexi.h"

#include <klineedit.h>

KexiComboBoxBase::KexiComboBoxBase()
{
	m_internalEditorValueChanged = false; //user has text or other value inside editor
	m_slotInternalEditorValueChanged_enabled = true;
	m_mouseBtnPressedWhenPopupVisible = false;
	m_insideCreatePopup = false;
	m_setValueOrTextInInternalEditor_enabled = true;
	m_updatePopupSelectionOnShow = true;
	m_moveCursorToEndInInternalEditor_enabled = true;
	m_selectAllInInternalEditor_enabled = true;
	m_setValueInInternalEditor_enabled = true;
	m_setVisibleValueOnSetValueInternal = false;
}

KexiComboBoxBase::~KexiComboBoxBase()
{
}

KexiDB::LookupFieldSchema *KexiComboBoxBase::lookupFieldSchema() const
{
	if (field() && field()->table()) {
		KexiDB::LookupFieldSchema *lookupFieldSchema = field()->table()->lookupFieldSchema( *field() );
		if (lookupFieldSchema && !lookupFieldSchema->rowSource().name().isEmpty())
			return lookupFieldSchema;
	}
	return 0;
}

int KexiComboBoxBase::rowToHighlightForLookupTable() const
{
	if (!popup())
		return -1;//err
	KexiDB::LookupFieldSchema *lookupFieldSchema = this->lookupFieldSchema();
	if (!lookupFieldSchema)
		return -1;
	if (lookupFieldSchema->boundColumn()==-1)
		return -1; //err
	bool ok;
	const int rowUid = origValue().toInt();
//! @todo for now we're assuming the id is INTEGER
	KexiTableViewData *tvData = popup()->tableView()->data();
	const int boundColumn = lookupFieldSchema->boundColumn();
	KexiTableViewData::Iterator it(tvData->iterator());
	int row=0;
	for (;it.current();++it, row++)
	{
		if (it.current()->at(boundColumn).toInt(&ok) == rowUid && ok || !ok)
			break;
	}
	if (!ok || !it.current()) //item not found: highlight 1st row, if available
		return -1;
	return row;
}

void KexiComboBoxBase::setValueInternal(const QVariant& add_, bool removeOld)
{
	Q_UNUSED(removeOld);
	m_mouseBtnPressedWhenPopupVisible = false;
	m_updatePopupSelectionOnShow = true;
	QString add(add_.toString());
	if (add.isEmpty()) {
		KexiTableViewData *relData = column() ? column()->relatedData() : 0;
		QVariant valueToSet;
		bool hasValueToSet = true;
		int rowToHighlight = -1;
		KexiDB::LookupFieldSchema *lookupFieldSchema = this->lookupFieldSchema();
		if (lookupFieldSchema) {
			//use 'lookup field' model
//! @todo support more RowSourceType's, not only table
			if (lookupFieldSchema->boundColumn()==-1)
//! @todo errmsg
				return;
			if (m_setVisibleValueOnSetValueInternal) {
				//only for table views
				if (!popup())
					createPopup(false/*!show*/);
			}
			if (popup()) {
				const int rowToHighlight = rowToHighlightForLookupTable();
				popup()->tableView()->setHighlightedRow(rowToHighlight);

				const int visibleColumn = lookupFieldSchema->visibleColumn( popup()->tableView()->data()->columnsCount() );
				if (m_setVisibleValueOnSetValueInternal && -1!=visibleColumn) {
					//only for table views
					KexiTableItem *it = popup()->tableView()->highlightedItem();
					if (it)
						valueToSet = it->at( visibleColumn );
				}
				else {
					hasValueToSet = false;
				}
			}
		}
		else if (relData) {
			//use 'related table data' model
			valueToSet = valueForString(origValue().toString(), &rowToHighlight, 0, 1);
		}
		else {
			//use 'enum hints' model
			const int row = origValue().toInt();
			valueToSet = field()->enumHint(row).stripWhiteSpace();
		}
		if (hasValueToSet)
			setValueOrTextInInternalEditor( valueToSet );
		/*impl.*/moveCursorToEndInInternalEditor();
		/*impl.*/selectAllInInternalEditor();
		
		if (popup()) {
			if (origValue().isNull()) {
				popup()->tableView()->clearSelection();
				popup()->tableView()->setHighlightedRow(0);
			} else {
				if (relData) {
					if (rowToHighlight!=-1)
						popup()->tableView()->setHighlightedRow(rowToHighlight);
				}
				else if (!lookupFieldSchema) {
					//popup()->tableView()->selectRow(origValue().toInt());
					popup()->tableView()->setHighlightedRow(origValue().toInt());
				}
			}
		}
	}
	else {
		//todo: autocompl.?
		if (popup())
			popup()->tableView()->clearSelection();
		/*impl.*/setValueInInternalEditor(add); //not setLineEditText(), because 'add' is entered by user!
		//setLineEditText( add );
		/*impl.*/moveCursorToEndInInternalEditor();
	}
}

KexiTableItem* KexiComboBoxBase::selectItemForEnteredValueInLookupTable(const QVariant& v)
{
	KexiDB::LookupFieldSchema *lookupFieldSchema = this->lookupFieldSchema();
	if (!popup() || !lookupFieldSchema)
		return 0; //safety
//-not effective for large sets: please cache it!
//.stripWhiteSpace() is not generic!

	const bool valueIsText = v.type()==QVariant::String || v.type()==QVariant::CString; //most common case
	const QString txt( valueIsText ? v.toString().stripWhiteSpace().lower() : QString::null );
	KexiTableViewData *lookupData = popup()->tableView()->data();
	const int visibleColumn = lookupFieldSchema->visibleColumn( lookupData->columnsCount() );
	if (-1 == visibleColumn)
		return 0;
	KexiTableViewData::Iterator it(lookupData->iterator());
	int row;
	for (row = 0;it.current();++it, row++) {
		if (valueIsText) {
			if (it.current()->at(visibleColumn).toString().stripWhiteSpace().lower() == txt)
				break;
		}
		else {
			if (it.current()->at(visibleColumn) == v)
				break;
		}
	}

	m_setValueOrTextInInternalEditor_enabled = false; // <-- this is the entered value, 
	                                                  //     so do not change the internal editor's contents
	if (it.current())
		popup()->tableView()->selectRow(row);
	else
		popup()->tableView()->clearSelection();

	m_setValueOrTextInInternalEditor_enabled = true;

	return it.current();
}

QString KexiComboBoxBase::valueForString(const QString& str, int* row, 
	uint lookInColumn, uint returnFromColumn, bool allowNulls)
{
	KexiTableViewData *relData = column() ? column()->relatedData() : 0;
	if (!relData)
		return QString::null; //safety
	//use 'related table data' model
	//-not effective for large sets: please cache it!
	//.stripWhiteSpace() is not generic!

	const QString txt = str.stripWhiteSpace().lower();
	KexiTableViewData::Iterator it( relData->iterator() );
	for (*row = 0;it.current();++it, (*row)++) {
		if (it.current()->at(lookInColumn).toString().stripWhiteSpace().lower()==txt)
			break;
	}
	if (it.current())
		return it.current()->at(returnFromColumn).toString();

	*row = -1;

	if (column() && column()->relatedDataEditable())
		return str; //new value entered and that's allowed

	kexiwarn << "KexiComboBoxBase::valueForString(): no related row found, ID will be painted!" << endl;
	if (allowNulls)
		return QString::null;
	return str; //for sanity but it's weird to show id to the user
}

QVariant KexiComboBoxBase::value()
{
	KexiTableViewData *relData = column() ? column()->relatedData() : 0;
	KexiDB::LookupFieldSchema *lookupFieldSchema = 0;
	if (relData) {
		if (m_internalEditorValueChanged) {
			//we've user-entered text: look for id
//TODO: make error if matching text not found?
			int rowToHighlight;
			return valueForString(m_userEnteredValue.toString(), &rowToHighlight, 1, 0, true/*allowNulls*/);
		}
		else {
			//use 'related table data' model
			KexiTableItem *it = popup() ? popup()->tableView()->selectedItem() : 0;
			return it ? it->at(0) : origValue();//QVariant();
		}
	}
	else if ((lookupFieldSchema = this->lookupFieldSchema()))
	{
		if (lookupFieldSchema->boundColumn()==-1)
			return origValue();
		KexiTableItem *it = popup() ? popup()->tableView()->selectedItem() : 0;
		if (/*!it &&*/ m_internalEditorValueChanged && !m_userEnteredValue.toString().isEmpty()) { //
			//try to select a row using the user-entered text
			if (!popup()) {
				QVariant prevUserEnteredValue = m_userEnteredValue;
				createPopup(false);
				m_userEnteredValue = prevUserEnteredValue;
			}
			it = selectItemForEnteredValueInLookupTable( m_userEnteredValue );
		}
		return it ? it->at( lookupFieldSchema->boundColumn() ) : QVariant();
	}
	else if (popup()) {
		//use 'enum hints' model
		const int row = popup()->tableView()->currentRow();
		if (row>=0)
			return QVariant( row );
	}

	if (valueFromInternalEditor().toString().isEmpty())
		return QVariant();
/*! \todo don't return just 1st row, but use autocompletion feature
      and: show message box if entered text does not match! */
	return origValue(); //unchanged
}

QVariant KexiComboBoxBase::visibleValueForLookupField()
{
	KexiDB::LookupFieldSchema *lookupFieldSchema = this->lookupFieldSchema();
	if (!popup() || !lookupFieldSchema)
		return QVariant();
	const int visibleColumn = lookupFieldSchema->visibleColumn( popup()->tableView()->data()->columnsCount() );
	if (-1 == visibleColumn)
		return QVariant();
	KexiTableItem *it = popup()->tableView()->selectedItem();
	return it ? it->at( QMIN( (uint)visibleColumn, it->count()-1)/*sanity*/ ) : QVariant();
}

QVariant KexiComboBoxBase::visibleValue()
{
	return m_visibleValue;
}

void KexiComboBoxBase::clear()
{
	if (popup())
		popup()->hide();
	slotInternalEditorValueChanged(QVariant());
}

tristate KexiComboBoxBase::valueChangedInternal()
{
	//avoid comparing values:
	KexiTableViewData *relData = column() ? column()->relatedData() : 0;
	KexiDB::LookupFieldSchema *lookupFieldSchema = this->lookupFieldSchema();
	if (relData || lookupFieldSchema) {
		if (m_internalEditorValueChanged)
			return true;

		//use 'related table data' model
		KexiTableItem *it = popup() ? popup()->tableView()->selectedItem() : 0;
		if (!it)
			return false;
	}
	else {
		//use 'enum hints' model
		const int row = popup() ? popup()->tableView()->currentRow() : -1;
		if (row<0 && !m_internalEditorValueChanged/*true if text box is cleared*/)
			return false;
	}

	return cancelled;
}

bool KexiComboBoxBase::valueIsNull()
{
//	bool ok;
	QVariant v( value() );
	return v.isNull();
//	return !ok || v.isNull();
}

bool KexiComboBoxBase::valueIsEmpty()
{
	return valueIsNull();
}

void KexiComboBoxBase::showPopup()
{
	createPopup(true);
}

void KexiComboBoxBase::createPopup(bool show)
{
	if (!field())
		return;
	m_insideCreatePopup = true;
	QWidget* thisWidget = dynamic_cast<QWidget*>(this);
	QWidget *widgetToFocus = internalEditor() ? internalEditor() : thisWidget;
	if (!popup()) {
		setPopup( column() ? new KexiComboBoxPopup(thisWidget, *column()) 
			: new KexiComboBoxPopup(thisWidget, *field()) );
		QObject::connect(popup(), SIGNAL(rowAccepted(KexiTableItem*,int)), 
			thisWidget, SLOT(slotRowAccepted(KexiTableItem*,int)));
		QObject::connect(popup()->tableView(), SIGNAL(itemSelected(KexiTableItem*)),
			thisWidget, SLOT(slotItemSelected(KexiTableItem*)));

		popup()->setFocusProxy( widgetToFocus );	
		popup()->tableView()->setFocusProxy( widgetToFocus );
		popup()->installEventFilter(thisWidget);

		if (origValue().isNull())
			popup()->tableView()->clearSelection();
		else {
			popup()->tableView()->selectRow( 0 );
			popup()->tableView()->setHighlightedRow( 0 );
		}
	}
	if (show && internalEditor() && !internalEditor()->isVisible())
		/*emit*/editRequested();

	QPoint posMappedToGlobal = mapFromParentToGlobal(thisWidget->pos());
	if (posMappedToGlobal != QPoint(-1,-1)) {
//! todo alter the position to fit the popup within screen boundaries
		popup()->move( posMappedToGlobal + QPoint(0, thisWidget->height()) );
		//to avoid flickering: first resize to 0-height, then show and resize back to prev. height
		const int w = popupWidthHint();
		popup()->resize(w, 0);
		if (show)
			popup()->show();
		popup()->updateSize(w);
		if (m_updatePopupSelectionOnShow) {
			int rowToHighlight = -1;
			KexiDB::LookupFieldSchema *lookupFieldSchema = this->lookupFieldSchema();
			KexiTableViewData *relData = column() ? column()->relatedData() : 0;
			if (lookupFieldSchema) {
				rowToHighlight = rowToHighlightForLookupTable();
			}
			else if (relData) {
				(void)valueForString(origValue().toString(), &rowToHighlight, 0, 1);
			}
			else //enum hint
				rowToHighlight = origValue().toInt();

/*-->*/	m_moveCursorToEndInInternalEditor_enabled = show;
			m_selectAllInInternalEditor_enabled = show;
			m_setValueInInternalEditor_enabled = show;
			if (rowToHighlight==-1) {
				rowToHighlight = QMAX( popup()->tableView()->highlightedRow(), 0);
				setValueInInternalEditor(QVariant());
			}
			popup()->tableView()->selectRow( rowToHighlight );
			popup()->tableView()->setHighlightedRow( rowToHighlight );
			if (rowToHighlight < popup()->tableView()->rowsPerPage())
				popup()->tableView()->ensureCellVisible( 0, -1 );

/*-->*/	m_moveCursorToEndInInternalEditor_enabled = true;
			m_selectAllInInternalEditor_enabled = true;
			m_setValueInInternalEditor_enabled = true;
		}
	}

	if (show) {
		moveCursorToEndInInternalEditor();
		selectAllInInternalEditor();
		widgetToFocus->setFocus();
	}
	m_insideCreatePopup = false;
}

void KexiComboBoxBase::hide()
{
	if (popup())
		popup()->hide();
}

void KexiComboBoxBase::slotRowAccepted(KexiTableItem * item, int row)
{
	Q_UNUSED(row);
	//update our value
	//..nothing to do?
	updateButton();
	slotItemSelected(item);
	/*emit*/acceptRequested();
}

void KexiComboBoxBase::acceptPopupSelection()
{
	if (!popup())
		return;
	KexiTableItem *item = popup()->tableView()->highlightedItem();
	if (item) {
		popup()->tableView()->selectRow( popup()->tableView()->highlightedRow() );
		slotRowAccepted(item, -1);
	}
	popup()->hide();
}

void KexiComboBoxBase::slotItemSelected(KexiTableItem*)
{
	kexidbg << "KexiComboBoxBase::slotItemSelected(): m_visibleValue = " << m_visibleValue << endl;

	QVariant valueToSet;
	KexiTableViewData *relData = column() ? column()->relatedData() : 0;
	KexiDB::LookupFieldSchema *lookupFieldSchema = this->lookupFieldSchema();

	m_visibleValue = lookupFieldSchema ? visibleValueForLookupField() : QVariant();

	if (relData) {
		//use 'related table data' model
		KexiTableItem *item = popup()->tableView()->selectedItem();
		if (item)
			valueToSet = item->at(1);
	}
	else if (lookupFieldSchema) {
		KexiTableItem *item = popup()->tableView()->selectedItem();
		const int visibleColumn = lookupFieldSchema->visibleColumn( popup()->tableView()->data()->columnsCount() );
		if (item && visibleColumn!=-1 /* && (int)item->size() >= visibleColumn --already checked*/) {
			valueToSet = item->at( QMIN( (uint)visibleColumn, item->count()-1)/*sanity*/ );
		}
	}
	else {
		//use 'enum hints' model
		valueToSet = field()->enumHint( popup()->tableView()->currentRow() );
		if (valueToSet.toString().isEmpty() && !m_insideCreatePopup) {
			clear();
			QWidget* thisWidget = dynamic_cast<QWidget*>(this);
			thisWidget->parentWidget()->setFocus();
			return;
		}
	}
	setValueOrTextInInternalEditor( valueToSet );
	if (m_setValueOrTextInInternalEditor_enabled) {
		moveCursorToEndInInternalEditor();
		selectAllInInternalEditor();
	}
	// a new (temp) popup table index is selected: do not update selection next time:
	m_updatePopupSelectionOnShow = false;
}

void KexiComboBoxBase::slotInternalEditorValueChanged(const QVariant& v)
{
	if (!m_slotInternalEditorValueChanged_enabled)
		return;
	m_userEnteredValue = v;
	m_internalEditorValueChanged = true;
	if (v.toString().isEmpty()) {
		if (popup()) {
			popup()->tableView()->clearSelection();
		}
		return;
	}
}

void KexiComboBoxBase::setValueOrTextInInternalEditor(const QVariant& value)
{
	if (!m_setValueOrTextInInternalEditor_enabled)
		return;
	setValueInInternalEditor( value );
	//this text is not entered by hand:
	m_userEnteredValue = QVariant();
	m_internalEditorValueChanged = false;
}

bool KexiComboBoxBase::handleKeyPressForPopup( QKeyEvent *ke )
{
	const int k = ke->key();
	int highlightedOrSelectedRow = popup() ? popup()->tableView()->highlightedRow() : -1;
	if (popup() && highlightedOrSelectedRow < 0)
		highlightedOrSelectedRow = popup()->tableView()->currentRow();

	const bool enterPressed = k==Qt::Key_Enter || k==Qt::Key_Return;

	// The editor may be active but the pull down menu not existant/visible,
	// e.g. when the user has pressed a normal button to activate the editor
	// Don't handle the event here in that case.
	if (!popup() || (!enterPressed && !popup()->isVisible())) {
		return false;
	}

	switch (k) {
	case Qt::Key_Up:
			popup()->tableView()->setHighlightedRow( 
				QMAX(highlightedOrSelectedRow-1, 0) );
			updateTextForHighlightedRow();
			return true;
	case Qt::Key_Down:
			popup()->tableView()->setHighlightedRow( 
				QMIN(highlightedOrSelectedRow+1, popup()->tableView()->rows()-1) );
			updateTextForHighlightedRow();
			return true;
	case Qt::Key_PageUp:
			popup()->tableView()->setHighlightedRow( 
				QMAX(highlightedOrSelectedRow-popup()->tableView()->rowsPerPage(), 0) );
			updateTextForHighlightedRow();
			return true;
	case Qt::Key_PageDown:
			popup()->tableView()->setHighlightedRow( 
				QMIN(highlightedOrSelectedRow+popup()->tableView()->rowsPerPage(), 
				 popup()->tableView()->rows()-1) );
			updateTextForHighlightedRow();
			return true;
	case Qt::Key_Home:
			popup()->tableView()->setHighlightedRow( 0 );
			updateTextForHighlightedRow();
			return true;
	case Qt::Key_End:
			popup()->tableView()->setHighlightedRow( popup()->tableView()->rows()-1 );
			updateTextForHighlightedRow();
			return true;
	case Qt::Key_Enter:
	case Qt::Key_Return: //accept
			//select row that is highlighted
			if (popup()->tableView()->highlightedRow()>=0)
				popup()->tableView()->selectRow( popup()->tableView()->highlightedRow() );
			//do not return true: allow to process event
	default: ;
	}
	return false;
}

void KexiComboBoxBase::updateTextForHighlightedRow()
{
	KexiTableItem *item = popup() ? popup()->tableView()->highlightedItem() : 0;
	if (item)
		slotItemSelected(item);
}

void KexiComboBoxBase::undoChanges()
{
	KexiDB::LookupFieldSchema *lookupFieldSchema = this->lookupFieldSchema();
	if (lookupFieldSchema) {
//		kexidbg << "KexiComboBoxBase::undoChanges(): m_visibleValue BEFORE = " << m_visibleValue << endl;
		if (popup())
			popup()->tableView()->selectRow( popup()->tableView()->highlightedRow() );
		m_visibleValue = visibleValueForLookupField();
//		kexidbg << "KexiComboBoxBase::undoChanges(): m_visibleValue AFTER = " << m_visibleValue << endl;
		setValueOrTextInInternalEditor( m_visibleValue );
	}
}
