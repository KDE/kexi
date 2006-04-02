/* This file is part of the KDE project
   Copyright (C) 2005-2006 Jaroslaw Staniek <js@iidea.pl>

   This work is based on kspread/dialogs/kspread_dlg_csv.cc
   and will be merged back with KOffice libraries.

   Copyright (C) 2002-2003 Norbert Andres <nandres@web.de>
			 (C) 2002-2003 Ariya Hidayat <ariya@kde.org>
			 (C) 2002	  Laurent Montel <montel@kde.org>
			 (C) 1999 David Faure <faure@kde.org>

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

#include <q3buttongroup.h>
#include <qcheckbox.h>
#include <qclipboard.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qmime.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <q3table.h>
#include <qlayout.h>
#include <q3filedialog.h>
#include <qpainter.h>
#include <qtextcodec.h>
#include <qtimer.h>
#include <qfontmetrics.h>
//Added by qt3to4:
#include <Q3VBoxLayout>
#include <Q3Frame>
#include <QKeyEvent>
#include <QEvent>
#include <QTextStream>
#include <Q3GridLayout>
#include <Q3CString>
#include <Q3ValueList>
#include <QPixmap>

#include <kapplication.h>
#include <kdebug.h>
#include <kdialogbase.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kcharsets.h>
#include <knuminput.h>
#include <kprogressbar.h>

#include <kexiutils/identifier.h>
#include <kexiutils/utils.h>
#include <core/kexi.h>
#include <core/kexiproject.h>
#include <core/kexipart.h>
#include <core/kexipartinfo.h>
#include <core/keximainwindow.h>
#include <core/kexiguimsghandler.h>
#include <kexidb/connection.h>
#include <kexidb/tableschema.h>
#include <kexidb/transaction.h>
#include <widget/kexicharencodingcombobox.h>

#include "kexicsvimportdialog.h"
#include "kexicsvimportoptionsdlg.h"
#include "kexicsvwidgets.h"

#ifdef Q_WS_WIN
#include <krecentdirs.h>
#include <windows.h>
#endif

#if 0
#include <kspread_cell.h>
#include <kspread_doc.h>
#include <kspread_sheet.h>
#include <kspread_undo.h>
#include <kspread_view.h>
#include <kglobal.h>
#include <q3tl.h>
#endif

#define _IMPORT_ICON "table" /*todo: change to "file_import" or so*/
#define _TEXT_TYPE 0
#define _NUMBER_TYPE 1
#define _DATE_TYPE 2
#define _TIME_TYPE 3
#define _DATETIME_TYPE 4
#define _PK_FLAG 5

//extra:
#define _NO_TYPE_YET -1 //allows to accept a number of empty cells, before something non-empty
#define _FP_NUMBER_TYPE 255 //_NUMBER_TYPE variant
#define MAX_COLUMNS 100 //max 100 columns is reasonable

class KexiCSVImportDialogTable : public Q3Table
{
public:
	KexiCSVImportDialogTable( QWidget * parent = 0, const char * name = 0 )
	: Q3Table(parent, name) {
		f = font();
		f.setBold(true);
	}
	virtual void paintCell( QPainter * p, int row, int col, const QRect & cr, bool selected, const QColorGroup & cg ) {
		if (row==0)
			p->setFont(f);
		else
			p->setFont(font());
		Q3Table::paintCell(p, row, col, cr, selected, cg);
	}
	virtual void setColumnWidth( int col, int w ) {
		//make columns a bit wider
		Q3Table::setColumnWidth( col, w + 16 );
	}
	QFont f;
};

//! Helper used to temporary disable keyboard and mouse events 
void installRecursiveEventFilter(QObject *filter, QObject *object)
{
	object->installEventFilter(filter);

	if (!object->children())
		return;

	QObjectList list = *object->children();
	for(QObject *obj = list.first(); obj; obj = list.next())
		installRecursiveEventFilter(filter, obj);
}

