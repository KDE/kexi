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

#include "KexiObjectInfoLabel.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QIcon>

#include <KIconLoader>
#include <KLocalizedString>

class Q_DECL_HIDDEN KexiObjectInfoLabel::Private
{
public:
    Private() {}
    QString className;
    QString classIconName;
    QString objectName;
    QLabel *objectIconLabel;
    QLabel *objectNameLabel;
};

KexiObjectInfoLabel::KexiObjectInfoLabel(QWidget* parent)
        : QWidget(parent)
        , d( new Private )
{
    QWidget::setObjectName("KexiObjectInfoLabel");
    QHBoxLayout *hlyr = new QHBoxLayout(this);
    hlyr->setContentsMargins(0, 0, 0, 0);
    hlyr->setSpacing(2);
    d->objectIconLabel = new QLabel(this);
    d->objectIconLabel->setMargin(2);
    setMinimumHeight(KIconLoader::global()->currentSize(KIconLoader::Small) + 2 + 2);
    hlyr->addWidget(d->objectIconLabel);
    d->objectNameLabel = new QLabel(this);
    d->objectNameLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    hlyr->addWidget(d->objectNameLabel);
}

KexiObjectInfoLabel::~KexiObjectInfoLabel()
{
    delete d;
}

QString KexiObjectInfoLabel::objectClassIconName() const
{
    return d->classIconName;
}

void KexiObjectInfoLabel::setObjectClassIconName(const QString &iconName)
{
    d->classIconName = iconName;
    if (d->classIconName.isEmpty()) {
        d->objectIconLabel->setFixedWidth(0);
    }
    else {
        d->objectIconLabel->setFixedWidth(KIconLoader::global()->currentSize(KIconLoader::Small) + 2 + 2);
    }
    const QIcon icon(QIcon::fromTheme(iconName));
    d->objectIconLabel->setPixmap(
        icon.pixmap(KIconLoader::global()->currentSize(KIconLoader::Small)));
}

QString KexiObjectInfoLabel::objectClassName() const
{
    return d->className;
}

void KexiObjectInfoLabel::setObjectClassName(const QString& name)
{
    d->className = name;
    updateName();
}

QString KexiObjectInfoLabel::objectName() const
{
    return d->objectName;
}

void KexiObjectInfoLabel::setObjectName(const QString& name)
{
    d->objectName = name;
    updateName();
}

void KexiObjectInfoLabel::updateName()
{
    QString txt(d->className);
    if (txt.isEmpty()) {
        txt = d->objectName;
    }
    else if (!d->objectName.isEmpty()) {
        txt = xi18nc("Object class \"objectName\", e.g. Text editor \"text\"", "%1 <resource>%2</resource>",
            txt, d->objectName);
    }
    d->objectNameLabel->setText(txt);
}

void KexiObjectInfoLabel::setBuddy(QWidget * buddy)
{
    d->objectNameLabel->setBuddy(buddy);
}
