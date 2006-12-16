/* This file is part of the KDE project
   Copyright (C) 2004 Adam Pigg <adam@piggz.co.uk>
   Copyright (C) 2004-2006 Jaroslaw Staniek <js@iidea.pl>
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

#include <qlabel.h>
#include <qlayout.h>
#include <qvbuttongroup.h>
#include <qradiobutton.h>
#include <qcheckbox.h>

#include <kcombobox.h>
#include <kmessagebox.h>
#include <kpushbutton.h>
#include <kdebug.h>
#include <klineedit.h>
#include <kiconloader.h>
#include <kbuttonbox.h>

#include <kexidb/drivermanager.h>
#include <kexidb/driver.h>
#include <kexidb/connectiondata.h>
#include <kexidb/utils.h>
#include <core/kexidbconnectionset.h>
#include <core/kexi.h>
#include <KexiConnSelector.h>
#include <KexiProjectSelector.h>
#include <KexiOpenExistingFile.h>
#include <KexiDBTitlePage.h>
#include <kexiutils/utils.h>
#include <kexidbdrivercombobox.h>
#include <kexitextmsghandler.h>
#include <widget/kexicharencodingcombobox.h>
#include <widget/kexiprjtypeselector.h>


using namespace KexiMigration;

//===========================================================
//
ImportWizard::ImportWizard(QWidget *parent, QMap<QString,QString>* args)
 : KWizard(parent)
 , m_args(args)
{
	setCaption(i18n("Import Database"));
	setIcon(DesktopIcon("database_import"));
	m_prjSet = 0;
	m_fileBasedDstWasPresented = false;
	m_setupFileBasedSrcNeeded = true;
	m_importExecuted = false;
	m_srcTypeCombo = 0;

	setMinimumSize(400, 400);
	parseArguments();
	setupIntro();
//	setupSrcType();
	setupSrcConn();
	setupSrcDB();
	setupDstType();
	setupDstTitle();
	setupDst();
	setupImportType();
	setupImporting();
	setupFinish();

	connect(this, SIGNAL(selected(const QString &)), this, SLOT(pageSelected(const QString &)));
	connect(this, SIGNAL(helpClicked()), this, SLOT(helpClicked()));

	if (m_predefinedConnectionData) {
		// setup wizard for predefined server source
		m_srcConn->showAdvancedConn();
		setAppropriate( m_srcConnPage, false );
		setAppropriate( m_srcDBPage, false );
	}
	else if (!m_predefinedDatabaseName.isEmpty()) {
		// setup wizard for predefined source
		// (used when external project type was opened in Kexi, e.g. mdb file)
//		MigrateManager manager;
//		QString driverName = manager.driverForMimeType( m_predefinedMimeType );
//		m_srcTypeCombo->setCurrentText( driverName );

//		showPage( m_srcConnPage );
		m_srcConn->showSimpleConn();
		m_srcConn->setSelectedFileName(m_predefinedDatabaseName);

		//disable all prev pages except "welcome" page
		for (int i=0; i<indexOf(m_dstTypePage); i++) {
			if (page(i)!=m_introPage)
				setAppropriate( page(i), false );
		}
	}

	m_sourceDBEncoding = QString::fromLatin1(KGlobal::locale()->encoding()); //default
}

//===========================================================
//
ImportWizard::~ImportWizard()
{
	delete m_prjSet;
}

//===========================================================
//
void ImportWizard::parseArguments()
{
	m_predefinedConnectionData = 0;
	if (!m_args)
		return;
	if (!(*m_args)["databaseName"].isEmpty() && !(*m_args)["mimeType"].isEmpty()) {
		m_predefinedDatabaseName = (*m_args)["databaseName"];
		m_predefinedMimeType = (*m_args)["mimeType"];
		if (m_args->contains("connectionData")) {
			m_predefinedConnectionData = new KexiDB::ConnectionData();
			KexiDB::fromMap( 
				KexiUtils::deserializeMap((*m_args)["connectionData"]), *m_predefinedConnectionData 
			);
		}
	}
	m_args->clear();
}

//===========================================================
//
void ImportWizard::setupIntro()
{
	m_introPage = new QWidget(this);
	QVBoxLayout *vbox = new QVBoxLayout(m_introPage, KDialog::marginHint());
	
	QLabel *lblIntro = new QLabel(m_introPage);
	lblIntro->setAlignment( Qt::AlignTop | Qt::AlignLeft | Qt::WordBreak );
	QString msg;
	if (m_predefinedConnectionData) { //predefined import: server source
		msg = i18n("<qt>Database Importing wizard is about to import \"%1\" database "
		"<nobr>(connection %2)</nobr> into a Kexi database.</qt>")
			.arg(m_predefinedDatabaseName).arg(m_predefinedConnectionData->serverInfoString());
	}
	else if (!m_predefinedDatabaseName.isEmpty()) { //predefined import: file source
//! @todo this message is currently ok for files only
		KMimeType::Ptr mimeTypePtr = KMimeType::mimeType(m_predefinedMimeType);
		msg = i18n("<qt>Database Importing wizard is about to import <nobr>\"%1\"</nobr> file "
		"of type \"%2\" into a Kexi database.</qt>")
			.arg(QDir::convertSeparators(m_predefinedDatabaseName)).arg(mimeTypePtr->comment());
	}
	else {
		msg = i18n("Database Importing wizard allows you to import an existing database "
			"into a Kexi database.");
	}
	lblIntro->setText(msg+"\n\n"
		+i18n("Click \"Next\" button to continue or \"Cancel\" button to exit this wizard."));
	vbox->addWidget( lblIntro );
	addPage(m_introPage, i18n("Welcome to the Database Importing Wizard"));
}

//===========================================================
//
/*
void ImportWizard::setupSrcType()
{
	m_srcTypePage = new QWidget(this);

//! @todo Would be good if KexiDBDriverComboBox worked for migration drivers 
	QVBoxLayout *vbox = new QVBoxLayout(m_srcTypePage, KDialog::marginHint());

	QHBoxLayout *hbox = new QHBoxLayout(vbox);
	QLabel *lbl = new QLabel(i18n("Source database type:")+" ", m_srcTypePage);
	hbox->addWidget(lbl);

	m_srcTypeCombo = new KComboBox(m_srcTypePage);
	hbox->addWidget(m_srcTypeCombo);
	hbox->addStretch(1);
	vbox->addStretch(1);
	lbl->setBuddy(m_srcTypeCombo);

	MigrateManager manager;
	QStringList names = manager.driverNames();

	m_srcTypeCombo->insertStringList(names);
	addPage(m_srcTypePage, i18n("Select Source Database Type"));
}
*/
//===========================================================
//
void ImportWizard::setupSrcConn()
{
	m_srcConnPage = new QWidget(this);
	QVBoxLayout *vbox = new QVBoxLayout(m_srcConnPage, KDialog::marginHint());

	m_srcConn = new KexiConnSelectorWidget(Kexi::connset(), 
		":ProjectMigrationSourceDir", m_srcConnPage, "m_srcConnSelector");

	m_srcConn->hideConnectonIcon();
	m_srcConn->showSimpleConn();

	QStringList excludedFilters;
//! @todo remove when support for kexi files as source prj is added in migration
	excludedFilters += KexiDB::Driver::defaultFileBasedDriverMimeType();
	excludedFilters += "application/x-kexiproject-shortcut";
	excludedFilters += "application/x-kexi-connectiondata";
	m_srcConn->m_fileDlg->setExcludedFilters(excludedFilters);

//	m_srcConn->hideHelpers();
	vbox->addWidget(m_srcConn);
	addPage(m_srcConnPage, i18n("Select Location for Source Database"));
}