KexiCSVImportDialog::KexiCSVImportDialog( Mode mode, KexiMainWindow* mainWin, 
	QWidget * parent, const char * name
)
 : KDialogBase( 
	KDialogBase::Plain, 
	i18n( "Import CSV Data File" )
//! @todo use "Paste CSV Data From Clipboard" caption for mode==Clipboard
	,
	(mode==File ? User1 : (ButtonCode)0) |Ok|Cancel, 
	Ok,
	parent, 
	name ? name : "KexiCSVImportDialog", 
	true, 
	false,
	KGuiItem( i18n("&Options"))
  ),
	m_mainWin(mainWin),
	m_cancelled( false ),
	m_adjustRows( 0 ),
	m_startline( 0 ),
	m_textquote( QString(KEXICSV_DEFAULT_TEXT_QUOTE)[0] ),
	m_mode(mode),
	m_prevSelectedCol(-1),
	m_columnsAdjusted(false),
	m_1stRowForFieldNamesDetected(false),
	m_firstFillTableCall(true),
	m_blockUserEvents(false),
	m_primaryKeyColumn(-1),
	m_dialogCancelled(false),
	m_conn(0),
	m_destinationTableSchema(0)
{
	setWFlags(getWFlags() | Qt::WStyle_Maximize | Qt::WStyle_SysMenu);
	hide();
	setButtonOK(KGuiItem( i18n("&Import..."), _IMPORT_ICON));

	m_typeNames.resize(5);
	m_typeNames[0] = i18n("text");
	m_typeNames[1] = i18n("number");
	m_typeNames[2] = i18n("date");
	m_typeNames[3] = i18n("time");
	m_typeNames[4] = i18n("date/time");

	KGlobal::config()->setGroup("ImportExport");
	m_maximumRowsForPreview = KGlobal::config()->readNumEntry("MaximumRowsForPreviewInImportDialog", MAX_COLUMNS);

	m_pkIcon = SmallIcon("key");

	m_uniquenessTest.setAutoDelete(true);

	setIcon(DesktopIcon(_IMPORT_ICON));
	setSizeGripEnabled( TRUE );

	m_encoding = QString::fromLatin1(KGlobal::locale()->encoding());
	m_file = 0;
	m_inputStream = 0;
	
	Q3VBoxLayout *lyr = new Q3VBoxLayout(plainPage(), 0, KDialogBase::spacingHint(), "lyr");

	m_infoLbl = new KexiCSVInfoLabel(
		m_mode==File ? i18n("Preview of data from file:")
		: i18n("Preview of data from clipboard:"),
		plainPage()
	);
	lyr->addWidget( m_infoLbl );

	QWidget* page = new Q3Frame( plainPage(), "page" );
	Q3GridLayout *glyr= new Q3GridLayout( page, 4, 5, 0, KDialogBase::spacingHint(), "glyr");
	lyr->addWidget( page );

	// Delimiter: comma, semicolon, tab, space, other
	m_delimiterWidget = new KexiCSVDelimiterWidget(true /*lineEditOnBottom*/, page);
	glyr->addMultiCellWidget( m_delimiterWidget, 1, 2, 0, 0 );

	QLabel *delimiterLabel = new QLabel(m_delimiterWidget, i18n("Delimiter:"), page);
	delimiterLabel->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
	glyr->addMultiCellWidget( delimiterLabel, 0, 0, 0, 0 );

	// Format: number, text, currency,
	m_formatComboText = i18n( "Format for column %1:" );
	m_formatCombo = new KComboBox(page, "m_formatCombo");
	m_formatCombo->insertItem(i18n("Text"));
	m_formatCombo->insertItem(i18n("Number"));
	m_formatCombo->insertItem(i18n("Date"));
	m_formatCombo->insertItem(i18n("Time"));
	m_formatCombo->insertItem(i18n("Date/Time"));
	glyr->addMultiCellWidget( m_formatCombo, 1, 1, 1, 1 );

	m_formatLabel = new QLabel(m_formatCombo, "", page);
	m_formatLabel->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
	glyr->addWidget( m_formatLabel, 0, 1 );

	m_primaryKeyField = new QCheckBox( i18n( "Primary key" ), page, "m_primaryKeyField" );
	glyr->addWidget( m_primaryKeyField, 2, 1 );
	connect(m_primaryKeyField, SIGNAL(toggled(bool)), this, SLOT(slotPrimaryKeyFieldToggled(bool)));

	m_comboQuote = new KexiCSVTextQuoteComboBox( page );
	glyr->addWidget( m_comboQuote, 1, 2 );

	TextLabel2 = new QLabel( m_comboQuote, i18n( "Text quote:" ), page, "TextLabel2" );
	TextLabel2->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Preferred );
	TextLabel2->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
	glyr->addWidget( TextLabel2, 0, 2 );

	m_startAtLineSpinBox = new KIntSpinBox( page, "m_startAtLineSpinBox" );
	m_startAtLineSpinBox->setMinValue(1);
	m_startAtLineSpinBox->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	m_startAtLineSpinBox->setMinimumWidth(QFontMetrics(m_startAtLineSpinBox->font()).width("8888888"));
	glyr->addWidget( m_startAtLineSpinBox, 1, 3 );

	m_startAtLineLabel = new QLabel( m_startAtLineSpinBox, "", 
		page, "TextLabel3" );
	m_startAtLineLabel->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Preferred );
	m_startAtLineLabel->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
	glyr->addWidget( m_startAtLineLabel, 0, 3 );

	QSpacerItem* spacer_2 = new QSpacerItem( 0, 0, QSizePolicy::Minimum, QSizePolicy::Preferred );
	glyr->addItem( spacer_2, 0, 4 );

	m_ignoreDuplicates = new QCheckBox( page, "m_ignoreDuplicates" );
	m_ignoreDuplicates->setText( i18n( "Ignore duplicated delimiters" ) );
	glyr->addMultiCellWidget( m_ignoreDuplicates, 2, 2, 2, 4 );

	m_1stRowForFieldNames = new QCheckBox( page, "m_1stRowForFieldNames" );
	m_1stRowForFieldNames->setText( i18n( "First row contains column names" ) );
	glyr->addMultiCellWidget( m_1stRowForFieldNames, 3, 3, 2, 4 );

	m_table = new KexiCSVImportDialogTable( plainPage(), "m_table" );
	lyr->addWidget( m_table );

	m_table->setSizePolicy( QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding, 1, 1) );
	m_table->setNumRows( 0 );
	m_table->setNumCols( 0 );

/** @todo reuse Clipboard too! */
/*
if ( m_mode == Clipboard )
  {
	setCaption( i18n( "Inserting From Clipboard" ) );
	QMimeSource * mime = QApplication::clipboard()->data();
	if ( !mime )
	{
	  KMessageBox::information( this, i18n("There is no data in the clipboard.") );
	  m_cancelled = true;
	  return;
	}

	if ( !mime->provides( "text/plain" ) )
	{
	  KMessageBox::information( this, i18n("There is no usable data in the clipboard.") );
	  m_cancelled = true;
	  return;
	}
	m_fileArray = QByteArray(mime->encodedData( "text/plain" ) );
  }
  else if ( mode == File )
  {*/
	m_dateRegExp1 = QRegExp("\\d{1,4}[/\\-\\.]\\d{1,2}[/\\-\\.]\\d{1,2}");
	m_dateRegExp2 = QRegExp("\\d{1,2}[/\\-\\.]\\d{1,2}[/\\-\\.]\\d{1,4}");
	m_timeRegExp1 = QRegExp("\\d{1,2}:\\d{1,2}:\\d{1,2}");
	m_timeRegExp2 = QRegExp("\\d{1,2}:\\d{1,2}");
	m_fpNumberRegExp = QRegExp("\\d*[,\\.]\\d+");

	if (m_mode == File) {
		QStringList mimetypes( csvMimeTypes() );
#ifdef Q_WS_WIN
		//! @todo remove
		QString recentDir = KGlobalSettings::documentPath();
		m_fname = Q3FileDialog::getOpenFileName( 
			KFileDialog::getStartURL(":CSVImportExport", recentDir).path(),
			KexiUtils::fileDialogFilterStrings(mimetypes, false),
			page, "KexiCSVImportDialog", i18n("Open CSV Data File"));
		if ( !m_fname.isEmpty() ) {
			//save last visited path
			KUrl url;
			url.setPath( m_fname );
			if (url.isLocalFile())
				KRecentDirs::add(":CSVImportExport", url.directory());
		}
#else
		m_fname = KFileDialog::getOpenFileName(":CSVImportExport", mimetypes.join(" "), this);
#endif
		//cancel action !
		if ( m_fname.isEmpty() )
		{
			actionButton( Ok )->setEnabled( false );
			m_cancelled = true;
			if (parentWidget())
				parentWidget()->raise();
			return;
		}
	}
	else if (m_mode == Clipboard) {
		Q3CString subtype("plain");
		m_data = QApplication::clipboard()->text(subtype, QClipboard::Clipboard);
/* debug
		for (int i=0;QApplication::clipboard()->data(QClipboard::Clipboard)->format(i);i++)
			kDebug() << i << ": " 
				<< QApplication::clipboard()->data(QClipboard::Clipboard)->format(i) << endl;
*/
	}
	else {
		return;
	}

	m_loadingProgressDlg = 0;
	m_importingProgressDlg = 0;
	if (m_mode == File) {
		m_loadingProgressDlg = new KProgressDialog(
			this, "m_loadingProgressDlg", i18n("Loading CSV Data"), i18n("Loading CSV Data from \"%1\"...")
			.arg(QDir::convertSeparators(m_fname)), true);
		m_loadingProgressDlg->progressBar()->setTotalSteps( m_maximumRowsForPreview+1 );
		m_loadingProgressDlg->show();
	}

	if (m_mode==Clipboard) {
		m_infoLbl->setIcon("editpaste");
	}
	updateRowCountInfo();

	m_table->setSelectionMode(Q3Table::NoSelection);

	connect(m_formatCombo, SIGNAL(activated(int)),
	  this, SLOT(formatChanged(int)));
	connect(m_delimiterWidget, SIGNAL(delimiterChanged(const QString&)),
	  this, SLOT(delimiterChanged(const QString&)));
	connect(m_startAtLineSpinBox, SIGNAL(valueChanged ( int )),
	  this, SLOT(startlineSelected(int)));
	connect(m_comboQuote, SIGNAL(activated(int)),
	  this, SLOT(textquoteSelected(int)));
	connect(m_table, SIGNAL(currentChanged(int, int)),
	  this, SLOT(currentCellChanged(int, int)));
	connect(m_table, SIGNAL(valueChanged(int,int)),
	  this, SLOT(cellValueChanged(int,int)));
	connect(m_ignoreDuplicates, SIGNAL(stateChanged(int)),
	  this, SLOT(ignoreDuplicatesChanged(int)));
	connect(m_1stRowForFieldNames, SIGNAL(stateChanged(int)),
	  this, SLOT(slot1stRowForFieldNamesChanged(int)));

	connect(this, SIGNAL(user1Clicked()), this, SLOT(optionsButtonClicked()));

	installRecursiveEventFilter(this, this);

	initLater();
}

