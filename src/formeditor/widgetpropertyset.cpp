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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#include "widgetpropertyset.h"

#include <qstringlist.h>
#include <qstrlist.h>
#include <qmetaobject.h>
#include <qvariant.h>
#include <qevent.h>
#include <qlayout.h>

#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>

#include "objecttree.h"
#include "form.h"
#include "container.h"
#include "formmanager.h"
#include "widgetlibrary.h"
#include "commands.h"

#include <kexiutils/utils.h>
#include <kexiutils/identifier.h>

using namespace KFormDesigner;

namespace KFormDesigner {
//! @internal
class WidgetPropertySetPrivate
{
	public:
		WidgetPropertySetPrivate()
		: manager(0), lastCommand(0), lastGeoCommand(0),
		 isUndoing(false), slotPropertyChangedEnabled(true),
		 slotPropertyChanged_addCommandEnabled(true),
		 origActiveColors(0)
		{}
		~WidgetPropertySetPrivate()
		{
			delete origActiveColors;
		}

		Set  set;
		// list of properties (not) to show in editor
		QStringList  properties;
		// list of widgets
		QPtrList<QWidget>  widgets;
		FormManager  *manager;

		// used to update command's value when undoing
		PropertyCommand  *lastCommand;
		GeometryPropertyCommand  *lastGeoCommand;
		bool isUndoing : 1;
		bool slotPropertyChangedEnabled : 1;
		bool slotPropertyChanged_addCommandEnabled : 1;

		// helper to change color palette when switching 'enabled' property
		QColorGroup* origActiveColors;

		// i18n stuff
		QMap<QCString, QString> propCaption;
		QMap<QCString, QString> propValCaption;
};
}

WidgetPropertySet::WidgetPropertySet(FormManager *manager)
 : QObject(manager, "kfd_widgetPropertySet")
{
	d = new WidgetPropertySetPrivate();
	d->manager = manager;

	connect(&d->set, SIGNAL(propertyChanged(KoProperty::Set&, KoProperty::Property&)),
		this, SLOT(slotPropertyChanged(KoProperty::Set&, KoProperty::Property&)));
	connect(&d->set, SIGNAL(propertyReset(KoProperty::Set&, KoProperty::Property&)),
		this, SLOT(slotPropertyReset(KoProperty::Set&, KoProperty::Property&)));

	initPropertiesDescription();
}

WidgetPropertySet::~WidgetPropertySet()
{
	delete d;
}

FormManager*
WidgetPropertySet::manager()
{
	return d->manager;
}

Property&
WidgetPropertySet::operator[](const QCString &name)
{
	return d->set[name];
}

Property&
WidgetPropertySet::property(const QCString &name)
{
	return d->set[name];
}

bool
WidgetPropertySet::contains(const QCString &property)
{
	return d->set.contains(property);
}

KoProperty::Set*
WidgetPropertySet::set()
{
	return &(d->set);
}

void
WidgetPropertySet::clearSet(bool dontSignalShowPropertySet)
{
	saveModifiedProperties();

	if (!dontSignalShowPropertySet)
		d->manager->showPropertySet(0);
	d->widgets.clear();
	d->lastCommand = 0;
	d->lastGeoCommand = 0;
	d->properties.clear();
	d->set.clear();

	if(!d->widgets.isEmpty())  {
		d->widgets.first()->removeEventFilter(this);
		disconnect(d->widgets.first(), 0, this, 0);
	}
}

void
WidgetPropertySet::saveModifiedProperties()
{
	QWidget * w = d->widgets.first();
	if(!w || d->widgets.count() > 1 || !d->manager->activeForm() || !d->manager->activeForm()->objectTree())
			return;
	ObjectTreeItem *tree = d->manager->activeForm()->objectTree()->lookup(w->name());
	if(!tree)
		return;

	for(Set::Iterator it(d->set); it.current(); ++it) {
		if(it.current()->isModified())
			tree->addModifiedProperty(it.current()->name(), it.current()->oldValue());
	}
}

void
WidgetPropertySet::setUndoing(bool isUndoing)
{
	d->isUndoing = isUndoing;
}

bool
WidgetPropertySet::isUndoing()
{
	return d->isUndoing;
}

/////////////// Functions related to adding widgets /////////////////////////////////////

void
WidgetPropertySet::setSelectedWidget(QWidget *w, bool add)
{
	if(!w) {
		clearSet();
		return;
	}

	// don't add a widget twice
	if(d->widgets.findRef(w) != -1) {
		kdDebug() << "WidgetPropertySet::setSelectedWidget: Warning: Widget is already selected" << endl;
		return;
	}
	// if our list is empty,don't use add parameter value
	if(d->widgets.count() == 0)
		add = false;

	if(add)
		addWidget(w);
	else {
		clearSet(true); //clear but do not reload to avoid blinking
		d->widgets.append(w);
		createPropertiesForWidget(w);

		w->installEventFilter(this);
		connect(w, SIGNAL(destroyed()), this, SLOT(slotWidgetDestroyed()));
	}

	d->manager->showPropertySet(this, true/*force*/);
}

