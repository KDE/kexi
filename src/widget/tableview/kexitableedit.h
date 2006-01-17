/* This file is part of the KDE project
   Copyright (C) 2002 Peter Simonsson <psn@linux.se>
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
 */

#ifndef _KEXITABLEEDIT_H_
#define _KEXITABLEEDIT_H_

#include <kexidataiteminterface.h>

#include <qvariant.h>
#include <qscrollview.h>

#include "kexitableviewdata.h"

namespace KexiDB {
	class Field;
	class QueryColumnInfo;
}

/*! @short Abstract class for a cell editor.
*/
class KEXIDATATABLE_EXPORT KexiTableEdit : public QWidget, public KexiDataItemInterface
{
	Q_OBJECT

	public:
		KexiTableEdit(KexiTableViewColumn &column, QScrollView* parent = 0, const char* name = 0);

		virtual ~KexiTableEdit();

		//! Implemented for KexiDataItemInterface.
		//! \return field information for this item
		virtual KexiDB::Field *field() const { return m_column->field(); }

		/*! A rich field information for db-aware data. 
		 For not-db-aware data it is always 0 (use field() instead. */
		virtual KexiDB::QueryColumnInfo *columnInfo() const { return m_column->fieldinfo; }

		//! Implemented for KexiDataItemInterface.
		//! Does nothing because instead KexiTableViewColumn is used to get field's schema.
		virtual void setColumnInfo(KexiDB::QueryColumnInfo *) { }

		//! \return column information for this item 
		//! (extended information, comparing to field()).
		inline KexiTableViewColumn *column() const { return m_column; }

		/*! Reimplemented: resizes a view(). */
		virtual void resize(int w, int h);

		/*! \return the view widget of this editor, e.g. line edit widget. */
		virtual QWidget* widget() { return m_view; }

		/*! Hides item's widget, if available. */
		inline virtual void hideWidget() { hide(); }

		/*! Shows item's widget, if available. */
		inline virtual void showWidget() { show(); }

		/*! Paints a border for the cell described by \a x, \a y, \a w, \a h on \a p painter.
		 The cell's value is \a val (may be usefull if you want to reimplement this method).
		*/
		virtual void paintFocusBorders( QPainter *p, QVariant &cal, int x, int y, int w, int h );

		/*! For reimplementation.
		 Sets up anmd paints cell's contents using context of \a val value. 
		 \a focused is true if the cell is focused. \a align is set using Qt::AlignmentFlags.
		 Some additional things may be painted using \a p,
		 it's not needed to paint the text (this is done automatically outside.

		 Before calling, \a x, \a y_offset, \a w, \a h parameters are initialized,
		 but you can tune these values depending on the context. 
		 You should set \a txt to a text representation of \a val, 
		 otherwise no text will be painted. */
		virtual void setupContents( QPainter *p, bool focused, QVariant val, 
			QString &txt, int &align, int &x, int &y_offset, int &w, int &h );

		/*! For reimplementation.
		 Paints selection's background using \a p. Most parameters are similar to these from 
		 setupContents().
		*/
		virtual void paintSelectionBackground( QPainter *p, bool focused, const QString& txt, 
			int align, int x, int y_offset, int w, int h, const QColor& fillColor,
			bool readOnly, bool fullRowSelection );

		/*! Sometimes, editor can contain non-standard margin, for example combobox editor contains
		 dropdown button at the right side. \return left margin's size; 
		 0 by default. For reimplementation.  */
		int leftMargin() const { return m_leftMargin; }

		/*! Sometimes, editor can contain non-standard margin, for example combobox editor contains
		 dropdown button at the right side. \return right margin;s size; 
		 0 by default. For reimplementation.  */
		int rightMargin() const { return m_rightMargin; }

		/*! Handles \a ke key event that came over the column that is bound to this editor.
		 For implementation: true should be returned if \a ke should be accepted.
		 If \a editorActive is true, this editor is currently active, i.e. the table view is in edit mode.
		 By default false is returned. */
		virtual bool handleKeyPress( QKeyEvent * /*ke*/, bool /*editorActive*/ ) { return false; }

		/*! \return width of \a value. For the default implementation \a val is converted to a string 
		 and width of this string is returned. */
		virtual int widthForValue( QVariant &val, QFontMetrics &fm );

		/*! \return total size of this editor, including any buttons, etc. (if present). 
		 Reimpelment this if you want to return more appropriate size. This impelmentation just
		 returns QWidget::size(). */
		virtual QSize totalSize() { return QWidget::size(); }

	signals:
		void editRequested();
		void cancelRequested();
		void acceptRequested();

	protected:
		virtual bool eventFilter(QObject* watched, QEvent* e);

		/*! Sets \a v as view widget for this editor. The view will be assigned as focus proxy
		 for the editor, its events will be filtered, it will be resized when neede, and so on. */
		void setViewWidget(QWidget *v);

		/*! Moves child widget within the viewport. Use this for child widgets that 
		 are outside of this editor widget, instead of calling QWidget::move(). */
		void moveChild( QWidget * child, int x, int y ) {
			m_scrollView->moveChild(child, x, y); }

		KexiTableViewColumn *m_column;
		int m_leftMargin;
		int m_rightMargin;
		QScrollView* m_scrollView;

	private:
		QWidget* m_view;
};

//! Declaration of cell editor factory
#define KEXI_DECLARE_CELLEDITOR_FACTORY_ITEM(factoryclassname) \
	class factoryclassname : public KexiCellEditorFactoryItem \
	{ \
		public: \
			factoryclassname(); \
			virtual ~factoryclassname(); \
	\
		protected: \
			virtual KexiTableEdit* createEditor(KexiTableViewColumn &column, QScrollView* parent = 0); \
	};

//! Implementation of cell editor factory
#define KEXI_CELLEDITOR_FACTORY_ITEM_IMPL(factoryclassname, itemclassname) \
factoryclassname::factoryclassname() \
 : KexiCellEditorFactoryItem() \
{} \
\
factoryclassname::~factoryclassname() \
{} \
\
KexiTableEdit* factoryclassname::createEditor( \
	KexiTableViewColumn &column, QScrollView* parent) \
{ \
	return new itemclassname(column, parent); \
}

#endif
