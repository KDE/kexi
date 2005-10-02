/***************************************************************************
 * manager.h
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

#ifndef KROSS_API_MANAGER_H
#define KROSS_API_MANAGER_H

#include <qstring.h>
#include <qstringlist.h>
#include <qmap.h>
//#include <qvariant.h>
//#include <kdebug.h>
#include <ksharedptr.h>

class QObject;

#include "../api/object.h"
#include "mainmodule.h"

namespace Kross { namespace Api {

    // Forward declarations.
    class Interpreter;
    class Object;
    class EventSlot;
    class EventSignal;
    class ScriptContainer;
    class ManagerPrivate;
    class InterpreterInfo;

    /**
     * The Manager class is the main entry point to work with
     * Kross. It spends an abstraction layer between what is
     * under the hood of Kross and the functionality you need
     * to access.
     * Use \a Interpreter to just work with some implementated
     * interpreter like python. While \a Script spends a more
     * flexible container.
     */
    class KROSS_MAIN_EXPORT Manager : public MainModule
    {
        protected:

            /**
             * Constructor. Use \a scriptManager() to access
             * the Manager singleton instance.
             */
            Manager();

        public:

            /**
             * Destructor.
             */
            ~Manager();

            /**
             * Return the Manager instance. Always use this
             * function to access the Manager singleton.
             */
            static Manager* scriptManager() {
                static Manager m_manager = Manager();
                return &m_manager;
            }

            /**
             * \return a map with \a InterpreterInfo* instances
             * used to describe interpreters.
             */
            QMap<QString, InterpreterInfo*> getInterpreterInfos();

            /**
             * \return the \a InterpreterInfo* matching to the defined
             * \p interpretername or NULL if there does not exists such
             * a interpreter.
             */
            InterpreterInfo* getInterpreterInfo(const QString& interpretername);

            /**
             * Return the existing \a ScriptContainer with scriptname
             * or create a new \a ScriptContainer instance and associate
             * the passed scriptname with it.
             *
             * \param scriptname The name of the script. This
             *        should be unique for each \a Script and
             *        could be something like the filename.
             * \return The \a ScriptContainer instance matching to
             *         scriptname.
             */
            KSharedPtr<ScriptContainer> getScriptContainer(const QString& scriptname);

            /**
             * Return the \a Interpreter instance defined by
             * the interpretername.
             *
             * \param interpretername The name of the interpreter.
             *        e.g. "python" or "kjs".
             * \return The Interpreter instance or NULL if there
             *         does not exists an interpreter with such
             *         an interpretername.
             */
            Interpreter* getInterpreter(const QString& interpretername);

            /**
             * \return a list of names of the at the backend
             * supported interpreters.
             */
            const QStringList getInterpreters();

            /**
             * Load an external module and add it as child to
             * the manager. The given \p modulename is the
             * name of the library we should try to load. Those
             * library needs to be a valid kross module.
             *
             * \param modulename The name of the module to load.
             * \return The loaded \a Object or NULL if loading
             *        failed.
             */
            Module* loadModule(const QString& modulename);

        private:
            /// Private d-pointer class.
            ManagerPrivate* d;
    };

}}

#endif