//===========================================================
//
void ImportWizard::setupSrcDB()
{
//	arrivesrcdbPage creates widgets on that page
	m_srcDBPage = new QWidget(this);
	m_srcDBName = NULL;
	addPage(m_srcDBPage, i18n("Select Source Database"));
}

//===========================================================
//
void ImportWizard::setupDstType()
{
	m_dstTypePage = new QWidget(this);

	KexiDB::DriverManager manager;
	KexiDB::Driver::InfoMap drvs = manager.driversInfo();

	QVBoxLayout *vbox = new QVBoxLayout(m_dstTypePage, KDialog::marginHint());

	QHBoxLayout *hbox = new QHBoxLayout(vbox);
	QLabel *lbl = new QLabel(i18n("Destination database type:")+" ", m_dstTypePage);
	lbl->setAlignment(Qt::AlignAuto|Qt::AlignTop);
	hbox->addWidget(lbl);

	m_dstPrjTypeSelector = new KexiPrjTypeSelector(m_dstTypePage);
	hbox->addWidget(m_dstPrjTypeSelector);
	m_dstPrjTypeSelector->option_file->setText(i18n("Database project stored in a file"));
	m_dstPrjTypeSelector->option_server->setText(i18n("Database project stored on a server"));

	QVBoxLayout *frame_server_vbox = new QVBoxLayout(m_dstPrjTypeSelector->frame_server, KDialog::spacingHint());
	m_dstServerTypeCombo = new KexiDBDriverComboBox(m_dstPrjTypeSelector->frame_server, drvs, 
		KexiDBDriverComboBox::ShowServerDrivers);
	frame_server_vbox->addWidget(m_dstServerTypeCombo);
	hbox->addStretch(1);
	vbox->addStretch(1);
	lbl->setBuddy(m_dstServerTypeCombo);

//! @todo hardcoded: find a way to preselect default engine item
	//m_dstTypeCombo->setCurrentText("SQLite3");
	addPage(m_dstTypePage, i18n("Select Destination Database Type"));
}

