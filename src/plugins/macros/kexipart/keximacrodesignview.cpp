/* This file is part of the KDE project
   Copyright (C) 2006 Sebastian Sauer <mail@dipe.org>

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
   Boston, MA 02110-1301, USA.
*/

#include "keximacrodesignview.h"
#include "keximacroproperty.h"

#include <qtimer.h>
#include <qdom.h>
#include <kdebug.h>

#include <kexidialogbase.h>
#include <kexidb/connection.h>
#include <kexidb/error.h>

#include <core/kexi.h>
#include <core/kexiproject.h>
#include <core/kexipartmanager.h>
#include <core/kexipartinfo.h>

#include <widget/kexidatatable.h>
#include <widget/tableview/kexitableview.h>
#include <widget/tableview/kexitableviewdata.h>
#include <widget/tableview/kexitableitem.h>
#include <widget/tableview/kexidataawarepropertyset.h>

#include <koproperty/set.h>
#include <koproperty/property.h>

#include "../lib/macro.h"
#include "../lib/macroitem.h"
#include "../lib/xmlhandler.h"

/// constants used to name columns instead of hardcoding indices
#define COLUMN_ID_ACTION 0
#define COLUMN_ID_COMMENT 1

/**
* \internal d-pointer class to be more flexible on future extension of the
* functionality without to much risk to break the binary compatibility.
*/
class KexiMacroDesignView::Private
{
	public:

		/**
		* The view used to display the actions
		* a \a Macro has.
		*/
		KexiDataTable* datatable;

		/**
		* For convenience. The table view ( datatable->tableView() ).
		*/
		KexiTableView* tableview;

		/**
		* The \a KexiTableViewData data-model for the
		* \a KexiTableView above.
		*/
		KexiTableViewData* tabledata;

		/**
		* The \a KexiDataAwarePropertySet is used to display
		* properties an action provides in the propertyview.
		*/
		KexiDataAwarePropertySet* propertyset;

		/// Boolean flag to avoid infinite recursion.
		bool reloadsProperties;
		/// Boolean flag to avoid infinite recursion.
		bool updatesProperties;

		/**
		* Constructor.
		*
		* \param m The passed \a KoMacro::Manager instance our
		*        \a manager points to.
		*/
		Private()
			: propertyset(0)
			, reloadsProperties(false)
			, updatesProperties(false)
		{
		}

		/**
		* Destructor.
		*/
		~Private()
		{
			delete propertyset;
		}

};

KexiMacroDesignView::KexiMacroDesignView(KexiMainWindow *mainwin, QWidget *parent, ::KoMacro::Macro* const macro)
	: KexiMacroView(mainwin, parent, macro, "KexiMacroDesignView")
	, d( new Private() )
{
	// The table's data-model.
	d->tabledata = new KexiTableViewData();
	d->tabledata->setSorting(-1); // disable sorting

	// Add the "Action" column.
	KexiTableViewColumn* actioncol = new KexiTableViewColumn(
		"action", // name/identifier
		KexiDB::Field::Enum, // fieldtype
		KexiDB::Field::NoConstraints, // constraints
		KexiDB::Field::NoOptions, // options
		0, // length
		0, // precision
		QVariant(), // default value
		i18n("Action"), // caption
		QString::null, // description
		0 // width
	);
	d->tabledata->addColumn(actioncol);

	QValueVector<QString> items;
	items.append(""); // empty means no action

	// Append the list of actions provided by Kexi.
	QStringList actionnames = KoMacro::Manager::self()->actionNames();
	QStringList::ConstIterator it, end( actionnames.constEnd() );
	for( it = actionnames.constBegin(); it != end; ++it) {
		KSharedPtr<KoMacro::Action> action = KoMacro::Manager::self()->action(*it);
		items.append( action->text() );
	}

	actioncol->field()->setEnumHints(items);

	// Add the "Comment" column.
	d->tabledata->addColumn( new KexiTableViewColumn(
		"comment", // name/identifier
		KexiDB::Field::Text, // fieldtype
		KexiDB::Field::NoConstraints, // constraints
		KexiDB::Field::NoOptions, // options
		0, // length
		0, // precision
		QVariant(), // default value
		i18n("Comment"), // caption
		QString::null, // description
		0 // width
	) );

	// Create the tableview.
	QHBoxLayout* layout = new QHBoxLayout(this);
	d->datatable = new KexiDataTable(mainWin(), this, "Macro KexiDataTable", false /*not db aware*/);
	layout->addWidget(d->datatable);
	d->tableview = d->datatable->tableView();
	d->tableview->setSpreadSheetMode();
	d->tableview->setColumnStretchEnabled( true, COLUMN_ID_COMMENT ); //last column occupies the rest of the area

	// We need to register our KexiMacroPropertyFactory to use our own
	// KoProperty::Property implementation.
	KexiMacroPropertyFactory::initFactory();

	// Create the propertyset.
	d->propertyset = new KexiDataAwarePropertySet(this, d->tableview);

	// Connect signals the KexiDataTable provides to local slots.
	connect(d->tabledata, SIGNAL(aboutToChangeCell(KexiTableItem*,int,QVariant&,KexiDB::ResultInfo*)),
	        this, SLOT(beforeCellChanged(KexiTableItem*,int,QVariant&,KexiDB::ResultInfo*)));
	connect(d->tabledata, SIGNAL(rowUpdated(KexiTableItem*)),
	        this, SLOT(rowUpdated(KexiTableItem*)));
	connect(d->tabledata, SIGNAL(rowInserted(KexiTableItem*,uint,bool)),
	        this, SLOT(rowInserted(KexiTableItem*,uint,bool)));
	connect(d->tabledata, SIGNAL(rowDeleted()),
	        this, SLOT(rowDeleted()));

	// Everything is ready. So, update the data now.
	updateData();
	setDirty(false);
}