KexiCSVImportDialog::~KexiCSVImportDialog()
{
	delete m_file;
}

void KexiCSVImportDialog::initLater()
{
	if (!openData())
		return;

	QChar detectedDelimiter;
	if (m_mode==File) { //only detect for File mode
		// try to detect delimiter
		// \t has priority, then , then ;
		for (uint i=0; i < qMin(4096, m_data.length()); i++) {
			const QChar c(m_data[i]);
			if (c=='\t') {
				detectedDelimiter = c;
				break;
			}
			else if (c==',' && detectedDelimiter!='\t') {
				detectedDelimiter = c;
			}
			else if (c==';' && detectedDelimiter!='\t' && detectedDelimiter!=',') {
				detectedDelimiter = c;
			}
		}
	}
	if (detectedDelimiter.isNull())
		detectedDelimiter = m_mode==File 
			? KEXICSV_DEFAULT_FILE_DELIMITER[0] : KEXICSV_DEFAULT_CLIPBOARD_DELIMITER[0]; //<-- defaults

	m_delimiterWidget->setDelimiter(QString(detectedDelimiter));
//	delimiterChanged(detectedDelimiter); // this will cause fillTable()
	m_columnsAdjusted = false;
	fillTable();
	delete m_loadingProgressDlg;
	m_loadingProgressDlg = 0;
	if (m_dialogCancelled) {
//		m_loadingProgressDlg->hide();
	//	m_loadingProgressDlg->close();
		QTimer::singleShot(0, this, SLOT(reject()));
		return;
	}

	currentCellChanged(0, 0);

//	updateGeometry();
	adjustSize();
	KDialog::centerOnScreen( this ); 

	if (m_loadingProgressDlg)
		m_loadingProgressDlg->hide();
	show();
	m_table->setFocus();
}

bool KexiCSVImportDialog::openData()
{
	if (m_mode!=File) //data already loaded, no encoding stuff needed
		return true;

	delete m_inputStream;
	m_inputStream = 0;
	if (m_file) {
		m_file->close();
		delete m_file;
	}
	m_file = new QFile(m_fname);
	if (!m_file->open(QIODevice::ReadOnly))
	{
		m_file->close();
		delete m_file;
		m_file = 0;
		KMessageBox::sorry( this, i18n("Cannot open input file <nobr>\"%1\"</nobr>.")
			.arg(QDir::convertSeparators(m_fname)) );
		actionButton( Ok )->setEnabled( false );
		m_cancelled = true;
		if (parentWidget())
			parentWidget()->raise();
		return false;
	}
	return true;
}

bool KexiCSVImportDialog::cancelled() const
{
	return m_cancelled;
}

void KexiCSVImportDialog::fillTable()
{
	KexiUtils::WaitCursor wc(true);
	repaint();
	m_blockUserEvents = true;
	QPushButton *pb = actionButton(KDialogBase::Cancel);
	if (pb)
		pb->setEnabled(true); //allow to cancel
	KexiUtils::WaitCursor wait;

	if (m_table->numRows()>0) //to accept editor
		m_table->setCurrentCell(0,0);

	int row, column, maxColumn;
	QString field = QString::null;

	for (row = 0; row < m_table->numRows(); ++row)
		for (column = 0; column < m_table->numCols(); ++column)
			m_table->clearCell(row, column);

	m_detectedTypes.clear();
	m_detectedTypes.resize(1024, _NO_TYPE_YET);//_TEXT_TYPE);
	m_uniquenessTest.clear();
	m_uniquenessTest.resize(1024);
	m_1stRowForFieldNamesDetected = true;

	if (true != loadRows(field, row, column, maxColumn, true))
		return;

	m_1stRowForFieldNamesDetected = false;

	// file with only one line without '\n'
	if (field.length() > 0)
	{
		setText(row - m_startline, column, field, true);
		++row;
		field = QString::null;
	}

	adjustRows( row - m_startline - (m_1stRowForFieldNames->isChecked()?1:0) );

	maxColumn = qMax( maxColumn, column );
	m_table->setNumCols(maxColumn);

	for (column = 0; column < m_table->numCols(); ++column)
	{
//		QString header = m_table->horizontalHeader()->label(column);
//		if (header != i18n("Text") && header != i18n("Number") &&
//			header != i18n("Date") && header != i18n("Currency"))
//		const int detectedType = m_detectedTypes[column+1];
//		m_table->horizontalHeader()->setLabel(column, m_typeNames[ detectedType ]); //i18n("Text"));
		updateColumnText(column);
		if (!m_columnsAdjusted)
			m_table->adjustColumn(column);
	}
	m_columnsAdjusted = true;

	if (m_primaryKeyColumn>=0 && m_primaryKeyColumn<m_table->numCols()) {
		if (_NUMBER_TYPE != m_detectedTypes[ m_primaryKeyColumn ]) {
			m_primaryKeyColumn = -1;
		}
	}

	m_prevSelectedCol = -1;
	m_table->setCurrentCell(0,0);
	currentCellChanged(0, 0);
	if (m_primaryKeyColumn != -1)
		m_table->setPixmap(0, m_primaryKeyColumn, m_pkIcon);

	const int count = qMax(0, m_table->numRows()-1+m_startline);
	const bool allRowsLoaded = count < m_maximumRowsForPreview;
	if (allRowsLoaded) {
		m_startAtLineSpinBox->setMaxValue(count);
		m_startAtLineSpinBox->setValue(m_startline+1);
	}
	m_startAtLineLabel->setText(i18n( "Start at line%1:").arg(
			allRowsLoaded ? QString(" (1-%1)").arg(count)
			: QString::null //we do not know what's real count
	));

	m_blockUserEvents = false;
	repaint();
	m_table->verticalScrollBar()->repaint();//avoid missing repaint
	m_table->horizontalScrollBar()->repaint();//avoid missing repaint
}

