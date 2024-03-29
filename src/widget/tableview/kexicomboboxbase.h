/* This file is part of the KDE project
   Copyright (C) 2002 Peter Simonsson <psn@linux.se>
   Copyright (C) 2003-2016 Jarosław Staniek <staniek@kde.org>

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

#ifndef _KEXICOMBOBOXBASE_H_
#define _KEXICOMBOBOXBASE_H_

#include "kexidatatable_export.h"
#include "kexiinputtableedit.h"

#include <KDbField>
#include <KDbLookupFieldSchema>
#include <KDbTristate>

class KDbTableViewColumn;
class KexiComboBoxPopup;

/*! @short A base class for handling data-aware combo boxes.
 This class is used by KexiComboBoxTableEdit and KexiDBComboBox.
*/
class KEXIDATATABLE_EXPORT KexiComboBoxBase
{
public:
    KexiComboBoxBase();
    virtual ~KexiComboBoxBase();

    //! \return column related to this combo; for KexiComboBoxTableEdit 0 is returned here
    virtual KDbTableViewColumn *column() = 0;

    //! @overload
    const KDbTableViewColumn *column() const;

    //! \return database field related to this combo
    virtual KDbField *field() = 0;

    //! \return the original value
    virtual QVariant origValue() const = 0;

    //! Note: Generally in current implementation this is integer > 0; may be null if no value is set
    virtual QVariant value();

    virtual QVariant visibleValue();

    //! Reimplement this and call this impl.: used to clear internal editor
    virtual void clear();

    virtual tristate valueChangedInternal();
    virtual bool valueIsNull();
    virtual bool valueIsEmpty();

    virtual void hide();

    void showPopup();

    //! Call this from slot
    virtual void slotRecordAccepted(KDbRecordData *data, int record);

    //! Call this from slot
    virtual void slotRecordSelected(KDbRecordData* data);

    //! Call this from slot
    void slotInternalEditorValueChanged(const QVariant &v);

    //! Implement this to return the internal editor
    virtual QWidget *internalEditor() const = 0;

protected:
    //! @return connection for this combo
    virtual KDbConnection *connection() = 0;

    void createPopup(bool show);

    virtual void setValueInternal(const QVariant& add, bool removeOld);

    //! Used to select record item for a user-entered value \a v.
    //! Only for "lookup table" mode.
    KDbRecordData* selectRecordForEnteredValueInLookupTable(const QVariant& v);

    /*! \return value from \a returnFromColumn related to \a str value from column \a lookInColumn.
     If \a allowNulls is true, NULL is returned if no matched column found, else:
     \a str is returned.
     Example: lookInColumn=0, returnFromColumn=1 --returns user-visible string
     for column #1 for id-column #0 */
    QString valueForString(const QString& str, int* record, int lookInColumn,
                           int returnFromColumn, bool allowNulls = false);

    //! sets \a value for the line edit without setting a flag (m_userEnteredValue) that indicates that
    //! the text has been entered by hand (by a user)
    void setValueOrTextInInternalEditor(const QVariant& value);

    //! \return lookup field schema for this combo box, if present and if is valid (i.e. has defined record source)
    KDbLookupFieldSchema* lookupFieldSchema();

    //! override
    const KDbLookupFieldSchema* lookupFieldSchema() const;

    int recordToHighlightForLookupTable() const;

    //! Implement this to perform "move cursor to end" in the internal editor
    virtual void moveCursorToEndInInternalEditor() = 0;

    //! Implement this to perform "select all" in the internal editor
    virtual void selectAllInInternalEditor() = 0;

    //! Implement this to perform "set value" in the internal editor
    virtual void setValueInInternalEditor(const QVariant& value) = 0;

    //! Implement this to return value from the internal editor
    virtual QVariant valueFromInternalEditor() = 0;

    //! Implement this as signal
    virtual void editRequested() = 0;

    //! Implement this as signal
    virtual void acceptRequested() = 0;

    //! Implement this to return a position \a pos mapped from parent (e.g. viewport)
    //! to global coordinates. QPoint(-1, -1) should be returned if this cannot be computed.
    virtual QPoint mapFromParentToGlobal(const QPoint& pos) const = 0;

    //! Implement this to return a hint for popup width.
    virtual int popupWidthHint() const = 0;

    //! Implement this to update button state. Table view just updates on/off state
    //! for the button depending on visibility of the popup
    virtual void updateButton() {}

    virtual KexiComboBoxPopup *popup() const = 0;
    virtual void setPopup(KexiComboBoxPopup *popup) = 0;

    virtual QVariant visibleValueForLookupField();

    void updateTextForHighlightedRecord();

    bool handleKeyPressForPopup(QKeyEvent *ke);

    void acceptPopupSelection();

    //! Used by KexiDBComboBox.
    void undoChanges();

    //! \return index of bound column.
    int boundColumnIndex() const;

    //! \return index of (actually, first as this is the current limitation) visible column.
    int visibleColumnIndex() const;

    //! A hack for createPopup(), used by forms only. Avoid magical disappearing of the popup in forms after 2nd and subsequent use.
    //! fix creating popup for forms instead!
    bool m_reinstantiatePopupOnShow;

    QVariant m_visibleValue;

    QVariant m_userEnteredValue; //!< value (usually a text) entered by hand (by the user)

    bool m_internalEditorValueChanged; //!< true if user has text or other value inside editor
    bool m_slotInternalEditorValueChanged_enabled; //!< Used in slotInternalEditorValueChanged()
    bool m_setValueOrTextInInternalEditor_enabled; //!< Used in setValueOrTextInInternalEditor() and slotItemSelected()
    bool m_mouseBtnPressedWhenPopupVisible; //!< Used only by KexiComboBoxTableEdit
    bool m_insideCreatePopup; //!< true if we're inside createPopup(); used in slotItemSelected()
    //! Set to false as soon as the item corresponding with the current
    //! value is selected in the popup table. This avoids selecting item
    //! for origValue() and thus loosing the recent choice.
    bool m_updatePopupSelectionOnShow;
    bool m_moveCursorToEndInInternalEditor_enabled;
    bool m_selectAllInInternalEditor_enabled;
    bool m_setValueInInternalEditor_enabled;
    //! Used in setValueInternal() to control whether
    //! we want to set visible value on setValueInternal()
    //! - true for table view's combo box
    bool m_setVisibleValueOnSetValueInternal;
    //! Checked in createPopup(), true for form's combo box, so the popup is focused before showing;
    //! false for table view's combo box, so the popup is focused after showing.
    //! False by default.
    bool m_focusPopupBeforeShow;
};

#endif
