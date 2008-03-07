/*
 * OpenRPT report writer and rendering engine
 * Copyright (C) 2001-2007 by OpenMFG, LLC
 * Copyright (C) 2007-2008 by Adam Pigg (adam@piggz.co.uk)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * Please contact info@openmfg.com with any questions on this license.
 */

#include "reportdesigner.h"
#include "reportsection.h"
#include "reportentities.h"
#include "reportscene.h"
#include "reportsceneview.h"
#include "reportentities.h"
#include "reportentitylabel.h"
#include "reportentityfield.h"
#include "reportentitytext.h"
#include "reportentityline.h"
#include "reportentitybarcode.h"
#include "reportentityimage.h"

// dialogs
#include "sectioneditor.h"
#include "reportsectiondetail.h"

// qt
#include <qpixmap.h>
#include <qlayout.h>
#include <qdom.h>
#include <qtextstream.h>
#include <qiodevice.h>
#include <qfile.h>
#include <QFileDialog>

#include <QCloseEvent>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <qregexp.h>
#include <qmessagebox.h>
#include <qsqlerror.h>
#include <qlineedit.h>
#include <qspinbox.h>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>

#include <kexidb/connection.h>
#include <kexidb/utils.h>
#include <kexidb/schemadata.h>
#include <kexiutils/tristate.h>

#include <koproperty/editor.h>
#include <KoRuler.h>
#include <KoZoomHandler.h>
#include <KoGlobal.h>
#include <KoPageFormat.h>
#include <kaction.h>
#include <kdebug.h>
//
// define and implement the ReportWriterSectionData class
// a simple class to hold/hide data in the ReportHandler class
//
class ReportWriterSectionData
{
	public:
		ReportWriterSectionData()
		{
			selected_items_rw = 0;
			mouseAction = ReportWriterSectionData::MA_None;
			insertItem = NoItem;
		}
		virtual ~ReportWriterSectionData()
		{
			selected_items_rw = 0;
		}

		enum ItemType
		{
			NoItem = 0,
			LabelItem,
			FieldItem,
			TextItem,
			LineItem,
			BarcodeItem,
			ImageItem,
			GraphItem
		};

		enum MouseAction
		{
			MA_None = 0,
			MA_Insert = 1,
			MA_Grab = 2,
			MA_MoveStartPoint,
			MA_MoveEndPoint,
			MA_ResizeNW = 8,
			MA_ResizeN,
			MA_ResizeNE,
			MA_ResizeE,
			MA_ResizeSE,
			MA_ResizeS,
			MA_ResizeSW,
			MA_ResizeW
		};

		int selected_x_offset;
		int selected_y_offset;

		ReportWindow * selected_items_rw;

		MouseAction mouseAction;
		ItemType insertItem;

		// copy data
		int copy_x_pos;        // the base x position of the copy (typically the first items original pos)
		int copy_y_pos;        // the base y position of the copy (typically the first items original pos)
		QList<ReportEntity*> copy_list;
		QList<ReportEntity*> cut_list;
};


ReportDesigner::ReportDesigner ( QWidget * parent, KexiDB::Connection *cn)
		: QWidget ( parent )
{
	conn = cn;
	init();
}
void ReportDesigner::init()
{
	_modified = false;
	detail = 0;
	hruler = 0;
	sectionData = new ReportWriterSectionData();
	createProperties();

	//setSizePolicy ( QSizePolicy::Expanding, QSizePolicy::Expanding );

	reportHead = reportFoot = 0;
	pageHeadFirst = pageHeadOdd = pageHeadEven = pageHeadLast = pageHeadAny = 0;
	pageFootFirst = pageFootOdd = pageFootEven = pageFootLast = pageFootAny = 0;

	grid = new QGridLayout ( this );
	grid->setSpacing ( 0 );
	grid->setMargin ( 0 );
	grid->setColumnStretch ( 1,1 );
	grid->setRowStretch ( 1,1 );
	grid->setSizeConstraint(QLayout::SetFixedSize);
	
	vboxlayout = new QVBoxLayout ( );
	vboxlayout->setSpacing ( 0 );
	vboxlayout->setMargin ( 0 );
	vboxlayout->setSizeConstraint(QLayout::SetFixedSize);
	//Create nice rulers
	zoom = new KoZoomHandler ();
	hruler = new KoRuler ( this, Qt::Horizontal, zoom );

	pageButton = new QPushButton ( this );

	//Messy, but i cant find another way
	delete hruler->tabChooser();
	hruler->setUnit ( KoUnit ( KoUnit::Centimeter ) );

	grid->addWidget ( pageButton, 0, 0 );
	grid->addWidget ( hruler, 0, 1 );
	grid->addLayout ( vboxlayout, 1,0, 1, 2 );

	pageButton->setMaximumSize ( QSize ( 22,22 ) );
	pageButton->setMinimumSize ( QSize ( 22,22 ) );

	detail = new ReportSectionDetail ( this );
	vboxlayout->insertWidget ( 0,detail );

	setLayout ( grid );

	connect ( pageButton, SIGNAL ( pressed() ), this, SLOT ( slotPageButton_Pressed() ) );
	emit pagePropertyChanged ( *set );

	connect ( set, SIGNAL ( propertyChanged ( KoProperty::Set &, KoProperty::Property & ) ), this, SLOT ( slotPropertyChanged ( KoProperty::Set &, KoProperty::Property & ) ) );
	
	changeSet ( set );
}

