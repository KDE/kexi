/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2003-2015 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIWINDOWBASE_H
#define KEXIWINDOWBASE_H

#include "kexipartguiclient.h"
#include "kexiactionproxy.h"
#include "kexi.h"
#include "kexipart.h"
#include "KexiView.h"

#include <QEvent>
#include <QCloseEvent>

class KexiMainWindow;
class KexiWindowData;
class KexiView;
class KPropertySet;
namespace KexiPart
{
class Part;
}

//! Base class for child window of Kexi's main application window.
/*! This class can contain a number of configurable views, switchable using toggle action.
 It also automatically works as a proxy for shared (application-wide) actions. */
class KEXICORE_EXPORT KexiWindow
            : public QWidget
            , public KexiActionProxy
            , public Kexi::ObjectStatus
{
    Q_OBJECT

public:
    virtual ~KexiWindow();

    //! \return true if the window is registered.
    bool isRegistered() const;

    //! \return currently selected view or 0 if there is no current view
    KexiView *selectedView() const;

    /*! \return a view for a given \a mode or 0 if there's no such mode available (or opened).
     This does not open mode if it's not opened. */
    KexiView *viewForMode(Kexi::ViewMode mode) const;

    //! Adds \a view for the dialog. It will be the _only_ view (of unspecified mode) for the dialog
    void addView(KexiView *view);

    /*! \return main (top level) widget inside this dialog.
     This widget is used for e.g. determining minimum size hint and size hint. */
//  virtual QWidget* mainWidget() = 0;

    /*! reimplemented: minimum size hint is inherited from currently visible view. */
    virtual QSize minimumSizeHint() const;

    /*! reimplemented: size hint is inherited from currently visible view. */
    virtual QSize sizeHint() const;

    //js todo: maybe remove this since it's often the same as partItem()->identifier()?:

    /*! This method sets internal identifier for the dialog, but
     if there is a part item associated with this dialog (see partItem()),
     partItem()->identifier() will be is used as identifier, so this method is noop.
     Thus, it's usable only for dialog types when no part item is assigned. */
    void setId(int id);

    /*! If there is a part item associated with this dialog (see partItem()),
     partItem()->identifier() is returned, otherwise internal dialog's identifier
     (previously set by setID()) is returned. */
    int id() const;

    //! \return Kexi part used to create this window
    KexiPart::Part* part() const;

    //! \return Kexi part item used to create this window
    KexiPart::Item* partItem() const;

    //! Kexi dialog's gui COMMON client.
    //! It's obtained by querying part object for this dialog.
    KexiPart::GUIClient* commonGUIClient() const;

    //! Kexi dialog's gui client for currently selected view.
    //! It's obtained by querying part object for this dialog.
    KexiPart::GUIClient* guiClient() const;

    /*! \return name of icon provided by part that created this dialog.
     The name is used by KexiMainWindow to set/reset icon for this dialog. */
    virtual QString iconName();

    /*! \return true if this dialog supports switching to \a mode.
     \a mode is one of Kexi::ViewMode enum elements.
     The flags are used e.g. for testing by KexiMainWindow, if actions
     of switching to given view mode is available.
     This member is intialised in KexiPart that creates this window object. */
    bool supportsViewMode(Kexi::ViewMode mode) const;

    /*! \return information about supported view modes. */
    Kexi::ViewModes supportedViewModes() const;

    /*! \return current view mode for this dialog. */
    Kexi::ViewMode currentViewMode() const;

    void setContextHelp(const QString& caption, const QString& text, const QString& iconName);

    //! \return true if the window is attached within the main window
    bool isAttached() const {
        return true;
    }

    /*! True if contents (data) of the dialog is dirty and need to be saved
     This may or not be used, depending if changes in the dialog
     are saved immediately (e.g. like in datatableview) or saved by hand (by user)
     (e.g. like in alter-table dialog).
     \return true if at least one "dirty" flag is set for one of the dialog's view. */
    bool isDirty() const;

    /*! \return a pointer to view that has recently set dirty flag.
     This value is cleared when dirty flag is set to false (i.e. upon saving changes). */
    KexiView* viewThatRecentlySetDirtyFlag() const;

    /*! \return true, if this dialog's data were never saved.
     If it's true we're usually try to ask a user if the dialog's
     data should be saved somewhere. After dialog construction,
     "neverSaved" flag is set to appropriate value.
     KexiPart::Item::neverSaved() is reused.
    */
    bool neverSaved() const;

    /*! \return property set provided by the current view,
     or NULL if there is no view set (or the view has no set assigned). */
    KPropertySet *propertySet();

    //! @return schema object associated with this window
    KDbObject* schemaObject() const;

    //! Sets 'owned' property for object data.
    //! If true, the window will delete the object data before destruction.
    //! By default object data is not owned.
    //! @see setSchemaObject(), KexiPart::loadSchemaObject(), KexiPart::loadAndSetSchemaObject()
    void setSchemaObjectOwned(bool set);

    /*! Used by KexiView subclasses. \return temporary data shared between
     views */
    KexiWindowData *data() const;

    /*! Called primarily by KexiMainWindow to activate dialog.
     Selected view (if present) is also informed about activation. */
    void activate();

    /*! Called primarily by KexiMainWindow to deactivate dialog.
     Selected view (if present) is also informed about deactivation. */
    void deactivate();

    //! Helper, returns consistent window title for \a item.
    //! Used both for setting window's title and tab names in the main window.
    //! @todo Use Item::captionOrName() if this is defined in settings?
    static QString windowTitleForItem(const KexiPart::Item &item);

public Q_SLOTS:
    virtual void setFocus();

    void updateCaption();

    /*! Internal. Called by KexiMainWindow::saveObject().
     Tells this dialog to save changes of the existing object
     to the backend. If \a dontAsk is true, no question dialog will
     be shown to the user. The default is false.
     \sa storeNewData()
     \return true on success, false on failure and cancelled when storing has been cancelled. */
    tristate storeData(bool dontAsk = false);

    /*! Internal. Called by KexiMainWindow::saveObject().
     Tells this dialog to create and store data of the new object
     to the backend.
     Object's object data has been never stored,
     so it is created automatically, using information obtained
     form part item. On success, part item's ID is updated to new value,
     and object data is set. \sa schemaObject().
     \return true on success, false on failure and cancelled when storing has been cancelled. */
    tristate storeNewData(KexiView::StoreNewDataOptions options = 0);

    /*! Internal. Called by KexiMainWindow::saveObject().
     Tells this dialog to create and store a copy of data of existing object to the backend.
     Object data has been never stored,
     so it is created automatically, using information obtained
     form the part item. On success, part item's ID is updated to new value,
     and object data is set. \sa schemaObject().
     \return true on success, false on failure and cancelled when storing has been cancelled. */
    tristate storeDataAs(KexiPart::Item *item, KexiView::StoreNewDataOptions options);

    /*! Reimplemented - we're informing the current view about performed
     detaching by calling KexiView::parentDialogDetached(), so the view
     can react on this event
     (by default KexiView::parentDialogDetached() does nothing, you can
     reimplement it). */
    void sendDetachedStateToCurrentView();

    /*! W're informing the current view about performed atttaching by calling
     KexiView::parentDialogAttached(), so the view can react on this event
     (by default KexiView::parentDialogAttached() does nothing, you can
     reimplement it). */
    void sendAttachedStateToCurrentView();

    /*! Saves settings for this window, for all views.
        @see KexiView::saveSettings() */
    bool saveSettings();

Q_SIGNALS:
    void updateContextHelp();

    //! emitted when the window is about to close
    void closing();

    /*! Emitted to inform the world that 'dirty' flag changes.
     Activated by KexiView::setDirty(). */
    void dirtyChanged(KexiWindow*);

protected Q_SLOTS:
    /*!  Sets 'dirty' flag on every dialog's view. */
    void setDirty(bool dirty);

    /*! Switches this dialog to \a newViewMode.
     \a viewMode is one of Kexi::ViewMode enum elements.
     \return true for successful switching
     True is returned also if user has cancelled switching
     (rarely, but for any reason) - cancelled is returned. */
    tristate switchToViewMode(Kexi::ViewMode newViewMode);

protected:
    //! Used by KexiPart::Part
    KexiWindow(QWidget *parent, Kexi::ViewModes supportedViewModes, KexiPart::Part *part,
               KexiPart::Item *item);

    //! Used by KexiInternalPart
    KexiWindow();

    /*! Used by Part::openInstance(),
     like switchToViewMode( Kexi::ViewMode newViewMode ), but passed \a staticObjectArgs.
     Only used for parts of class KexiPart::StaticPart. */
    tristate switchToViewMode(Kexi::ViewMode newViewMode,
                              QMap<QString, QVariant>* staticObjectArgs,
                              bool *proposeOpeningInTextViewModeBecauseOfProblems);

    void registerWindow();

    virtual void closeEvent(QCloseEvent * e);

    //! \internal
    void addView(KexiView *view, Kexi::ViewMode mode);

    //! \internal
    void removeView(Kexi::ViewMode mode);

    //! \internal
    virtual bool eventFilter(QObject *obj, QEvent *e);

    //! Used by \a view to inform the dialog about changing state of the "dirty" flag.
    void dirtyChanged(KexiView* view);

    bool isDesignModePreloadedForTextModeHackUsed(Kexi::ViewMode newViewMode) const;

    /*! Created view's mode - helper for switchToViewMode(),
     KexiView ctor uses that info. >0 values are useful. */
    Kexi::ViewMode creatingViewsMode() const;

    /*! Sets temporary data shared between views. */
    void setData(KexiWindowData* data);

    //! Used by KexiView
    QVariant internalPropertyValue(const QByteArray& name,
                                   const QVariant& defaultValue = QVariant()) const;

    //! Sets schema object associated with this window to @a schemaObject
    void setSchemaObject(KDbObject* schemaObject);

private Q_SLOTS:
    /*! Helper, calls KexiMainWindowIface::switchToViewMode() which in turn calls KexiWindow::switchToViewMode()
     to get error handling and reporting as well on main window level. */
    tristate switchToViewModeInternal(Kexi::ViewMode newViewMode);

private:
    //! Closes the window and all views. If @a force is true, attempts to close every
    //! view even if one of them refuses to close. If @a force is false, false is returned
    //! as soon as first view refuses to close.
    //! @return true on sucessfull close; forced close always returns true
    bool close(bool force = false);

    void createSubwidgets();
    void removeView(KexiView *view);

    class Private;
    Private *d;

    bool m_destroying; //!< true after entering to the dctor

    friend class KexiMainWindow;
    friend class KexiPart::Part;
    friend class KexiInternalPart;
    friend class KexiView;
};

#endif
