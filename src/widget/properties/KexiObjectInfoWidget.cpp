/* This file is part of the KDE project
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

#include "KexiObjectInfoWidget.h"
#include "KexiPropertyPaneLineEdit.h"
#include <KexiStyle.h>

#include <QCoreApplication>
#include <QEvent>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>

#include <KIconLoader>
#include <KLocalizedString>

class KexiObjectInfoWidget::Private
{
public:
    Private() {}
    QString classIconName;
    QLabel *objectIconLabel;
    QLabel *objectClassLabel;
    KexiPropertyPaneLineEdit *objectNameBox;
};

KexiObjectInfoWidget::KexiObjectInfoWidget(QWidget* parent)
        : QWidget(parent)
        , d( new Private )
{
    QWidget::setObjectName("KexiObjectInfoWidget");
    QHBoxLayout *hlyr = new QHBoxLayout(this);
    hlyr->setContentsMargins(0, 0, 0, 0);
    hlyr->setSpacing(0);

    const KexiStyle::PropertyPane &s = KexiStyle::propertyPane();

    hlyr->addSpacing(s.sectionTitleIndent);
    d->objectIconLabel = new QLabel;
    d->objectIconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    hlyr->addWidget(d->objectIconLabel, 3);

    d->objectClassLabel = new QLabel;
    d->objectClassLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    d->objectClassLabel->setPalette(s.sectionTitlePalette(d->objectClassLabel->palette()));
    hlyr->addWidget(d->objectClassLabel, 0);

    hlyr->addSpacing(s.horizontalSpacingAfterLabel);

    d->objectNameBox = new KexiPropertyPaneLineEdit;
    hlyr->addWidget(d->objectNameBox, 2);
    d->objectNameBox->setClearButtonEnabled(true);
    d->objectNameBox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    d->objectClassLabel->setBuddy(d->objectNameBox);

    hlyr->addSpacing(s.margins.right());
}

KexiObjectInfoWidget::~KexiObjectInfoWidget()
{
    delete d;
}

QString KexiObjectInfoWidget::objectClassIconName() const
{
    return d->classIconName;
}

void KexiObjectInfoWidget::setObjectClassIconName(const QString &iconName)
{
    const KexiStyle::PropertyPane &s = KexiStyle::propertyPane();
    d->classIconName = iconName;
    if (d->classIconName.isEmpty()) {
        d->objectIconLabel->setFixedWidth(0);
    }
    else {
        d->objectIconLabel->setMaximumWidth(IconSize(KIconLoader::Small) + s.horizontalSpacingAfterIcon);
    }
    d->objectIconLabel->setPixmap(QIcon::fromTheme(iconName).pixmap(IconSize(KIconLoader::Small)));
}

QString KexiObjectInfoWidget::objectClassName() const
{
    return d->objectClassLabel->text();
}

void KexiObjectInfoWidget::setObjectClassName(const QString& name)
{
    d->objectClassLabel->setText(name);
}

QString KexiObjectInfoWidget::objectName() const
{
    return d->objectNameBox->text();
}

void KexiObjectInfoWidget::setObjectName(const QString& name)
{
    d->objectNameBox->setText(name);
    d->objectNameBox->setCursorPosition(0);
}
