/* This file is part of the KDE project
   Copyright (C) 2004-2009 Adam Pigg <adam@piggz.co.uk>
   Copyright (C) 2004-2016 Jarosław Staniek <staniek@kde.org>
   Copyright (C) 2005 Martin Ellis <martin.ellis@kdemail.net>

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

#include "importwizard.h"
#include "keximigrate.h"
#include "importoptionsdlg.h"
#include <core/KexiMainWindowIface.h>
#include <core/kexidbconnectionset.h>
#include <core/kexi.h>
#include <kexiutils/utils.h>
#include <kexidbdrivercombobox.h>
#include <kexitextmsghandler.h>
#include <widget/kexicharencodingcombobox.h>
#include <widget/kexiprjtypeselector.h>
#include <widget/KexiConnectionSelectorWidget.h>
#include <widget/KexiProjectSelectorWidget.h>
#include <widget/KexiDBCaptionPage.h>
#include <widget/KexiDBPasswordDialog.h>
#include <widget/KexiStartupFileHandler.h>
#include <KexiIcon.h>

#include <KDbDriverManager>
#include <KDbDriver>
#include <KDbConnectionData>
#include <KDbUtils>
#include <KDb>
#include <KDbIdentifierValidator>

#include <KMessageBox>

#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QRadioButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDir>
#include <QApplication>
#include <QLineEdit>
#include <QMimeDatabase>
#include <QMimeType>
#include <QPushButton>
#include <QDebug>
#include <QProgressBar>

using namespace KexiMigration;

class Q_DECL_HIDDEN ImportWizard::Private
{
public:

    Private(QMap<QString, QString>* args_)
      : srcProjectSelector(0)
      , fileBasedDstWasPresented(false)
      , setupFileBasedSrcNeeded(true)
      , importExecuted(false)
      , prjSet(0)
      , args(args_)
    {
    }

    ~Private()
    {
        delete prjSet;
    }

    QWidget *introPageWidget, *srcConnPageWidget, *srcDBPageWidget,
      *dstTypePageWidget, *dstPageWidget, *importTypePageWidget,
      *importingPageWidget, *finishPageWidget;

    KPageWidgetItem *introPageItem, *srcConnPageItem, *srcDBPageItem,
      *dstTypePageItem, *dstPageItem, *importTypePageItem,
      *importingPageItem, *finishPageItem;


    QGroupBox *importTypeGroupBox;
    QRadioButton *importTypeStructureAndDataCheckBox;
    QRadioButton *importTypeStructureOnlyCheckBox;
    KexiDBCaptionPage* dstCaptionPageWidget;
    KPageWidgetItem *dstCaptionPageItem;

    KexiPrjTypeSelector *dstPrjTypeSelector;

    KexiConnectionSelectorWidget *srcConn, *dstConn;
    QString driverIdForSelectedSource;

    QLineEdit *dstNewDBCaptionLineEdit;
    QLabel *dstNewDBNameLabel;
    QLineEdit *dstNewDBNameLineEdit;

    QLabel *dstNewDBNameUrlLabel;
    KUrlRequester *dstNewDBNameUrl;
    KexiStartupFileHandler *dstNewDBFileHandler;
    KexiProjectSelectorWidget *srcProjectSelector;

    QLabel *lblImportingTxt, *lblImportingErrTxt, *finishLbl;
    QCheckBox *openImportedProjectCheckBox;
    bool fileBasedDstWasPresented;
    bool setupFileBasedSrcNeeded;
    bool importExecuted; //!< used in import()
    KexiProjectSet* prjSet;
    QProgressBar *progressBar;
    QPushButton* importOptionsButton;
    QMap<QString, QString> *args;
    QString predefinedDatabaseName, predefinedMimeType;
    KDbConnectionData *predefinedConnectionData;
    MigrateManager migrateManager; //!< object lives here, so status messages can be globally preserved

    //! Encoding for source db. Currently only used for MDB driver.
//! @todo Hardcoded. Move to KexiMigrate driver's impl.
    QString sourceDBEncoding;

};

//===========================================================
//
ImportWizard::ImportWizard(QWidget *parent, QMap<QString, QString>* args)
        : KAssistantDialog(parent)
        , d(new Private(args))
{
    setModal(true);
    setWindowTitle(xi18nc("@title:window", "Import Database"));
    setWindowIcon(KexiIcon("database-import"));

    KexiMainWindowIface::global()->setReasonableDialogSize(this);

    parseArguments();

    setupIntro();
    setupSrcConn();
    setupSrcDB();
    setupDstType();
    setupDstCaption();
    setupDst();
    setupImportType();
    setupImporting();
    setupFinish();

    connect(this, SIGNAL(currentPageChanged(KPageWidgetItem*,KPageWidgetItem*)), this, SLOT(slot_currentPageChanged(KPageWidgetItem*,KPageWidgetItem*)));
    connect(button(QDialogButtonBox::Help), &QPushButton::clicked, this, &ImportWizard::helpClicked);

    if (d->predefinedConnectionData) {
        // setup wizard for predefined server source
        d->srcConn->showAdvancedConnection();
        setAppropriate(d->srcConnPageItem, false);
        setAppropriate(d->srcDBPageItem, false);
    } else if (!d->predefinedDatabaseName.isEmpty()) {
        // setup wizard for predefined source
        // (used when external project type was opened in Kexi, e.g. mdb file)
        setAppropriate(d->srcConnPageItem, false);
        setAppropriate(d->srcDBPageItem, false);
        d->srcConn->showSimpleConnection();
        d->srcConn->setSelectedFile(d->predefinedDatabaseName);

        #if 0
        //disable all prev pages except "welcome" page
        for (int i = 0; i < indexOf(d->dstTypePage); i++) {
            if (page(i) != d->introPage)
                setAppropriate(page(i), false);
        }
        #endif
    }

    d->sourceDBEncoding = QString::fromLatin1(KexiUtils::encoding()); //default
}

//===========================================================
//
ImportWizard::~ImportWizard()
{
    delete d;
}

//===========================================================
//
void ImportWizard::parseArguments()
{
    d->predefinedConnectionData = 0;
    if (!d->args)
        return;
    if (!(*d->args)["databaseName"].isEmpty() && !(*d->args)["mimeType"].isEmpty()) {
        d->predefinedDatabaseName = (*d->args)["databaseName"];
        d->predefinedMimeType = (*d->args)["mimeType"];
        if (d->args->contains("connectionData")) {
            bool ok;
            d->predefinedConnectionData = new KDbConnectionData(
                KDbUtils::deserializeMap((*d->args)["connectionData"]), &ok);
            if (!ok) {
                delete d->predefinedConnectionData;
                d->predefinedConnectionData = 0;
            }
        }
    }
    d->args->clear();
}

QString ImportWizard::selectedSourceFileName() const
{
    if (d->predefinedDatabaseName.isEmpty())
        return d->srcConn->selectedFile();

    return d->predefinedDatabaseName;
}

//===========================================================
//
void ImportWizard::setupIntro()
{
    d->introPageWidget = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout();

    d->introPageWidget->setLayout(vbox);

    KexiUtils::setStandardMarginsAndSpacing(vbox);

    QLabel *lblIntro = new QLabel(d->introPageWidget);
    lblIntro->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    lblIntro->setWordWrap(true);
    lblIntro->setTextFormat(Qt::RichText);
    KLocalizedString msg;
    if (d->predefinedConnectionData) { // predefined import: server source
        msg = kxi18nc("@info",
                      "Database Importing Assistant is about to import <resource>%1</resource> "
                      "database "
                      "(connection <resource>%2</resource>) into a KEXI project.")
                  .subs(d->predefinedDatabaseName)
                  .subs(d->predefinedConnectionData->toUserVisibleString());
    } else if (!d->predefinedDatabaseName.isEmpty()) { // predefined import: file source
        //! @todo this message is currently ok for files only
        QMimeDatabase db;
        QMimeType mime = db.mimeTypeForName(d->predefinedMimeType);
        if (!mime.isValid()) {
            qWarning() << QString("'%1' mimetype not installed!").arg(d->predefinedMimeType);
        }
        d->driverIdForSelectedSource = driverIdForMimeType(mime);
        msg = kxi18nc(
                  "@info",
                  "Database Importing Assistant is about to import <filename>%1</filename> file "
                  "of type <resource>%2</resource> into a KEXI project.")
                  .subs(QDir::toNativeSeparators(d->predefinedDatabaseName))
                  .subs(mime.isValid() ? mime.comment() : "???");
    } else {
        msg = kxi18nc("@info",
                      "Database Importing Assistant allows you to import an existing database "
                      "into a KEXI project.");
    }
    const QString finalMessage = KexiUtils::localizedSentencesToHtml(
        msg,
        kxi18nc("@info",
                "<para>Click <interface>Next</interface> button to continue or "
                "<interface>Cancel</interface> button to exit this assistant.</para>"));
    lblIntro->setText(finalMessage);
    vbox->addWidget(lblIntro);

    d->introPageItem = new KPageWidgetItem(d->introPageWidget,
                                           xi18n("Welcome to the Database Importing Assistant"));
    addPage(d->introPageItem);
}

//===========================================================
//
void ImportWizard::setupSrcConn()
{
    d->srcConnPageWidget = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout(d->srcConnPageWidget);
    KexiUtils::setStandardMarginsAndSpacing(vbox);

    d->srcConn = new KexiConnectionSelectorWidget(&Kexi::connset(),
                         QUrl("kfiledialog:///ProjectMigrationSourceDir"),
                         KexiConnectionSelectorWidget::Opening, d->srcConnPageWidget);

    d->srcConn->hideConnectonIcon();
    d->srcConn->showSimpleConnection();
    connect(d->srcConn, &KexiConnectionSelectorWidget::connectionSelected,
            this, &ImportWizard::sourceConnectionSelected);

    const QStringList excludedMimeTypes({
//! @todo remove when support for kexi files as source prj is added in migration
        KDb::defaultFileBasedDriverMimeType(),
        "application/x-kexiproject-shortcut",
        "application/x-kexi-connectiondata"});
    d->srcConn->setExcludedMimeTypes(excludedMimeTypes);
    vbox->addWidget(d->srcConn);

    d->srcConnPageItem = new KPageWidgetItem(d->srcConnPageWidget, xi18n("Select Location for Source Database"));
    addPage(d->srcConnPageItem);
}

//===========================================================
//
void ImportWizard::setupSrcDB()
{
// arrivesrcdbPage creates widgets on that page
    d->srcDBPageWidget = new QWidget(this);

    d->srcDBPageItem = new KPageWidgetItem(d->srcDBPageWidget, xi18n("Select Source Database"));
    addPage(d->srcDBPageItem);
}

//===========================================================
//
void ImportWizard::setupDstType()
{
    d->dstTypePageWidget = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout(d->dstTypePageWidget);
    KexiUtils::setStandardMarginsAndSpacing(vbox);

    QHBoxLayout *hbox = new QHBoxLayout;
    vbox->addLayout(hbox);
    KexiUtils::setStandardMarginsAndSpacing(hbox);
    QLabel *lbl = new QLabel(xi18n("Destination database type:") /*+ ' '*/, d->dstTypePageWidget);
    lbl->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    lbl->setTextFormat(Qt::RichText);
    hbox->addWidget(lbl);

    d->dstPrjTypeSelector = new KexiPrjTypeSelector(d->dstTypePageWidget);
    hbox->addWidget(d->dstPrjTypeSelector);
    d->dstPrjTypeSelector->option_file->setText(xi18n("Database project stored in a file"));
    d->dstPrjTypeSelector->option_server->setText(xi18n("Database project stored on a server"));
    hbox->addStretch(1);
    vbox->addStretch(1);

    d->dstTypePageItem = new KPageWidgetItem(d->dstTypePageWidget, xi18n("Select Destination Database Type"));
    addPage(d->dstTypePageItem);
}

