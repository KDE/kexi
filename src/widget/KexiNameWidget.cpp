/* This file is part of the KDE project
   Copyright (C) 2004 Jarosław Staniek <staniek@kde.org>

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

#include "KexiNameWidget.h"
#include <core/kexi.h>

#include <KDbValidator>
#include <KDb>

#include <KMessageBox>
#include <KLocalizedString>

#include <QLabel>
#include <QGridLayout>
#include <QLineEdit>

class KexiNameWidget::Private
{
public:

    Private() {}

    QLabel* lbl_message;
    QLabel* lbl_caption;
    QLabel* lbl_name;
    QLineEdit* le_caption;
    QLineEdit* le_name;
    QGridLayout* lyr;
    KDbMultiValidator *validator;
    QString nameWarning, captionWarning;
    QString originalNameText;

    bool le_name_txtchanged_disable;
    bool le_name_autofill;
    bool caption_required;
};

KexiNameWidget::KexiNameWidget(const QString& message, QWidget* parent)
        : QWidget(parent)
        , d(new Private)
{
    init(message, QString(), QString(), QString(), QString());
}

KexiNameWidget::KexiNameWidget(const QString& message,
                               const QString& nameLabel, const QString& nameText,
                               const QString& captionLabel, const QString& captionText,
                               QWidget * parent)
        : QWidget(parent)
        , d(new Private)
{
    init(message, nameLabel, nameText, captionLabel, captionText);
}

void KexiNameWidget::init(
    const QString& message,
    const QString& nameLabel, const QString& nameText,
    const QString& captionLabel, const QString& captionText)
{
    Q_UNUSED(captionText);
    setObjectName("KexiNameWidget");

    d->le_name_txtchanged_disable = false;
    d->le_name_autofill = true;
    d->caption_required = false;

    d->lyr = new QGridLayout(this);

    d->lbl_message = new QLabel(this);
    setMessageText(message);
    d->lbl_message->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    d->lbl_message->setAlignment(Qt::AlignTop|Qt::AlignLeft);
    d->lbl_message->setWordWrap(true);
    d->lbl_message->setTextInteractionFlags(Qt::TextBrowserInteraction);
    d->lyr->addWidget(d->lbl_message, 0, 0, 1, 2);

    d->lbl_caption = new QLabel(captionLabel.isEmpty() ? xi18n("Caption:") : captionLabel,
                             this);
    d->lbl_caption->setAlignment(Qt::AlignVCenter|Qt::AlignRight);
    d->lyr->addWidget(d->lbl_caption, 1, 0);

    d->lbl_name = new QLabel(nameLabel.isEmpty() ? xi18n("Name:") : nameLabel,
                          this);
    d->lbl_name->setAlignment(Qt::AlignVCenter|Qt::AlignRight);
    d->lyr->addWidget(d->lbl_name, 2, 0);

    d->le_caption = new QLineEdit(this);
    setCaptionText(nameText);
    QSizePolicy le_captionSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    le_captionSizePolicy.setHorizontalStretch(1);
    d->le_caption->setSizePolicy(le_captionSizePolicy);
    d->le_caption->setClearButtonEnabled(true);
    d->lyr->addWidget(d->le_caption, 1, 1);

    d->le_name = new QLineEdit(this);
    setNameText(nameText);
    QSizePolicy le_nameSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    le_captionSizePolicy.setHorizontalStretch(1);
    d->le_name->setSizePolicy(le_captionSizePolicy);
    d->le_name->setClearButtonEnabled(true);
    KDbIdentifierValidator *idValidator = new KDbIdentifierValidator(0);
    idValidator->setLowerCaseForced(true);
    d->le_name->setValidator(d->validator = new KDbMultiValidator(idValidator, this));
    d->lyr->addWidget(d->le_name, 2, 1);

    setFocusProxy(d->le_caption);
    resize(QSize(342, 123).expandedTo(minimumSizeHint()));

    d->nameWarning = xi18n("Please enter the name.");
    d->captionWarning = xi18n("Please enter the caption.");

    connect(d->le_caption, SIGNAL(textChanged(QString)),
            this, SLOT(slotCaptionTextChanged(QString)));
    connect(d->le_name, SIGNAL(textChanged(QString)),
            this, SLOT(slotNameTextChanged(QString)));
    connect(d->le_caption, SIGNAL(returnPressed()),
            this, SIGNAL(returnPressed()));
    connect(d->le_name, SIGNAL(returnPressed()),
            this, SIGNAL(returnPressed()));
}

KexiNameWidget::~KexiNameWidget()
{
    delete d;
}

QLabel* KexiNameWidget::captionLabel() const
{
    return d->lbl_caption;
}

QLabel* KexiNameWidget::nameLabel() const
{
    return d->lbl_name;
}

QLineEdit* KexiNameWidget::captionLineEdit() const
{
    return d->le_caption;
}

QLineEdit* KexiNameWidget::nameLineEdit() const
{
    return d->le_name;
}

QLabel* KexiNameWidget::messageLabel() const
{
    return d->lbl_message;
}

QString KexiNameWidget::messageText() const
{
    return d->lbl_message->text();
}



void KexiNameWidget::slotCaptionTextChanged(const QString &capt)
{
    emit textChanged();
    if (d->le_name->text().isEmpty())
        d->le_name_autofill = true;
    if (d->le_name_autofill) {
        d->le_name_txtchanged_disable = true;
        d->le_name->setText(KDb::stringToIdentifier(capt).toLower());
        d->le_name_txtchanged_disable = false;
    }
}

void KexiNameWidget::slotNameTextChanged(const QString &)
{
    emit textChanged();
    if (d->le_name_txtchanged_disable)
        return;
    d->le_name_autofill = false;
}

void KexiNameWidget::clear()
{
    d->le_name->clear();
    d->le_caption->clear();
}

bool KexiNameWidget::empty() const
{
    return d->le_name->text().isEmpty() || d->le_caption->text().trimmed().isEmpty();
}

void KexiNameWidget::setNameRequired(bool set)
{
    d->validator->setAcceptsEmptyValue(!set);
}

bool KexiNameWidget::isCaptionRequired() const {
    return d->caption_required;
}

void KexiNameWidget::setCaptionRequired(bool set) {
    d->caption_required = set;
}


bool KexiNameWidget::isNameRequired() const
{
    return !d->validator->acceptsEmptyValue();
}

void KexiNameWidget::setCaptionText(const QString& capt)
{
    d->le_caption->setText(capt);
    d->le_name_autofill = true;
}

void KexiNameWidget::setNameText(const QString& name)
{
    d->le_name->setText(name);
    d->originalNameText = name;
    d->le_name_autofill = true;
}

void KexiNameWidget::setWarningForName(const QString& txt)
{
    d->nameWarning = txt;
}

void KexiNameWidget::setWarningForCaption(const QString& txt)
{
    d->captionWarning = txt;
}


void KexiNameWidget::setMessageText(const QString& msg)
{
    if (msg.trimmed().isEmpty()) {
        d->lbl_message->setText(QString());
        d->lbl_message->hide();
    } else {
        d->lbl_message->setText(msg.trimmed() + "<br>");
        d->lbl_message->show();
    }
    messageChanged();
}

QString KexiNameWidget::captionText() const
{
    return d->le_caption->text().trimmed();
}

QString KexiNameWidget::nameText() const
{
    return d->le_name->text().trimmed();
}

QString KexiNameWidget::originalNameText() const
{
    return d->originalNameText;
}


bool KexiNameWidget::checkValidity()
{
    if (isNameRequired() && d->le_name->text().trimmed().isEmpty()) {
        KMessageBox::sorry(0, d->nameWarning);
        d->le_name->setFocus();
        return false;
    }
    if (isCaptionRequired() && d->le_caption->text().trimmed().isEmpty()) {
        KMessageBox::sorry(0, d->captionWarning);
        d->le_caption->setFocus();
        return false;
    }
    QString dummy, message, details;
    if (d->validator->check(dummy, d->le_name->text(), message, details)
            == KDbValidator::Error)
    {
        KMessageBox::detailedSorry(0, message, details);
        d->le_name->setFocus();
        return false;
    }
    return true;
}

KDbValidator *KexiNameWidget::nameValidator() const
{
    return d->validator;
}

void KexiNameWidget::addNameSubvalidator(KDbValidator* validator, bool owned)
{
    d->validator->addSubvalidator(validator, owned);
}

