/* This file is part of the KDE project
   Copyright (C) 2005 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2004-2012 Jarosław Staniek <staniek@kde.org>
   Copyright (C) 2014 Wojciech Kosowicz <pcellix@gmail.com>

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

#ifndef KexiDBTextEdit_H
#define KexiDBTextEdit_H

#include <widget/dataviewcommon/kexiformdataiteminterface.h>
#include "kexidbtextwidgetinterface.h"
#include "kexidbutils.h"
#include <formeditor/FormWidgetInterface.h>
#include <ktextedit.h>
#include <QPaintEvent>

class DataSourceLabel;

//! @short Multiline edit widget for Kexi forms
class KEXIFORMUTILS_EXPORT KexiDBTextEdit :  public KTextEdit,
                                             protected KexiDBTextWidgetInterface,
                                             public KexiFormDataItemInterface,
                                             public KFormDesigner::FormWidgetInterface
{
    Q_OBJECT
    Q_PROPERTY(QString dataSource READ dataSource WRITE setDataSource)
    Q_PROPERTY(QString dataSourcePartClass READ dataSourcePartClass WRITE setDataSourcePartClass)

public:
    explicit KexiDBTextEdit(QWidget *parent);
    virtual ~KexiDBTextEdit();

    inline QString dataSource() const {
        return KexiFormDataItemInterface::dataSource();
    }
    inline QString dataSourcePartClass() const {
        return KexiFormDataItemInterface::dataSourcePartClass();
    }
    virtual QVariant value();
    virtual void setInvalidState(const QString& displayText);

    //! \return true if editor's value is null (not empty)
    //! Used for checking if a given constraint within table of form is met.
    virtual bool valueIsNull();

    //! \return true if editor's value is empty (not necessary null).
    //! Only few data types can accept "EMPTY" property
    //! (use KDbField::hasEmptyProperty() to check this).
    //! Used for checking if a given constraint within table or form is met.
    virtual bool valueIsEmpty();

    /*! \return 'readOnly' flag for this widget. */
    virtual bool isReadOnly() const;

    /*! \return the view widget of this item, e.g. line edit widget. */
    virtual QWidget* widget();

    virtual bool cursorAtStart();
    virtual bool cursorAtEnd();
    virtual void clear();

    virtual void setColumnInfo(KDbQueryColumnInfo* cinfo);

    /*! If \a displayDefaultValue is true, the value set by KexiDataItemInterface::setValue()
     is displayed in a special way. Used by KexiFormDataProvider::fillDataItems().
     \a widget is equal to 'this'.
     Reimplemented after KexiFormDataItemInterface. */
    virtual void setDisplayDefaultValue(QWidget* widget, bool displayDefaultValue);

    //! Windows uses Ctrl+Tab for moving between tabs, so do not steal this shortcut
    virtual void keyPressEvent(QKeyEvent *ke);

    virtual bool event(QEvent *e);

    //! Selects contents of the widget if there is such behaviour set (it is by default).
//! @todo add option for not selecting the field
    virtual void selectAllOnFocusIfNeeded();

public Q_SLOTS:
    void setDataSource(const QString &ds);

    void setDataSourcePartClass(const QString &partClass);

    virtual void setReadOnly(bool readOnly);

    //! Reimplemented, so "undo" means the same as "cancelEditor" action
//! @todo enable "real" undo internally so user can use ctrl+z while editing
    virtual void undo();

    //! Implemented for KexiDataItemInterface
    virtual void moveCursorToEnd();

    //! Implemented for KexiDataItemInterface
    virtual void moveCursorToStart();

    //! Implemented for KexiDataItemInterface
    virtual void selectAll();

protected Q_SLOTS:
    void slotTextChanged();

protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void contextMenuEvent(QContextMenuEvent *e);
    virtual void changeEvent(QEvent *e);
    virtual void setValueInternal(const QVariant& add, bool removeOld);
    virtual void focusOutEvent(QFocusEvent *e);
    QMenu * createPopupMenu(const QPoint & pos);
    void updateTextForDataSource();
    void createDataSourceLabel();
    void updatePalette();

private:
    //! Used for extending context menu
    KexiDBWidgetContextMenuExtender m_menuExtender;

    //! Used to disable slotTextChanged()
    bool m_slotTextChanged_enabled;

    DataSourceLabel *m_dataSourceLabel;

    //! Text length allowed
    uint m_length;

    QPalette m_originalPalette; //!< Used for read-only case
    bool m_paletteChangeEvent_enabled;
};

#endif
