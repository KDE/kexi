/* This file is part of the KDE project
   Copyright (C) 2004 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2005-2006 Jaroslaw Staniek <js@iidea.pl>

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

#include <qobjectlist.h>
#include <qpainter.h>
#include <qcursor.h>
#include <qapplication.h>
#include <qfocusdata.h>

#include <kdebug.h>

#include "kexidbform.h"
#include "kexiformscrollview.h"

#include <formeditor/objecttree.h>
#include <formeditor/formmanager.h> 
#include <widget/tableview/kexidataawareobjectiface.h>
#include <widget/kexiscrollview.h>

//! @internal
class KexiDBForm::Private
{
	public:
		Private()
		 : dataAwareObject(0)
		 , orderedFocusWidgetsIterator(orderedFocusWidgets)
		 , autoTabStops(false)
		 , popupFocused(false)
		{
		}
		KexiDataAwareObjectInterface* dataAwareObject;
		//! ordered list of focusable widgets (can be both data-widgets or buttons, etc.)
		QPtrList<QWidget> orderedFocusWidgets; 
		//! ordered list of data-aware widgets
		QPtrList<QWidget> orderedDataAwareWidgets;
		QMap<KexiDataItemInterface*, uint> indicesForDataAwareWidgets; //!< a subset of orderedFocusWidgets mapped to indices
		QPtrListIterator<QWidget> orderedFocusWidgetsIterator;
		QPixmap buffer; //!< stores grabbed entire form's area for redraw
		QRect prev_rect; //!< previously selected rectangle
//		QGuardedPtr<QWidget> widgetFocusedBeforePopup;
		bool autoTabStops : 1;
		bool popupFocused : 1; //!< used in KexiDBForm::eventFilter()
};

//========================

KexiDBForm::KexiDBForm(QWidget *parent, KexiDataAwareObjectInterface* dataAwareObject, 
	const char *name/*, KexiDB::Connection *conn*/)
 : KexiDBFormBase(parent, name)
 , KexiFormDataItemInterface()
 , d(new Private())
{
	installEventFilter(this);
//test	setDisplayMode( KexiGradientWidget::SimpleGradient );
	editedItem = 0;
	d->dataAwareObject = dataAwareObject;
	m_hasFocusableWidget = false;

//	setFocusPolicy(NoFocus);

	//m_conn = conn;
	kexipluginsdbg << "KexiDBForm::KexiDBForm(): " << endl;
	setCursor(QCursor(Qt::ArrowCursor)); //to avoid keeping Size cursor when moving from form's boundaries
	setAcceptDrops( true );
}

KexiDBForm::~KexiDBForm()
{
	kexipluginsdbg << "KexiDBForm::~KexiDBForm(): close" << endl;
	delete d;
}

KexiDataAwareObjectInterface* KexiDBForm::dataAwareObject() const { return d->dataAwareObject; }

//repaint all children widgets
static void repaintAll(QWidget *w)
{
	QObjectList *list = w->queryList("QWidget");
	QObjectListIt it(*list);
	for (QObject *obj; (obj=it.current()); ++it ) {
		static_cast<QWidget*>(obj)->repaint();
	}
	delete list;
}

void
KexiDBForm::drawRect(const QRect& r, int type)
{
	QValueList<QRect> l;
	l.append(r);
	drawRects(l, type);
}

