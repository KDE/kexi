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

#ifndef KEXIMAINWINDOWIFACE_H
#define KEXIMAINWINDOWIFACE_H

//#define KEXI_IMPL_WARNINGS

#include <QMap>

#include <kexi_global.h>
#include <KDbTristate>

#include "KexiMigrateManagerInterface.h"
#include "kexisharedactionhost.h"
#include "kexi.h"

class KexiWindow;
class KexiProject;
class KActionCollection;
class KexiSearchableModel;
class KexiUserFeedbackAgent;
class KexiMigrateManagerInterface;
namespace KexiPart
{
class Item;
class Info;
}
class KToolBar;

/**
 * @short Kexi's main window interface
 * This interface is implemented by KexiMainWindow class.
 * KexiMainWindow offers simple features what lowers cross-dependency (and also avoids
 * circular dependencies between Kexi modules).
 */
class KEXICORE_EXPORT KexiMainWindowIface : public KexiSharedActionHost
{
public:
    //! Used by printActionForItem()
    enum PrintActionType {
        PrintItem,
        PreviewItem,
        PageSetupForItem
    };

    KexiMainWindowIface();
    virtual ~KexiMainWindowIface();

    //! \return KexiMainWindowImpl global singleton (if it is instantiated)
    static KexiMainWindowIface* global();

    QWidget* thisWidget();

    //! Project data of currently opened project or NULL if no project here yet.
    virtual KexiProject *project() = 0;

#ifdef KEXI_IMPL_WARNINGS
#ifdef __GNUC__
#warning TODO virtual KActionCollection* actionCollection() const = 0;
#else
#pragma WARNING( TODO virtual KActionCollection* actionCollection() const = 0; )
#endif
#endif
    virtual KActionCollection* actionCollection() const = 0;

#ifdef KEXI_IMPL_WARNINGS
#ifdef __GNUC__
#warning TODO virtual QWidget* focusWidget() const = 0;
#else
#pragma WARNING( TODO virtual QWidget* focusWidget() const = 0; )
#endif
#endif
    virtual QWidget* focusWidget() const = 0;

    /*! Registers window \a window for watching and adds it to the main window's stack. */
    virtual void registerChild(KexiWindow *window) = 0;

    /*! \return a list of all actions defined by application.
     Not all of them are shared. Don't use plug these actions
     in your windows by hand but user methods from KexiView! */
    virtual QList<QAction*> allActions() const = 0;

    /*! \return currently active window or 0 if there is no active window. */
    virtual KexiWindow* currentWindow() const = 0;

    /*! Switches \a window to view \a mode.
     Activates the window if it is not the current window. */
    virtual tristate switchToViewMode(KexiWindow& window, Kexi::ViewMode viewMode) = 0;

    /*! \return true if this window is in the User Mode. */
    virtual bool userMode() const = 0;

// Q_SIGNALS:
    //! Emitted to make sure the project can be close.
    //! Connect a slot here and set \a cancel to true to cancel the closing.
    virtual void acceptProjectClosingRequested(bool *cancel) = 0;

    //! Emitted before closing the project (and destroying all it's data members).
    //! You can do you cleanup of your structures here.
    virtual void beforeProjectClosing() = 0;

    //! Emitted after closing the project.
    virtual void projectClosed() = 0;

// public Q_SLOTS:
    /*! Creates new object of type defined by \a info part info.
     \a openingCancelled is set to true is opening has been cancelled.
     \return true on success. */
    virtual bool newObject(KexiPart::Info *info, bool *openingCancelled) = 0;

    //! Opens object pointed by \a item in a view \a viewMode
    virtual KexiWindow* openObject(KexiPart::Item *item, Kexi::ViewMode viewMode,
                                   bool *openingCancelled, QMap<QString, QVariant>* staticObjectArgs = 0,
                                   QString* errorMessage = 0) = 0;

