/***************************************************************************
 * krossconfig.h
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

#ifndef KROSS_MAIN_KROSSCONFIG_H
#define KROSS_MAIN_KROSSCONFIG_H

/**
 * The Kross scripting bridge to embed scripting functionality
 * into an application.
 *
 * - abstract API to access the scripting functionality.
 * - interpreter independend to be able to decide on runtime
 *   if we like to use the python, kjs (KDE JavaScript) or
 *   whatever scripting interpreter.
 * - flexibility by beeing able to connect different
 *   scripting interpreters together into something like
 *   a "working chain" (e.g. python-script script1 spends
 *   some functionality the kjs-script script2 likes to
 *   use.
 * - transparently bridge functionality wrappers like
 *   \a Kross::KexiDB together with interpreters like \a Kross::Python.
 * - Introspection where needed to be able to manipulate
 *   behaviours and functionality on runtime.
 * - Qt/KDE based, so use the extended techs both spends.
 * - integrate nicly as powerfull scripting system into the
 *   Kexi application.
 *
 * \author Sebastian Sauer
 * \sa http://www.koffice.org/kexi
 * \sa http://www.dipe.org/kross
 */
namespace Kross {

    /**
     * The common Kross API used as common codebase.
     *
     * The API spends \a Kross::Api::Object and more specialized
     * classes to bridge other Kross parts together. Interaction
     * between objects got wrapped at runtime and introspection-
     * functionality enables dynamic manipulations.
     * The proxy functionality prevents cross-dependencies
     * between Kross parts like the \a Kross::Python implementation
     * and the \a Kross::KexiDB wrapper.
     *
     * \author Sebastian Sauer
     */
    namespace Api {

        //#define KROSS_API_OBJECT_CTOR_DEBUG
        //#define KROSS_API_OBJECT_DTOR_DEBUG
        //#define KROSS_API_OBJECT_ADDCHILD_DEBUG
        //#define KROSS_API_OBJECT_REMCHILD_DEBUG
        //#define KROSS_API_OBJECT_CALL_DEBUG

        //#define KROSS_API_EVENT_CALL_DEBUG

        #define KROSS_API_CALLABLE_CALL_DEBUG
        #define KROSS_API_CALLABLE_CHECKARG_DEBUG

        //#define KROSS_API_EVENTSLOT_CALL_DEBUG
    }

    /**
     * Wrapper around the Kexi::KexiDB classes.
     *
     * Only \a Kross::Api is used. So, this wrapper is independend
     * to the used scripting backend and any interpreter who wraps
     * the Kross-Api should be able to make such wrappers accessible
     * from within scripting-languages.
     *
     * \author Sebastian Sauer
     */
    namespace KexiDB {
    }

    // The name of the global krosspyhon library. Loaded dynamicly
    // during runtime. Comment out to disable the usage or for
    // developing refer to your not jet installed lib like I did at
    // the following line.
    //#define KROSS_PYTHON_LIBRARY "/home/snoopy/cvs/kde/head/koffice/kexi/scripting/python/krosspython.la"
    #define KROSS_PYTHON_LIBRARY "krosspython"

}

#endif