tristate KexiCSVImportDialog::loadRows(QString &field, int &row, int &column, int &maxColumn, 
	bool inGUI)
{
	enum { S_START, S_QUOTED_FIELD, S_MAYBE_END_OF_QUOTED_FIELD, S_END_OF_QUOTED_FIELD,
		 S_MAYBE_NORMAL_FIELD, S_NORMAL_FIELD } state = S_START;
	const QChar delimiter(m_delimiterWidget->delimiter()[0]);
	field = QString::null;
	const bool ignoreDups = m_ignoreDuplicates->isChecked();
	bool lastCharDelimiter = false;
	bool nextRow = false;
	row = column = 1;
	maxColumn = 0;
	QChar x;
	delete m_inputStream;
	if ( m_mode == Clipboard ) {
		m_inputStream = new QTextStream(m_data, QIODevice::ReadOnly);
	}
	else {
		m_file->at(0); //always seek at 0 because loadRows() is called many times
		m_inputStream = new QTextStream(m_file);
		if (m_encoding != QString::fromLatin1(KGlobal::locale()->encoding())) {
			QTextCodec *codec = KGlobal::charsets()->codecForName(m_encoding);
			if (codec)
				m_inputStream->setCodec(codec); //QTextCodec::codecForName("CP1250"));
		}
	}
	int progressStep = 0;
	if (m_importingProgressDlg)
		progressStep = qMax( 1, m_importingProgressDlg->progressBar()->totalSteps()/200 );
	int offset = 0;
	for (;!m_inputStream->atEnd(); offset++)
	{
		if (column >= m_maximumRowsForPreview)
			return true;

		if (m_importingProgressDlg && ((offset % progressStep) < 5)) {
			//update progr. bar dlg on final exporting
			m_importingProgressDlg->progressBar()->setValue(offset);
			qApp->processEvents();
			if (m_importingProgressDlg->wasCanceled()) {
				delete m_importingProgressDlg;
				m_importingProgressDlg = 0;
				return ::cancelled;
			}
		}

		(*m_inputStream) >> x; // read one char

		if (x == '\r') {
			continue; // eat '\r', to handle RFC-compliant files
		}

		switch (state)
		{
		case S_START :
			if (x == m_textquote)
			{
				state = S_QUOTED_FIELD;
			}
			else if (x == delimiter)
			{
				setText(row - m_startline, column, field, inGUI);
				field = QString::null;
				if ((ignoreDups == false) || (lastCharDelimiter == false))
					++column;
				lastCharDelimiter = true;
			}
			else if (x == '\n')
			{
				if (!inGUI) {
					//fill remaining empty fields (database wants them explicity)
					for (int additionalColumn = column; additionalColumn <= maxColumn; additionalColumn++) {
						setText(row - m_startline, additionalColumn, QString::null, inGUI);
					}
				}
				nextRow = true;
				maxColumn = qMax( maxColumn, column );
				column = 1;
			}
			else
			{
				field += x;
				state = S_MAYBE_NORMAL_FIELD;
			}
			break;
		case S_QUOTED_FIELD :
			if (x == m_textquote)
			{
				state = S_MAYBE_END_OF_QUOTED_FIELD;
			}
/*allow \n inside quoted fields
			else if (x == '\n')
			{
				setText(row - m_startline, column, field, inGUI);
				field = "";
				if (x == '\n')
				{
					nextRow = true;
					maxColumn = qMax( maxColumn, column );
					column = 1;
				}
				else
				{
					if ((ignoreDups == false) || (lastCharDelimiter == false))
						++column;
					lastCharDelimiter = true;
				}
				state = S_START;
			}*/
			else
			{
				field += x;
			}
			break;
		case S_MAYBE_END_OF_QUOTED_FIELD :
			if (x == m_textquote)
			{
				field += x; //no, this was just escaped quote character
				state = S_QUOTED_FIELD;
			}
			else if (x == delimiter || x == '\n')
			{
				setText(row - m_startline, column, field, inGUI);
				field = QString::null;
				if (x == '\n')
				{
					nextRow = true;
					maxColumn = qMax( maxColumn, column );
					column = 1;
				}
				else
				{
					if ((ignoreDups == false) || (lastCharDelimiter == false))
						++column;
					lastCharDelimiter = true;
				}
				state = S_START;
			}
			else
			{
				state = S_END_OF_QUOTED_FIELD;
			}
			break;
		case S_END_OF_QUOTED_FIELD :
			if (x == delimiter || x == '\n')
			{
				setText(row - m_startline, column, field, inGUI);
				field = QString::null;
				if (x == '\n')
				{
					nextRow = true;
					maxColumn = qMax( maxColumn, column );
					column = 1;
				}
				else
				{
					if ((ignoreDups == false) || (lastCharDelimiter == false))
						++column;
					lastCharDelimiter = true;
				}
				state = S_START;
			}
			else
			{
				state = S_END_OF_QUOTED_FIELD;
			}
			break;
		case S_MAYBE_NORMAL_FIELD :
			if (x == m_textquote)
			{
				field = QString::null;
				state = S_QUOTED_FIELD;
				break;
			}
		case S_NORMAL_FIELD :
			if (x == delimiter || x == '\n')
			{
				setText(row - m_startline, column, field, inGUI);
				field = QString::null;
				if (x == '\n')
				{
					nextRow = true;
					maxColumn = qMax( maxColumn, column );
					column = 1;
				}
				else
				{
					if ((ignoreDups == false) || (lastCharDelimiter == false))
						++column;
					lastCharDelimiter = true;
				}
				state = S_START;
			}
			else
			{
				field += x;
			}
		}
		if (x != delimiter)
			lastCharDelimiter = false;

		if (nextRow) {
			nextRow = false;
			if (!inGUI && row==1 && m_1stRowForFieldNames->isChecked()) {
				// do not save to the database 1st row if it contains column names
				m_importingStatement->clearArguments();
			}
			else if (!saveRow(inGUI))
				return false;
			++row;
		}

		if (m_firstFillTableCall && row==2 
			&& !m_1stRowForFieldNames->isChecked() && m_1stRowForFieldNamesDetected) 
		{
			//'1st row for field name' flag detected: reload table
			m_1stRowForFieldNamesDetected = false;
			m_table->setNumRows( 0 );
			m_firstFillTableCall = false; //this trick is allowed only once, on startup
			m_1stRowForFieldNames->setChecked(true); //this will reload table
			//slot1stRowForFieldNamesChanged(1);
			m_blockUserEvents = false;
			repaint();
			return false;
		}

		if (!m_importingProgressDlg && row % 20 == 0) {
			qApp->processEvents();
			//only for GUI mode:
			if (!m_firstFillTableCall && m_loadingProgressDlg && m_loadingProgressDlg->wasCanceled()) {
				delete m_loadingProgressDlg;
				m_loadingProgressDlg = 0;
				m_dialogCancelled = true;
				reject();
				return false;
			}
		}

		if (!m_firstFillTableCall && m_loadingProgressDlg) {
			m_loadingProgressDlg->progressBar()->setValue(qMin(m_maximumRowsForPreview, row));
		}

		if ( inGUI && row > (m_maximumRowsForPreview + (m_1stRowForFieldNamesDetected?1:0)) ) {
			kexipluginsdbg << "KexiCSVImportDialog::fillTable() loading stopped at row #" 
				<< m_maximumRowsForPreview << endl;
			break;
		}
	}
	return true;
}