//===========================================================
//
void ImportWizard::setupDstCaption()
{
    d->dstCaptionPageWidget = new KexiDBCaptionPage(xi18n("Destination project's caption:"), this);
    d->dstCaptionPageWidget->layout()->setMargin(KexiUtils::marginHint());
    d->dstCaptionPageWidget->updateGeometry();
    d->dstNewDBCaptionLineEdit = d->dstCaptionPageWidget->le_caption;
    connect(d->dstNewDBCaptionLineEdit, &QLineEdit::textChanged, this,
            &ImportWizard::destinationCaptionTextChanged);
    d->dstNewDBNameUrlLabel = d->dstCaptionPageWidget->label_requester;
    d->dstNewDBNameUrl = d->dstCaptionPageWidget->file_requester;
    d->dstNewDBFileHandler = new KexiStartupFileHandler(
        QUrl("kfiledialog:///ProjectMigrationDestinationDir"),
        KexiFileFilters::SavingFileBasedDB,
        d->dstCaptionPageWidget->file_requester);
    d->dstNewDBNameLabel = new QLabel(xi18n("Destination project's name:"), d->dstCaptionPageWidget);
    d->dstCaptionPageWidget->formLayout->setWidget(2, QFormLayout::LabelRole, d->dstNewDBNameLabel);
    d->dstNewDBNameLineEdit = new QLineEdit(d->dstCaptionPageWidget);
    d->dstNewDBNameLineEdit->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    KDbIdentifierValidator *idValidator = new KDbIdentifierValidator(this);
    idValidator->setLowerCaseForced(true);
    d->dstNewDBNameLineEdit->setValidator(idValidator);
    d->dstCaptionPageWidget->formLayout->setWidget(2, QFormLayout::FieldRole, d->dstNewDBNameLineEdit);

    d->dstCaptionPageItem = new KPageWidgetItem(d->dstCaptionPageWidget, xi18n("Enter Destination Database Project's Caption"));
    addPage(d->dstCaptionPageItem);
}

