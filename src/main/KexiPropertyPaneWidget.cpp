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

#include "KexiPropertyPaneWidget.h"
#include <KexiPropertyEditorView.h>

#include <KLocalizedString>

#include <QToolBox>
#include <QVBoxLayout>

class KexiPropertyPaneWidget::Private
{
public:
    Private() {}
    QVBoxLayout *mainLyr;
    QToolBox *toolBox;
    KexiPropertyEditorView *editor;
};

KexiPropertyPaneWidget::KexiPropertyPaneWidget(QWidget *parent)
 : QWidget(parent), d(new Private)
{
    d->mainLyr = new QVBoxLayout(this);
    d->mainLyr->setContentsMargins(0, 0, 0, 0);
    d->toolBox = new QToolBox;
    d->mainLyr->addWidget(d->toolBox);
    
    d->editor = new KexiPropertyEditorView(this);
    d->toolBox->addItem(d->editor, d->editor->windowTitle());
    setWindowTitle(d->editor->windowTitle());
}

KexiPropertyPaneWidget::~KexiPropertyPaneWidget()
{
    delete d;
}

KexiPropertyEditorView* KexiPropertyPaneWidget::editor() const
{
    return d->editor;
}

QToolBox* KexiPropertyPaneWidget::toolBox() const
{
    return d->toolBox;
}