//===========================================================
//
void ImportWizard::setupDstTitle()
{
	m_dstTitlePage = new KexiDBTitlePage(i18n("Destination project's caption:"), 
		this, "KexiDBTitlePage");
	m_dstTitlePage->layout()->setMargin( KDialog::marginHint() );
	m_dstTitlePage->updateGeometry();
	m_dstNewDBNameLineEdit = m_dstTitlePage->le_caption;
	addPage(m_dstTitlePage, i18n("Select Destination Database Project's Caption"));
}

//===========================================================
//
void ImportWizard::setupDst()
{
	m_dstPage = new QWidget(this);
	QVBoxLayout *vbox = new QVBoxLayout(m_dstPage, KDialog::marginHint());

	m_dstConn = new KexiConnSelectorWidget(Kexi::connset(), 
		":ProjectMigrationDestinationDir", m_dstPage, "m_dstConnSelector");
	m_dstConn->hideHelpers();
	//me: Can't connect m_dstConn->m_fileDlg here, it doesn't exist yet
	//connect(this, SLOT(next()), m_dstConn->m_fileDlg, SIGNAL(accepted()));

	vbox->addWidget( m_dstConn );
	connect(m_dstConn,SIGNAL(connectionItemExecuted(ConnectionDataLVItem*)),
		this,SLOT(next()));

//	m_dstConn->hideHelpers();
	m_dstConn->showSimpleConn();
	//anyway, db files will be _saved_
	m_dstConn->m_fileDlg->setMode( KexiStartupFileDialog::SavingFileBasedDB );
//	m_dstConn->hideHelpers();
//	m_dstConn->m_file->btn_advanced->hide();
//	m_dstConn->m_file->label->hide();
//	m_dstConn->m_file->lbl->hide();
	//m_dstConn->m_file->spacer7->hide();


	//js dstNewDBName = new KLineEdit(dstControls);
	//   dstNewDBName->setText(i18n("Enter new database name here"));
	addPage(m_dstPage, i18n("Select Location for Destination Database"));
}

//===========================================================
//
void ImportWizard::setupImportType()
{
	m_importTypePage = new QWidget(this);
	QVBoxLayout *vbox = new QVBoxLayout(m_importTypePage, KDialog::marginHint());
	m_importTypeButtonGroup = new QVButtonGroup(m_importTypePage);
	m_importTypeButtonGroup->setLineWidth(0);
	vbox->addWidget( m_importTypeButtonGroup );

	(void)new QRadioButton(i18n("Structure and data"), m_importTypeButtonGroup);
	(void)new QRadioButton(i18n("Structure only"), m_importTypeButtonGroup);

	m_importTypeButtonGroup->setExclusive( true );
	m_importTypeButtonGroup->setButton( 0 );
	addPage(m_importTypePage, i18n("Select Type of Import"));
}

//===========================================================
//
void ImportWizard::setupImporting()
{
	m_importingPage = new QWidget(this);
	m_importingPage->hide();
	QVBoxLayout *vbox = new QVBoxLayout(m_importingPage, KDialog::marginHint());
	m_lblImportingTxt = new QLabel(m_importingPage);
	m_lblImportingTxt->setAlignment( Qt::AlignTop | Qt::AlignLeft | Qt::WordBreak );

	m_lblImportingErrTxt = new QLabel(m_importingPage);
	m_lblImportingErrTxt->setAlignment( Qt::AlignTop | Qt::AlignLeft | Qt::WordBreak );

	m_progressBar = new KProgress(100, m_importingPage);
	m_progressBar->hide();

	vbox->addWidget( m_lblImportingTxt );
	vbox->addWidget( m_lblImportingErrTxt );
	vbox->addStretch(1);

	KButtonBox *optionsBox = new KButtonBox(m_importingPage);
	vbox->addWidget( optionsBox );
	m_importOptionsButton = optionsBox->addButton(i18n("Advanced Options"), this, SLOT(slotOptionsButtonClicked()));
	m_importOptionsButton->setIconSet(SmallIconSet("configure"));
	optionsBox->addStretch(1);

	vbox->addWidget( m_progressBar );

	vbox->addStretch(2);

	m_importingPage->show();

	addPage(m_importingPage, i18n("Importing"));
}

