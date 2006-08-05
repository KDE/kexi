/***************************************************************************
 * This file is part of the KDE project
 * copyright (C) 2006 by Sebastian Sauer (mail@dipe.org)
 * copyright (C) 2006 by Sascha Kupper (kusato@kfnv.de)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 ***************************************************************************/

#include "macroitem.h"

#include <kdebug.h>

using namespace KoMacro;

namespace KoMacro {

	/**
	* @internal d-pointer class to be more flexible on future extension of the
	* functionality without to much risk to break the binary compatibility.
	*/
	class MacroItem::Private
	{
		public:
			/**
			* The @a Action this @a MacroItem has.
			*/
			KSharedPtr<Action> action;
			
			/**
			* The comment this @a MacroItem has.
			*/
			QString comment;
			
			/**
			* The @a QMap of @a Variable this @a MacroItem has.
			*/
			Variable::Map variables;

			/** 
			* define a @a QVariant -cast as inline for better performance
			* @return the casted @a QVariant by passing a @param variant and its
			* expected QVariant::Type @param type.
			*/
			inline const QVariant cast(const QVariant& variant, QVariant::Type type) const
			{
				// If ok is true the QVariant v holds our new and to the correct type
				// casted variant value. If ok is false the as argument passed variant
				// QVariant contains the (maybe uncasted string to prevent data-loosing
				// what would happen if we e.g. would expect an integer and cast it to
				// an incompatible non-int string) value.
				bool ok = false;
				QVariant v;

				// Try to cast the passed variant to the expected variant-type.
				switch(type) {
					case QVariant::Bool: {
						const QString s = variant.toString();
						ok = (s == "true" || s == "false" || s == "0" || s == "1" || s == "-1");
						v = QVariant( variant.toBool(), 0 );
					} break;
					case QVariant::Int: {
						v = variant.toInt(&ok);
						// Check if the cast is correct.
						Q_ASSERT(!ok || v.toString() == variant.toString());
					} break;
					case QVariant::UInt: {
						v = variant.toUInt(&ok); 
						Q_ASSERT(!ok || v.toString() == variant.toString());
					} break;
					case QVariant::LongLong: {
						v = variant.toLongLong(&ok); 
						Q_ASSERT(!ok || v.toString() == variant.toString());
					} break;
					case QVariant::ULongLong: {
						v = variant.toULongLong(&ok); 
						Q_ASSERT(!ok || v.toString() == variant.toString());
					} break;
					case QVariant::Double: {
						v = variant.toDouble(&ok);
						Q_ASSERT(!ok || v.toString() == variant.toString());
					} break;
					case QVariant::String: {
						ok = true; // cast will always be successfully
						v = variant.toString();
					} break;
					default: {
						// If we got another type we try to let Qt handle it...
						ok = v.cast(type);
						kdWarning()<<"MacroItem::Private::cast() Unhandled ok="<<ok<<" type="<<type<<" value="<<v<<endl;
					} break;
				}

				return ok ? v : variant;
			}

	};

}

MacroItem::MacroItem()
	: KShared()
	, d( new Private() )
{
}

MacroItem::~MacroItem()
{
	delete d;
}

QString MacroItem::comment() const
{
	return d->comment;
}

void MacroItem::setComment(const QString& comment)
{
	d->comment = comment;
}

KSharedPtr<Action> MacroItem::action() const
{
	return d->action;
}

void MacroItem::setAction(KSharedPtr<Action> action)
{
	d->action = action;
}

QVariant MacroItem::variant(const QString& name, bool checkaction) const
{
	KSharedPtr<Variable> v = variable(name, checkaction);
	return v.data() ? v->variant() : QVariant();
}

KSharedPtr<Variable> MacroItem::variable(const QString& name, bool checkaction) const
{
	if(d->variables.contains(name))
		return d->variables[name];
	if(checkaction && d->action.data())
		return d->action->variable(name);
	return KSharedPtr<Variable>(0);
}

Variable::Map MacroItem::variables() const
{
	return d->variables;
}

bool MacroItem::setVariant(const QString& name, const QVariant& variant)
{
	// Let's look if there is an action defined for the variable. If that's
	// the case, we try to use that action to preserve the type of the variant.
	KSharedPtr<Variable> actionvariable = d->action ? d->action->variable(name) : KSharedPtr<Variable>(0);

	// If we know the expected type, we try to cast the variant to the expected
	// type else the variant stays untouched (so, it will stay a string).
	const QVariant v = actionvariable.data()
		? d->cast(variant, actionvariable->variant().type()) // try to cast the variant
		: variant; // don't cast anything, just leave the string-type...

	// Now let's try to determinate the variable which should be changed.
	KSharedPtr<Variable> variable = d->variables[name];
	if(! variable.data()) {
		// if there exists no such variable yet, create one.
		kdDebug() << "MacroItem::setVariable() Creating new variable name=" << name << endl;

		variable = KSharedPtr<Variable>( new Variable() );
		variable->setName(name);
		d->variables.replace(name, variable);
	}

	// Remember the previous value for the case we like to restore it.
	const QVariant oldvar = variable->variant();

	// Set the variable.
	variable->setVariant(v);

	// Now we inform the referenced action that a variable changed. If
	// notifyUpdated() returns false, the action rejects the new variable
	// and we need to restore the previous value.
	if(d->action && ! d->action->notifyUpdated(this, name)) {
		kdWarning() << "MacroItem::setVariable() Notify failed for variable name=" << name << endl;
		variable->setVariant(oldvar);
		return false; // the action rejected the changed variable whyever...
	}

	// Job done successfully. The variable is changed to the new value.
	return true;
}

KSharedPtr<Variable> MacroItem::addVariable(const QString& name, const QVariant& variant)
{
	Q_ASSERT(! d->variables.contains(name) );
	// Create a new Variable.
	KSharedPtr<Variable> variable = KSharedPtr<Variable>( new Variable() );
	variable->setName(name);
	
	// Put it into the Variable-map.
	d->variables.replace(name, variable);
	
	// Set the variant of the Variable.
	this->setVariant(name, variant);
	return variable;
}
