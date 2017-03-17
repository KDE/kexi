/* This file is part of the KDE project
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2004-2016 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIPROPERTYEDITORVIEW_H
#define KEXIPROPERTYEDITORVIEW_H

#include "kexiextwidgets_export.h"

#include <KPropertyEditorView>

//! @short The container (acts as a dock window) for KexiPropertyEditor.
/*! The widget displays KexiObjectInfoWidget on its top, to show user what
 object the properties belong to. Read the KexiObjectInfoWidget documentation for
 the description what information is displayed.

 There are properties obtained from KexiMainWindow's current property set
 that help to customize displaying this information:
 - "this:classString property" of type string describes object's class name
 - "this:iconName" property of type string describes class name
 - "name" or "caption" property of type string describes object's name
 - "this:useCaptionAsObjectName" property of type boolean forces displaying "caption"
   property instead of "name" - this can be usable when we know that "caption" properties
   are available for a given type of objects (this is the case for Table Designer fields)
*/
class KEXIEXTWIDGETS_EXPORT KexiPropertyEditorView : public QWidget
{
    Q_OBJECT

public:
    explicit KexiPropertyEditorView(QWidget* parent);
    virtual ~KexiPropertyEditorView();

    QSize sizeHint() const Q_DECL_OVERRIDE;
    QSize minimumSizeHint() const Q_DECL_OVERRIDE;
    KPropertyEditorView *editor() const;

protected Q_SLOTS:
    //! Update information about selected object
    void slotPropertySetChanged(KPropertySet* set);

protected:
    class Private;
    Private * const d;
};

#endif