//===========================================================
//
void ImportWizard::setupFinish()
{
	m_finishPage = new QWidget(this);
	m_finishPage->hide();
	QVBoxLayout *vbox = new QVBoxLayout(m_finishPage, KDialog::marginHint());
	m_finishLbl = new QLabel(m_finishPage);
	m_finishLbl->setAlignment( Qt::AlignTop | Qt::AlignLeft | Qt::WordBreak );

	vbox->addWidget( m_finishLbl );
	m_openImportedProjectCheckBox = new QCheckBox(i18n("Open imported project"), 
		m_finishPage, "openImportedProjectCheckBox");
	m_openImportedProjectCheckBox->setChecked(true);
	vbox->addSpacing( KDialog::spacingHint() );
	vbox->addWidget( m_openImportedProjectCheckBox );
	vbox->addStretch(1);

	addPage(m_finishPage, i18n("Success"));
}

//===========================================================
//
bool ImportWizard::checkUserInput()
{
	QString finishtxt;

	if (m_dstNewDBNameLineEdit->text().isEmpty())
	{
		finishtxt = finishtxt + "<br>" + i18n("No new database name was entered.");
	}

	Kexi::ObjectStatus result;
	KexiMigrate* sourceDriver = prepareImport(result);
	if (sourceDriver && sourceDriver->isSourceAndDestinationDataSourceTheSame())
	{
		finishtxt = finishtxt + "<br>" + i18n("Source database is the same as destination.");
	}

	if (! finishtxt.isNull())
	{
		finishtxt = "<qt>" + i18n("Following problems were found with the data you entered:") +
					"<br>" + finishtxt + "<br><br>" +
					i18n("Please click 'Back' button and correct these errors.");
		m_lblImportingErrTxt->setText(finishtxt);
	}

	return finishtxt.isNull();
}

void ImportWizard::arriveSrcConnPage()
{
	m_srcConnPage->hide();

//	checkIfSrcTypeFileBased(m_srcTypeCombo->currentText());
//	if (fileBasedSrcSelected()) {
//moved		m_srcConn->showSimpleConn();
		/*! @todo KexiStartupFileDialog needs "open file" and "open server" modes
		in addition to just "open" */
		if (m_setupFileBasedSrcNeeded) {
			m_setupFileBasedSrcNeeded = false;
			QStringList additionalMimeTypes;
	/* moved
			if (m_srcTypeCombo->currentText().contains("Access")) {
	//! @todo tmp: hardcoded!
				additionalMimeTypes << "application/x-msaccess";
			}*/
			m_srcConn->m_fileDlg->setMode(KexiStartupFileDialog::Opening);
			m_srcConn->m_fileDlg->setAdditionalFilters(additionalMimeTypes);
/*moved			if (m_srcTypeCombo->currentText().contains("Access")) {
	//! @todo tmp: hardcoded!
	#ifdef Q_WS_WIN
				m_srcConn->m_fileDlg->setSelectedFilter("*.mdb");
	#else
				m_srcConn->m_fileDlg->setFilter("*.mdb");
	#endif
			}*/
			//m_srcConn->m_file->label->hide();
			//m_srcConn->m_file->btn_advanced->hide();
			//m_srcConn->m_file->label->parentWidget()->hide();
		}
//	} else {
//		m_srcConn->showAdvancedConn();
//	}
	/*! @todo Support different file extensions based on MigrationDriver */
	m_srcConnPage->show();
}

void ImportWizard::arriveSrcDBPage()
{
	if (fileBasedSrcSelected()) {
		//! @todo Back button doesn't work after selecting a file to import
		//moved	showPage(m_dstTypePage);
	}
	else if (!m_srcDBName) {
		m_srcDBPage->hide();
		kdDebug() << "Looks like we need a project selector widget!" << endl;

		KexiDB::ConnectionData* condata = m_srcConn->selectedConnectionData();
		if(condata) {
			m_prjSet = new KexiProjectSet(*condata);
			QVBoxLayout *vbox = new QVBoxLayout(m_srcDBPage, KDialog::marginHint());
			m_srcDBName = new KexiProjectSelectorWidget(m_srcDBPage,
				"KexiMigrationProjectSelector", m_prjSet);
			vbox->addWidget( m_srcDBName );
			m_srcDBName->label->setText(i18n("Select source database you wish to import:"));
		}
		m_srcDBPage->show();
	}
}

