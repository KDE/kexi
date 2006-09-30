/* This file is part of the KDE project
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2005 Jaroslaw Staniek <js@iidea.pl>

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
#include <qlayout.h>
#include <qcheckbox.h>
#include <qtooltip.h>

#include <kiconloader.h>
#include <kdebug.h>
#include <klocale.h>
#include <kpushbutton.h>

#include "form.h"
#include "objecttreeview.h"

#include "tabstopdialog.h"

using namespace KFormDesigner;

//////////////////////////////////////////////////////////////////////////////////
//////////  The Tab Stop Dialog to edit tab order  ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

TabStopDialog::TabStopDialog(QWidget *parent)
: KDialogBase(parent, "tabstop_dialog", true, i18n("Edit Tab Order"), Ok|Cancel, Ok, false)
{
	QFrame *frame = makeMainWidget();
	QGridLayout *l = new QGridLayout(frame, 2, 2, 0, 6);
	m_treeview = new ObjectTreeView(frame, "tabstops_treeview", true);
	m_treeview->setItemsMovable(true);
	m_treeview->setDragEnabled(true);
	m_treeview->setDropVisualizer(true);
	m_treeview->setAcceptDrops(true);
	m_treeview->setFocus();
	l->addWidget(m_treeview, 0, 0);

	m_treeview->m_form = 0;
	connect(m_treeview, SIGNAL(currentChanged(QListViewItem*)), this, SLOT(updateButtons(QListViewItem*)));
	connect(m_treeview, SIGNAL(moved(QListViewItem*, QListViewItem*, QListViewItem*)), this, SLOT(updateButtons(QListViewItem*)));

	QVBoxLayout *vbox = new QVBoxLayout();
	l->addLayout(vbox, 0, 1);
	m_btnUp = new KPushButton(SmallIconSet("1uparrow"), i18n("Move Up"), frame);
	QToolTip::add( m_btnUp, i18n("Move widget up") );
	vbox->addWidget(m_btnUp);
	connect(m_btnUp, SIGNAL(clicked()), this, SLOT(moveItemUp()));

	m_btnDown = new KPushButton(SmallIconSet("1downarrow"), i18n("Move Down"), frame);
	QToolTip::add( m_btnDown, i18n("Move widget down") );
	vbox->addWidget(m_btnDown);
	connect(m_btnDown, SIGNAL(clicked()), this, SLOT(moveItemDown()));
	vbox->addStretch();

	m_check = new QCheckBox(i18n("Handle tab order automatically"), frame, "tabstops_check");
	connect(m_check, SIGNAL(toggled(bool)), this, SLOT(slotRadioClicked(bool)));
	l->addMultiCellWidget(m_check, 1, 1, 0, 1);

	updateGeometry();
	setInitialSize(QSize(500+m_btnUp->width(), QMAX(400,m_treeview->height())));
}

TabStopDialog::~TabStopDialog()
{
}

int TabStopDialog::exec(Form *form)
{
	m_treeview->clear();
	m_treeview->m_form = form;

	if(form->autoTabStops())
		form->autoAssignTabStops();
	form->updateTabStopsOrder();
	ObjectTreeListIterator it( form->tabStopsIterator() );
	it.toLast();
	for(;it.current(); --it)
		new ObjectTreeViewItem(m_treeview, it.current());

	m_check->setChecked(form->autoTabStops());

	if (m_treeview->firstChild()) {
		m_treeview->setCurrentItem(m_treeview->firstChild());
		m_treeview->setSelected(m_treeview->firstChild(), true);
	}

	if (QDialog::Rejected == KDialogBase::exec())
		return QDialog::Rejected;

	//accepted
	form->setAutoTabStops(m_check->isChecked());
	if(form->autoTabStops())
	{
		form->autoAssignTabStops();
		return QDialog::Accepted;
	}

	//add items to the order list
	form->tabStops()->clear();
	ObjectTreeViewItem *item = (ObjectTreeViewItem*)m_treeview->firstChild();
	while(item)
	{
		ObjectTreeItem *tree = item->objectTree();
		if(tree)
			form->tabStops()->append(tree);
		item = (ObjectTreeViewItem*)item->nextSibling();
	}
	return QDialog::Accepted;
}

void
TabStopDialog::moveItemUp()
{
	if (!m_treeview->selectedItem())
		return;
	QListViewItem *before = m_treeview->selectedItem()->itemAbove();
	before->moveItem(m_treeview->selectedItem());
	updateButtons(m_treeview->selectedItem());
}

void
TabStopDialog::moveItemDown()
{
	QListViewItem *item = m_treeview->selectedItem();
	if (!item)
		return;
	item->moveItem( item->nextSibling());
	updateButtons(item);
}

void
TabStopDialog::updateButtons(QListViewItem *item)
{
	m_btnUp->setEnabled( item && (item->itemAbove() && m_treeview->isEnabled()
	/*&& (item->itemAbove()->parent() == item->parent()))*/ ));
	m_btnDown->setEnabled( item && item->nextSibling() && m_treeview->isEnabled() );
}

void
TabStopDialog::slotRadioClicked(bool isOn)
{
	m_treeview->setEnabled(!isOn);
	updateButtons( m_treeview->selectedItem() );
}

bool
TabStopDialog::autoTabStops() const
{
	return m_check->isChecked();
}

#include "tabstopdialog.moc"

