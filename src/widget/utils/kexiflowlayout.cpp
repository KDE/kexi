/* This file is part of the KDE project
   Copyright (C) 2005 Cedric Pasteur <cedric.pasteur@free.fr>

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

#include "kexiflowlayout.h"

#include <KDebug>

#ifdef __GNUC__
#warning KexiFlowLayout ported to Qt4 but not tested
#else
#pragma WARNING( KexiFlowLayout ported to Qt4 but not tested )
#endif

/* 2.0 removed
/// Iterator class

class KexiFlowLayoutIterator : public QGLayoutIterator
{
	public:
		KexiFlowLayoutIterator( Q3PtrList<QLayoutItem> *list )
		  : m_idx(0), m_list( list )
		{}
		uint count() const;
		QLayoutItem *current();
		QLayoutItem *next();
		QLayoutItem *takeCurrent();

	private:
		int m_idx;
		Q3PtrList<QLayoutItem> *m_list;
};

uint
KexiFlowLayoutIterator::count() const
{
	return m_list->count();
}

QLayoutItem *
KexiFlowLayoutIterator::current()
{
	return (m_idx < (int)count()) ? m_list->at(m_idx) : 0;
}

QLayoutItem *
KexiFlowLayoutIterator::next()
{
	m_idx++;
	return current();
}

QLayoutItem *
KexiFlowLayoutIterator::takeCurrent()
{
	return (m_idx < (int)count()) ? m_list->take(m_idx) : 0;
}
*/

//// The layout itself

KexiFlowLayout::KexiFlowLayout(QWidget *parent, int margin, int spacing)
 : QLayout(parent)
{
	setMargin(margin);
	setSpacing(spacing);
	m_orientation = Qt::Horizontal;
	m_justify = false;
	m_cached_width = 0;
}

KexiFlowLayout::KexiFlowLayout(QLayout* parent, int spacing)
 : QLayout()
{
	parent->addItem(this);
	setSpacing(spacing);
	m_orientation = Qt::Horizontal;
	m_justify = false;
	m_cached_width = 0;
}

KexiFlowLayout::KexiFlowLayout(int spacing)
 : QLayout()
 {
	setSpacing(spacing);
	m_orientation = Qt::Horizontal;
	m_justify = false;
	m_cached_width = 0;
 }

KexiFlowLayout::~KexiFlowLayout()
{
	qDeleteAll(m_list);
}

void
KexiFlowLayout::addItem(QLayoutItem *item)
{
	m_list.append(item);
}

void KexiFlowLayout::addSpacing(int size)
{
	if (m_orientation == Qt::Horizontal)
		addItem( new QSpacerItem( size, 0, QSizePolicy::Fixed, QSizePolicy::Minimum ) );
	else
		addItem( new QSpacerItem( 0, size, QSizePolicy::Minimum, QSizePolicy::Fixed ) );
}

/*2.0 removed
QLayoutIterator
KexiFlowLayout::iterator()
{
	return QLayoutIterator( new KexiFlowLayoutIterator(&m_list) );
}*/

QList<QWidget*>* KexiFlowLayout::widgetList() const
{
	QList<QWidget*> *list = new QList<QWidget*>();
	foreach (QLayoutItem* item, m_list) {
		if (item->widget())
			list->append(item->widget());
	}
	return list;
}

void KexiFlowLayout::invalidate()
{
	QLayout::invalidate();
	m_cached_sizeHint = QSize();
	m_cached_minSize = QSize();
	m_cached_width = 0;
}

int KexiFlowLayout::count() const
{
	return m_list.size();
}

bool KexiFlowLayout::isEmpty() const
{
	return m_list.isEmpty();
}

bool KexiFlowLayout::hasHeightForWidth() const
{
	return (m_orientation == Qt::Horizontal);
}

int KexiFlowLayout::heightForWidth(int w) const
{
	if(m_cached_width != w) {
		// workaround to allow this method to stay 'const'
		KexiFlowLayout *mthis = (KexiFlowLayout*)this;
		int h = mthis->simulateLayout( QRect(0,0,w,0) );
		mthis->m_cached_hfw = h;
		mthis->m_cached_width = w;
		return h;
 	}
	return m_cached_hfw;
}

