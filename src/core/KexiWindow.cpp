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

#include "KexiWindow.h"
#include "KexiWindowData.h"
#include "KexiView.h"
#include "KexiMainWindowIface.h"
#include "kexipart.h"
//! @todo KEXI3 #include "kexistaticpart.h"
#include "kexipartitem.h"
#include "kexipartinfo.h"
#include "kexiproject.h"
#include <kexiutils/utils.h>
#include <kexiutils/SmallToolButton.h>
#include <kexiutils/FlowLayout.h>

#include <KDbConnection>
#include <KDbTransactionGuard>

#include <KStandardGuiItem>
#include <KMessageBox>

#include <QStackedWidget>
#include <QEvent>
#include <QCloseEvent>
#include <QDebug>

//----------------------------------------------------------

//! @internal
class Q_DECL_HIDDEN KexiWindow::Private
{
public:
    explicit Private(KexiWindow *window)
            : win(window)
            , schemaObject(0)
            , schemaObjectOwned(false)
            , isRegistered(false)
            , dirtyChangedEnabled(true)
            , switchToViewModeEnabled(true)
    {
        supportedViewModes = Kexi::NoViewMode; //will be set by KexiPart
        openedViewModes = Kexi::NoViewMode;
        currentViewMode = Kexi::NoViewMode; //no view available yet
        creatingViewsMode = Kexi::NoViewMode;
        id = -1;
        item = 0;
    }

    ~Private() {
        setSchemaObject(0);
    }

    void setSchemaObject(KDbObject* data)
    {
        if (schemaObjectOwned) {
            delete schemaObject;
        }
        schemaObject = data;
    }

    bool setupSchemaObject(KDbObject *object, KexiPart::Item *item,
                           KexiView::StoreNewDataOptions options) const
    {
        object->setName(item->name());
        object->setCaption(item->caption());
        object->setDescription(item->description());

        KexiProject *project = KexiMainWindowIface::global()->project();
        KexiPart::Item* existingItem = project->item(part->info(), object->name());
        if (existingItem && !(options & KexiView::OverwriteExistingData)) {
            KMessageBox::information(win,
                                     xi18n("Could not create new object.")
                                     + win->part()->i18nMessage("Object <resource>%1</resource> already exists.", win)
                                       .subs(object->name()).toString());
            return false;
        }
        return true;
    }

    KexiWindow *win;
    QVBoxLayout* mainLyr;
    QStackedWidget* stack;

    Kexi::ViewModes supportedViewModes;
    Kexi::ViewModes openedViewModes;
    Kexi::ViewMode currentViewMode;

#ifdef KEXI_SHOW_CONTEXT_HELP
    KexiContextHelpInfo *contextHelpInfo;
#endif
    int id;
    QPointer<KexiPart::Part> part;
    KexiPart::Item *item;
    KDbObject* schemaObject;
    bool schemaObjectOwned;
    QPointer<KexiView> newlySelectedView; //!< Used in isDirty(), temporary set in switchToViewMode()
    //!< during view setup, when a new view is not yet raised.
    //! Used in viewThatRecentlySetDirtyFlag(), modified in dirtyChanged().
    QPointer<KexiView> viewThatRecentlySetDirtyFlag;
    QPointer<KexiWindowData> data; //!< temporary data shared between views

    /*! Created view's mode - helper for switchToViewMode(),
     KexiView ctor uses that info. >0 values are useful. */
    Kexi::ViewMode creatingViewsMode;

    bool isRegistered;
    bool dirtyChangedEnabled; //!< used in setDirty(), affects dirtyChanged()
    bool switchToViewModeEnabled; //!< used internally switchToViewMode() to avoid infinite loop
    QMap<Kexi::ViewMode, KexiView*> views;
};

//----------------------------------------------------------

KexiWindow::KexiWindow(QWidget *parent, Kexi::ViewModes supportedViewModes,
                       KexiPart::Part *part, KexiPart::Item *item)
        : QWidget(parent)
        , KexiActionProxy(this, KexiMainWindowIface::global())
        , d(new Private(this))
        , m_destroying(false)
{
    d->part = part;
    d->item = item;
    d->supportedViewModes = supportedViewModes;
    createSubwidgets();
#ifdef KEXI_SHOW_CONTEXT_HELP
    d->contextHelpInfo = new KexiContextHelpInfo();
#endif
    updateCaption();
}

