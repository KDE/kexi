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

#ifndef KEXIDATAITEMINTERFACE_H
#define KEXIDATAITEMINTERFACE_H

#include <qvariant.h>
#include <qwidget.h>
#include <qguardedptr.h>

class KexiDataItemInterface;
namespace KexiDB {
	class Field;
	class QueryColumnInfo;
}

//! An helper class used to react on KexiDataItemInterface objects' changes.
class KEXICORE_EXPORT KexiDataItemChangesListener
{
	public:
		KexiDataItemChangesListener();
		virtual ~KexiDataItemChangesListener();

		/*! Implement this to react for change of \a item.
		 Called by KexiDataItemInterface::valueChanged() */
		virtual void valueChanged(KexiDataItemInterface* item) = 0;

		/*! Implement this to return information whether we're currently at new row or now.
		 This can be used e.g. by data-aware widgets to determine if "(autonumber)" 
		 label should be displayed. */
		virtual bool cursorAtNewRow() = 0;
};

//! An interface for declaring widgets to be data-aware.
class KEXICORE_EXPORT KexiDataItemInterface
{
	public:
		KexiDataItemInterface();
		virtual ~KexiDataItemInterface();

		/*! Just initializes \a value, and calls setValueInternal(const QString& add, bool removeOld). 
		 If \a removeOld is true, current value is set up as \a add.
		 If \a removeOld if false, current value is set up as \a value + \a add.
		 \a value is stored as 'old value' -it'd be usable in the future
		 (e.g. Combo Box editor can use old value if current value does not 
		 match any item on the list).
		 Called by KexiTableView and others. */
		void setValue(const QVariant& value, const QVariant& add = QVariant(), bool removeOld = false);

		//! \return field information for this item
		virtual KexiDB::Field *field() const = 0;

		//! \return column information for this item
		virtual KexiDB::QueryColumnInfo* columnInfo() const = 0;

		//! Used internally to set column information.
		virtual void setColumnInfo(KexiDB::QueryColumnInfo* cinfo) = 0;

		//! Sets listener. No need to reimplement this.
		virtual void installListener(KexiDataItemChangesListener* listener);

//		//! Sets value \a value for a widget. 
//		//! Just calls setValueInternal(), but also blocks valueChanged() 
//		//! as you we don't want to react on our own change
//		void setValue(const QVariant& value);

		//! \return value currently represented by this item.
		virtual QVariant value() = 0;

		//! \return true if editor's value is valid for a given type
		//! Used for checking if an entered value is valid,
		//! E.g. a part of time value can be entered: "12:8" and this is invalid, not only null.
		//! Null time or date is valid in Kexi, so it is not enough to test value().isValid().
		//! Default implementation just returns true.
		virtual bool valueIsValid();

		//! \return true if editor's value is null (not empty)
		//! Used for checking if a given constraint within table or form is met.
		virtual bool valueIsNull() = 0;

		//! \return true if editor's value is empty (not necessary null). 
		//! Only few data types can accept "EMPTY" property 
		//! (use KexiDB::Field::hasEmptyProperty() to check this).
		//! Used for checking if a given constraint within table of form is met.
		virtual bool valueIsEmpty() = 0;

		/*! \return 'readOnly' flag for this item. The flag is usually taken from
		 the item's widget, e.g. KLineEdit::isReadOnly(). 
		 By default, always returns false. */
		virtual bool isReadOnly() const { return false; }

		/*! \return the view widget of this item, e.g. line edit widget. */
		virtual QWidget* widget() = 0;

		/*! Hides item's widget, if available. */
		virtual void hideWidget() { if (widget()) widget()->hide(); }

		/*! Shows item's widget, if available. */
		virtual void showWidget() { if (widget()) widget()->show(); }

		//! \return true if editor's value is changed (compared to original value)
		virtual bool valueChanged();

		/*! \return true if the item widget's cursor (whatever that means, eg. line edit cursor)
		 is at the beginning of editor's contents. This can inform table/form view that 
		 after pressing "left arrow" key should stop editing and move to a field on the left hand. */
		virtual bool cursorAtStart() = 0;

