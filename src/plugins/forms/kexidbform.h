/* This file is part of the KDE project
   Copyright (C) 2004 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2005 Jaroslaw Staniek <js@iidea.pl>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#ifndef KEXIDBFORM_H
#define KEXIDBFORM_H

#include <qpixmap.h>

#include <kexigradientwidget.h>
#include <form.h>

#include "kexiformdataiteminterface.h"

class KexiDataAwareObjectInterface;
class KexiFormScrollView;

#define SET_FOCUS_USING_REASON(widget, reason) \
	{ QEvent fe( QEvent::FocusIn ); \
	QFocusEvent::setReason(reason); \
	QApplication::sendEvent( widget, &fe ); \
	QFocusEvent::resetReason(); }

//! A DB-aware form widget
class KexiDBForm : 
	public KexiGradientWidget,
	public KFormDesigner::FormWidget,
	public KexiFormDataItemInterface
{
	Q_OBJECT
	Q_PROPERTY(QString dataSource READ dataSource WRITE setDataSource DESIGNABLE true)
	Q_PROPERTY(bool autoTabStops READ autoTabStops WRITE setAutoTabStops DESIGNABLE true)

	public:
		KexiDBForm(QWidget *parent, KexiDataAwareObjectInterface* dataAwareObject, const char *name="kexi_dbform");
		virtual ~KexiDBForm();

		inline QString dataSource() const { return KexiFormDataItemInterface::dataSource(); }

		//! no effect
		QVariant value() { return QVariant(); }

		virtual void setInvalidState( const QString& displayText );

		virtual void drawRect(const QRect& r, int type);
		virtual void drawRects(const QValueList<QRect> &list, int type);
		virtual void initBuffer();
		virtual void clearForm();
		virtual void highlightWidgets(QWidget *from, QWidget *to/*, const QPoint &p*/);

		virtual QSize sizeHint() const;

		bool autoTabStops() const;

		QPtrList<QWidget>* orderedFocusWidgets() const;

		QPtrList<QWidget>* orderedDataAwareWidgets() const;
		
		int indexForDataItem( KexiDataItemInterface* item ) const;

		void updateTabStopsOrder(KFormDesigner::Form* form);

		void updateTabStopsOrder();

		virtual bool eventFilter ( QObject * watched, QEvent * e );

		virtual bool valueIsNull();
		virtual bool valueIsEmpty();
		virtual bool isReadOnly() const;
		virtual QWidget* widget();
		virtual bool cursorAtStart();
		virtual bool cursorAtEnd();
		virtual void clear();

		bool preview() const;

	public slots:
		void setAutoTabStops(bool set);
		inline void setDataSource(const QString &ds) { KexiFormDataItemInterface::setDataSource(ds); }

	protected:
		//! no effect
		virtual void setValueInternal(const QVariant&, bool) {}

		//! Points to a currently edited data item. 
		//! It is cleared when the focus is moved to other 
		KexiFormDataItemInterface *editedItem;

		class Private;
		Private *d;

		friend class KexiFormScrollView;
};

#endif
