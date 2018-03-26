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
    bool focusObjectNameBoxOnChange;

    QByteArray objectNamePropertyName() const
    {
        if (!editor->propertySet()) {
            return "objectName";
        }
        return editor->propertySet()->propertyValue("this:visibleObjectNameProperty", "objectName")
            .toByteArray();
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
    d->mainLyr->addSpacing(s.margins.top());

    d->editor = new KPropertyEditorView(this);
    s.setupEditor(d->editor);
    d->editor->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    // Note: QWidgetItem is used instead of addWidget() so the editor's sizeHint() is used.
    // This displays as many property items as possible.
    d->mainLyr->addItem(new QWidgetItem(d->editor));
    d->mainLyr->addSpacing(s.verticalSpacing); // extra spacing after the editor

    d->mainLyr->addSpacing(s.verticalSpacing);

    d->firstSectionIndex = d->mainLyr->count();
    setFocusProxy(d->editor);
    setFocusPolicy(Qt::WheelFocus);

    changePropertySet(0);
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

void KexiPropertyPaneWidget::changePropertySet(KPropertySet* set,
                                               const QByteArray& propertyToSelect,
                                               KPropertyEditorView::SetOptions options,
                                               const QString& textToDisplayForNullSet)
{
    if (set != d->editor->propertySet()) {
        slotObjectNameChangeAccepted(); // last chance to update
        d->editor->changeSet(set, propertyToSelect, options);
    }
    updateInfoLabelForPropertySet(textToDisplayForNullSet);
}

void KexiPropertyPaneWidget::updateInfoLabelForPropertySet(const QString& textToDisplayForNullSet)
{
    const KPropertySet* set = d->editor->propertySet();
    QString className, iconName, objectName;
    if (set) {
        className = set->propertyValue("this:classString").toString();
        iconName = set->propertyValue("this:iconName").toString();
        const bool useNameAsObjectName = d->objectNamePropertyName() == "objectName";
        d->infoLabel->setObjectNameIsIdentifier(useNameAsObjectName);
        objectName = set->propertyValue(d->objectNamePropertyName()).toString();
        if (objectName.isEmpty() && !useNameAsObjectName) {
            // get name if there is no caption/etc.
            objectName = set->propertyValue("objectName").toString();
        }
        d->infoLabel->setObjectNameReadOnly(
            set->propertyValue("this:objectNameReadOnly", false).toBool());
    } else {
        className = KDb::iifNotEmpty(textToDisplayForNullSet, xi18n("No object selected"));
        iconName.clear();
    }
    d->editor->setVisible(set);

    if (className.isEmpty() && objectName.isEmpty()) {
        d->infoLabel->hide();
    } else {
        d->infoLabel->setObjectNameVisible(set);
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
        if (set && set->propertyValue(d->objectNamePropertyName()).toString() != d->infoLabel->objectName()) {
            d->infoLabel->focusObjectNameBox();
        }
    }
    d->infoLabel->setObjectName(objectName);

    d->infoLabel->layout()->update();
    update();
}

void KexiPropertyPaneWidget::slotObjectNameChangeAccepted()
{
    if (d->editor->propertySet()) {
        d->editor->propertySet()->changeProperty(d->objectNamePropertyName(), d->infoLabel->objectName());
        d->focusObjectNameBoxOnChange = true;
    }
}