QSize KexiFlowLayout::sizeHint() const
{
	if(m_cached_sizeHint.isEmpty()) {
		KexiFlowLayout *mthis = (KexiFlowLayout*)this;
		QRect r = QRect(0, 0, 2000, 2000);
		mthis->simulateLayout(r);
	}
	return m_cached_sizeHint;
}

QSize KexiFlowLayout::minimumSize() const
{
//js: do we really need to simulate layout here?
//    I commented this out because it was impossible to stretch layout conveniently.
//    Now, minimum size is computed automatically based on item's minimumSize...
#if 0
	if(m_cached_minSize.isEmpty()) {
		KexiFlowLayout *mthis = (KexiFlowLayout*)this;
		QRect r = QRect(0, 0, 2000, 2000);
		mthis->simulateLayout(r);
	}
#endif
	return m_cached_minSize;
}

/*QSizePolicy::ExpandData
KexiFlowLayout::expanding() const*/
Qt::Orientations KexiFlowLayout::expandingDirections() const
{
	if(m_orientation == Qt::Vertical)
		return Qt::Vertical;
	else
		return Qt::Horizontal;
}

void KexiFlowLayout::setGeometry(const QRect &r)
{
	QLayout::setGeometry(r);
	if(m_orientation == Qt::Horizontal)
		doHorizontalLayout(r);
	else
		doVerticalLayout(r);
}

int KexiFlowLayout::simulateLayout(const QRect &r)
{
	if(m_orientation == Qt::Horizontal)
		return doHorizontalLayout(r, true);
	else
		return doVerticalLayout(r, true);
}

