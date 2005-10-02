/* This file is part of the KDE project
   Copyright (C) 2004 Lucijan Busch <lucijan@kde.org>

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

#include "kexirelationmaindlg.h"

#include <klocale.h>
#include <kdebug.h>
#include <kiconloader.h>

#include <qlayout.h>

#include <kexidb/connection.h>

#include "keximainwindow.h"
#include "kexiproject.h"
#include "kexirelationwidget.h"
#include "kexirelationview.h"

KexiRelationMainDlg::KexiRelationMainDlg(KexiMainWindow *mainWin, QWidget *parent, const char *name)
 : KexiViewBase(mainWin, parent, name)
{
	kdDebug() << "KexiRelationMainDlg()" << endl;
//	setIcon(SmallIcon("relation"));
	m_defaultIconName = "relation";
	setCaption( i18n("Relationships") );
//	setDocID( win->generatePrivateDocID() );

	m_rel = new KexiRelationWidget(mainWin, this);
	//the view can receive some our actions
	addActionProxyChild( m_rel );
//	addActionProxyChild( m_view->relationView() );
	
	QVBoxLayout *g = new QVBoxLayout(this);
	g->addWidget(m_rel);

	//show all tables
	KexiDB::Connection *conn = mainWin->project()->dbConnection();
	QStringList tables = conn->tableNames();
	for (QStringList::ConstIterator it = tables.constBegin(); it!=tables.constEnd(); ++it) {
		m_rel->addTable( *it );
	}
}

KexiRelationMainDlg::~KexiRelationMainDlg()
{
}

QSize KexiRelationMainDlg::sizeHint() const
{	
	return QSize(600,300);
}

QWidget*
KexiRelationMainDlg::mainWidget()
{
	return m_rel;
}

QString KexiRelationMainDlg::itemIcon()
{
	return "relation";
}

#include "kexirelationmaindlg.moc"

