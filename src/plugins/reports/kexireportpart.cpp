/* This file is part of the KDE project
   Copyright (C) 2004 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>

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

#include <kdebug.h>
#include <kgenericfactory.h>

#include "kexiviewbase.h"
#include "keximainwindow.h"
#include "kexiproject.h"
#include <kexipartitem.h>
#include <kexidialogbase.h>

#include <kexidb/connection.h>
#include <kexidb/fieldlist.h>
#include <kexidb/field.h>

#include <form.h>
#include <formIO.h>
#include <widgetlibrary.h>

#include <kexiformmanager.h>
#include <kexiformpart.h>

#include "kexireportview.h"
#include "kexireportpart.h"

KFormDesigner::WidgetLibrary* KexiReportPart::static_reportsLibrary = 0L;

KexiReportPart::KexiReportPart(QObject *parent, const char *name, const QStringList &l)
 : KexiPart::Part(parent, name, l)
{
	// REGISTERED ID:
	m_registeredPartID = (int)KexiPart::ReportObjectType;

	kexipluginsdbg << "KexiReportPart::KexiReportPart()" << endl;
	m_names["instanceName"] 
		= i18n("Translate this word using only lowercase alphanumeric characters (a..z, 0..9). "
		"Use '_' character instead of spaces. First character should be a..z character. "
		"If you cannot use latin characters in your language, use english word.", 
		"report");
	m_names["instanceCaption"] = i18n("Report");
	m_supportedViewModes = Kexi::DataViewMode | Kexi::DesignViewMode;

	// Only create form manager if it's not yet created.
	// KexiFormPart could have created is already.
	KFormDesigner::FormManager *formManager = KFormDesigner::FormManager::self();
	if (!formManager) 
		formManager = new KexiFormManager(this, "kexi_form_and_report_manager");

	// Create and store a handle to report' library. Forms will have their own library too.
/* @todo add configuration for supported factory groups */
	QStringList supportedFactoryGroups;
	supportedFactoryGroups += "kexi-report";
	static_reportsLibrary = KFormDesigner::FormManager::createWidgetLibrary(
		formManager, supportedFactoryGroups);
	static_reportsLibrary->setAdvancedPropertiesVisible(false);
}

KexiReportPart::~KexiReportPart()
{
}

KFormDesigner::WidgetLibrary* KexiReportPart::library()
{
	return static_reportsLibrary;
}

void
KexiReportPart::initPartActions()
{
}

void
KexiReportPart::initInstanceActions()
{
	KFormDesigner::FormManager::self()->createActions(
		library(), actionCollectionForMode(Kexi::DesignViewMode), guiClient());
}

KexiDialogTempData*
KexiReportPart::createTempData(KexiDialogBase* dialog)
{
	return new KexiReportPart::TempData(dialog);
}

KexiViewBase*
KexiReportPart::createView(QWidget *parent, KexiDialogBase* dialog,
	KexiPart::Item &item, int, QMap<QString,QString>*)
{
	kexipluginsdbg << "KexiReportPart::createView()" << endl;
	KexiMainWindow *win = dialog->mainWin();
	if (!win || !win->project() || !win->project()->dbConnection())
		return 0;

	KexiReportView *view = new KexiReportView(win, parent, item.name().latin1(),
		win->project()->dbConnection() );

	return view;
}

QString
KexiReportPart::i18nMessage(const QCString& englishMessage, KexiDialogBase* dlg) const
{
	Q_UNUSED(dlg);
	if (englishMessage=="Design of object \"%1\" has been modified.")
		return i18n("Design of report \"%1\" has been modified.");
	if (englishMessage=="Object \"%1\" already exists.")
		return i18n("Report \"%1\" already exists.");
	return englishMessage;
}

//---------------

KexiReportPart::TempData::TempData(QObject* parent)
 : KexiDialogTempData(parent)
{
}

KexiReportPart::TempData::~TempData()
{
}

#include "kexireportpart.moc"