		/*! \return true if the item widget's cursor (whatever that means, eg. line edit cursor)
		 is at the end of editor's contents. This can inform table/form view that 
		 after pressing "right arrow" key should stop editing and move to a field on the right hand. */
		virtual bool cursorAtEnd() = 0;

		//! clears item's data, so the data will contain NULL data
		virtual void clear() = 0;

		/*! \return true if this editor offers a widget (e.g. line edit) that we can move focus to.
		 Editor for boolean values has this set to false (see KexiBoolTableEdit). 
		 This is true by default. You can override this flag by changing 
		 m_hasFocusableWidget in your subclass' constructor. */
		inline bool hasFocusableWidget() const { return m_hasFocusableWidget; }

		/*! Displays additional elements that are needed for indicating that the current cell
		 is selected. For example, combobox editor (KexiComboBoxTableEdit) moves and shows
		 dropdown button. \a r is the rectangle for the cell. 
		 If \a readOnly is true, additional elements should be visually disabled, 
		 e.g. dropdown button of the combobox editor should be disabled.
		 For reimplementation. By default does nothing. */
		virtual void showFocus( const QRect& r, bool readOnly );

		/*! Hides additional elements that are needed for indicating that the current cell
		 is selected. 
		 For reimplementation. By default does nothing. */
		virtual void hideFocus();

		/*! Allows to define reaction for clicking on cell's contents. 
		 Currently it's used for editor of type boolean, where we want to toggle true/false
		 on single mouse click. \sa hasFocusableWidget(), KexiBoolTableEdit. 
		 Default implementation does nothing. */
		virtual void clickedOnContents();

		/*! \return true if editing should be accepted immediately after
		 deleting contents for the cell (usually using Delete key).
		 This flag is false by default, and is true e.g. for date, time and datetime types. */
		bool acceptEditorAfterDeleteContents() const { return m_acceptEditorAfterDeleteContents; }

		inline virtual void setFocus() { if (widget()) widget()->setFocus(); }

		inline bool cursorAtNewRow() { return m_listener ? m_listener->cursorAtNewRow() : false; }

		/*! Sets a pointer to a Parent Data Item Interface. This pointer is 0 by default, 
		 but can be set by parent widget if this interface is a building block of a larger data widget. 
		 It is the case for KexiDBFieldEdit widget (see KexiDBFieldEdit::createEditor()). Use with care.
		 signalValueChanged() method will check this pointer, and if it's not 0, 
		 m_parentDataItemInterface->signalValueChanged() is called, so a changes can be signalled at higher level. */
		void setParentDataItemInterface(KexiDataItemInterface* parentDataItemInterface) {
			m_parentDataItemInterface = parentDataItemInterface;
		}

		/*! \return a pointer to a Parent Data Item Interface. 
		 @see setParentDataItemInterface() */
		inline KexiDataItemInterface* parentInterface() const { return m_parentDataItemInterface; }

	protected:
		/*! Initializes this editor with \a add value, which should be somewhat added to the current
		 value (already storted in m_origValue). 
		 If \a removeOld is true, a value should be set to \a add, otherwise 
		 -it should be set to current \a m_origValue + \a add, if possible.
		 Implement this. */
		virtual void setValueInternal(const QVariant& add, bool removeOld) = 0;

//		//! Sets value \a value for a widget. 
//		//! Implement this method to allow setting value for this widget item.
//		virtual void setValueInternal(const QVariant& value) = 0;

		/*! Call this in your implementation when value changes, 
		 so installed listener can react on this change. If there is a parent data item defined
		 (see setParentDataItemInterface()), parent's signalValueChanged() method will be called instead. */
		void signalValueChanged();

//moved to KexiFormDataItemInterface: QString m_dataSource;
		KexiDataItemChangesListener* m_listener;
		QVariant m_origValue;

		/*! @see parentDataItemInterface() */
		KexiDataItemInterface* m_parentDataItemInterface;
		bool m_hasFocusableWidget : 1;
		bool m_disable_signalValueChanged : 1;
		bool m_acceptEditorAfterDeleteContents : 1;
};

#endif
