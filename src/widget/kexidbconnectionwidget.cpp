/* This file is part of the KDE project
   Copyright (C) 2005-2014 Jarosław Staniek <staniek@kde.org>
   Copyright (C) 2014 Roman Shtemberko <shtemberko@gmail.com>

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

#include "kexidbconnectionwidget.h"
#include <kexi.h>
#include <kexiguimsghandler.h>
#include <widget/KexiDBPasswordDialog.h>
#include "kexidbdrivercombobox.h"
#include <KexiIcon.h>

#include <KDbConnection>
#include <KDbUtils>
#include <KDbDriverManager>

#include <KStandardAction>

#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWhatsThis>
#include <QDialogButtonBox>
#include <QPushButton>

//! Templorary hides db list
//! @todo reenable this when implemented
#define NO_LOAD_DB_LIST

// @internal
class KexiDBConnectionWidget::Private
{
public:
    Private()
            : connectionOnly(false) {
    }

    QPushButton *btnSaveChanges, *btnTestConnection;
    bool connectionOnly;
    KexiProjectData data;
    KexiDBDriverComboBox *driversCombo;
    QAction *savePasswordHelpAction;
};

class KexiDBConnectionDialog::Private
{
public:
    Private() { }

    KexiDBConnectionTabWidget *tabWidget;
};

//---------

KexiDBConnectionWidget::KexiDBConnectionWidget(QWidget* parent)
        : QWidget(parent)
        , d(new Private)
{
    setupUi(this);
    setObjectName("KexiConnectionSelectorWidget");
    iconLabel->setPixmap(DesktopIcon(Kexi::serverIconName()));

    QVBoxLayout *driversComboLyr = new QVBoxLayout(frmEngine);
    driversComboLyr->setMargin(0);
    d->driversCombo = new KexiDBDriverComboBox(frmEngine, KexiDBDriverComboBox::ShowServerDrivers);
    driversComboLyr->addWidget(d->driversCombo);
    frmEngine->setFocusProxy(d->driversCombo);
    lblEngine->setBuddy(d->driversCombo);
    QWidget::setTabOrder(lblEngine, d->driversCombo);

#ifdef NO_LOAD_DB_LIST
    btnLoadDBList->hide();
#endif
    btnLoadDBList->setIcon(koIcon("view-refresh"));
    btnLoadDBList->setToolTip(xi18n("Load database list from the server"));
    btnLoadDBList->setWhatsThis(
        xi18n("Loads database list from the server, so you can select one using the <interface>Name</interface> combo box."));

    btnSavePasswordHelp->setIcon(koIcon("help-contextual"));
    btnSavePasswordHelp->setToolTip(KStandardAction::whatsThis(0, 0, btnSavePasswordHelp)->text().remove('&'));
    d->savePasswordHelpAction = QWhatsThis::createAction(chkSavePassword);
    connect(btnSavePasswordHelp, SIGNAL(clicked()), this, SLOT(slotShowSavePasswordHelp()));

    QHBoxLayout *hbox = new QHBoxLayout(frmBottom);
    hbox->addStretch(2);
    d->btnSaveChanges = new QPushButton(
        KGuiItem(
            xi18n("Save Changes"), "document-save",
            xi18n("Save all changes made to this connection information"),
            xi18n("Save all changes made to this connection information. "
                 "You can later reuse this information.")),
        frmBottom);
    d->btnSaveChanges->setObjectName("savechanges");
    hbox->addWidget(d->btnSaveChanges);
    hbox->addSpacing(KexiUtils::spacingHint());
    QWidget::setTabOrder(titleEdit, d->btnSaveChanges);
    d->btnSaveChanges->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    d->btnTestConnection = new QPushButton(
//! @todo add Test Connection icon
        KGuiItem(xi18n("&Test Connection"), QString(),
                 xi18n("Test database connection"),
                 xi18n("Tests database connection. "
                      "You can check validity of connection information.")),
        frmBottom);
    d->btnTestConnection->setObjectName("testConnection");
    hbox->addWidget(d->btnTestConnection);
    setTabOrder(d->btnSaveChanges, d->btnTestConnection);
    d->btnTestConnection->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    connect(localhostRBtn, SIGNAL(clicked()), this, SLOT(slotLocationRadioClicked()));
    connect(remotehostRBtn, SIGNAL(clicked()), this, SLOT(slotLocationRadioClicked()));
    connect(chkPortDefault, SIGNAL(toggled(bool)), this , SLOT(slotCBToggled(bool)));
    connect(btnLoadDBList, SIGNAL(clicked()), this, SIGNAL(loadDBList()));
    connect(d->btnSaveChanges, SIGNAL(clicked()), this, SIGNAL(saveChanges()));
}

KexiDBConnectionWidget::~KexiDBConnectionWidget()
{
    delete d;
}

bool KexiDBConnectionWidget::connectionOnly() const
{
    return d->connectionOnly;
}

void KexiDBConnectionWidget::setDataInternal(const KexiProjectData& data, bool connectionOnly,
        const QString& shortcutFileName)
{
    d->data = data;
    d->connectionOnly = connectionOnly;

    if (d->connectionOnly) {
        nameLabel->hide();
        nameCombo->hide();
        btnLoadDBList->hide();
        dbGroupBox->setTitle(xi18n("Database Connection"));
    } else {
        nameLabel->show();
        nameCombo->show();
#ifndef NO_LOAD_DB_LIST
        btnLoadDBList->show();
#endif
        nameCombo->setEditText(d->data.databaseName());
        dbGroupBox->setTitle(xi18n("Database"));
    }
//! @todo what if there's no such driver name?
    d->driversCombo->setCurrentDriverId(d->data.connectionData()->driverId());
    hostEdit->setText(d->data.connectionData()->hostName);
    if (d->data.connectionData()->hostName.isEmpty()) {
        localhostRBtn->setChecked(true);
    }
    else {
        remotehostRBtn->setChecked(true);
    }
    slotLocationRadioClicked();
    if (d->data.connectionData()->port != 0) {
        chkPortDefault->setChecked(false);
        customPortEdit->setValue(d->data.connectionData()->port);
    } else {
        chkPortDefault->setChecked(true);
        /* @todo default port # instead of 0 */
        customPortEdit->setValue(0);
    }
    userEdit->setText(d->data.connectionData()->userName);
    passwordEdit->setText(d->data.connectionData()->password);
    if (d->connectionOnly)
        titleEdit->setText(d->data.connectionData()->caption);
    else
        titleEdit->setText(d->data.caption());

    if (shortcutFileName.isEmpty()) {
        d->btnSaveChanges->hide();
    } else {
        if (!QFileInfo(shortcutFileName).isWritable()) {
            d->btnSaveChanges->setEnabled(false);
        }
    }
    chkSavePassword->setChecked(d->data.connectionData()->savePassword);
    adjustSize();
}