KexiMacroDesignView::~KexiMacroDesignView()
{
	delete d;
}

void KexiMacroDesignView::updateData()
{
	kdDebug() << "KexiMacroDesignView::updateData()" << endl;

	// Remove previous content of tabledata.
	d->tabledata->deleteAllRows();
	// Remove old property sets.
	d->propertyset->clear();

	// Add some empty rows
	for (int i=0; i<50; i++) {
		d->tabledata->append( d->tabledata->createItem() );
	}

	// Set the MacroItem's
	QStringList actionnames = KoMacro::Manager::self()->actionNames();
	KoMacro::MacroItem::List macroitems = macro()->items();
	KoMacro::MacroItem::List::ConstIterator it(macroitems.constBegin()), end(macroitems.constEnd());
	for(uint idx = 0; it != end; ++it, idx++) {
		KexiTableItem* tableitem = d->tabledata->at(idx);
		if(! tableitem) {
			// If there exists no such item, add it.
			tableitem = d->tabledata->createItem();
			d->tabledata->append(tableitem);
		}
		// Set the action-column.
		KSharedPtr<KoMacro::Action> action = (*it)->action();
		if(action.data()) {
			int i = actionnames.findIndex( action->name() );
			if(i >= 0) {
				tableitem->at(COLUMN_ID_ACTION) = i + 1;
				//setAction(tableitem, action->name());
			}
		}
		// Set the comment-column.
		tableitem->at(COLUMN_ID_COMMENT) = (*it)->comment();
	}

	// set data for our spreadsheet: this will clear our sets
	d->tableview->setData(d->tabledata);

	// Add the property sets.
	it = macroitems.constBegin();
	for(uint idx = 0; it != end; ++it, idx++) {
		updateProperties(idx, 0, *it);
	}

	// work around a bug in the KexiTableView where we lose the stretch-setting...
	d->tableview->setColumnStretchEnabled( true, COLUMN_ID_COMMENT ); //last column occupies the rest of the area

	propertySetReloaded(true);
}

bool KexiMacroDesignView::loadData()
{
	if(! KexiMacroView::loadData()) {
		return false;
	}
	updateData(); // update the tableview's data.
	return true;
}

KoProperty::Set* KexiMacroDesignView::propertySet()
{
	return d->propertyset->currentPropertySet();
}

