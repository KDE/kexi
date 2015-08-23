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

#include "kexipart.h"
#include "kexipartinfo.h"
#include "kexipartitem.h"
//! @todo KEXI3 #include "kexistaticpart.h"
#include "KexiWindow.h"
#include "KexiWindowData.h"
#include "KexiView.h"
#include "kexipartguiclient.h"
#include "KexiMainWindowIface.h"
#include "kexi.h"
#include <kexiutils/utils.h>

#include <KDb>
#include <KDbConnection>

#include <KActionCollection>
#include <KMessageBox>

#include <QDebug>

namespace KexiPart
{
//! @internal
class Part::Private
{
public:
    Private()
    : guiClient(0)
    , newObjectsAreDirty(false)
    , instanceActionsInitialized(false)
    {
    }

    //! Helper, used in Part::openInstance()
    tristate askForOpeningInTextMode(KexiWindow *window, KexiPart::Item *item,
                                     Kexi::ViewModes supportedViewModes, Kexi::ViewMode viewMode) {
        if (viewMode != Kexi::TextViewMode
                && supportedViewModes & Kexi::TextViewMode
                && window->data()->proposeOpeningInTextViewModeBecauseOfProblems) {
            //ask
            KexiUtils::WaitCursorRemover remover;
            //! @todo use message handler for this to enable non-gui apps
            QString singleStatusString(window->singleStatusString());
            if (!singleStatusString.isEmpty())
                singleStatusString.prepend(QString("\n\n") + xi18n("Details:") + " ");
            if (KMessageBox::No == KMessageBox::questionYesNo(0,
                    ((viewMode == Kexi::DesignViewMode)
                     ? xi18n("Object \"%1\" could not be opened in Design View.", item->name())
                     : xi18n("Object could not be opened in Data View.")) + "\n"
                    + xi18n("Do you want to open it in Text View?") + singleStatusString, 0,
                    KStandardGuiItem::open(), KStandardGuiItem::cancel())) {
                return false;
            }
            return true;
        }
        return cancelled;
    }

    QString toolTip;
    QString whatsThis;
    QString instanceName;

    GUIClient *guiClient;
    QMap<int, GUIClient*> instanceGuiClients;
    Kexi::ObjectStatus status;

    bool newObjectsAreDirty;
    bool instanceActionsInitialized;
};
}

//----------------------------------------------------------------

using namespace KexiPart;

Part::Part(QObject *parent,
           const QString& instanceName,
           const QString& toolTip,
           const QString& whatsThis,
           const QVariantList& list)
    : PartBase(parent, list)
    , d(new Private())
{
    d->instanceName = KDb::stringToIdentifier(
        instanceName.isEmpty()
        ? xi18nc("Translate this word using only lowercase alphanumeric characters (a..z, 0..9). "
                "Use '_' character instead of spaces. First character should be a..z character. "
                "If you cannot use latin characters in your language, use english word.",
                "object").toLower()
        : instanceName);
    d->toolTip = toolTip;
    d->whatsThis = whatsThis;
}

/*! @todo KEXI3
Part::Part(QObject* parent, StaticPartInfo *info)
    : PartBase(parent, QVariantList())
        , d(new Private())
{
    setObjectName("StaticPart");
    setInfo(info);
}*/

Part::~Part()
{
    delete d;
}

void Part::createGUIClients()//KexiMainWindow *win)
{
    if (!d->guiClient) {
        //create part's gui client
        d->guiClient = new GUIClient(this, false, "part");

        //default actions for part's gui client:
        QAction* act = info()->newObjectAction();
        // - update action's tooltip and "what's this"
        QString tip(toolTip());
        if (!tip.isEmpty()) {
            act->setToolTip(tip);
        }
        QString what(whatsThis());
        if (!what.isEmpty()) {
            act->setWhatsThis(what);
        }

        //default actions for part instance's gui client:
        //NONE
        //let init specific actions for part instances
        for (int mode = 1; mode <= 0x01000; mode <<= 1) {
            if (info()->supportedViewModes() & (Kexi::ViewMode)mode) {
                GUIClient *instanceGuiClient = new GUIClient(
                    this, true, Kexi::nameForViewMode((Kexi::ViewMode)mode).toLatin1());
                d->instanceGuiClients.insert((Kexi::ViewMode)mode, instanceGuiClient);
            }
        }
        // also add an instance common for all modes (mode==0)
        GUIClient *instanceGuiClient = new GUIClient(this, true, "allViews");
        d->instanceGuiClients.insert(Kexi::AllViewModes, instanceGuiClient);

        initPartActions();
    }
}

