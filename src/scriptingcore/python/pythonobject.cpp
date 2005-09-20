/***************************************************************************
 * pythonobject.cpp
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

#include "pythonobject.h"
#include "pythonextension.h"
//#include "pythoninterpreter.h"

#include <kdebug.h>

using namespace Kross::Python;

PythonObject::PythonObject(const Py::Object& object)
    : Kross::Api::Object(object.as_string().c_str(), 0)
    , m_pyobject(object)
{
    kdDebug() << QString("PythonObject::PythonObject() constructor") << endl;

    Py::List x( object.dir() );
    for(Py::Sequence::iterator i= x.begin(); i != x.end(); ++i) {
        std::string s = (*i).str();
        if(s == "__init__")
            continue;

        //if(! m_pyobject.hasAttr( (*i).str() )) continue;
        Py::Object o = m_pyobject.getAttr(s);

        QString t;
        if(o.isCallable()) t += "isCallable ";
        if(o.isDict()) t += "isDict ";
        if(o.isList()) t += "isList ";
        if(o.isMapping()) t += "isMapping ";
        if(o.isNumeric()) t += "isNumeric ";
        if(o.isSequence()) t += "isSequence ";
        if(o.isTrue()) t += "isTrue ";
        if(o.isInstance()) t += "isInstance ";
        kdDebug() << QString("PythonObject::PythonObject() method '%1' (%2)").arg( (*i).str().as_string().c_str() ).arg(t) << endl;

        if(o.isCallable())
            m_calls.append( (*i).str().as_string().c_str() );
    }
}

PythonObject::~PythonObject()
{
}

const QString PythonObject::getClassName() const
{
    return "Kross::Python::PythonObject";
}

Kross::Api::Object::Ptr PythonObject::call(const QString& name, Kross::Api::List::Ptr arguments)
{
    kdDebug() << QString("PythonObject::call(%1)").arg(name) << endl;

    if(m_pyobject.isInstance()) {
        //if(! m_calls.contains(n)) throw ...

        char* n = const_cast<char*>( name.latin1() );
        PyObject* r = PyObject_CallMethod(m_pyobject.ptr(), n, 0);
        if(! r) { //FIXME happens too if e.g. number of arguments doesn't match !!!
            Py::Object errobj = Py::value(Py::Exception()); // get last error
            throw Kross::Api::Exception::Ptr( new Kross::Api::Exception(QString("Failed to call method '%1': %2").arg(name).arg(errobj.as_string().c_str())) );
        }
        Py::Object result(r, true);

        //kdDebug() << QString("PythonObject::call(%1) call return value = '%2'").arg(name).arg(result.as_string().c_str()) << endl;
        return PythonExtension::toObject(result);
    }
    /*TODO??? ELSE create class instance for class-definitions???
    Kross::Api::ClassBase* clazz = new Kross::Api::ClassBase("", this);
    return new PythonExtension(clazz);
    */

    return Kross::Api::Object::call(name, arguments);
}

QStringList PythonObject::getCalls()
{
    return m_calls;
}