void
KexiDBForm::drawRects(const QValueList<QRect> &list, int type)
{
	QPainter p;
	p.begin(this, true);
	bool unclipped = testWFlags( WPaintUnclipped );
	setWFlags( WPaintUnclipped );

	if (d->prev_rect.isValid()) {
		//redraw prev. selection's rectangle
		p.drawPixmap( QPoint(d->prev_rect.x()-2, d->prev_rect.y()-2), d->buffer, 
			QRect(d->prev_rect.x()-2, d->prev_rect.y()-2, d->prev_rect.width()+4, d->prev_rect.height()+4));
	}
	p.setBrush(QBrush::NoBrush);
	if(type == 1) // selection rect
		p.setPen(QPen(white, 1, Qt::DotLine));
	else if(type == 2) // insert rect
		p.setPen(QPen(white, 2));
	p.setRasterOp(XorROP);

	d->prev_rect = QRect();
	QValueList<QRect>::ConstIterator endIt = list.constEnd();
	for(QValueList<QRect>::ConstIterator it = list.constBegin(); it != endIt; ++it) {
		p.drawRect(*it);
		if (d->prev_rect.isValid())
			d->prev_rect = d->prev_rect.unite(*it);
		else
			d->prev_rect = *it;
	}

	if (!unclipped)
		clearWFlags( WPaintUnclipped );
	p.end();
}

void
KexiDBForm::initBuffer()
{
	repaintAll(this);
	d->buffer.resize( width(), height() );
	d->buffer = QPixmap::grabWindow( winId() );
	d->prev_rect = QRect();
}

void
KexiDBForm::clearForm()
{
	QPainter p;
	p.begin(this, true);
	bool unclipped = testWFlags( WPaintUnclipped );
	setWFlags( WPaintUnclipped );

	//redraw entire form surface
	p.drawPixmap( QPoint(0,0), d->buffer, QRect(0,0,d->buffer.width(), d->buffer.height()) );

	if (!unclipped)
		clearWFlags( WPaintUnclipped );
	p.end();

	repaintAll(this);
}

void
KexiDBForm::highlightWidgets(QWidget *from, QWidget *to)//, const QPoint &point)
{
	QPoint fromPoint, toPoint;
	if(from && from->parentWidget() && (from != this))
		fromPoint = from->parentWidget()->mapTo(this, from->pos());
	if(to && to->parentWidget() && (to != this))
		toPoint = to->parentWidget()->mapTo(this, to->pos());

	QPainter p;
	p.begin(this, true);
	bool unclipped = testWFlags( WPaintUnclipped );
	setWFlags( WPaintUnclipped );

	if (d->prev_rect.isValid()) {
		//redraw prev. selection's rectangle
		p.drawPixmap( QPoint(d->prev_rect.x(), d->prev_rect.y()), d->buffer, 
			QRect(d->prev_rect.x(), d->prev_rect.y(), d->prev_rect.width(), d->prev_rect.height()));
	}

	p.setPen( QPen(Qt::red, 2) );

	if(to)
	{
		QPixmap pix1 = QPixmap::grabWidget(from);
		QPixmap pix2 = QPixmap::grabWidget(to);

		if((from != this) && (to != this))
			p.drawLine( from->parentWidget()->mapTo(this, from->geometry().center()), to->parentWidget()->mapTo(this, to->geometry().center()) );

		p.drawPixmap(fromPoint.x(), fromPoint.y(), pix1);
		p.drawPixmap(toPoint.x(), toPoint.y(), pix2);

		if(to == this)
			p.drawRoundRect(2, 2, width()-4, height()-4, 4, 4);
		else
			p.drawRoundRect(toPoint.x(), toPoint.y(), to->width(), to->height(), 5, 5);
	}

	if(from == this)
		p.drawRoundRect(2, 2, width()-4, height()-4, 4, 4);
	else
		p.drawRoundRect(fromPoint.x(),  fromPoint.y(), from->width(), from->height(), 5, 5);

	if((to == this) || (from == this))
		d->prev_rect = QRect(0, 0, d->buffer.width(), d->buffer.height());
	else if(to)
	{
		d->prev_rect.setX( (fromPoint.x() < toPoint.x()) ? (fromPoint.x() - 5) : (toPoint.x() - 5) );
		d->prev_rect.setY( (fromPoint.y() < toPoint.y()) ? (fromPoint.y() - 5) : (toPoint.y() - 5) );
		d->prev_rect.setRight( (fromPoint.x() < toPoint.x()) ? (toPoint.x() + to->width() + 10) : (fromPoint.x() + from->width() + 10) );
		d->prev_rect.setBottom( (fromPoint.y() < toPoint.y()) ? (toPoint.y() + to->height() + 10) : (fromPoint.y() + from->height() + 10) ) ;
	}
	else
		d->prev_rect = QRect(fromPoint.x()- 5,  fromPoint.y() -5, from->width() + 10, from->height() + 10);

	if (!unclipped)
		clearWFlags( WPaintUnclipped );
	p.end();
}