void KexiCSVImportDialog::updateColumnText(int col)
{
	QString colName;
	if (col<(int)m_columnNames.count() && (m_1stRowForFieldNames->isChecked() || m_changedColumnNames[col]))
		colName = m_columnNames[ col ];
	if (colName.isEmpty()) {
		colName = i18n("Column %1").arg(col+1); //will be changed to a valid identifier on import
		m_changedColumnNames[ col ] = false;
	}
	int detectedType = m_detectedTypes[col];
	if (detectedType==_FP_NUMBER_TYPE)
		detectedType=_NUMBER_TYPE; //we're simplifying that for now
	else if (detectedType==_NO_TYPE_YET) {
		m_detectedTypes[col]=_TEXT_TYPE; //entirely empty column
		detectedType=_TEXT_TYPE;
	}
	m_table->horizontalHeader()->setLabel(col, 
		i18n("Column %1").arg(col+1) + "  \n(" + m_typeNames[ detectedType ] + ")  ");
	m_table->setText(0, col, colName);
	m_table->horizontalHeader()->adjustHeaderSize();

	//check uniqueness
	Q3ValueList<int> *list = m_uniquenessTest[col];
	if (m_primaryKeyColumn==-1 && list && !list->isEmpty()) {
		qHeapSort(*list);
		Q3ValueList<int>::ConstIterator it=list->constBegin();
		int prevValue = *it;
		++it;
		for(; it!=list->constEnd() && prevValue!=(*it); ++it)
			prevValue=(*it);
		if (it!=list->constEnd()) {
			//duplicates:
			list->clear();
		}
		else {
			//a candidate for PK (autodetected)!
			if (-1==m_primaryKeyColumn) {
				m_primaryKeyColumn=col;
			}
		}
	}
	if (list) //not needed now: conserve memory
		list->clear();
}

void KexiCSVImportDialog::detectTypeAndUniqueness(int row, int col, const QString& text)
{
	int intValue;
	const int type = m_detectedTypes[col];
	if (row==1 || type!=_TEXT_TYPE) {
		bool found = false;
		if (text.isEmpty() && type==_NO_TYPE_YET)
			found = true; //real type should be found later
		//detect type because it's 1st row or all prev. rows were not text
		//-number?
		if (!found && (row==1 || type==_NUMBER_TYPE || type==_NO_TYPE_YET)) {
			bool ok = text.isEmpty();//empty values allowed
			if (!ok)
				intValue = text.toInt(&ok);
			if (ok && (row==1 || type==_NO_TYPE_YET)) {
				m_detectedTypes[col]=_NUMBER_TYPE;
				found = true; //yes
			}
		}
		//-FP number?
		if (!found && (row==1 || type==_FP_NUMBER_TYPE || type==_NO_TYPE_YET)) {
			bool ok = text.isEmpty() || m_fpNumberRegExp.exactMatch(text);
			if (!ok)
				text.toInt(&ok);
			if (ok && (row==1 || type==_NO_TYPE_YET)) {
				m_detectedTypes[col]=_FP_NUMBER_TYPE;
				found = true; //yes
			}
		}
		//-date?
		if (!found && (row==1 || type==_DATE_TYPE || type==_NO_TYPE_YET)) {
			if ((row==1 || type==_NO_TYPE_YET)
				&& (text.isEmpty() || m_dateRegExp1.exactMatch(text) || m_dateRegExp2.exactMatch(text)))
			{
				m_detectedTypes[col]=_DATE_TYPE;
				found = true; //yes
			}
		}
		//-time?
		if (!found && (row==1 || type==_TIME_TYPE || type==_NO_TYPE_YET)) {
			if ((row==1 || type==_NO_TYPE_YET)
				&& (text.isEmpty() || m_timeRegExp1.exactMatch(text) || m_timeRegExp2.exactMatch(text)))
			{
				m_detectedTypes[col]=_TIME_TYPE;
				found = true; //yes
			}
		}
		//-date/time?
		if (!found && (row==1 || type==_TIME_TYPE || type==_NO_TYPE_YET)) {
			if (row==1 || type==_NO_TYPE_YET) {
				bool detected = text.isEmpty();
				if (!detected) {
					const QStringList dateTimeList( QStringList::split(" ", text) );
					bool ok = dateTimeList.count()>=2;
//! @todo also support ISODateTime's "T" separator?
//! @todo also support timezones?
					if (ok) {
						//try all combinations
						QString datePart( dateTimeList[0].trimmed() );
						QString timePart( dateTimeList[1].trimmed() );
						ok = (m_dateRegExp1.exactMatch(datePart) || m_dateRegExp2.exactMatch(datePart))
							&& (m_timeRegExp1.exactMatch(timePart) || m_timeRegExp2.exactMatch(timePart));
					}
					detected = ok;
				}
				if (detected) {
					m_detectedTypes[col]=_DATETIME_TYPE;
					found = true; //yes
				}
			}
		}
		if (!found && type==_NO_TYPE_YET && !text.isEmpty()) {
			//eventually, a non-emptytext after a while
			m_detectedTypes[col]=_TEXT_TYPE;
			found = true; //yes
		}
		//default: text type (already set)
	}
	//check uniqueness for this value
	Q3ValueList<int> *list = m_uniquenessTest[col];
	if (row==1 && (!list || !list->isEmpty()) && !text.isEmpty() && _NUMBER_TYPE == m_detectedTypes[col]) {
		if (!list) {
			list = new Q3ValueList<int>();
			m_uniquenessTest.insert(col, list);
		}
		list->append( intValue );
	}
	else {
		//the value is empty or uniqueness test failed in the past
		if (list && !list->isEmpty())
			list->clear(); //indicate that uniqueness test failed
	}
}