KexiWindow::KexiWindow()
        : QWidget(0)
        , KexiActionProxy(this, KexiMainWindowIface::global())
        , d(new Private(this))
        , m_destroying(false)
{
    createSubwidgets();
#ifdef KEXI_SHOW_CONTEXT_HELP
    d->contextHelpInfo = new KexiContextHelpInfo();
#endif
    updateCaption();
}

KexiWindow::~KexiWindow()
{
    close(true /*force*/);
    m_destroying = true;
    delete d;
    d = 0;
}

void KexiWindow::createSubwidgets()
{
    d->mainLyr = new QVBoxLayout(this);
    d->mainLyr->setContentsMargins(0, 0, 0, 0);
    d->stack = new QStackedWidget(this);
    d->mainLyr->addWidget(d->stack);
}

KexiView *KexiWindow::selectedView() const
{
    if (m_destroying)
        return 0;
    return static_cast<KexiView*>(d->stack->currentWidget());
}

KexiView *KexiWindow::viewForMode(Kexi::ViewMode mode) const
{
    return d->views.value(mode);
}

void KexiWindow::addView(KexiView *view)
{
    addView(view, Kexi::NoViewMode);
}

void KexiWindow::addView(KexiView *view, Kexi::ViewMode mode)
{
    d->stack->addWidget(view);
    d->views.insert(mode, view);
    d->openedViewModes |= mode;
}

void KexiWindow::removeView(Kexi::ViewMode mode)
{
    removeView(viewForMode(mode));
    d->openedViewModes |= mode;
    d->openedViewModes ^= mode;
}

void KexiWindow::removeView(KexiView *view)
{
    if (view) {
        d->stack->removeWidget(view);
        d->views.remove(view->viewMode());
        d->openedViewModes |= view->viewMode();
        d->openedViewModes ^= view->viewMode();
    }
}

QSize KexiWindow::minimumSizeHint() const
{
    KexiView *v = selectedView();
    if (!v)
        return QWidget::minimumSizeHint();
    return v->minimumSizeHint();
}

QSize KexiWindow::sizeHint() const
{
    KexiView *v = selectedView();
    if (!v)
        return QWidget::sizeHint();
    return v->preferredSizeHint(v->sizeHint());
}

void KexiWindow::setId(int id)
{
    d->id = id;
}

KexiPart::Part* KexiWindow::part() const
{
    return d->part;
}

KexiPart::Item *KexiWindow::partItem() const
{
    return d->item;
}

bool KexiWindow::supportsViewMode(Kexi::ViewMode mode) const
{
    return d->supportedViewModes & mode;
}

Kexi::ViewModes KexiWindow::supportedViewModes() const
{
    return d->supportedViewModes;
}

Kexi::ViewMode KexiWindow::currentViewMode() const
{
    return d->currentViewMode;
}

KexiView* KexiWindow::viewThatRecentlySetDirtyFlag() const
{
    return d->viewThatRecentlySetDirtyFlag;
}

void KexiWindow::registerWindow()
{
    if (d->isRegistered)
        return;
    KexiMainWindowIface::global()->registerChild(this);
    d->isRegistered = true;
}

bool KexiWindow::isRegistered() const
{
    return d->isRegistered;
}

int KexiWindow::id() const
{
    return (partItem() && partItem()->identifier() > 0)
           ? partItem()->identifier() : d->id;
}

void KexiWindow::setContextHelp(const QString& caption,
                                const QString& text, const QString& iconName)
{
#ifdef KEXI_SHOW_CONTEXT_HELP
    d->contextHelpInfo->caption = caption;
    d->contextHelpInfo->text = text;
    d->contextHelpInfo->text = iconName;
    updateContextHelp();
#else
    Q_UNUSED(caption);
    Q_UNUSED(text);
    Q_UNUSED(iconName);
#endif
}

bool KexiWindow::close(bool force)
{
    KexiMainWindowIface::global()->acceptPropertySetEditing();

    //let any view send "closing" signal
    QList<KexiView *> list(findChildren<KexiView*>());
    QList< QPointer<KexiView> > listPtr;
    foreach(KexiView * view, list) { // use QPointers for sanity
        listPtr.append(QPointer<KexiView>(view));
    }
    foreach(QPointer<KexiView> viewPtr, listPtr) {
        if (viewPtr && viewPtr->parent() == d->stack) {
            bool cancel = false;
            emit viewPtr->closing(&cancel);
            if (!force && cancel) {
                     return false;
            }
        }
    }
    emit closing();
    foreach(QPointer<KexiView> viewPtr, listPtr) {
        if (viewPtr && viewPtr->parent() == d->stack) {
            removeView(viewPtr.data());
            delete viewPtr.data();
        }
    }
    return true;
}