void
WidgetPropertySet::addWidget(QWidget *w)
{
	d->widgets.append(w);

	// Reset some stuff
	d->lastCommand = 0;
	d->lastGeoCommand = 0;
	d->properties.clear();

	QCString classname;
	if(d->widgets.first()->className() == w->className())
		classname = d->widgets.first()->className();

	// show only properties shared by widget (properties chosed by factory)
	bool isTopLevel = d->manager->isTopLevel(w);
	for(Set::Iterator it(d->set); it.current(); ++it) {
		if(!isPropertyVisible(it.currentKey(), isTopLevel, classname))
			d->set[it.currentKey()].setVisible(false);
	}

	if (d->widgets.count()==2) {
		//second widget, update metainfo
		d->set["this:classString"].setValue(
			i18n("Multiple Widgets") + QString(" (%1)").arg(d->widgets.count()) );
		d->set["this:iconName"].setValue("multiple_obj");
		//name doesn't make sense for now
		d->set["name"].setValue("");
	}
}

void
WidgetPropertySet::createPropertiesForWidget(QWidget *w)
{
	if (!d->manager || !d->manager->activeForm() || !d->manager->activeForm()->objectTree()) {
		kdWarning() << "WidgetPropertySet::createPropertiesForWidget() no manager or active form!!!" << endl;
		return;
	}
	ObjectTreeItem *tree = d->manager->activeForm()->objectTree()->lookup(w->name());
	if(!tree)
		return;

	const QVariantMap* modifiedProperties = tree->modifiedProperties();
	QVariantMapConstIterator modifiedPropertiesIt;
	bool isTopLevel = d->manager->isTopLevel(w);
	int count = 0;
	Property *newProp = 0;
	WidgetInfo *winfo = d->manager->lib()->widgetInfoForClassName(w->className());

	QStrList pList = w->metaObject()->propertyNames(true);
	QStrListIterator it(pList);

	// iterate over the property list, and create Property objects
	for(; it.current() != 0; ++it)  {
		count = w->metaObject()->findProperty(*it, true);
		const QMetaProperty *meta = w->metaObject()->property(count, true);
		const char* propertyName = meta->name();

		if(meta->designable(w) && !d->set.contains(propertyName))  {
			//! \todo add another list for property description
			QString desc( d->propCaption[meta->name()] );
			//! \todo change i18n
			if (desc.isEmpty())  //try to get property description from factory
				desc = d->manager->lib()->propertyDescForName(winfo, propertyName);

			modifiedPropertiesIt = modifiedProperties->find(propertyName);
			const bool oldValueExists = modifiedPropertiesIt!=modifiedProperties->constEnd();

			if(meta->isEnumType()) {
				QStringList keys = QStringList::fromStrList( meta->enumKeys() );
				if(qstrcmp(propertyName, "alignment") == 0)  {
					createAlignProperty(meta, w);
					continue;
				}

				newProp = new Property(propertyName, createValueList(winfo, keys),
					/* assign current or older value */
					oldValueExists ? modifiedPropertiesIt.data() : 
						meta->valueToKey( w->property(propertyName).toInt() ), 
					desc, desc );
				//now set current value, so the old one is stored as old
				if (oldValueExists) {
					newProp->setValue( meta->valueToKey( w->property(propertyName).toInt() ) );
				}
			}
			else {
				newProp = new Property(propertyName, 
					/* assign current or older value */
					oldValueExists ? modifiedPropertiesIt.data() : w->property(propertyName), 
					desc, desc);
				//now set current value, so the old one is stored as old
				if (oldValueExists) {
					newProp->setValue( w->property(propertyName) );
				}
			}

			d->set.addProperty(newProp);
			if(!isPropertyVisible(propertyName, isTopLevel))
				newProp->setVisible(false);
			//! TMP
			if(newProp->type() == 0) // invalid type == null pixmap ?
				newProp->setType(KoProperty::Pixmap);
		}

//		if(0==qstrcmp(propertyName, "name"))
//			(*this)["name"].setAutoSync(0); // name should be updated only when pressing Enter

		// \todo js what does this mean? why do you use WidgetInfo and not WidgetLibrary
		/*if (winfo) {
			tristate autoSync = winfo->autoSyncForProperty( propertyName );
			if (! ~autoSync)
				d->set[propertyName].setAutoSync( autoSync );
		}*/

		// update the Property.oldValue() and isModified() using the value stored in the ObjectTreeItem
		updatePropertyValue(tree, propertyName);
	}

	(*this)["name"].setAutoSync(false); // name should be updated only when pressing Enter
	(*this)["enabled"].setValue( QVariant(tree->isEnabled(), 3));

	if (winfo) {
		d->manager->lib()->setPropertyOptions(*this, *winfo, w);
		//add meta-information
		d->set.addProperty( newProp = new Property("this:classString", winfo->name()) );
		newProp->setVisible(false);
		d->set.addProperty( newProp = new Property("this:iconName", winfo->pixmap()) );
		newProp->setVisible(false);
		d->set.addProperty( newProp = new Property("this:iconName", winfo->pixmap()) );
		newProp->setVisible(false);
	}
	d->set.addProperty( newProp = new Property("this:className", w->className()) ); 
	newProp->setVisible(false);

	/*!  let's forget it for now, until we have new complete events editor
	if (m_manager->lib()->advancedPropertiesVisible()) {
		// add the signals property
		QStrList strlist = w->metaObject()->signalNames(true);
		QStrListIterator strIt(strlist);
		QStringList list;
		for(; strIt.current() != 0; ++strIt)
			list.append(*strIt);
		Property *prop = new Property("signals", i18n("Events")"",
			new KexiProperty::ListData(list, descList(winfo, list)),
			));
	}*/

	if(d->manager->activeForm() && tree->container()) // we are a container -> layout property
		createLayoutProperty(tree);
}