void KexiCSVImportDialog::setText(int row, int col, const QString& text, bool inGUI)
{
	if (!inGUI) {
		//save text directly to database buffer
		if (col==1) { //1st col
			m_importingStatement->clearArguments();
			if (m_implicitPrimaryKeyAdded)
				*m_importingStatement << QVariant(); //id will be autogenerated here
		}
		const int detectedType = m_detectedTypes[col-1];
		if (detectedType==_NUMBER_TYPE) {
			*m_importingStatement << ( text.isEmpty() ? QVariant() : text.toInt() );
//! @todo what about time and float/double types and different integer subtypes?
		}
		else if (detectedType==_FP_NUMBER_TYPE) {
			*m_importingStatement << ( text.isEmpty() ? QVariant() : text.toDouble() );
		}
		else if (detectedType==_DATE_TYPE) {
			QDate date( QDate::fromString(text, Qt::ISODate) ); //same as m_dateRegExp1
			if (!date.isValid() && m_dateRegExp2.exactMatch(text)) //dd-mm-yyyy
				date = QDate(text.mid(6,4).toInt(), text.mid(3,2).toInt(), text.left(2).toInt());
			*m_importingStatement << date;
		}
		else if (detectedType==_TIME_TYPE) {
			QTime time( QTime::fromString(text, Qt::ISODate) ); //same as m_timeRegExp1
			if (!time.isValid() && m_timeRegExp2.exactMatch(text)) //hh:mm:ss
				time = QTime(text.left(2).toInt(), text.mid(3,2).toInt(), text.mid(6,2).toInt());
			*m_importingStatement << time;
		}
		else if (detectedType==_DATETIME_TYPE) {
			const QStringList dateTimeList( QStringList::split(" ", text) );
//! @todo also support ISODateTime's "T" separator?
//! @todo also support timezones?
			if (dateTimeList.count()>=2) {
				//try all combinations
				QString datePart( dateTimeList[0].trimmed() );
				QString timePart( dateTimeList[1].trimmed() );
				QDateTime dateTime;
				dateTime.setDate( QDate::fromString(datePart, Qt::ISODate) ); //same as m_dateRegExp1
				if (!dateTime.date().isValid() && m_dateRegExp2.exactMatch(datePart)) //dd-mm-yyyy
					dateTime.setDate( QDate(datePart.mid(6,4).toInt(), 
						datePart.mid(3,2).toInt(), datePart.left(2).toInt()) );
				dateTime.setTime( QTime::fromString(timePart, Qt::ISODate) ); //same as m_timeRegExp1
				if (!dateTime.time().isValid() && m_timeRegExp2.exactMatch(timePart)) //hh:mm:ss
					dateTime.setTime( QTime(timePart.left(2).toInt(), 
						timePart.mid(3,2).toInt(), timePart.mid(6,2).toInt()) );
				*m_importingStatement << dateTime;
			}
		}
		else //_TEXT_TYPE and the rest
			*m_importingStatement << text;
		return;
	}
	//save text to GUI (table view)
	if (m_table->numCols() < col) {
		m_table->setNumCols(col);
		if ((int)m_columnNames.size() < m_table->numCols()) {
			m_columnNames.resize(m_table->numCols()+10);
			m_changedColumnNames.resize(m_table->numCols()+10);
		}
	}

	if (m_1stRowForFieldNames->isChecked()) {
		if ((row+m_startline)==1) {//this is for column name
			if ((col-1) < (int)m_changedColumnNames.size() && false==m_changedColumnNames[col-1]) {
				//this column has no custom name entered by a user
				//-get the name from the data cell
				QString colName(text.simplified());
				if (!colName.isEmpty()) {
					if (colName.left(1)>="0" && colName.left(1)<="9")
						colName.prepend(i18n("Column")+" ");
					m_columnNames[ col-1 ] = colName;
				}
			}
			return;
		}
	}
	else {
		if ((row+m_startline)==1) {//this row is for column name
			if (m_1stRowForFieldNamesDetected && !m_1stRowForFieldNames->isChecked()) {
				QString f( text.simplified() );
				if (f.isEmpty() || !f[0].isLetter())
					m_1stRowForFieldNamesDetected = false; //this couldn't be a column name
			}
		}
		row++; //1st row was for column names
	}

	if (row < 2) // skipped by the user
		return;

	if (m_table->numRows() < row) {
//		if (m_maximumRowsForPreview >= row+100)
		m_table->setNumRows(row+100); /* We add more rows at a time to limit recalculations */
		//else
//			m_table->setNumRows(m_maximumRowsForPreview);
		m_table->verticalHeader()->setLabel(0, i18n("Column name")+"   ");
		m_adjustRows=1;
	}

	m_table->setText(row - 1, col - 1, text);
	m_table->verticalHeader()->setLabel(row-1, QString::number(row-1));

	detectTypeAndUniqueness(row-1, col-1, text);
}

bool KexiCSVImportDialog::saveRow(bool inGUI)
{
	if (inGUI) {
		//nothing to do
		return true;
	}
	//save db buffer
	bool res = m_importingStatement->execute();
//todo: move
	m_importingStatement->clearArguments();
	return res;
//	return m_conn->insertRecord(*m_destinationTableSchema, m_dbRowBuffer);
}

void KexiCSVImportDialog::adjustRows(int iRows)
{
	if (m_adjustRows)
	{
		m_table->setNumRows( iRows );
		m_adjustRows=0;
		for (int i = 0; i<iRows; i++)
			m_table->adjustRow(i);
	}
}

void KexiCSVImportDialog::formatChanged(int id)
{
	if (id==_PK_FLAG) {
		if (m_primaryKeyColumn>=0 && m_primaryKeyColumn<m_table->numCols()) {
			m_table->setPixmap(0, m_primaryKeyColumn, QPixmap());
		}
		if (m_primaryKeyField->isChecked()) {
			m_primaryKeyColumn = m_table->currentColumn();
			m_table->setPixmap(0, m_primaryKeyColumn, m_pkIcon);
		}
		else
			m_primaryKeyColumn = -1;
		return;
	}
	else {
		m_detectedTypes[m_table->currentColumn()]=id;
		m_primaryKeyField->setEnabled( _NUMBER_TYPE == id );
		m_primaryKeyField->setChecked( m_primaryKeyColumn == m_table->currentColumn() && m_primaryKeyField->isEnabled() );
	}
	updateColumnText(m_table->currentColumn());
}

void KexiCSVImportDialog::delimiterChanged(const QString& delimiter)
{
	Q_UNUSED(delimiter);
	m_columnsAdjusted = false;
	//delayed, otherwise combobox won't be repainted
	QTimer::singleShot(10, this, SLOT(fillTable()));
}

void KexiCSVImportDialog::textquoteSelected(int)
{
	const QString tq(m_comboQuote->textQuote());
	if (tq.isEmpty())
		m_textquote = 0;
	else
		m_textquote = tq[0];

	//delayed, otherwise combobox won't be repainted
	QTimer::singleShot(10, this, SLOT(fillTable()));
}

