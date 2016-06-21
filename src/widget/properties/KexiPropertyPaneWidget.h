/* This file is part of the KDE project
   Copyright (C) 2016 Jarosław Staniek <staniek@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef KEXIPROPERTYPANEWIDGET_H
#define KEXIPROPERTYPANEWIDGET_H

#include "kexiextwidgets_export.h"

#include <KPropertyEditorView>

#include <QWidget>

class KPropertyEditorView;

//! @short A widget handling entire Property Pane
class KEXIEXTWIDGETS_EXPORT KexiPropertyPaneWidget : public QWidget
{
    Q_OBJECT
public:
    explicit KexiPropertyPaneWidget(QWidget *parent = 0);

    virtual ~KexiPropertyPaneWidget();

    KPropertyEditorView *editor() const;

    void addSection(QWidget *widget, const QString &title);

    //! Removes all sections added by addSection() from the pane's layout and hide them.
    //! Does not delete the sections; they should be owned by parts and can be reused later.
    //! Used by the main window when pane should be reset.
    void removeAllSections();

    /*! Changes property set to @a set.
      If @a set is @c nullptr and @a textToDisplayForNullSet string is not empty, this
      string is displayed (without icon or any other additional part).
      If not specified, "No object selected" message is used. */
    void changePropertySet(KPropertySet* set,
                           const QByteArray& propertyToSelect = QByteArray(),
                           KPropertyEditorView::SetOptions options = KPropertyEditorView::SetOption::None,
                           const QString& textToDisplayForNullSet = QString());

    /*! Updates info label of the property editor by reusing properties provided
     by the current property set.

     Following internal properties in @a set can customize displaying this information:
     - "this:classString" property of type string describes object's class name
     - "this:iconName" property of type string describes class' icon, if missing, the icon
        will be hidden
     - "objectName" or "caption" property of type string describes object's name
     - "this:visibleObjectNameProperty" property of type string specified name of property
        that contains "object name" to display; this can be usable when we know that e.g.
        "caption" property is available for a given type of objects and is better to use
        (this is the case for Table Designer fields); if missing, "objectName" property is used
     - "this:objectNameReadOnly" property of type boolean makes the object name box
        read-only for the user; false by default

     If object's class and name is empty, the entire info label widget becomes hidden.
     If @a set is @c nullptr, the property editor (editor()) becomes hidden.

     @see KexiMainWindow::updatePropertyEditorInfoLabel() */
    void updateInfoLabelForPropertySet(const QString& textToDisplayForNullSet);

protected Q_SLOTS:
    //! Possible name change accepted by the user
    void slotObjectNameChangeAccepted();

private:
    class Private;
    Private * const d;
};

#endif
