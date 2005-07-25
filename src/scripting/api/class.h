/***************************************************************************
 * class.h
 * This file is part of the KDE project
 * copyright (C)2004-2005 by Sebastian Sauer (mail@dipe.org)
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
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 ***************************************************************************/

#ifndef KROSS_API_CLASS_H
#define KROSS_API_CLASS_H

#include <qstring.h>
//#include <qvaluelist.h>
//#include <qmap.h>
#include <qobject.h>
//#include <klocale.h>
#include <kdebug.h>

//#include "../main/krossconfig.h"
#include "object.h"
#include "event.h"
#include "list.h"
#include "exception.h"
#include "argument.h"
#include "variant.h"

namespace Kross { namespace Api {

    /**
     * From \a Event inherited template-class to represent
     * class-structures. Classes implemetating this template
     * are able to dynamicly define \a Event methodfunctions
     * accessible from within scripts.
     */
    template<class T>
    class Class : public Event<T>
    {
        public:

            /**
             * Constructor.
             *
             * \param name The name this class has.
             * \param parent The parent this class is child of
             *        or NULL if this class has no parent.
             */
            explicit Class(const QString& name, Object::Ptr parent = 0, const QString& documentation = QString::null)
                : Event<T>(name, parent, documentation)
            {
            }

            /**
             * Destructor.
             */
            virtual ~Class()
            {
            }

            /*
            virtual Object::Ptr call(const QString& name, List::Ptr arguments)
            {
                kdDebug() << QString("Class::call(%1)").arg(name) << endl;
                return Event<T>::call(name, arguments);
            }
            */

    };

}}

#endif

