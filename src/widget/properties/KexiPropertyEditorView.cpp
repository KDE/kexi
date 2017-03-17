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

#include "KexiPropertyEditorView.h"
#include "KexiObjectInfoWidget.h"
#include <KexiMainWindowIface.h>
#include <KPropertySet>
#include <KPropertyEditorView>
#include <KProperty>

#include <QLayout>

#include <KLocalizedString>

//! @internal
class Q_DECL_HIDDEN KexiPropertyEditorView::Private
{
public:
    Private() {
    }
    QVBoxLayout *mainLayout;
    KexiObjectInfoWidget *infoLabel;
    KPropertyEditorView *editor;
};

KexiPropertyEditorView::KexiPropertyEditorView(QWidget* parent)
        : QWidget(parent)
        , d(new Private())
{
    setObjectName("KexiPropertyEditorView");

    d->mainLayout = new QVBoxLayout(this);
    d->mainLayout->setContentsMargins(0, 0, 0, 0);
    d->mainLayout->setSpacing(0);

    d->infoLabel = new KexiObjectInfoWidget;
    d->mainLayout->addWidget(d->infoLabel);

    d->editor = new KPropertyEditorView(this);
    d->editor->setGridLineColor(QColor());
    d->editor->setFrameShape(QFrame::NoFrame);
    d->mainLayout->addWidget(d->editor);
    setFocusProxy(d->editor);
    setFocusPolicy(Qt::WheelFocus);

    connect(d->editor, SIGNAL(propertySetChanged(KPropertySet*)),
            this, SLOT(slotPropertySetChanged(KPropertySet*)));

    slotPropertySetChanged(0);
}

KexiPropertyEditorView::~KexiPropertyEditorView()
{
    delete d;
}

QSize KexiPropertyEditorView::sizeHint() const
{
    return QSize(200, 200);
}

QSize KexiPropertyEditorView::minimumSizeHint() const
{
    return QSize(200, 200);
}

KPropertyEditorView *KexiPropertyEditorView::editor() const
{
    return d->editor;
}

void KexiPropertyEditorView::slotPropertySetChanged(KPropertySet* set)
{
    QString className, iconName, objectName;
    if (set) {
        className = set->propertyValue("this:classString").toString();
        iconName = set->propertyValue("this:iconName").toString();
        const bool useCaptionAsObjectName
            = set->propertyValue("this:useCaptionAsObjectName", false).toBool();
        objectName = set->propertyValue(
            useCaptionAsObjectName ? "caption" : "objectName").toString();
        if (objectName.isEmpty() && useCaptionAsObjectName) {
            // get name if there is no caption
            objectName = set->propertyValue("objectName").toString();
        }
    }
    if (!set || objectName.isEmpty()) {
//! @todo don't hardcode
        QString textToDisplayForNullSet(xi18n("No field selected"));
        objectName = textToDisplayForNullSet;
        className.clear();
        iconName.clear();
    }

    if (className.isEmpty() && objectName.isEmpty())
        d->infoLabel->hide();
    else
        d->infoLabel->show();

    if (d->infoLabel->objectClassName() == className
            && d->infoLabel->objectClassIconName() == iconName
            && d->infoLabel->objectName() == objectName)
        return;

    d->infoLabel->setObjectClassIconName(iconName);
    d->infoLabel->setObjectClassName(className);
    d->infoLabel->setObjectName(objectName);

    d->editor->setEnabled(set);
}