void ImportWizard::arriveDstTitlePage()
{
	if(fileBasedSrcSelected()) {
		QString suggestedDBName( QFileInfo(m_srcConn->selectedFileName()).fileName() );
		const QFileInfo fi( suggestedDBName );
		suggestedDBName = suggestedDBName.left(suggestedDBName.length() 
			- (fi.extension().length() ? (fi.extension().length()+1) : 0));
		m_dstNewDBNameLineEdit->setText( suggestedDBName );
	} else {
		if (m_predefinedConnectionData) {
			// server source db is predefined
			m_dstNewDBNameLineEdit->setText( m_predefinedDatabaseName );
		}
		else {
			if (!m_srcDBName || !m_srcDBName->selectedProjectData()) {
				back(); //todo!
				return;
			}
			m_dstNewDBNameLineEdit->setText( m_srcDBName->selectedProjectData()->databaseName() );
		}
	}
}

void ImportWizard::arriveDstPage()
{
	m_dstPage->hide();

//	checkIfDstTypeFileBased(m_dstTypeCombo->currentText());
	if(fileBasedDstSelected()) {
		m_dstConn->showSimpleConn();
		m_dstConn->m_fileDlg->setMode( KexiStartupFileDialog::SavingFileBasedDB );
		if (!m_fileBasedDstWasPresented) {
			//without extension - it will be added automatically
			m_dstConn->m_fileDlg->setLocationText(m_dstNewDBNameLineEdit->text());
		}
		m_fileBasedDstWasPresented = true;
	} else {
		m_dstConn->showAdvancedConn();
	}
	m_dstPage->show();
}

void ImportWizard::arriveImportingPage() {
//	checkIfDstTypeFileBased(m_dstTypeCombo->currentText());
/*moved	if (m_fileBasedDstWasPresented) {
		if (!m_dstConn->m_fileDlg->checkFileName()) {
			back();
			return;
		}
	}*/
	m_importingPage->hide();
	if (checkUserInput()) {
		setNextEnabled(m_importingPage, true);
	}
	else {
		setNextEnabled(m_importingPage, false);
	}

	m_lblImportingTxt->setText(i18n(
				 "All required information has now "
				 "been gathered. Click \"Next\" button to start importing.\n\n"
				 "Depending on size of the database this may take some time."
				 /*"Note: You may be asked for extra "
				 "information such as field types if "
				 "the wizard could not automatically "
				 "determine this for you."*/));

//todo

	//temp. hack for MS Access driver only
//! @todo for other databases we will need KexiMigration::Conenction 
//! and KexiMigration::Driver classes
	bool showOptions = false;
	if (fileBasedSrcSelected()) {
		Kexi::ObjectStatus result;
		KexiMigrate* sourceDriver = prepareImport(result);
		if (sourceDriver) {
			showOptions = !result.error() 
				&& sourceDriver->propertyValue( "source_database_has_nonunicode_encoding" ).toBool();
			KexiMigration::Data *data = sourceDriver->data();
			sourceDriver->setData( 0 );
			delete data;
		}
	}
	if (showOptions)
		m_importOptionsButton->show();
	else
		m_importOptionsButton->hide();

	m_importingPage->show();
}

void ImportWizard::arriveFinishPage() {
//	backButton()->hide();
//	cancelButton()->setEnabled(false);
//	m_finishLbl->setText(	m_successText.arg(m_dstNewDBNameLineEdit->text()) );
}

bool ImportWizard::fileBasedSrcSelected() const
{
	if (m_predefinedConnectionData)
		return false;

//	kdDebug() << (m_srcConn->selectedConnectionType()==KexiConnSelectorWidget::FileBased) << endl;
	return m_srcConn->selectedConnectionType()==KexiConnSelectorWidget::FileBased;
}

bool ImportWizard::fileBasedDstSelected() const
{
//	QString dstType(m_dstServerTypeCombo->currentText());

	return m_dstPrjTypeSelector->buttonGroup->selectedId() == 1;

/*	if ((dstType == "PostgreSQL") || (dstType == "MySQL")) {
		return false;
	} else {
		return true;
	}*/
}


void ImportWizard::progressUpdated(int percent) {
	m_progressBar->setProgress(percent);
	KApplication::kApplication()->processEvents();
}

//===========================================================
//
QString ImportWizard::driverNameForSelectedSource()
{
	if (fileBasedSrcSelected()) {
		KMimeType::Ptr ptr = KMimeType::findByFileContent( m_srcConn->selectedFileName() );
		if (!ptr || ptr.data()->name()=="application/octet-stream" || ptr.data()->name()=="text/plain") {
			//try by URL:
			ptr = KMimeType::findByURL( m_srcConn->selectedFileName() );
		}
		return ptr ? m_migrateManager.driverForMimeType( ptr.data()->name() ) : QString::null;
	}

	//server-based
	if (m_predefinedConnectionData) {
		return m_predefinedConnectionData->driverName;
	}

	return m_srcConn->selectedConnectionData() 
		? m_srcConn->selectedConnectionData()->driverName : QString::null;
}