void ImportWizard::destinationCaptionTextChanged(const QString &text)
{
    Q_UNUSED(text);
    updateDestinationDBFileName();
}

void ImportWizard::updateDestinationDBFileName()
{
    d->dstNewDBFileHandler->updateUrl(d->dstNewDBCaptionLineEdit->text());
    d->dstNewDBNameLineEdit->setText(d->dstNewDBCaptionLineEdit->text());
}

//===========================================================
//
void ImportWizard::setupDst()
{
    d->dstPageWidget = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout(d->dstPageWidget);
    KexiUtils::setStandardMarginsAndSpacing(vbox);

    d->dstConn = new KexiConnectionSelectorWidget(&Kexi::connset(),
                         QUrl("kfiledialog:///ProjectMigrationDestinationDir"),
                         KexiConnectionSelectorWidget::Saving, d->dstPageWidget);
    d->dstConn->hideHelpers();

    vbox->addWidget(d->dstConn);
    connect(d->dstConn, SIGNAL(connectionItemExecuted(ConnectionDataLVItem*)),
            this, SLOT(next()));

    d->dstConn->showSimpleConnection();
    //anyway, db files will be _saved_
    d->dstConn->setFileMode(KexiFileFilters::SavingFileBasedDB);
    d->dstPageItem = new KPageWidgetItem(d->dstPageWidget, xi18n("Select Location for Destination Database Project"));
    addPage(d->dstPageItem);
}