void KexiWindow::closeEvent(QCloseEvent * e)
{
    if (!close(false /* !force*/)) {
        e->ignore();
        return;
    }
    QWidget::closeEvent(e);
}

bool KexiWindow::isDirty() const
{
    //look for "dirty" flag
    int m = d->openedViewModes;
    int mode = 1;
    while (m > 0) {
        if (m & 1) {
            KexiView *view = viewForMode(static_cast<Kexi::ViewMode>(mode));
            if (view && view->isDirty()) {
                return true;
            }
        }
        m >>= 1;
        mode <<= 1;
    }
    return false;
}

void KexiWindow::setDirty(bool dirty)
{
    d->dirtyChangedEnabled = false;
    int m = d->openedViewModes;
    int mode = 1;
    while (m > 0) {
        if (m & 1) {
            KexiView *view = viewForMode(static_cast<Kexi::ViewMode>(mode));
            if (view) {
                view->setDirty(dirty);
            }
        }
        m >>= 1;
        mode <<= 1;
    }
    d->dirtyChangedEnabled = true;
    dirtyChanged(d->viewThatRecentlySetDirtyFlag); //update
}

QString KexiWindow::iconName()
{
    if (!d->part || !d->part->info()) {
        KexiView *v = selectedView();
        if (v) {
            return v->defaultIconName();
        }
        return QString();
    }
    return d->part->info()->iconName();
}

KexiPart::GUIClient* KexiWindow::guiClient() const
{
    if (!d->part || d->currentViewMode == 0)
        return 0;
    return d->part->instanceGuiClient(d->currentViewMode);
}

KexiPart::GUIClient* KexiWindow::commonGUIClient() const
{
    if (!d->part)
        return 0;
    return d->part->instanceGuiClient(Kexi::AllViewModes);
}

bool KexiWindow::isDesignModePreloadedForTextModeHackUsed(Kexi::ViewMode newViewMode) const
{
    return newViewMode == Kexi::TextViewMode
           && !viewForMode(Kexi::DesignViewMode)
           && supportsViewMode(Kexi::DesignViewMode);
}

