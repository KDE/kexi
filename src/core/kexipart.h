/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2003-2007 Jarosław Staniek <staniek@kde.org>

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

#include <qobject.h>
#include <qmap.h>

#include <kexiutils/tristate.h>
#include <kexiutils/InternalPropertyMap.h>
#include "kexi.h"

class KActionCollection;
class KexiWindow;
class KexiWindowData;
class KexiView;
class KAction;
class KShortcut;
class KTabWidget;

namespace KexiPart
{
class Info;
class Item;
class GUIClient;
class StaticInfo;

/*! Official (registered) type IDs for objects like table, query, form... */
enum ObjectTypes {
    TableObjectType = KexiDB::TableObjectType, //!< 1, like in KexiDB::ObjectTypes
    QueryObjectType = KexiDB::QueryObjectType, //!< 2, like in KexiDB::ObjectTypes
    FormObjectType = 3,
    ReportObjectType = 4,
    ScriptObjectType = 5,
    WebObjectType = 6,
    MacroObjectType = 7,
    LastObjectType = 7, //ALWAYS UPDATE THIS

    UserObjectType = 100 //!< external types
};

//! @short The main class for kexi frontend parts (plugins) like tables, queries, forms and reports
/*!
  Plugins create windows (KexiWindow) for a given type of object.

  Notes for plugins implementors:  This class supports InternalPropertyMap interface,
  so supported internal properties affecting its behaviour are:
  - newObjectsAreDirty: True if newly created, unsaved objects are dirty. False by default.
  - instanceName: base i18n'd name of objects created by a plugin, e.g. "table",
    see instanceName().
  - instanceCaption: i18n'd instance name usable for displaying in gui as object's caption,
    e.g. "Table", see instanceCaption()
  - instanceToolTip: i18n'd tool tip for "Create" action for a given part, e.g. "Create new table"
  - instanceWhatsThis: i18n'd "What's This" text for "Create" action for a given part,
    e.g. "Creates new table."
  - textViewModeCaption: custum action text replacing standard "Text View" text.
    Used in for query's "SQL View".
  In general: a whole set of i18n'd action names, initialised on KexiPart::Part subclass ctor.
  The names are useful because the same action can have other name for each part
  E.g. "New table" vs "New query" can have different forms for some languages.
  So this is a flexible way for customizing translatable strings.
 */
class KEXICORE_EXPORT Part : public QObject, protected KexiUtils::InternalPropertyMap
{
    Q_OBJECT

public:
    Part(QObject *parent, const QStringList &);

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

    /*! \return supported modes for dialogs created by this part, i.e. a combination
     of Kexi::ViewMode enum elements.
     Set this member in your KexiPart subclass' ctor, if you need to override the default value
     that equals Kexi::DataViewMode | Kexi::DesignViewMode,
     or Kexi::DesignViewMode in case of Kexi::PartStaticPart object.
     This information is used to set supported view modes for every
     KexiView-derived object created by this KexiPart. */
    Kexi::ViewModes supportedViewModes() const;

