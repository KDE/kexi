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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Original Author:  Till Busch <till@bux.at>
   Original Project: buX (www.bux.at)
*/

#include "kexitableview_p.h"
#include "kexitableedit.h"

#include <qlabel.h>

#include <kglobalsettings.h>

//---------------

static const unsigned int  autonumber_png_len = 245;
static const unsigned char autonumber_png_data[] = {
    0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,0x00,0x00,0x00,0x0d,0x49,0x48,
    0x44,0x52,0x00,0x00,0x00,0x0b,0x00,0x00,0x00,0x0d,0x08,0x06,0x00,0x00,
    0x00,0x7f,0xf5,0x94,0x3b,0x00,0x00,0x00,0x06,0x62,0x4b,0x47,0x44,0x00,
    0xff,0x00,0xff,0x00,0xff,0xa0,0xbd,0xa7,0x93,0x00,0x00,0x00,0x09,0x70,
    0x48,0x59,0x73,0x00,0x00,0x0b,0x11,0x00,0x00,0x0b,0x11,0x01,0x7f,0x64,
    0x5f,0x91,0x00,0x00,0x00,0x07,0x74,0x49,0x4d,0x45,0x07,0xd4,0x08,0x14,
    0x0c,0x09,0x11,0x18,0x18,0x1d,0x4f,0x00,0x00,0x00,0x82,0x49,0x44,0x41,
    0x54,0x78,0x9c,0x8d,0x91,0x41,0x0e,0x03,0x31,0x08,0x03,0x87,0xbe,0x2e,
    0x1c,0xb3,0xff,0xbf,0xf6,0x1d,0xee,0x81,0xa0,0x05,0xaa,0x55,0x6b,0x29,
    0x92,0x03,0x06,0x59,0x06,0x49,0x48,0x02,0xa4,0xe4,0xf1,0x5f,0x1b,0xa4,
    0x78,0x6b,0xc3,0xc2,0x24,0x61,0x86,0x00,0x24,0x8c,0x83,0x53,0x33,0xe9,
    0xe6,0xaf,0x29,0x4a,0x48,0x29,0xf4,0x0d,0xbc,0xc1,0xe1,0xc9,0x46,0xb5,
    0x72,0xfa,0xcf,0xe2,0x2a,0x4c,0x71,0xf3,0x5c,0x2d,0xd5,0x5a,0xc0,0xcd,
    0x62,0xea,0x6f,0xf4,0x88,0x86,0x95,0xf0,0x4a,0xf2,0xee,0x6b,0xf8,0x1e,
    0x03,0x55,0xf8,0x73,0xf3,0x28,0x7e,0x6d,0x6e,0x69,0xc4,0xc6,0xfb,0x52,
    0x23,0x8d,0x3c,0x56,0x5e,0xd0,0x2f,0x40,0xd1,0xf4,0x6b,0xc4,0xd5,0xf8,
    0x07,0x69,0x14,0xc6,0x69,0x9a,0x12,0x79,0x9a,0x00,0x00,0x00,0x00,0x49,
    0x45,0x4e,0x44,0xae,0x42,0x60,0x82
};

/* Generated by qembed */
#include <qcstring.h>
#include <qdict.h>
static struct Embed {
    unsigned int size;
    const unsigned char *data;
    const char *name;
} embed_vec[] = {
    { 245, autonumber_png_data, "autonumber.png" },
    { 0, 0, 0 }
};

static const QByteArray& qembed_findData( const char* name )
{
    static QDict<QByteArray> dict;
    QByteArray* ba = dict.find( name );
    if ( !ba ) {
	for ( int i = 0; embed_vec[i].data; i++ ) {
	    if ( strcmp(embed_vec[i].name, name) == 0 ) {
		ba = new QByteArray;
		ba->setRawData( (char*)embed_vec[i].data,
				embed_vec[i].size );
		dict.insert( name, ba );
		break;
	    }
	}
	if ( !ba ) {
	    static QByteArray dummy;
	    return dummy;
	}
    }
    return *ba;
}

//---------------

KexiTableViewPrivate::KexiTableViewPrivate(KexiTableView* t)
 : appearance(t)
 , autonumberIcon( qembed_findData("autonumber.png") )
{
	clearVariables();
	tv = t;
//moved	pInsertItem = 0;

	editOnDoubleClick = true;
	pBufferPm = 0;
//moved	deletionPolicy = KexiTableView::AskDelete;
	disableDrawContents = false;
//moved	readOnly = -1; //don't know
//moved	insertingEnabled = -1; //don't know
	
//moved	contentsMousePressEvent_dblClick = false;
//moved	isSortingEnabled = true;
	navigatorEnabled = true;
	contextMenuEnabled = true;
//moved	filteringEnabled = true;
//moved	navPanel = 0;
	skipKeyPress = false;
	vScrollBarValueChanged_enabled = true;
	scrollbarToolTipsEnabled = true;
	scrollBarTipTimerCnt = 0;
	scrollBarTip = 0;
//moved	inside_acceptEditor = false;
//moved	internal_acceptsRowEditAfterCellAccepting = false;
//moved	acceptsRowEditAfterCellAccepting = false;
//moved	emptyRowInsertingEnabled = false;
//moved	dragIndicatorLine = -1;
//moved		rowWillBeDeleted = -1;
//	dropsAtRowEnabled = false;
//moved	initDataContentsOnShow = false;
//moved	cursorPositionSetExplicityBeforeShow = false;
	verticalHeaderAlreadyAdded = false;
	ensureCellVisibleOnShow = QPoint(-1,-1);
//moved	spreadSheetMode = false;
	internal_bottomMargin = tv->horizontalScrollBar()->sizeHint().height()/2;
	highlightedRow = -1;
	moveCursorOnMouseRelease = false;
}

KexiTableViewPrivate::~KexiTableViewPrivate()
{
	delete pBufferPm;
//moved	delete pInsertItem;
//moved	delete pRowEditBuffer;
	delete scrollBarTip;
}

void KexiTableViewPrivate::clearVariables()
{
	// Initialize variables
//moved	pEditor = 0;
//	numRows = 0;
//moved	curRow = -1;
//moved	curCol = -1;
//moved	pCurrentItem=0;
//moved	pRowEditBuffer=0;
//	pInsertItem = 0;
//moved	rowEditing = false;
//moved	newRowEditing = false;
//	sortedColumn = -1;
//	sortOrder = true;
//	recordIndicator = false;
}

