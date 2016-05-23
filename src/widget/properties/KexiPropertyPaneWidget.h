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

#include <QWidget>

class KPropertySet;
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

    void updateInfoLabelForPropertySet(KPropertySet* set);

protected Q_SLOTS:
    //! Update information about selected object
    void slotPropertySetChanged(KPropertySet* set);

private:
    class Private;
    Private * const d;
};

#endif