    //! For convenience
    virtual KexiWindow* openObject(const QString& mime, const QString& name,
                                   Kexi::ViewMode viewMode, bool *openingCancelled,
                                   QMap<QString, QVariant>* staticObjectArgs = 0) = 0;

    /*! Closes the object for \a item.
     \return true on success (closing can be dealyed though), false on failure and cancelled
     if the object has "opening" job assigned. */
    virtual tristate closeObject(KexiPart::Item* item) = 0;

    /*! Called to accept property butter editing. */
    virtual void acceptPropertySetEditing() = 0;

    /*! Received information from active view that \a window has switched
    its property set, so property editor contents should be reloaded.
     If \a force is true, property editor's data is reloaded even
     if the currently pointed property set is the same as before.
     If \a preservePrevSelection is true and there was a property set
     set before call, previously selected item will be preselected
     in the editor (if found). */
    virtual void propertySetSwitched(KexiWindow *window, bool force = false,
                                     bool preservePrevSelection = true,
                                     bool sortedProperties = false,
                                     const QByteArray& propertyToSelect = QByteArray()) = 0;

    //! Options used in saveObject()
    enum SaveObjectOption
    {
        DoNotAsk = 1,    //!< Do not ask for confirmation of overwriting
        SaveObjectAs = 2 //!< Saving object with a new name
    };
    Q_DECLARE_FLAGS(SaveObjectOptions, SaveObjectOption)

    /*! Saves window's \a window data. If window's data is never saved,
     user is asked for name and title, before saving (see getNewObjectInfo()).
     \return true on successul saving or false on error.
     If saving was cancelled by user, cancelled is returned.
     \a messageWhenAskingForName is a i18n'ed text that will be visible
     within name/caption dialog (see KexiNameDialog), which is popped
     up for never saved objects.
     Saving object with a new name is also supported here, to do so
     SaveObjectOption::SaveObjectAs should be added to @a options. */
    virtual tristate saveObject(KexiWindow *window,
                                const QString& messageWhenAskingForName = QString(),
                                SaveObjectOptions options = 0) = 0;

    /*! Closes window \a window. If window's data (see KexiWindow::isDirty()) is unsaved,
     used will be asked if saving should be perforemed.
     \return true on successull closing or false on closing error.
     If closing was cancelled by user, cancelled is returned.
     If \a window is 0, the current one will be closed. */
    virtual tristate closeWindow(KexiWindow *window) = 0;

    /*! Find window for a given \a identifier.
     \return 0 if no windows found. */
    virtual KexiWindow *openedWindowFor(int identifier) = 0;

    /*! Find window for a given \a item.
     \return 0 if no windows found. */
    virtual KexiWindow *openedWindowFor(const KexiPart::Item* item) = 0;

    /*! Parametrs for query with given id. */
    virtual QList<QVariant> currentParametersForQuery(int queryId) const = 0;

    //! \return query schema currently unsaved (edited) in a window corresponding to Kexi object identified by \a identifier.
    /*! For implementation in plugins, default implementation returns 0.
     * In implementations 0 should be returned if there is no such Kexi object
     * in the current project or if the object's window is not opened or if
     * the window contains no edited query at the moment.
     * If the query is "unsaved" the window displaying the corresponding Kexi object is marked as "dirty".
     * Currently supported type of Kexi objects are only queries being in data view.
     * See KexiQueryPart::unsavedQuery(int) for this implementation.
     * The query schema returned by this method can be used for example by data
     * exporting routines so users can export result of running unsaved
     * query without prior saving its design.
     * The changes to design can be even discarded without consequences this way.
    @note Returned pointer leads to a temporary query schema object owned by the corresponding view,
     * so lifetime of the object is limited to the lifetime of the view and its window.
     * Do not store the pointer after the window is closed to avoid dangling pointers.
    \see KexiPart::Part::currentQuery(KexiView*) KexiWindow::isDirty()
    */
    virtual KDbQuerySchema* unsavedQuery(int identifier) = 0;