//===========================================================
//
void ImportWizard::setupImportType()
{
    d->importTypePageWidget = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout(d->importTypePageWidget);
    KexiUtils::setStandardMarginsAndSpacing(vbox);
    d->importTypeGroupBox = new QGroupBox(d->importTypePageWidget);

    vbox->addWidget(d->importTypeGroupBox);
    QVBoxLayout *importTypeGroupBoxLyr = new QVBoxLayout;

    importTypeGroupBoxLyr->addWidget(
        d->importTypeStructureAndDataCheckBox = new QRadioButton(
            xi18nc("Scope of import", "Structure and data"), d->importTypeGroupBox));
    d->importTypeStructureAndDataCheckBox->setChecked(true);

    importTypeGroupBoxLyr->addWidget(
        d->importTypeStructureOnlyCheckBox = new QRadioButton(
            xi18nc("Scope of import", "Structure only"), d->importTypeGroupBox));

    importTypeGroupBoxLyr->addStretch(1);
    d->importTypeGroupBox->setLayout(importTypeGroupBoxLyr);

    d->importTypePageItem = new KPageWidgetItem(d->importTypePageWidget,
                                                xi18n("Select Scope of Import"));
    addPage(d->importTypePageItem);
}

//===========================================================
//
void ImportWizard::setupImporting()
{
    d->importingPageWidget = new QWidget(this);
    d->importingPageWidget->hide();
    QVBoxLayout *vbox = new QVBoxLayout(d->importingPageWidget);
    KexiUtils::setStandardMarginsAndSpacing(vbox);
    d->lblImportingTxt = new QLabel(d->importingPageWidget);
    d->lblImportingTxt->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    d->lblImportingTxt->setWordWrap(true);
    d->lblImportingTxt->setTextFormat(Qt::RichText);

    d->lblImportingErrTxt = new QLabel(d->importingPageWidget);
    d->lblImportingErrTxt->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    d->lblImportingErrTxt->setWordWrap(true);
    d->lblImportingErrTxt->setTextFormat(Qt::RichText);

    d->progressBar = new QProgressBar(d->importingPageWidget);
    d->progressBar->setRange(0, 100);
    d->progressBar->hide();

    vbox->addWidget(d->lblImportingTxt);
    vbox->addWidget(d->lblImportingErrTxt);
    vbox->addStretch(1);

    QWidget *options_widget = new QWidget(d->importingPageWidget);
    vbox->addWidget(options_widget);
    QVBoxLayout *options_vbox = new QVBoxLayout(options_widget);
    options_vbox->setSpacing(KexiUtils::spacingHint());
    QHBoxLayout *importOptionsButtonLyr = new QHBoxLayout;
    options_vbox->addLayout(importOptionsButtonLyr);
    d->importOptionsButton = new QPushButton(koIcon("configure"),
                                             xi18n("Advanced Options"), options_widget);
    connect(d->importOptionsButton, SIGNAL(clicked()),
            this, SLOT(slotOptionsButtonClicked()));
    importOptionsButtonLyr->addStretch(1);
    importOptionsButtonLyr->addWidget(d->importOptionsButton);
    importOptionsButtonLyr->addStretch(1);
    options_vbox->addStretch(1);

    vbox->addWidget(d->progressBar);
    vbox->addStretch(2);
    d->importingPageWidget->show();

    d->importingPageItem = new KPageWidgetItem(d->importingPageWidget, xi18n("Importing"));
    addPage(d->importingPageItem);
}

//===========================================================
//
void ImportWizard::setupFinish()
{
    d->finishPageWidget = new QWidget(this);
    d->finishPageWidget->hide();
    QVBoxLayout *vbox = new QVBoxLayout(d->finishPageWidget);
    KexiUtils::setStandardMarginsAndSpacing(vbox);
    d->finishLbl = new QLabel(d->finishPageWidget);
    d->finishLbl->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    d->finishLbl->setWordWrap(true);
    d->finishLbl->setTextFormat(Qt::RichText);

    vbox->addWidget(d->finishLbl);
    d->openImportedProjectCheckBox = new QCheckBox(xi18n("Open imported project"),
            d->finishPageWidget);
    d->openImportedProjectCheckBox->setChecked(true);
    vbox->addSpacing(KexiUtils::spacingHint());
    vbox->addWidget(d->openImportedProjectCheckBox);
    vbox->addStretch(1);

    d->finishPageItem = new KPageWidgetItem(d->finishPageWidget, xi18n("Success"));
    addPage(d->finishPageItem);
}

//===========================================================
//
bool ImportWizard::checkUserInput()
{
    QString issues;

    if (d->dstNewDBCaptionLineEdit->text().isEmpty()) {
        issues = xi18nc("@info", "<para>No new database name was entered.</para>");
    }

    Kexi::ObjectStatus result;
    KexiMigrate* sourceDriver = prepareImport(result);
    if (sourceDriver && sourceDriver->isSourceAndDestinationDataSourceTheSame()) {
        // note: we're using .arg() here because the 'issues' argument is already in rich-text format
        issues = xi18nc("@info", "%1<para>Source database is the same as destination.</para>")
                        .arg(issues);
    }

    if (!issues.isEmpty()) {
        // note: we're using .arg() here because the 'issues' argument is already in rich-text format
        d->lblImportingErrTxt->setText(
            xi18nc("@info", "<para>Following issues were found with the data you entered:</para>"
                   "%1"
                   "<para>Please click <interface>Back</interface> button and correct these issues.</para>")
                   .arg(issues));
        return false;
    }
    return true;
}

void ImportWizard::arriveSrcConnPage()
{
    d->srcConnPageWidget->hide();

    /*! @todo KexiFileWidget needs "open file" and "open server" modes
    in addition to just "open" */
    if (d->setupFileBasedSrcNeeded) {
        d->setupFileBasedSrcNeeded = false;
        d->srcConn->setFileMode(KexiFileFilters::Opening);
        d->srcConn->setAdditionalMimeTypes(QStringList());
    }

    /*! @todo Support different file extensions based on MigrationDriver */
    d->srcConnPageWidget->show();
}