ReportDesigner::ReportDesigner (QWidget *parent, KexiDB::Connection *cn, const QString & d )
{
	kDebug() << "***********************************************************" << endl;
	kDebug() << d << endl;
	kDebug() << "***********************************************************" << endl;
	
	conn = cn;
	init();
	QDomDocument doc;
	doc.setContent(d);
	QDomElement root = doc.documentElement();
	if ( root.tagName() != "report" )
	{
		// arg we got an xml file but not one i know of
		kDebug() <<"root element was not <report>" << endl;;
	}
	
	deleteDetail();

	QDomNodeList nlist = root.childNodes();
	QDomNode it;

	for ( int i = 0; i < nlist.count(); i++ )
	{
		it = nlist.item ( i );
		// at this level all the children we get should be Elements
		if ( it.isElement() )
		{
			QString n = it.nodeName();
			if ( n == "title" )
			{
				setReportTitle ( it.firstChild().nodeValue() );
			}
//			else if ( n == "name" )
//			{
//				setReportName ( it.firstChild().nodeValue() );
//			}
			else if ( n == "datasource" )
			{
				setReportDataSource ( it.firstChild().nodeValue() );
			}
			else if ( n == "grid" )
			{
				setGridOptions ( it.toElement().attribute ( "visible" ).toInt() == 0?false:true, it.toElement().attribute ( "divisions" ).toInt() );
			}
			
			//TODO Load page options
			else if ( n == "size" )
			{
				if ( it.firstChild().isText() )
				{
					propertySet()->property("PageSize").setValue(it.firstChild().nodeValue());
				}
				else
				{
					//bad code! bad code!
					// this code doesn't check the elements and assumes they are what they should be.
					QDomNode n1 = it.firstChild();
					QDomNode n2 = n1.nextSibling();
					if ( n1.nodeName() == "width" )
					{
						propertySet()->property("CustomWidth").setValue(n1.firstChild().nodeValue().toDouble());
						
						propertySet()->property("CustomHeight").setValue(n2.firstChild().nodeValue().toDouble());
					}
					else
					{
						propertySet()->property("CustomHeight").setValue(n1.firstChild().nodeValue().toDouble());
						
						propertySet()->property("CustomWidth").setValue(n2.firstChild().nodeValue().toDouble());
					}
					propertySet()->property("PageSize").setValue("Custom");
				}
			}
			else if ( n == "labeltype" )
			{
				//TODO Labels
				//rd->pageOptions->setLabelType ( it.firstChild().nodeValue() );
			}
			else if ( n == "portrait" )
			{
				propertySet()->property("Orientation").setValue("Portrait");
			}
			else if ( n == "landscape" )
			{
				propertySet()->property("Orientation").setValue("Landscape");
			}
			else if ( n == "topmargin" )
			{
				
				propertySet()->property("TopMargin").setValue(pageUnit().toUserValue(it.firstChild().nodeValue().toDouble()));
			}
			else if ( n == "bottommargin" )
			{
				propertySet()->property("BottomMargin").setValue(pageUnit().toUserValue(it.firstChild().nodeValue().toDouble()));
			}
			else if ( n == "leftmargin" )
			{
				propertySet()->property("LeftMargin").setValue(pageUnit().toUserValue(it.firstChild().nodeValue().toDouble()));
			}
			else if ( n == "rightmargin" )
			{
				propertySet()->property("RightMargin").setValue(pageUnit().toUserValue(it.firstChild().nodeValue().toDouble()));
			}
			else if ( n == "rpthead" )
			{
				if ( getSection(ReportDesigner::ReportHead) == 0 )
				{
					insertSection(ReportDesigner::ReportHead);
					getSection(ReportDesigner::ReportHead)->initFromXML ( it );
				}
				else
				{
					kDebug() << "While loading xml tried to add more than one rpthead" << endl;
				}
			}
			else if ( n == "rptfoot" )
			{
				if ( getSection(ReportDesigner::ReportFoot) == 0 )
				{
					insertSection(ReportDesigner::ReportFoot);
					getSection(ReportDesigner::ReportFoot)->initFromXML ( it );
				}
				else
				{
					kDebug() << "While loading xml tried to add more than one rpthead" << endl;
				}
			}
			else if ( n == "pghead" )
			{
				// we need to determine which page this is for
				// firstpage | odd | even | lastpage
				// or any if none was specified
				ReportSection * rs = 0;
				if ( !it.namedItem ( "firstpage" ).isNull() )
				{
					if ( getSection(ReportDesigner::PageHeadFirst) == 0 )
					{
						insertSection(ReportDesigner::PageHeadFirst);
						rs = getSection(ReportDesigner::PageHeadFirst);
					}
					else
					{
						kDebug() << "tried to load more than one page head first" << endl;
					}
				}
				else if ( !it.namedItem ( "odd" ).isNull() )
				{
					if ( getSection(ReportDesigner::PageHeadOdd) == 0 )
					{
						insertSection(ReportDesigner::PageHeadOdd);
						rs = getSection(ReportDesigner::PageHeadOdd);
					}
					else
					{
						kDebug() << "tried to load more than one page head odd" << endl;
					}
				}
				else if ( !it.namedItem ( "even" ).isNull() )
				{
					if ( getSection(ReportDesigner::PageHeadEven) == 0 )
					{
						insertSection(ReportDesigner::PageHeadEven);
						rs = getSection(ReportDesigner::PageHeadEven);
					}
					else
					{
						kDebug() << "tried to load more than one page head even" << endl;
					}
				}
				else if ( !it.namedItem ( "lastpage" ).isNull() )
				{
					if ( getSection(ReportDesigner::PageHeadLast) == 0 )
					{
						insertSection(ReportDesigner::PageHeadLast);
						rs = getSection(ReportDesigner::PageHeadLast);
					}
					else
					{
						kDebug() << "tried to load more than one page head last" << endl;
					}
				}
				else
				{
					// we have an any pghead
					if ( getSection(ReportDesigner::PageHeadAny) == 0 )
					{
						insertSection(ReportDesigner::PageHeadAny);
						rs = getSection(ReportDesigner::PageHeadAny);
					}
					else
					{
						kDebug() << "tried to load more than one page head any" << endl;
					}
				}
				if ( rs ) rs->initFromXML ( it );
			}
			else if ( n == "pgfoot" )
			{
				// we need to determine which page this is for
				ReportSection * rs = 0;
				if ( !it.namedItem ( "firstpage" ).isNull() )
				{
					if ( getSection(ReportDesigner::PageFootFirst) == 0 )
					{
						insertSection(ReportDesigner::PageFootFirst);
						rs = getSection(ReportDesigner::PageFootFirst);
					}
					else
					{
						kDebug() << "tried to load more than one page foot first" << endl;
					}
				}
				else if ( !it.namedItem ( "odd" ).isNull() )
				{
					if ( getSection(ReportDesigner::PageFootOdd) == 0 )
					{
						insertSection(ReportDesigner::PageFootOdd);
						rs = getSection(ReportDesigner::PageFootOdd);
					}
					else
					{
						kDebug() << "tried to load more than one page foot odd" << endl;
					}
				}
				else if ( !it.namedItem ( "even" ).isNull() )
				{
					if ( getSection(ReportDesigner::PageFootEven) == 0 )
					{
						insertSection(ReportDesigner::PageFootEven);
						rs = getSection(ReportDesigner::PageFootEven);
					}
					else
					{
						kDebug() << "tried to load more than one page foot even" << endl;
					}
				}
				else if ( !it.namedItem ( "lastpage" ).isNull() )
				{
					if ( getSection(ReportDesigner::PageFootLast) == 0 )
					{
						insertSection(ReportDesigner::PageFootLast);
						rs = getSection(ReportDesigner::PageFootLast);
					}
					else
					{
						kDebug() << "tried to load more than one page foot last" << endl;
					}
				}
				else
				{
					// we have the any page foot
					if ( getSection(ReportDesigner::PageFootAny) == 0 )
					{
						insertSection(ReportDesigner::PageFootAny);
						rs = getSection(ReportDesigner::PageFootAny);
					}
					else
					{
						kDebug() << "tried to load more than one page foot any" << endl;
					}
				}
				if ( rs ) rs->initFromXML ( it );
			}
			else if ( n == "section" )
			{
				ReportSectionDetail * rsd = new ReportSectionDetail ( this );
				rsd->initFromXML ( it );
				setDetail(rsd);
			}
			else
			{
				kDebug() << "Encountered an unknown Element: "  << n << endl;
			}
		}
		else
		{
			kDebug() << "Encountered a child node of root that is not an Element" << endl;
		}
	}

	setModified ( false );
}

