/* This file is part of the KDE project
   Copyright (C) 2006-2008 Sebastian Sauer <mail@dipe.org>

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

#ifndef KEXISCRIPTADAPTOR_H
#define KEXISCRIPTADAPTOR_H

#include <QObject>
#include <QMetaObject>
#include <QAction>
#include <QJSEngine>
#include <QJSValue>

#include <KDbConnection>

#include <kexi.h>
#include <kexipart.h>
#include <kexiproject.h>
#include <KexiMainWindowIface.h>
#include <KexiWindow.h>
#include <KexiView.h>

#include "../kexidb/kexidbmodule.h"
#include "KexiScriptingDebug.h"

/**
* Adaptor class that provides Kexi specific functionality to
* the scripting world.
*/
class KexiScriptAdaptor : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE KexiScriptAdaptor() {
        KexiScriptingDebug() << "Created Script Adaptor";
    }
    virtual ~KexiScriptAdaptor() {}

    /**
    * Returns the current KexiWindow widget.
    */
    Q_INVOKABLE QWidget* windowWidget() const {
        return currentWindow();
    }

    /**
    * Returns the current KexiView widget.
    */
    Q_INVOKABLE QWidget* viewWidget() const {
        return currentView();
    }

    /**
    * Returns a list of all QAction instances the Kexi main
    * window provides.
    *
    * Python sample that prints the list of all actions the
    * main window does provide.
    * \code
    * import Kexi
    * for a in Kexi.actions():
    *     print "name=%s text=%s" % (a.objectName,a.text)
    * \endcode
    */
    Q_INVOKABLE QVariantList actions() {
        QVariantList list;
        foreach(QAction* action, mainWindow()->allActions()) {
            QVariant v;
            v.setValue((QObject*) action);
            list << v;
        }
        return list;
    }

    /**
    * Returns the QAction instance the Kexi main window provides that
    * has the objectName \p name or NULL if there is no such action.
    */
    Q_INVOKABLE QObject* action(const QString& name) {
        foreach(QAction* action, mainWindow()->allActions()) {
            if (action->objectName() == name)
                return action;
        }
        return 0;
    }

    /**
    * Returns true if we are connected with a project else false
    * is returned.
    */
    Q_INVOKABLE bool isConnected() {
        return project() ? project()->isConnected() : false;
    }

    /**
    * Returns the KexiDBConnection object that belongs to the opened
    * project or return NULL if there was no project opened (no
    * connection established).
    */
    Q_INVOKABLE QObject* getConnection() {
        KDbConnection *connection = project() ? project()->dbConnection() : 0;
        if (connection) {
            QObject* result = 0;
            if (QMetaObject::invokeMethod(&m_kexidbmodule, "connectionWrapper", Q_RETURN_ARG(QObject*, result), Q_ARG(KDbConnection*, connection)))
                return result;
        }
        return 0;
    }

    /**
    * Returns a list of names of all items the part class provides. Possible
    * classes are for example "org.kexi-project.table", "org.kexi-project.query",
    * "org.kexi-project.form" or "org.kexi-project.script".
    *
    * Python sample that prints all tables within the current project.
    * \code
    * import Kexi
    * print Kexi.items("table")
    * \endcode
    */
    Q_INVOKABLE QStringList items(const QString& plugin) {
        QStringList list;
        if (project()) {
            KexiPart::ItemList l;
            project()->getSortedItemsForPluginId(&l, pluginId(plugin).toUtf8());
            l.sort();
            foreach(KexiPart::Item* i, l) {
                list << i->name();
            }
        }
        return list;
    }

    /**
    * Returns the caption for the item defined with \p pluginId and \p name .
    */
    Q_INVOKABLE QString itemCaptioq_gadgetn(const QString& plugin, const QString& name) const {
        KexiPart::Item *item = partItem(pluginId(plugin), name);
        return item ? item->caption() : QString();
    }

    /**
    * Set the caption for the item defined with \p pluginId and \p name .
    */
    Q_INVOKABLE void setItemCaption(const QString& plugin, const QString& name, const QString& caption) {
        if (KexiPart::Item *item = partItem(pluginId(plugin), name))
            item->setCaption(caption);
    }

    /**
    * Returns the description for the item defined with \p className and \p name .
    */
    Q_INVOKABLE QString itemDescription(const QString& plugin, const QString& name) const {
        KexiPart::Item *item = partItem(pluginId(plugin), name);
        return item ? item->description() : QString();
    }

    /**
    * Set the description for the item defined with \p className and \p name .
    */
    Q_INVOKABLE void setItemDescription(const QString& plugin, const QString& name, const QString& description) {
        if (KexiPart::Item *item = partItem(pluginId(plugin), name))
            item->setDescription(description);
    }

    /**
    * Open an item. A window for the item defined with \p pluginId and \p name will
    * be opened and we switch to it. The \p viewmode could be for example "data" (the
    * default), "design" or "text" while the \args are optional arguments passed
    * to the item.
    *
    * Python sample that opens the "cars" form in design view mode and sets then the
    * dirty state to mark the formular as modified.
    * \code
    * import Kexi
    * Kexi.openItem("form","cars","design")
    * Kexi.windowWidget().setDirty(True)
    * \endcode
    */
    Q_INVOKABLE bool openItem(const QString& plugin, const QString& name, const QString& viewmode = QString(),
                  QVariantMap args = QVariantMap())
    {
        bool openingCancelled;
        KexiPart::Item *item = partItem(pluginId(plugin), name);
        KexiWindow* window = item
            ? mainWindow()->openObject(
                item,
                stringToViewMode(viewmode),
                &openingCancelled,
                args.isEmpty() ? 0 : &args
              )
            : 0;
        return (window && ! openingCancelled);
    }

    /**
    * Close an opened item. The window for the item defined with \p className and \p name
    * will be closed.
    *
    * Python sample that opens the "table1" table and closes the window right after
    * being opened.
    * \code
    * import Kexi
    * Kexi.openItem("table","table1")
    * Kexi.closeItem("table","table1")
    * \endcode
    */
    Q_INVOKABLE bool closeItem(const QString& plugin, const QString& name) {
        if (KexiPart::Item *item = partItem(pluginId(plugin), name))
            return mainWindow()->closeObject(item) == true;
        return false;
    }

    /**
    * Print the item defined with \p plugin and \p name .
    */
    Q_INVOKABLE bool printItem(const QString& plugin, const QString& name, bool preview = false) {
        if (KexiPart::Item *item = partItem(pluginId(plugin), name))
            return (preview ? mainWindow()->printPreviewForItem(item) : mainWindow()->printItem(item)) == true;
        return false;
    }

    /**
    * Executes custom action for the item defined with \p pluginId and \p name .
    */
    Q_INVOKABLE bool executeItem(const QString& plugin, const QString& name, const QString& actionName) {
        if (KexiPart::Item *item = partItem(pluginId(plugin), name))
            return mainWindow()->executeCustomActionForObject(item, actionName) == true;
        return false;
    }


    /**
    * Returns the name of the current viewmode. This could be for example "data",
    * "design", "text" or just an empty string if there is no view at the moment.
    */
    Q_INVOKABLE QString viewMode() const {
        return currentView() ? viewModeToString(currentView()->viewMode()) : QString();
    }

    /**
    * Returns a list of names of all available viewmodes the view supports.
    */
    Q_INVOKABLE QStringList viewModes() const {
        QStringList list;
        if (currentWindow()) {
            Kexi::ViewModes modes = currentWindow()->supportedViewModes();
            if (modes & Kexi::DataViewMode)
                list << "data";
            if (modes & Kexi::DesignViewMode)
                list << "design";
            if (modes & Kexi::TextViewMode)
                list << "text";
        }
        return list;
    }

    /**
    * Returns true if there is a current view and those current view is dirty aka
    * has the dirty-flag set that indicates that something changed.
    */
    Q_INVOKABLE bool viewIsDirty() const {
        return currentView() ? currentView()->isDirty() : false;
    }

