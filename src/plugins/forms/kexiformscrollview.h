/* This file is part of the KDE project
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2004-2005 Jaroslaw Staniek <js@iidea.pl>

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

#ifndef KEXIFORMSCROLLVIEW_H
#define KEXIFORMSCROLLVIEW_H

#include "kexidataprovider.h"
#include "kexiformeventhandler.h"
#include "widgets/kexidbform.h"
#include <widget/kexiscrollview.h>
#include <widget/utils/kexirecordnavigator.h>
#include <widget/utils/kexisharedactionclient.h>
#include <widget/tableview/kexidataawareobjectiface.h>

//! @short KexiFormScrollView class provides a widget for displaying data in a form view
/*! This class also implements:
    - record navigation handling (KexiRecordNavigatorHandler)
    - shared actions handling (KexiSharedActionClient)
    - data-aware behaviour (KexiDataAwareObjectInterface)
    - data provider bound to data-aware widgets (KexiFormDataProvider)

    @see KexiTableView
*/
class KEXIFORMUTILS_EXPORT KexiFormScrollView : 
	public KexiScrollView,
	public KexiRecordNavigatorHandler,
	public KexiSharedActionClient,
	public KexiDataAwareObjectInterface,
	public KexiFormDataProvider,
	public KexiFormEventHandler
{
	Q_OBJECT
	KEXI_DATAAWAREOBJECTINTERFACE

	public:
		KexiFormScrollView(QWidget *parent, bool preview);
		virtual ~KexiFormScrollView();

		void setForm(KFormDesigner::Form *form) { m_form = form; }

		/*! Reimplemented from KexiDataAwareObjectInterface
		 for checking 'readOnly' flag from a widget
		 ('readOnly' flag from data member is still checked though). */
		virtual bool columnEditable(int col);

		/*! \return number of visible columns in this view. 
		 There can be a number of duplicated columns defined
		 (data-aware widgets, see KexiFormScrollView::columns()),
		 so columns() can return greater number than dataColumns(). */
		virtual int columns() const;

		/*! \return column information for column number \a col. 
		 Reimplemented for KexiDataAwareObjectInterface:
		 column data corresponding to widget number is used here
		 (see fieldNumberForColumn()). */
		virtual KexiTableViewColumn* column(int col);

		/*! \return field number within data model connected to a data-aware
		 widget at column \a col. */
		virtual int fieldNumberForColumn(int col) {
			KexiFormDataItemInterface *item = dynamic_cast<KexiFormDataItemInterface*>(
				dbFormWidget()->orderedDataAwareWidgets()->at( col ));
			if (!item)
				return -1;
			KexiFormDataItemInterfaceToIntMap::ConstIterator it(m_fieldNumbersForDataItems.find( item ));
			return it!=m_fieldNumbersForDataItems.constEnd() ? (int)it.data() : -1;
		}

		/*! @internal Used by KexiFormView in view switching. */
		void beforeSwitchView();

	public slots:
		/*! Reimplemented to update resize policy. */
		virtual void show();

		//virtual void setFocus();

		//! Implementation for KexiDataAwareObjectInterface
		//! \return arbitraty value of 10.
		virtual int rowsPerPage() const;

		//! Implementation for KexiDataAwareObjectInterface
		virtual void ensureCellVisible(int row, int col/*=-1*/);

		virtual void moveToRecordRequested(uint r);
		virtual void moveToLastRecordRequested();
		virtual void moveToPreviousRecordRequested();
		virtual void moveToNextRecordRequested();
		virtual void moveToFirstRecordRequested();
		virtual void addNewRecordRequested() { KexiDataAwareObjectInterface::addNewRecordRequested(); }

		/*! Reverts current editor's value to old one. */
		virtual void cancelEditor();

	public slots:
		/*! Reimplemented to also clear command history right after final resize. */
		virtual void refreshContentsSize();

	signals:
		virtual void itemChanged(KexiTableItem *, int row, int col);
		virtual void itemChanged(KexiTableItem *, int row, int col, QVariant oldValue);
		virtual void itemDeleteRequest(KexiTableItem *, int row, int col);
		virtual void currentItemDeleteRequest();
		virtual void dataRefreshed();
		virtual void dataSet( KexiTableViewData *data );
		virtual void itemSelected(KexiTableItem *);
		virtual void cellSelected(int col, int row);
		virtual void sortedColumnChanged(int col);
		virtual void rowEditStarted(int row);
		virtual void rowEditTerminated(int row);
		virtual void reloadActions();

	protected slots:
		void slotResizingStarted();

		//! Handles KexiTableViewData::rowRepaintRequested() signal
		virtual void slotRowRepaintRequested(KexiTableItem& item);

		//! Handles KexiTableViewData::aboutToDeleteRow() signal. Prepares info for slotRowDeleted().
		virtual void slotAboutToDeleteRow(KexiTableItem& item, KexiDB::ResultInfo* result, bool repaint)
		{ KexiDataAwareObjectInterface::slotAboutToDeleteRow(item, result, repaint); }

		//! Handles KexiTableViewData::rowDeleted() signal to repaint when needed.
		virtual void slotRowDeleted() { KexiDataAwareObjectInterface::slotRowDeleted(); }

		//! Handles KexiTableViewData::rowInserted() signal to repaint when needed.
		virtual void slotRowInserted(KexiTableItem *item, bool repaint);

		//! Like above, not db-aware version
		virtual void slotRowInserted(KexiTableItem *item, uint row, bool repaint);

		virtual void slotRowsDeleted( const QValueList<int> & );

		virtual void slotDataDestroying() { KexiDataAwareObjectInterface::slotDataDestroying(); }

		/*! Reloads data for this widget.
		 Handles KexiTableViewData::reloadRequested() signal. */
		virtual void reloadData() { KexiDataAwareObjectInterface::reloadData(); }

	protected:
		//! Implementation for KexiDataAwareObjectInterface
		virtual void clearColumnsInternal(bool repaint);

		//! Implementation for KexiDataAwareObjectInterface
		virtual void addHeaderColumn(const QString& caption, const QString& description, int width);

		//! Implementation for KexiDataAwareObjectInterface
		virtual int currentLocalSortingOrder() const;

		//! Implementation for KexiDataAwareObjectInterface
		virtual int currentLocalSortColumn() const;

		//! Implementation for KexiDataAwareObjectInterface
		virtual void setLocalSortingOrder(int col, int order);

		//! Implementation for KexiDataAwareObjectInterface
		void sortColumnInternal(int col, int order = 0);

		//! Implementation for KexiDataAwareObjectInterface
		virtual void updateGUIAfterSorting();

		//! Implementation for KexiDataAwareObjectInterface
		virtual void createEditor(int row, int col, const QString& addText = QString::null, 
			bool removeOld = false);

		//! Implementation for KexiDataAwareObjectInterface
		virtual KexiDataItemInterface *editor( int col, bool ignoreMissingEditor = false );

		//! Implementation for KexiDataAwareObjectInterface
		virtual void editorShowFocus( int row, int col );

		/*! Implementation for KexiDataAwareObjectInterface
		 Redraws specified cell. */
		virtual void updateCell(int row, int col);

		/*! Implementation for KexiDataAwareObjectInterface
		 Redraws all cells of specified row. */
		virtual void updateRow(int row);

		/*! Implementation for KexiDataAwareObjectInterface
		 Updates contents of the widget. Just call update() here on your widget. */
		virtual void updateWidgetContents();

		/*! Implementation for KexiDataAwareObjectInterface
		 Implementation for KexiDataAwareObjectInterface
		 Updates widget's contents size e.g. using QScrollView::resizeContents(). */
		virtual void updateWidgetContentsSize();

		/*! Implementation for KexiDataAwareObjectInterface
		 Updates scrollbars of the widget. 
		 QScrollView::updateScrollbars() will be usually called here. */
		virtual void updateWidgetScrollBars();

		KexiDBForm* dbFormWidget() const;

		//! Reimplemented from KexiFormDataProvider. Reaction for change of \a item.
		virtual void valueChanged(KexiDataItemInterface* item);

		/*! Reimplemented from KexiFormDataProvider. 
		 \return information whether we're currently at new row or now.
		 This can be used e.g. by data-aware widgets to determine if "(autonumber)" 
		 label should be displayed. */
		virtual bool cursorAtNewRow();

		//! Implementation for KexiDataAwareObjectInterface
		//! Called by KexiDataAwareObjectInterface::setCursorPosition() 
		//! if cursor's position is really changed.
		inline virtual void selectCellInternal();

		/*! Reimplementation: used to refresh "editing indicator" visibility. */
		virtual void initDataContents();

		/*! @internal
		 Updates row appearance after canceling row edit. 
		 Reimplemented from KexiDataAwareObjectInterface: just undoes changes for every data item.
		 Used by cancelRowEdit(). */
		virtual void updateAfterCancelRowEdit();

		/*! @internal
		 Updates row appearance after accepting row edit. 
		 Reimplemented from KexiDataAwareObjectInterface: just clears 'edit' indicator.
		 Used by cancelRowEdit(). */
		virtual void updateAfterAcceptRowEdit();

		//virtual bool focusNextPrevChild( bool next );

		KFormDesigner::Form *m_form;
		int m_currentLocalSortColumn, m_localSortingOrder;
		//! Used in selectCellInternal() to avoid fetching the same record twice
		KexiTableItem *m_previousItem;
};

#endif
