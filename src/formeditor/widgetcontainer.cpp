/* This file is part of the KDE libraries
   Copyright (C) 2002 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kaction.h>
#include <klineedit.h>

#include <qptrlist.h>
#include <qsize.h>
#include <qpainter.h>
#include <qpen.h>
#include <qcolor.h>
#include <qcursor.h>
#include <qpixmap.h>
#include <qcolor.h>
#include <qpushbutton.h>
#include <qtimer.h>
#include <kurlrequester.h>

#include <qobjectlist.h>

#include "widgetcontainer.h"
#include "resizehandle.h"

namespace KFormEditor {

	WidgetContainer::WidgetContainer(QWidget *parent, const char *name, QString identifier)
	: QWidget(parent,name)
	{
		setMinimumWidth(50);
		setMinimumHeight(50);

	
		setCaption(i18n("%1 [Edit Mode]").arg(identifier));

		KIconLoader *iloader = KGlobal::iconLoader();
		setIcon(iloader->loadIcon("form", KIcon::Small));
	
		m_dotSpacing = dotSpacing();

		resize( 250, 250 );

		setBackgroundPixmap(m_dotBg);
	
		m_widgetRectRequested = false;
		m_widgetRect = false;
		m_activeWidget=0;
		m_activeMoveWidget=0;
		m_resizeHandleSet=0;

	}

	int WidgetContainer::dotSpacing()
	{ 
		return 10;
	}

	void WidgetContainer::slotWidgetLineEdit()
	{
		kdDebug() << "add line edit widget at " << this << endl;
		m_pendingWidget = new KLineEdit(this);
		m_widgetRectRequested = true;
	}

	void WidgetContainer::slotWidgetPushButton()
	{
		m_pendingWidget = new QPushButton("push button", this);
		m_widgetRectRequested = true;
	}

	void WidgetContainer::slotWidgetURLRequester()
	{
		m_pendingWidget = new KURLRequester("urlrequest", this);
		m_widgetRectRequested = true;
	}

	void WidgetContainer::mouseMoveEvent(QMouseEvent *ev)
	{
		if(m_widgetRectRequested)
		{
			m_widgetRect = true;
			m_widgetRectBX = (((float)ev->x())/((float)m_dotSpacing)+0.5);
			m_widgetRectBX*=m_dotSpacing;
			m_widgetRectBY = (((float)ev->y())/((float)m_dotSpacing)+0.5);
			m_widgetRectBY*=m_dotSpacing;
			m_widgetRectEX = m_widgetRectBX;
			m_widgetRectEY = m_widgetRectBY;
			m_widgetRectRequested = false;
		}

		if(m_widgetRect)
		{
        	        m_widgetRectEX = (((float)ev->x())/((float)m_dotSpacing)+0.5);
			m_widgetRectEX*=m_dotSpacing;
	                m_widgetRectEY = (((float)ev->y())/((float)m_dotSpacing)+0.5);
			m_widgetRectEY*=m_dotSpacing;
			repaint();
		}
	}

	void WidgetContainer::resizeEvent(QResizeEvent *ev)
	{
		QPainter *p = new QPainter();
		m_dotBg = QPixmap(size());
		p->begin(&m_dotBg, this);
	
		QColor c = paletteBackgroundColor();
		p->setPen(QPen(c));
		QBrush bg(c);
		p->setBrush(bg);
		p->drawRect(0, 0, width(), height());
		QPen dots(black, 1);
		p->setPen(dots);
		int cols = width() / m_dotSpacing;
		int rows = height() / m_dotSpacing;
	
		for(int rowcursor = 1; rowcursor < rows; ++rowcursor)
		{
			for(int colcursor = 1; colcursor < cols; ++colcursor)
			{
				p->drawPoint(colcursor * m_dotSpacing, rowcursor * m_dotSpacing);
			}
		}
	
		p->end();
		setPaletteBackgroundPixmap(m_dotBg);
	}

	void WidgetContainer::paintEvent(QPaintEvent *ev)
	{
		QPainter p(this);
		if(m_widgetRect)
		{
			QPen wpen(black, 2);
			p.setPen(wpen);
			p.drawRect(m_widgetRectBX, m_widgetRectBY, m_widgetRectEX-m_widgetRectBX, m_widgetRectEY-m_widgetRectBY);
		}
		p.end();
	}


	void WidgetContainer::installEventFilterRecursive(QObject *obj)
	{
		obj->installEventFilter(this);
		static_cast<QWidget*>(obj)->setCursor(QCursor(SizeAllCursor));
		if ( obj->children() )
		{
			QObjectListIt it( *obj->children() );
			QObject *obj1;
	        	while( (obj1=it.current()) != 0 )
			{
				++it;
				if (obj1->isWidgetType())
			    	installEventFilterRecursive(obj1);
	            	}
	        }

	}

	void WidgetContainer::mouseReleaseEvent(QMouseEvent *ev)
	{
		if(m_widgetRect)
		{
			insertWidget(m_pendingWidget, m_widgetRectBX, m_widgetRectBY,
				m_widgetRectEX-m_widgetRectBX, m_widgetRectEY-m_widgetRectBY);
			m_widgetRectBX = 0;
			m_widgetRectBY = 0;
			m_widgetRectEX = 0;
			m_widgetRectEY = 0;

			installEventFilterRecursive(m_pendingWidget);

			m_widgetRect = false;


			repaint();
		}
	}

	void WidgetContainer::insertWidget(QWidget *widget, int x, int y, int w, int h)
	{
		widget->move(x, y);
		widget->resize(w, h);
		widget->show();
		widget->setFocusPolicy(QWidget::NoFocus);
		activateWidget(widget);
	}

	void WidgetContainer::setResizeHandles(QWidget *m_activeWidget)
	{
		if (!m_resizeHandleSet)
		{
			m_resizeHandleSet=new ResizeHandleSet(m_activeWidget);
			return;
		}
		if (m_resizeHandleSet &&(m_resizeHandleSet->widget()!=m_activeWidget))
		{
			delete m_resizeHandleSet;
			m_resizeHandleSet=new ResizeHandleSet(m_activeWidget);
		}

	}

	void WidgetContainer::activateWidget(QWidget *widget)
	{
		m_activeWidget=widget;
		while (!(m_activeWidget->parentWidget(true)==this))
			m_activeWidget=m_activeWidget->parentWidget();
		setResizeHandles(m_activeWidget);

	}

	bool WidgetContainer::eventFilter(QObject *obj, QEvent *ev)
	{
		kdDebug() << "event!" << endl;
		QWidget *sh;
		switch (ev->type())
		{
			case QEvent::MouseButtonPress:
				activateWidget(static_cast<QWidget*>(obj));
				m_activeMoveWidget=m_activeWidget;
				m_moveBX=static_cast<QMouseEvent*>(ev)->x();
				m_moveBY=static_cast<QMouseEvent*>(ev)->y();
				return true;
				break;
			case QEvent::MouseButtonRelease:
				m_activeMoveWidget=0;
				return true;
				break;
			case QEvent::MouseMove:
				if (m_activeMoveWidget)
				{
					int tmpx,tmpy;
			                tmpx = (((float)(m_activeMoveWidget->x()+static_cast<QMouseEvent*>(ev)->x()-m_moveBX))/
						((float)m_dotSpacing)+0.5);
			                tmpx*=m_dotSpacing;
			                tmpy = (((float)(m_activeMoveWidget->y()+static_cast<QMouseEvent*>(ev)->y()-m_moveBY))/
						((float)m_dotSpacing)+0.5);
			                tmpy*=m_dotSpacing;
					if ((tmpx!=m_activeMoveWidget->x()) ||(tmpy!=m_activeMoveWidget->y()) )
						m_activeMoveWidget->move(tmpx,tmpy);
				}
				return true;
				break;
			default:
				break;
		}
		switch( ev->type() )
		{
			case QEvent::MouseButtonPress:
			case QEvent::MouseButtonRelease:
			case QEvent::MouseButtonDblClick:
			case QEvent::MouseMove:
			case QEvent::KeyPress:
			case QEvent::KeyRelease:
			case QEvent::Enter:
			case QEvent::Leave:
			case QEvent::Wheel:
			case QEvent::ContextMenu:
				return true; // ignore
			default:
				break;
		}
		return false;
	}

	WidgetContainer::~WidgetContainer(){
	}

};

#include "widgetcontainer.moc"
