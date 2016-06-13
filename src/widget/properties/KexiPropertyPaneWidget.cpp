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

#include <KDb>
#include <KPropertySet>
#include <KPropertyEditorView>
#include <KLocalizedString>

#include <QCoreApplication>
#include <QPointer>
#include <QVBoxLayout>

class KexiPropertyPaneWidget::Private
{
public:
    Private() : focusObjectNameBoxOnChange(false) {}
    QVBoxLayout *mainLyr;
    KexiObjectInfoWidget *infoLabel;
    KPropertyEditorView *editor;
    //! Needed by removeAllSections()
    int firstSectionIndex;
    QPointer<KPropertySet> propertySet;
    bool focusObjectNameBoxOnChange;

    const char* objectNamePropertyName() const {
        const bool useCaptionAsObjectName
            = propertySet->propertyValue("this:useCaptionAsObjectName", false).toBool();
        return useCaptionAsObjectName ? "caption" : "objectName";
    }
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
    connect(d->infoLabel, &KexiObjectInfoWidget::objectNameChangeAccepted,
            this, &KexiPropertyPaneWidget::slotObjectNameChangeAccepted);
    d->mainLyr->addSpacing(s.verticalSpacing);

    d->editor = new KPropertyEditorView(this);
    s.setupEditor(d->editor);
    d->editor->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    // Note: QWidgetItem is used instead of addWidget() so the editor's sizeHint() is used.
    // This displays as many property items as possible.
    d->mainLyr->addItem(new QWidgetItem(d->editor));

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
    Q_UNUSED(title);
    if (d->mainLyr->indexOf(widget) != -1) {
        return;
    }
    d->mainLyr->addWidget(widget);
    widget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    widget->show();
}

void KexiPropertyPaneWidget::slotPropertySetChanged(KPropertySet* set)
{
    d->propertySet = set;
    updateInfoLabelForPropertySet(set, QString());
}

void KexiPropertyPaneWidget::updateInfoLabelForPropertySet(KPropertySet* set,
                                                   const QString& textToDisplayForNullSet)
{
    QString className, iconName, objectName;
    if (set) {
        className = set->propertyValue("this:classString").toString();
        iconName = set->propertyValue("this:iconName").toString();
        const bool useCaptionAsObjectName
            = set->propertyValue("this:useCaptionAsObjectName", false).toBool();
        d->infoLabel->setObjectNameIsIdentifier(!useCaptionAsObjectName);
        objectName = set->propertyValue(
            useCaptionAsObjectName ? "caption" : "objectName").toString();
        if (objectName.isEmpty() && useCaptionAsObjectName) {
            // get name if there is no caption
            objectName = set->propertyValue("objectName").toString();
        }
    }
    if (!set) {
        className = KDb::iifNotEmpty(textToDisplayForNullSet, xi18n("No field selected"));
        iconName.clear();
    }

    if (className.isEmpty() && objectName.isEmpty()) {
        d->infoLabel->hide();
    } else {
        d->infoLabel->setObjectVisible(set);
        d->infoLabel->show();
    }

    if (d->infoLabel->objectClassName() == className
            && d->infoLabel->objectClassIconName() == iconName
            && d->infoLabel->objectName() == objectName)
    {
        return;
    }

    d->infoLabel->setObjectClassIconName(iconName);
    d->infoLabel->setObjectClassName(className);
    if (d->focusObjectNameBoxOnChange) {
        d->focusObjectNameBoxOnChange = false;
        if (d->propertySet && d->propertySet->propertyValue(d->objectNamePropertyName()).toString() != d->infoLabel->objectName()) {
            d->infoLabel->focusObjectNameBox();
        }
    }
    d->infoLabel->setObjectName(objectName);

    d->infoLabel->layout()->update();
    update();
}

void KexiPropertyPaneWidget::slotObjectNameChangeAccepted()
{
    d->propertySet->changeProperty(d->objectNamePropertyName(), d->infoLabel->objectName());
    d->focusObjectNameBoxOnChange = true;
}