tristate KexiWindow::switchToViewMode(
    Kexi::ViewMode newViewMode,
    QMap<QString, QVariant>* staticObjectArgs,
    bool *proposeOpeningInTextViewModeBecauseOfProblems)
{
    Q_ASSERT(proposeOpeningInTextViewModeBecauseOfProblems);
    clearStatus();
    KexiMainWindowIface::global()->acceptPropertySetEditing();

    const bool designModePreloadedForTextModeHack = isDesignModePreloadedForTextModeHackUsed(newViewMode);
    tristate res = true;
    if (designModePreloadedForTextModeHack) {
        /* A HACK: open design BEFORE text mode: otherwise Query schema becames crazy */
        bool _proposeOpeningInTextViewModeBecauseOfProblems = false; // used because even if opening the view failed,
        // text view can be opened
        res = switchToViewMode(Kexi::DesignViewMode, staticObjectArgs, &_proposeOpeningInTextViewModeBecauseOfProblems);
        if ((!res && !_proposeOpeningInTextViewModeBecauseOfProblems) || ~res)
            return res;
    }

    bool dontStore = false;
    KexiView *view = selectedView();

    if (d->currentViewMode == newViewMode)
        return true;
    if (!supportsViewMode(newViewMode)) {
        qWarning() << "!" << Kexi::nameForViewMode(newViewMode);
        return false;
    }

    if (view) {
        res = true;
        if (view->isDataEditingInProgress()) {
            KGuiItem saveItem(KStandardGuiItem::save());
            saveItem.setText(xi18n("Save Changes"));
            KGuiItem dontSaveItem(KStandardGuiItem::dontSave());
            KGuiItem cancelItem(KStandardGuiItem::cancel());
            cancelItem.setText(xi18n("Do Not Switch"));
            const int res = KMessageBox::questionTwoActionsCancel(
                selectedView(),
                xi18nc("@info",
                       "<para>There are unsaved changes in object <resource>%1</resource>.</para>"
                       "<para>Do you want to save these changes before switching to other "
                       "view?</para>",
                       partItem()->captionOrName()),
                xi18n("Confirm Saving Changes"), saveItem, dontSaveItem, cancelItem, QString(),
                KMessageBox::Notify | KMessageBox::Dangerous);
            if (res == KMessageBox::PrimaryAction) {
                if (true != view->saveDataChanges())
                    return cancelled;
            }
            else if (res == KMessageBox::No) {
                if (true != view->cancelDataChanges())
                    return cancelled;
            }
            else { // Cancel:
                return cancelled;
            }
        }
        if (!designModePreloadedForTextModeHack) {
            const bool wasDirty = view->isDirty(); // remember and restore the flag if the view was clean
            res = view->beforeSwitchTo(newViewMode, &dontStore);
            if (!wasDirty) {
                view->setDirty(false);
            }
        }
        if (~res || !res)
            return res;
        if (!dontStore && view->isDirty()) {
            res = KexiMainWindowIface::global()->saveObject(this, xi18n("Design has been changed. "
                    "You must save it before switching to other view."));
            if (~res || !res)
                return res;
//   KMessageBox::questionTwoActions(0, xi18n("Design has been changed. You must save it before switching to other view."))
//    ==KMessageBox::No
        }
    }

    //get view for viewMode
    KexiView *newView = viewForMode(newViewMode);
    if (newView && !newView->inherits("KexiView")) {
        newView = 0;
    }
    if (!newView) {
        KexiUtils::setWaitCursor();
        //ask the part to create view for the new mode
        d->creatingViewsMode = newViewMode;
/*! @todo KEXI3 StaticPart
        KexiPart::StaticPart *staticPart = dynamic_cast<KexiPart::StaticPart*>((KexiPart::Part*)d->part);
        if (staticPart)
            newView = staticPart->createView(this, this, d->item, newViewMode, staticObjectArgs);
        else*/
            newView = d->part->createView(this, this, d->item, newViewMode, staticObjectArgs);
        KexiUtils::removeWaitCursor();
        if (!newView) {
            //js TODO error?
            qWarning() << "Switching to mode" << newViewMode << "failed. Previous mode "
            << d->currentViewMode << "restored.";
            return false;
        }
        d->creatingViewsMode = Kexi::NoViewMode;
        newView->initViewActions();
        newView->initMainMenuActions();
        addView(newView, newViewMode);
    }
    const Kexi::ViewMode prevViewMode = d->currentViewMode;
    res = true;
    if (designModePreloadedForTextModeHack) {
        d->currentViewMode = Kexi::NoViewMode; //SAFE?
    }
    bool wasDirty = newView->isDirty(); // remember and restore the flag if the view was clean
    res = newView->beforeSwitchTo(newViewMode, &dontStore);
    if (!wasDirty) {
        newView->setDirty(false);
    }
    *proposeOpeningInTextViewModeBecauseOfProblems
        = data()->proposeOpeningInTextViewModeBecauseOfProblems;
    if (!res) {
        removeView(newViewMode);
        delete newView;
        qWarning() << "Switching to mode" << newViewMode << "failed. Previous mode"
                   << d->currentViewMode << "restored.";
        return false;
    }
    d->currentViewMode = newViewMode;
    d->newlySelectedView = newView;
    if (prevViewMode == Kexi::NoViewMode)
        d->newlySelectedView->setDirty(false);

    if ((prevViewMode == Kexi::DesignViewMode && d->currentViewMode == Kexi::TextViewMode)
            || (prevViewMode == Kexi::TextViewMode && d->currentViewMode == Kexi::DesignViewMode)) {
        if (view) {
            wasDirty = view->isDirty(); // synchronize the dirty flag between Design and Text views
        }
    } else {
        wasDirty = newView->isDirty(); // remember and restore the flag if the view was clean
    }

    res = newView->afterSwitchFrom(
              designModePreloadedForTextModeHack ? Kexi::NoViewMode : prevViewMode);
    newView->setDirty(wasDirty);

    *proposeOpeningInTextViewModeBecauseOfProblems
        = data()->proposeOpeningInTextViewModeBecauseOfProblems;
    if (!res) {
        removeView(newViewMode);
        delete newView;
        qWarning() << "Switching to mode" << newViewMode << "failed. Previous mode"
                   << prevViewMode << "restored.";
        const Kexi::ObjectStatus status(*this);
        setStatus(KexiMainWindowIface::global()->project()->dbConnection(),
                  xi18n("Switching to other view failed (%1).", Kexi::nameForViewMode(newViewMode)), "");
        append(status);
        d->currentViewMode = prevViewMode;
        return false;
    }
    d->newlySelectedView = 0;
    if (~res) {
        d->currentViewMode = prevViewMode;
        return cancelled;
    }
    if (view) {
        takeActionProxyChild(view);   //take current proxy child
        // views have distinct local toolbars, and user has switched the mode button so switch it back
        //view->toggleViewModeButtonBack();
    }
    addActionProxyChild(newView);   //new proxy child
    d->stack->setCurrentWidget(newView);
    newView->propertySetSwitched();
    KexiMainWindowIface::global()->invalidateSharedActions(newView);
    newView->setFocus();
    return true;
}