void KexiDBConnectionWidget::setData(const KexiProjectData& data, const QString& shortcutFileName)
{
    setDataInternal(data, false /*!connectionOnly*/, shortcutFileName);
}

void KexiDBConnectionWidget::setData(const KDbConnectionData& data, const QString& shortcutFileName)
{
    KexiProjectData pdata(data);
    setDataInternal(pdata, true /*connectionOnly*/, shortcutFileName);
}

QPushButton* KexiDBConnectionWidget::saveChangesButton() const
{
    return d->btnSaveChanges;
}

QPushButton* KexiDBConnectionWidget::testConnectionButton() const
{
    return d->btnTestConnection;
}

KexiDBDriverComboBox* KexiDBConnectionWidget::driversCombo() const
{
    return d->driversCombo;
}


KexiProjectData KexiDBConnectionWidget::data()
{
    return d->data;
}

void KexiDBConnectionWidget::slotLocationRadioClicked()
{
    hostLbl->setEnabled(remotehostRBtn->isChecked());
    hostEdit->setEnabled(remotehostRBtn->isChecked());
}

void KexiDBConnectionWidget::slotCBToggled(bool on)
{
    if (sender() == chkPortDefault) {
        customPortEdit->setEnabled(!on);
        portLbl->setEnabled(!on);
        if (on) {
            portLbl->setBuddy(customPortEdit);
        }
    }
}

void KexiDBConnectionWidget::slotShowSavePasswordHelp()
{
    QWhatsThis::showText(chkSavePassword->mapToGlobal(QPoint(0, chkSavePassword->height())),
                         chkSavePassword->whatsThis());
}

//-----------

KexiDBConnectionWidgetDetails::KexiDBConnectionWidgetDetails(QWidget* parent)
        : QWidget(parent)
{
    setupUi(this);
    customSocketEdit->setMode(KFile::File | KFile::ExistingOnly | KFile::LocalOnly);
}

KexiDBConnectionWidgetDetails::~KexiDBConnectionWidgetDetails()
{
}

//-----------

