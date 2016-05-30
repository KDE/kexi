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

#include "KexiPropertyPaneLineEdit.h"
#include <KexiStyle.h>

#include <KColorScheme>
#include <KColorUtils>

#include <QKeyEvent>
#include <QProxyStyle>
#include <QStyleOption>

class KexiPropertyPaneLineEdit::Private
{
public:
    Private(KexiPropertyPaneLineEdit *qq) : q(qq) {}
    KexiPropertyPaneLineEdit * const q;
};

KexiPropertyPaneLineEdit::KexiPropertyPaneLineEdit(QWidget* parent)
        : QLineEdit(parent)
        , d(new Private(this))
{
    KexiStyle::propertyPane().alterLineEditStyle(this);
}

KexiPropertyPaneLineEdit::~KexiPropertyPaneLineEdit()
{
    delete d;
}

void KexiPropertyPaneLineEdit::keyPressEvent(QKeyEvent *event)
{
    QLineEdit::keyPressEvent(event);
    if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        emit enterPressed();
    }
}

void KexiPropertyPaneLineEdit::focusOutEvent(QFocusEvent *event)
{
    emit focusOut();
    QLineEdit::focusOutEvent(event);
}