int KexiFlowLayout::doHorizontalLayout(const QRect &r, bool testOnly)
{
	int x = r.x();
	int y = r.y();
	int h = 0; // height of this line
	int availableSpace = r.width() + spacing();
	int expandingWidgets=0; // number of widgets in the line with QSizePolicy == Expanding
	QListIterator<QLayoutItem*> it(m_list);
	QList<QLayoutItem*> currentLine;
	QSize minSize, sizeHint(20, 20);
	int minSizeHeight = 0 - spacing();

	while ( it.hasNext() ) {
		QLayoutItem *o = it.next();
		if (o->isEmpty()) // do not consider hidden widgets
			continue;

//		kDebug() << "- doHorizontalLayout(): " << o->widget()->className() << " " << o->widget()->name() << endl;
		QSize oSizeHint = o->sizeHint(); // we cache these ones because it can take 
		                                 // a while to get it (eg for child layouts)
		if ((x + oSizeHint.width()) > r.right() && h > 0) {
			// do the layout of current line
			QListIterator<QLayoutItem*> it2(currentLine);
			QLayoutItem *item;
			int wx = r.x();
			int sizeHintWidth = 0 -spacing(), minSizeWidth=0 - spacing(), lineMinHeight=0;
			while ( it2.hasNext() ) {
				item = it2.next();
				QSize itemSizeHint = item->sizeHint(); // we cache these ones because it can take
				QSize itemMinSize = item->minimumSize(); // a while to get them
				QSize s;
				if (m_justify) {
					if (expandingWidgets != 0) {
						if (item->expandingDirections() & Qt::Horizontal)
							s = QSize(
								qMin(itemSizeHint.width() + availableSpace/expandingWidgets, r.width()),
								itemSizeHint.height()
							);
						else
							s = QSize( qMin(itemSizeHint.width(), r.width()), itemSizeHint.height() );
					}
					else
						s = QSize(
							qMin(itemSizeHint.width() + availableSpace/(int)currentLine.count(), r.width()),
							itemSizeHint.height()
						);
				}
				else
					s = QSize ( qMin(itemSizeHint.width(), r.width()), itemSizeHint.height() );
				if (!testOnly)
					item->setGeometry( QRect(QPoint(wx, y), s) );
				wx = wx + s.width() + spacing();
				minSizeWidth = minSizeWidth + spacing() + itemMinSize.width();
				sizeHintWidth = sizeHintWidth + spacing() +  itemSizeHint.width();
				lineMinHeight = qMax( lineMinHeight, itemMinSize.height() );
			}
			sizeHint = sizeHint.expandedTo( QSize(sizeHintWidth, 0) );
			minSize = minSize.expandedTo( QSize(minSizeWidth, 0) );
			minSizeHeight = minSizeHeight + spacing() + lineMinHeight;
			// start a new line
			y = y + spacing() + h;
			h = 0;
			x = r.x();
			currentLine.clear();
			expandingWidgets = 0;
			availableSpace = r.width() + spacing();
		}

		x = x + spacing() + oSizeHint.width();
		h = qMax( h,  oSizeHint.height() );
		currentLine.append(o);
		if (o->expandingDirections() & Qt::Horizontal)
			++expandingWidgets;
		availableSpace = qMax(0, availableSpace - spacing() - oSizeHint.width());
	}

	// don't forget to layout the last line
	QListIterator<QLayoutItem*> it2(currentLine);
	int wx = r.x();
	int sizeHintWidth = 0 - spacing(), minSizeWidth=0 - spacing(), lineMinHeight=0;
	while ( it2.hasNext() ) {
		QLayoutItem *item = it2.next();
		QSize itemSizeHint = item->sizeHint(); // we cache these ones because it can take
		QSize itemMinSize = item->minimumSize(); // a while to get them
		QSize s;
		if (m_justify) {
			if (expandingWidgets != 0) {
				if (item->expandingDirections() & Qt::Horizontal)
					s = QSize(
						qMin(itemSizeHint.width() + availableSpace/expandingWidgets, r.width()),
						itemSizeHint.height()
					);
				else
					s = QSize( qMin(itemSizeHint.width(), r.width()), itemSizeHint.height() );
			}
			else
				s = QSize(
					qMin(itemSizeHint.width() + availableSpace/(int)currentLine.count(), r.width()),
					itemSizeHint.height()
				);
		}
		else
			s = QSize ( qMin(itemSizeHint.width(), r.width()), itemSizeHint.height() );
		if (!testOnly)
			item->setGeometry( QRect(QPoint(wx, y), s) );
		wx = wx + s.width() + spacing();
		minSizeWidth = minSizeWidth + spacing() + itemMinSize.width();
		sizeHintWidth = sizeHintWidth + spacing() +  itemSizeHint.width();
		lineMinHeight = qMax( lineMinHeight, itemMinSize.height() );
	}
	sizeHint = sizeHint.expandedTo( QSize(sizeHintWidth, y + spacing() + h) );
	minSizeHeight = minSizeHeight + spacing() + lineMinHeight;
	minSize = minSize.expandedTo( QSize(minSizeWidth, minSizeHeight) );

	// store sizeHint() and minimumSize()
	m_cached_sizeHint = sizeHint + QSize(2* margin(), 2*margin());
	m_cached_minSize = minSize + QSize(2* margin() , 2*margin());
	// return our height
	return y + h - r.y();
}