QSize
KexiDBForm::sizeHint() const
{
	//todo: find better size (user configured?)
	return QSize(400,300);
}

void KexiDBForm::setInvalidState( const QString& displayText )
{
	Q_UNUSED( displayText );

	//! @todo draw "invalid data source" text on the surface?
}

bool KexiDBForm::autoTabStops() const
{
	return d->autoTabStops;
}

void KexiDBForm::setAutoTabStops(bool set)
{
	d->autoTabStops = set;
}

QPtrList<QWidget>* KexiDBForm::orderedFocusWidgets() const
{
	return &d->orderedFocusWidgets;
}

QPtrList<QWidget>* KexiDBForm::orderedDataAwareWidgets() const
{
	return &d->orderedDataAwareWidgets;
}

void KexiDBForm::updateTabStopsOrder(KFormDesigner::Form* form)
{
	QWidget *fromWidget = 0;
	//QWidget *topLevelWidget = form->widget()->topLevelWidget();
//js	form->updateTabStopsOrder(); //certain widgets can have now updated focusPolicy properties, fix this
	uint numberOfDataAwareWidgets = 0;
//	if (d->orderedFocusWidgets.isEmpty()) {
		//generate a new list
		for (KFormDesigner::ObjectTreeListIterator it(form->tabStopsIterator()); it.current(); ++it) {
			if (it.current()->widget()->focusPolicy() & QWidget::TabFocus) {
				//this widget has tab focus:
				it.current()->widget()->installEventFilter(this);
				//also filter events for data-aware children of this widget (i.e. KexiDBAutoField's editors)
				QObjectList *children = it.current()->widget()->queryList("QWidget");
				for (QObjectListIt childrenIt(*children); childrenIt.current(); ++childrenIt) {
	//				if (dynamic_cast<KexiFormDataItemInterface*>(childrenIt.current())) {
						kexipluginsdbg << "KexiDBForm::updateTabStopsOrder(): also adding '" << childrenIt.current()->className() << " " << childrenIt.current()->name()  << "' child to filtered widgets" << endl;
						//it.current()->widget()->installEventFilter(static_cast<QWidget*>(childrenIt.current()));
						childrenIt.current()->installEventFilter(this);
		//			}
				}
				delete children;
				if (fromWidget) {
					kexipluginsdbg << "KexiDBForm::updateTabStopsOrder() tab order: " << fromWidget->name() 
						<< " -> " << it.current()->widget()->name() << endl;
	//				setTabOrder( fromWidget, it.current()->widget() );
				}
				fromWidget = it.current()->widget();
				d->orderedFocusWidgets.append( it.current()->widget() );
			}

			KexiFormDataItemInterface* dataItem = dynamic_cast<KexiFormDataItemInterface*>( it.current()->widget() );
			if (dataItem && !dataItem->dataSource().isEmpty()) {
				kexipluginsdbg << "#" << numberOfDataAwareWidgets << ": " 
					<< dataItem->dataSource() << " (" << it.current()->widget()->name() << ")" << endl;

//	/*! @todo d->indicesForDataAwareWidgets SHOULDNT BE UPDATED HERE BECAUSE
//	THERE CAN BE ALSO NON-TABSTOP DATA WIDGETS!
//	*/
				d->indicesForDataAwareWidgets.replace( 
					dataItem, 
					numberOfDataAwareWidgets );
				numberOfDataAwareWidgets++;

				d->orderedDataAwareWidgets.append( it.current()->widget() );
			}
		}//for
//	}
/*	else {
		//restore ordering
		for (QPtrListIterator<QWidget> it(d->orderedFocusWidgets); it.current(); ++it) {
			if (fromWidget) {
				kdDebug() << "KexiDBForm::updateTabStopsOrder() tab order: " << fromWidget->name() 
					<< " -> " << it.current()->name() << endl;
				setTabOrder( fromWidget, it.current() );
			}
			fromWidget = it.current();
		}
//		SET_FOCUS_USING_REASON(focusWidget(), QFocusEvent::Tab);
	}*/
}