ReportDesigner::~ReportDesigner()
{
}

void ReportDesigner::closeEvent ( QCloseEvent * e )
{
	if ( _modified != false )
	{
		switch ( QMessageBox::information ( this,tr ( "Report Writer" ),tr ( "The document '%1' contains unsaved changes.\nDo you want to save the changes before closing?" ).arg ( windowTitle() ),tr ( "Save" ),tr ( "Discard" ),tr ( "Cancel" ), 0, 2 ) )
		{
			case 0:
				// save the doc...
				// if we get a not save result we'll bail so the
				// user doesn't loose any work.
//				if ( !saveToDb() ) return;
			case 1:
				// all we have to do is just accept the close event
				break;
			case 2:
				return;
			default:
				qDebug ( "Encountered a problem in the close event handler...." );
				// should we just go ahead and close??? or should we not close???
		}
	}
	e->accept();
}

void ReportDesigner::slotSectionEditor()
{
	SectionEditor * se = new SectionEditor ( this );
	se->init ( this );
	se->exec();
	delete se;
}

ReportSection * ReportDesigner::getSection(ReportDesigner::Section s) 
{ 
	ReportSection *sec;
	switch (s)
	{
		case ReportDesigner::PageHeadAny:
			sec = pageHeadAny;
			break;
		case ReportDesigner::PageHeadEven:
			sec = pageHeadEven;
			break;
		case ReportDesigner::PageHeadOdd:
			sec = pageHeadOdd;
			break;
		case ReportDesigner::PageHeadFirst:
			sec = pageHeadFirst;
			break;
		case ReportDesigner::PageHeadLast:
			sec = pageHeadLast;
			break;
		case ReportDesigner::PageFootAny:
			sec = pageFootAny;
			break;
		case ReportDesigner::PageFootEven:
			sec = pageFootEven;
			break;
		case ReportDesigner::PageFootOdd:
			sec = pageFootOdd;
			break;
		case ReportDesigner::PageFootFirst:
			sec = pageFootFirst;
			break;
		case ReportDesigner::PageFootLast:
			sec = pageFootLast;
			break;
		case ReportDesigner::ReportHead:
			sec = reportHead;
			break;
		case ReportDesigner::ReportFoot:
			sec = reportFoot;
			break;
		default:
			sec = NULL;
	}
	return sec;
}
void ReportDesigner::removeSection(ReportDesigner::Section s)
{
	ReportSection* sec = getSection(s);
	if ( sec != NULL )
	{
		delete sec;
		
		switch (s)
		{
			case ReportDesigner::PageHeadAny:
				pageHeadAny = NULL;
				break;
			case ReportDesigner::PageHeadEven:
				sec = pageHeadEven = NULL;
				break;
			case ReportDesigner::PageHeadOdd:
				pageHeadOdd = NULL;
				break;
			case ReportDesigner::PageHeadFirst:
				pageHeadFirst = NULL;
				break;
			case ReportDesigner::PageHeadLast:
				pageHeadLast = NULL;
				break;
			case ReportDesigner::PageFootAny:
				pageFootAny = NULL;
				break;
			case ReportDesigner::PageFootEven:
				pageFootEven = NULL;
				break;
			case ReportDesigner::PageFootOdd:
				pageFootOdd = NULL;
				break;
			case ReportDesigner::PageFootFirst:
				pageFootFirst = NULL;
				break;
			case ReportDesigner::PageFootLast:
				pageFootLast = NULL;
				break;
			case ReportDesigner::ReportHead:
				reportHead = NULL;
				break;
			case ReportDesigner::ReportFoot:
				reportFoot = NULL;
				break;
			default:
				sec = NULL;
		}
		
		setModified ( true );
		adjustSize();
	}
}
void ReportDesigner::insertSection(ReportDesigner::Section s)
{
	ReportSection* sec = getSection(s);
	if ( sec == NULL )
	{
		int idx = 0;
		for (int i = 1; i <= s; ++i)
		{
			if (getSection((ReportDesigner::Section)(i)) != NULL)
				idx++;
		}
		if (s > ReportDesigner::ReportHead)
			idx++;
		kDebug() << idx << endl;
		ReportSection *rs = new ReportSection ( this );
		vboxlayout->insertWidget ( idx,rs );
		
		switch (s)
		{
			case ReportDesigner::PageHeadAny:
				rs->setTitle ( i18n ( "Page Header (Any)" ) );
				pageHeadAny = rs;
				break;
			case ReportDesigner::PageHeadEven:
				rs->setTitle ( i18n ( "Page Header (Even)" ) );
				pageHeadEven = rs;
				break;
			case ReportDesigner::PageHeadOdd:
				rs->setTitle ( i18n ( "Page Header (Odd)" ) );
				pageHeadOdd = rs;
				break;
			case ReportDesigner::PageHeadFirst:
				rs->setTitle ( i18n ( "Page Header (First)" ) );
				pageHeadFirst = rs;
				break;
			case ReportDesigner::PageHeadLast:
				rs->setTitle ( i18n ( "Page Header (Last)" ) );
				pageHeadLast = rs;
				break;
			case ReportDesigner::PageFootAny:
				rs->setTitle ( i18n ( "Page Footer (Any)" ) );
				pageFootAny = rs;
				break;
			case ReportDesigner::PageFootEven:
				rs->setTitle ( i18n ( "Page Footer (Even)" ) );
				pageFootEven = rs;
				break;
			case ReportDesigner::PageFootOdd:
				rs->setTitle ( i18n ( "Page Footer (Odd)" ) );
				pageFootOdd = rs;
				break;
			case ReportDesigner::PageFootFirst:
				rs->setTitle ( i18n ( "Page Footer (First)" ) );
				pageFootFirst = rs;
				break;
			case ReportDesigner::PageFootLast:
				rs->setTitle ( i18n ( "Page Footer (Last)" ) );
				pageFootLast = rs;
				break;
			case ReportDesigner::ReportHead:
				rs->setTitle ( i18n ( "Report Header" ) );
				reportHead = rs;
				break;
			case ReportDesigner::ReportFoot:
				rs->setTitle ( i18n ( "Report Footer" ) );
				reportFoot = rs;
				break;
		}

		rs->show();
		setModified ( true );
		adjustSize();
		emit pagePropertyChanged ( *set );
	}
}