void
WidgetPropertySet::updatePropertyValue(ObjectTreeItem *tree, const char *property)
{
	if (!d->set.contains(property))
		return;
	Property p = d->set[property];

//! \todo what about set properties, and lists properties
	QMap<QString, QVariant>::ConstIterator it( tree->modifiedProperties()->find(property) );
	if (it != tree->modifiedProperties()->constEnd()) {
		blockSignals(true);
		p.setValue(it.data(), false );
		p.setValue(p.value(), true);
		blockSignals(false);
	}
}

bool
WidgetPropertySet::isPropertyVisible(const QCString &property, bool isTopLevel, const QCString &classname)
{
	const bool multiple = d->widgets.count() >= 2;
	if(multiple && classname.isEmpty())
		return false;
/* moved to WidgetLibrary::isPropertyVisible()
	if(d->widgets.count() < 2)
	{
		if(d->properties.isEmpty() && !isTopLevel)
			d->properties << "caption" << "icon" << "sizeIncrement" << "iconText";
		 // don't show these properties for a non-toplevel widget

		if(! (d->properties.grep(property)).isEmpty() )
			return false;
	}
	else
	{
		if(classname.isEmpty())
			return false;

		if(d->properties.isEmpty())  {
			d->properties << "font" << "paletteBackgroundColor" << "enabled" << "paletteForegroundColor"
			   << "cursor" << "paletteBackgroundPixmap";
		} // properties always shown in multiple mode
		if(! (d->properties.grep(property)).isEmpty() )
			return true;
	}
*/

//	return d->manager->lib()->isPropertyVisible(d->widgets.first()->className(), d->widgets.first(),
	return d->manager->lib()->isPropertyVisible(d->widgets.first()->className(), d->widgets.first(),
		 property, multiple, isTopLevel);
}

////////////////  Slots called when properties are modified ///////////////

