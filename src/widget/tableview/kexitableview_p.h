/* This file is part of the KDE project
   Copyright (C) 2002 Till Busch <till@bux.at>
   Copyright (C) 2003 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2003 Daniel Molkentin <molkentin@kde.org>
   Copyright (C) 2003 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2003-2004 Jaroslaw Staniek <js@iidea.pl>

   This program is free software; you can redistribute it and,or
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

#ifndef KEXITABLEVIEW_P_H
#define KEXITABLEVIEW_P_H

#include "kexitableview.h"
#include "kexitableheader.h"

#include <kexidb/roweditbuffer.h>
#include <widget/utils/kexidisplayutils.h>

#include <qevent.h>
#include <qtimer.h>
#include <qvalidator.h>
#include <qasciidict.h>

#include <kpushbutton.h>
#include <ktoolbarbutton.h>
#include <klineedit.h>
#include <kpopupmenu.h>
#include <kaction.h>

class KexiTableItem;
class KexiTableRM;
class KexiTableEdit;
class QLabel;
class TableViewHeader;

/*! KexiTableView internal data
 @internal */
class KexiTableViewPrivate 
{
	public:

	KexiTableViewPrivate(KexiTableView* t);
	~KexiTableViewPrivate();

	void clearVariables();

	KexiTableView *tv;

	// foreign widgets
	TableViewHeader *pTopHeader;

	//! editors: one for each column (indexed by KexiTableViewColumn)
	QPtrDict<KexiTableEdit> editors;

	int rowHeight;

	QPixmap *pBufferPm;
	QTimer *pUpdateTimer;
	int menu_id_addRecord;
	int menu_id_removeRecord;

#if 0//(js) doesn't work!
	QTimer *scrollTimer;
#endif
	
	KexiTableView::ScrollDirection scrollDirection;

	bool editOnDoubleClick : 1;

	bool needAutoScroll : 1;

	bool disableDrawContents : 1;

	/*! true if the navigation panel is enabled (visible) for the view.
	 True by default. */
	bool navigatorEnabled : 1;

	/*! true if the context menu is enabled (visible) for the view.
	 True by default. */
	bool contextMenuEnabled : 1;

	/*! used to force single skip keyPress event. */
	bool skipKeyPress : 1;
	
	/*! Used to enable/disable execution of KexiTableView::vScrollBarValueChanged()
	 when users navigates rows using keyboard, so vscrollbar tooltips are not visible then. */
	bool vScrollBarValueChanged_enabled : 1;

	/*! True, if vscrollbar tooltips are enabled (true by default) */
	bool scrollbarToolTipsEnabled : 1;

	/*! Needed because pTopHeader->isVisible() is not always accurate. True by default.  */
	bool horizontalHeaderVisible : 1;

	/*! true if cursor should be moved on mouse release evenr rather than mouse press 
	 in handleContentsMousePressOrRelease().
	 False by default. Used by KeixComboBoxPopup. */
	bool moveCursorOnMouseRelease : 1;

	KexiTableView::Appearance appearance;
	
	QLabel *scrollBarTip;
	QTimer scrollBarTipTimer;
	uint scrollBarTipTimerCnt; //!< helper for timeout counting
	
	//! brushes, fonts
	QBrush diagonalGrayPattern;

	KexiDisplayUtils::DisplayParameters autonumberSignDisplayParameters;

	//! Used by delayed mode of maximizeColumnsWidth() 
	QValueList<int> maximizeColumnsWidthOnShow;

	/*! Used for delayed call of ensureCellVisible() after show().
	 It's equal to (-1,-1) if ensureCellVisible() shouldn't e called. */
	QPoint ensureCellVisibleOnShow;

	/*! @internal Changes bottom margin settings, in pixels. 
	 At this time, it's used by KexiComboBoxPopup to decrease margin for popup's table. */
	int internal_bottomMargin;

	/*! Helper for "highlighted row" effect. */
	int highlightedRow;

	/*! Id of context menu key (cached). */
	int contextMenuKey;
};

#endif