QDomDocument ReportDesigner::document()
{
	QDomDocument doc = QDomDocument ( "pgzKexiReportDef" );
	QDomElement root = doc.createElement ( "report" );
	doc.appendChild ( root );

	//title
	QDomElement title = doc.createElement ( "title" );
	title.appendChild ( doc.createTextNode ( reportTitle() ) );
	root.appendChild ( title );

	QDomElement rds = doc.createElement ( "datasource" );
	rds.appendChild ( doc.createTextNode ( reportDataSource() ) );
	root.appendChild ( rds );

	QDomElement grd = doc.createElement ( "grid" );
	grd.setAttribute ( "visible", _showGrid->value().toBool() );
	grd.setAttribute ( "divisions", _gridDivisions->value().toInt() );
	grd.setAttribute ( "snap", _gridSnap->value().toBool() );
	root.appendChild ( grd );

	// pageOptions
	// -- size
	QDomElement size = doc.createElement ( "size" );

	if ( _pageSize->value().toString() == "Custom" )
	{
		QDomElement page_width = doc.createElement ( "width" );
		page_width.appendChild ( doc.createTextNode ( QString::number ( pageUnit().fromUserValue ( _customWidth->value().toInt() ) ) ) );
		size.appendChild ( page_width );
		QDomElement page_height = doc.createElement ( "height" );
		page_height.appendChild ( doc.createTextNode ( QString::number ( pageUnit().fromUserValue ( _customWidth->value().toInt() ) ) ) );
		size.appendChild ( page_height );
	}
	else if ( _pageSize->value().toString() == "Labels" )
	{
		size.appendChild ( doc.createTextNode ( "Labels" ) );
		QDomElement labeltype = doc.createElement ( "labeltype" );
		labeltype.appendChild ( doc.createTextNode ( _labelType->value().toString() ) );
		root.appendChild ( labeltype );
	}
	else
	{
		size.appendChild ( doc.createTextNode ( _pageSize->value().toString() ) );
	}
	root.appendChild ( size );
	// -- orientation
	root.appendChild ( doc.createElement ( _orientation->value().toString().lower() ) );
	// -- margins
	QDomElement margin;
	margin = doc.createElement ( "topmargin" );
	margin.appendChild ( doc.createTextNode ( QString::number ( pageUnit().fromUserValue ( _topMargin->value().toInt() ) ) ) );
	root.appendChild ( margin );
	margin = doc.createElement ( "bottommargin" );
	margin.appendChild ( doc.createTextNode ( QString::number ( pageUnit().fromUserValue ( _bottomMargin->value().toInt() ) ) ) );
	root.appendChild ( margin );
	margin = doc.createElement ( "rightmargin" );
	margin.appendChild ( doc.createTextNode ( QString::number ( pageUnit().fromUserValue ( _rightMargin->value().toInt() ) ) ) );
	root.appendChild ( margin );
	margin = doc.createElement ( "leftmargin" );
	margin.appendChild ( doc.createTextNode ( QString::number ( pageUnit().fromUserValue ( _leftMargin->value().toInt() ) ) ) );
	root.appendChild ( margin );

	QDomElement section;

	// report head
	if ( reportHead )
	{
		section = doc.createElement ( "rpthead" );
		reportHead->buildXML ( doc, section );
		root.appendChild ( section );
	}

	// page head first
	if ( pageHeadFirst )
	{
		section = doc.createElement ( "pghead" );
		section.appendChild ( doc.createElement ( "firstpage" ) );
		pageHeadFirst->buildXML ( doc, section );
		root.appendChild ( section );
	}
	// page head odd
	if ( pageHeadOdd )
	{
		section = doc.createElement ( "pghead" );
		section.appendChild ( doc.createElement ( "odd" ) );
		pageHeadOdd->buildXML ( doc, section );
		root.appendChild ( section );
	}
	// page head even
	if ( pageHeadEven )
	{
		section = doc.createElement ( "pghead" );
		section.appendChild ( doc.createElement ( "even" ) );
		pageHeadEven->buildXML ( doc, section );
		root.appendChild ( section );
	}
	// page head last
	if ( pageHeadLast )
	{
		section = doc.createElement ( "pghead" );
		section.appendChild ( doc.createElement ( "lastpage" ) );
		pageHeadLast->buildXML ( doc, section );
		root.appendChild ( section );
	}
	// page head any
	if ( pageHeadAny )
	{
		section = doc.createElement ( "pghead" );
		pageHeadAny->buildXML ( doc, section );
		root.appendChild ( section );
	}

	section = doc.createElement ( "section" );
	detail->buildXML ( doc,section );
	root.appendChild ( section );

	// page foot first
	if ( pageFootFirst )
	{
		section = doc.createElement ( "pgfoot" );
		section.appendChild ( doc.createElement ( "firstpage" ) );
		pageFootFirst->buildXML ( doc, section );
		root.appendChild ( section );
	}
	// page foot odd
	if ( pageFootOdd )
	{
		section = doc.createElement ( "pgfoot" );
		section.appendChild ( doc.createElement ( "odd" ) );
		pageFootOdd->buildXML ( doc, section );
		root.appendChild ( section );
	}
	// page foot even
	if ( pageFootEven )
	{
		section = doc.createElement ( "pgfoot" );
		section.appendChild ( doc.createElement ( "even" ) );
		pageFootEven->buildXML ( doc, section );
		root.appendChild ( section );
	}
	// page foot last
	if ( pageFootLast )
	{
		section = doc.createElement ( "pgfoot" );
		section.appendChild ( doc.createElement ( "lastpage" ) );
		pageFootLast->buildXML ( doc, section );
		root.appendChild ( section );
	}
	// page foot any
	if ( pageFootAny )
	{
		section = doc.createElement ( "pgfoot" );
		pageFootAny->buildXML ( doc, section );
		root.appendChild ( section );
	}

	// report foot
	if ( reportFoot )
	{
		section = doc.createElement ( "rptfoot" );
		reportFoot->buildXML ( doc, section );
		root.appendChild ( section );
	}

	return doc;
}
						      
