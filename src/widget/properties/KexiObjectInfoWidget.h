/* This file is part of the KDE project
   Copyright (C) 2004-2009 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIOBJECTINFOWIDGET_H
#define KEXIOBJECTINFOWIDGET_H

#include "kexiextwidgets_export.h"

#include <QWidget>

//! @short A widget displaying small icon with class name and editable object's name
/*! The info widget is displayed in a form:
 <i>[ObjectClassIcon] ClassName [ObjectName]</i>

 The <i>ObjectClassIcon</i> is optional. If <i>ClassName</i> is empty, the information
 is displayed as:
 <i>[ObjectClassIcon] [ObjectName]</i>

 Example uses:
 - [button_icon] Button [quit]
 - [label_icon] Label [welcome]
*/
class KEXIEXTWIDGETS_EXPORT KexiObjectInfoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit KexiObjectInfoWidget(QWidget* parent = 0);
    virtual ~KexiObjectInfoWidget();

    void setObjectClassIconName(const QString &name);

    QString objectClassIconName() const;

    void setObjectClassName(const QString& name);

    QString objectClassName() const;

    void setObjectName(const QString& name);

    QString objectName() const;

    void setObjectVisible(bool set);

    void focusObjectNameBox();

    bool checkObjectName();

    //! If @a set is @c true, object name is assumed to be identifier (as in programming).
    //! Otherwise it can be any text.
    void setObjectNameIsIdentifier(bool set);

    //! @return true if object name is assumed to be identifier (as in programming).
    bool isObjectNameIdentifier() const;

Q_SIGNALS:
    //! Possible name change accepted by the user with Enter or Return key or after Focus Out
    void objectNameChangeAccepted();

protected Q_SLOTS:
    void slotObjectNameEnterPressed();

private:
    class Private;
    Private * const d;
};

#endif