tristate KexiWindow::switchToViewModeInternal(Kexi::ViewMode newViewMode)
{
    return KexiMainWindowIface::global()->switchToViewMode(*this, newViewMode);
}

tristate KexiWindow::switchToViewMode(Kexi::ViewMode newViewMode)
{
    if (newViewMode == d->currentViewMode)
        return true;
    if (!d->switchToViewModeEnabled)
        return false;
    bool dummy;
    return switchToViewMode(newViewMode, 0, &dummy);
}

void KexiWindow::setFocus()
{
    if (d->stack->currentWidget()) {
        if (d->stack->currentWidget()->inherits("KexiView"))
            static_cast<KexiView*>(d->stack->currentWidget())->setFocus();
        else
            d->stack->currentWidget()->setFocus();
    } else {
        QWidget::setFocus();
    }
    activate();
}

KPropertySet*
KexiWindow::propertySet()
{
    KexiView *v = selectedView();
    if (!v)
        return 0;
    return v->propertySet();
}

void KexiWindow::setSchemaObject(KDbObject* object)
{
    d->setSchemaObject(object);
}

KDbObject* KexiWindow::schemaObject() const
{
    return d->schemaObject;
}

void KexiWindow::setSchemaObjectOwned(bool set)
{
    d->schemaObjectOwned = set;
}

KexiWindowData *KexiWindow::data() const
{
    return d->data;
}

void KexiWindow::setData(KexiWindowData* data)
{
    if (data != d->data)
        delete d->data;
    d->data = data;
}

bool KexiWindow::eventFilter(QObject *obj, QEvent *e)
{
    if (QWidget::eventFilter(obj, e))
        return true;
    /*if (e->type()==QEvent::FocusIn) {
      QWidget *w = m_parentWindow->activeWindow();
      w=0;
    }*/
    if ((e->type() == QEvent::FocusIn && KexiMainWindowIface::global()->currentWindow() == this)
            || e->type() == QEvent::MouseButtonPress) {
        if (d->stack->currentWidget() && KDbUtils::hasParent(d->stack->currentWidget(), obj)) {
            //pass the activation
            activate();
        }
    }
    return false;
}

void KexiWindow::dirtyChanged(KexiView* view)
{
    if (!d->dirtyChangedEnabled)
        return;
    d->viewThatRecentlySetDirtyFlag = isDirty() ? view : 0;
    updateCaption();
    emit dirtyChanged(this);
}

//static
QString KexiWindow::windowTitleForItem(const KexiPart::Item &item)
{
    return item.name();
}

void KexiWindow::updateCaption()
{
    if (!d->item || !d->part)
        return;
    const QString fullCapt(windowTitleForItem(*d->item));
    setWindowTitle(isDirty() ? xi18nc("@title:window with dirty indicator", "%1*", fullCapt)
                             : fullCapt);
}

bool KexiWindow::neverSaved() const
{
    return d->item ? d->item->neverSaved() : true;
}

tristate KexiWindow::storeNewData(KexiView::StoreNewDataOptions options)
{
    if (!neverSaved()) {
        return false;
    }
    if (d->schemaObject) {
        return false; //schema must not exist
    }
    KexiView *v = selectedView();
    if (!v) {
        return false;
    }
    //create schema object and assign information
    KexiProject *project = KexiMainWindowIface::global()->project();
    KDbObject object(project->typeIdForPluginId(d->part->info()->pluginId()));
    if (!d->setupSchemaObject(&object, d->item, options)) {
        return false;
    }

    bool cancel = false;
    d->schemaObject = v->storeNewData(object, options, &cancel);
    if (cancel)
        return cancelled;
    if (!d->schemaObject) {
        setStatus(project->dbConnection(), xi18n("Saving object's definition failed."), "");
        return false;
    }

    if (project->typeIdForPluginId(part()->info()->pluginId()) < 0) {
        if (!project->createIdForPart(*part()->info()))
            return false;
    }
    /* Sets 'dirty' flag on every dialog's view. */
    setDirty(false);
    //new object data has now ID updated to a unique value
    //-assign that to item's identifier
    d->item->setIdentifier(d->schemaObject->id());
    project->addStoredItem(part()->info(), d->item);

    return true;
}

