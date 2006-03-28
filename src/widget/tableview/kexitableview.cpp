/* This file is part of the KDE project
   Copyright (C) 2002 Till Busch <till@bux.at>
   Copyright (C) 2003 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2003 Daniel Molkentin <molkentin@kde.org>
   Copyright (C) 2003 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2003-2005 Jaroslaw Staniek <js@iidea.pl>

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

   Original Author:  Till Busch <till@bux.at>
   Original Project: buX (www.bux.at)
*/

#include <qpainter.h>
#include <qkeycode.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qwmatrix.h>
#include <qtimer.h>
#include <qpopupmenu.h>
#include <qcursor.h>
#include <qstyle.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>
#include <kapplication.h>
#include <kiconloader.h>
#include <kmessagebox.h>

#ifndef KEXI_NO_PRINT
# include <kprinter.h>
#endif

#include "kexitableview.h"
#include <kexiutils/utils.h>
#include <kexiutils/validator.h>

#include "kexidatetableedit.h"
#include "kexitimetableedit.h"
#include "kexidatetimetableedit.h"
#include "kexicelleditorfactory.h"
#include "kexitableedit.h"
#include "kexiinputtableedit.h"
#include "kexicomboboxtableedit.h"
#include "kexiblobtableedit.h"
#include "kexibooltableedit.h"
#include "kexitableview_p.h"
#include <widget/utils/kexirecordmarker.h>
#include <widget/utils/kexidisplayutils.h>

KexiTableView::Appearance::Appearance(QWidget *widget)
 : alternateBackgroundColor( KGlobalSettings::alternateBackgroundColor() )
{
	//set defaults
	if (qApp) {
		QPalette p = widget ? widget->palette() : qApp->palette();
		baseColor = p.active().base();
		textColor = p.active().text();
		borderColor = QColor(200,200,200);
		emptyAreaColor = p.active().color(QColorGroup::Base);
		rowHighlightingColor = KexiUtils::blendedColors(p.active().highlight(), baseColor, 33, 66);
		rowMouseOverHighlightingColor = KexiUtils::blendedColors(p.active().highlight(), baseColor, 10, 90);
		rowMouseOverAlternateHighlightingColor = KexiUtils::blendedColors(p.active().highlight(), alternateBackgroundColor, 10, 90);
		rowHighlightingTextColor = textColor;
		rowMouseOverHighlightingTextColor = textColor;
	}
	backgroundAltering = true;
	rowMouseOverHighlightingEnabled = true;
	rowHighlightingEnabled = true;
	persistentSelections = true;
	navigatorEnabled = true;
	fullRowSelection = false;
	gridEnabled = true;
}


//-----------------------------------------

TableViewHeader::TableViewHeader(QWidget * parent, const char * name) 
	: QHeader(parent, name)
	, m_lastToolTipSection(-1)
{
	installEventFilter(this);
	connect(this, SIGNAL(sizeChange(int,int,int)), 
		this, SLOT(slotSizeChange(int,int,int)));
}

int TableViewHeader::addLabel ( const QString & s, int size )
{
	m_toolTips += "";
	slotSizeChange(0,0,0);//refresh
	return QHeader::addLabel(s, size);
}

int TableViewHeader::addLabel ( const QIconSet & iconset, const QString & s, int size )
{
	m_toolTips += "";
	slotSizeChange(0,0,0);//refresh
	return QHeader::addLabel(iconset, s, size);
}

void TableViewHeader::removeLabel( int section )
{
	if (section < 0 || section >= count())
		return;
	QStringList::Iterator it = m_toolTips.begin();
	it += section;
	m_toolTips.remove(it);
	slotSizeChange(0,0,0);//refresh
	QHeader::removeLabel(section);
}

void TableViewHeader::setToolTip( int section, const QString & toolTip )
{
	if (section < 0 || section >= (int)m_toolTips.count())
		return;
	m_toolTips[ section ] = toolTip;
}

bool TableViewHeader::eventFilter(QObject * watched, QEvent * e)
{
	if (e->type()==QEvent::MouseMove) {
		const int section = sectionAt( static_cast<QMouseEvent*>(e)->x() );
		if (section != m_lastToolTipSection && section >= 0 && section < (int)m_toolTips.count()) {
			QToolTip::remove(this, m_toolTipRect);
			QString tip = m_toolTips[ section ];
			if (tip.isEmpty()) { //try label
				QFontMetrics fm(font());
				int minWidth = fm.width( label( section ) ) + style().pixelMetric( QStyle::PM_HeaderMargin );
				QIconSet *iset = iconSet( section );
				if (iset)
					minWidth += (2+iset->pixmap( QIconSet::Small, QIconSet::Normal ).width()); //taken from QHeader::sectionSizeHint()
				if (minWidth > sectionSize( section ))
					tip = label( section );
			}
			if (tip.isEmpty()) {
				m_lastToolTipSection = -1;
			}
			else {
				QToolTip::add(this, m_toolTipRect = sectionRect(section), tip);
				m_lastToolTipSection = section;
			}
		}
	}
//			if (e->type()==QEvent::MouseButtonPress) {
//	todo
//			}
	return QHeader::eventFilter(watched, e);
}

void TableViewHeader::slotSizeChange(int /*section*/, int /*oldSize*/, int /*newSize*/ )
{
	if (m_lastToolTipSection>0)
		QToolTip::remove(this, m_toolTipRect);
	m_lastToolTipSection = -1; //tooltip's rect is now invalid
}

//-----------------------------------------

//! @internal A special What's This class displaying information about a given column
class KexiTableView::WhatsThis : public QWhatsThis
{
	public:
		WhatsThis(KexiTableView* tv) : QWhatsThis(tv), m_tv(tv)
		{
			Q_ASSERT(tv);
		}
		virtual ~WhatsThis()
		{
		}
		virtual QString text( const QPoint & pos)
		{
			const int leftMargin = m_tv->verticalHeaderVisible() ? m_tv->verticalHeader()->width() : 0;
			//const int topMargin = m_tv->horizontalHeaderVisible() ? m_tv->d->pTopHeader->height() : 0;
			//const int bottomMargin = m_tv->d->appearance.navigatorEnabled ? m_tv->m_navPanel->height() : 0;
			if (KexiUtils::hasParent(m_tv->verticalHeader(), m_tv->childAt(pos))) {
				return i18n("Contains a pointer to the currently selected row");
			}
			else if (KexiUtils::hasParent(m_tv->m_navPanel, m_tv->childAt(pos))) {
				return i18n("Row navigator");
//				return QWhatsThis::textFor(m_tv->m_navPanel, QPoint( pos.x(), pos.y() - m_tv->height() + bottomMargin ));
			}
			KexiDB::Field *f = m_tv->field( m_tv->columnAt(pos.x()-leftMargin) );
			if (!f)
				return QString::null;
			return f->description().isEmpty() ? f->captionOrName() : f->description();
		}
		protected:
			KexiTableView *m_tv;
};

//-----------------------------------------

bool KexiTableView_cellEditorFactoriesInitialized = false;

// Initializes standard editor cell editor factories
void KexiTableView::initCellEditorFactories()
{
	if (KexiTableView_cellEditorFactoriesInitialized)
		return;
	KexiCellEditorFactoryItem* item;
	item = new KexiBlobEditorFactoryItem();
	KexiCellEditorFactory::registerItem( *item, KexiDB::Field::BLOB );

	item = new KexiDateEditorFactoryItem();
	KexiCellEditorFactory::registerItem( *item, KexiDB::Field::Date );

	item = new KexiTimeEditorFactoryItem();
	KexiCellEditorFactory::registerItem( *item, KexiDB::Field::Time );

	item = new KexiDateTimeEditorFactoryItem();
	KexiCellEditorFactory::registerItem( *item, KexiDB::Field::DateTime );

	item = new KexiComboBoxEditorFactoryItem();
	KexiCellEditorFactory::registerItem( *item, KexiDB::Field::Enum );

	item = new KexiBoolEditorFactoryItem();
	KexiCellEditorFactory::registerItem( *item, KexiDB::Field::Boolean );

	item = new KexiKIconTableEditorFactoryItem();
	KexiCellEditorFactory::registerItem( *item, KexiDB::Field::Text, "KIcon" );

	//default type
	item = new KexiInputEditorFactoryItem();
	KexiCellEditorFactory::registerItem( *item, KexiDB::Field::InvalidType );

	KexiTableView_cellEditorFactoriesInitialized = true;
}



KexiTableView::KexiTableView(KexiTableViewData* data, QWidget* parent, const char* name)
: QScrollView(parent, name, /*Qt::WRepaintNoErase | */Qt::WStaticContents /*| Qt::WResizeNoErase*/)
, KexiRecordNavigatorHandler()
, KexiSharedActionClient()
, KexiDataAwareObjectInterface()
{
	KexiTableView::initCellEditorFactories();

	d = new KexiTableViewPrivate(this);

	connect( kapp, SIGNAL( settingsChanged(int) ), SLOT( slotSettingsChanged(int) ) );
	slotSettingsChanged(KApplication::SETTINGS_SHORTCUTS);

	m_data = new KexiTableViewData(); //to prevent crash because m_data==0
	m_owner = true;                   //-this will be deleted if needed

	setResizePolicy(Manual);
	viewport()->setBackgroundMode(NoBackground);
//	viewport()->setFocusPolicy(StrongFocus);
	viewport()->setFocusPolicy(WheelFocus);
	setFocusPolicy(WheelFocus); //<--- !!!!! important (was NoFocus), 
	//                             otherwise QApplication::setActiveWindow() won't activate 
	//                             this widget when needed!
//	setFocusProxy(viewport());
	viewport()->installEventFilter(this);

	//setup colors defaults
	setBackgroundMode(PaletteBackground);
//	setEmptyAreaColor(d->appearance.baseColor);//palette().active().color(QColorGroup::Base));

//	d->baseColor = colorGroup().base();
//	d->textColor = colorGroup().text();

//	d->altColor = KGlobalSettings::alternateBackgroundColor();
//	d->grayColor = QColor(200,200,200);
	d->diagonalGrayPattern = QBrush(d->appearance.borderColor, BDiagPattern);

	setLineWidth(1);
	horizontalScrollBar()->installEventFilter(this);
	horizontalScrollBar()->raise();
	verticalScrollBar()->raise();
	
	// setup scrollbar tooltip
	d->scrollBarTip = new QLabel("abc",0, "scrolltip",WStyle_Customize |WStyle_NoBorder|WX11BypassWM|WStyle_StaysOnTop|WStyle_Tool);
	d->scrollBarTip->setPalette(QToolTip::palette());
	d->scrollBarTip->setMargin(2);
	d->scrollBarTip->setIndent(0);
	d->scrollBarTip->setAlignment(AlignCenter);
	d->scrollBarTip->setFrameStyle( QFrame::Plain | QFrame::Box );
	d->scrollBarTip->setLineWidth(1);
	connect(verticalScrollBar(),SIGNAL(sliderReleased()),this,SLOT(vScrollBarSliderReleased()));
	connect(&d->scrollBarTipTimer,SIGNAL(timeout()),this,SLOT(scrollBarTipTimeout()));
	
	//context menu
	m_popup = new KPopupMenu(this, "contextMenu");
#if 0 //moved to mainwindow's actions
	d->menu_id_addRecord = m_popup->insertItem(i18n("Add Record"), this, SLOT(addRecord()), CTRL+Key_Insert);
	d->menu_id_removeRecord = m_popup->insertItem(
		kapp->iconLoader()->loadIcon("button_cancel", KIcon::Small),
		i18n("Remove Record"), this, SLOT(removeRecord()), CTRL+Key_Delete);
#endif

#ifdef Q_WS_WIN
	d->rowHeight = fontMetrics().lineSpacing() + 4;
#else
	d->rowHeight = fontMetrics().lineSpacing() + 1;
#endif

	if(d->rowHeight < 17)
		d->rowHeight = 17;

	d->pUpdateTimer = new QTimer(this);

//	setMargins(14, fontMetrics().height() + 4, 0, 0);

	// Create headers
	d->pTopHeader = new TableViewHeader(this, "topHeader");
	d->pTopHeader->setOrientation(Horizontal);
	d->pTopHeader->setTracking(false);
	d->pTopHeader->setMovingEnabled(false);
	connect(d->pTopHeader, SIGNAL(sizeChange(int,int,int)), this, SLOT(slotTopHeaderSizeChange(int,int,int)));

	m_verticalHeader = new KexiRecordMarker(this, "rm");
	m_verticalHeader->setCellHeight(d->rowHeight);
//	m_verticalHeader->setFixedWidth(d->rowHeight);
	m_verticalHeader->setCurrentRow(-1);

	setMargins(
		QMIN(d->pTopHeader->sizeHint().height(), d->rowHeight),
		d->pTopHeader->sizeHint().height(), 0, 0);

	setupNavigator();

//	setMinimumHeight(horizontalScrollBar()->height() + d->rowHeight + topMargin());

//	navPanelLyr->addStretch(25);
//	enableClipper(true);

	if (data)
		setData( data );

#if 0//(js) doesn't work!
	d->scrollTimer = new QTimer(this);
	connect(d->scrollTimer, SIGNAL(timeout()), this, SLOT(slotAutoScroll()));
#endif

//	setBackgroundAltering(true);
//	setFullRowSelectionEnabled(false);

	setAcceptDrops(true);
	viewport()->setAcceptDrops(true);

	// Connect header, table and scrollbars
	connect(horizontalScrollBar(), SIGNAL(valueChanged(int)), d->pTopHeader, SLOT(setOffset(int)));
	connect(verticalScrollBar(), SIGNAL(valueChanged(int)), m_verticalHeader, SLOT(setOffset(int)));
	connect(d->pTopHeader, SIGNAL(sizeChange(int, int, int)), this, SLOT(slotColumnWidthChanged(int, int, int)));
	connect(d->pTopHeader, SIGNAL(sectionHandleDoubleClicked(int)), this, SLOT(slotSectionHandleDoubleClicked(int)));
	connect(d->pTopHeader, SIGNAL(clicked(int)), this, SLOT(sortColumnInternal(int)));

	connect(d->pUpdateTimer, SIGNAL(timeout()), this, SLOT(slotUpdate()));
	
//	horizontalScrollBar()->show();
	updateScrollBars();
//	resize(sizeHint());
//	updateContents();
//	setMinimumHeight(horizontalScrollBar()->height() + d->rowHeight + topMargin());

//TMP
//setVerticalHeaderVisible(false);
//setHorizontalHeaderVisible(false);

//will be updated by setAppearance:	updateFonts();
	setAppearance(d->appearance); //refresh

	new WhatsThis(this);
}

