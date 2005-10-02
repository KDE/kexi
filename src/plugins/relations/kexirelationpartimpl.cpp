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
#include "kexirelationpartimpl.h"

#include <kexirelationwidget.h>
#include <kexidialogbase.h>
#include <keximainwindow.h>

#include <kgenericfactory.h>
#include <kiconloader.h>
#include <kdebug.h>

KexiRelationPartImpl::KexiRelationPartImpl(QObject *parent, const char *name, const QStringList &args)
 : KexiInternalPart(parent, name, args)
{
	kdDebug() << "KexiRelationPartImpl()" << endl;
}

KexiRelationPartImpl::~KexiRelationPartImpl()
{
}

/*QWidget *
KexiRelationPartImpl::createWidget(const char* , KexiMainWindow* mainWin, 
 QWidget *parent, const char *objName)
{
	return new KexiRelationWidget(mainWin, parent, objName);
}*/

/*KexiDialogBase *
KexiRelationPartImpl::createDialog(KexiMainWindow* mainWin, const char *)
{
	kdDebug() << "KexiRelationPartImpl::createDialog()" << endl;
	KexiDialogBase * dlg = new KexiDialogBase(mainWin, i18n("Relations"));
	dlg->setIcon(SmallIcon("relation"));
	dlg->setDocID( mainWin->generatePrivateDocID() );

	KexiRelationMainDlg *view = new KexiRelationMainDlg(mainWin, 0, "relations");
	dlg->addView(view);
//	dlg->show();
//	dlg->registerDialog();

	return dlg;
}*/

KexiViewBase *
KexiRelationPartImpl::createView(KexiMainWindow* mainWin, QWidget *parent, const char *)
{
//	kdDebug() << "KexiRelationPartImpl::createDialog()" << endl;
//	KexiDialogBase * dlg = new KexiDialogBase(mainWin, i18n("Relations"));
//	dlg->setIcon(SmallIcon("relation"));
//	dlg->setDocID( mainWin->generatePrivateDocID() );

	KexiRelationMainDlg *view = new KexiRelationMainDlg(mainWin, parent, "relations");
//	dlg->addView(view);
//	dlg->show();
//	dlg->registerDialog();

	return view;
}


K_EXPORT_COMPONENT_FACTORY( kexihandler_relation, 
	KGenericFactory<KexiRelationPartImpl>("kexihandler_relation") )

#include "kexirelationpartimpl.moc"

