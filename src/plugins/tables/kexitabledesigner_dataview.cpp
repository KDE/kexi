/* This file is part of the KDE project
   Copyright (C) 2004 Jaroslaw Staniek <js@iidea.pl>

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

#include "kexitabledesigner_dataview.h"

#include <kexidb/connection.h>
#include <kexidb/cursor.h>
#include <kexiutils/utils.h>
#include "kexitableview.h"
#include "kexidatatableview.h"
#include "keximainwindow.h"

KexiTableDesigner_DataView::KexiTableDesigner_DataView(KexiMainWindow *win, QWidget *parent)
 : KexiDataTable(win, parent, "KexiTableDesigner_DataView", true/*db-aware*/)
{
}

KexiTableDesigner_DataView::~KexiTableDesigner_DataView()
{
	if (dynamic_cast<KexiDataTableView*>(tableView()) 
		&& dynamic_cast<KexiDataTableView*>(tableView())->cursor())
	{
		mainWin()->project()->dbConnection()->deleteCursor( 
			dynamic_cast<KexiDataTableView*>(tableView())->cursor() );
	}
}

tristate KexiTableDesigner_DataView::beforeSwitchTo(int mode, bool &dontStore)
{
	Q_UNUSED( dontStore );

	if (mode != Kexi::DataViewMode) {
		//accept editing before switching
//		if (!m_view->acceptRowEdit()) {
		if (!acceptRowEdit()) {
			return cancelled;
		}
	}

	return true;
}

tristate KexiTableDesigner_DataView::afterSwitchFrom(int mode)
{
	Q_UNUSED( mode );

	if (tempData()->tableSchemaChangedInPreviousView) {
		KexiUtils::WaitCursor wait;
		KexiDB::Cursor *c = mainWin()->project()->dbConnection()->prepareQuery(*tempData()->table);
		if (!c)
			return false;
		setData(c);
		tempData()->tableSchemaChangedInPreviousView = false;
	}
	return true;
}

KexiTablePart::TempData* KexiTableDesigner_DataView::tempData() const
{
	return static_cast<KexiTablePart::TempData*>(parentDialog()->tempData());
}

#include "kexitabledesigner_dataview.moc"