KActionCollection* Part::actionCollectionForMode(Kexi::ViewMode viewMode) const
{
    GUIClient *cli = d->instanceGuiClients.value((int)viewMode);
    return cli ? cli->actionCollection() : 0;
}

QAction * Part::createSharedAction(Kexi::ViewMode mode, const QString &text,
                                  const QString &pix_name, const QKeySequence &cut, const char *name,
                                  const char *subclassName)
{
    GUIClient *instanceGuiClient = d->instanceGuiClients.value((int)mode);
    if (!instanceGuiClient) {
        qWarning() << "no gui client for mode " << mode << "!";
        return 0;
    }
    return KexiMainWindowIface::global()->createSharedAction(text, pix_name, cut, name,
            instanceGuiClient->actionCollection(), subclassName);
}

QAction * Part::createSharedPartAction(const QString &text,
                                      const QString &pix_name, const QKeySequence &cut, const char *name,
                                      const char *subclassName)
{
    if (!d->guiClient)
        return 0;
    return KexiMainWindowIface::global()->createSharedAction(text, pix_name, cut, name,
            d->guiClient->actionCollection(), subclassName);
}

QAction * Part::createSharedToggleAction(Kexi::ViewMode mode, const QString &text,
                                        const QString &pix_name, const QKeySequence &cut, const char *name)
{
    return createSharedAction(mode, text, pix_name, cut, name, "KToggleAction");
}

QAction * Part::createSharedPartToggleAction(const QString &text,
        const QString &pix_name, const QKeySequence &cut, const char *name)
{
    return createSharedPartAction(text, pix_name, cut, name, "KToggleAction");
}

void Part::setActionAvailable(const char *action_name, bool avail)
{
    for (QMap<int, GUIClient*>::Iterator it = d->instanceGuiClients.begin(); it != d->instanceGuiClients.end(); ++it) {
        QAction *act = it.value()->actionCollection()->action(action_name);
        if (act) {
            act->setEnabled(avail);
            return;
        }
    }
    KexiMainWindowIface::global()->setActionAvailable(action_name, avail);
}

KexiWindow* Part::openInstance(QWidget* parent, KexiPart::Item *item, Kexi::ViewMode viewMode,
                               QMap<QString, QVariant>* staticObjectArgs)
{
    Q_ASSERT(item);
    //now it's the time for creating instance actions
    if (!d->instanceActionsInitialized) {
        initInstanceActions();
        d->instanceActionsInitialized = true;
    }

    d->status.clearStatus();
    KexiWindow *window = new KexiWindow(parent,
                                        info()->supportedViewModes(), this, item);

    KexiProject *project = KexiMainWindowIface::global()->project();
    KDbObject object(project->typeIdForPluginId(info()->pluginId()));
    object.setName(item->name());
    object.setCaption(item->caption());
    object.setDescription(item->description());

    /*! @todo js: apply settings for caption displaying method; there can be option for
     - displaying item.caption() as caption, if not empty, without instanceName
     - displaying the same as above in tabCaption (or not) */
    window->setId(item->identifier()); //not needed, but we did it
    window->setWindowIcon(QIcon::fromTheme(window->iconName()));
    KexiWindowData *windowData = createWindowData(window);
    if (!windowData) {
        d->status = Kexi::ObjectStatus(KexiMainWindowIface::global()->project()->dbConnection(),
                                       xi18n("Could not create object's window."), xi18n("The plugin or object definition may be corrupted."));
        delete window;
        return 0;
    }
    window->setData(windowData);

    if (!item->neverSaved()) {
        //we have to load object data for this dialog
        loadAndSetSchemaObject(window, object, viewMode);
        if (!window->schemaObject()) {
            //last chance:
            if (false == d->askForOpeningInTextMode(
                        window, item, window->supportedViewModes(), viewMode)) {
                delete window;
                return 0;
            }
            viewMode = Kexi::TextViewMode;
            loadAndSetSchemaObject(window, object, viewMode);
        }
        if (!window->schemaObject()) {
            if (!d->status.error())
                d->status = Kexi::ObjectStatus(KexiMainWindowIface::global()->project()->dbConnection(),
                                               xi18n("Could not load object's definition."), xi18n("Object design may be corrupted."));
            d->status.append(
                Kexi::ObjectStatus(xi18n("You can delete \"%1\" object and create it again.",
                                        item->name()), QString()));

            window->close();
            delete window;
            return 0;
        }
    }

    bool switchingFailed = false;
    bool dummy;
    tristate res = window->switchToViewMode(viewMode, staticObjectArgs, &dummy);
    if (!res) {
        tristate askForOpeningInTextModeRes
        = d->askForOpeningInTextMode(window, item, window->supportedViewModes(), viewMode);
        if (true == askForOpeningInTextModeRes) {
            delete window->schemaObject(); //old one
            window->close();
            delete window;
            //try in text mode
            return openInstance(parent, item, Kexi::TextViewMode, staticObjectArgs);
        } else if (false == askForOpeningInTextModeRes) {
            delete window->schemaObject(); //old one
            window->close();
            delete window;
            qWarning() << "!window, cannot switch to a view mode" <<
                Kexi::nameForViewMode(viewMode);
            return 0;
        }
        //the window has an error info
        switchingFailed = true;
    }
    if (~res)
        switchingFailed = true;

    if (switchingFailed) {
        d->status = window->status();
        window->close();
        delete window;
        qWarning() << "!window, switching to view mode failed, " <<
            Kexi::nameForViewMode(viewMode);
        return 0;
    }
    window->registerWindow(); //ok?
    window->show();

    window->setMinimumSize(window->minimumSizeHint().width(), window->minimumSizeHint().height());

    //dirty only if it's a new object
    if (window->selectedView()) {
        window->selectedView()->setDirty(
            internalPropertyValue("newObjectsAreDirty", false).toBool() ? item->neverSaved() : false);
    }
    return window;
}