void
WidgetPropertySet::slotPropertyChanged(KoProperty::Set& set, KoProperty::Property& p)
{
	if(!d->slotPropertyChangedEnabled || !d->manager || !d->manager->activeForm()
		|| ! d->manager->activeForm()->objectTree())
		return;

	QCString property = p.name();
	if (0==property.find("this:"))
		return; //starts with magical prefix: it's meta prop.

	QVariant value = p.value();

	// check if the name is valid (ie is correct identifier) and there is no name conflict
	if(property == "name") {
		if(d->widgets.count()!=1)
			return;
		if(!isNameValid(value.toString()))
			return;
	}
	// a widget with a background pixmap should have its own origin
	else if(property == "paletteBackgroundPixmap") {
		d->set["backgroundOrigin"] = "WidgetOrigin";
	//else if(property == "signals")
	//	return;
	// special types of properties handled separately
	} else if((property == "hAlign") || (property == "vAlign") || (property == "wordbreak")) {
		saveAlignProperty(property);
		return;
	}
	else if((property == "layout") || (property == "layoutMargin") || (property == "layoutSpacing")) {
		saveLayoutProperty(property, value);
		return;
	}
	// we cannot really disable the widget, we just change its color palette
	else if(property == "enabled")  {
		saveEnabledProperty(value.toBool());
		return;
	}

	 // make sure we are not already undoing -> avoid recursion
	if(d->isUndoing)
		return;

	const bool alterLastCommand = d->lastCommand && d->lastCommand->property() == property;

	if(d->widgets.count() == 1) // one widget selected
	{
		// If the last command is the same, we just change its value
		if(alterLastCommand)
			d->lastCommand->setValue(value);
		else  {
//			if(m_widgets.first() && ((m_widgets.first() != m_manager->activeForm()->widget()) || (property != "geometry"))) {
			if (d->slotPropertyChanged_addCommandEnabled) {
				d->lastCommand = new PropertyCommand(this, d->widgets.first()->name(),
						d->widgets.first()->property(property), value, property);
				d->manager->activeForm()->addCommand(d->lastCommand, false);
			}

			// If the property is changed, we add it in ObjectTreeItem modifProp
			ObjectTreeItem *tree = d->manager->activeForm()->objectTree()->lookup(d->widgets.first()->name());
			if (tree && p.isModified())
				tree->addModifiedProperty(property, d->widgets.first()->property(property));
		}

			if(property == "name")
				emit widgetNameChanged(d->widgets.first()->name(), p.value().toCString());
			d->widgets.first()->setProperty(property, value);
			emit widgetPropertyChanged(d->widgets.first(), property, value);
	}
	else
	{
		if(alterLastCommand)
			d->lastCommand->setValue(value);
		else {
			if (d->slotPropertyChanged_addCommandEnabled) {
				// We store old values for each widget
				QMap<QCString, QVariant> list;
				for(QWidget *w = d->widgets.first(); w; w = d->widgets.next())
					list.insert(w->name(), w->property(property));

				d->lastCommand = new PropertyCommand(this, list, value, property);
				d->manager->activeForm()->addCommand(d->lastCommand, false);
			}
		}

			for(QWidget *w = d->widgets.first(); w; w = d->widgets.next())
			{
				if (!alterLastCommand) {
					ObjectTreeItem *tree = d->manager->activeForm()->objectTree()->lookup(w->name());
					if(tree && p.isModified())
						tree->addModifiedProperty(property, w->property(property));
				}
				w->setProperty(property, value);
				emit widgetPropertyChanged(w, property, value);
			}
		
	}
}

//const QCString& propertyName, 
//	const QVariant& propertyValue)

void
WidgetPropertySet::setPropertyValueInDesignMode(QWidget* widget, 
	const QMap<QCString, QVariant> &propValues, const QString& commandName)
{
	if (!widget || propValues.isEmpty())
		return;
	
	//is this widget is selected? (if so, use property system)
	const bool widgetIsSelected = d->manager->activeForm()->selectedWidget() == widget;

	d->slotPropertyChanged_addCommandEnabled = false;
	QMap<QCString, QVariant>::ConstIterator endIt = propValues.constEnd();
	CommandGroup *group = new CommandGroup(commandName);
	for(QMap<QCString, QVariant>::ConstIterator it = propValues.constBegin(); it != endIt; ++it)
	{
		if (!d->set.contains(it.key()))
			continue;
		PropertyCommand *subCommand = new PropertyCommand(this, widget->name(),
					widget->property(it.key()), it.data(), it.key());
		group->addCommand( subCommand );
		if (widgetIsSelected) {
			d->set[it.key()].setValue(it.data());
		}
		else {
			if (-1!=widget->metaObject()->findProperty(it.key(), true) && widget->property(it.key())!=it.data()) {
				ObjectTreeItem *tree = d->manager->activeForm()->objectTree()->lookup(widget->name());
				if (tree)
					tree->addModifiedProperty(it.key(), widget->property(it.key()));
				widget->setProperty(it.key(), it.data());
				emit widgetPropertyChanged(widget, it.key(), it.data());
			}
		}
	}
	d->lastCommand = 0;
	d->manager->activeForm()->addCommand(group, false/*no exec*/);
	d->slotPropertyChanged_addCommandEnabled = true;
//	}
}

//! \todo make it support undo
void
WidgetPropertySet::saveEnabledProperty(bool value)
{
	for(QWidget *w = d->widgets.first(); w; w = d->widgets.next()) {
		ObjectTreeItem *tree = d->manager->activeForm()->objectTree()->lookup(w->name());
		if(tree->isEnabled() == value)
			continue;

		QPalette p = w->palette();
		if (!d->origActiveColors)
			d->origActiveColors = new QColorGroup( p.active() );
		if (value) {
			if (d->origActiveColors)
				p.setActive( *d->origActiveColors ); //revert
		}
		else {
			QColorGroup cg = p.disabled();
			//also make base color a bit disabled-like
			cg.setColor(QColorGroup::Base, cg.color(QColorGroup::Background));
			p.setActive(cg);
		}
		w->setPalette(p);

		tree->setEnabled(value);
		emit widgetPropertyChanged(w, "enabled", QVariant(value, 3));
	}
}

