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

#include "kexidbsubform.h"

#include "kexidbform.h"
#include "../kexiformview.h"
#include <kexidb/utils.h>
#include <formeditor/formIO.h>
#include <formeditor/objecttree.h>
#include <formeditor/utils.h>
#include <formeditor/container.h>
#include <formeditor/formmanager.h>

KexiDBSubForm::KexiDBSubForm(KFormDesigner::Form *parentForm, QWidget *parent, const char *name)
: QScrollView(parent, name), m_parentForm(parentForm), m_form(0), m_widget(0)
{
	setFrameStyle(QFrame::WinPanel | QFrame::Sunken);
	viewport()->setPaletteBackgroundColor(colorGroup().mid());
}
/*
void
KexiDBSubForm::paintEvent(QPaintEvent *ev)
{
	QScrollView::paintEvent(ev);
	QPainter p;

	setWFlags(WPaintUnclipped);

	QString txt("Subform");
	QFont f = font();
	f.setPointSize(f.pointSize() * 3);
	QFontMetrics fm(f);
	const int txtw = fm.width(txt), txth = fm.height();

	p.begin(this, true);
	p.setPen(black);
	p.setFont(f);
	p.drawText(width()/2, height()/2, txt, Qt::AlignCenter|Qt::AlignVCenter);
	p.end();

	clearWFlags( WPaintUnclipped );
}
*/
void
KexiDBSubForm::setFormName(const QString &name)
{
	if(m_formName==name)
		return;

	m_formName = name; //assign, even if the name points to nowhere

	if(name.isEmpty()) {
		delete m_widget;
		m_widget = 0;
		updateScrollBars();
		return;
	}

	QWidget *pw = parentWidget();
	KexiFormView *view = 0;
	QStringList list;
	while(pw) {
		if(pw->isA("KexiDBSubForm")) {
			if(list.contains(pw->name())) {
//! @todo error message
				return; // Be sure to don't run into a endless-loop cause of recursive subforms.
			}
			list.append(pw->name());
		}
		else if(! view && pw->isA("KexiFormView"))
			view = static_cast<KexiFormView*>(pw); // we need a KexiFormView*
		pw = pw->parentWidget();
	}

	if (!view || !view->parentDialog() || !view->parentDialog()->mainWin()
		|| !view->parentDialog()->mainWin()->project()->dbConnection())
		return;

	KexiDB::Connection *conn = view->parentDialog()->mainWin()->project()->dbConnection();

	// we check if there is a form with this name
	int id = KexiDB::idForObjectName(*conn, name, KexiPart::FormObjectType);
	if((id == 0) || (id == view->parentDialog()->id())) // == our form
		return; // because of recursion when loading

	// we create the container widget
	delete m_widget;
	m_widget = new KexiDBFormBase(viewport(), "KexiDBSubForm_widget");
	m_widget->show();
	addChild(m_widget);
	m_form = new KFormDesigner::Form(KexiFormPart::library(), this->name());
	m_form->createToplevel(m_widget);

	// and load the sub form
	QString data;
	tristate res = conn->loadDataBlock(id, data, QString::null);
	if (res == true)
		res = KFormDesigner::FormIO::loadFormFromString(m_form, m_widget, data);
	if(res != true) {
		delete m_widget;
		m_widget = 0;
		updateScrollBars();
		m_formName = QString::null;
		return;
	}
	m_form->setDesignMode(false);

	// Install event filters on the whole newly created form
	KFormDesigner::ObjectTreeItem *tree = m_parentForm->objectTree()->lookup(QObject::name());
	KFormDesigner::installRecursiveEventFilter(this, tree->eventEater());
}

#include "kexidbsubform.moc"