KexiDBConnectionTabWidget::KexiDBConnectionTabWidget(QWidget* parent)
        : QTabWidget(parent)
{
    mainWidget = new KexiDBConnectionWidget(this);
    mainWidget->setObjectName("mainWidget");
    mainWidget->layout()->setMargin(KexiUtils::marginHint());
    addTab(mainWidget, xi18n("Parameters"));

    detailsWidget = new KexiDBConnectionWidgetDetails(this);
    detailsWidget->setObjectName("detailsWidget");
    addTab(detailsWidget, xi18n("Details"));
    connect(detailsWidget->chkSocketDefault, SIGNAL(toggled(bool)),
            this, SLOT(slotSocketComboboxToggled(bool)));
    connect(detailsWidget->chkUseSocket, SIGNAL(toggled(bool)),
            this, SLOT(slotSocketComboboxToggled(bool)));

    connect(mainWidget->testConnectionButton(), SIGNAL(clicked()),
            this, SLOT(slotTestConnection()));
}

KexiDBConnectionTabWidget::~KexiDBConnectionTabWidget()
{
}

void KexiDBConnectionTabWidget::setData(const KexiProjectData& data, const QString& shortcutFileName)
{
    mainWidget->setData(data, shortcutFileName);
    detailsWidget->chkUseSocket->setChecked(data.constConnectionData()->useLocalSocketFile);
    detailsWidget->customSocketEdit->setUrl(data.constConnectionData()->localSocketFileName);
    detailsWidget->customSocketEdit->setEnabled(detailsWidget->chkUseSocket->isChecked());
    detailsWidget->chkSocketDefault->setChecked(data.constConnectionData()->localSocketFileName.isEmpty());
    detailsWidget->chkSocketDefault->setEnabled(detailsWidget->chkUseSocket->isChecked());
    detailsWidget->descriptionEdit->setText(data.description());
}

void KexiDBConnectionTabWidget::setData(const KDbConnectionData& data,
                                        const QString& shortcutFileName)
{
    mainWidget->setData(data, shortcutFileName);
    detailsWidget->chkUseSocket->setChecked(data.useLocalSocketFile);
    detailsWidget->customSocketEdit->setUrl(data.localSocketFileName);
    detailsWidget->customSocketEdit->setEnabled(detailsWidget->chkUseSocket->isChecked());
    detailsWidget->chkSocketDefault->setChecked(data.localSocketFileName.isEmpty());
    detailsWidget->chkSocketDefault->setEnabled(detailsWidget->chkUseSocket->isChecked());
    detailsWidget->descriptionEdit->setText(data.description);
}

KexiProjectData KexiDBConnectionTabWidget::currentProjectData()
{
    KexiProjectData data;

//! @todo check if that's database of connection shortcut. Now we're assuming db shortcut only!

    // collect data from the form's fields
    if (mainWidget->connectionOnly()) {
        data.connectionData()->caption = mainWidget->titleEdit->text();
        data.setCaption(QString());
        data.connectionData()->description = detailsWidget->descriptionEdit->toPlainText();
        data.setDatabaseName(QString());
    } else {
        data.connectionData()->caption.clear(); /* connection name is not specified... */
        data.setCaption(mainWidget->titleEdit->text());
        data.setDescription(detailsWidget->descriptionEdit->toPlainText());
        data.setDatabaseName(mainWidget->nameCombo->currentText());
    }
    data.connectionData()->setDriverId(mainWidget->driversCombo()->currentDriverId());
    data.connectionData()->hostName =
        (mainWidget->remotehostRBtn->isChecked()/*remote*/)
        ? mainWidget->hostEdit->text() : QString();
    data.connectionData()->port = mainWidget->chkPortDefault->isChecked()
                                  ? 0 : mainWidget->customPortEdit->value();
    data.connectionData()->localSocketFileName = detailsWidget->chkSocketDefault->isChecked()
            ? QString() : detailsWidget->customSocketEdit->url().toLocalFile();
    data.connectionData()->useLocalSocketFile = detailsWidget->chkUseSocket->isChecked();
//UNSAFE!!!!
    data.connectionData()->userName = mainWidget->userEdit->text();
    if (mainWidget->chkSavePassword->isChecked()) {
        // avoid keeping potentially wrong password that then will be re-used
        data.connectionData()->password = mainWidget->passwordEdit->text();
    }
    data.connectionData()->savePassword = mainWidget->chkSavePassword->isChecked();
    /*! @todo add "options=", eg. as string list? */
    return data;
}

bool KexiDBConnectionTabWidget::savePasswordOptionSelected() const
{
    return mainWidget->chkSavePassword->isChecked();
}

