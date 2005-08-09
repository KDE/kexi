/***************************************************************************
 * object.cpp
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

#include "object.h"
#include "list.h"
#include "variant.h"
#include "event.h"
#include "exception.h"

#include <klocale.h>
#include <kdebug.h>

using namespace Kross::Api;

Object::Object(const QString& name, Object::Ptr parent)
    : KShared()
    , m_name(name)
    , m_parent(parent)
{
#ifdef KROSS_API_OBJECT_CTOR_DEBUG
    kdDebug() << QString("Kross::Api::Object::Constructor() name='%1' refcount='%2'").arg(m_name).arg(_KShared_count()) << endl;
#endif
}

Object::~Object()
{
#ifdef KROSS_API_OBJECT_DTOR_DEBUG
    kdDebug() << QString("Kross::Api::Object::Destructor() name='%1' refcount='%2'").arg(m_name).arg(_KShared_count()) << endl;
#endif
    //removeAllChildren(); // not needed cause we use KShared to handle ref-couting and freeing.
}

const QString& Object::getName() const
{
    return m_name;
}

const QString Object::toString()
{
    return QString("%1 (%2)").arg(m_name).arg(getClassName());
}

Object::Ptr Object::getParent() const
{
    return m_parent;
}

bool Object::hasChild(const QString& name) const
{
    return m_children.contains(name);
}

Object::Ptr Object::getChild(const QString& name) const
{
    return m_children[name];
}

QMap<QString, Object::Ptr> Object::getChildren() const
{
    return m_children;
}

bool Object::addChild(Object::Ptr object)
{
    QString name = object->getName();
#ifdef KROSS_API_OBJECT_ADDCHILD_DEBUG
    kdDebug() << QString("Kross::Api::Object::addChild() object.name='%2' object.classname='%3'")
        .arg(name).arg(object->getClassName()) << endl;
#endif

    if(m_children.contains(name))
        return false;
    object->m_parent = this;
    m_children.replace(name, object);
    return true;
}

void Object::removeChild(const QString& name)
{
#ifdef KROSS_API_OBJECT_REMCHILD_DEBUG
    kdDebug() << QString("Kross::Api::Object::removeChild() name='%1'").arg(name) << endl;
#endif
    m_children.remove(name);
}

void Object::removeAllChildren()
{
#ifdef KROSS_API_OBJECT_REMCHILD_DEBUG
    kdDebug() << "Kross::Api::Object::removeAllChildren()" << endl;
#endif
    m_children.clear();
}

Object::Ptr Object::call(const QString& name, List::Ptr arguments)
{
#ifdef KROSS_API_OBJECT_CALL_DEBUG
    kdDebug() << QString("Kross::Api::Object::call(%1) name=%2 class=%3").arg(name).arg(getName()).arg(getClassName()) << endl;
#endif

    if(name.isEmpty()) // return a self-reference if no functionname is defined.
        return this;

    // if name is defined try to get the matching child and pass the call to it.
    Object::Ptr object = getChild(name);
    if(object) {
        //FIXME namespace, e.g. "mychild1.mychild2.myfunction"
        return object->call(name, arguments);
    }

    // If there exists no such object throw an exception.
    throw new Exception(i18n("Object '%1' has no function named '%2'.").arg(getName()).arg(name));
}