void ReportDesigner::setReportTitle ( const QString & str )
{
	if ( reportTitle() != str )
	{
		_title->setValue ( str );
		setModified ( true );
	}
}
QString ReportDesigner::reportTitle() { return _title->value().toString(); }

QString ReportDesigner::reportDataSource() { return _dataSource->value().toString(); }
void ReportDesigner::setReportDataSource ( const QString &q )
{
	if ( _dataSource->value().toString() != q )
	{
		_dataSource->setValue ( q );
		setModified ( true );
	}
}

bool ReportDesigner::isModified() { return _modified; }
void ReportDesigner::setModified ( bool mod )
{
	if ( _modified != mod )
	{
		_modified = mod;
	}
	if (_modified )
	{
		emit(dirty());

	}
}

QStringList ReportDesigner::queryList()
{
	//Get the list of queries in the database
	kDebug() << endl;
	QStringList qs;
	if ( conn && conn->isConnected() )
	{
		QList<int> tids = conn->tableIds();
		qs << "";
		for ( int i = 0; i < tids.size(); ++i )
		{
			KexiDB::TableSchema* tsc = conn->tableSchema ( tids[i] );
			if ( tsc )
				qs << tsc->name();
			else
				kDebug() << "Error retrieving table schema: " << tids[i] << endl;
		}

		QList<int> qids = conn->queryIds();
		qs << "";
		for ( int i = 0; i < qids.size(); ++i )
		{
			KexiDB::QuerySchema* qsc = conn->querySchema ( qids[i] );
			if ( qsc )
				qs << qsc->name();
			else
				kDebug() << "Error retrieving query schema: " << qids[i] << endl;
		}
	}
	kDebug() << "done" << endl;
	return qs;
}

QStringList ReportDesigner::fieldList ()
{
	QStringList qs;
	qs << "";

	if ( isConnected() )
	{
		//Get the list of fields in the selected query
		KexiDB::TableOrQuerySchema *flds = new KexiDB::TableOrQuerySchema ( conn, _dataSource->value().toString().toLocal8Bit() );

		KexiDB::QueryColumnInfo::Vector cs = flds->columns();

		for ( int i = 0 ; i < cs.count(); ++i )
		{
			qs << cs[i]->field->name();
		}
	}
	else
	{
		kDebug() << "Cannot return field list" << endl;
	}
	return qs;
}