int KexiFlowLayout::doVerticalLayout(const QRect &r, bool testOnly)
{
	int x = r.x();
	int y = r.y();
	int w = 0; // width of this line
	int availableSpace = r.height() + spacing();
	int expandingWidgets=0; // number of widgets in the line with QSizePolicy == Expanding
	QListIterator<QLayoutItem*> it(m_list);
	QList<QLayoutItem*> currentLine;
	QSize minSize, sizeHint(20, 20);
	int minSizeWidth = 0 - spacing();

	while ( it.hasNext() ) {
		QLayoutItem *o = it.next();
		if (o->isEmpty()) // do not consider hidden widgets
			continue;

		QSize oSizeHint = o->sizeHint(); // we cache these ones because it can take
		                                 // a while to get it (eg for child layouts)
		if (y + oSizeHint.height() > r.bottom() && w > 0) {
			// do the layout of current line
			QListIterator<QLayoutItem*> it2(currentLine);
			int wy = r.y();
			int sizeHintHeight = 0 - spacing(), minSizeHeight = 0 - spacing(), colMinWidth=0;
			while( it2.hasNext() ) {
				QLayoutItem *item = it2.next();
				QSize itemSizeHint = item->sizeHint(); // we cache these ones because it can take
				QSize itemMinSize = item->minimumSize(); // a while to get them
				QSize s;
				if(m_justify) {
					if(expandingWidgets != 0) {
						if(item->expandingDirections() & Qt::Vertical)
							s = QSize(
								itemSizeHint.width(),
								qMin(itemSizeHint.height() + availableSpace/expandingWidgets, r.height())
							);
						else
							s = QSize( itemSizeHint.width(), qMin(itemSizeHint.height(), r.height()) );
					}
					else
						s = QSize(
							itemSizeHint.width(),
							qMin(itemSizeHint.height() + availableSpace/(int)currentLine.count(), r.height())
						);
				}
				else
					s = QSize (  itemSizeHint.width(), qMin(itemSizeHint.height(), r.height()) );
				if(!testOnly)
					item->setGeometry( QRect(QPoint(x, wy), s) );
				wy = wy + s.height() + spacing();
				minSizeHeight = minSizeHeight + spacing() + itemMinSize.height();
				sizeHintHeight = sizeHintHeight + spacing() + itemSizeHint.height();
				colMinWidth = qMax( colMinWidth, itemMinSize.width() );
			}
			sizeHint = sizeHint.expandedTo( QSize(0, sizeHintHeight) );
			minSize = minSize.expandedTo( QSize(0, minSizeHeight) );
			minSizeWidth = minSizeWidth + spacing() + colMinWidth;
			// start a new column
			x = x + spacing() + w;
			w = 0;
			y = r.y();
			currentLine.clear();
			expandingWidgets = 0;
			availableSpace = r.height() + spacing();
		}

		y = y + spacing() + oSizeHint.height();
		w = qMax( w,  oSizeHint.width() );
		currentLine.append(o);
		if(o->expandingDirections() & Qt::Vertical)
			++expandingWidgets;
		availableSpace = qMax(0, availableSpace - spacing() - oSizeHint.height());
	}

	// don't forget to layout the last line
	QListIterator<QLayoutItem*> it2(currentLine);
	int wy = r.y();
	int sizeHintHeight = 0 - spacing(), minSizeHeight = 0 - spacing(), colMinWidth=0;
	while( it2.hasNext() ) {
		QLayoutItem *item = it2.next();
		QSize itemSizeHint = item->sizeHint(); // we cache these ones because it can take
		QSize itemMinSize = item->minimumSize(); // a while to get them
		QSize s;
		if(m_justify) {
			if(expandingWidgets != 0) {
				if(item->expandingDirections() & Qt::Vertical)
					s = QSize(
						itemSizeHint.width(),
						qMin(itemSizeHint.height() + availableSpace/expandingWidgets, r.height())
					);
				else
					s = QSize( itemSizeHint.width(), qMin(itemSizeHint.height(), r.height()) );
			}
			else
				s = QSize(
					itemSizeHint.width(),
					qMin(itemSizeHint.height() + availableSpace/(int)currentLine.count(), r.height())
				);
		}
		else
			s = QSize ( itemSizeHint.width(), qMin(itemSizeHint.height(), r.height()) );
		if(!testOnly)
			item->setGeometry( QRect(QPoint(x, wy), s) );
		wy = wy + s.height() + spacing();
		minSizeHeight = minSizeHeight + spacing() + itemMinSize.height();
		sizeHintHeight = sizeHintHeight + spacing() + itemSizeHint.height();
		colMinWidth = qMax( colMinWidth, itemMinSize.width() );
	}
	sizeHint = sizeHint.expandedTo( QSize( x + spacing() + w, sizeHintHeight) );
	minSizeWidth = minSizeWidth + spacing() + colMinWidth;
	minSize = minSize.expandedTo( QSize(minSizeWidth, minSizeHeight) );

	// store sizeHint() and minimumSize()
	m_cached_sizeHint = sizeHint + QSize(2* margin(), 2*margin());
	m_cached_minSize = minSize + QSize(2* margin(), 2*margin());
	// return our width
	return x + w - r.x();
}

QLayoutItem *KexiFlowLayout::itemAt(int index) const
{
	return m_list.value(index);
}

QLayoutItem *KexiFlowLayout::takeAt(int index)
{
	if (index >= 0 && index < m_list.size())
			return m_list.takeAt(index);
			
	return 0;
}
