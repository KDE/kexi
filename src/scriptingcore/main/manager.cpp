/***************************************************************************
 * manager.cpp
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
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 ***************************************************************************/

#include "manager.h"

#include "../api/interpreter.h"
//#include "../api/qtobject.h"
#include "../api/eventslot.h"
#include "../api/eventsignal.h"
//#include "../api/script.h"

#include "krossconfig.h"
#include "scriptcontainer.h"

#include <qobject.h>
#include <kdebug.h>
#include <klibloader.h>

extern "C"
{
    typedef Kross::Api::Object* (*def_module_func)(Kross::Api::Manager*);
}

using namespace Kross::Api;

namespace Kross { namespace Api {

    /// @internal
    class ManagerPrivate
    {
        public:
            /// List of \a InterpreterInfo instances.
            QMap<QString, InterpreterInfo*> interpreterinfos;

            /// Loaded modules.
            QMap<QString, Module::Ptr> modules;
    };

}}

Manager::Manager()
    : MainModule("Kross") // the manager has the name "Kross"
    , d( new ManagerPrivate() )
{
#ifdef KROSS_PYTHON_LIBRARY
    InterpreterInfo::Option::Map pythonoptions;
    pythonoptions.replace("restricted",
        new InterpreterInfo::Option("Restricted", "Restricted Python interpreter", QVariant(false))
    );
    d->interpreterinfos.replace("python",
        new InterpreterInfo("python",
            KROSS_PYTHON_LIBRARY, // library
            QStringList() << "text/x-python" << "application/x-python", // mimetypes
            pythonoptions // options
        )
    );
#endif
}

Manager::~Manager()
{
    for(QMap<QString, InterpreterInfo*>::Iterator it = d->interpreterinfos.begin(); it != d->interpreterinfos.end(); ++it)
        delete it.data();
    delete d;
}

QMap<QString, InterpreterInfo*> Manager::getInterpreterInfos()
{
    return d->interpreterinfos;
}

InterpreterInfo* Manager::getInterpreterInfo(const QString& interpretername)
{
    return d->interpreterinfos[interpretername];
}

ScriptContainer::Ptr Manager::getScriptContainer(const QString& scriptname)
{
    if(hadException())
        return 0;

    //TODO at the moment we don't share ScriptContainer instances.

    //if(d->m_scriptcontainers.contains(scriptname))
    //    return d->m_scriptcontainers[scriptname];
    ScriptContainer* scriptcontainer = new ScriptContainer(scriptname);
    //ScriptContainer script(this, scriptname);
    //d->m_scriptcontainers.replace(scriptname, scriptcontainer);

    return scriptcontainer;
}

Interpreter* Manager::getInterpreter(const QString& interpretername)
{
    setException(0); // clear previous exceptions

    if(! d->interpreterinfos.contains(interpretername)) {
        setException( new Exception(QString("No such interpreter '%1'").arg(interpretername)) );
        return 0;
    }

    return d->interpreterinfos[interpretername]->getInterpreter();
}

const QStringList Manager::getInterpreters()
{
    QStringList list;

    QMap<QString, InterpreterInfo*>::Iterator it( d->interpreterinfos.begin() );
    for(; it != d->interpreterinfos.end(); ++it)
        list << it.key();

list << "TestCase";

    return  list;
}

bool Manager::addModule(Module* module)
{
    QString name = module->getName();
    if( d->modules.contains(name) )
        return false;
    d->modules.replace(name, module);
    return true;
}

Module* Manager::loadModule(const QString& modulename)
{
    Module* module = 0;
    if(d->modules.contains(modulename)) {
        module = d->modules[modulename];
        if(module)
            return module;
        else
            kdDebug() << QString("Manager::loadModule(%1) =======> Modulename registered, but module is invalid!").arg(modulename) << endl;
    }

    KLibLoader* loader = KLibLoader::self();
    KLibrary* lib = loader->globalLibrary(modulename.latin1());
    if(! lib) {
        kdWarning() << QString("Failed to load module '%1': %2").arg(modulename).arg(loader->lastErrorMessage()) << endl;
        return 0;
    }
    kdDebug() << QString("Successfully loaded module '%1'").arg(modulename) << endl;

    def_module_func func;
    func = (def_module_func) lib->symbol("init_module");

    if(! func) {
        kdWarning() << QString("Failed to determinate init function in module '%1'").arg(modulename) << endl;
        return 0;
    }

    module = (Kross::Api::Module*) (func)(this);
    lib->unload();

    if(! module) {
        kdWarning() << QString("Failed to load module '%1'").arg(modulename) << endl;
        return 0;
    }

    //kdDebug() << "Kross::Api::Manager::loadModule " << module->toString() << endl;
    d->modules.replace(modulename, module);
    return module;
}

