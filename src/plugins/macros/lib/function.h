/***************************************************************************
 * This file is part of the KDE project
 * copyright (C) 2005 by Sebastian Sauer (mail@dipe.org)
 * copyright (C) 2005 by Tobi Krebs (tobi.krebs@gmail.com)
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

#ifndef KOMACRO_FUNCTION_H
#define KOMACRO_FUNCTION_H

#include <qobject.h>

#include "action.h"
#include "metaobject.h"

// Forward declarations.
class QDomElement;

namespace KoMacro {

	class Manager;

	/**
	* The Function class implements a callable function with optional
	* arguments and a returnvalue.
	*/
	class KOMACRO_EXPORT Function : public Action
	{
			Q_OBJECT

		public:

			/**
			* Constructor.
			*
			* @param name The name this @a Function has.
			*/
			Function(const QString& name);

			/**
			* Destructor.
			*/
			virtual ~Function();

			/**
			* @return a string-representation of the function.
			*/
			virtual const QString toString() const;

			/**
			* @return the name of the receiver object.
			*/
			const QString receiver() const;

			/**
			* @return the name of the slot.
			*/
			const QString slot() const;

			/**
			* @return the @a MetaObject receiver object or NULL if there
			* is no receiver object defined.
			*/
			KSharedPtr<MetaObject> receiverObject();

			/**
			* Set the receiver object to \p metaobject .
			*/
			void setReceiverObject(KSharedPtr<MetaObject> metaobject);

			//KSharedPtr<MetaMethod> receiverMethod();
			//void setReceiverMethod(KSharedPtr<MetaMethod>);

		public slots:

			/**
			* Called if the @a Function should be executed within
			* the context \p context .
			*/
			virtual void activate(KSharedPtr<Context> context);

		signals:
			void activated();

		private:
			/// @internal d-pointer class.
			class Private;
			/// @internal d-pointer instance.
			Private* const d;
	};

}

#endif