bool
WidgetPropertySet::isNameValid(const QString &name)
{
	//! \todo add to undo buffer
	QWidget *w = d->widgets.first();
	//also update widget's name in QObject member
	if (!KexiUtils::isIdentifier(name)) {
		KMessageBox::sorry(d->manager->activeForm()->widget(),
			i18n("Could not rename widget. ")
			+i18n("\"%1\" is not a valid name for a widget.\n").arg(name)
			+i18n("Its name will be reverted to \"%1\"").arg(w->name()));
		d->slotPropertyChangedEnabled = false;
		d->set["name"].resetValue();
		d->slotPropertyChangedEnabled = true;
		return false;
	}

	if (d->manager->activeForm()->objectTree()->lookup(name)) {
		KMessageBox::sorry( d->manager->activeForm()->widget(),
			i18n("Could not rename widget. ")
			+i18n("A widget with name \"%1\" already exists.\n").arg(name)
			+i18n("Its name will be reverted to \"%1\"").arg(w->name()));
		d->slotPropertyChangedEnabled = false;
		d->set["name"].resetValue();
		d->slotPropertyChangedEnabled = true;
		return false;
	}

	return true; //ie name is correct
}

void
WidgetPropertySet::slotPropertyReset(KoProperty::Set& set, KoProperty::Property& property)
{
	if(d->widgets.count() < 2)
		return;

	// We use the old value in modifProp for each widget
	for(QWidget *w = d->widgets.first(); w; w = d->widgets.next())  {
		ObjectTreeItem *tree = d->manager->activeForm()->objectTree()->lookup(w->name());
		if(tree->modifiedProperties()->contains(property.name()))
			w->setProperty(property.name(), tree->modifiedProperties()->find(property.name()).data());
	}
}

void
WidgetPropertySet::slotWidgetDestroyed()
{
	//only clear this set if it contains the destroyed widget
	if(d->widgets.findRef(dynamic_cast<const QWidget*>(sender())) != -1) {
		clearSet();
	}
}

bool
WidgetPropertySet::eventFilter(QObject *o, QEvent *ev)
{
	if(o == d->widgets.first() && d->widgets.count() < 2)
	{
		if((ev->type() == QEvent::Resize) || (ev->type() == QEvent::Move))  {
			if(!d->set.contains("geometry"))
				return false;
			if(d->set["geometry"].value() == o->property("geometry")) // to avoid infinite recursion
				return false;

			d->set["geometry"] = static_cast<QWidget*>(o)->geometry();
		}
	}
	else if(d->widgets.count() > 1 && ev->type() == QEvent::Move) // the widget is being moved, we update the property
	{
		if(d->isUndoing)
			return false;

		if(d->lastGeoCommand)
			d->lastGeoCommand->setPos(static_cast<QMoveEvent*>(ev)->pos());
		else  {
			QStringList list;
			for(QWidget *w = d->widgets.first(); w; w = d->widgets.next())
				list.append(w->name());

			d->lastGeoCommand = new GeometryPropertyCommand(this, list, static_cast<QMoveEvent*>(ev)->oldPos());
			if (d->manager->activeForm())
				d->manager->activeForm()->addCommand(d->lastGeoCommand, false);
		}
	}

	return false;
}

// Alignment-related functions /////////////////////////////

void
WidgetPropertySet::createAlignProperty(const QMetaProperty *meta, QWidget *obj)
{
	if (!d->manager->activeForm() || !d->manager->activeForm()->objectTree())
		return;

	QStringList list;
	QString value;
	QStringList keys = QStringList::fromStrList( meta->valueToKeys(obj->property("alignment").toInt()) );

	QStrList *enumKeys = new QStrList(meta->enumKeys());
	QStringList possibleValues = QStringList::fromStrList(*enumKeys);
	delete enumKeys;

	ObjectTreeItem *tree = d->manager->activeForm()->objectTree()->lookup(obj->name());
	bool isTopLevel = d->manager->isTopLevel(obj);

	if(!possibleValues.grep("AlignHCenter").empty())  {
		// Create the horizontal alignment property
		if(!keys.grep("AlignHCenter").isEmpty())
			value = "AlignHCenter";
		else if(!keys.grep("AlignRight").isEmpty())
			value = "AlignRight";
		else if(!keys.grep("AlignLeft").isEmpty())
			value = "AlignLeft";
		else if(!keys.grep("AlignJustify").isEmpty())
			value = "AlignJustify";
		else
			value = "AlignAuto";

		list << "AlignAuto" << "AlignLeft" << "AlignRight" << "AlignHCenter" << "AlignJustify";
		Property *p = new Property("hAlign", createValueList(0, list), value,
			i18n("Translators: please keep this string short (less than 20 chars)", "Hor. Alignment"),
			i18n("Horizontal Alignment"));
		d->set.addProperty(p);
		if(!isPropertyVisible(p->name(), isTopLevel)) {
			p->setVisible(false);
		}
		updatePropertyValue(tree, "hAlign");
		list.clear();
	}

	if(!possibleValues.grep("AlignTop").empty())
	{
		// Create the ver alignment property
		if(!keys.grep("AlignTop").empty())
			value = "AlignTop";
		else if(!keys.grep("AlignBottom").empty())
			value = "AlignBottom";
		else
			value = "AlignVCenter";

		list << "AlignTop" << "AlignVCenter" << "AlignBottom";
		Property *p = new Property("vAlign", createValueList(0, list), value,
			i18n("Translators: please keep this string short (less than 20 chars)", "Ver. Alignment"),
			i18n("Vertical Alignment"));
		d->set.addProperty(p);
		if(!isPropertyVisible(p->name(), isTopLevel)) {
			p->setVisible(false);
		}
		updatePropertyValue(tree, "vAlign");
	}

	if(!possibleValues.grep("WordBreak").empty()
	  && !obj->inherits("QLineEdit") /* QLineEdit doesn't support 'word break' is this generic enough?*/
	) {
		// Create the wordbreak property
		Property *p = new Property("wordbreak", QVariant(false, 3), i18n("Word Break"), i18n("Word Break") );
		d->set.addProperty(p);
		updatePropertyValue(tree, "wordbreak");
	}
}