KDbObject* Part::loadSchemaObject(KexiWindow *window, const KDbObject& object,
        Kexi::ViewMode viewMode, bool *ownedByWindow)
{
    Q_UNUSED(window);
    Q_UNUSED(viewMode);
    KDbObject *newObject = new KDbObject();
    *newObject = object;
    if (ownedByWindow)
        *ownedByWindow = true;
    return newObject;
}

void Part::loadAndSetSchemaObject(KexiWindow *window, const KDbObject& object,
    Kexi::ViewMode viewMode)
{
    bool schemaObjectOwned = true;
    KDbObject* sd = loadSchemaObject(window, object, viewMode, &schemaObjectOwned);
    window->setSchemaObject(sd);
    window->setSchemaObjectOwned(schemaObjectOwned);
}

bool Part::loadDataBlock(KexiWindow *window, QString *dataString, const QString& dataID)
{
    if (!KexiMainWindowIface::global()->project()->dbConnection()->loadDataBlock(
                window->id(), dataString, dataID)) {
        d->status = Kexi::ObjectStatus(KexiMainWindowIface::global()->project()->dbConnection(),
                                       xi18n("Could not load object's data."),
                                       xi18n("Data identifier: \"%1\".", dataID));
        d->status.append(*window);
        return false;
    }
    return true;
}

void Part::initPartActions()
{
}

void Part::initInstanceActions()
{
}

tristate Part::remove(KexiPart::Item *item)
{
    Q_ASSERT(item);
    KDbConnection *conn = KexiMainWindowIface::global()->project()->dbConnection();
    if (!conn)
        return false;
    return conn->removeObject(item->identifier());
}

KexiWindowData* Part::createWindowData(KexiWindow* window)
{
    return new KexiWindowData(window);
}

QString Part::instanceName() const
{
    return d->instanceName;
}

QString Part::toolTip() const
{
    return d->toolTip;
}

QString Part::whatsThis() const
{
    return d->whatsThis;
}

tristate Part::rename(KexiPart::Item *item, const QString& newName)
{
    Q_UNUSED(item);
    Q_UNUSED(newName);
    return true;
}

GUIClient* Part::instanceGuiClient(Kexi::ViewMode mode) const
{
    return d->instanceGuiClients.value((int)mode);
}

GUIClient* Part::guiClient() const
{
    return d->guiClient;
}

const Kexi::ObjectStatus& Part::lastOperationStatus() const
{
    return d->status;
}

KDbQuerySchema* Part::currentQuery(KexiView* view)
{
    Q_UNUSED(view);
    return 0;
}

KEXICORE_EXPORT QString KexiPart::fullCaptionForItem(KexiPart::Item *item, KexiPart::Part *part)
{
    Q_ASSERT(item);
    Q_ASSERT(part);
    if (part)
        return item->name() + " : " + part->info()->name();
    return item->name();
}