void ImportWizard::arriveSrcDBPage()
{
    if (fileBasedSrcSelected()) {
        //! @todo Back button doesn't work after selecting a file to import
    }
    else {
        if (!d->srcProjectSelector) {
            QVBoxLayout *vbox = new QVBoxLayout(d->srcDBPageWidget);
            d->srcProjectSelector = new KexiProjectSelectorWidget(d->srcDBPageWidget);
            vbox->addWidget(d->srcProjectSelector);
            KexiUtils::setStandardMarginsAndSpacing(vbox);
            d->srcProjectSelector->label()->setText(xi18n("Select source database you wish to import:"));
        }
        d->srcDBPageWidget->hide();
        KDbConnectionData* condata = d->srcConn->selectedConnectionData();
        Q_ASSERT(condata);
        Q_ASSERT(d->prjSet);
        d->srcProjectSelector->setProjectSet(d->prjSet);
        d->srcDBPageWidget->show();
    }
}

void ImportWizard::arriveDstCaptionPage()
{
    d->dstNewDBNameUrlLabel->setVisible(fileBasedDstSelected());
    d->dstNewDBNameUrl->setVisible(fileBasedDstSelected());
    d->dstNewDBNameLabel->setVisible(!fileBasedDstSelected());
    d->dstNewDBNameLineEdit->setVisible(!fileBasedDstSelected());
    if (fileBasedSrcSelected()) {
        const QString fname(selectedSourceFileName());
        QString suggestedDBName(QFileInfo(fname).fileName());
        const QFileInfo fi(suggestedDBName);
        suggestedDBName = suggestedDBName.left(suggestedDBName.length()
                                               - (fi.completeSuffix().isEmpty() ? 0 : (fi.completeSuffix().length() + 1)));
        d->dstNewDBCaptionLineEdit->setText(suggestedDBName);
    }
    else {
        if (d->predefinedConnectionData) {
            // server source db is predefined
            d->dstNewDBCaptionLineEdit->setText(d->predefinedDatabaseName);
        } else {
            if (!d->srcProjectSelector || !d->srcProjectSelector->selectedProjectData()) {
                back(); //!< @todo
                return;
            }
            d->dstNewDBCaptionLineEdit->setText(d->srcProjectSelector->selectedProjectData()->databaseName());
        }
    }
    d->dstNewDBCaptionLineEdit->selectAll();
    d->dstNewDBCaptionLineEdit->setFocus();
    updateDestinationDBFileName();
}

void ImportWizard::arriveDstPage()
{
    if (fileBasedDstSelected()) {
        d->dstPageWidget->hide();
        KAssistantDialog::next();
        return;
    }
    else {
        d->dstConn->showAdvancedConnection();
    }
    d->dstPageWidget->show();
}

void ImportWizard::arriveImportingPage()
{
    d->importingPageWidget->hide();
    nextButton()->setEnabled(checkUserInput());
    d->lblImportingTxt->setText(xi18nc("@info",
                                   "<para>All required information has now "
                                   "been gathered. Click <interface>Next</interface> button to start importing.</para>"
                                   "<para><note>Depending on size of the database this may take some time.</note></para>"
                                   /*"Note: You may be asked for extra "
                                   "information such as field types if "
                                   "the wizard could not automatically "
                                   "determine this for you."*/));

    //temp. hack for MS Access driver only
//! @todo for other databases we will need KexiMigration::Connection
//!       and KexiMigration::Driver classes
    bool showOptions = false;
    if (fileBasedSrcSelected()) {
        Kexi::ObjectStatus result;
        KexiMigrate* sourceDriver = prepareImport(result);
        if (sourceDriver) {
            showOptions = !result.error()
                          && sourceDriver->propertyValue("source_database_has_nonunicode_encoding").toBool();
            sourceDriver->setData(nullptr);
        }
    }
    if (showOptions)
        d->importOptionsButton->show();
    else
        d->importOptionsButton->hide();

    d->importingPageWidget->show();
}

void ImportWizard::arriveFinishPage()
{
}

bool ImportWizard::fileBasedSrcSelected() const
{
    if (d->predefinedConnectionData)
        return false;

// qDebug() << (d->srcConn->selectedConnectionType()==KexiConnectionSelectorWidget::FileBased);
    return d->srcConn->selectedConnectionType() == KexiConnectionSelectorWidget::FileBased;
}

bool ImportWizard::fileBasedDstSelected() const
{
    return d->dstPrjTypeSelector->option_file->isChecked();
}

void ImportWizard::progressUpdated(int percent)
{
    d->progressBar->setValue(percent);
    qApp->processEvents();
}

QString ImportWizard::driverIdForMimeType(const QMimeType &mime) const
{
    if (!mime.isValid()) {
        return QString();
    }
    const QStringList ids(d->migrateManager.driverIdsForMimeType(mime.name()));
    //! @todo do we want to return first migrate driver for the mime type or allow to select it?
    return ids.isEmpty() ? QString() : ids.first();
}