void KexiDBConnectionTabWidget::slotTestConnection()
{
    KDbConnectionData connectionData = *currentProjectData().connectionData();
    bool savePasswordChecked = connectionData.savePassword;
    if (!savePasswordChecked) {
        connectionData.password = mainWidget->passwordEdit->text(); //not saved otherwise
    }
    if (mainWidget->passwordEdit->text().isEmpty()) {
        connectionData.password = QString::null;
        if (savePasswordChecked) {
            connectionData.savePassword = false; //for getPasswordIfNeeded()
        }
        if (~KexiDBPasswordDialog::getPasswordIfNeeded(&connectionData,this)) {
            return;
        }
    }
    KexiGUIMessageHandler msgHandler;
    KDb::connectionTestDialog(this, connectionData, &msgHandler);
}

void KexiDBConnectionTabWidget::slotSocketComboboxToggled(bool on)
{
    if (sender() == detailsWidget->chkSocketDefault) {
        detailsWidget->customSocketEdit->setEnabled(!on);
    } else if (sender() == detailsWidget->chkUseSocket) {
        detailsWidget->customSocketEdit->setEnabled(
            on && !detailsWidget->chkSocketDefault->isChecked());
        detailsWidget->chkSocketDefault->setEnabled(on);
    }
}

//--------

//! @todo set proper help ctxt ID

KexiDBConnectionDialog::KexiDBConnectionDialog(QWidget* parent, const KexiProjectData& data,
        const QString& shortcutFileName, const KGuiItem& acceptButtonGuiItem)
        : QDialog(parent)
        , d(new Private)
{
    setWindowTitle(xi18nc("@title:window", "Open Database"));
    d->tabWidget = new KexiDBConnectionTabWidget(this);
    d->tabWidget->setData(data, shortcutFileName);
    init(acceptButtonGuiItem);
}

KexiDBConnectionDialog::KexiDBConnectionDialog(QWidget* parent,
        const KDbConnectionData& data,
        const QString& shortcutFileName, const KGuiItem& acceptButtonGuiItem)
        : QDialog(parent)
        , d(new Private)
{
    setWindowTitle(xi18nc("@title:window", "Connect to a Database Server"));
    d->tabWidget = new KexiDBConnectionTabWidget(this);
    d->tabWidget->setData(data, shortcutFileName);
    init(acceptButtonGuiItem);
}

KexiDBConnectionDialog::~KexiDBConnectionDialog()
{
    delete d;
}

void KexiDBConnectionDialog::init(const KGuiItem& acceptButtonGuiItem)
{
    setObjectName("KexiDBConnectionDialog");
    setModal(true);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    mainLayout->addWidget(d->tabWidget);
    connect(d->tabWidget->mainWidget, SIGNAL(saveChanges()), this, SIGNAL(saveChanges()));
    connect(d->tabWidget, SIGNAL(testConnection()), this, SIGNAL(testConnection()));

    if (d->tabWidget->mainWidget->connectionOnly())
        d->tabWidget->mainWidget->driversCombo()->setFocus();
    else if (d->tabWidget->mainWidget->nameCombo->currentText().isEmpty())
        d->tabWidget->mainWidget->nameCombo->setFocus();
    else if (d->tabWidget->mainWidget->userEdit->text().isEmpty())
        d->tabWidget->mainWidget->userEdit->setFocus();
    else if (d->tabWidget->mainWidget->passwordEdit->text().isEmpty())
        d->tabWidget->mainWidget->passwordEdit->setFocus();
    else //back
        d->tabWidget->mainWidget->nameCombo->setFocus();

    // buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Help);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    KGuiItem::assign(button(QDialogButtonBox::Ok),
                     acceptButtonGuiItem.text().isEmpty()
                     ? KGuiItem(xi18n("&Open"), koIconName("document-open"), xi18n("Open Database Connection"))
                     : acceptButtonGuiItem
                    );
    mainLayout->addWidget(buttonBox);

    adjustSize();
    resize(width(), d->tabWidget->height());
}

KexiProjectData KexiDBConnectionDialog::currentProjectData()
{
    return d->tabWidget->currentProjectData();
}

bool KexiDBConnectionDialog::savePasswordOptionSelected() const
{
    return d->tabWidget->savePasswordOptionSelected();
}

KexiDBConnectionWidget* KexiDBConnectionDialog::mainWidget() const
{
    return d->tabWidget->mainWidget;
}

KexiDBConnectionWidgetDetails* KexiDBConnectionDialog::detailsWidget() const
{
    return d->tabWidget->detailsWidget;
}

