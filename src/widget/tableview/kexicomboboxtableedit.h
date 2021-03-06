/* This file is part of the KDE project
   Copyright (C) 2002   Peter Simonsson <psn@linux.se>
   Copyright (C) 2003-2006 Jarosław Staniek <staniek@kde.org>

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

#ifndef _KEXICOMBOBOXTABLEEDIT_H_
#define _KEXICOMBOBOXTABLEEDIT_H_

#include "kexiinputtableedit.h"
#include "kexicomboboxbase.h"
#include <KDbField>
#include <KDbLookupFieldSchema>
#include <KDbTableViewColumn>

#include <QKeyEvent>
#include <QEvent>

class KDbTableViewColumn;
class KexiComboBoxPopup;

/*! @short Drop-down cell editor.
*/
class KexiComboBoxTableEdit : public KexiInputTableEdit, virtual public KexiComboBoxBase
{
    Q_OBJECT

public:
    explicit KexiComboBoxTableEdit(KDbTableViewColumn *column, QWidget *parent = 0);
    virtual ~KexiComboBoxTableEdit();

    //! Implemented for KexiComboBoxBase
    KDbTableViewColumn *column() override {
        return m_column;
    }

    //! Implemented for KexiComboBoxBase
    KDbField *field() override {
        return m_column->field();
    }

    //! Implemented for KexiComboBoxBase
    virtual QVariant origValue() const override;

    virtual void setValueInternal(const QVariant& add, bool removeOld) override {
        KexiComboBoxBase::setValueInternal(add, removeOld);
    }

    virtual QVariant value() override {
        return KexiComboBoxBase::value();
    }

    virtual void clear() override;

    virtual bool valueChanged() override;

    virtual QVariant visibleValue() override;

    /*! Reimplemented: resizes a view(). */
    virtual void resize(int w, int h) override;

    virtual void showFocus(const QRect& r, bool readOnly) override;

    virtual void hideFocus() override;

    virtual void paintFocusBorders(QPainter *p, QVariant &cal, int x, int y, int w, int h) override;

    /*! Setups contents of the cell. As a special case, if there is lookup field schema
     defined, \a val already contains the visible value (usually the text)
     set by \ref KexiTableView::paintcell(), so there is noo need to lookup the value
     in the combo box's popup. */
    virtual void setupContents(QPainter *p, bool focused, const QVariant& val,
                               QString &txt, int &align, int &x, int &y_offset, int &w, int &h) override;

    /*! Used to handle key press events for the item. */
    virtual bool handleKeyPress(QKeyEvent *ke, bool editorActive) override;

    virtual int widthForValue(const QVariant &val, const QFontMetrics &fm) override;

    virtual void hide() override;
    virtual void show();

    /*! \return total size of this editor, including popup button. */
    virtual QSize totalSize() const override;

    virtual void createInternalEditor(KDbConnection *conn, const KDbQuerySchema& schema) override;

    /*! Reimplemented after KexiInputTableEdit. */
    virtual void handleAction(const QString& actionName) override;

    /*! Reimplemented after KexiInputTableEdit.
     For a special case (combo box), \a visibleValue can be provided,
     so it can be copied to the clipboard instead of unreadable \a value. */
    virtual void handleCopyAction(const QVariant& value, const QVariant& visibleValue) override;

public Q_SLOTS:
    //! Implemented for KexiDataItemInterface
    virtual void moveCursorToEnd() override;

    //! Implemented for KexiDataItemInterface
    virtual void moveCursorToStart() override;

    //! Implemented for KexiDataItemInterface
    virtual void selectAll() override;

protected Q_SLOTS:
    void slotButtonClicked();
    void slotRecordAccepted(KDbRecordData *data, int record) override {
        KexiComboBoxBase::slotRecordAccepted(data, record);
    }
    void slotRecordSelected(KDbRecordData* data) override {
        KexiComboBoxBase::slotRecordSelected(data);
    }
    void slotInternalEditorValueChanged(const QVariant& v) {
        KexiComboBoxBase::slotInternalEditorValueChanged(v);
    }
    void slotLineEditTextChanged(const QString& s);
    void slotPopupHidden();

protected:
    //! Implemented for KexiComboBoxBase
    KDbConnection *connection() override;

    //! internal
    void updateFocus(const QRect& r);

    virtual bool eventFilter(QObject *o, QEvent *e) override;

    //! Implemented for KexiComboBoxBase
    virtual QWidget *internalEditor() const override;

    //! Implemented for KexiComboBoxBase
    virtual void moveCursorToEndInInternalEditor() override;

    //! Implemented for KexiComboBoxBase
    virtual void selectAllInInternalEditor() override;

    //! Implemented for KexiComboBoxBase
    virtual void setValueInInternalEditor(const QVariant& value) override;

    //! Implemented for KexiComboBoxBase
    virtual QVariant valueFromInternalEditor() override;

    //! Implemented for KexiComboBoxBase
    virtual void editRequested() override {
        KexiInputTableEdit::editRequested();
    }

    //! Implemented for KexiComboBoxBase
    virtual void acceptRequested() override {
        KexiInputTableEdit::acceptRequested();
    }

    //! Implemented for KexiComboBoxBase
    virtual QPoint mapFromParentToGlobal(const QPoint& pos) const override;

    //! Implemented for KexiComboBoxBase
    virtual int popupWidthHint() const override;

    //! Implemented this to update button state.
    virtual void updateButton() override;

    virtual KexiComboBoxPopup *popup() const override;
    virtual void setPopup(KexiComboBoxPopup *popup) override;

    class Private;
    Private * const d;
};

KEXI_DECLARE_CELLEDITOR_FACTORY_ITEM(KexiComboBoxEditorFactoryItem)

#endif
