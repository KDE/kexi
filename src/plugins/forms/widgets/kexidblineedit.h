/* This file is part of the KDE project
   Copyright (C) 2005 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2004-2014 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIDBLINEEDIT_H
#define KEXIDBLINEEDIT_H

#include <QValidator>
#include <QEvent>
#include <QPaintEvent>
#include <QLineEdit>
#include <QPointer>

#include <widget/dataviewcommon/kexiformdataiteminterface.h>
#include "kexidbtextwidgetinterface.h"
#include "kexidbutils.h"
#include <kexi_global.h>
#include <widget/tableview/kexitextformatter.h>
#include <formeditor/FormWidgetInterface.h>

class KexiDBWidgetContextMenuExtender;
class KexiDBLineEditStyle;

//! @short Line edit widget for Kexi forms
/*! Handles many data types. User input is validated by using validators
 and/or input masks.
*/
class KEXIFORMUTILS_EXPORT KexiDBLineEdit : public QLineEdit,
                                            protected KexiDBTextWidgetInterface,
                                            public KexiFormDataItemInterface,
                                            public KexiSubwidgetInterface,
                                            public KFormDesigner::FormWidgetInterface
{
    Q_OBJECT
    Q_PROPERTY(QString dataSource READ dataSource WRITE setDataSource)
    Q_PROPERTY(QString dataSourcePartClass READ dataSourcePluginId WRITE setDataSourcePluginId)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)
    Q_PROPERTY(QString clickMessage READ placeholderText
                                    WRITE setPlaceholderText) // Internal, equivalent of placeholderText
                                                              // For backward compatibility Kexi projects
                                                              // created with Qt < 4.7.
    Q_PROPERTY(bool showClearButton READ isClearButtonEnabled
                                    WRITE setClearButtonEnabled) // Internal, equivalent of clearButtonEnabled
                                                                 // For backward compatibility Kexi projects
                                                                 // created with Qt 4.
public:
    explicit KexiDBLineEdit(QWidget *parent);
    virtual ~KexiDBLineEdit();

    inline QString dataSource() const {
        return KexiFormDataItemInterface::dataSource();
    }
    inline QString dataSourcePluginId() const {
        return KexiFormDataItemInterface::dataSourcePluginId();
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

    /*! \return true if the value is valid */
    virtual bool valueIsValid();

    /*! \return 'readOnly' flag for this widget. */
    virtual bool isReadOnly() const;

    /*! If \a displayDefaultValue is true, the value set by KexiDataItemInterface::setValue()
     is displayed in a special way. Used by KexiFormDataProvider::fillDataItems().
     \a widget is equal to 'this'.
     Reimplemented after KexiFormDataItemInterface. */
    virtual void setDisplayDefaultValue(QWidget* widget, bool displayDefaultValue);

    /*! \return the view widget of this item, e.g. line edit widget. */
    virtual QWidget* widget();

    virtual bool cursorAtStart();
    virtual bool cursorAtEnd();
    virtual void clear();

    virtual void setColumnInfo(KDbQueryColumnInfo* cinfo);

    /*! Handles action having standard name \a actionName.
     Action could be: "edit_copy", "edit_paste", etc.
     Reimplemented after KexiDataItemInterface. */
    virtual void handleAction(const QString& actionName);

    /*! Called by top-level form on key press event to consume widget-specific shortcuts. */
    virtual bool keyPressed(QKeyEvent *ke);

    //! Used when read only flag is true
    QString originalText() const { return m_originalText; }

    //! Used when read only flag is true
    int originalCursorPosition() const;

public Q_SLOTS:
    void setDataSource(const QString &ds);

    void setDataSourcePluginId(const QString &pluginId);

    virtual void setReadOnly(bool readOnly);

    //! Reimplemented, so "undo" means the same as "cancelEditor" action
    virtual void undo();

    //! Implemented for KexiDataItemInterface
    virtual void moveCursorToEnd();

    //! Implemented for KexiDataItemInterface
    virtual void moveCursorToStart();

    //! Implemented for KexiDataItemInterface
    virtual void selectAll();

    //! Implemented for KexiDataItemInterface
    virtual bool fixup();

protected Q_SLOTS:
    void slotTextChanged(const QString&);

    void slotTextEdited(const QString& text);

    void slotCursorPositionChanged(int oldPos, int newPos);

    //! Used to protect m_readWriteValidator against after validator is destroyed
    void slotReadWriteValidatorDestroyed(QObject*);

protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void setValueInternal(const QVariant& add, bool removeOld);
    virtual bool event(QEvent *);
    virtual void contextMenuEvent(QContextMenuEvent *e);
    virtual void changeEvent(QEvent *e);

    //! Implemented for KexiSubwidgetInterface
    virtual bool appendStretchRequired(KexiDBAutoField* autoField) const;

    void updateTextForDataSource();

    void updatePalette();

    //! Used to format text
    KexiTextFormatter m_textFormatter;

    //! Used for read only flag to disable editing
    QPointer<QValidator> m_readOnlyValidator;

    //! Used to remember the previous validator used for r/w mode, after setting
    //! the read only flag
    const QValidator* m_readWriteValidator;

    //! Used for extending context menu
    KexiDBWidgetContextMenuExtender m_menuExtender;

    //! Used in isReadOnly, as sometimes we want to have the flag set tot true when QLineEdit::isReadOnly
    //! is still false.
    bool m_internalReadOnly;

    //! Used in slotTextChanged()
    bool m_slotTextChanged_enabled;

    QString m_originalText;
    int m_cursorPosition;
    QPalette m_originalPalette; //!< Used for read-only case
    bool m_paletteChangeEvent_enabled;
    bool m_inStyleChangeEvent;
    QPointer<KexiDBLineEditStyle> m_internalStyle;
};

#endif