void KexiDBForm::updateTabStopsOrder()
{
	for (QPtrListIterator<QWidget> it( d->orderedFocusWidgets ); it.current();) {
		if (! (it.current()->focusPolicy() & QWidget::TabFocus))
			d->orderedFocusWidgets.remove( it.current() );
		else
			++it;
	}
}

void KexiDBForm::updateReadOnlyFlags()
{
	for (QPtrListIterator<QWidget> it(d->orderedDataAwareWidgets); it.current(); ++it) {
		KexiFormDataItemInterface* dataItem = dynamic_cast<KexiFormDataItemInterface*>( it.current() );
		if (dataItem && !dataItem->dataSource().isEmpty()) {
			if (dataAwareObject()->isReadOnly()) {
				dataItem->setReadOnly( true );
			}
		}
	}
}

bool KexiDBForm::eventFilter( QObject * watched, QEvent * e )
{
	//kexipluginsdbg << e->type() << endl;
	if (e->type()==QEvent::Resize && watched == this)
		kexipluginsdbg << "RESIZE" << endl;
	if (e->type()==QEvent::KeyPress) {
		if (preview()) {
			QKeyEvent *ke = static_cast<QKeyEvent*>(e);
			// allow the editor widget to grab the key press event
			QObject *o = watched; //focusWidget();
			QWidget* realWidget = 0;
			while (true) {
				if (!o || o == dynamic_cast<QObject*>(d->dataAwareObject))
					break;
				if (dynamic_cast<KexiFormDataItemInterface*>(o)) {
					realWidget = dynamic_cast<QWidget*>(o); //will beused below (for tab/backtab handling)
					if (realWidget == this) //we have ancountered 'this' form surface, give up
						return false;
					if (dynamic_cast<KexiFormDataItemInterface*>(o)->keyPressed(ke))
						return false;
					break;
				}
				o = o->parent();
			}
			// handle Esc key
			if (ke->state() == Qt::NoButton && ke->key() == Qt::Key_Escape) {
				//cancel field editing/row editing
				if (d->dataAwareObject->editor()) {
					d->dataAwareObject->cancelEditor();
				}
				else if (d->dataAwareObject->rowEditing()) {
					d->dataAwareObject->cancelRowEdit();
				}
				ke->accept();
				return true;
			}
			// jstaniek: Fix for Qt bug (handling e.g. Alt+2, Ctrl+2 keys on every platform)
			//           It's important because we're using alt+2 short cut by default
			//           Damn! I've reported this to Trolltech in November 2004 - still not fixed.
			if (ke->isAccepted() && (ke->state() & Qt::AltButton) && ke->text()>="0" && ke->text()<="9")
				return true;
			const bool tab = ke->state() == Qt::NoButton && ke->key() == Qt::Key_Tab;
			const bool backtab = ((ke->state() == Qt::NoButton || ke->state() == Qt::ShiftButton) && ke->key() == Qt::Key_Backtab)
				|| (ke->state() == Qt::ShiftButton && ke->key() == Qt::Key_Tab);

			if (tab || backtab) {
				//the watched widget can be a subwidget of a real widget, e.g. autofield: find it
				//QWidget* realWidget = static_cast<QWidget*>(watched);
				while (dynamic_cast<KexiDataItemInterface*>(realWidget) && dynamic_cast<KexiDataItemInterface*>(realWidget)->parentInterface())
					realWidget = dynamic_cast<QWidget*>( dynamic_cast<KexiDataItemInterface*>(realWidget)->parentInterface() );

				if (d->orderedFocusWidgetsIterator.current() != realWidget) {
					d->orderedFocusWidgetsIterator.toFirst();
					while (d->orderedFocusWidgetsIterator.current() && d->orderedFocusWidgetsIterator.current()!=realWidget) {
						//QWidget *ww = d->orderedFocusWidgetsIterator.current();
						++d->orderedFocusWidgetsIterator;
					}
				}
				kexipluginsdbg << realWidget->name() << endl;
				if (tab) {
					if (d->orderedFocusWidgets.first() && realWidget == d->orderedFocusWidgets.last()) {
						d->orderedFocusWidgetsIterator.toFirst();
					}
					else if (realWidget == d->orderedFocusWidgetsIterator.current()) {
/*	QEvent fe( QEvent::FocusOut );
	QFocusEvent::setReason(QFocusEvent::Tab);
	QApplication::sendEvent( d->orderedFocusWidgetsIterator.current(), &fe );
	QFocusEvent::resetReason();*/
						++d->orderedFocusWidgetsIterator; //next
					}
					else
						return true; //ignore
					//set focus, but don't use just setFocus() because certain widgets
					//behaves differently (e.g. QLineEdit calls selectAll()) when 
					//focus event's reason is QFocusEvent::Tab
					QWidget *widgetToFocus = d->orderedFocusWidgetsIterator.current();
					widgetToFocus = widgetToFocus->focusProxy();
					if (d->dataAwareObject->acceptEditor()) {//try to accept this will validate the current 
						                                       //input (if any)
						UNSET_FOCUS_USING_REASON(realWidget, QFocusEvent::Tab);
						SET_FOCUS_USING_REASON(widgetToFocus, QFocusEvent::Tab);
						kexipluginsdbg << "focusing " << widgetToFocus->name() << endl;
					}
					return true;
				} else if (backtab) {
					if (d->orderedFocusWidgets.last() && realWidget == d->orderedFocusWidgets.first()) {
						d->orderedFocusWidgetsIterator.toLast();
					}
					else if (realWidget == d->orderedFocusWidgetsIterator.current()) {
						--d->orderedFocusWidgetsIterator; //prev
					}
					else
						return true; //ignore
					if (d->dataAwareObject->acceptEditor()) {//try to accept this will validate the current 
						                                       //input (if any)
						UNSET_FOCUS_USING_REASON(realWidget, QFocusEvent::Backtab);
						//set focus, see above note
						SET_FOCUS_USING_REASON(d->orderedFocusWidgetsIterator.current(), QFocusEvent::Backtab);
						kexipluginsdbg << "focusing " << d->orderedFocusWidgetsIterator.current()->name() << endl;
					}
					return true;
				}
			}
		}
	}
	else if (e->type()==QEvent::FocusIn) {
		bool focusDataWidget = preview();
		if (static_cast<QFocusEvent*>(e)->reason()==QFocusEvent::Popup) {
			kdDebug() << "->>> focus IN, popup" <<endl;
			focusDataWidget = !d->popupFocused;
			d->popupFocused = false;
//			if (d->widgetFocusedBeforePopup) {
//				watched = d->widgetFocusedBeforePopup;
//				d->widgetFocusedBeforePopup = 0;
//			}
		}

		if (focusDataWidget) {
			kexipluginsdbg << "KexiDBForm: FocusIn: " << watched->className() << " " << watched->name() << endl;
			if (d->dataAwareObject) {
				QWidget *dataItem = dynamic_cast<QWidget*>(watched);
				while (dataItem) {
					while (dataItem && !dynamic_cast<KexiDataItemInterface*>(dataItem))
						dataItem = dataItem->parentWidget();
					if (!dataItem)
						break;
					kexipluginsdbg << "KexiDBForm: FocusIn: FOUND " << dataItem->className() << " " << dataItem->name() << endl;
					QMapConstIterator<KexiDataItemInterface*, uint> indicesForDataAwareWidgetsIt(
						d->indicesForDataAwareWidgets.find(dynamic_cast<KexiDataItemInterface*>(dataItem)));
					if (indicesForDataAwareWidgetsIt!=d->indicesForDataAwareWidgets.constEnd()) {
						const uint index = indicesForDataAwareWidgetsIt.data();
						kexipluginsdbg << "KexiDBForm: moving cursor to column #" << index << endl;
						editedItem = 0;
						if ((int)index!=d->dataAwareObject->currentColumn()) {
							d->dataAwareObject->setCursorPosition( d->dataAwareObject->currentRow(), index /*column*/ );
						}
						break;
					}
					else
						dataItem = dataItem->parentWidget();
					dataItem->update();
				}
			}
		}
	}
	else if (e->type()==QEvent::FocusOut) {
		if (static_cast<QFocusEvent*>(e)->reason()==QFocusEvent::Popup) {
			//d->widgetFocusedBeforePopup = (QWidget*)watched;
			d->popupFocused = true;
		}
		else
			d->popupFocused = false;
//			d->widgetFocusedBeforePopup = 0;
//		kdDebug() << "e->type()==QEvent::FocusOut " << watched->className() << " " <<watched->name() << endl;
//		UNSET_FOCUS_USING_REASON(watched, static_cast<QFocusEvent*>(e)->reason());
	}
	return KexiDBFormBase::eventFilter(watched, e);
}

