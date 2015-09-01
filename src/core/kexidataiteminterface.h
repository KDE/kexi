/* This file is part of the KDE project
   Copyright (C) 2005-2012 Jarosław Staniek <staniek@kde.org>

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

#include <QVariant>
#include <QWidget>

#include "kexicore_export.h"

class KexiDataItemInterface;
class KDbField;
class KDbQueryColumnInfo;

//! An helper class used to react on KexiDataItemInterface objects' changes.
class KEXICORE_EXPORT KexiDataItemChangesListener
{
public:
    KexiDataItemChangesListener();
    virtual ~KexiDataItemChangesListener();

    /*! Implement this to react for change of \a item.
     Called by KexiDataItemInterface::valueChanged() */
    virtual void valueChanged(KexiDataItemInterface* item) = 0;

    /*! Implement this to return information whether we're currently at new record or not.
     This can be used e.g. by data-aware widgets to determine if "(auto)"
     label should be displayed. */
    virtual bool cursorAtNewRecord() const = 0;

    /*! Implement this to react when length of data has been exceeded. */
    virtual void lengthExceeded(KexiDataItemInterface *item, bool lengthExceeded) = 0;

    /*! Implement this to react when "length of data has been exceeded" message need updating. */
    virtual void updateLengthExceededMessage(KexiDataItemInterface *item) = 0;
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

     \a visibleValue (if not NULL) is passed to provide visible value to display instead of \a value.
     This is currently used only in case of the combo box form widget, where displayed content
     (usually a text of image) differs from the value of the widget (a numeric index).

     This method is called by table view's and form's editors. */
    void setValue(const QVariant& value, const QVariant& add = QVariant(), bool removeOld = false,
                  const QVariant* visibleValue = 0);

    //! \return field information for this item
    virtual KDbField *field() const = 0;

    //! \return column information for this item
    virtual KDbQueryColumnInfo* columnInfo() const = 0;

    //! Used internally to set column information.
    virtual void setColumnInfo(KDbQueryColumnInfo* cinfo) = 0;

    //! Sets listener. No need to reimplement this.
    virtual void installListener(KexiDataItemChangesListener* listener);

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
    //! (use KDbField::hasEmptyProperty() to check this).
    //! Used for checking if a given constraint within table of form is met.
    virtual bool valueIsEmpty() = 0;

    //! \return value that should be displayed for this item.
    //! Only used for items like combo box, where real value is an integer while
    //! displayed value is usually a text. For other item types this method should be empty.
    virtual QVariant visibleValue();

    /*! \return 'readOnly' flag for this item. The flag is usually taken from
     the item's widget, e.g. QLineEdit::isReadOnly().
     By default, always returns false. */
    virtual bool isReadOnly() const;

    /*! \return the view widget of this item, e.g. line edit widget. */
    virtual QWidget* widget() = 0;

    /*! Hides item's widget, if available. */
    virtual void hideWidget();

    /*! Shows item's widget, if available. */
    virtual void showWidget();

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

    /*! Moves cursor after the last character (or element).
     For implementation in items supporting text cursor's movement; by default does nothing. */
    virtual void moveCursorToEnd() {}

    /*! Moves cursor before the first character (or element).
     For implementation in items supporting text cursor's movement; by default does nothing. */
    virtual void moveCursorToStart() {}

    /*! Selects all characters (or elements) of the item.
     For implementation in items supporting text or elements; by default does nothing. */
    virtual void selectAll() {}

    //! clears item's data, so the data will contain NULL data
    virtual void clear() = 0;

    /*! \return true if this editor offers a widget (e.g. line edit) that we can move focus to.
     Editor for boolean values has this set to false (see KexiBoolTableEdit).
     This is true by default. You can override this flag by changing
     hasFocusableWidget in your subclass' constructor. */
    bool hasFocusableWidget() const;

    void setHasFocusableWidget(bool set) const;

    /*! Displays additional elements that are needed for indicating that the current cell
     is selected. For example, combobox editor (KexiComboBoxTableEdit) moves and shows
     dropdown button. \a r is the rectangle for the cell.
     If \a readOnly is true, additional elements should be visually disabled,
     e.g. dropdown button of the combobox editor should be disabled.
     For reimplementation. By default does nothing. */
    virtual void showFocus(const QRect& r, bool readOnly);

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
    bool acceptEditorAfterDeleteContents() const;

    void setAcceptEditorAfterDeleteContents(bool set) const;

    virtual void setFocus();

    bool cursorAtNewRecord();

    /*! Sets a pointer to a Parent Data Item Interface. This pointer is 0 by default,
     but can be set by parent widget if this interface is a building block of a larger data widget.
     It is the case for KexiDBFieldEdit widget (see KexiDBFieldEdit::createEditor()). Use with care.
     signalValueChanged() method will check this pointer, and if it's not 0,
     parentDataItemInterface()->signalValueChanged() is called, so a changes can be signalled at higher level. */
    void setParentDataItemInterface(KexiDataItemInterface* parentDataItemInterface);

    /*! \return a pointer to a Parent Data Item Interface.
     @see setParentDataItemInterface() */
    KexiDataItemInterface* parentDataItemInterface() const;

    /*! Handles action having standard name \a actionName.
     Action could be: "edit_cut", "edit_paste", etc.
     For reimplementation. */
    virtual void handleAction(const QString& actionName);

    virtual bool isComboBox() const;

    virtual QWidget* internalEditor() const;

    //! Called (e.g. by KexiDataItemInterface) to change invalid value so it is valid.
    //! @return true on successful fixing
    //! Default implementation just returns true.
    virtual bool fixup();

    bool lengthExceededEmittedAtPreviousChange() const;

    void setLengthExceededEmittedAtPreviousChange(bool set);

protected:
    /*! Initializes this editor with \a add value, which should be somewhat added to the current
     value (see originalValue()).
     If \a removeOld is true, a value should be set to \a add, otherwise
     -it should be set to current \a originalValue() + \a add, if possible.
     Implement this. */
    virtual void setValueInternal(const QVariant& add, bool removeOld) = 0;

    QVariant originalValue() const;

    /*! Initializes this editor with \a value visible value.
     This is currently used only in case of the combo box form widget, where displayed content
     (usually a text of image) differs from the value of the widget (a numeric index).
     For implementation in the combo box widget, by default does nothing. */
    virtual void setVisibleValueInternal(const QVariant& value);

    /*! Call this in your implementation when value changes,
     so installed listener can react on this change. If there is a parent data item defined
     (see setParentDataItemInterface()), parent's signalValueChanged() method will be called instead. */
    virtual void signalValueChanged();

    /*! Used to perform some actions before signalValueChanged() call.
     We need this because the interface is not QObject and thus has got no real signals.
     Used in KexiDBComboBox. */
    virtual void beforeSignalValueChanged() {}

    /*! Used to indicate that length of data has been exceeded. */
    virtual void signalLengthExceeded(bool lengthExceeded);

    /*! Used to request update for "length has been exceeded" message. */
    virtual void signalUpdateLengthExceededMessage();

    /*! Emits request for showing or hiding the "Length exceeded" warning, if needed. */
    void emitLengthExceededIfNeeded(bool lengthExceeded);

    KexiDataItemChangesListener* listener();

    void setFocusableWidget(bool set);

    class Private;
    Private* const d;
};

#endif