void ReportDesigner::createProperties()
{
	QStringList keys, strings;
	set = new KoProperty::Set ( 0, "Report" );

	connect ( set, SIGNAL ( propertyChanged ( KoProperty::Set &, KoProperty::Property & ) ), this, SLOT ( propertyChanged ( KoProperty::Set &, KoProperty::Property & ) ) );

	_title = new KoProperty::Property ( "Title", "Report", "Title", "Report Title" );

	keys = queryList();
	_dataSource = new KoProperty::Property ( "DataSource", keys, keys, "", "Data Source" );

	keys.clear();
	keys = pageFormats();
	_pageSize = new KoProperty::Property ( "PageSize", keys, keys, "A4", "Page Size" );

	keys.clear();strings.clear();
	keys << "Portrait" << "Landscape";
	strings << i18n ( "Portrait" ) << i18n ( "Landscape" );
	_orientation = new KoProperty::Property ( "Orientation", keys, strings, "Portrait", "Page Orientation" );

	keys.clear();strings.clear();
	keys = KoUnit::listOfUnitName();
	strings = KoUnit::listOfUnitName();

	KoUnit u ( KoUnit::Centimeter );
	_unit = new KoProperty::Property ( "PageUnit", keys, strings, KoUnit::unitDescription ( u ), "Page Unit" );

	_showGrid = new KoProperty::Property ( "ShowGrid", true, "Show Grid", "Show Grid" );
	_gridSnap = new KoProperty::Property ( "GridSnap", true, "Grid Snap", "Grid Snap" );
	_gridDivisions = new KoProperty::Property ( "GridDivisions", 4, "Grid Divisions", "Grid Divisions" );

	_leftMargin = new KoProperty::Property ( "LeftMargin", 1.0, "Left Margin", "Left Margin" );
	_rightMargin = new KoProperty::Property ( "RightMargin", 1.0, "Right Margin", "Right Margin" );
	_topMargin = new KoProperty::Property ( "TopMargin", 1.0, "Top Margin", "Top Margin" );
	_bottomMargin = new KoProperty::Property ( "BottomMargin", 1.0, "Bottom Margin", "Bottom Margin" );

	set->addProperty ( _title );
	set->addProperty ( _dataSource );
	set->addProperty ( _pageSize );
	set->addProperty ( _orientation );
	set->addProperty ( _unit );
	set->addProperty ( _gridSnap );
	set->addProperty ( _showGrid );
	set->addProperty ( _gridDivisions );
	set->addProperty ( _leftMargin );
	set->addProperty ( _rightMargin );
	set->addProperty ( _topMargin );
	set->addProperty ( _bottomMargin );

	KoProperty::Property* _customHeight;
	KoProperty::Property* _customWidth;

}

void ReportDesigner::slotPropertyChanged ( KoProperty::Set &s, KoProperty::Property &p )
{
	emit pagePropertyChanged ( s );

	if ( p.name() == "PageUnit" )
	{
		hruler->setUnit ( pageUnit() );
	}

}

void ReportDesigner::slotPageButton_Pressed()
{
	changeSet ( set );
}

QStringList ReportDesigner::pageFormats()
{
	QStringList lst;
	lst << "A4" << "Letter" << "Legal" << "A3" << "A5";
	return lst;
}

QSize ReportDesigner::sizeHint() const
{
	int w = 0;
	int h = 0;
	
	if ( pageFootAny )
		h += pageFootAny->sizeHint().height();
	if ( pageFootEven )
		h += pageFootEven->sizeHint().height();
	if ( pageFootFirst )
		h += pageFootFirst->sizeHint().height();
	if ( pageFootLast )
		h += pageFootLast->sizeHint().height();
	if ( pageFootOdd )
		h += pageFootOdd->sizeHint().height();
	if ( pageHeadAny )
		h += pageHeadAny->sizeHint().height();
	if ( pageHeadEven )
		h += pageHeadEven->sizeHint().height();
	if ( pageHeadFirst )
		h += pageHeadFirst->sizeHint().height();
	if ( pageHeadLast )
		h += pageHeadLast->sizeHint().height();
	if ( pageHeadOdd )
		h += pageHeadOdd->sizeHint().height();
	if ( reportHead )
		h += reportHead->sizeHint().height();
	if ( reportFoot )
	{
		h += reportFoot->sizeHint().height();
	
	}	
	if ( detail )
	{
		h += detail->sizeHint().height();
		w += detail->sizeHint().width();
	}

	h+=hruler->height();

	return QSize ( w,h );
}

int ReportDesigner::pageWidthPx() const
{
	int cw = 0;
	int ch = 0;
	int width = 0;

	KoPageFormat::Format pf = KoPageFormat::formatFromString ( set->property ( "PageSize" ).value().toString() );

	cw = POINT_TO_INCH ( MM_TO_POINT ( KoPageFormat::width ( pf, KoPageFormat::Portrait ) ) ) * KoGlobal::dpiX();

	ch = POINT_TO_INCH ( MM_TO_POINT ( KoPageFormat::height ( pf, KoPageFormat::Portrait ) ) ) * KoGlobal::dpiX();

	width = ( set->property ( "Orientation" ).value().toString() == "Portrait" ? cw : ch );

	width = width - POINT_TO_INCH ( pageUnit().fromUserValue ( set->property ( "LeftMargin" ).value().toDouble() ) ) * KoGlobal::dpiX();
	width = width - POINT_TO_INCH ( pageUnit().fromUserValue ( set->property ( "RightMargin" ).value().toDouble() ) ) * KoGlobal::dpiX();

	return width;
}

void ReportDesigner::resizeEvent ( QResizeEvent * event )
{
	//hruler->setRulerLength ( vboxlayout->geometry().width() );
	hruler->setRulerLength ( pageWidthPx() );
}

void ReportDesigner::setDetail ( ReportSectionDetail *rsd )
{
	if ( detail == NULL )
	{
		int idx = 0;
		if ( pageHeadFirst ) idx++;
		if ( pageHeadOdd ) idx++;
		if ( pageHeadEven ) idx++;
		if ( pageHeadLast ) idx++;
		if ( pageHeadAny ) idx++;
		if ( reportHead ) idx++;
		detail = rsd;
		vboxlayout->insertWidget ( idx,detail );
	}
}
void ReportDesigner::deleteDetail()
{
	if ( detail != 0 )
	{
		delete detail;
		detail = 0;
	}
}

KoUnit ReportDesigner::pageUnit() const
{
	QString u;
	bool found;

	u = set->property ( "PageUnit" ).value().toString();
	u = u.mid ( u.indexOf ( "(" ) + 1, 2 );

	KoUnit unit = KoUnit::unit ( u, &found );
	if ( !found )
	{
		unit = KoUnit::unit ( "cm" );
	}

	return unit;
}

void ReportDesigner::setGridOptions ( bool vis, int div )
{
	_showGrid->setValue ( QVariant ( vis ) );
	_gridDivisions->setValue ( div );
}