bool KexiDBForm::valueIsNull()
{
	return true;
}

bool KexiDBForm::valueIsEmpty()
{
	return true;
}

bool KexiDBForm::isReadOnly() const
{
	if (d->dataAwareObject)
		return d->dataAwareObject->isReadOnly();
//! @todo ?
	return false;
}

void KexiDBForm::setReadOnly( bool readOnly )
{
	if (d->dataAwareObject)
		d->dataAwareObject->setReadOnly( readOnly ); //???
}

QWidget* KexiDBForm::widget()
{
	return this;
}

bool KexiDBForm::cursorAtStart()
{
	return false;
}

bool KexiDBForm::cursorAtEnd()
{
	return false;
}

void KexiDBForm::clear()
{
	//! @todo clear all fields?
}

bool KexiDBForm::preview() const {
	return dynamic_cast<KexiScrollView*>(d->dataAwareObject)
		? dynamic_cast<KexiScrollView*>(d->dataAwareObject)->preview() : false;
}

int KexiDBForm::indexForDataItem( KexiDataItemInterface* item ) const
{
	if (!item)
		return -1;
	QMapConstIterator<KexiDataItemInterface*, uint> it = d->indicesForDataAwareWidgets.find( item );
	if (it==d->indicesForDataAwareWidgets.constEnd())
		return -1;
	return it.data();
}

void KexiDBForm::dragMoveEvent( QDragMoveEvent *e )
{
	emit handleDragMoveEvent(e);
}

void KexiDBForm::dropEvent( QDropEvent *e ) 
{
	emit handleDropEvent(e);
}

void KexiDBForm::setCursor( const QCursor & cursor )
{
	//js: empty, to avoid fscking problems with random cursors!
	//! @todo?

	if (KFormDesigner::FormManager::self()->isInserting()) //exception
		KexiDBFormBase::setCursor(cursor);
}

//! @todo: Qt4? XORed resize rectangles instead of black widgets
/*
void KexiDBForm::paintEvent( QPaintEvent *e )
{
	QPainter p;
	p.begin(this, true);
	bool unclipped = testWFlags( WPaintUnclipped );
	setWFlags( WPaintUnclipped );

	p.setPen(white);
	p.setRasterOp(XorROP);
	p.drawLine(e->rect().topLeft(), e->rect().bottomRight());

	if (!unclipped)
		clearWFlags( WPaintUnclipped );
	p.end();
	KexiDBFormBase::paintEvent(e);
}
*/

#include "kexidbform.moc"