//===========================================================
//
void ImportWizard::accept()
{
	/*moved
	backButton()->setEnabled(false);
	finishButton()->setEnabled(false);
//	cancelButton()->setEnabled(false);
	acceptImport();
	backButton()->setEnabled(true);
	finishButton()->setEnabled(true);
//	cancelButton()->setEnabled(true);
*/
	if (m_args) {
		if ((!fileBasedDstSelected() && !m_args->contains("destinationConnectionShortcut"))
			|| !m_openImportedProjectCheckBox->isChecked())
		{
			//do not open dest db if used didn't want it
			//for server connections, destinationConnectionShortcut must be defined
			m_args->remove("destinationDatabaseName");
		}
	}
	KWizard::accept();
}

KexiMigrate* ImportWizard::prepareImport(Kexi::ObjectStatus& result)
{
	KexiUtils::WaitCursor wait;
	
	// Start with a driver manager
	KexiDB::DriverManager manager;

	kdDebug() << "Creating destination driver..." << endl;

	// Get a driver to the destination database
	KexiDB::Driver *destDriver = manager.driver(
		m_dstConn->selectedConnectionData() ? m_dstConn->selectedConnectionData()->driverName //server based
		 : KexiDB::Driver::defaultFileBasedDriverName()
		// : m_dstTypeCombo->currentText() //file based
	);
	if (!destDriver || manager.error())
	{
		result.setStatus(&manager);
		kdDebug() << "Manager error..." << endl;
		manager.debugError();
//		result.setStatus(&manager);
	}

	// Set up destination connection data
	KexiDB::ConnectionData *cdata;
	bool cdataOwned = false;
	QString dbname;
	if (!result.error())
	{
		if (m_dstConn->selectedConnectionData())
		{
			//server-based project
			kdDebug() << "Server destination..." << endl;
			cdata = m_dstConn->selectedConnectionData();
			dbname = m_dstNewDBNameLineEdit->text();
		}
		else // if (m_dstTypeCombo->currentText().lower() == KexiDB::Driver::defaultFileBasedDriverName()) 
		{
			//file-based project
			kdDebug() << "File Destination..." << endl;
			cdata = new KexiDB::ConnectionData();
			cdataOwned = true;
			cdata->caption = m_dstNewDBNameLineEdit->text();
			cdata->driverName = KexiDB::Driver::defaultFileBasedDriverName();
			dbname = m_dstConn->selectedFileName();
			cdata->setFileName( dbname );
			kdDebug() << "Current file name: " << dbname << endl;
		}
/*		else
		{
			//TODO This needs a better message
			//KMessageBox::error(this, 
			result.setStatus(i18n("No connection data is available. You did not select a destination filename."),"");
			//return false;
		} */
	}

	// Find a source (migration) driver name
	QString sourceDriverName;
	if (!result.error())
	{
		sourceDriverName = driverNameForSelectedSource();
		if (sourceDriverName.isEmpty())
			result.setStatus(i18n("No appropriate migration driver found."), 
				m_migrateManager.possibleProblemsInfoMsg());
	}

	// Get a source (migration) driver
	KexiMigrate* sourceDriver = 0;
	if (!result.error())
	{
		sourceDriver = m_migrateManager.driver( sourceDriverName );
		if(!sourceDriver || m_migrateManager.error()) {
			kdDebug() << "Import migrate driver error..." << endl;
			result.setStatus(&m_migrateManager);
		}
	}

	KexiUtils::removeWaitCursor();

	// Set up source (migration) data required for connection
	if (sourceDriver && !result.error())
	{
		// Setup progress feedback for the GUI
		if(sourceDriver->progressSupported()) {
			m_progressBar->updateGeometry();
			disconnect(sourceDriver, SIGNAL(progressPercent(int)), 
				this, SLOT(progressUpdated(int)));
			connect(sourceDriver, SIGNAL(progressPercent(int)),
				this, SLOT(progressUpdated(int)));
			progressUpdated(0);
		}

		bool keepData;
		if (m_importTypeButtonGroup->selectedId() == 0)
		{
			kdDebug() << "Structure and data selected" << endl;
			keepData = true;
		}
		else if (m_importTypeButtonGroup->selectedId() == 1)
		{
			kdDebug() << "structure only selected" << endl;
			keepData = false;
		}
		else
		{
			kdDebug() << "Neither radio button is selected (not possible?) presume keep data" << endl;
			keepData = true;
		}
		
		KexiMigration::Data* md = new KexiMigration::Data();
	//	delete md->destination;
		md->destination = new KexiProjectData(*cdata, dbname);
		if(fileBasedSrcSelected()) {
			KexiDB::ConnectionData* conn_data = new KexiDB::ConnectionData();
			conn_data->setFileName(m_srcConn->selectedFileName());
			md->source = conn_data;
			md->sourceName = "";
		}
		else 
		{
			if (m_predefinedConnectionData)
				md->source = m_predefinedConnectionData;
			else
				md->source = m_srcConn->selectedConnectionData();

			if (!m_predefinedDatabaseName.isEmpty())
				md->sourceName = m_predefinedDatabaseName;
			else
				md->sourceName = m_srcDBName->selectedProjectData()->databaseName();
	//! @todo Aah, this is so C-like. Move to performImport().
		}
		md->keepData = keepData;
		sourceDriver->setData(md);
		return sourceDriver;
	}
	return 0;
}

