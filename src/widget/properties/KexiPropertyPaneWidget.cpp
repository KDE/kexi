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
#include "KexiObjectInfoWidget.h"
#include <KexiStyle.h>

#include <KPropertySet>
#include <KPropertyEditorView>
#include <KLocalizedString>

#include <QVBoxLayout>

class KexiPropertyPaneWidget::Private
{
public:
    Private() {}
    QVBoxLayout *mainLyr;
    KexiObjectInfoWidget *infoLabel;
    KPropertyEditorView *editor;
    //! Needed by removeAllSections()
    int firstSectionIndex;
};

KexiPropertyPaneWidget::KexiPropertyPaneWidget(QWidget *parent)
 : QWidget(parent), d(new Private)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    d->mainLyr = new QVBoxLayout(this);
    d->mainLyr->setContentsMargins(0, 0, 0, 0);
    d->mainLyr->setSpacing(0);

    const KexiStyle::PropertyPane &s = KexiStyle::propertyPane();
    d->mainLyr->addSpacing(s.margins.top());

    d->infoLabel = new KexiObjectInfoWidget;
    d->mainLyr->addWidget(d->infoLabel);
    d->infoLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    d->mainLyr->addSpacing(s.verticalSpacing);

    d->editor = new KPropertyEditorView(this);
    s.setupEditor(d->editor);
    d->editor->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
    d->mainLyr->addWidget(d->editor, 1);

    d->mainLyr->addSpacing(s.verticalSpacing);

    d->firstSectionIndex = d->mainLyr->count();
    setFocusProxy(d->editor);
    setFocusPolicy(Qt::WheelFocus);

    connect(d->editor, &KPropertyEditorView::propertySetChanged,
            this, &KexiPropertyPaneWidget::slotPropertySetChanged);

    slotPropertySetChanged(0);
}

KexiPropertyPaneWidget::~KexiPropertyPaneWidget()
{
    delete d;
}

KPropertyEditorView* KexiPropertyPaneWidget::editor() const
{
    return d->editor;
}

void KexiPropertyPaneWidget::removeAllSections()
{
    while (d->mainLyr->count() > d->firstSectionIndex) {
        QLayoutItem *item = d->mainLyr->itemAt(d->firstSectionIndex);
        if (item->widget()) {
            item->widget()->hide();
        }
        d->mainLyr->removeItem(d->mainLyr->itemAt(d->firstSectionIndex));
    }
}

void KexiPropertyPaneWidget::addSection(QWidget *widget, const QString &title)
{
    if (d->mainLyr->indexOf(widget) != -1) {
        return;
    }
    d->mainLyr->addWidget(widget);
    widget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    widget->show();
}

void KexiPropertyPaneWidget::slotPropertySetChanged(KPropertySet* set)
{
    updateInfoLabelForPropertySet(set);
}

void KexiPropertyPaneWidget::updateInfoLabelForPropertySet(KPropertySet* set)
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
    {
        return;
    }

    d->infoLabel->setObjectClassIconName(iconName);
    d->infoLabel->setObjectClassName(className);
    d->infoLabel->setObjectName(objectName);

    d->infoLabel->layout()->update();
    update();
}