void
WidgetPropertySet::saveAlignProperty(const QString &property)
{
	if (!d->manager->activeForm())
		return;

	QStrList list;
	if( d->set.contains("hAlign") )
		list.append( d->set["hAlign"].value().toCString() );
	if( d->set.contains("vAlign") )
		list.append( d->set["vAlign"].value().toCString() );
	if( d->set.contains("wordbreak") && d->set["wordbreak"].value().toBool() )
		list.append("WordBreak");

	int count = d->widgets.first()->metaObject()->findProperty("alignment", true);
	const QMetaProperty *meta = d->widgets.first()->metaObject()->property(count, true);
	d->widgets.first()->setProperty("alignment", meta->keysToValue(list));


	ObjectTreeItem *tree = d->manager->activeForm()->objectTree()->lookup(d->widgets.first()->name());
	if(tree && d->set[property.latin1()].isModified())
		tree->addModifiedProperty(property.latin1(), d->set[property.latin1()].oldValue());

	if(d->isUndoing)
		return;

	if(d->lastCommand && d->lastCommand->property() == "alignment")
		d->lastCommand->setValue(meta->keysToValue(list));
	else {
		d->lastCommand = new PropertyCommand(this, d->widgets.first()->name(),
			d->widgets.first()->property("alignment"), meta->keysToValue(list), "alignment");
		d->manager->activeForm()->addCommand(d->lastCommand, false);
	}
}

// Layout-related functions  //////////////////////////

void
WidgetPropertySet::createLayoutProperty(ObjectTreeItem *item)
{
	Container *container = item->container();
	if (!container || !d->manager->activeForm() ||
		!d->manager->activeForm()->objectTree() || !container->widget())
		return;
	// special containers have no 'layout' property, as it should not be changed
	QCString className = container->widget()->className();
	if((className == "HBox") || (className == "VBox") || (className == "Grid"))
		return;

	QStringList list;
	QString value = Container::layoutTypeToString(container->layoutType());

	list << "NoLayout" << "HBox" << "VBox" << "Grid" << "HFlow" << "VFlow";

	Property *p = new Property("layout", createValueList(0, list), value,
		i18n("Container's Layout"), i18n("Container's Layout"));
	p->setVisible( d->manager->lib()->advancedPropertiesVisible() );
	d->set.addProperty(p);

	updatePropertyValue(item, "layout");

	p = new Property("layoutMargin", container->layoutMargin(), i18n("Layout Margin"), i18n("Layout Margin"));
	d->set.addProperty(p);
	updatePropertyValue(item, "layoutMargin");
	if(container->layoutType() == Container::NoLayout)
		p->setVisible(false);

	p = new Property("layoutSpacing", container->layoutSpacing(), i18n("Layout Spacing"), i18n("Layout Spacing"));
	d->set.addProperty(p);
	updatePropertyValue(item, "layoutSpacing");
	if(container->layoutType() == Container::NoLayout)
		p->setVisible(false);

}