void KexiMacroDesignView::beforeCellChanged(KexiTableItem* item, int colnum, QVariant& newvalue, KexiDB::ResultInfo* result)
{
	Q_UNUSED(result);
	kdDebug() << "KexiMacroDesignView::beforeCellChanged() colnum=" << colnum << " newvalue=" << newvalue.toString() << endl;

	int rowindex = d->tabledata->findRef(item);
	if(rowindex < 0) {
		kdWarning() << "KexiMacroDesignView::beforeCellChanged() No such item" << endl;
		return;
	}

	// If the rowindex doesn't exists yet, we need to append new
	// items till we are able to access the item we like to use.
	for(int i = macro()->items().count(); i <= rowindex; i++) {
		macro()->addItem( KSharedPtr<KoMacro::MacroItem>( new KoMacro::MacroItem() ) );
	}

	// Get the matching MacroItem.
	KSharedPtr<KoMacro::MacroItem> macroitem = macro()->items()[rowindex];
	if(! macroitem.data()) {
		kdWarning() << "KexiMacroDesignView::beforeCellChanged() Invalid item for rowindex=" << rowindex << endl;
		return;
	}

	// Handle the column that should be changed
	switch(colnum) {
		case COLUMN_ID_ACTION: { // The "Action" column
			QString actionname;
			bool ok;
			int selectedindex = newvalue.toInt(&ok);
			if(ok && selectedindex > 0) {
				QStringList actionnames = KoMacro::Manager::self()->actionNames();
				actionname = actionnames[ selectedindex - 1 ]; // first item is empty
			}
			KSharedPtr<KoMacro::Action> action = KoMacro::Manager::self()->action(actionname);
			macroitem->setAction(action);
			updateProperties(d->propertyset->currentRow(), d->propertyset->currentPropertySet(), macroitem);
			propertySetReloaded(true);
		} break;
		case COLUMN_ID_COMMENT: { // The "Comment" column
			macroitem->setComment( newvalue.toString() );
		} break;
		default:
			kdWarning() << "KexiMacroDesignView::beforeCellChanged() No such column number " << colnum << endl;
			return;
	}

	setDirty();
}

void KexiMacroDesignView::rowUpdated(KexiTableItem* item)
{
    int rowindex = d->tabledata->findRef(item);
	kdDebug() << "KexiMacroDesignView::rowUpdated() rowindex=" << rowindex << endl;
	//propertySetSwitched();
	//propertySetReloaded(true);
	//setDirty();
}

void KexiMacroDesignView::rowInserted(KexiTableItem*, uint row, bool)
{
	kdDebug() << "KexiMacroDesignView::rowInserted() rowindex=" << row << endl;
	KoMacro::MacroItem::List& macroitems = macro()->items();

	if(row < macroitems.count()) {
		// If a new item was inserted, we need to insert a new item to our
		// list of MacroItems too. If the new item was appended, we don't
		// need to do anything yet cause the new item will be handled on
		// beforeCellChanged() anyway.
		kdDebug() << "KexiMacroDesignView::rowInserted() Inserting new MacroItem" << endl;
		KSharedPtr<KoMacro::MacroItem> macroitem = KSharedPtr<KoMacro::MacroItem>( new KoMacro::MacroItem() );
		KoMacro::MacroItem::List::Iterator it = macroitems.at(row);
		macroitems.insert(it, macroitem);
	}
}

void KexiMacroDesignView::rowDeleted()
{
	int rowindex = d->propertyset->currentRow();
	if(rowindex < 0) {
		kdWarning() << "KexiMacroDesignView::rowDeleted() No such item" << endl;
		return;
	}
	kdDebug() << "KexiMacroDesignView::rowDeleted() rowindex=" << rowindex << endl;
	KoMacro::MacroItem::List& macroitems = macro()->items();
	macroitems.remove( macroitems.at(rowindex) );
}

bool KexiMacroDesignView::updateSet(KoProperty::Set* set, KSharedPtr<KoMacro::MacroItem> macroitem, const QString& variablename)
{
	kdDebug() << "KexiMacroDesignView::updateSet() variablename=" << variablename << endl;
	KoProperty::Property* property = KexiMacroProperty::createProperty(macroitem, variablename);
	if(! property)
		return false;
	set->addProperty(property);
	return true;
}