void KexiCSVImportDialog::startlineSelected(int startline)
{
//	const int startline = line.toInt() - 1;
	if (m_startline == (startline-1))
		return;
	m_startline = startline-1;
	m_adjustRows=1;
	fillTable();
	m_table->setFocus();
}

void KexiCSVImportDialog::currentCellChanged(int, int col)
{
	if (m_prevSelectedCol==col)
		return;
	m_prevSelectedCol = col;
	int type = m_detectedTypes[col];
	if (type==_FP_NUMBER_TYPE)
		type=_NUMBER_TYPE; //we're simplifying that for now

	m_formatCombo->setCurrentItem( type );
	m_formatLabel->setText( m_formatComboText.arg(col+1) );
	m_primaryKeyField->setEnabled( _NUMBER_TYPE == m_detectedTypes[col]);
	m_primaryKeyField->blockSignals(true); //block to disable executing slotPrimaryKeyFieldToggled()
	 m_primaryKeyField->setChecked( m_primaryKeyColumn == col );
	m_primaryKeyField->blockSignals(false);
}

void KexiCSVImportDialog::cellValueChanged(int row,int col)
{
	if (row==0) {//column name has changed
		m_columnNames[ col ] = m_table->text(row, col);
		m_changedColumnNames.setBit( col );
	}
}

void KexiCSVImportDialog::accept()
{
//! @todo MOVE MOST OF THIS TO CORE/ (KexiProject?) after KexiDialogBase code is moved to non-gui place

	KexiGUIMessageHandler msg; //! @todo make it better integrated with main window

	const uint numRows( m_table->numRows() );
	if (numRows == 0)
		return; //impossible

	if (numRows == 1) {
		if (KMessageBox::No == KMessageBox::questionYesNo(this, 
			i18n("Data set contains no rows. Do you want to import empty table?")))
			return;
	}

	KexiProject* project = m_mainWin->project();
	if (!project) {
		msg.showErrorMessage(i18n("No project available."));
		return;
	}
	m_conn = project->dbConnection(); //cache this pointer
	if (!m_conn) {
		msg.showErrorMessage(i18n("No database connection available."));
		return;
	}
	KexiPart::Part *part = Kexi::partManager().partForMimeType("kexi/table");
	if (!part) {
		msg.showErrorMessage(&Kexi::partManager());
		return;
	}

	//get suggested name based on the file name
	QString suggestedName;
	if (m_mode==File) {
		suggestedName = KUrl::fromPathOrURL(m_fname).fileName();
		//remove extension
		if (!suggestedName.isEmpty()) {
			const int idx = suggestedName.findRev(".");
			if (idx!=-1)
				suggestedName = suggestedName.mid(0, idx ).simplified();
		}
	}

	//-new part item
	KexiPart::Item* partItemForSavedTable = project->createPartItem(part->info(), suggestedName);
	if (!partItemForSavedTable) {
	//		msg.showErrorMessage(project);
		return;
	}

#define _ERR \
	{ project->deleteUnstoredItem(partItemForSavedTable); \
	  m_conn = 0; \
	  delete m_destinationTableSchema; \
	  m_destinationTableSchema = 0; \
	return; }

	//-ask for table name/title
	// (THIS IS FROM KexiMainWindowImpl::saveObject())
	bool allowOverwriting = true;
	tristate res = m_mainWin->getNewObjectInfo( partItemForSavedTable, part, allowOverwriting );
	if (~res || !res) {
		//! @todo: err
		_ERR;
	}
	//(allowOverwriting is now set to true, if user accepts overwriting, 
	// and overwriting will be needed)

//	KexiDB::SchemaData sdata(part->info()->projectPartID());
//	sdata.setName( partItem->name() );

	//-create table schema (and thus schema object)
	//-assign information (THIS IS FROM KexiDialogBase::storeNewData())
	m_destinationTableSchema = new KexiDB::TableSchema(partItemForSavedTable->name());
	m_destinationTableSchema->setCaption( partItemForSavedTable->caption() );
	m_destinationTableSchema->setDescription( partItemForSavedTable->description() );
	const uint numCols( m_table->numCols() );

	m_implicitPrimaryKeyAdded = false;
	//add PK if user wanted it
	int msgboxResult;
	if (m_primaryKeyColumn==-1
		&& KMessageBox::No != (msgboxResult = KMessageBox::questionYesNoCancel(this, 
			i18n("No Primary Key (autonumber) has been defined.\n"
			"Should it be automatically defined on import (recommended)?\n\n"
			"Note: An imported table without a Primary Key may not be editable (depending on database type)."),
			QString::null, KGuiItem(i18n("Add Database Primary Key to a Table", "Add Primary Key"), "key"),
			KGuiItem(i18n("Do Not Add Database Primary Key to a Table", "Do Not Add")))))
	{
		if (msgboxResult == KMessageBox::Cancel)
			_ERR; //cancel accepting

		//add implicit PK field
//! @todo make this field hidden (what about e.g. pgsql?)
		m_implicitPrimaryKeyAdded = true;

		QString fieldName("id");
		QString fieldCaption("Id");

		QStringList colnames;
		for (uint col = 0; col < numCols; col++)
			colnames.append( m_table->text(0, col).lower().simplified() );

		if (colnames.find(fieldName)!=colnames.end()) {
			int num = 1;
			while (colnames.find(fieldName+QString::number(num))!=colnames.end())
				num++;
			fieldName += QString::number(num);
			fieldCaption += QString::number(num);
		}
		KexiDB::Field *field = new KexiDB::Field(
			fieldName,
			KexiDB::Field::Integer,
			KexiDB::Field::NoConstraints,
			KexiDB::Field::NoOptions,
			0,0, //uint length=0, uint precision=0,
			QVariant(), //QVariant defaultValue=QVariant(),
			fieldCaption
		); //no description and width for now
		field->setPrimaryKey(true);
		field->setAutoIncrement(true);
		m_destinationTableSchema->addField( field );
	}

	for (uint col = 0; col < numCols; col++) {
		QString fieldCaption( m_table->text(0, col).simplified() );
		QString fieldName( KexiUtils::string2Identifier( fieldCaption ) );
		if (m_destinationTableSchema->field(fieldName)) {
			QString fixedFieldName;
			uint i = 2; //"apple 2, apple 3, etc. if there're many "apple" names
			do {
				fixedFieldName = fieldName + "_" + QString::number(i);
				if (!m_destinationTableSchema->field(fixedFieldName))
					break;
				i++;
			} while (true);
			fieldName = fixedFieldName;
			fieldCaption += (" " + QString::number(i));
		}
		const int detectedType = m_detectedTypes[col];
		KexiDB::Field::Type fieldType;
		if (detectedType==_DATE_TYPE)
			fieldType = KexiDB::Field::Date;
		if (detectedType==_TIME_TYPE)
			fieldType = KexiDB::Field::Time;
		if (detectedType==_DATETIME_TYPE)
			fieldType = KexiDB::Field::DateTime;
		else if (detectedType==_NUMBER_TYPE)
			fieldType = KexiDB::Field::Integer;
		else if (detectedType==_FP_NUMBER_TYPE)
			fieldType = KexiDB::Field::Double;
//! @todo what about time and float/double types and different integer subtypes?
		else //_TEXT_TYPE and the rest
			fieldType = KexiDB::Field::Text;
//! @todo what about long text?

		KexiDB::Field *field = new KexiDB::Field(
			fieldName,
			fieldType,
			KexiDB::Field::NoConstraints,
			KexiDB::Field::NoOptions,
			0,0, //uint length=0, uint precision=0,
			QVariant(), //QVariant defaultValue=QVariant(),
			fieldCaption
		); //no description and width for now

		if ((int)col == m_primaryKeyColumn) {
			field->setPrimaryKey(true);
			field->setAutoIncrement(true);
		}
		m_destinationTableSchema->addField( field );
	}

	KexiDB::Transaction transaction = m_conn->beginTransaction();
	if (transaction.isNull()) {
		msg.showErrorMessage(m_conn);
		_ERR;
	}
	KexiDB::TransactionGuard tg(transaction);

	//-create physical table
	if (!m_conn->createTable(m_destinationTableSchema, allowOverwriting)) {
			msg.showErrorMessage(m_conn);
		_ERR;
	}

#define _DROP_DEST_TABLE_AND_RETURN \
	{ \
	if (m_importingProgressDlg) \
		m_importingProgressDlg->hide(); \
	project->deleteUnstoredItem(partItemForSavedTable); \
	m_conn->dropTable(m_destinationTableSchema); /*alsoRemoveSchema*/ \
	m_destinationTableSchema = 0; \
	m_conn = 0; \
	return; \
	}

	m_importingStatement = m_conn->prepareStatement(
		KexiDB::PreparedStatement::InsertStatement, *m_destinationTableSchema);
	if (!m_importingStatement) {
		msg.showErrorMessage(m_conn);
		_DROP_DEST_TABLE_AND_RETURN;
	}

	if (m_file) {
		if (!m_importingProgressDlg) {
			m_importingProgressDlg = new KProgressDialog( this, "m_importingProgressDlg", 
				i18n("Importing CSV Data"), QString::null, true );
		}
		m_importingProgressDlg->setLabel(
			i18n("Importing CSV Data from <nobr>\"%1\"</nobr> into \"%2\" table...")
			.arg(QDir::convertSeparators(m_fname)).arg(m_destinationTableSchema->name()) );
		m_importingProgressDlg->progressBar()->setTotalSteps( QFileInfo(*m_file).size() );
		m_importingProgressDlg->show();
	}

	int row, column, maxColumn;
	QString field = QString::null;

	// main job
	res = loadRows(field, row, column, maxColumn, false /*!gui*/ );

	delete m_importingProgressDlg;
  m_importingProgressDlg = 0;
	if (true != res) {
		//importing cancelled or failed
		if (!res) //do not display err msg when res == cancelled
			msg.showErrorMessage(m_conn);
		_DROP_DEST_TABLE_AND_RETURN;
	}

	// file with only one line without '\n'
	if (field.length() > 0)
	{
		setText(row - m_startline, column, field, false /*!gui*/);
		//fill remaining empty fields (database wants them explicity)
		for (int additionalColumn = column; additionalColumn <= maxColumn; additionalColumn++) {
			setText(row - m_startline, additionalColumn, QString::null, false /*!gui*/);
		}
		if (!saveRow(false /*!gui*/)) {
			msg.showErrorMessage(m_conn);
			_DROP_DEST_TABLE_AND_RETURN;
		}
		++row;
		field = QString::null;
	}

	if (!tg.commit()) {
		msg.showErrorMessage(m_conn);
		_DROP_DEST_TABLE_AND_RETURN;
	}

	//-now we can store the item
	partItemForSavedTable->setIdentifier( m_destinationTableSchema->id() );
	project->addStoredItem( part->info(), partItemForSavedTable );

	QDialog::accept();
	KMessageBox::information(this, i18n("Data has been successfully imported to table \"%1\".")
		.arg(m_destinationTableSchema->name()));
	parentWidget()->raise();
	m_conn = 0;
}