void
WidgetPropertySet::saveLayoutProperty(const QString &prop, const QVariant &value)
{
	Container *container=0;
	if(!d->manager->activeForm() || !d->manager->activeForm()->objectTree())
		return;
	ObjectTreeItem *item = d->manager->activeForm()->objectTree()->lookup(d->widgets.first()->name());
	if(!item)
		return;
	container = item->container();

	if(prop == "layout") {
		Container::LayoutType type = Container::stringToLayoutType(value.toString());

		if(d->lastCommand && d->lastCommand->property() == "layout" && !d->isUndoing)
			d->lastCommand->setValue(value);
		else if(!d->isUndoing)  {
			d->lastCommand = new LayoutPropertyCommand(this, d->widgets.first()->name(),
				d->set["layout"].oldValue(), value);
			d->manager->activeForm()->addCommand(d->lastCommand, false);
		}

		container->setLayout(type);
		bool show = (type != Container::NoLayout);
		if(show != d->set["layoutMargin"].isVisible())  {
			d->set["layoutMargin"].setVisible(show);
			d->set["layoutSpacing"].setVisible(show);
			d->manager->showPropertySet(this, true/*force*/);
		}
		return;
	}

	if(prop == "layoutMargin" && container->layout()) {
		container->setLayoutMargin(value.toInt());
		container->layout()->setMargin(value.toInt());
	}
	else if(prop == "layoutSpacing" && container->layout())  {
		container->setLayoutSpacing(value.toInt());
		container->layout()->setSpacing(value.toInt());
	}

	ObjectTreeItem *tree = d->manager->activeForm()->objectTree()->lookup(d->widgets.first()->name());
	if(tree && d->set[ prop.latin1() ].isModified())
		tree->addModifiedProperty(prop.latin1(), d->set[prop.latin1()].oldValue());

	if(d->isUndoing)
		return;

	if(d->lastCommand && (QString(d->lastCommand->property()) == prop))
		d->lastCommand->setValue(value);
	else  {
		d->lastCommand = new PropertyCommand(this, d->widgets.first()->name(),
			d->set[ prop.latin1() ].oldValue(), value, prop.latin1());
		d->manager->activeForm()->addCommand(d->lastCommand, false);
	}
}



////////////////////////////////////////// i18n related functions ////////

