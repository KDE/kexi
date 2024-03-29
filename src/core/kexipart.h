/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2003-2014 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIPART_H
#define KEXIPART_H

#include <QObject>
#include <QMap>

#include <kexiutils/InternalPropertyMap.h>
#include "kexipartbase.h"

#include <KDbTristate>

class KActionCollection;
class KexiWindow;
class KexiWindowData;
class KexiView;
class KDbQuerySchema;
class QAction;
class QKeySequence;

namespace KexiPart
{
class Item;
class GUIClient;
class StaticPartInfo;

/*! Official (registered) type IDs for objects like table, query, form... */
enum ObjectType {
    UnknownObjectType = KDb::UnknownObjectType, //!< -1, helper
    AnyObjectType = KDb::AnyObjectType,         //!<  0, helper
    TableObjectType = KDb::TableObjectType,     //!<  1, like in KDb::ObjectType
    QueryObjectType = KDb::QueryObjectType,     //!<  2, like in KDb::ObjectType
    FormObjectType = 3,
    ReportObjectType = 4,
    ScriptObjectType = 5,
    WebObjectType = 6,
    MacroObjectType = 7,
    LastObjectType = 7, //ALWAYS UPDATE THIS

    UserObjectType = 100 //!< external types
};

//! @return Kexi Part API version: "major.minor"
//! @since 3.1
KEXICORE_EXPORT QString version();

//! @short The main class for kexi frontend parts (plugins) like tables, queries, forms and reports
/*!
  Plugins create windows (KexiWindow) for a given type of object.

  Notes for plugins implementors:  This class supports InternalPropertyMap interface,
  so supported internal properties affecting its behaviour are:
  - newObjectsAreDirty: True if newly created, unsaved objects are dirty. False by default.
  - textViewModeCaption: custum i18n'd action text replacing standard "Text View" text.
  - textViewModeToolTip: custum i18n'd action tool tip replacing standard "Switch to text view" text.
    Used in for query's "SQL View".
  In general: a whole set of i18n'd action names, initialised on KexiPart::Part subclass ctor.
  The names are useful because the same action can have other name for each part,
  e.g. "New table" vs "New query" can have different forms for some languages.
  So this is a flexible way for customizing translatable strings.
 */
class KEXICORE_EXPORT Part : public PartBase
{
    Q_OBJECT

public:
    virtual ~Part();

//! @todo make it protected, outside world should use KexiProject
    /*! Try to execute the part. Implementations of this \a Part
    are able to overwrite this method to offer execution.
    \param item The \a KexiPart::Item that should be executed.
    \param sender The sender QObject which likes to execute this \a Part or
    NULL if there is no sender. The KFormDesigner uses this to pass
    the actual widget (e.g. the button that was pressed).
    \return true if execution was successfully else false.
                */
    virtual bool execute(KexiPart::Item* item, QObject* sender = 0) {
        Q_UNUSED(item);
        Q_UNUSED(sender);
        return false;
    }

//! @todo make it protected, outside world should use KexiProject
    /*! "Opens" an instance that the part provides, pointed by \a item in a mode \a viewMode.
     \a viewMode is one of Kexi::ViewMode enum.
     \a staticObjectArgs can be passed for static Kexi Parts.
     The new widget will be a child of \a parent. */
    KexiWindow* openInstance(QWidget* parent, KexiPart::Item *item,
                             Kexi::ViewMode viewMode = Kexi::DataViewMode, QMap<QString, QVariant>* staticObjectArgs = 0);

//! @todo make it protected, outside world should use KexiProject
    /*! Removes any stored data pointed by \a item (example: table is dropped for table part).
     From now this data is inaccesible, and \a item disappear.
     You do not need to remove \a item, or remove object's schema stored in the database,
     beacuse this will be done automatically by KexiProject after successful
     call of this method. All object's data blocks are also automatically removed from database
     (from "kexi__objectdata" table).
     For this, a database connection associated with kexi project owned by \a win can be used.

     Database transaction is started by KexiProject before calling this method,
     and it will be rolled back if you return false here.
     You shouldn't use by hand transactions here.

     Default implementation just removes object from kexi__* system structures
     at the database backend using KDbConnection::removeObject(). */
    virtual tristate remove(KexiPart::Item *item);

    /*! Renames stored data pointed by \a item to \a newName
     (example: table name is altered in the database).
     For this, a database connection associated with kexi project owned by \a win can be used.
     You do not need to change \a item, and change object's schema stored in the database,
     beacuse this is automatically handled by KexiProject.

     Database transaction is started by KexiProject before calling this method,
     and it will be rolled back if you return false here.
     You shouldn't use by hand transactions here.

     Default implementation does nothing and returns true. */
    virtual tristate rename(KexiPart::Item *item, const QString& newName);

    /*! Creates and returns a new temporary data for a window  \a window.
     This method is called on openInstance() once per dialog.
     Reimplement this to return KexiWindowData subclass instance.
     Default implemention just returns empty KexiWindowData object. */
    Q_REQUIRED_RESULT virtual KexiWindowData *createWindowData(KexiWindow *window);

    /*! Creates a new view for mode \a viewMode, \a item and \a parent. The view will be
     used inside \a dialog. */
    virtual KexiView *createView(QWidget *parent, KexiWindow *window, KexiPart::Item *item,
                                 Kexi::ViewMode viewMode = Kexi::DataViewMode,
                                 QMap<QString, QVariant> *staticObjectArgs = nullptr) /*Q_REQUIRED_RESULT*/ = 0;

    //virtual void initTabs();

