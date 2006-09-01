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

#ifndef KEXIFLOWLAYOUT_H
#define KEXIFLOWLAYOUT_H

#include <qlayout.h>
#include <qptrlist.h>

//! @short a special "flow" layout
class KEXIGUIUTILS_EXPORT KexiFlowLayout : public QLayout
{
	public:
		KexiFlowLayout(QWidget *parent, int border=0, int space=-1, const char *name=0);
		KexiFlowLayout(QLayout* parent, int space=-1, const char *name=0);
		KexiFlowLayout(int space=-1, const char *name=0);

		~KexiFlowLayout();

		/*! \return the widgets in the order of the layout,
		 ie as it is stored in m_list. You must delete the list after using it. */
		QPtrList<QWidget>* widgetList() const;

		/*! Sets layout's orientation to \a orientation. Default orientation is Vertical. */
		void  setOrientation(Orientation orientation) { m_orientation = orientation; }

		/*! \return layout's orientation. */
		Qt::Orientation orientation() const { return m_orientation; }

		void setJustified(bool justify) { m_justify = justify; }
		bool isJustified() const { return m_justify; }

		virtual void addItem(QLayoutItem *item);
		virtual void addSpacing(int size);
		virtual QLayoutIterator iterator();
		virtual void  invalidate();

		virtual bool hasHeightForWidth() const;
		virtual int heightForWidth(int width) const;
		virtual QSize sizeHint() const;
		virtual QSize minimumSize() const;
		virtual QSizePolicy::ExpandData expanding() const;

		virtual bool isEmpty();

	protected:
		virtual void setGeometry(const QRect&);
		int simulateLayout(const QRect &r);
		int doHorizontalLayout(const QRect&, bool testonly = false);
		int doVerticalLayout(const QRect&, bool testonly = false);

	private:
		QPtrList<QLayoutItem> m_list;
		int m_cached_width;
		int m_cached_hfw;
		bool m_justify;
		Orientation m_orientation;
		QSize m_cached_sizeHint;
		QSize m_cached_minSize;
};

#endif