KexiTableView::~KexiTableView()
{
	cancelRowEdit();

	KexiTableViewData *data = m_data;
	m_data = 0;
	if (m_owner) {
		if (data)
			data->deleteLater();
	}
	delete d;
}

void KexiTableView::clearVariables()
{
	KexiDataAwareObjectInterface::clearVariables();
	d->clearVariables();
}

/*void KexiTableView::initActions(KActionCollection *ac)
{
	emit reloadActions(ac);
}*/

void KexiTableView::setupNavigator()
{
	updateScrollBars();
	
	m_navPanel = new KexiRecordNavigator(this, leftMargin(), "navPanel");
	m_navPanel->setRecordHandler(this);
	m_navPanel->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Preferred);
}

void KexiTableView::initDataContents()
{
	updateWidgetContentsSize();

	KexiDataAwareObjectInterface::initDataContents();

	m_navPanel->showEditingIndicator(false);
}

void KexiTableView::addHeaderColumn(const QString& caption, const QString& description, int width)
{
	const int nr = d->pTopHeader->count();
	d->pTopHeader->addLabel(caption, width);
	if (!description.isEmpty())
		d->pTopHeader->setToolTip(nr, description);
}

void KexiTableView::updateWidgetContentsSize()
{
	QSize s(tableSize());
	resizeContents(s.width(), s.height());
}

void KexiTableView::slotRowsDeleted( const QValueList<int> &rows )
{
	viewport()->repaint();
	updateWidgetContentsSize();
	setCursorPosition(QMAX(0, (int)m_curRow - (int)rows.count()), -1, true);
}


/*void KexiTableView::addDropFilter(const QString &filter)
{
	d->dropFilters.append(filter);
	viewport()->setAcceptDrops(true);
}*/

void KexiTableView::setFont( const QFont &font )
{
	QScrollView::setFont(font);
	updateFonts(true);
}

void KexiTableView::updateFonts(bool repaint)
{
#ifdef Q_WS_WIN
	d->rowHeight = fontMetrics().lineSpacing() + 4;
#else
	d->rowHeight = fontMetrics().lineSpacing() + 1;
#endif
	if (d->appearance.fullRowSelection) {
		d->rowHeight -= 1;
	}
	if(d->rowHeight < 17)
		d->rowHeight = 17;
//	if(d->rowHeight < 22)
//		d->rowHeight = 22;
	setMargins(
		QMIN(d->pTopHeader->sizeHint().height(), d->rowHeight),
		d->pTopHeader->sizeHint().height(), 0, 0);
//	setMargins(14, d->rowHeight, 0, 0);
	m_verticalHeader->setCellHeight(d->rowHeight);

	KexiDisplayUtils::initDisplayForAutonumberSign(d->autonumberSignDisplayParameters, this);

	if (repaint)
		updateContents();
}

void KexiTableView::updateAllVisibleRowsBelow(int row)
{
	//get last visible row
	int r = rowAt(clipper()->height()+contentsY());
	if (r==-1) {
		r = rows()+1+(isInsertingEnabled()?1:0);
	}
	//update all visible rows below 
	int leftcol = d->pTopHeader->sectionAt( d->pTopHeader->offset() );
//	int row = m_curRow;
	updateContents( columnPos( leftcol ), rowPos(row), 
		clipper()->width(), clipper()->height() - (rowPos(row) - contentsY()) );
}

void KexiTableView::clearColumnsInternal(bool /*repaint*/)
{
	while(d->pTopHeader->count()>0)
		d->pTopHeader->removeLabel(0);
}

void KexiTableView::slotUpdate()
{
//	kdDebug(44021) << " KexiTableView::slotUpdate() -- " << endl;
//	QSize s(tableSize());
//	viewport()->setUpdatesEnabled(false);
///	resizeContents(s.width(), s.height());
//	viewport()->setUpdatesEnabled(true);

	updateContents();
	updateScrollBars();
	if (m_navPanel)
		m_navPanel->updateGeometry(leftMargin());
//	updateNavPanelGeometry();

	updateWidgetContentsSize();
//	updateContents(0, contentsY()+clipper()->height()-2*d->rowHeight, clipper()->width(), d->rowHeight*3);
	
	//updateGeometries();
//	updateContents(0, 0, viewport()->width(), contentsHeight());
//	updateGeometries();
}

int KexiTableView::currentLocalSortingOrder() const
{
	if (d->pTopHeader->sortIndicatorSection()==-1)
		return 0;
	return (d->pTopHeader->sortIndicatorOrder() == Qt::Ascending) ? 1 : -1;
}

void KexiTableView::setLocalSortingOrder(int col, int order)
{
	if (order == 0)
		col = -1;
	if (col>=0)
		d->pTopHeader->setSortIndicator(col, (order==1) ? Qt::Ascending : Qt::Descending);
}

int KexiTableView::currentLocalSortColumn() const
{
	return d->pTopHeader->sortIndicatorSection();
}

void KexiTableView::updateGUIAfterSorting()
{
	int cw = columnWidth(m_curCol);
	int rh = rowHeight();

//	m_verticalHeader->setCurrentRow(m_curRow);
	center(columnPos(m_curCol) + cw / 2, rowPos(m_curRow) + rh / 2);
//	updateCell(oldRow, m_curCol);
//	updateCell(m_curRow, m_curCol);
//	slotUpdate();

	updateContents();
//	d->pUpdateTimer->start(1,true);
}