tristate ImportWizard::import()
{
	m_importExecuted = true;

	Kexi::ObjectStatus result;
	KexiMigrate* sourceDriver = prepareImport(result);

	bool acceptingNeeded = false;

	// Perform import
	if (sourceDriver && !result.error())
	{
		if (!m_sourceDBEncoding.isEmpty()) {
			sourceDriver->setPropertyValue( "source_database_nonunicode_encoding", 
				QVariant(m_sourceDBEncoding.upper().replace(' ',"")) // "CP1250", not "cp 1250" 
			);
		}

		if (!sourceDriver->checkIfDestinationDatabaseOverwritingNeedsAccepting(&result, acceptingNeeded)) {
			kdDebug() << "Abort import cause checkIfDestinationDatabaseOverwritingNeedsAccepting returned false." << endl;
			return false;
		}

		kdDebug() << sourceDriver->data()->destination->databaseName() << endl;
		kdDebug() << "Performing import..." << endl;
	}

	if (sourceDriver && !result.error() && acceptingNeeded) { // ok, the destination-db already exists...
		if (KMessageBox::Yes != KMessageBox::warningYesNo(this,
			"<qt>"+i18n("Database %1 already exists."
			"<p>Do you want to replace it with a new one?")
			.arg(sourceDriver->data()->destination->infoString()),
			0, KGuiItem(i18n("&Replace")), KGuiItem(i18n("No"))))
		{
			return cancelled;
		}
	}

	if (sourceDriver && !result.error() && sourceDriver->progressSupported()) {
		m_progressBar->show();
	}

	if (sourceDriver && !result.error() && sourceDriver->performImport(&result))
	{
		if (m_args) {
//				if (fileBasedDstSelected()) {
			m_args->insert("destinationDatabaseName", 
				sourceDriver->data()->destination->databaseName());
//				}
			QString destinationConnectionShortcut( 
				Kexi::connset().fileNameForConnectionData( m_dstConn->selectedConnectionData() ) );
			if (!destinationConnectionShortcut.isEmpty()) {
				m_args->insert("destinationConnectionShortcut", destinationConnectionShortcut);
			}
		}
		setTitle(m_finishPage, i18n("Success"));
		return true;
	}

	if (!sourceDriver || result.error())
	{
		m_progressBar->setProgress(0);
		m_progressBar->hide();

		QString msg, details;
		KexiTextMessageHandler handler(msg, details);
		handler.showErrorMessage(&result);

		kdDebug() << msg << "\n" << details << endl;
		setTitle(m_finishPage, i18n("Failure"));
		m_finishLbl->setText(	
			i18n("<p>Import failed.</p>%1<p>%2</p><p>You can click \"Back\" button and try again.</p>")
			.arg(msg).arg(details));
		return false;
	}
//	delete kexi_conn;
	return true;
} 

void ImportWizard::reject()
{
	KWizard::reject();
}