//
// methods for the sectionMouse*Event()
//
void ReportDesigner::sectionMousePressEvent ( ReportScene * s, QGraphicsSceneMouseEvent * e )
{
	if ( e->button() == Qt::RightButton )
	{
		QMenu pop;

		QAction *popCut = 0;
		QAction *popCopy = 0;
		QAction *popPaste = 0;
		QAction *popProperties = 0;
		if ( selectionCount() > 0 )
		{
			popCut = pop.addAction ( i18n ( "Cut" ) );
			popCopy = pop.addAction ( i18n ( "Copy" ) );
		}
		if ( sectionData->copy_list.count() > 0 )
			popPaste = pop.addAction ( i18n ( "Paste" ) );

		QAction * ret = pop.exec ( QCursor::pos() );
		if ( ret == popCut )
			slotEditCut();
		else if ( ret == popCopy )
			slotEditCopy();
		else if ( ret == popPaste )
			slotEditPaste ( s, e->scenePos() );
	}
}

void ReportDesigner::sectionMouseReleaseEvent ( ReportSceneView * v, QMouseEvent * e )
{
	e->accept();
	QGraphicsItem * item = 0;
	if ( e->button() == Qt::LeftButton )
	{
		switch ( sectionData->mouseAction )
		{
			case ReportWriterSectionData::MA_Insert:
				switch ( sectionData->insertItem )
				{
					case ReportWriterSectionData::LabelItem :
						item = new ReportEntityLabel ( v->document(), v->scene() );
						break;
					case ReportWriterSectionData::FieldItem :
						item = new ReportEntityField ( v->document(), v->scene() );
						break;
					case ReportWriterSectionData::TextItem :
						item = new ReportEntityText ( v->document(), v->scene() );
						break;
					case ReportWriterSectionData::BarcodeItem :
						item = new ReportEntityBarcode ( v->document(), v->scene() );
						break;
					case ReportWriterSectionData::ImageItem :
						item = new ReportEntityImage ( v->document(), v->scene() );
						break;
					case ReportWriterSectionData::LineItem :
						kDebug() << "Adding Line" << endl;
						item = new ReportEntityLine ( v->document(), v->scene() );
						//dynamic_cast<QGraphicsLineItem*>(item)->setLine ( e->x()-10, e->y(), e->x()+10, e->y() );
						dynamic_cast<QGraphicsLineItem*> ( item )->setLine ( e->x(), e->y(), e->x() +20, e->y() );
						break;
						//TODO add graph
						//case ReportWriterSectionData::GraphItem :
						//  item = new ReportEntityGraph(v->document(), v->canvas());
						//  break;
					default:
						kDebug() << "attempted to insert an unknown item" << endl;;
				}
				if ( item )
				{
					if ( sectionData->insertItem != ReportWriterSectionData::LineItem )
						item->setPos ( e->x(), e->y() );

					item->setVisible ( true );
					if ( v && v->document() )
						v->document()->setModified ( true );
				}

				sectionData->mouseAction = ReportWriterSectionData::MA_None;
				sectionData->insertItem = ReportWriterSectionData::NoItem;
				break;
			default:
				// what to do? Nothing
				// either we don't know what is going on
				// or everything has been done elsewhere
				break;
		}
	}
}

unsigned int ReportDesigner::selectionCount()
{
	return activeScene()->selectedItems().count();
}

//
// Item Actions
//
void ReportDesigner::slotItemLabel()
{
	sectionData->mouseAction = ReportWriterSectionData::MA_Insert;
	sectionData->insertItem = ReportWriterSectionData::LabelItem;
}

void ReportDesigner::slotItemField()
{
	sectionData->mouseAction = ReportWriterSectionData::MA_Insert;
	sectionData->insertItem = ReportWriterSectionData::FieldItem;
}

void ReportDesigner::slotItemText()
{
	sectionData->mouseAction = ReportWriterSectionData::MA_Insert;
	sectionData->insertItem = ReportWriterSectionData::TextItem;
}

void ReportDesigner::slotItemLine()
{
	sectionData->mouseAction = ReportWriterSectionData::MA_Insert;
	sectionData->insertItem = ReportWriterSectionData::LineItem;
}

void ReportDesigner::slotItemBarcode()
{
	sectionData->mouseAction = ReportWriterSectionData::MA_Insert;
	sectionData->insertItem = ReportWriterSectionData::BarcodeItem;
}

void ReportDesigner::slotItemImage()
{
	sectionData->mouseAction = ReportWriterSectionData::MA_Insert;
	sectionData->insertItem = ReportWriterSectionData::ImageItem;
}

void ReportDesigner::slotItemGraph()
{
	sectionData->mouseAction = ReportWriterSectionData::MA_Insert;
	sectionData->insertItem = ReportWriterSectionData::GraphItem;
}

void ReportDesigner::changeSet ( KoProperty::Set *s )
{
	_itmset = s;
	emit(propertySetChanged());
}



//
// Edit Actions
//
void ReportDesigner::slotEditDelete()
{
	QGraphicsItem * item = 0;
	while ( selectionCount() > 0 )
	{
		item = activeScene()->selectedItems() [0];
		if ( item )
		{
			setModified ( true );
			QGraphicsScene * scene = item->scene();
			delete item;
			scene->update();
			sectionData->mouseAction = ReportWriterSectionData::MA_None;
		}
	}
	activeScene()->selectedItems().clear();
	sectionData->selected_items_rw = 0;

	//TODO temporary
	//clears cut and copy lists to make sure we dont crash
	//if weve deleted something in the list
	//should really check if an item is in the list first
	//and remove it.
	sectionData->cut_list.clear();
	sectionData->copy_list.clear();
}