QSizePolicy KexiTableView::sizePolicy() const
{
	// this widget is expandable
	return QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

QSize KexiTableView::sizeHint() const
{
	const QSize &ts = tableSize();
	int w = QMAX( ts.width() + leftMargin()+ verticalScrollBar()->sizeHint().width() + 2*2, 
		(m_navPanel->isVisible() ? m_navPanel->width() : 0) );
	int h = QMAX( ts.height()+topMargin()+horizontalScrollBar()->sizeHint().height(), 
		minimumSizeHint().height() );
	w = QMIN( w, qApp->desktop()->width()*3/4 ); //stretch
	h = QMIN( h, qApp->desktop()->height()*3/4 ); //stretch

//	kdDebug() << "KexiTableView::sizeHint()= " <<w <<", " <<h << endl;

	return QSize(w, h);
		/*QSize(
		QMAX( ts.width() + leftMargin() + 2*2, (m_navPanel ? m_navPanel->width() : 0) ),
		//+ QMIN(m_verticalHeader->width(),d->rowHeight) + margin()*2,
		QMAX( ts.height()+topMargin()+horizontalScrollBar()->sizeHint().height(), 
			minimumSizeHint().height() )
	);*/
//		QMAX(ts.height() + topMargin(), minimumSizeHint().height()) );
}

QSize KexiTableView::minimumSizeHint() const
{
	return QSize(
		leftMargin() + ((columns()>0)?columnWidth(0):KEXI_DEFAULT_DATA_COLUMN_WIDTH) + 2*2, 
		d->rowHeight*5/2 + topMargin() + (m_navPanel && m_navPanel->isVisible() ? m_navPanel->height() : 0)
	);
}

void KexiTableView::createBuffer(int width, int height)
{
	if(!d->pBufferPm)
		d->pBufferPm = new QPixmap(width, height);
	else
		if(d->pBufferPm->width() < width || d->pBufferPm->height() < height)
			d->pBufferPm->resize(width, height);
//	d->pBufferPm->fill();
}

//internal
inline void KexiTableView::paintRow(KexiTableItem *item,
	QPainter *pb, int r, int rowp, int cx, int cy, 
	int colfirst, int collast, int maxwc)
{
	if (!item)
		return;
	// Go through the columns in the row r
	// if we know from where to where, go through [colfirst, collast],
	// else go through all of them
	if (colfirst==-1)
		colfirst=0;
	if (collast==-1)
		collast=columns()-1;

	int transly = rowp-cy;

	if (d->appearance.rowHighlightingEnabled && r == m_curRow && !d->appearance.fullRowSelection) {
		pb->fillRect(0, transly, maxwc, d->rowHeight, d->appearance.rowHighlightingColor);
	}
	else if (d->appearance.rowMouseOverHighlightingEnabled && r == d->highlightedRow) {
		if(d->appearance.backgroundAltering && (r%2 != 0))
			pb->fillRect(0, transly, maxwc, d->rowHeight, d->appearance.rowMouseOverAlternateHighlightingColor);
		else
			pb->fillRect(0, transly, maxwc, d->rowHeight, d->appearance.rowMouseOverHighlightingColor);
	}
	else {
		if(d->appearance.backgroundAltering && (r%2 != 0))
			pb->fillRect(0, transly, maxwc, d->rowHeight, d->appearance.alternateBackgroundColor);
		else
			pb->fillRect(0, transly, maxwc, d->rowHeight, d->appearance.baseColor);
	}

	for(int c = colfirst; c <= collast; c++)
	{
		// get position and width of column c
		int colp = columnPos(c);
		if (colp==-1)
			continue; //invisible column?
		int colw = columnWidth(c);
		int translx = colp-cx;

		// Translate painter and draw the cell
		pb->saveWorldMatrix();
		pb->translate(translx, transly);
			paintCell( pb, item, c, r, QRect(colp, rowp, colw, d->rowHeight));
		pb->restoreWorldMatrix();
	}

	if (m_dragIndicatorLine>=0) {
		int y_line = -1;
		if (r==(rows()-1) && m_dragIndicatorLine==rows()) {
			y_line = transly+d->rowHeight-3; //draw at last line
		}
		if (m_dragIndicatorLine==r) {
			y_line = transly+1;
		}
		if (y_line>=0) {
			RasterOp op = pb->rasterOp();
			pb->setRasterOp(XorROP);
			pb->setPen( QPen(white, 3) );
			pb->drawLine(0, y_line, maxwc, y_line);
			pb->setRasterOp(op);
		}
	}
}

void KexiTableView::drawContents( QPainter *p, int cx, int cy, int cw, int ch)
{
	if (d->disableDrawContents)
		return;
	int colfirst = columnAt(cx);
	int rowfirst = rowAt(cy);
	int collast = columnAt(cx + cw-1);
	int rowlast = rowAt(cy + ch-1);
	bool inserting = isInsertingEnabled();
	bool plus1row = false; //true if we should show 'inserting' row at the end
	bool paintOnlyInsertRow = false;

/*	kdDebug(44021) << QString(" KexiTableView::drawContents(cx:%1 cy:%2 cw:%3 ch:%4)")
			.arg(cx).arg(cy).arg(cw).arg(ch) << endl;*/

	if (rowlast == -1) {
		rowlast = rows() - 1;
		plus1row = inserting;
		if (rowfirst == -1) {
			if (rowAt(cy - d->rowHeight) != -1) {
				paintOnlyInsertRow = true;
//				kdDebug(44021) << "-- paintOnlyInsertRow --" << endl;
			}
		}
	}
//	kdDebug(44021) << "rowfirst="<<rowfirst<<" rowlast="<<rowlast<<" rows()="<<rows()<<endl;
//	kdDebug(44021)<<" plus1row=" << plus1row<<endl;
	
	if ( collast == -1 )
		collast = columns() - 1;

	if (colfirst>collast) {
		int tmp = colfirst;
		colfirst = collast;
		collast = tmp;
	}
	if (rowfirst>rowlast) {
		int tmp = rowfirst;
		rowfirst = rowlast;
		rowlast = tmp;
	}

// 	qDebug("cx:%3d cy:%3d w:%3d h:%3d col:%2d..%2d row:%2d..%2d tsize:%4d,%4d", 
//	cx, cy, cw, ch, colfirst, collast, rowfirst, rowlast, tableSize().width(), tableSize().height());
//	triggerUpdate();

	if (rowfirst == -1 || colfirst == -1) {
		if (!paintOnlyInsertRow && !plus1row) {
			paintEmptyArea(p, cx, cy, cw, ch);
			return;
		}
	}

	createBuffer(cw, ch);
	if(d->pBufferPm->isNull())
		return;
	QPainter *pb = new QPainter(d->pBufferPm, this);
//	pb->fillRect(0, 0, cw, ch, colorGroup().base());

//	int maxwc = QMIN(cw, (columnPos(d->numCols - 1) + columnWidth(d->numCols - 1)));
	int maxwc = columnPos(columns() - 1) + columnWidth(columns() - 1);
//	kdDebug(44021) << "KexiTableView::drawContents(): maxwc: " << maxwc << endl;

	pb->fillRect(cx, cy, cw, ch, d->appearance.baseColor);

	int rowp;
	int r;
	if (paintOnlyInsertRow) {
		r = rows();
		rowp = rowPos(r); // 'insert' row's position
	}
	else {
		QPtrListIterator<KexiTableItem> it = m_data->iterator();
		it += rowfirst;//move to 1st row
		rowp = rowPos(rowfirst); // row position 
		for (r = rowfirst;r <= rowlast; r++, ++it, rowp+=d->rowHeight) {
			paintRow(it.current(), pb, r, rowp, cx, cy, colfirst, collast, maxwc);
		}
	}

	if (plus1row) { //additional - 'insert' row
		paintRow(m_insertItem, pb, r, rowp, cx, cy, colfirst, collast, maxwc);
	}

	delete pb;

	p->drawPixmap(cx,cy,*d->pBufferPm, 0,0,cw,ch);

  //(js)
	paintEmptyArea(p, cx, cy, cw, ch);
}

void KexiTableView::paintCell(QPainter* p, KexiTableItem *item, int col, int row, const QRect &cr, bool print)
{
	p->save();
//	kdDebug() <<"KexiTableView::paintCell(col=" << col <<"row="<<row<<")"<<endl;
	Q_UNUSED(print);
	int w = cr.width();
	int h = cr.height();
	int x2 = w - 1;
	int y2 = h - 1;

	//	Draw our lines
	QPen pen(p->pen());

	if (d->appearance.gridEnabled) {
		p->setPen(d->appearance.borderColor);
		p->drawLine( x2, 0, x2, y2 );	// right
		p->drawLine( 0, y2, x2, y2 );	// bottom
	}
	p->setPen(pen);

	if (m_editor && row == m_curRow && col == m_curCol //don't paint contents of edited cell
		&& m_editor->hasFocusableWidget() //..if it's visible
	   ) {
		p->restore();
		return;
	}

	KexiTableEdit *edit = dynamic_cast<KexiTableEdit*>( editor( col, /*ignoreMissingEditor=*/true ) );
//	if (!edit)
//		return;

/*
#ifdef Q_WS_WIN
	int x = 1;
	int y_offset = -1;
#else
	int x = 1;
	int y_offset = 0;
#endif

//	const int ctype = columnType(col);*/
//	int x=1;
	int x = edit ? edit->leftMargin() : 0;
	int y_offset=0;

	int align = SingleLine | AlignVCenter;
	QString txt; //text to draw

	QVariant cell_value;
	if ((uint)col < item->count()) {
		if (m_currentItem == item) {
			if (m_editor && row == m_curRow && col == m_curCol 
				&& !m_editor->hasFocusableWidget())
			{
				//we're over editing cell and the editor has no widget
				// - we're displaying internal values, not buffered
//				bool ok;
				cell_value = m_editor->value();
			}
			else {
				//we're displaying values from edit buffer, if available
				cell_value = *bufferedValueAt(col);
			}
		}
		else {
			cell_value = item->at(col);
		}
	}

	if (edit)
		edit->setupContents( p, m_currentItem == item && col == m_curCol, 
			cell_value, txt, align, x, y_offset, w, h );

	if (!d->appearance.gridEnabled)
		y_offset++; //correction because we're not drawing cell borders

/*
	if (KexiDB::Field::isFPNumericType( ctype )) {
#ifdef Q_WS_WIN
#else
			x = 0;
#endif
//js TODO: ADD OPTION to desplaying NULL VALUES as e.g. "(null)"
		if (!cell_value.isNull())
			txt = KGlobal::locale()->formatNumber(cell_value.toDouble());
		w -= 6;
		align |= AlignRight;
	}
	else if (ctype == KexiDB::Field::Enum)
	{
		txt = m_data->column(col)->field->enumHints().at(cell_value.toInt());
		align |= AlignLeft;
	}
	else if (KexiDB::Field::isIntegerType( ctype )) {
		int num = cell_value.toInt();
#ifdef Q_WS_WIN
		x = 1;
#else
		x = 0;
#endif
		w -= 6;
		align |= AlignRight;
		if (!cell_value.isNull())
			txt = QString::number(num);
	}
	else if (ctype == KexiDB::Field::Boolean) {
		int s = QMAX(h - 5, 12);
		QRect r(w/2 - s/2 + x, h/2 - s/2 - 1, s, s);
		p->setPen(QPen(colorGroup().text(), 1));
		p->drawRect(r);
		if (cell_value.asBool())
		{
			p->drawLine(r.x() + 2, r.y() + 2, r.right() - 1, r.bottom() - 1);
			p->drawLine(r.x() + 2, r.bottom() - 2, r.right() - 1, r.y() + 1);
		}
	}
	else if (ctype == KexiDB::Field::Date) { //todo: datetime & time
#ifdef Q_WS_WIN
		x = 5;
#else
		x = 5;
#endif
		if(cell_value.toDate().isValid())
		{
#ifdef USE_KDE
			txt = KGlobal::locale()->formatDate(cell_value.toDate(), true);
#else
			if (!cell_value.isNull())
				txt = cell_value.toDate().toString(Qt::LocalDate);
#endif
		}
		align |= AlignLeft;
	}
	else {//default:
#ifdef Q_WS_WIN
		x = 5;
//		y_offset = -1;
#else
		x = 5;
//		y_offset = 0;
#endif
		if (!cell_value.isNull())
			txt = cell_value.toString();
		align |= AlignLeft;
	}*/
	
	// draw selection background
//	const bool has_focus = hasFocus() || viewport()->hasFocus() || m_popup->hasFocus();

	const bool columnReadOnly = m_data->column(col)->isReadOnly();

	const bool dontPaintNonpersistentSelectionBecauseDifferentRowHasBeenHighlighted 
		= d->appearance.rowHighlightingEnabled && !d->appearance.persistentSelections 
			&& m_curRow /*d->highlightedRow*/ >= 0 && row != m_curRow; //d->highlightedRow;

	if (m_currentItem == item && col == m_curCol) {
		if (edit && (d->appearance.rowHighlightingEnabled && !d->appearance.fullRowSelection || (row == m_curRow && d->highlightedRow==-1 && d->appearance.fullRowSelection))) //!dontPaintNonpersistentSelectionBecauseDifferentRowHasBeenHighlighted)
			edit->paintSelectionBackground( p, isEnabled(), txt, align, x, y_offset, w, h,
				isEnabled() ? colorGroup().highlight() : QColor(200,200,200),//d->grayColor,
				columnReadOnly, d->appearance.fullRowSelection );
	}

/*
	if (!txt.isEmpty() && m_currentItem == item 
		&& col == m_curCol && !columnReadOnly) //js: && !d->recordIndicator)
	{
		QRect bound=fontMetrics().boundingRect(x, y_offset, w - (x+x), h, align, txt);
		bound.setX(bound.x()-1);
		bound.setY(0);
		bound.setWidth( QMIN( bound.width()+2, w - (x+x)+1 ) );
		bound.setHeight(d->rowHeight-1);
		if (has_focus)
			p->fillRect(bound, colorGroup().highlight());
		else
			p->fillRect(bound, gray);
	}
*/	
	if (!edit){
		p->fillRect(0, 0, x2, y2, d->diagonalGrayPattern);
	}

//	If we are in the focus cell, draw indication
	if(m_currentItem == item && col == m_curCol //js: && !d->recordIndicator)
		&& !d->appearance.fullRowSelection) 
	{
//		kdDebug() << ">>> CURRENT CELL ("<<m_curCol<<"," << m_curRow<<") focus="<<has_focus<<endl;
//		if (has_focus) {
		if (isEnabled()) {
			p->setPen(d->appearance.textColor);
		}
		else {
			QPen gray_pen(p->pen());
			gray_pen.setColor(d->appearance.borderColor);
			p->setPen(gray_pen);
		}
		if (edit)
			edit->paintFocusBorders( p, cell_value, 0, 0, x2, y2 );
		else
			p->drawRect(0, 0, x2, y2);
	}

///	bool autonumber = false;
	if ((!m_newRowEditing &&item == m_insertItem) 
		|| (m_newRowEditing && item == m_currentItem && cell_value.isNull())) {
		//we're in "insert row"
		if (m_data->column(col)->field()->isAutoIncrement()) {
			//"autonumber" column
//			txt = i18n("(autonumber)");
//			autonumber = true;
//		if (autonumber) {
			KexiDisplayUtils::drawAutonumberSign(d->autonumberSignDisplayParameters, p, 
				x, y_offset, w - x - x - ((align & Qt::AlignLeft)?2:0), h, align);
//		}
		}
	}
	
	// draw text
	if (!txt.isEmpty()) {
		if (d->appearance.fullRowSelection && (row == d->highlightedRow || (row == m_curRow && d->highlightedRow==-1)) ) 
			p->setPen(d->appearance.rowHighlightingTextColor); //special case
		else if (d->appearance.fullRowSelection && row == m_curRow) 
			p->setPen(d->appearance.textColor); //special case for full row selection
		else if (m_currentItem == item && col == m_curCol && !columnReadOnly 
			 && !dontPaintNonpersistentSelectionBecauseDifferentRowHasBeenHighlighted)
			p->setPen(colorGroup().highlightedText());
		else if (d->appearance.rowHighlightingEnabled && row == m_curRow /*d->highlightedRow*/ && !dontPaintNonpersistentSelectionBecauseDifferentRowHasBeenHighlighted)
			p->setPen(d->appearance.rowHighlightingTextColor);
		else if (d->appearance.rowMouseOverHighlightingEnabled && row == d->highlightedRow && !dontPaintNonpersistentSelectionBecauseDifferentRowHasBeenHighlighted)
			p->setPen(d->appearance.rowMouseOverHighlightingTextColor);
		else
			p->setPen(d->appearance.textColor);
		p->drawText(x, y_offset, w - (x + x)- ((align & AlignLeft)?2:0)/*right space*/, h,
			align, txt);
	}
	p->restore();
}

QPoint KexiTableView::contentsToViewport2( const QPoint &p )
{
	return QPoint( p.x() - contentsX(), p.y() - contentsY() );
}

void KexiTableView::contentsToViewport2( int x, int y, int& vx, int& vy )
{
	const QPoint v = contentsToViewport2( QPoint( x, y ) );
	vx = v.x();
	vy = v.y();
}

QPoint KexiTableView::viewportToContents2( const QPoint& vp )
{
	return QPoint( vp.x() + contentsX(),
		   vp.y() + contentsY() );
}

void KexiTableView::paintEmptyArea( QPainter *p, int cx, int cy, int cw, int ch )
{
//  qDebug("%s: paintEmptyArea(x:%d y:%d w:%d h:%d)", (const char*)parentWidget()->caption(),cx,cy,cw,ch);

	// Regions work with shorts, so avoid an overflow and adjust the
	// table size to the visible size
	QSize ts( tableSize() );
//	ts.setWidth( QMIN( ts.width(), visibleWidth() ) );
//	ts.setHeight( QMIN( ts.height() - (m_navPanel ? m_navPanel->height() : 0), visibleHeight()) );
/*	kdDebug(44021) << QString(" (cx:%1 cy:%2 cw:%3 ch:%4)")
			.arg(cx).arg(cy).arg(cw).arg(ch) << endl;
	kdDebug(44021) << QString(" (w:%3 h:%4)")
			.arg(ts.width()).arg(ts.height()) << endl;*/
	
	// Region of the rect we should draw, calculated in viewport
	// coordinates, as a region can't handle bigger coordinates
	contentsToViewport2( cx, cy, cx, cy );
	QRegion reg( QRect( cx, cy, cw, ch ) );

//kdDebug() << "---cy-- " << contentsY() << endl;

	// Subtract the table from it
//	reg = reg.subtract( QRect( QPoint( 0, 0 ), ts-QSize(0,m_navPanel->isVisible() ? m_navPanel->height() : 0) ) );
	reg = reg.subtract( QRect( QPoint( 0, 0 ), ts
		-QSize(0,QMAX((m_navPanel ? m_navPanel->height() : 0), horizontalScrollBar()->sizeHint().height())
			- (horizontalScrollBar()->isVisible() ? horizontalScrollBar()->sizeHint().height()/2 : 0)
			+ (horizontalScrollBar()->isVisible() ? 0 : 
				d->internal_bottomMargin
//	horizontalScrollBar()->sizeHint().height()/2
		)
//- /*d->bottomMargin */ horizontalScrollBar()->sizeHint().height()*3/2
			+ contentsY()
//			- (verticalScrollBar()->isVisible() ? horizontalScrollBar()->sizeHint().height()/2 : 0)
			)
		) );
//	reg = reg.subtract( QRect( QPoint( 0, 0 ), ts ) );

	// And draw the rectangles (transformed inc contents coordinates as needed)
	QMemArray<QRect> r = reg.rects();
	for ( int i = 0; i < (int)r.count(); i++ ) {
		QRect rect( viewportToContents2(r[i].topLeft()), r[i].size() );
/*		kdDebug(44021) << QString("- pEA: p->fillRect(x:%1 y:%2 w:%3 h:%4)")
			.arg(rect.x()).arg(rect.y())
			.arg(rect.width()).arg(rect.height()) << endl;*/
//		p->fillRect( QRect(viewportToContents2(r[i].topLeft()),r[i].size()), d->emptyAreaColor );
		p->fillRect( rect, d->appearance.emptyAreaColor );
//		p->fillRect( QRect(viewportToContents2(r[i].topLeft()),r[i].size()), viewport()->backgroundBrush() );
	}
}

void KexiTableView::contentsMouseDoubleClickEvent(QMouseEvent *e)
{
//	kdDebug(44021) << "KexiTableView::contentsMouseDoubleClickEvent()" << endl;
	m_contentsMousePressEvent_dblClick = true;
	contentsMousePressEvent(e);
	m_contentsMousePressEvent_dblClick = false;

	if(m_currentItem)
	{
		if(d->editOnDoubleClick && columnEditable(m_curCol) 
			&& columnType(m_curCol) != KexiDB::Field::Boolean)
		{
			startEditCurrentCell();
//			createEditor(m_curRow, m_curCol, QString::null);
		}

		emit itemDblClicked(m_currentItem, m_curRow, m_curCol);
	}
}

void KexiTableView::contentsMousePressEvent( QMouseEvent* e )
{
//	kdDebug(44021) << "KexiTableView::contentsMousePressEvent() ??" << endl;
	setFocus();
	if(m_data->count()==0 && !isInsertingEnabled()) {
		QScrollView::contentsMousePressEvent( e );
		return;
	}

	if (columnAt(e->pos().x())==-1) { //outside a colums
		QScrollView::contentsMousePressEvent( e );
		return;
	}
//	d->contentsMousePressEvent_ev = *e;
//	d->contentsMousePressEvent_enabled = true;
//	QTimer::singleShot(2000, this, SLOT( contentsMousePressEvent_Internal() ));
//	d->contentsMousePressEvent_timer.start(100,true);
	
//	if (!d->contentsMousePressEvent_enabled)
//		return;
//	d->contentsMousePressEvent_enabled=false;

	if (!d->moveCursorOnMouseRelease) {
		if (!handleContentsMousePressOrRelease(e, false))
			return;
	}

//	kdDebug(44021)<<"void KexiTableView::contentsMousePressEvent( QMouseEvent* e ) by now the current items should be set, if not -> error + crash"<<endl;
	if(e->button() == RightButton)
	{
		showContextMenu(e->globalPos());
	}
	else if(e->button() == LeftButton)
	{
		if(columnType(m_curCol) == KexiDB::Field::Boolean && columnEditable(m_curCol))
		{
			//only accept clicking on the [x] rect (copied from KexiBoolTableEdit::setupContents())
			int s = QMAX(d->rowHeight - 5, 12);
			s = QMIN( d->rowHeight-3, s );
			s = QMIN( columnWidth(m_curCol)-3, s ); //avoid too large box
			const QRect r( columnPos(m_curCol) + QMAX( columnWidth(m_curCol)/2 - s/2, 0 ), rowPos(m_curRow) +d->rowHeight/2 - s/2 /*- 1*/, s, s);
			kdDebug() << r << endl;
			if (r.contains(e->pos())) {
//				kdDebug() << "e->x:" << e->x() << " e->y:" << e->y() << " " << rowPos(m_curRow) << 
//					" " << columnPos(m_curCol) << endl;
				boolToggled();
			}
		}
#if 0 //js: TODO
		else if(columnType(m_curCol) == QVariant::StringList && columnEditable(m_curCol))
		{
			createEditor(m_curRow, m_curCol);
		}
#endif
	}
//ScrollView::contentsMousePressEvent( e );
}

void KexiTableView::contentsMouseReleaseEvent( QMouseEvent* e )
{
//	kdDebug(44021) << "KexiTableView::contentsMousePressEvent() ??" << endl;
	if(m_data->count()==0 && !isInsertingEnabled())
		return;

	if (d->moveCursorOnMouseRelease)
		handleContentsMousePressOrRelease(e, true);

	int col = columnAt(e->pos().x());
	int row = rowAt(e->pos().y());

	if (!m_currentItem || col==-1 || row==-1 || col!=m_curCol || row!=m_curRow)//outside a current cell
		return;

	QScrollView::contentsMouseReleaseEvent( e );

	emit itemMouseReleased(m_currentItem, m_curRow, m_curCol);
}

//! @internal called by contentsMouseOrEvent() contentsMouseReleaseEvent() to move cursor
bool KexiTableView::handleContentsMousePressOrRelease(QMouseEvent* e, bool release)
{
	// remember old focus cell
	int oldRow = m_curRow;
	int oldCol = m_curCol;
	kdDebug(44021) << "oldRow=" << oldRow <<" oldCol=" << oldCol <<endl;
	bool onInsertItem = false;

	int newrow, newcol;
	//compute clicked row nr
	if (isInsertingEnabled()) {
		if (rowAt(e->pos().y())==-1) {
			newrow = rowAt(e->pos().y() - d->rowHeight);
			if (newrow==-1 && m_data->count()>0) {
				if (release)
					QScrollView::contentsMouseReleaseEvent( e );
				else
					QScrollView::contentsMousePressEvent( e );
				return false;
			}
			newrow++;
			kdDebug(44021) << "Clicked just on 'insert' row." << endl;
			onInsertItem=true;
		}
		else {
			// get new focus cell
			newrow = rowAt(e->pos().y());
		}
	}
	else {
		if (rowAt(e->pos().y())==-1 || columnAt(e->pos().x())==-1) {
			if (release)
				QScrollView::contentsMouseReleaseEvent( e );
			else
				QScrollView::contentsMousePressEvent( e );
			return false; //clicked outside a grid
		}
		// get new focus cell
		newrow = rowAt(e->pos().y());
	}
	newcol = columnAt(e->pos().x());

	if(e->button() != NoButton) {
		setCursorPosition(newrow,newcol);
	}
	return true;
}

void KexiTableView::showContextMenu(const QPoint& _pos)
{
	if (!d->contextMenuEnabled || m_popup->count()<1)
		return;
	QPoint pos(_pos);
	if (pos==QPoint(-1,-1)) {
		pos = viewport()->mapToGlobal( QPoint( columnPos(m_curCol), rowPos(m_curRow) + d->rowHeight ) );
	}
	//show own context menu if configured
//	if (updateContextMenu()) {
		selectRow(m_curRow);
		m_popup->exec(pos);
/*	}
	else {
		//request other context menu
		emit contextMenuRequested(m_currentItem, m_curCol, pos);
	}*/
}

void KexiTableView::contentsMouseMoveEvent( QMouseEvent *e )
{
	if (d->appearance.rowMouseOverHighlightingEnabled /*rowHighlightingEnabled*/) {
		int row;
		if (columnAt(e->x())<0) {
			row = -1;
		} else {
			row = rowAt( e->y(), true /*ignoreEnd*/ );
			if (row > (rows() - 1 + (isInsertingEnabled()?1:0)))
				row = -1; //no row to paint
		}

//	const col = columnAt(e->x());
//	columnPos(col) + columnWidth(col)
//	columnPos(d->numCols - 1) + columnWidth(d->numCols - 1)));

		if (row != d->highlightedRow) {
			const int oldRow = d->highlightedRow;
			d->highlightedRow = row;
			updateRow(oldRow);
			updateRow(d->highlightedRow);
//			if (m_curRow>=0 && oldRow!=m_curRow && d->highlightedRow!=m_curRow && !d->appearance.persistentSelections) {
				//currently selected (not necessary highlighted) row needs to be repainted
				updateRow(m_curRow);
//			}
		}
	}

#if 0//(js) doesn't work!

	// do the same as in mouse press
	int x,y;
	contentsToViewport(e->x(), e->y(), x, y);

	if(y > visibleHeight())
	{
		d->needAutoScroll = true;
		d->scrollTimer->start(70, false);
		d->scrollDirection = ScrollDown;
	}
	else if(y < 0)
	{
		d->needAutoScroll = true;
		d->scrollTimer->start(70, false);
		d->scrollDirection = ScrollUp;
	}
	else if(x > visibleWidth())
	{
		d->needAutoScroll = true;
		d->scrollTimer->start(70, false);
		d->scrollDirection = ScrollRight;
	}
	else if(x < 0)
	{
		d->needAutoScroll = true;
		d->scrollTimer->start(70, false);
		d->scrollDirection = ScrollLeft;
	}
	else
	{
		d->needAutoScroll = false;
		d->scrollTimer->stop();
		contentsMousePressEvent(e);
	}
#endif
	QScrollView::contentsMouseMoveEvent(e);
}

#if 0//(js) doesn't work!
void KexiTableView::contentsMouseReleaseEvent(QMouseEvent *)
{
	if(d->needAutoScroll)
	{
		d->scrollTimer->stop();
	}
}
#endif

static bool overrideEditorShortcutNeeded(QKeyEvent *e)
{
	//perhaps more to come...
	return e->key() == Qt::Key_Delete && e->state()==Qt::ControlButton;
}

bool KexiTableView::shortCutPressed( QKeyEvent *e, const QCString &action_name )
{
	const int k = e->key();
	KAction *action = m_sharedActions[action_name];
	if (action) {
		if (!action->isEnabled())//this action is disabled - don't process it!
			return false; 
		if (action->shortcut() == KShortcut( KKey(e) )) {
			//special cases when we need to override editor's shortcut
			if (overrideEditorShortcutNeeded(e)) {
				return true;
			}
			return false;//this shortcut is owned by shared action - don't process it!
		}
	}

	//check default shortcut (when user app has no action shortcuts defined
	// but we want these shortcuts to still work)
	if (action_name=="data_save_row")
		return (k == Key_Return || k == Key_Enter) && e->state()==ShiftButton;
	if (action_name=="edit_delete_row")
		return k == Key_Delete && e->state()==ControlButton;
	if (action_name=="edit_delete")
		return k == Key_Delete && e->state()==NoButton;
	if (action_name=="edit_edititem")
		return k == Key_F2 && e->state()==NoButton;
	if (action_name=="edit_insert_empty_row")
		return k == Key_Insert && e->state()==(ShiftButton | ControlButton);

	return false;
}

void KexiTableView::keyPressEvent(QKeyEvent* e)
{
	if (!hasData())
		return;
//	kdDebug() << "KexiTableView::keyPressEvent: key=" <<e->key() << " txt=" <<e->text()<<endl;

	const int k = e->key();
	const bool ro = isReadOnly();
	QWidget *w = focusWidget();
//	if (!w || w!=viewport() && w!=this && (!m_editor || w!=m_editor->view() && w!=m_editor)) {
//	if (!w || w!=viewport() && w!=this && (!m_editor || w!=m_editor->view())) {
	if (!w || w!=viewport() && w!=this && (!m_editor || !KexiUtils::hasParent(dynamic_cast<QObject*>(m_editor), w))) {
		//don't process stranger's events
		e->ignore();
		return;
	}
	if (d->skipKeyPress) {
		d->skipKeyPress=false;
		e->ignore();
		return;
	}
	
	if(m_currentItem == 0 && (m_data->count() > 0 || isInsertingEnabled()))
	{
		setCursorPosition(0,0);
	}
	else if(m_data->count() == 0 && !isInsertingEnabled())
	{
		e->accept();
		return;
	}

	if(m_editor) {// if a cell is edited, do some special stuff
		if (k == Key_Escape) {
			cancelEditor();
			e->accept();
			return;
		} else if (k == Key_Return || k == Key_Enter) {
			if (columnType(m_curCol) == KexiDB::Field::Boolean) {
				boolToggled();
			}
			else {
				acceptEditor();
			}
			e->accept();
			return;
		}
	}
	else if (m_rowEditing) {// if a row is in edit mode, do some special stuff
		if (shortCutPressed( e, "data_save_row")) {
			kdDebug() << "shortCutPressed!!!" <<endl;
			acceptRowEdit();
			return;
		}
	}

	if(k == Key_Return || k == Key_Enter)
	{
		emit itemReturnPressed(m_currentItem, m_curRow, m_curCol);
	}

	int curRow = m_curRow;
	int curCol = m_curCol;

	const bool nobtn = e->state()==NoButton;
	bool printable = false;

	//check shared shortcuts
	if (!ro) {
		if (shortCutPressed(e, "edit_delete_row")) {
			deleteCurrentRow();
			e->accept();
			return;
		} else if (shortCutPressed(e, "edit_delete")) {
			deleteAndStartEditCurrentCell();
			e->accept();
			return;
		}
		else if (shortCutPressed(e, "edit_insert_empty_row")) {
			insertEmptyRow();
			e->accept();
			return;
		}
	}

/*	case Key_Delete:
		if (e->state()==Qt::ControlButton) {//remove current row
			deleteCurrentRow();
		}
		else if (nobtn) {//remove contents of the current cell
			deleteAndStartEditCurrentCell();
		}
		break;*/

	if (k == Key_Shift || k == Key_Alt || k == Key_Control || k == Key_Meta) {
		e->ignore();
	}
	else if (k == Key_Up && nobtn) {
		selectPrevRow();
		e->accept();
		return;
	}
	else if (k == Key_Down && nobtn) {
//			curRow = QMIN(rows() - 1 + (isInsertingEnabled()?1:0), curRow + 1);
		selectNextRow();
		e->accept();
		return;
	}
	else if (k == Key_PageUp && nobtn) {
//			curRow -= visibleHeight() / d->rowHeight;
//			curRow = QMAX(0, curRow);
		selectPrevPage();
		e->accept();
		return;
	}
	else if (k == Key_PageDown && nobtn) {
//			curRow += visibleHeight() / d->rowHeight;
//			curRow = QMIN(rows() - 1 + (isInsertingEnabled()?1:0), curRow);
		selectNextPage();
		e->accept();
		return;
	}
	else if (k == Key_Home) {
		if (d->appearance.fullRowSelection) {
			//we're in row-selection mode: home key always moves to 1st row
			curRow = 0;//to 1st row
		}
		else {//cell selection mode: different actions depending on ctrl and shift keys state
			if (nobtn) {
				curCol = 0;//to 1st col
			}
			else if (e->state()==ControlButton) {
				curRow = 0;//to 1st row
			}
			else if (e->state()==(ControlButton|ShiftButton)) {
				curRow = 0;//to 1st row and col
				curCol = 0;
			}
		}
	}
	else if (k == Key_End) {
		if (d->appearance.fullRowSelection) {
			//we're in row-selection mode: home key always moves to last row
			curRow = m_data->count()-1+(isInsertingEnabled()?1:0);//to last row
		}
		else {//cell selection mode: different actions depending on ctrl and shift keys state
			if (nobtn) {
				curCol = columns()-1;//to last col
			}
			else if (e->state()==ControlButton) {
				curRow = m_data->count()-1+(isInsertingEnabled()?1:0);//to last row
			}
			else if (e->state()==(ControlButton|ShiftButton)) {
				curRow = m_data->count()-1+(isInsertingEnabled()?1:0);//to last row and col
				curCol = columns()-1;//to last col
			}
		}
	}
	else if (k == Key_Backspace && nobtn) {
		if (!ro && columnType(curCol) != KexiDB::Field::Boolean && columnEditable(curCol))
			createEditor(curRow, curCol, QString::null, true);
	}
	else if (k == Key_Space) {
		if (nobtn && !ro && columnEditable(curCol)) {
			if (columnType(curCol) == KexiDB::Field::Boolean) {
				boolToggled();
			}
			else
				printable = true; //just space key
		}
	}
	else if (k == Key_Escape) {
		if (nobtn && m_rowEditing) {
			cancelRowEdit();
			return;
		}
	}
	else {
		//others:
		if (nobtn && (k==Key_Tab || k==Key_Right)) {
//! \todo add option for stopping at 1st column for Key_left
			//tab
			if (acceptEditor()) {
				if (curCol == (columns() - 1)) {
					if (curRow < (rows()-1+(isInsertingEnabled()?1:0))) {//skip to next row
						curRow++;
						curCol = 0;
					}
				}
				else
					curCol++;
			}
		}
		else if ((e->state()==ShiftButton && k==Key_Tab)
		 || (nobtn && k==Key_Backtab)
		 || (e->state()==ShiftButton && k==Key_Backtab)
		 || (nobtn && k==Key_Left)
			) {
//! \todo add option for stopping at last column
			//backward tab
			if (acceptEditor()) {
				if (curCol == 0) {
					if (curRow>0) {//skip to previous row
						curRow--;
						curCol = columns() - 1;
					}
				}
				else
					curCol--;
			}
		}
		else if ( nobtn && (k==Key_Enter || k==Key_Return || shortCutPressed(e, "edit_edititem")) ) {
			startEditOrToggleValue();
		}
		else if (nobtn && k==d->contextMenuKey) { //Key_Menu:
			showContextMenu();
		}
		else {
			KexiTableEdit *edit = dynamic_cast<KexiTableEdit*>( editor( m_curCol ) );
			if (edit && edit->handleKeyPress(e, m_editor==edit)) {
				//try to handle the event @ editor's level
				e->accept();
				return;
			}

			kdDebug() << "KexiTableView::KeyPressEvent(): default" << endl;
			if (e->text().isEmpty() || !e->text().isEmpty() && !e->text()[0].isPrint() ) {
				kdDebug(44021) << "NOT PRINTABLE: 0x0" << QString("%1").arg(k,0,16) <<endl;
//				e->ignore();
				QScrollView::keyPressEvent(e);
				return;
			}

			printable = true;
		}
	}
	//finally: we've printable char:
	if (printable && !ro) {
		KexiTableViewColumn *colinfo = m_data->column(curCol);
		if (colinfo->acceptsFirstChar(e->text()[0])) {
			kdDebug(44021) << "KexiTableView::KeyPressEvent(): ev pressed: acceptsFirstChar()==true" << endl;
	//			if (e->text()[0].isPrint())
			createEditor(curRow, curCol, e->text(), true);
		}
		else {
//TODO show message "key not allowed eg. on a statusbar"
			kdDebug(44021) << "KexiTableView::KeyPressEvent(): ev pressed: acceptsFirstChar()==false" << endl;
		}
	}

	d->vScrollBarValueChanged_enabled=false;

	// if focus cell changes, repaint
	setCursorPosition(curRow, curCol);

	d->vScrollBarValueChanged_enabled=true;

	e->accept();
}

void KexiTableView::emitSelected()
{
	if(m_currentItem)
		emit itemSelected(m_currentItem);
}

int KexiTableView::rowsPerPage() const
{
	return visibleHeight() / d->rowHeight;
}

KexiDataItemInterface *KexiTableView::editor( int col, bool ignoreMissingEditor )
{
	if (!m_data || col<0 || col>=columns())
		return 0;
	KexiTableViewColumn *tvcol = m_data->column(col);
//	int t = tvcol->field->type();

	//find the editor for this column
	KexiTableEdit *editor = d->editors[ tvcol ];
	if (editor)
		return editor;

	//not found: create
//	editor = KexiCellEditorFactory::createEditor(*m_data->column(col)->field, this);
	editor = KexiCellEditorFactory::createEditor(*m_data->column(col), this);
	if (!editor) {//create error!
		if (!ignoreMissingEditor) {
			//js TODO: show error???
			cancelRowEdit();
		}
		return 0;
	}
	editor->hide();
	connect(editor,SIGNAL(editRequested()),this,SLOT(slotEditRequested()));
	connect(editor,SIGNAL(cancelRequested()),this,SLOT(cancelEditor()));
	connect(editor,SIGNAL(acceptRequested()),this,SLOT(acceptEditor()));

	editor->resize(columnWidth(col)-1, rowHeight()-1);
	editor->installEventFilter(this);
	if (editor->widget())
		editor->widget()->installEventFilter(this);
	//store
	d->editors.insert( tvcol, editor );
	return editor;
}

void KexiTableView::editorShowFocus( int /*row*/, int col )
{
	KexiDataItemInterface *edit = editor( col );
	/*nt p = rowPos(row);
	 (!edit || (p < contentsY()) || (p > (contentsY()+clipper()->height()))) {
		kdDebug()<< "KexiTableView::editorShowFocus() : OUT" << endl;
		return;
	}*/
	if (edit) {
		kdDebug()<< "KexiTableView::editorShowFocus() : IN" << endl;
		QRect rect = cellGeometry( m_curRow, m_curCol );
//		rect.moveBy( -contentsX(), -contentsY() );
		edit->showFocus( rect, isReadOnly() || m_data->column(col)->isReadOnly() );
	}
}

void KexiTableView::slotEditRequested()
{
//	KexiTableEdit *edit = editor( m_curCol );
//	if (edit) {

	createEditor(m_curRow, m_curCol);
}

void KexiTableView::createEditor(int row, int col, const QString& addText, bool removeOld)
{
	kdDebug(44021) << "KexiTableView::createEditor('"<<addText<<"',"<<removeOld<<")"<<endl;
	if (isReadOnly()) {
		kdDebug(44021) << "KexiTableView::createEditor(): DATA IS READ ONLY!"<<endl;
		return;
	}

	if (m_data->column(col)->isReadOnly()) {//d->pColumnModes.at(d->numCols-1) & ColumnReadOnly)
		kdDebug(44021) << "KexiTableView::createEditor(): COL IS READ ONLY!"<<endl;
		return;
	}

	const bool startRowEdit = !m_rowEditing; //remember if we're starting row edit

	if (!m_rowEditing) {
		//we're starting row editing session
		m_data->clearRowEditBuffer();
		
		m_rowEditing = true;
		//indicate on the vheader that we are editing:
		m_verticalHeader->setEditRow(m_curRow);
		if (isInsertingEnabled() && m_currentItem==m_insertItem) {
			//we should know that we are in state "new row editing"
			m_newRowEditing = true;
			//'insert' row editing: show another row after that:
			m_data->append( m_insertItem );
			//new empty 'inserting' item
			m_insertItem = m_data->createItem(); //new KexiTableItem(dataColumns());
//			updateContents();
			m_verticalHeader->addLabel();
			m_verticalHeaderAlreadyAdded = true;
			updateWidgetContentsSize();
			//refr. current and next row
			updateContents(columnPos(0), rowPos(row), viewport()->width(), d->rowHeight*2);
//			updateContents(columnPos(0), rowPos(row+1), viewport()->width(), d->rowHeight);
//js: warning this breaks behaviour (cursor is skipping, etc.): qApp->processEvents(500);
			ensureVisible(columnPos(m_curCol), rowPos(row+1)+d->rowHeight-1, columnWidth(m_curCol), d->rowHeight);

			m_verticalHeader->setOffset(contentsY());
		}
	}	

	m_editor = editor( col );
	QWidget *m_editorWidget = dynamic_cast<QWidget*>(m_editor);
	if (!m_editorWidget)
		return;

	m_editor->setValue(*bufferedValueAt(col), addText, removeOld);
	if (m_editor->hasFocusableWidget()) {
		moveChild(m_editorWidget, columnPos(m_curCol), rowPos(m_curRow));

		m_editorWidget->resize(columnWidth(m_curCol)-1, rowHeight()-1);
		m_editorWidget->show();

		m_editor->setFocus();
	}

	if (startRowEdit) {
		m_navPanel->showEditingIndicator(true); //this will allow to enable 'next' btn
//		m_navPanel->updateButtons(rows()); //refresh 'next' btn
		emit rowEditStarted(m_curRow);
	}
}

void KexiTableView::focusInEvent(QFocusEvent*)
{
	updateCell(m_curRow, m_curCol);
}


void KexiTableView::focusOutEvent(QFocusEvent*)
{
	d->scrollBarTipTimer.stop();
	d->scrollBarTip->hide();
	
	updateCell(m_curRow, m_curCol);
}

bool KexiTableView::focusNextPrevChild(bool /*next*/)
{
	return false; //special Tab/BackTab meaning
/*	if (m_editor)
		return true;
	return QScrollView::focusNextPrevChild(next);*/
}

void KexiTableView::resizeEvent(QResizeEvent *e)
{
	QScrollView::resizeEvent(e);
	//updateGeometries();
	
	if (m_navPanel)
		m_navPanel->updateGeometry(leftMargin());
//	updateNavPanelGeometry();

	if ((contentsHeight() - e->size().height()) <= d->rowHeight) {
		slotUpdate();
		triggerUpdate();
	}
//	d->pTopHeader->repaint();


/*		m_navPanel->setGeometry(
			frameWidth(),
			viewport()->height() +d->pTopHeader->height() 
			-(horizontalScrollBar()->isVisible() ? 0 : horizontalScrollBar()->sizeHint().height())
			+frameWidth(),
			m_navPanel->sizeHint().width(), // - verticalScrollBar()->sizeHint().width() - horizontalScrollBar()->sizeHint().width(),
			horizontalScrollBar()->sizeHint().height()
		);*/
//		updateContents();
//		m_navPanel->setGeometry(1,horizontalScrollBar()->pos().y(),
	//		m_navPanel->width(), horizontalScrollBar()->height());
//	updateContents(0,0,2000,2000);//js
//	erase(); repaint();
}

void KexiTableView::viewportResizeEvent( QResizeEvent *e )
{
	QScrollView::viewportResizeEvent( e );
	updateGeometries();
//	erase(); repaint();
}

void KexiTableView::showEvent(QShowEvent *e)
{
	QScrollView::showEvent(e);
	if (!d->maximizeColumnsWidthOnShow.isEmpty()) {
		maximizeColumnsWidth(d->maximizeColumnsWidthOnShow);
		d->maximizeColumnsWidthOnShow.clear();
	}

	if (m_initDataContentsOnShow) {
		//full init
		m_initDataContentsOnShow = false;
		initDataContents();
	}
	else {
		//just update size
		QSize s(tableSize());
//	QRect r(cellGeometry(rows() - 1 + (isInsertingEnabled()?1:0), columns() - 1 ));
//	resizeContents(r.right() + 1, r.bottom() + 1);
		resizeContents(s.width(),s.height());
	}
	updateGeometries();

	//now we can ensure cell's visibility ( if there was such a call before show() )
	if (d->ensureCellVisibleOnShow!=QPoint(-1,-1)) {
		ensureCellVisible( d->ensureCellVisibleOnShow.x(), d->ensureCellVisibleOnShow.y() );
		d->ensureCellVisibleOnShow = QPoint(-1,-1); //reset the flag
	}
	if (m_navPanel)
		m_navPanel->updateGeometry(leftMargin());
//	updateNavPanelGeometry();
}

void KexiTableView::contentsDragMoveEvent(QDragMoveEvent *e)
{
	if (!hasData())
		return;
	if (m_dropsAtRowEnabled) {
		QPoint p = e->pos();
		int row = rowAt(p.y());
		KexiTableItem *item = 0;
//		if (row==(rows()-1) && (p.y() % d->rowHeight) > (d->rowHeight*2/3) ) {
		if ((p.y() % d->rowHeight) > (d->rowHeight*2/3) ) {
			row++;
		}
		item = m_data->at(row);
		emit dragOverRow(item, row, e);
		if (e->isAccepted()) {
			if (m_dragIndicatorLine>=0 && m_dragIndicatorLine != row) {
				//erase old indicator
				updateRow(m_dragIndicatorLine);
			}
			if (m_dragIndicatorLine != row) {
				m_dragIndicatorLine = row;
				updateRow(m_dragIndicatorLine);
			}
		}
		else {
			if (m_dragIndicatorLine>=0) {
				//erase old indicator
				updateRow(m_dragIndicatorLine);
			}
			m_dragIndicatorLine = -1;
		}
	}
	else
		e->acceptAction(false);
/*	for(QStringList::Iterator it = d->dropFilters.begin(); it != d->dropFilters.end(); it++)
	{
		if(e->provides((*it).latin1()))
		{
			e->acceptAction(true);
			return;
		}
	}*/
//	e->acceptAction(false);
}

void KexiTableView::contentsDropEvent(QDropEvent *ev)
{
	if (!hasData())
		return;
	if (m_dropsAtRowEnabled) {
		//we're no longer dragging over the table
		if (m_dragIndicatorLine>=0) {
			int row2update = m_dragIndicatorLine;
			m_dragIndicatorLine = -1;
			updateRow(row2update);
		}
		QPoint p = ev->pos();
		int row = rowAt(p.y());
		if ((p.y() % d->rowHeight) > (d->rowHeight*2/3) ) {
			row++;
		}
		KexiTableItem *item = m_data->at(row);
		KexiTableItem *newItem = 0;
		emit droppedAtRow(item, row, ev, newItem);
		if (newItem) {
			const int realRow = (row==m_curRow ? -1 : row);
			insertItem(newItem, realRow);
			setCursorPosition(row, 0);
//			m_currentItem = newItem;
		}
	}
}

void KexiTableView::viewportDragLeaveEvent( QDragLeaveEvent * )
{
	if (!hasData())
		return;
	if (m_dropsAtRowEnabled) {
		//we're no longer dragging over the table
		if (m_dragIndicatorLine>=0) {
			int row2update = m_dragIndicatorLine;
			m_dragIndicatorLine = -1;
			updateRow(row2update);
		}
	}
}

void KexiTableView::updateCell(int row, int col)
{
//	kdDebug(44021) << "updateCell("<<row<<", "<<col<<")"<<endl;
	updateContents(cellGeometry(row, col));
/*	QRect r = cellGeometry(row, col);
	r.setHeight(r.height()+6);
	r.setTop(r.top()-3);
	updateContents();*/
}

void KexiTableView::updateRow(int row)
{
//	kdDebug(44021) << "updateRow("<<row<<")"<<endl;
	if (row < 0 || row >= (rows() + 2/* sometimes we want to refresh the row after last*/ ))
		return;
	//int leftcol = d->pTopHeader->sectionAt( d->pTopHeader->offset() );

	//kdDebug() << contentsX() << " " << contentsY() << endl;
	//kdDebug() << QRect( columnPos( leftcol ), rowPos(row), clipper()->width(), rowHeight() ) << endl;
	//	updateContents( QRect( columnPos( leftcol ), rowPos(row), clipper()->width(), rowHeight() ) ); //columnPos(rightcol)+columnWidth(rightcol), rowHeight() ) );
	updateContents( QRect( contentsX(), rowPos(row), clipper()->width(), rowHeight() ) ); //columnPos(rightcol)+columnWidth(rightcol), rowHeight() ) );
}

void KexiTableView::slotColumnWidthChanged( int, int, int )
{
	QSize s(tableSize());
	int w = contentsWidth();
	viewport()->setUpdatesEnabled(false);
	resizeContents( s.width(), s.height() );
	viewport()->setUpdatesEnabled(true);
	if (contentsWidth() < w) {
		updateContents(contentsX(), 0, viewport()->width(), contentsHeight());
//		repaintContents( s.width(), 0, w - s.width() + 1, contentsHeight(), TRUE );
	} 
	else {
	//	updateContents( columnPos(col), 0, contentsWidth(), contentsHeight() );
		updateContents(contentsX(), 0, viewport()->width(), contentsHeight());
	//	viewport()->repaint();
	}

//	updateContents(0, 0, d->pBufferPm->width(), d->pBufferPm->height());
	QWidget *m_editorWidget = dynamic_cast<QWidget*>(m_editor);
	if (m_editorWidget)
	{
		m_editorWidget->resize(columnWidth(m_curCol)-1, rowHeight()-1);
		moveChild(m_editorWidget, columnPos(m_curCol), rowPos(m_curRow));
	}
	updateGeometries();
	updateScrollBars();
	if (m_navPanel)
		m_navPanel->updateGeometry(leftMargin());
//	updateNavPanelGeometry();
}

void KexiTableView::slotSectionHandleDoubleClicked( int section )
{
	adjustColumnWidthToContents(section);
	slotColumnWidthChanged(0,0,0); //to update contents and redraw
}


void KexiTableView::updateGeometries()
{
	QSize ts = tableSize();
	if (d->pTopHeader->offset() && ts.width() < (d->pTopHeader->offset() + d->pTopHeader->width()))
		horizontalScrollBar()->setValue(ts.width() - d->pTopHeader->width());

//	m_verticalHeader->setGeometry(1, topMargin() + 1, leftMargin(), visibleHeight());
	d->pTopHeader->setGeometry(leftMargin() + 1, 1, visibleWidth(), topMargin());
	m_verticalHeader->setGeometry(1, topMargin() + 1, leftMargin(), visibleHeight());
}

int KexiTableView::columnWidth(int col) const
{
	if (!hasData())
		return 0;
	int vcID = m_data->visibleColumnID( col );
	return vcID==-1 ? 0 : d->pTopHeader->sectionSize( vcID );
}

int KexiTableView::rowHeight() const
{
	return d->rowHeight;
}

int KexiTableView::columnPos(int col) const
{
	if (!hasData())
		return 0;
	//if this column is hidden, find first column before that is visible
	int c = QMIN(col, (int)m_data->columnsCount()-1), vcID = 0;
	while (c>=0 && (vcID=m_data->visibleColumnID( c ))==-1)
		c--;
	if (c<0)
		return 0;
	if (c==col)
		return d->pTopHeader->sectionPos(vcID);
	return d->pTopHeader->sectionPos(vcID)+d->pTopHeader->sectionSize(vcID);
}

int KexiTableView::rowPos(int row) const
{
	return d->rowHeight*row;
}

int KexiTableView::columnAt(int pos) const
{
	if (!hasData())
		return -1;
	int r = d->pTopHeader->sectionAt(pos);
	if (r<0)
		return r;
	return m_data->globalColumnID( r );

//	if (r==-1)
//		kdDebug() << "columnAt("<<pos<<")==-1 !!!" << endl;
//	return r;
}

int KexiTableView::rowAt(int pos, bool ignoreEnd) const
{
	if (!hasData())
		return -1;
	pos /=d->rowHeight;
	if (pos < 0)
		return 0;
	if ((pos >= (int)m_data->count()) && !ignoreEnd)
		return -1;
	return pos;
}

QRect KexiTableView::cellGeometry(int row, int col) const
{
	return QRect(columnPos(col), rowPos(row),
		columnWidth(col), rowHeight());
}

QSize KexiTableView::tableSize() const
{
	if ((rows()+ (isInsertingEnabled()?1:0) ) > 0 && columns() > 0) {
/*		kdDebug() << "tableSize()= " << columnPos( columns() - 1 ) + columnWidth( columns() - 1 ) 
			<< ", " << rowPos( rows()-1+(isInsertingEnabled()?1:0)) + d->rowHeight
//			+ QMAX(m_navPanel ? m_navPanel->height() : 0, horizontalScrollBar()->sizeHint().height())
			+ (m_navPanel->isVisible() ? QMAX( m_navPanel->height(), horizontalScrollBar()->sizeHint().height() ) :0 )
			+ margin() << endl;
*/
//		kdDebug()<< m_navPanel->isVisible() <<" "<<m_navPanel->height()<<" "
//		<<horizontalScrollBar()->sizeHint().height()<<" "<<rowPos( rows()-1+(isInsertingEnabled()?1:0))<<endl;

		//int xx = horizontalScrollBar()->sizeHint().height()/2;

		QSize s( 
			columnPos( columns() - 1 ) + columnWidth( columns() - 1 ),
//			+ verticalScrollBar()->sizeHint().width(),
			rowPos( rows()-1+(isInsertingEnabled()?1:0) ) + d->rowHeight
			+ (horizontalScrollBar()->isVisible() ? 0 : horizontalScrollBar()->sizeHint().height())
			+ d->internal_bottomMargin
//				horizontalScrollBar()->sizeHint().height()/2
//			- /*d->bottomMargin */ horizontalScrollBar()->sizeHint().height()*3/2

//			+ ( (m_navPanel && m_navPanel->isVisible() && verticalScrollBar()->isVisible()
	//			&& !horizontalScrollBar()->isVisible()) 
		//		? horizontalScrollBar()->sizeHint().height() : 0)

//			+ QMAX( (m_navPanel && m_navPanel->isVisible()) ? m_navPanel->height() : 0, 
//				horizontalScrollBar()->isVisible() ? horizontalScrollBar()->sizeHint().height() : 0)

//			+ (m_navPanel->isVisible() 
//				? QMAX( m_navPanel->height(), horizontalScrollBar()->sizeHint().height() ) :0 )

//			- (horizontalScrollBar()->isVisible() ? horizontalScrollBar()->sizeHint().height() :0 )
			+ margin() 
//-2*d->rowHeight
		);

//		kdDebug() << rows()-1 <<" "<< (isInsertingEnabled()?1:0) <<" "<< (m_rowEditing?1:0) << " " <<  s << endl;
		return s;
//			+horizontalScrollBar()->sizeHint().height() + margin() );
	}
	return QSize(0,0);
}

void KexiTableView::ensureCellVisible(int row, int col/*=-1*/)
{
	if (!isVisible()) {
		//the table is invisible: we can't ensure visibility now
		d->ensureCellVisibleOnShow = QPoint(row,col);
		return;
	}

	//quite clever: ensure the cell is visible:
	QRect r( columnPos(col==-1 ? m_curCol : col), rowPos(row) +(d->appearance.fullRowSelection?1:0), 
		columnWidth(col==-1 ? m_curCol : col), rowHeight());

/*	if (m_navPanel && horizontalScrollBar()->isHidden() && row == rows()-1) {
		//when cursor is moved down and navigator covers the cursor's area,
		//area is scrolled up
		if ((viewport()->height() - m_navPanel->height()) < r.bottom()) {
			scrollBy(0,r.bottom() - (viewport()->height() - m_navPanel->height()));
		}
	}*/

	if (m_navPanel && m_navPanel->isVisible() && horizontalScrollBar()->isHidden()) {
		//a hack: for visible navigator: increase height of the visible rect 'r'
		r.setBottom(r.bottom()+m_navPanel->height());
	}

	QPoint pcenter = r.center();
	ensureVisible(pcenter.x(), pcenter.y(), r.width()/2, r.height()/2);
//	updateContents();
//	updateNavPanelGeometry();
//	slotUpdate();
}

void KexiTableView::updateAfterCancelRowEdit()
{
	KexiDataAwareObjectInterface::updateAfterCancelRowEdit();
	m_navPanel->showEditingIndicator(false);
}

void KexiTableView::updateAfterAcceptRowEdit()
{
	KexiDataAwareObjectInterface::updateAfterAcceptRowEdit();
	m_navPanel->showEditingIndicator(false);
}

//reimpl.
void KexiTableView::removeEditor()
{
	if (!m_editor)
		return;
	KexiDataAwareObjectInterface::removeEditor();
	viewport()->setFocus();
}

void KexiTableView::slotRowRepaintRequested(KexiTableItem& item)
{
	updateRow( m_data->findRef(&item) );
}

//(js) unused
void KexiTableView::slotAutoScroll()
{
	kdDebug(44021) << "KexiTableView::slotAutoScroll()" <<endl;
	if (!d->needAutoScroll)
		return;

	switch(d->scrollDirection)
	{
		case ScrollDown:
			setCursorPosition(m_curRow + 1, m_curCol);
			break;

		case ScrollUp:
			setCursorPosition(m_curRow - 1, m_curCol);
			break;
		case ScrollLeft:
			setCursorPosition(m_curRow, m_curCol - 1);
			break;

		case ScrollRight:
			setCursorPosition(m_curRow, m_curCol + 1);
			break;
	}
}

#ifndef KEXI_NO_PRINT
void
KexiTableView::print(KPrinter &/*printer*/)
{
//	printer.setFullPage(true);
#if 0
	int leftMargin = printer.margins().width() + 2 + d->rowHeight;
	int topMargin = printer.margins().height() + 2;
//	int bottomMargin = topMargin + ( printer.realPageSize()->height() * printer.resolution() + 36 ) / 72;
	int bottomMargin = 0;
	kdDebug(44021) << "KexiTableView::print: bottom = " << bottomMargin << endl;

	QPainter p(&printer);

	KexiTableItem *i;
	int width = leftMargin;
	for(int col=0; col < columns(); col++)
	{
		p.fillRect(width, topMargin - d->rowHeight, columnWidth(col), d->rowHeight, QBrush(gray));
		p.drawRect(width, topMargin - d->rowHeight, columnWidth(col), d->rowHeight);
		p.drawText(width, topMargin - d->rowHeight, columnWidth(col), d->rowHeight, AlignLeft | AlignVCenter, d->pTopHeader->label(col));
		width = width + columnWidth(col);
	}

	int yOffset = topMargin;
	int row = 0;
	int right = 0;
	for(i = m_data->first(); i; i = m_data->next())
	{
		if(!i->isInsertItem())
		{	kdDebug(44021) << "KexiTableView::print: row = " << row << " y = " << yOffset << endl;
			int xOffset = leftMargin;
			for(int col=0; col < columns(); col++)
			{
				kdDebug(44021) << "KexiTableView::print: col = " << col << " x = " << xOffset << endl;
				p.saveWorldMatrix();
				p.translate(xOffset, yOffset);
				paintCell(&p, i, col, QRect(0, 0, columnWidth(col) + 1, d->rowHeight), true);
				p.restoreWorldMatrix();
//			p.drawRect(xOffset, yOffset, columnWidth(col), d->rowHeight);
				xOffset = xOffset + columnWidth(col);
				right = xOffset;
			}

			row++;
			yOffset = topMargin  + row * d->rowHeight;
		}

		if(yOffset > 900)
		{
			p.drawLine(leftMargin, topMargin, leftMargin, yOffset);
			p.drawLine(leftMargin, topMargin, right - 1, topMargin);
			printer.newPage();
			yOffset = topMargin;
			row = 0;
		}
	}
	p.drawLine(leftMargin, topMargin, leftMargin, yOffset);
	p.drawLine(leftMargin, topMargin, right - 1, topMargin);

//	p.drawLine(60,60,120,150);
	p.end();
#endif
}
#endif

QString KexiTableView::columnCaption(int colNum) const
{
	return d->pTopHeader->label(colNum);
}

KexiDB::Field* KexiTableView::field(int colNum) const
{
	if (!m_data || !m_data->column(colNum))
		return 0;
	return m_data->column(colNum)->field();
}

void KexiTableView::adjustColumnWidthToContents(int colNum)
{
	if (!hasData())
		return;
	if (columns()<=colNum || colNum < -1)
		return;

	if (colNum==-1) {
//		const int cols = columns();
		for (int i=0; i<columns(); i++)
			adjustColumnWidthToContents(i);
		return;
	}

	KexiCellEditorFactoryItem *item = KexiCellEditorFactory::item( columnType(colNum) );
	if (!item)
		return;
	QFontMetrics fm(font());
	int maxw = fm.width( d->pTopHeader->label( colNum ) );
//	int start = rowAt(contentsY());
//	int end = QMAX( start, rowAt( contentsY() + viewport()->height() - 1 ) );
//	for (int i=start; i<=end; i++) {

//! \todo js: this is NOT EFFECTIVE for big data sets!!!!

	KexiTableEdit *ed = dynamic_cast<KexiTableEdit*>( editor( colNum ) );
//	KexiDB::Field *f = m_data->column( colNum )->field;
	if (ed) {
//		KexiDB::Field *f = m_data->column(colNum)->field;
		for (QPtrListIterator<KexiTableItem> it = m_data->iterator(); it.current(); ++it) {
			maxw = QMAX( maxw, ed->widthForValue( it.current()->at( colNum ), fm ) );
//			maxw = QMAX( maxw, item->widthForValue( *f, it.current()->at( colNum ), fm ) );
		}
		maxw += (fm.width("  ") + ed->leftMargin() + ed->rightMargin());
	}
	if (maxw < KEXITV_MINIMUM_COLUMN_WIDTH )
		maxw = KEXITV_MINIMUM_COLUMN_WIDTH; //not too small
	setColumnWidth( colNum, maxw );
}

void KexiTableView::setColumnWidth(int colNum, int width)
{
	if (columns()<=colNum || colNum < 0)
		return;
	const int oldWidth = d->pTopHeader->sectionSize( colNum );
	d->pTopHeader->resizeSection( colNum, width );
	slotTopHeaderSizeChange( colNum, oldWidth, d->pTopHeader->sectionSize( colNum ) );
}

void KexiTableView::maximizeColumnsWidth( const QValueList<int> &columnList )
{
	if (!isVisible()) {
		d->maximizeColumnsWidthOnShow += columnList;
		return;
	}
	if (width() <= d->pTopHeader->headerWidth())
		return;
	//sort the list and make it unique
	QValueList<int>::const_iterator it;
	QValueList<int> cl, sortedList = columnList;
	qHeapSort(sortedList);
	int i=-999;

	for (it=sortedList.constBegin(); it!=sortedList.end(); ++it) {
		if (i!=(*it)) {
			cl += (*it);
			i = (*it);
		}
	}
	//resize
	int sizeToAdd = (width() - d->pTopHeader->headerWidth()) / cl.count() - verticalHeader()->width();
	if (sizeToAdd<=0)
		return;
	for (it=cl.constBegin(); it!=cl.end(); ++it) {
		int w = d->pTopHeader->sectionSize(*it);
		if (w>0) {
			d->pTopHeader->resizeSection(*it, w+sizeToAdd);
		}
	}
	updateContents();
	editorShowFocus( m_curRow, m_curCol );
}

void KexiTableView::adjustHorizontalHeaderSize()
{
	d->pTopHeader->adjustHeaderSize();
}

void KexiTableView::setColumnStretchEnabled( bool set, int colNum )
{
	d->pTopHeader->setStretchEnabled( set, colNum );
}

void KexiTableView::setEditableOnDoubleClick(bool set)
{
	d->editOnDoubleClick = set;
}
bool KexiTableView::editableOnDoubleClick() const
{
	return d->editOnDoubleClick;
}

bool KexiTableView::verticalHeaderVisible() const
{
	return m_verticalHeader->isVisible();
}

void KexiTableView::setVerticalHeaderVisible(bool set)
{
	int left_width;
	if (set) {
		m_verticalHeader->show();
		left_width = QMIN(d->pTopHeader->sizeHint().height(), d->rowHeight);
	}
	else {
		m_verticalHeader->hide();
		left_width = 0;
	}
	setMargins( left_width, horizontalHeaderVisible() ? d->pTopHeader->sizeHint().height() : 0, 0, 0);
}

bool KexiTableView::horizontalHeaderVisible() const
{
	return d->pTopHeader->isVisible();
}

void KexiTableView::setHorizontalHeaderVisible(bool set)
{
	int top_height;
	if (set) {
		d->pTopHeader->show();
		top_height = d->pTopHeader->sizeHint().height();
	}
	else {
		d->pTopHeader->hide();
		top_height = 0;
	}
	setMargins( verticalHeaderVisible() ? m_verticalHeader->width() : 0, top_height, 0, 0);
}

void KexiTableView::triggerUpdate()
{
//	kdDebug(44021) << "KexiTableView::triggerUpdate()" << endl;
//	if (!d->pUpdateTimer->isActive())
		d->pUpdateTimer->start(20, true);
//		d->pUpdateTimer->start(200, true);
}

void KexiTableView::setHBarGeometry( QScrollBar & hbar, int x, int y, int w, int h )
{
/*todo*/
	kdDebug(44021)<<"KexiTableView::setHBarGeometry"<<endl;
	if (d->appearance.navigatorEnabled) {
		m_navPanel->setHBarGeometry( hbar, x, y, w, h );
	}
	else {
		hbar.setGeometry( x , y, w, h );
	}
}

void KexiTableView::setSpreadSheetMode()
{
	KexiDataAwareObjectInterface::setSpreadSheetMode();
	//copy m_navPanelEnabled flag
	Appearance a = d->appearance;
	a.navigatorEnabled = m_navPanelEnabled;
	setAppearance( a );
}

bool KexiTableView::scrollbarToolTipsEnabled() const
{
	return d->scrollbarToolTipsEnabled;
}

void KexiTableView::setScrollbarToolTipsEnabled(bool set)
{
	d->scrollbarToolTipsEnabled=set;
}

int KexiTableView::validRowNumber(const QString& text)
{
	bool ok=true;
	int r = text.toInt(&ok);
	if (!ok || r<1)
		r = 1;
	else if (r > (rows()+(isInsertingEnabled()?1:0)))
		r = rows()+(isInsertingEnabled()?1:0);
	return r-1;
}

void KexiTableView::moveToRecordRequested( uint r )
{
	if (r > uint(rows()+(isInsertingEnabled()?1:0)))
		r = rows()+(isInsertingEnabled()?1:0);
	setFocus();
	selectRow( r );
}

void KexiTableView::moveToLastRecordRequested()
{
	setFocus();
	selectRow(rows()>0 ? (rows()-1) : 0);
}

void KexiTableView::moveToPreviousRecordRequested()
{
	setFocus();
	selectPrevRow();
}

void KexiTableView::moveToNextRecordRequested()
{
	setFocus();
	selectNextRow();
}

void KexiTableView::moveToFirstRecordRequested()
{
	setFocus();
	selectFirstRow();
}

bool KexiTableView::eventFilter( QObject *o, QEvent *e )
{
	//don't allow to stole key my events by others:
//	kdDebug() << "spontaneous " << e->spontaneous() << " type=" << e->type() << endl;

	if (e->type()==QEvent::KeyPress) {
		if (e->spontaneous() /*|| e->type()==QEvent::AccelOverride*/) {
			QKeyEvent *ke = static_cast<QKeyEvent*>(e);
			const int k = ke->key();
			int s = ke->state();
			//cell editor's events:
			//try to handle the event @ editor's level
			KexiTableEdit *edit = dynamic_cast<KexiTableEdit*>( editor( m_curCol ) );
			if (edit && edit->handleKeyPress(ke, m_editor==edit)) {
				ke->accept();
				return true;
			}
			else if (m_editor && (o==dynamic_cast<QObject*>(m_editor) || o==m_editor->widget())) {
				if ( (k==Key_Tab && (s==NoButton || s==ShiftButton))
					|| (overrideEditorShortcutNeeded(ke))
					|| (k==Key_Enter || k==Key_Return || k==Key_Up || k==Key_Down) 
					|| (k==Key_Left && m_editor->cursorAtStart())
					|| (k==Key_Right && m_editor->cursorAtEnd())
					)
				{
					//try to steal the key press from editor or it's internal widget...
					keyPressEvent(ke);
					if (ke->isAccepted())
						return true;
				}
			}
			/*
			else if (e->type()==QEvent::KeyPress && (o==this || (m_editor && o==m_editor->widget()))){//|| o==viewport())
				keyPressEvent(ke);
				if (ke->isAccepted())
					return true;
			}*/
/*todo			else if ((k==Key_Tab || k==(SHIFT|Key_Tab)) && o==d->navRowNumber) {
				//tab key focuses tv
				ke->accept();
				setFocus();
				return true;
			}*/
		}
	}
	else if (o==horizontalScrollBar()) {
		if ((e->type()==QEvent::Show && !horizontalScrollBar()->isVisible()) 
			|| (e->type()==QEvent::Hide && horizontalScrollBar()->isVisible())) {
			updateWidgetContentsSize();
		}
	}
	else if (e->type()==QEvent::Leave) {
		if (o==viewport() && d->appearance.rowMouseOverHighlightingEnabled
			&& d->appearance.persistentSelections)
		{
			if (d->highlightedRow!=-1) {
				int oldRow = d->highlightedRow;
				d->highlightedRow = -1;
				updateRow(oldRow);
				const bool dontPaintNonpersistentSelectionBecauseDifferentRowHasBeenHighlighted 
					= d->appearance.rowHighlightingEnabled && !d->appearance.persistentSelections;
				if (oldRow!=m_curRow && m_curRow>=0 && !dontPaintNonpersistentSelectionBecauseDifferentRowHasBeenHighlighted) {
					//no highlight for now: show selection again
					updateRow(m_curRow);
				}
			}
		}
	}
/*	else if (e->type()==QEvent::FocusOut && o->inherits("QWidget")) {
		//hp==true if currently focused widget is a child of this table view
		const bool hp = KexiUtils::hasParent( static_cast<QWidget*>(o), focusWidget());
		if (!hp && KexiUtils::hasParent( this, static_cast<QWidget*>(o))) {
			//accept row editing if focus is moved to foreign widget 
			//(not a child, like eg. editor) from one of our table view's children
			//or from table view itself
			if (!acceptRowEdit()) {
				static_cast<QWidget*>(o)->setFocus();
				return true;
			}
		}
	}*/
	return QScrollView::eventFilter(o,e);
}

void KexiTableView::vScrollBarValueChanged(int v)
{
	if (!d->vScrollBarValueChanged_enabled)
		return;
	kdDebug(44021) << "VCHANGED: " << v << " / " << horizontalScrollBar()->maxValue() <<  endl;
	
//	updateContents();
	m_verticalHeader->update(); //<-- dirty but needed

	if (d->scrollbarToolTipsEnabled) {
		QRect r = verticalScrollBar()->sliderRect();
		int row = rowAt(contentsY())+1;
		if (row<=0) {
			d->scrollBarTipTimer.stop();
			d->scrollBarTip->hide();
			return;
		}
		d->scrollBarTip->setText( i18n("Row: ") + QString::number(row) );
		d->scrollBarTip->adjustSize();
		d->scrollBarTip->move( 
		 mapToGlobal( r.topLeft() + verticalScrollBar()->pos() ) + QPoint( - d->scrollBarTip->width()-5, r.height()/2 - d->scrollBarTip->height()/2) );
		if (verticalScrollBar()->draggingSlider()) {
			kdDebug(44021) << "  draggingSlider()  " << endl;
			d->scrollBarTipTimer.stop();
			d->scrollBarTip->show();
			d->scrollBarTip->raise();
		}
		else {
			d->scrollBarTipTimerCnt++;
			if (d->scrollBarTipTimerCnt>4) {
				d->scrollBarTipTimerCnt=0;
				d->scrollBarTip->show();
				d->scrollBarTip->raise();
				d->scrollBarTipTimer.start(500, true);
			}
		}
	}
	//update bottom view region
	if (m_navPanel && (contentsHeight() - contentsY() - clipper()->height()) <= QMAX(d->rowHeight,m_navPanel->height())) {
		slotUpdate();
		triggerUpdate();
	}
}

void KexiTableView::vScrollBarSliderReleased()
{
	kdDebug(44021) << "vScrollBarSliderReleased()" << endl;
	d->scrollBarTip->hide();
}

void KexiTableView::scrollBarTipTimeout()
{
	if (d->scrollBarTip->isVisible()) {
		kdDebug(44021) << "TIMEOUT! - hide" << endl;
		if (d->scrollBarTipTimerCnt>0) {
			d->scrollBarTipTimerCnt=0;
			d->scrollBarTipTimer.start(500, true);
			return;
		}
		d->scrollBarTip->hide();
	}
	d->scrollBarTipTimerCnt=0;
}

void KexiTableView::slotTopHeaderSizeChange( 
	int /*section*/, int /*oldSize*/, int /*newSize*/ )
{
	editorShowFocus( m_curRow, m_curCol );
}

void KexiTableView::setBottomMarginInternal(int pixels)
{
	d->internal_bottomMargin = pixels;
}

void KexiTableView::paletteChange( const QPalette & )
{
}

KexiTableView::Appearance KexiTableView::appearance() const
{
	return d->appearance;
}

void KexiTableView::setAppearance(const Appearance& a)
{
//	if (d->appearance.fullRowSelection != a.fullRowSelection) {
	if (a.fullRowSelection) {
		d->rowHeight -= 1;
	}
	else {
		d->rowHeight += 1;
	}
	if (m_verticalHeader)
		m_verticalHeader->setCellHeight(d->rowHeight);
	if (d->pTopHeader) {
		setMargins(
			QMIN(d->pTopHeader->sizeHint().height(), d->rowHeight),
			d->pTopHeader->sizeHint().height(), 0, 0);
	}
//	}
	if (a.rowHighlightingEnabled)
		m_updateEntireRowWhenMovingToOtherRow = true;

	if(!a.navigatorEnabled)
		m_navPanel->hide();
	else
		m_navPanel->show();
//	}

	d->highlightedRow = -1;
//! @todo is setMouseTracking useful for other purposes?
	viewport()->setMouseTracking(a.rowMouseOverHighlightingEnabled);

	d->appearance = a;

	setFont(font()); //this also updates contents
}

int KexiTableView::highlightedRow() const
{
	return d->highlightedRow;
}

void KexiTableView::setHighlightedRow(int row)
{
	if (row!=-1) {
		row = QMIN(rows() - 1 + (isInsertingEnabled()?1:0), row);
		row = QMAX(0, row);
		ensureCellVisible(row, -1);
	}
	const int previouslyHighlightedRow = d->highlightedRow;
	if (previouslyHighlightedRow == row) {
		if (previouslyHighlightedRow!=-1)
			updateRow(previouslyHighlightedRow);
		return;
	}
	d->highlightedRow = row;
	if (d->highlightedRow!=-1)
		updateRow(d->highlightedRow);

	if (previouslyHighlightedRow!=-1)
		updateRow(previouslyHighlightedRow);

	if (m_curRow>=0 && (previouslyHighlightedRow==-1 || previouslyHighlightedRow==m_curRow)
		&& d->highlightedRow!=m_curRow && !d->appearance.persistentSelections)
	{
		//currently selected row needs to be repainted
		updateRow(m_curRow);
	}
}

KexiTableItem *KexiTableView::highlightedItem() const
{
	return d->highlightedRow == -1 ? 0 : m_data->at(d->highlightedRow);
}

void KexiTableView::slotSettingsChanged(int category)
{
	if (category==KApplication::SETTINGS_SHORTCUTS) {
		d->contextMenuKey = KGlobalSettings::contextMenuKey();
	}
}


#include "kexitableview.moc"