tristate KexiWindow::storeData(bool dontAsk)
{
    if (neverSaved())
        return false;
    KexiView *v = selectedView();
    if (!v)
        return false;

#define storeData_ERR \
    setStatus(KexiMainWindowIface::global()->project()->dbConnection(), \
        xi18n("Saving object's data failed."),"");

    //save changes using transaction
    KDbTransaction transaction = KexiMainWindowIface::global()
                                      ->project()->dbConnection()->beginTransaction();
    if (transaction.isNull()) {
        storeData_ERR;
        return false;
    }
    KDbTransactionGuard tg(transaction);

    const tristate res = v->storeData(dontAsk);
    if (~res) //trans. will be cancelled
        return res;
    if (!res) {
        storeData_ERR;
        return res;
    }
    if (!tg.commit()) {
        storeData_ERR;
        return false;
    }
    /* Sets 'dirty' flag on every dialog's view. */
    setDirty(false);
    return true;
}

tristate KexiWindow::storeDataAs(KexiPart::Item *item, KexiView::StoreNewDataOptions options)
{
    if (neverSaved()) {
        qWarning() << "The data was never saved, so storeNewData() should be called instead, giving up.";
        return false;
    }
    KexiView *v = selectedView();
    if (!v) {
        return false;
    }
    //create schema object and assign information
    KexiProject *project = KexiMainWindowIface::global()->project();
    KDbObject object(project->typeIdForPluginId(d->part->info()->pluginId()));
    if (!d->setupSchemaObject(&object, item, options)) {
        return false;
    }

    bool cancel = false;
    KDbObject *newSchemaObject;
    if (isDirty()) { // full save of new data
        newSchemaObject = v->storeNewData(object, options, &cancel);
    }
    else { // there were no changes; full copy of the data is enough
           // - gives better performance (e.g. tables are copied on server side)
           // - works without bothering the user (no unnecessary questions)
        newSchemaObject = v->copyData(object, options, &cancel);
    }

    if (cancel) {
        return cancelled;
    }
    if (!newSchemaObject) {
        setStatus(project->dbConnection(), xi18n("Saving object's definition failed."), "");
        return false;
    }
    setSchemaObject(newSchemaObject); // deletes previous schema if owned

    if (project->typeIdForPluginId(part()->info()->pluginId()) < 0) {
        if (!project->createIdForPart(*part()->info()))
            return false;
    }
    // clear 'dirty' for old window
    setDirty(false);
    // for now this Window has new item assigned
    d->item = item;

    // new object data has now ID updated to a unique value
    // -assign that to item's identifier
    item->setIdentifier(d->schemaObject->id());

    project->addStoredItem(part()->info(), d->item);

    // set 'dirty' flag on every dialog's view
    setDirty(false);

    return true;
}

void KexiWindow::activate()
{
    KexiView *v = selectedView();
    //qDebug() << "focusWidget():" << focusWidget()->name();
    if (!KDbUtils::hasParent(v, KexiMainWindowIface::global()->focusWidget())) {
        //ah, focused widget is not in this view, move focus:
        if (v)
            v->setFocus();
    }
    if (v)
        v->updateActions(true);
}

void KexiWindow::deactivate()
{
    KexiView *v = selectedView();
    if (v)
        v->updateActions(false);
}

void KexiWindow::sendDetachedStateToCurrentView()
{
    KexiView *v = selectedView();
    if (v)
        v->windowDetached();
}

void KexiWindow::sendAttachedStateToCurrentView()
{
    KexiView *v = selectedView();
    if (v)
        v->windowAttached();
}

bool KexiWindow::saveSettings()
{
    bool result = true;
    for (int i = 0; i < d->stack->count(); ++i) {
        KexiView *view = qobject_cast<KexiView*>(d->stack->widget(i));
        if (!view->saveSettings()) {
            result = false;
        }
    }
    return result;
}

Kexi::ViewMode KexiWindow::creatingViewsMode() const
{
    return d->creatingViewsMode;
}

QVariant KexiWindow::internalPropertyValue(const QByteArray& name,
        const QVariant& defaultValue) const
{
    return d->part->internalPropertyValue(name, defaultValue);
}