void ReportDesigner::slotEditCut()
{
	if ( selectionCount() > 0 )
	{
		//First delete any items that are curerntly in the list
		//so as not to leak memory
		for ( int i = 0; i < sectionData->cut_list.count(); i++ )
		{
			delete sectionData->cut_list[i];
		}
		sectionData->cut_list.clear();
		
		QGraphicsItem * item = activeScene()->selectedItems().first();
		if ( item )
		{
			if ( item->type() == KRObjectData::EntityLine )
			{
				sectionData->copy_x_pos = ( ( ReportEntityLine* ) item )->line().p1().x();
				sectionData->copy_y_pos = ( ( ReportEntityLine* ) item )->line().p1().y();
			}
			else
			{
				sectionData->copy_x_pos = ( int ) item->x();
				sectionData->copy_y_pos = ( int ) item->y();
			}
			
			sectionData->copy_list.clear();
			
			for ( int i = 0; i < activeScene()->selectedItems().count(); i++ )
			{
				QGraphicsItem *itm = activeScene()->selectedItems()[i];
				sectionData->cut_list.append (dynamic_cast<ReportEntity*>(itm));
				sectionData->copy_list.append(dynamic_cast<ReportEntity*>(itm));
			}
			int c = activeScene()->selectedItems().count();
			for ( int i = 0; i < c; i++ )
			{
				QGraphicsItem *itm = activeScene()->selectedItems()[0];
				activeScene()->removeItem(itm);
				activeScene()->update();
			}
			sectionData->selected_x_offset = 10;
			sectionData->selected_y_offset = 10;
		}
	}
}

void ReportDesigner::slotEditCopy()
{
	if ( selectionCount() < 1 )
		return;
	
	QGraphicsItem * item = activeScene()->selectedItems().first();
	if ( item )
	{
		sectionData->copy_list.clear();
		if ( item->type() == KRObjectData::EntityLine )
		{
			sectionData->copy_x_pos = ( ( ReportEntityLine* ) item )->line().p1().x();
			sectionData->copy_y_pos = ( ( ReportEntityLine* ) item )->line().p1().y();
		}
		else
		{
			sectionData->copy_x_pos = ( int ) item->x();
			sectionData->copy_y_pos = ( int ) item->y();
		}
		
		for ( int i = 0; i < activeScene()->selectedItems().count(); i++ )
		{
			sectionData->copy_list.append ( dynamic_cast<ReportEntity*>(activeScene()->selectedItems() [i]) );
		}
		sectionData->selected_x_offset = 10;
		sectionData->selected_y_offset = 10;
	}
}

void ReportDesigner::slotEditPaste()
{
	// call the editPaste function passing it a reportsection and point
	//  that make sense as defaults (same canvas / slightly offset pos of orig copy)
	QPoint p;
	p.setX ( sectionData->copy_x_pos + sectionData->selected_x_offset );
	p.setY ( sectionData->copy_y_pos + sectionData->selected_x_offset );
	slotEditPaste ( activeScene(), p );
}

void ReportDesigner::slotEditPaste ( QGraphicsScene * canvas, const QPointF & pos )
{
	// paste a new item of the copy we have in the specified location
	if ( sectionData->copy_list.count() > 0 )
	{
		QGraphicsItem * pasted_ent = 0;
		canvas->clearSelection();
		sectionData->mouseAction = ReportWriterSectionData::MA_None;
		
		for ( int i = 0; i < sectionData->copy_list.count(); i++ )
		{
			pasted_ent = 0;
			int type = dynamic_cast<KRObjectData*> ( sectionData->copy_list[i] )->type();
			kDebug() << type << endl;
			QPointF o(sectionData->selected_x_offset,sectionData->selected_y_offset);
			if ( type == KRObjectData::EntityLabel )
			{
				ReportEntityLabel * ent = dynamic_cast<ReportEntityLabel*>(sectionData->copy_list[i])->clone();
				ent->setPos(ent->pos() + o);
				pasted_ent = ent;
			}
			else if ( type == KRObjectData::EntityField  )
			{
				ReportEntityField * ent = dynamic_cast<ReportEntityField*>(sectionData->copy_list[i])->clone();
				ent->setPos(ent->pos() + o);
				pasted_ent = ent;
			}
			else if ( type == KRObjectData::EntityText  )
			{
				ReportEntityText * ent = dynamic_cast<ReportEntityText*>(sectionData->copy_list[i])->clone();
				ent->setPos(ent->pos() + o);
				pasted_ent = ent;
			}
			else if ( type == KRObjectData::EntityLine  )
			{
				ReportEntityLine * ent = dynamic_cast<ReportEntityLine*>(sectionData->copy_list[i])->clone();
				
				pasted_ent = ent;
			}
			else if ( type == KRObjectData::EntityBarcode  )
			{
				ReportEntityBarcode * ent = dynamic_cast<ReportEntityBarcode*>(sectionData->copy_list[i])->clone();
				ent->setPos(ent->pos() + o);
				pasted_ent = ent;
			}
			else if ( type == KRObjectData::EntityImage  )
			{
				ReportEntityImage * ent = dynamic_cast<ReportEntityImage*>(sectionData->copy_list[i])->clone();
				ent->setPos(ent->pos() + o);
				pasted_ent = ent;
			}
			//TODO add graph
			//else if(cp.copy_item == ReportWriterSectionData::GraphItem) {
			//    ReportEntityGraph * ent = new ReportEntityGraph(cp.copy_graph, rw, canvas);
			//    ent->setX(pos.x() + cp.copy_offset_x);
			//    ent->setY(pos.y() + cp.copy_offset_y);
			//    ent->setSize(cp.copy_rect.width(), cp.copy_rect.height());
			//    ent->show();
			//    pasted_ent = ent;
			//}
			else
			{
				kDebug() << "Tried to paste an item I don't understand." << endl;
			}
			
			if ( pasted_ent )
			{
				canvas->addItem(pasted_ent);
				pasted_ent->show();
				sectionData->mouseAction = ReportWriterSectionData::MA_Grab;
				setModified ( true );
			}
		}
		sectionData->selected_x_offset += 10;
		sectionData->selected_y_offset += 10;
	}
}
void ReportDesigner::slotRaiseSelected()
{
	dynamic_cast<ReportScene*>(activeScene())->raiseSelected();
}

void ReportDesigner::slotLowerSelected()
{
	dynamic_cast<ReportScene*>(activeScene())->lowerSelected();
}