QString ImportWizard::findDriverIdForSelectedSource()
{
    if (fileBasedSrcSelected()) {
        QMimeDatabase db;
        QMimeType mime = db.mimeTypeForFile(selectedSourceFileName());
        if (!mime.isValid()
                || mime.name() == "application/octet-stream"
                || mime.name() == "text/plain"
                || mime.name() == "application/zip")
        {
            //try by URL:
            mime = db.mimeTypeForFile(selectedSourceFileName());
        }
        return driverIdForMimeType(mime);
    }

    //server-based
    QString sourceDriverId;
    if (d->predefinedConnectionData) {
        sourceDriverId = d->predefinedConnectionData->driverId();
    } else if (d->srcConn->selectedConnectionData()) {
        sourceDriverId = d->srcConn->selectedConnectionData()->driverId();
    }
    const QStringList migrationDriverIds(d->migrateManager.driverIdsForSourceDriver(sourceDriverId));

    //! @todo First found driver ID is picked. It's OK as long as there is one migration
    //! driver per source database type. How about allowing users to pick migration driver?
    return migrationDriverIds.isEmpty() ? QString() : migrationDriverIds.first();
}

//===========================================================
//
void ImportWizard::accept()
{
    if (d->args) {
        if ((!fileBasedDstSelected() && !d->args->contains("destinationConnectionShortcut"))
                || !d->openImportedProjectCheckBox->isChecked()) {
            //do not open dest db if used didn't want it
            //for server connections, destinationConnectionShortcut must be defined
            d->args->remove("destinationDatabaseName");
        }
    }
    KAssistantDialog::accept();
}

KexiMigrate* ImportWizard::prepareImport(Kexi::ObjectStatus& result)
{
    KexiUtils::WaitCursor wait;

    // Start with a driver manager
    KDbDriverManager manager;

    //qDebug() << "Creating destination driver...";

    // Get a driver to the destination database
    KDbDriver *destDriver = manager.driver(
                        d->dstConn->selectedConnectionData()
                        ? d->dstConn->selectedConnectionData()->driverId()
                        : KDb::defaultFileBasedDriverId());
    if (!destDriver || manager.result().isError()) {
        result.setStatus(manager.resultable());
        qWarning() << "Manager error:" << manager.result();
    }

    // Set up destination connection data
    KDbConnectionData *cdata = 0;
    QScopedPointer<KDbConnectionData> cdataDeleter;
    QString dbname;
    if (!result.error()) {
        if (d->dstConn->selectedConnectionData()) {
            //server-based project
            qDebug() << "Server destination...";
            cdata = d->dstConn->selectedConnectionData();
            dbname = d->dstNewDBNameLineEdit->text();
        }
        else {
            //file-based project
            qDebug() << "File Destination...";
            cdata = new KDbConnectionData();
            cdataDeleter.reset(cdata); // ownership won't be transferred
            cdata->setCaption(d->dstNewDBCaptionLineEdit->text());
            cdata->setDriverId(KDb::defaultFileBasedDriverId());
            dbname = d->dstCaptionPageWidget->file_requester->url().toLocalFile();
            cdata->setDatabaseName(dbname);
            qDebug() << "Current file name:" << dbname;
        }
    }

    // Find a source (migration) driver name
    if (!result.error()) {
        if (d->driverIdForSelectedSource.isEmpty())
            result.setStatus(xi18n("No appropriate migration driver found."),
                             d->migrateManager.possibleProblemsMessage());
    }

    // Get a source (migration) driver
    KexiMigrate* sourceDriver = 0;
    if (!result.error()) {
        sourceDriver = d->migrateManager.driver(d->driverIdForSelectedSource);
        if (!sourceDriver || d->migrateManager.result().isError()) {
            qDebug() << "Import migrate driver error...";
            result.setStatus(d->migrateManager.resultable());
        }
    }

    KexiUtils::removeWaitCursor();

    // Set up source (migration) data required for connection
    if (sourceDriver && !result.error() && cdata) {
        // Setup progress feedback for the GUI
        if (sourceDriver->progressSupported()) {
            d->progressBar->updateGeometry();
            disconnect(sourceDriver, SIGNAL(progressPercent(int)),
                       this, SLOT(progressUpdated(int)));
            connect(sourceDriver, SIGNAL(progressPercent(int)),
                    this, SLOT(progressUpdated(int)));
            progressUpdated(0);
        }

        bool keepData;
        if (d->importTypeStructureAndDataCheckBox->isChecked()) {
            qDebug() << "Structure and data selected";
            keepData = true;
        } else if (d->importTypeStructureOnlyCheckBox->isChecked()) {
            qDebug() << "structure only selected";
            keepData = false;
        } else {
            qDebug() << "Neither radio button is selected (not possible?) presume keep data";
            keepData = true;
        }

        KexiMigration::Data* md = new KexiMigration::Data();
        md->setDestinationProjectData(new KexiProjectData(*cdata, dbname));
        if (fileBasedSrcSelected()) {
            KDbConnectionData* conn_data = new KDbConnectionData();
            conn_data->setDatabaseName(selectedSourceFileName());
            md->source = conn_data;
            md->sourceName.clear();
        } else {
            if (d->predefinedConnectionData)
                md->source = d->predefinedConnectionData;
            else
                md->source = d->srcConn->selectedConnectionData();

            if (!d->predefinedDatabaseName.isEmpty())
                md->sourceName = d->predefinedDatabaseName;
            else
                md->sourceName = d->srcProjectSelector->selectedProjectData()->databaseName();
            //! @todo Aah, this is so C-like. Move to performImport().
        }
        md->setShouldCopyData(keepData);
        sourceDriver->setData(md);
        return sourceDriver;
    }
    return 0;
}

