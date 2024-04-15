/* This file is part of the KDE project
   Copyright (C) 2016 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIPROPERTYPANELINEEDIT_H
#define KEXIPROPERTYPANELINEEDIT_H

#include "kexiextwidgets_export.h"

#include <QLineEdit>

//! @short A line edit for use in the property pane
/*! The widget has modified look */
class KEXIEXTWIDGETS_EXPORT KexiPropertyPaneLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit KexiPropertyPaneLineEdit(QWidget* parent = nullptr);
    virtual ~KexiPropertyPaneLineEdit();

    void setReadOnly(bool set);

Q_SIGNALS:
    void enterPressed();
    void focusOut();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    class Private;
    Private * const d;
};

#endif