int KexiCSVImportDialog::getHeader(int col)
{
	QString header = m_table->horizontalHeader()->label(col);

	if (header == i18n("Text type for column", "Text"))
		return TEXT;
	else if (header == i18n("Numeric type for column", "Number"))
		return NUMBER;
	else if (header == i18n("Currency type for column", "Currency"))
		return CURRENCY;
	else
		return DATE;
}

QString KexiCSVImportDialog::getText(int row, int col)
{
	return m_table->text(row, col);
}

void KexiCSVImportDialog::ignoreDuplicatesChanged(int)
{
	fillTable();
}

void KexiCSVImportDialog::slot1stRowForFieldNamesChanged(int)
{
	m_adjustRows=1;
	if (m_1stRowForFieldNames->isChecked() && m_startline>0 && m_startline>=(m_startAtLineSpinBox->maxValue()-1))
		m_startline--;
	fillTable();
}

void KexiCSVImportDialog::optionsButtonClicked()
{
	KexiCSVImportOptionsDialog dlg(m_encoding, this);
	if (QDialog::Accepted != dlg.exec())
		return;

	if (m_encoding != dlg.encodingComboBox()->selectedEncoding()) {
		m_encoding = dlg.encodingComboBox()->selectedEncoding();
		if (!openData())
			return;
		fillTable();
	}
}

bool KexiCSVImportDialog::eventFilter ( QObject * watched, QEvent * e )
{
	QEvent::Type t = e->type();
	// temporary disable keyboard and mouse events for time-consuming tasks
	if (m_blockUserEvents && (t==QEvent::KeyPress || t==QEvent::KeyRelease 
		|| t==QEvent::MouseButtonPress || t==QEvent::MouseButtonDblClick
		|| t==QEvent::Paint ))
		return true;

	if (watched == m_startAtLineSpinBox && t==QEvent::KeyPress) {
		QKeyEvent *ke = static_cast<QKeyEvent*>(e);
		if (ke->key()==Qt::Key_Enter || ke->key()==Qt::Key_Return) {
			m_table->setFocus();
			return true;
		}
	}
	return QDialog::eventFilter( watched, e );
}

void KexiCSVImportDialog::slotPrimaryKeyFieldToggled(bool on)
{
	Q_UNUSED(on);
	formatChanged(_PK_FLAG);
}

void KexiCSVImportDialog::updateRowCountInfo()
{
//! @todo infoLbl->setFileName( m_fname + " " + i18n("row count", "(rows: %1)").arg(10);
	m_infoLbl->setFileName( m_fname );
}

#include "kexicsvimportdialog.moc"