tristate ImportWizard::import()
{
    d->importExecuted = true;
    Kexi::ObjectStatus result;
    KexiMigrate* sourceDriver = prepareImport(result);
    bool acceptingNeeded = false;

    // Perform import
    if (sourceDriver && !result.error()) {
        if (!d->sourceDBEncoding.isEmpty()) {
            sourceDriver->setPropertyValue("source_database_nonunicode_encoding",
                                           QVariant(d->sourceDBEncoding.toUpper().remove(' ')) // "CP1250", not "cp 1250"
                                          );
        }
        if (!sourceDriver->checkIfDestinationDatabaseOverwritingNeedsAccepting(&result, &acceptingNeeded)) {
            qDebug() << "Abort import cause checkIfDestinationDatabaseOverwritingNeedsAccepting "
            "returned false.";
            return false;
        }

        qDebug() << sourceDriver->data()->destinationProjectData()->databaseName();
        qDebug() << "Performing import...";
    }

    if (sourceDriver && !result.error() && acceptingNeeded) {
        // ok, the destination-db already exists...
        if (KMessageBox::Yes != KMessageBox::warningYesNo(this,
                        xi18nc("@info (don't add tags around %1, it's done already)",
                               "<para>Database %1 already exists.</para>"
                               "<para>Do you want to replace it with a new one?</para>",
                               KexiUtils::localizedStringToHtmlSubstring(
                                   sourceDriver->data()->destinationProjectData()->infoString())),
                0, KGuiItem(xi18nc("@action:button Replace Database", "&Replace")), KStandardGuiItem::no()))
        {
            return cancelled;
        }
    }

    if (sourceDriver && !result.error() && sourceDriver->progressSupported()) {
        d->progressBar->show();
    }

    if (sourceDriver && !result.error() && sourceDriver->performImport(&result)) {
        if (d->args) {
            d->args->insert("destinationDatabaseName",
                            fileBasedDstSelected()
                            ? sourceDriver->data()->destinationProjectData()->connectionData()->databaseName()
                            : sourceDriver->data()->destinationProjectData()->databaseName());
            QString destinationConnectionShortcut;
            if (d->dstConn->selectedConnectionData()) {
                destinationConnectionShortcut
                    = Kexi::connset().fileNameForConnectionData(*d->dstConn->selectedConnectionData());
            }
            if (!destinationConnectionShortcut.isEmpty()) {
                d->args->insert("destinationConnectionShortcut", destinationConnectionShortcut);
            }
        }
        d->finishPageItem->setHeader(xi18n("Success"));
        return true;
    }

    if (!sourceDriver || result.error()) {
        d->progressBar->setValue(0);
        d->progressBar->hide();

        QString msg, details;
        KexiTextMessageHandler handler(&msg, &details);
        handler.showErrorMessage(&result);

        qDebug() << msg << "\n" << details;

        d->finishPageItem->setHeader(xi18n("Failure"));
        // note: we're using .arg() here because the msg and details arguments are already in rich-text format
        d->finishLbl->setText(
            xi18nc("@info",
                   "<para>Import failed.</para>"
                   "<para>%1</para>"
                   "<para>%2</para>"
                   "<para>You can click <interface>Back</interface> button and try again.</para>")
                    .arg(msg).arg(details));
        return false;
    }
    return true;
}

void ImportWizard::reject()
{
    KAssistantDialog::reject();
}

