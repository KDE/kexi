/* This file is part of the KDE project
   Copyright (C) 2004-2012 Jarosław Staniek <staniek@kde.org>

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

#include "KexiNameDialog.h"
#include "KexiNameWidget.h"
#include <core/kexipartinfo.h>
#include <kexi_global.h>

#include <KDbConnection>

#include <KIconLoader>
#include <KMessageBox>

#include <QDebug>
#include <QGridLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLineEdit>

KexiNameDialogValidator::KexiNameDialogValidator()
{
}

KexiNameDialogValidator::~KexiNameDialogValidator()
{
}

// --

class KexiNameDialog::Private
{

public:
    Private()
        : validator(0)
        , checkIfObjectExists(false)
        , allowOverwriting(false)
    {
    }
    ~Private() {
        delete validator;
    }

    QLabel *icon;
    KexiNameWidget* widget;
    const KexiProject *project;
    const KexiPart::Part *part;
    KexiNameDialogValidator *validator;
    QDialogButtonBox *buttonBox;
    bool checkIfObjectExists;
    bool allowOverwriting;
    bool overwriteNeeded;
};

KexiNameDialog::KexiNameDialog(const QString& message, QWidget * parent)
        : QDialog(parent)
        , d(new Private)
{
    d->widget = new KexiNameWidget(message, this);
    init();
}

KexiNameDialog::KexiNameDialog(const QString& message,
                               const QString& nameLabel, const QString& nameText,
                               const QString& captionLabel, const QString& captionText,
                               QWidget * parent)
        : QDialog(parent)
        , d(new Private)
{
    d->widget = new KexiNameWidget(message, nameLabel, nameText, captionLabel, captionText);
    init();
}

KexiNameDialog::~KexiNameDialog()
{
    delete d;
}

void KexiNameDialog::init()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(d->widget);

    QGridLayout *lyr = new QGridLayout;
    mainLayout->addLayout(lyr);
    d->icon = new QLabel;
    d->icon->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    QSizePolicy sp(QSizePolicy::Fixed, QSizePolicy::Preferred);
    sp.setHorizontalStretch(1);
    d->icon->setSizePolicy(sp);
    d->icon->setFixedWidth(50);
    lyr->addWidget(d->icon, 0, 0);

    sp = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    sp.setHorizontalStretch(1);
    d->widget->setSizePolicy(sp);
    lyr->addWidget(d->widget, 0, 1);
    lyr->addItem(new QSpacerItem(25, 10, QSizePolicy::Expanding, QSizePolicy::Minimum), 0, 2);
    lyr->addItem(new QSpacerItem(5, 10, QSizePolicy::Minimum, QSizePolicy::Expanding), 1, 1);
    connect(d->widget, SIGNAL(messageChanged()), this, SLOT(updateSize()));

    // buttons
    d->buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Help);
    QPushButton *okButton = d->buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    okButton->setEnabled(true);
    connect(d->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(d->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    mainLayout->addWidget(d->buttonBox);

    updateSize();
    slotTextChanged();
    connect(d->widget, SIGNAL(textChanged()), this, SLOT(slotTextChanged()));
}

void KexiNameDialog::updateSize()
{
  resize(QSize(400, 140 + (!d->widget->messageLabel()->text().isEmpty() ?
                           d->widget->messageLabel()->height() : 0))
           .expandedTo(minimumSizeHint()));
}

void KexiNameDialog::slotTextChanged()
{
    bool enable = true;
    if (   (d->widget->isNameRequired() && d->widget->nameText().isEmpty())
        || (d->widget->isCaptionRequired() && d->widget->captionText().isEmpty()) )
    {
        enable = false;
    }
    d->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enable);
}

bool KexiNameDialog::canOverwrite()
{
    KDbObject tmpObject;
    tristate result = d->project->dbConnection()->loadObjectData(
                          d->project->typeIdForPluginId(d->part->info()->pluginId()),
                          widget()->nameText(), &tmpObject);
    if (result == cancelled) {
        return true;
    }
    if (result == false) {
        qWarning() << "Cannot load object data for" << widget()->nameText();
        return false;
    }
    if (widget()->originalNameText() == tmpObject.name()) {
        return true;
    }
    if (!d->allowOverwriting) {
        KMessageBox::information(this,
                                 "<p>" + d->part->i18nMessage("Object <resource>%1</resource> already exists.", 0)
                                             .subs(widget()->nameText()).toString()
                                 + "</p><p>" + xi18n("Please choose other name.") + "</p>");
        return false;
    }

    QString msg =
        "<p>" + d->part->i18nMessage("Object <resource>%1</resource> already exists.", 0)
                    .subs(widget()->nameText()).toString()
        + "</p><p>" + xi18n("Do you want to replace it?") + "</p>";
    KGuiItem yesItem(KStandardGuiItem::yes());
    yesItem.setText(xi18n("&Replace"));
    yesItem.setToolTip(xi18n("Replace object"));
    const KMessageBox::ButtonCode res = KMessageBox::warningYesNo(
                  this, msg, QString(),
                  yesItem, KGuiItem(xi18nc("@action:button", "&Choose Other Name...")),
                  QString(),
                  KMessageBox::Notify | KMessageBox::Dangerous);
    if (res == KMessageBox::Yes) {
        d->overwriteNeeded = true;
    }
    return res == KMessageBox::Yes;
}

void KexiNameDialog::accept()
{
    if (d->validator) {
        if (!d->validator->validate(this)) {
            return;
        }
    }
    if (!d->widget->checkValidity())
        return;

    if (d->checkIfObjectExists && d->project) {
        if (!canOverwrite()) {
            return;
        }
    }

    QDialog::accept();
}

void KexiNameDialog::setDialogIcon(const QString &iconName)
{
    d->icon->setPixmap(DesktopIcon(iconName, KIconLoader::SizeMedium));
}

void KexiNameDialog::showEvent(QShowEvent * event)
{
    d->widget->captionLineEdit()->selectAll();
    d->widget->captionLineEdit()->setFocus();
    QDialog::showEvent(event);
}

KexiNameWidget* KexiNameDialog::widget() const
{
    return d->widget;
}

int KexiNameDialog::execAndCheckIfObjectExists(const KexiProject &project,
                                               const KexiPart::Part &part,
                                               bool *overwriteNeeded)
{
    d->project = &project;
    d->part = &part;
    d->checkIfObjectExists = true;
    if (overwriteNeeded) {
        *overwriteNeeded = false;
        d->overwriteNeeded = false;
    }
    int res = exec();
    d->project = 0;
    d->part = 0;
    d->checkIfObjectExists = false;
    if (overwriteNeeded) {
        *overwriteNeeded = d->overwriteNeeded;
    }
    return res;
}

void KexiNameDialog::setAllowOverwriting(bool set)
{
    d->allowOverwriting = set;
}

void KexiNameDialog::setValidator(KexiNameDialogValidator *validator)
{
    delete d->validator;
    d->validator = validator;
}

QDialogButtonBox* KexiNameDialog::buttonBox()
{
    return d->buttonBox;
}