//===========================================================
//
void ImportWizard::next()
{
	if (currentPage() == m_srcConnPage) {
		if (fileBasedSrcSelected()
			&& /*! @todo use KURL? */!QFileInfo(m_srcConn->selectedFileName()).isFile()) {

			KMessageBox::sorry(this,i18n("Select source database filename."));
			return;
		}

		if ( (! fileBasedSrcSelected()) && (! m_srcConn->selectedConnectionData()) ) {
			KMessageBox::sorry(this,i18n("Select source database."));
			return;
		}

		KexiMigrate* import = m_migrateManager.driver( driverNameForSelectedSource() );
		if(!import || m_migrateManager.error()) {
			QString dbname;
			if (fileBasedSrcSelected())
				dbname = m_srcConn->selectedFileName();
			else
				dbname = m_srcConn->selectedConnectionData() 
					? m_srcConn->selectedConnectionData()->serverInfoString() : QString::null;
				if (!dbname.isEmpty())
					dbname = QString(" \"%1\"").arg(dbname);
			KMessageBox::error(this, i18n("Could not import database%1. This type is not supported.")
				.arg(dbname));
			return;
		}
	}
	else if (currentPage() == m_dstPage) {
		if (m_fileBasedDstWasPresented) {
			if (fileBasedDstSelected() && !m_dstConn->m_fileDlg->checkFileName())
				return;
		}
	}
	else if (currentPage() == m_importingPage) {
		if (!m_importExecuted) {
			m_importOptionsButton->hide();
			nextButton()->setEnabled(false);
			finishButton()->setEnabled(false);
			backButton()->setEnabled(false);
			m_lblImportingTxt->setText(i18n("Importing in progress..."));
			tristate res = import();
			if (true == res) {
				m_finishLbl->setText( 
					i18n("Database has been imported into Kexi database project \"%1\".")
					.arg(m_dstNewDBNameLineEdit->text()) );
				cancelButton()->setEnabled(false);
				setBackEnabled(m_finishPage, false);
				setFinishEnabled(m_finishPage, true);
				m_openImportedProjectCheckBox->show();
				next();
				return;
			}

			m_progressBar->hide();
			cancelButton()->setEnabled(true);
			setBackEnabled(m_finishPage, true);
			setFinishEnabled(m_finishPage, false);
			m_openImportedProjectCheckBox->hide();
			if (!res)
				next();
			else if (~res) {
				arriveImportingPage();
	//			back();
			}
			m_importExecuted = false;
			return;
		}
	}

	setAppropriate( m_srcDBPage, !fileBasedSrcSelected() && !m_predefinedConnectionData ); //skip m_srcDBPage
	KWizard::next();
}

void ImportWizard::back()
{
	setAppropriate( m_srcDBPage, !fileBasedSrcSelected() && !m_predefinedConnectionData ); //skip m_srcDBPage
	KWizard::back();
}

void ImportWizard::pageSelected(const QString &)
{
	if (currentPage() == m_introPage) {
	}
//	else if (currentPage() == m_srcTypePage) {
//	}
	else if (currentPage() == m_srcConnPage) {
		arriveSrcConnPage();
	}
	else if (currentPage() == m_srcDBPage) {
		arriveSrcDBPage();
	}
	else if (currentPage() == m_dstTypePage) {
	}
	else if (currentPage() == m_dstTitlePage) {
		arriveDstTitlePage();
	}
	else if (currentPage() == m_dstPage) {
		arriveDstPage();
	}
	else if (currentPage() == m_importingPage) {
		arriveImportingPage();
	}
	else if (currentPage() == m_finishPage) {
		arriveFinishPage();
	}
}

void ImportWizard::helpClicked()
{
	if (currentPage() == m_introPage)
	{
		KMessageBox::information(this, i18n("No help is available for this page."), i18n("Help"));
	}
/*	else if (currentPage() == m_srcTypePage)
	{
		KMessageBox::information(this, i18n("Here you can choose the type of data to import data from."), i18n("Help"));
	}*/
	else if (currentPage() == m_srcConnPage)
	{
		KMessageBox::information(this, i18n("Here you can choose the location to import data from."), i18n("Help"));
	}
	else if (currentPage() == m_srcDBPage)
	{
		KMessageBox::information(this, i18n("Here you can choose the actual database to import data from."), i18n("Help"));
	}
	else if (currentPage() == m_dstTypePage)
	{
		KMessageBox::information(this, i18n("Here you can choose the location to save the data."), i18n("Help"));
	}
	else if (currentPage() == m_dstPage)
	{
		KMessageBox::information(this, i18n("Here you can choose the location to save the data in and the new database name."), i18n("Help"));
	}
	else if (currentPage() == m_finishPage || currentPage() == m_importingPage)
	{
		KMessageBox::information(this, i18n("No help is available for this page."), i18n("Help"));
	}
}

void ImportWizard::slotOptionsButtonClicked()
{
	OptionsDialog dlg(m_srcConn->selectedFileName(), m_sourceDBEncoding, this);
	if (QDialog::Accepted != dlg.exec())
		return;

	if (m_sourceDBEncoding != dlg.encodingComboBox()->selectedEncoding()) {
		m_sourceDBEncoding = dlg.encodingComboBox()->selectedEncoding();
	}
}

#include "importwizard.moc"
