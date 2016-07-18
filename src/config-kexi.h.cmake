/* This file is part of the KDE project
   Copyright (C) 2006-2016 Jarosław Staniek <staniek@kde.org>

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
 * Boston, MA 02110-1301, USA.
*/

#ifndef KEXI_CONFIG_H
#define KEXI_CONFIG_H

/* config-kexi.h. Generated by cmake from config-kexi.h.cmake */

/*! @file config-kexi.h
    Global Kexi configuration (build time)
*/

#include <config-kdb.h>

//! @def KEXI_MOBILE
//! @brief If defined, a mobile version of Kexi if compiled
#cmakedefine KEXI_MOBILE

/* define if you have libreadline available */
/* TODO: detect #define HAVE_READLINE 1 */

//! @def HAVE_UNAME
//! @brief If defined, uname(2) is available
#cmakedefine HAVE_UNAME 1

/*! For KexiUtils::encoding() */
#cmakedefine01 HAVE_LANGINFO_H

//! @def HAVE_KCRASH
//! @brief if defined, KCrash is available
#cmakedefine HAVE_KCRASH

//! @def HAVE_MARBLE
//! @brief if defined, Marble widget library is available
#cmakedefine HAVE_MARBLE

//! @def HAVE_SETMARBLEWIDGET
//! @brief if defined, Marble widget library has setMarbleWidget()
#cmakedefine HAVE_SETMARBLEWIDGET

//! @def HAVE_SETMARBLEWIDGET
//! @brief if defined, QtWebKit widgets library is available
#cmakedefine HAVE_QTWEBKITWIDGETS

//! @def COMPILING_TESTS
//! @brief if defined, tests are enabled
#cmakedefine COMPILING_TESTS

//! @def KEXI_DEBUG_GUI
//! @brief If defined, a debugging GUI for Kexi is enabled
#cmakedefine KEXI_DEBUG_GUI

#if defined KEXI_DEBUG_GUI && !defined KDB_DEBUG_GUI
# error KEXI_DEBUG_GUI requires a KDB_DEBUG_GUI cmake option to be set too in KDb.
#endif

//! @def KEXI_MIGRATEMANAGER_DEBUG
//! @brief Defined if debugging for the migrate driver manager is enabled
#cmakedefine KEXI_MIGRATEMANAGER_DEBUG

/* -- Experimental -- */

//! @def KEXI_SCRIPTS_SUPPORT
//! @brief If defined, scripting GUI plugin is enabled in Kexi
#cmakedefine KEXI_SCRIPTS_SUPPORT

//! @def KEXI_MACROS_SUPPORT
//! @brief If defined, macro GUI plugin is enabled in Kexi
#cmakedefine KEXI_MACROS_SUPPORT

//! @def KEXI_SHOW_UNFINISHED
//! @brief If defined unfinished features are enabled and presented in Kexi.
//! This is useful for testing but may confuse end-users.
#cmakedefine KEXI_SHOW_UNFINISHED

//! @def KEXI_SHOW_UNIMPLEMENTED
//! @brief If defined show menu entries and dialogs just to give impression about development plans for Kexi
//! Only recommended for test/development versions.
#cmakedefine KEXI_SHOW_UNIMPLEMENTED

//! @def KEXI_PROJECT_TEMPLATES
//! @brief If defined, support for project templates is enabled in Kexi
#cmakedefine KEXI_PROJECT_TEMPLATES

//! @def KEXI_AUTORISE_TABBED_TOOLBAR
//! @brief If defined, tabs in the main tabbed toolbar autorise in Kexi
#cmakedefine KEXI_AUTORISE_TABBED_TOOLBAR

//! @def KEXI_FORM_CURSOR_PROPERTY_SUPPORT
//! @brief If defined, "cursor" property is displayed in the form designer
#cmakedefine KEXI_FORM_CURSOR_PROPERTY_SUPPORT

//! @def KEXI_SHOW_CONTEXT_HELP
//! @brief If defined, context help is displayed in Kexi main window
#cmakedefine KEXI_SHOW_CONTEXT_HELP

//! @def KEXI_QUICK_PRINTING_SUPPORT
//! @brief If defined, print/print preview/print setup for tables/queries is enabled in the project navigator
#cmakedefine KEXI_QUICK_PRINTING_SUPPORT

//! @def KEXI_AUTOFIELD_FORM_WIDGET_SUPPORT
//! @brief If defined, "auto field" form widget is available in the form designer
#cmakedefine KEXI_AUTOFIELD_FORM_WIDGET_SUPPORT

//! @def KEXI_LIST_FORM_WIDGET_SUPPORT
//! @brief If defined, "list" form widget is available in the form designer
#cmakedefine KEXI_LIST_FORM_WIDGET_SUPPORT

//! @def KEXI_PIXMAP_COLLECTIONS_SUPPORT
//! @brief If defined, support for pixmap collections is enabled
#cmakedefine KEXI_PIXMAP_COLLECTIONS_SUPPORT

#endif