private:
    Scripting::KexiDBModule m_kexidbmodule;

    KexiMainWindowIface* mainWindow() const {
        return KexiMainWindowIface::global();
    }
    KexiProject* project() const {
        return mainWindow()->project();
    }
    KexiWindow* currentWindow() const {
        return mainWindow()->currentWindow();
    }
    KexiView* currentView() const {
        return currentWindow() ? currentWindow()->selectedView() : 0;
    }
    KexiPart::Item* partItem(const QString& plugin, const QString& name) const {
        return project() ? project()->itemForPluginId(pluginId(plugin), name) : 0;
    }
    QString pluginId(const QString& pluginId) const {
        return pluginId.contains('.') ? pluginId : (QString::fromLatin1("org.kexi-project.")+pluginId);
    }
    QString viewModeToString(Kexi::ViewMode mode, const QString& defaultViewMode = QString()) const {
        switch (mode) {
        case Kexi::DataViewMode:
            return "data";
        case Kexi::DesignViewMode:
            return "design";
        case Kexi::TextViewMode:
            return "text";
        default:
            break;
        }
        return defaultViewMode;
    }
    Kexi::ViewMode stringToViewMode(const QString& mode, Kexi::ViewMode defaultViewMode = Kexi::DataViewMode) const {
        if (mode == "data")
            return Kexi::DataViewMode;
        if (mode == "design")
            return Kexi::DesignViewMode;
        if (mode == "text")
            return Kexi::TextViewMode;
        return defaultViewMode;
    }

};

#endif