void KexiMacroDesignView::updateProperties(int row, KoProperty::Set* set, KSharedPtr<KoMacro::MacroItem> macroitem)
{
	kdDebug() << "KexiMacroDesignView::updateProperties() row=" << row << endl;

	if(row < 0 || d->updatesProperties) {
		return; // ignore invalid rows and avoid infinite recursion.
	}

	KSharedPtr<KoMacro::Action> action = macroitem->action();
	if(! action.data()) {
		// don't display a propertyset if there is no action defined.
		d->propertyset->remove(row);
		return; // job done.
	}

	d->updatesProperties = true;

	if(set) {
		// we need to clear old data before adding the new content.
		set->clear();
	}
	else {
		// if there exists no such propertyset yet, create one.
		set = new KoProperty::Set(d->propertyset, action->name());
		d->propertyset->insert(row, set, true);
		connect(set, SIGNAL(propertyChanged(KoProperty::Set&, KoProperty::Property&)),
		        this, SLOT(propertyChanged(KoProperty::Set&, KoProperty::Property&)));
	}

	// The caption.
	KoProperty::Property* prop = new KoProperty::Property("this:classString", action->text());
	prop->setVisible(false);
	set->addProperty(prop);

	// Display the list of variables.
	QStringList varnames = action->variableNames();
	for(QStringList::Iterator it = varnames.begin(); it != varnames.end(); ++it) {
		if(updateSet(set, macroitem, *it)) {
			KSharedPtr<KoMacro::Variable> variable = macroitem->variable(*it, true);
			kdDebug()<<"KexiMacroDesignView::updateProperties() name=" << *it << " variable=" << variable->variant().toString() << endl;
#if 0
			macroitem->setVariable(*it, variable);
#endif
		}
	}

	d->updatesProperties = false;
}

void KexiMacroDesignView::propertyChanged(KoProperty::Set& set, KoProperty::Property& property)
{
	Q_UNUSED(set);
	kdDebug() << "!!!!! KexiMacroDesignView::propertyChanged() propertyname=" << property.name() << endl;
	setDirty();

	/*
	if(d->reloadsProperties) // be sure to don't update properties if we are still on reloading.
		return;
	d->reloadsProperties = true;

	const int row = d->propertyset->currentRow();
	const QCString name = property.name();
	kdDebug() << "KexiMacroDesignView::propertyChanged() name=" << name << " row=" << row << endl;

	//TODO reload is only needed if something changed!
	bool dirty = true; bool reload = true;//dirtyvarnames.count()>0;

	if(dirty || reload) { // Only reload properties if it's really needed.
		setDirty();
		if(reload) {
			// The MacroItem which should be changed.
			KSharedPtr<KoMacro::MacroItem> macroitem = macro()->items()[row];
			// Update the properties.
			updateProperties(row, &set, macroitem);
		}
		// It's needed to call the reload delayed cause in KoProperty::Editor
		// QTimer::singleShot(10, this, SLOT(selectItemLater())); may lead
		// to crashes if we are to fast.
		QTimer::singleShot(50, this, SLOT(reloadPropertyLater()));
	}

	d->reloadsProperties = false;
	*/

	/*
	QStringList dirtyvarnames = macroitem->setVariable(name, KSharedPtr<KoMacro::Variable>(pv));
	bool dirty = false;
	bool reload = false;
	for(QStringList::Iterator it = dirtyvarnames.begin(); it != dirtyvarnames.end(); ++it) {
		KSharedPtr<KoMacro::Variable> variable = macroitem->variable(*it);
		if(! variable.data()) {
			kdDebug() << "KexiMacroDesignView::propertyChanged() name=" << name << " it=" << *it << " skipped cause such a variable is not known." << endl;
			continue;
		}

		if(! set.contains( (*it).latin1() )) {
			// If there exist no such property yet, we need to add it.
			if(updateSet(&set, macroitem, *it))
				reload = true; // we like to reload the whole set
			continue;
		}

		kdDebug() << "KexiMacroDesignView::propertyChanged() set existing property=" << *it << endl;
		KoProperty::Property& p = set.property((*it).latin1());
		KoMacro::Variable::List children = variable->children();
		if(children.count() > 0) {
			QStringList keys, names;
			KoMacro::Variable::List::Iterator childit(children.begin()), childend(children.end());
			for(; childit != childend; ++childit) {
				const QString s = (*childit)->variant().toString();
				keys << s;
				names << s;
			}
			p.setListData( new KoProperty::Property::ListData(keys, names) );
		}
		p.setValue(variable->variant());
		dirty = true;
	}

	// If there are expired aka not any longer needed properties around, we
	// need to reload the whole set.
	for(KoProperty::Set::Iterator setit = set; setit.current(); ++setit) {
		if(setit.currentKey() == name) continue; // don't remove ourself
		if(setit.currentKey().left(5) == QCString("this:")) continue; // don't remove internal properties
		if(setit.currentKey() == QCString("newrow")) continue; // also an internal used property
		if(action.data() && action->hasVariable(setit.currentKey())) continue; // the property is still valid
		reload = true; // we like to reload the whole set
	}
	*/
}

void KexiMacroDesignView::reloadPropertyLater()
{
	propertySetReloaded(true);
}

#include "keximacrodesignview.moc"