    /*! \return supported modes for dialogs created by this part in "user mode", i.e. a combination
     of Kexi::ViewMode enum elements.
     Set this member in your KexiPart subclass' ctor, if you need to override the default value
     that equals Kexi::DataViewMode. or 0 in case of Kexi::PartStaticPart object.
     This information is used to set supported view modes for every
     KexiView-derived object created by this KexiPart. */
    Kexi::ViewModes supportedUserViewModes() const;

//! @todo make it protected, outside world should use KexiProject
    /*! "Opens" an instance that the part provides, pointed by \a item in a mode \a viewMode.
     \a viewMode is one of Kexi::ViewMode enum.
     \a staticObjectArgs can be passed for static Kexi Parts.
     The new widget will be a child of \a parent. */
    KexiWindow* openInstance(QWidget* parent, KexiPart::Item &item,
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
     at the database backend using KexiDB::Connection::removeObject(). */
    virtual bool remove(KexiPart::Item & item);

    /*! Renames stored data pointed by \a item to \a newName
     (example: table name is altered in the database).
     For this, a database connection associated with kexi project owned by \a win can be used.
     You do not need to change \a item, and change object's schema stored in the database,
     beacuse this is automatically handled by KexiProject.

     Database transaction is started by KexiProject before calling this method,
     and it will be rolled back if you return false here.
     You shouldn't use by hand transactions here.

     Default implementation does nothing and returns true. */
    virtual tristate rename(KexiPart::Item &item, const QString& newName);

    /*! Creates and returns a new temporary data for a window  \a window.
     This method is called on openInstance() once per dialog.
     Reimplement this to return KexiWindowData subclass instance.
     Default implemention just returns empty KexiWindowData object. */
    virtual KexiWindowData* createWindowData(KexiWindow *window);

    /*! Creates a new view for mode \a viewMode, \a item and \a parent. The view will be
     used inside \a dialog. */
    virtual KexiView* createView(QWidget *parent, KexiWindow *window,
                                 KexiPart::Item &item, 
                                 Kexi::ViewMode viewMode = Kexi::DataViewMode, 
                                 QMap<QString, QVariant>* staticObjectArgs = 0) = 0;

    /*! i18n'd instance name usable for displaying in gui as object's name.
     The name is valid identifier - contains latin1 lowercase characters only.
     @todo move this to Info class when the name could be moved as localized property
     to service's .desktop file. */
    QString instanceName() const;

    /*! i18n'd instance name usable for displaying in gui as object's caption.
     @todo move this to Info class when the name could be moved as localized property
     to service's .desktop file. */
    QString instanceCaption() const;

    Info *info() const;

    /*! \return part's GUI Client, so you can
     create part-wide actions using this client. */
    GUIClient *guiClient() const;

    /*! \return part's GUI Client, so you can
     create instance-wide actions using this client. */
    GUIClient *instanceGuiClient(Kexi::ViewMode mode = Kexi::AllViewModes) const;

#if 0
    /**
     * @returns the datasource object of this part
     * reeimplement it to make a part work as dataprovider ;)
     */
    virtual DataSource *dataSource() {
        return 0;
    }
#endif

    /*! \return action collection for mode \a viewMode. */
    KActionCollection* actionCollectionForMode(Kexi::ViewMode viewMode) const;

    const Kexi::ObjectStatus& lastOperationStatus() const;

    /*! \return i18n'd message translated from \a englishMessage.
     This method is useful for messages like:
     "<p>Table \"%1\" has been modified.</p>",
     -- such messages can be accurately translated,
     while this could not: "<p>%1 \"%2\" has been modified.</p>".
     See implementation of this method in KexiTablePart to see
     what strings are needed for translation.

     Default implementation returns generic \a englishMessage.
     In special cases, \a englishMessage can start with ":",
     to indicate that empty string will be generated if
     a part does not offer a message for such \a englishMessage.
     This is used e.g. in KexiMainWindow::closeWindow().

     Note: As number of %n parameters is unspecified,
     you should add appropriate number of parameters using .subs().
     to result of results of i18nMessage().
     In your your implementation, you should use ki18n(I18N_NOOP())
     or ki18nc(I18N_NOOP2()) instead of i18n() or i18nc().
     Example:
     @code
      QString tableName = "Employees";
      QString translated
       = part->i18nMessage("Design of object \"%1\" has been modified.")
        .subs(tableName).toString();
     @endcode */
    virtual KLocalizedString i18nMessage(const QString& englishMessage,
                                         KexiWindow *window) const;

    /*! @internal
     Creates GUIClients for this part, attached to the main window.
     This method is called by KexiMainWindow. */
    void createGUIClients();

    /*! @internal
     This method can be reimplemented to setup additional tabs
     in the property editor panel. Default implementation does nothing.
     This method is called whenever current window (KexiWindow) is switched and
     type (mime type) of its contents differs from previous one.
     For example, if a user switched from Table Designer to Form Designer,
     additional tab containing Form Designer's object tree should be shown. */
    virtual void setupCustomPropertyPanelTabs(KTabWidget *tab);

signals:
    void newObjectRequest(KexiPart::Info *info);

protected slots:
    void slotCreate();

protected:
    //! Used by StaticPart
    Part(QObject* parent, StaticInfo *info);

#if 0
    /*! For reimplementation. Create here all part actions (KAction or similar).
     "Part action" is an action that is bound to given part, not for dialogs
     created with this part, eg. "Open external project" action for Form part.
     Default implementation does nothing.
    */
    virtual void initPartActions(KActionCollection *) {};

    /*! For reimplementation. You should here create all instance actions (KAction or similar)
     for \a mode (this method called for every value given by Kexi::ViewMode enum,
     and in special cases, in the future - for user-defined part-specific modes).
     Actions should be bound to action collection \a col.
     "Instance action" is an action that is bound to given dialog instance (created with a part),
     for specific view. \a mo; eg. "Filter data" action for DataViewMode of Table part.
     By creating actions here, you can ensure that after switching to other view mode (eg. from
     Design view to Data view), appropriate actions will be switched/hidden.
     \a mode equal Kexi::AllViewModes means that given actions will be available for
     all supported views.
     Default implementation does nothing.
    */
    virtual void initInstanceActions(int mode, KActionCollection *col) {};
#endif

    virtual void initPartActions();
    virtual void initInstanceActions();

    virtual KexiDB::SchemaData* loadSchemaData(KexiWindow *window,
            const KexiDB::SchemaData& sdata, Kexi::ViewMode viewMode);

    bool loadDataBlock(KexiWindow *window, QString &dataString, const QString& dataID = QString());

    /*! Creates shared action for action collection declared
     for 'instance actions' of this part.
     See KexiSharedActionHost::createSharedAction() for details.
     Pass desired KAction subclass with \a subclassName (e.g. "KToggleAction") to have
     that subclass allocated instead just KAction (what is the default). */
    KAction* createSharedAction(Kexi::ViewMode mode, const QString &text,
                                const QString &pix_name, const KShortcut &cut, const char *name,
                                const char *subclassName = 0);

    /*! Convenience version of above method - creates shared toggle action. */
    KAction* createSharedToggleAction(Kexi::ViewMode mode, const QString &text,
                                      const QString &pix_name, const KShortcut &cut, const char *name);

    /*! Creates shared action for action collection declared
     for 'part actions' of this part.
     See KexiSharedActionHost::createSharedAction() for details.
     Pass desired KAction subclass with \a subclassName (e.g. "KToggleAction") to have
     that subclass allocated instead just KAction (what is the default). */
    KAction* createSharedPartAction(const QString &text,
                                    const QString &pix_name, const KShortcut &cut, const char *name,
                                    const char *subclassName = 0);

    /*! Convenience version of above method - creates shared toggle action
     for 'part actions' of this part. */
    KAction* createSharedPartToggleAction(const QString &text,
                                          const QString &pix_name, const KShortcut &cut, const char *name);

    void setActionAvailable(const char *action_name, bool avail);

    void setInfo(Info *info);

//    int registeredPartID() const;

    /*! Sets supported modes for windows created by this part in "user mode".
     The default is Kexi::DataViewMode. It is altered in classes like KexiSimplePrintingPart.
     @see supportedUserViewModes() */
    void setSupportedUserViewModes(Kexi::ViewModes modes);

    /*! Sets supported view modes for this part. */
    void setSupportedViewModes(Kexi::ViewModes modes);

    /*! Creates actions for view mode @a mode. The action are appended in the local toolbar(s).
     For implementation.
     @since 2.0 */
//  virtual QList<KAction*> createViewActions(Kexi::ViewMode mode) { return QList<KAction*>(); };

    /*! @return action for name @a name, shared between views.
     @since 2.0 */
//  KAction* sharedViewAction(const char* name) const;

private:
    class Private;
    Private * const d;

    friend class Manager;
    friend class ::KexiWindow;
    friend class GUIClient;
};

/*! \return full caption for item \a item and part \a part.
 If \a part is provided, the captions will be in a form of "name : inctancetype", e.g. "Employees : Table",
 otherwise it will be in a form of "name", e.g. "Employees". */
KEXICORE_EXPORT QString fullCaptionForItem(KexiPart::Item& item, KexiPart::Part *part);

}

#endif