void
WidgetPropertySet::initPropertiesDescription()
{
//! \todo perhaps a few of them shouldn't be translated within KFD mode,
//!       to be more Qt Designer friendly?
	d->propCaption["name"] = i18n("Name");
	d->propCaption["caption"] = i18n("Caption");
	d->propCaption["text"] = i18n("Text");
	d->propCaption["paletteBackgroundPixmap"] = i18n("Background Pixmap");
	d->propCaption["enabled"] = i18n("Enabled");
	d->propCaption["geometry"] = i18n("Geometry");
	d->propCaption["sizePolicy"] = i18n("Size Policy");
	d->propCaption["minimumSize"] = i18n("Minimum Size");
	d->propCaption["maximumSize"] = i18n("Maximum Size");
	d->propCaption["font"] = i18n("Font");
	d->propCaption["cursor"] = i18n("Cursor");
	d->propCaption["paletteForegroundColor"] = i18n("Foreground Color");
	d->propCaption["paletteBackgroundColor"] = i18n("Background Color");
	d->propCaption["focusPolicy"] = i18n("Focus Policy");
	d->propCaption["margin"] = i18n("Margin");
	d->propCaption["readOnly"] = i18n("Read Only");
	//any QFrame
	d->propCaption["frame"] = i18n("Frame");
	d->propCaption["lineWidth"] = i18n("Frame Width");
	d->propCaption["midLineWidth"] = i18n("Mid Frame Width");
	d->propCaption["frameShape"] = i18n("Frame Shape");
	d->propCaption["frameShadow"] = i18n("Frame Shadow");
	//any QScrollbar
	d->propCaption["vScrollBarMode"] = i18n("Vertical ScrollBar");
	d->propCaption["hScrollBarMode"] = i18n("Horizontal ScrollBar");

	d->propValCaption["NoBackground"] = i18n("No Background");
	d->propValCaption["PaletteForeground"] = i18n("Palette Foreground");
	d->propValCaption["AutoText"] = i18n("Auto\n\n(HINT: for AutoText)", "Auto");

	d->propValCaption["AlignAuto"] = i18n("Auto\n\n(HINT: for Align)", "Auto");
	d->propValCaption["AlignLeft"] = i18n("Left\n\n(HINT: for Align)", "Left");
	d->propValCaption["AlignRight"] = i18n("Right\n\n(HINT: for Align)", "Right");
	d->propValCaption["AlignHCenter"] = i18n("Center\n\n(HINT: for Align)", "Center");
	d->propValCaption["AlignJustify"] = i18n("Justify\n\n(HINT: for Align)", "Justify");
	d->propValCaption["AlignVCenter"] = i18n("Center\n\n(HINT: for Align)", "Center");
	d->propValCaption["AlignTop"] = i18n("Top\n\n(HINT: for Align)", "Top");
	d->propValCaption["AlignBottom"] = i18n("Bottom\n\n(HINT: for Align)", "Bottom");

	d->propValCaption["NoFrame"] = i18n("No Frame\n\n(HINT: for Frame Shape)", "No Frame");
	d->propValCaption["Box"] = i18n("Box\n\n(HINT: for Frame Shape)", "Box");
	d->propValCaption["Panel"] = i18n("Panel\n\n(HINT: for Frame Shape)", "Panel");
	d->propValCaption["WinPanel"] = i18n("Windows Panel\n\n(HINT: for Frame Shape)", "Windows Panel");
	d->propValCaption["HLine"] = i18n("Horiz. Line\n\n(HINT: for Frame Shape)", "Horiz. Line");
	d->propValCaption["VLine"] = i18n("Vertical Line\n\n(HINT: for Frame Shape)", "Vertical Line");
	d->propValCaption["StyledPanel"] = i18n("Styled\n\n(HINT: for Frame Shape)", "Styled");
	d->propValCaption["PopupPanel"] = i18n("Popup\n\n(HINT: for Frame Shape)", "Popup");
	d->propValCaption["MenuBarPanel"] = i18n("Menu Bar\n\n(HINT: for Frame Shape)", "Menu Bar");
	d->propValCaption["ToolBarPanel"] = i18n("Toolbar\n\n(HINT: for Frame Shape)", "Toolbar");
	d->propValCaption["LineEditPanel"] = i18n("Text Box\n\n(HINT: for Frame Shape)", "Text Box");
	d->propValCaption["TabWidgetPanel"] = i18n("Tab Widget\n\n(HINT: for Frame Shape)", "Tab Widget");
	d->propValCaption["GroupBoxPanel"] = i18n("Group Box\n\n(HINT: for Frame Shape)", "Group Box");

	d->propValCaption["Plain"] = i18n("Plain\n\n(HINT: for Frame Shadow)", "Plain");
	d->propValCaption["Raised"] = i18n("Raised\n\n(HINT: for Frame Shadow)", "Raised");
	d->propValCaption["Sunken"] = i18n("Sunken\n\n(HINT: for Frame Shadow)", "Sunken");
	d->propValCaption["MShadow"] = i18n("for Frame Shadow", "Internal");

	d->propValCaption["NoFocus"] = i18n("No Focus\n\n(HINT: for Focus)", "No Focus");
	d->propValCaption["TabFocus"] = i18n("Tab\n\n(HINT: for Focus)", "Tab");
	d->propValCaption["ClickFocus"] = i18n("Click\n\n(HINT: for Focus)", "Click");
	d->propValCaption["StrongFocus"] = i18n("Tab/Click\n\n(HINT: for Focus)", "Tab/Click");
	d->propValCaption["WheelFocus"] = i18n("Tab/Click/MouseWheel\n\n(HINT: for Focus)", "Tab/Click/MouseWheel");

	d->propValCaption["Auto"] = i18n("Auto");
	d->propValCaption["AlwaysOff"] = i18n("Always Off");
	d->propValCaption["AlwaysOn"] = i18n("Always On");

	//orientation
	d->propValCaption["Horizontal"] = i18n("Horizontal");
	d->propValCaption["Vertical"] = i18n("Vertical");
}

QString
WidgetPropertySet::propertyCaption(const QCString &name)
{
	return d->propCaption[name];
}

QString
WidgetPropertySet::valueCaption(const QCString &name)
{
	return d->propValCaption[name];
}

KoProperty::Property::ListData*
WidgetPropertySet::createValueList(WidgetInfo *winfo, const QStringList &list)
{
//	QMap <QString, QVariant> map;
	QStringList names;
	QStringList::ConstIterator endIt = list.end();
	for(QStringList::ConstIterator it = list.begin(); it != endIt; ++it) {
		QString n( d->propValCaption[ (*it).latin1() ] );
		if (n.isEmpty()) { //try within factory and (maybe) parent factory
			if (winfo)
				n = d->manager->lib()->propertyDescForValue( winfo, (*it).latin1() );
			if (n.isEmpty())
				names.append( *it ); //untranslated
//				map.insert(*it, (*it).latin1()); //untranslated
			else
				names.append( n );
//				map.insert(*it, n);
		}
		else
			names.append( n );
//			map.insert(*it, n);
	}
	return new KoProperty::Property::ListData(list, names);
}

void
WidgetPropertySet::addPropertyCaption(const QCString &property, const QString &caption)
{
	if(!d->propCaption.contains(property))
		d->propCaption[property] = caption;
}

void
WidgetPropertySet::addValueCaption(const QCString &value, const QString &caption)
{
	if(!d->propValCaption.contains(value))
		d->propValCaption[value] = caption;
}

#include "widgetpropertyset.moc"