//===========================================================
//
void ImportWizard::next()
{
    if (currentPage() == d->srcConnPageItem) {
        if (fileBasedSrcSelected()
                && /*! @todo use QUrl? */!QFileInfo(selectedSourceFileName()).isFile()) {

            KMessageBox::sorry(this, xi18n("Select source database filename."));
            return;
        }

        KDbConnectionData* conndata = d->srcConn->selectedConnectionData();
        if (!fileBasedSrcSelected() && !conndata) {
            KMessageBox::sorry(this, xi18n("Select source database."));
            return;
        }

        d->driverIdForSelectedSource = findDriverIdForSelectedSource(); // cache
        KexiMigrate* import = d->migrateManager.driver(d->driverIdForSelectedSource);
        if (!import || d->migrateManager.result().isError()) {
            QString dbname;
            if (fileBasedSrcSelected())
                dbname = QDir::toNativeSeparators(selectedSourceFileName());
            else
                dbname = conndata ? conndata->toUserVisibleString() : QString();
            KMessageBox::error(this,
                               dbname.isEmpty() ?
                               xi18n("Could not import database. This type is not supported.")
                               : xi18nc("@info",
                                        "Could not import database <resource>%1</resource>. "
                                        "This type is not supported.", dbname));
            return;
        }

        if (!fileBasedSrcSelected()) {
            // make sure we have password if needed
            tristate passwordNeeded = false;
            if (conndata->password().isEmpty()) {
                passwordNeeded = KexiDBPasswordDialog::getPasswordIfNeeded(conndata, this);
            }
            bool ok = passwordNeeded != cancelled;
            if (ok) {
                KexiGUIMessageHandler handler;
                d->prjSet = new KexiProjectSet(&handler);
                if (!d->prjSet->setConnectionData(conndata)) {
                    handler.showErrorMessage(d->prjSet->result());
                    ok = false;
                }
            }
            if (!ok) {
                if (passwordNeeded == true) {
                    conndata->setPassword(QString::null); // not clear(), we have to remove password
                }
                delete d->prjSet;
                d->prjSet = 0;
                return;
            }
        }
    } else if (currentPage() == d->dstCaptionPageItem) {
        if (fileBasedDstSelected()) {
            if (QFileInfo::exists(d->dstNewDBNameUrl->url().toLocalFile())) {
                if (!KexiUtils::askForFileOverwriting(d->dstNewDBNameUrl->url().toLocalFile(), this)) {
                    return;
                }
            }
        }
    } else if (currentPage() == d->importTypePageItem) {
        if (!fileBasedDstSelected()) {
            // make sure we have password if needed
            tristate passwordNeeded = false;
            KDbConnectionData* condata = d->dstConn->selectedConnectionData();
            if (condata->password().isEmpty()) {
                passwordNeeded = KexiDBPasswordDialog::getPasswordIfNeeded(condata, this);
            }
            bool ok = passwordNeeded != cancelled;
            if (!ok) {
                if (passwordNeeded == true) {
                    condata->setPassword(QString::null); // not clear(), we have to remove password
                }
                return;
            }
        }
    } else if (currentPage() == d->importingPageItem) {
        if (!d->importExecuted) {
            d->importOptionsButton->hide();
            backButton()->setEnabled(false);
            nextButton()->setEnabled(false);
            finishButton()->setEnabled(false);
            d->lblImportingTxt->setText(xi18n("Importing in progress..."));
            tristate res = import();
            if (true == res) {
                d->finishLbl->setText(
                    xi18nc("@info",
                           "Database has been imported into Kexi project <resource>%1</resource>.",
                           d->dstNewDBNameLineEdit->text()));
                button(QDialogButtonBox::Cancel)->setEnabled(false);
                backButton()->setEnabled(false);
                nextButton()->setEnabled(true);
                finishButton()->setEnabled(false);
                d->openImportedProjectCheckBox->show();
                next();
                return;
            }

            d->progressBar->hide();

            button(QDialogButtonBox::Cancel)->setEnabled(true);
            backButton()->setEnabled(true);
            nextButton()->setEnabled(false);
            finishButton()->setEnabled(false);

            d->openImportedProjectCheckBox->hide();
            if (!res)
                next();
            else if (~res) {
                arriveImportingPage();
            }
            d->importExecuted = false;
            return;
        }
    }

    setAppropriate(d->srcDBPageItem, !fileBasedSrcSelected() && !d->predefinedConnectionData);
    setAppropriate(d->dstPageItem, !fileBasedDstSelected());
    KAssistantDialog::next();
}

void ImportWizard::back()
{
    setAppropriate(d->srcDBPageItem, !fileBasedSrcSelected() && !d->predefinedConnectionData);
    KAssistantDialog::back();
}

void ImportWizard::slot_currentPageChanged(KPageWidgetItem* curPage,KPageWidgetItem* prevPage)
{
    Q_UNUSED(prevPage);
    if (curPage == d->introPageItem) {
    }
    else if (curPage == d->srcConnPageItem) {
        arriveSrcConnPage();
    } else if (curPage == d->srcDBPageItem) {
        arriveSrcDBPage();
    } else if (curPage == d->dstTypePageItem) {
    } else if (curPage == d->dstCaptionPageItem) {
        arriveDstCaptionPage();
    } else if (curPage == d->dstPageItem) {
        if (fileBasedDstSelected()) {
            if (prevPage == d->importTypePageItem) {
                KAssistantDialog::back();
            }
            else {
                KAssistantDialog::next();
            }
        }
        else {
            arriveDstPage();
        }
    } else if (curPage == d->importingPageItem) {
        arriveImportingPage();
    } else if (curPage == d->finishPageItem) {
        arriveFinishPage();
    }
}

void ImportWizard::helpClicked()
{
    if (currentPage() == d->introPageItem) {
        KMessageBox::information(this, xi18n("No help is available for this page."), xi18n("Help"));
    }
    else if (currentPage() == d->srcConnPageItem) {
        KMessageBox::information(this, xi18n("Here you can choose the location to import data from."), xi18n("Help"));
    } else if (currentPage() == d->srcDBPageItem) {
        KMessageBox::information(this, xi18n("Here you can choose the actual database to import data from."), xi18n("Help"));
    } else if (currentPage() == d->dstTypePageItem) {
        KMessageBox::information(this, xi18n("Here you can choose the location to save the data."), xi18n("Help"));
    } else if (currentPage() == d->dstPageItem) {
        KMessageBox::information(this, xi18n("Here you can choose the location to save the data in and the new database name."), xi18n("Help"));
    } else if (currentPage() == d->finishPageItem || currentPage() == d->importingPageItem) {
        KMessageBox::information(this, xi18n("No help is available for this page."), xi18n("Help"));
    }
}

void ImportWizard::slotOptionsButtonClicked()
{
    OptionsDialog dlg(selectedSourceFileName(), d->sourceDBEncoding, this);
    if (QDialog::Accepted == dlg.exec()) {
        d->sourceDBEncoding = dlg.encodingComboBox()->selectedEncoding();
    }
}

void ImportWizard::sourceConnectionSelected(bool selected)
{
    if (selected) {
        next();
    }
}