    /*! @return i18n'd instance name usable for displaying in gui as object's name,
     e.g. "table".
     The name is valid identifier - contains latin-1 lowercase characters only. */
    QString instanceName() const;

    /*! @return i18n'd tooltip that can also act as descriptive name of the action.
     Example: "Create new table". */
    QString toolTip() const;

    /*! @return i18n'd "what's this" string. Example: "Creates new table." */
    QString whatsThis() const;

    /*! \return part's GUI Client, so you can
     create part-wide actions using this client. */
    GUIClient *guiClient() const;

    /*! \return part's GUI Client, so you can
     create instance-wide actions using this client. */
    GUIClient *instanceGuiClient(Kexi::ViewMode mode = Kexi::AllViewModes) const;

    /*! \return action collection for mode \a viewMode. */
    KActionCollection* actionCollectionForMode(Kexi::ViewMode viewMode) const;

    const Kexi::ObjectStatus& lastOperationStatus() const;

    /*! \return query schema currently edited in the \a view.
     * It may be the original/saved query if user has no unsaved changes so far
     * or a temporary unsaved query if there are unsaved modifications.
     * The query can be used for example by data exporting routines so user can
     * export result of a running unsaved query without prior saving it. For implementation in plugins. */
    virtual KDbQuerySchema *currentQuery(KexiView* view);

    /*! @internal
     Creates GUIClients for this part, attached to the main window.
     This method is called by KexiMainWindow. */
    void createGUIClients();

Q_SIGNALS:
    void newObjectRequest(KexiPart::Info *info);

protected:
    /*!
     Creates new Plugin
     @param parent parent of this plugin
     @param instanceName i18n'd instance name written using only lowercase alphanumeric
            characters (a..z, 0..9).
            Use '_' character instead of spaces. First character should be a..z character.
            If you cannot use latin characters in your language, use english word.
            Example: "table".
     @param toolTip i18n'd tooltip that can also act as descriptive name of the action.
                    Example: "Create new table".
     @param whatsThis i18n'd "what's this" string. Example: "Creates new table."
     @param list extra arguments passed to the plugin
    */
    Part(QObject *parent,
        const QString& instanceName,
        const QString& toolTip,
        const QString& whatsThis,
        const QVariantList& list);

    //! Used by StaticPart
    Part(QObject* parent, StaticPartInfo *info);

    virtual void initPartActions();
    virtual void initInstanceActions();

    /*! Can be reimplemented if object data is extended beyond the default set of properties. This
     is the case for table and query schema objects, where object of KDbObject subclass is returned.
     In this case value pointed by @a ownedByWindow is set to false. Default implemenatation returns
     owned KDbObject object (value pointed by @a ownedByWindow is set to true).
     @a ownedByWindow is required. */
    Q_REQUIRED_RESULT virtual KDbObject *loadSchemaObject(KexiWindow *window, const KDbObject &object,
                                        Kexi::ViewMode viewMode, bool *ownedByWindow);

    bool loadDataBlock(KexiWindow *window, QString *dataString, const QString& dataID = QString());

    /*! Creates shared action for action collection declared
     for 'instance actions' of this part.
     See KexiSharedActionHost::createSharedAction() for details.
     Pass desired QAction subclass with \a subclassName (e.g. "KToggleAction") to have
     that subclass allocated instead just QAction (what is the default). */
    QAction * createSharedAction(Kexi::ViewMode mode, const QString &text,
                                const QString &pix_name, const QKeySequence &cut, const char *name,
                                const char *subclassName = 0);

    /*! Convenience version of above method - creates shared toggle action. */
    QAction * createSharedToggleAction(Kexi::ViewMode mode, const QString &text,
                                      const QString &pix_name, const QKeySequence &cut, const char *name);

    /*! Creates shared action for action collection declared
     for 'part actions' of this part.
     See KexiSharedActionHost::createSharedAction() for details.
     Pass desired QAction subclass with \a subclassName (e.g. "KToggleAction") to have
     that subclass allocated instead just QAction (what is the default). */
    QAction * createSharedPartAction(const QString &text,
                                    const QString &pix_name, const QKeySequence &cut, const char *name,
                                    const char *subclassName = 0);

    /*! Convenience version of above method - creates shared toggle action
     for 'part actions' of this part. */
    QAction * createSharedPartToggleAction(const QString &text,
                                          const QString &pix_name, const QKeySequence &cut, const char *name);

    void setActionAvailable(const char *action_name, bool avail);

private:
    //! Calls loadSchemaObject() (virtual), updates ownership of object data for @a window
    //! and assigns the created data to @a window.
    void loadAndSetSchemaObject(KexiWindow *window, const KDbObject& object,
                               Kexi::ViewMode viewMode);

    Q_DISABLE_COPY(Part)

    class Private;
    Private * const d;

    friend class Manager;
    friend class ::KexiWindow;
    friend class GUIClient;
};

/*! \return full caption for item \a item and part \a part.
 If \a part is provided, the captions will be in a form of "name : inctancetype", e.g. "Employees : Table",
 otherwise it will be in a form of "name", e.g. "Employees". */
KEXICORE_EXPORT QString fullCaptionForItem(KexiPart::Item *item, KexiPart::Part *part);

/*! \return i18n'd actionText and iconName for "Open in text view" action specific for @a pluginId.
 Currently it the only special is for "org.kexi-project.query".
 The default is "Design in Text View" and no icon. */
KEXICORE_EXPORT void getTextViewAction(const QString& pluginId, QString *actionText, QString *iconName);

} // namespace KexiPart

#endif