    /*! Displays a dialog for entering object's name and title.
     Used on new object saving.
     \return true on successul closing or cancelled on cancel returned.
     It's unlikely to have false returned here.
     \a messageWhenAskingForName is a i18n'ed text that will be visible
     within name/caption dialog (see KexiNameDialog).
     If \a allowOverwriting is true, user will be asked for existing
     object's overwriting, else it will be impossible to enter
     a name of existing object.
     You can check \a overwriteNeeded after calling this method.
     If it's true, user agreed on overwriting, if it's false, user picked
     nonexisting name, so no overwrite will be needed.
     If \a originalName is not empty, the dialog will make sure the entered name
     is different, what is useful for "Saving As" objects.
    */
    virtual tristate getNewObjectInfo(KexiPart::Item *partItem,
                                      const QString &originalName,
                                      KexiPart::Part *part,
                                      bool allowOverwriting,
                                      bool *overwriteNeeded,
                                      const QString& messageWhenAskingForName = QString()) = 0;

    /*! Highlights object of mime \a mime and name \a name.
     This can be done in the Project Navigator or so.
     If a window for the object is opened (in any mode), it should be raised. */
    virtual void highlightObject(const QString& mime, const QString& name) = 0;

    //! Shows "print" dialog for \a item.
    //! \return true on success.
    virtual tristate printItem(KexiPart::Item* item) = 0;

    //! Shows "print preview" window.
    //! \return true on success.
    virtual tristate printPreviewForItem(KexiPart::Item* item) = 0;

    //! Shows "page setup" window for \a item.
    //! \return true on success and cancelled when the action was cancelled.
    virtual tristate showPageSetupForItem(KexiPart::Item* item) = 0;

    /*! Executes custom action for the main window, usually provided by a plugin.
     Also used by KexiFormEventAction. */
    virtual tristate executeCustomActionForObject(KexiPart::Item* item, const QString& actionName) = 0;

//! @todo temporary solution before the tabbed toolbar framework emerges
    /*! Appends widget @a widget to tabbed toolbar declared as @a name.
     @a widget will be reparented but the ownership is not taken. */
    virtual void appendWidgetToToolbar(const QString& name, QWidget* widget) = 0;

//! @todo temporary solution before the tabbed toolbar framework emerges
    /*! Shows or hides widget in the tabbed toolbar. */
    virtual void setWidgetVisibleInToolbar(QWidget* widget, bool visible) = 0;

//! @todo replace with the final Actions API
    virtual void addToolBarAction(const QString& toolBarName, QAction *action) = 0;

//! @todo replace with the final Actions API
    virtual KToolBar *toolBar(const QString& name) const = 0;

    /*! Updates info label of the property editor by reusing properties provided
     by the current property set.
     Read documentation of KexiPropertyEditorView class for information about accepted properties.
     If the current property is 0 and @a textToDisplayForNullSet string is not empty, this string is displayed
     (without icon or any other additional part).
     If the current property is 0 and @a textToDisplayForNullSet string is empty, the info label widget becomes
     hidden. */
    virtual void updatePropertyEditorInfoLabel(const QString& textToDisplayForNullSet = QString()) = 0;

    /*! Add searchable model to the main window. This extends search to a new area.
     One example is Project Navigator. */
    virtual void addSearchableModel(KexiSearchableModel *model) = 0;

    virtual KexiUserFeedbackAgent* userFeedbackAgent() const = 0;

    //! Interface to the migrate manager
    virtual KexiMigrateManagerInterface* migrateManager() = 0;

    //! Sets reasonable dialog size based on main window size, that is 80% of its size.
    virtual void setReasonableDialogSize(QDialog *dialog) = 0;

protected: // Q_SLOTS:
    virtual void slotObjectRenamed(const KexiPart::Item &item, const QString& oldName) = 0;

};

Q_DECLARE_OPERATORS_FOR_FLAGS(KexiMainWindowIface::SaveObjectOptions)

#endif
