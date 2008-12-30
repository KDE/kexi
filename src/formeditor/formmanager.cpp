/* This file is part of the KDE project
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2005 Jarosław Staniek <staniek@kde.org>

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

#include <kdebug.h>

#include <qworkspace.h>
#include <qcursor.h>
#include <qstring.h>
#include <qlabel.h>
#include <qstylefactory.h>
#include <qmetaobject.h>
#include <qregexp.h>
#include <q3vbox.h>
#include <QList>

#include <klocale.h>
#include <kiconloader.h>
#include <kmenu.h>
#include <kstandardaction.h>
#include <kaction.h>
#include <kxmlguiclient.h>
#include <kxmlguiwindow.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <kstyle.h>
#include <ktoggleaction.h>
#include <kselectaction.h>
#include <kapplication.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kdialog.h>
#include <ktextedit.h>
#include <ktabwidget.h>
#include <kfontdialog.h>
#include <KPageDialog>

#include <kdeversion.h>
#include <kactioncollection.h>

#include "widgetpropertyset.h"
#include "objecttree.h"
#include "widgetlibrary.h"
#include "form.h"
#include "container.h"
#include "formIO.h"
#include "objecttreeview.h"
#include "commands.h"
#include "tabstopdialog.h"
#include "connectiondialog.h"
#include "events.h"
#include "utils.h"
//todo #include "kfdpixmapedit.h"
#include <koproperty/EditorView.h>
#include <koproperty/Property.h>
#include <koproperty/Factory.h>
#include <kexiutils/utils.h>
#include <kexi_global.h>

#include "formmanager.h"

#define KFD_NO_STYLES //disables; styles support needs improvements

#define KEXI_NO_PIXMAPCOLLECTION
#ifdef __GNUC__
#warning pixmapcollection
#endif
#ifndef KEXI_NO_PIXMAPCOLLECTION
#include "pixmapcollection.h"
#endif

namespace KFormDesigner
{

/*TODO

//! @internal
class PropertyFactory : public KoProperty::CustomPropertyFactory
{
public:
    PropertyFactory(QObject *parent)
            : KoProperty::CustomPropertyFactory(parent)
//   m_manager(manager)
    {
    }
    virtual ~PropertyFactory() {}

    KoProperty::CustomProperty* createCustomProperty(KoProperty::Property *) {
        return 0;
    }

    KoProperty::Widget* createCustomWidget(KoProperty::Property *prop) {
        return new KFDPixmapEdit(prop);
    }
};
*/

}

using namespace KFormDesigner;

struct FormManagerInternal {
    FormManagerInternal() : manager(0) {}
    ~FormManagerInternal() {
        delete manager; manager = 0;
    }
    FormManager *manager;
};

K_GLOBAL_STATIC(FormManagerInternal, g_self)

FormManager::FormManager(QObject *parent, int options, const char *name)
        : QObject(parent)
#ifdef KEXI_DEBUG_GUI
        , m_uiCodeDialog(0)
        , m_options(options)
#endif
        , m_objectBlockingPropertyEditorUpdating(0)
        , m_style(0)
        , m_isRedoing(false)
{
    setObjectName(name);
    Q_UNUSED(options);
#ifdef KEXI_STANDALONE
    KGlobal::locale()->insertCatalog("standalone_kformdesigner");
#else
    KGlobal::locale()->insertCatalog("kformdesigner");
#endif

    connect(KGlobalSettings::self(), SIGNAL(settingsChanged(int)), SLOT(slotSettingsChanged(int)));
    slotSettingsChanged(KGlobalSettings::SETTINGS_SHORTCUTS);

//moved to createWidgetLibrary() m_lib = new WidgetLibrary(this, supportedFactoryGroups);
    m_propSet = new WidgetPropertySet(this);

    m_widgetActionGroup = new QActionGroup(this);

    //unused m_editor = 0;
    m_active = 0;
    m_inserting = false;
    m_drawingSlot = false;
    m_collection = 0;
    m_connection = 0;
    m_popup = 0;
    m_treeview = 0;
    m_emitSelectionSignalsUpdatesPropertySet = false;
    m_domDoc.appendChild(m_domDoc.createElement("UI"));
    m_menuNoBuddy = 0;

//Qt4    m_deleteWidgetLater_list.setAutoDelete(true);
    connect(&m_deleteWidgetLater_timer, SIGNAL(timeout()), this, SLOT(deleteWidgetLaterTimeout()));
    connect(this, SIGNAL(connectionCreated(KFormDesigner::Form*, KFormDesigner::Connection&)),
            this, SLOT(slotConnectionCreated(KFormDesigner::Form*, KFormDesigner::Connection&)));

    // register kfd custom editors
#ifdef __GNUC__
#warning register factory
#else
#pragma WARNING( register factory )
#endif
//todo    KoProperty::FactoryManager::self()->registerFactoryForEditor(KoProperty::Pixmap,
//todo            new PropertyFactory(KoProperty::FactoryManager::self()));
}

FormManager::~FormManager()
{
#ifdef __GNUC__
#warning OK? destroy singleton
#else
#pragma WARNING( OK? destroy singleton )
#endif
    g_self.destroy();
    delete m_popup;
    delete m_connection;
#ifdef KEXI_DEBUG_GUI
    delete m_uiCodeDialog;
#endif
    delete m_style;
// delete m_propFactory;
}


FormManager* FormManager::self()
{
    return g_self->manager;
}

WidgetLibrary*
FormManager::createWidgetLibrary(FormManager* m, const QStringList& supportedFactoryGroups)
{
// if(!_self)
//  m_managerDeleter.setObject( _self, m );
    if (!g_self->manager)
        g_self->manager = m;
    return new WidgetLibrary(m, supportedFactoryGroups);
}

void
FormManager::setEditor(KoProperty::EditorView *editor)
{
    m_editor = editor;

    if (editor)
        editor->changeSet(m_propSet->set());
}

void
FormManager::setObjectTreeView(ObjectTreeView *treeview)
{
    m_treeview = treeview;
    if (m_treeview)
        connect(m_propSet, SIGNAL(widgetNameChanged(const QByteArray&, const QByteArray&)),
                m_treeview, SLOT(renameItem(const QByteArray&, const QByteArray&)));
}

ActionList
FormManager::createActions(WidgetLibrary *lib, KActionCollection* collection, KXMLGUIClient* client)
{
    m_collection = collection;

    ActionList actions = lib->createWidgetActions(client, m_collection,
                         this, SLOT(insertWidget(const QByteArray &)));

    if (m_options & HideSignalSlotConnections)
        m_dragConnection = 0;
    else {
        m_dragConnection = new KToggleAction(
            KIcon("signalslot"), i18n("Connect Signals/Slots"), m_collection);
        m_dragConnection->setObjectName("drag_connection");
        m_widgetActionGroup->addAction(m_dragConnection);
        connect(m_dragConnection, SIGNAL(triggered()),
                this, SLOT(startCreatingConnection()));
        //to be exclusive with any 'widget' action
//kde4 not needed   m_dragConnection->setExclusiveGroup("LibActionWidgets");
        m_dragConnection->setChecked(false);
        actions.append(m_dragConnection);
    }

    m_pointer = new KToggleAction(
        KIcon("mouse_pointer"), i18n("Pointer"), m_collection);
    m_pointer->setObjectName("pointer");
    m_widgetActionGroup->addAction(m_pointer);
    connect(m_pointer, SIGNAL(triggered()),
            this, SLOT(slotPointerClicked()));
//kde4 not needed m_pointer->setExclusiveGroup("LibActionWidgets"); //to be exclusive with any 'widget' action
    m_pointer->setChecked(true);
    actions.append(m_pointer);

    m_snapToGrid = new KToggleAction(
        i18n("Snap to Grid"), m_collection);
    m_snapToGrid->setObjectName("snap_to_grid");
    m_widgetActionGroup->addAction(m_snapToGrid);
    m_snapToGrid->setChecked(true);
    actions.append(m_snapToGrid);

    // Create the Style selection action (with a combo box in toolbar and submenu items)
    KSelectAction *styleAction = new KSelectAction(
        i18n("Style"), m_collection);
    styleAction->setObjectName("change_style");
    connect(styleAction, SIGNAL(triggered()),
            this, SLOT(slotStyle()));
    styleAction->setEditable(false);

//js: unused? KGlobalGroup cg = KGlobal::config()->group("General");
    QString currentStyle(kapp->style()->objectName().toLower());
    const QStringList styles = QStyleFactory::keys();
    styleAction->setItems(styles);
    styleAction->setCurrentItem(0);

    QStringList::ConstIterator endIt = styles.constEnd();
    int idx = 0;
    for (QStringList::ConstIterator it = styles.constBegin(); it != endIt; ++it, ++idx) {
        if ((*it).toLower() == currentStyle) {
            styleAction->setCurrentItem(idx);
            break;
        }
    }
    styleAction->setToolTip(i18n("Set the current view style."));
    styleAction->setMenuAccelsEnabled(true);
    actions.append(styleAction);

    lib->addCustomWidgetActions(m_collection);

    return actions;
}

bool
FormManager::isPasteEnabled()
{
    return m_domDoc.namedItem("UI").hasChildNodes();
}

void
FormManager::undo()
{
    if (!activeForm() || !activeForm()->objectTree())
        return;

    activeForm()->commandHistory()->undo();
}

void
FormManager::redo()
{
    if (!activeForm() || !activeForm()->objectTree())
        return;

    m_isRedoing = true;
    activeForm()->commandHistory()->redo();
    m_isRedoing = false;
}

void
FormManager::insertWidget(const QByteArray &classname)
{
    if (m_drawingSlot)
        stopCreatingConnection();

    m_inserting = true;

    foreach (Form *form, m_forms) {
        if (form->toplevelContainer()) {
            form->widget()->setCursor(QCursor(Qt::CrossCursor));
        }
        const QList<QWidget*> list(form->widget()->findChildren<QWidget*>());
        foreach (QWidget *w, list) {
            if (w->testAttribute(Qt::WA_SetCursor)) {
                form->d->cursors.insert(w, w->cursor());
                w->setCursor(QCursor(Qt::CrossCursor));
            }
        }
    }

    m_selectedClass = classname;
    m_pointer->setChecked(false);
}

void
FormManager::stopInsert()
{
    if (m_drawingSlot)
        stopCreatingConnection();
    if (!m_inserting)
        return;

//#ifndef KEXI_NO_CURSOR_PROPERTY
    foreach (Form *form, m_forms) {
        form->widget()->unsetCursor();
        const QList<QWidget*> list(form->widget()->findChildren<QWidget*>());
        foreach (QWidget *w, list) {
            w->unsetCursor();
#if 0
            if (((QWidget*)o)->ownCursor()) {
                QMap<QObject*, QCursor>::ConstIterator curIt(form->d->cursors.find(o));
                if (curIt != form->d->cursors.constEnd())
                    static_cast<QWidget*>(o)->setCursor(*curIt);
//    ((QWidget*)o)->setCursor( (*(form->d->cursors))[o->name()] ) ;
            }
#endif
        }
    }
//#endif
    m_inserting = false;
    m_pointer->setChecked(true);
}

void
FormManager::slotPointerClicked()
{
    if (m_inserting)
        stopInsert();
    else if (m_dragConnection)
        stopCreatingConnection();
}

void
FormManager::startCreatingConnection()
{
    if (m_options & HideSignalSlotConnections)
        return;

    if (m_inserting)
        stopInsert();

    // We set a Pointing hand cursor while drawing the connection
    foreach (Form *form, m_forms) {
//  form->d->cursors = new QMap<QString, QCursor>();
        form->d->mouseTrackers = new QStringList();
        if (form->toplevelContainer()) {
            form->widget()->setCursor(QCursor(Qt::PointingHandCursor));
            form->widget()->setMouseTracking(true);
        }
        const QList<QWidget*> list(form->widget()->findChildren<QWidget*>());
        foreach(QWidget *w, list) {
            if (w->testAttribute(Qt::WA_SetCursor)) {
                form->d->cursors.insert(w, w->cursor());
//    form->d->cursors->insert(w->name(), w->cursor());
                w->setCursor(QCursor(Qt::PointingHandCursor));
            }
            if (w->hasMouseTracking())
                form->d->mouseTrackers->append(w->objectName());
            w->setMouseTracking(true);
        }
    }
    delete m_connection;
    m_connection = new Connection();
    m_drawingSlot = true;
    if (m_dragConnection)
        m_dragConnection->setChecked(true);
}

void
FormManager::resetCreatedConnection()
{
    if (m_options & HideSignalSlotConnections)
        return;

    delete m_connection;
    m_connection = new Connection();

    if (m_active && m_active->formWidget()) {
        Form *ff = (Form*)m_active;
        FormWidget *fw = 0;
        if (ff)
            fw = ff->formWidget();
        m_active->formWidget()->clearForm();
    }
    if (m_active && m_active->widget())
        m_active->widget()->repaint();
}

void
FormManager::stopCreatingConnection()
{
    if (m_options & HideSignalSlotConnections)
        return;
    if (!m_drawingSlot)
        return;

    if (m_active && m_active->formWidget())
        m_active->formWidget()->clearForm();

    foreach (Form *form, m_forms) {
        form->widget()->unsetCursor();
        form->widget()->setMouseTracking(false);
        const QList<QWidget*> list(form->widget()->findChildren<QWidget*>());
        foreach (QWidget *w, list) {
            if (w->testAttribute(Qt::WA_SetCursor)) {
                QHash<QObject*, QCursor>::ConstIterator curIt(form->d->cursors.find(w));
                if (curIt != form->d->cursors.constEnd())
                    w->setCursor(*curIt);
            }
            w->setMouseTracking(form->d->mouseTrackers->contains(w->objectName()));
        }
        delete(form->d->mouseTrackers);
        form->d->mouseTrackers = 0;
    }

    if (m_connection->slot().isNull())
        emit connectionAborted(activeForm());
    delete m_connection;
    m_connection = 0;
    m_drawingSlot = false;
    m_pointer->setChecked(true);
}

bool
FormManager::snapWidgetsToGrid()
{
    return m_snapToGrid->isChecked();
}

void
FormManager::windowChanged(QWidget *w)
{
    kDebug() << "FormManager::windowChanged("
    << (w ? (QString(w->metaObject()->className()) + " " + w->objectName()) : QString("0")) << ")";

    if (!w) {
        m_active = 0;
        if (m_treeview)
            m_treeview->setForm(0);
        emit propertySetSwitched(0);
        if (isCreatingConnection())
            stopCreatingConnection();

        emitNoFormSelected();
        return;
    }

    Form *previousActive = m_active;
    foreach (Form *form, m_forms) {
        if (form->toplevelContainer() && form->widget() == w) {
            if (m_treeview)
                m_treeview->setForm(form);
            //if(m_propSet)
            // m_propList->setCollection(form->pixmapCollection());

            kDebug() << "FormManager::windowChanged() active form is "
            << form->objectTree()->name();

            if (m_collection) {
#ifndef KFD_NO_STYLES
                // update the 'style' action
                KSelectAction *style = (KSelectAction*)m_collection->action("change_style", "KSelectAction");
                const QString currentStyle = form->widget()->style().name();
                const QStringList styles = style->items();

                int idx = 0;
                QStringList::ConstIterator endIt = styles.constEnd();
                for (QStringList::ConstIterator it = styles.constBegin(); it != endIt; ++it, ++idx) {
                    if ((*it).toLower() == currentStyle) {
                        kDebug() << "Updating the style to " << currentStyle;
                        style->setCurrentItem(idx);
                        break;
                    }
                }
#endif
            }

            if ((form != previousActive) && isCreatingConnection())
                resetCreatedConnection();

            m_active = form;

            emit  dirty(form, form->isModified());
            // update actions state
            m_active->emitActionSignals();
            //update the buffer too
            form->emitSelectionSignals();
            if (!m_emitSelectionSignalsUpdatesPropertySet)
                showPropertySet(propertySet(), true);
            return;
        }
    }

    foreach (Form *form, m_preview) {
        kDebug() << (form->widget() ? form->widget()->objectName() : "");
        if (form->toplevelContainer() && form->widget() == w) {
            kDebug() << "Active preview form is " << form->widget()->objectName();

            if (m_collection) {
#ifndef KFD_NO_STYLES
                // update the 'style' action
                KSelectAction *style = (KSelectAction*)m_collection->action("change_style", "KSelectAction");
                const QString currentStyle = form->widget()->style().name();
                const QStringList styles = style->items();

                int idx = 0;
                QStringList::ConstIterator endIt = styles.constEnd();
                for (QStringList::ConstIterator it = styles.constBegin(); it != endIt; ++it, ++idx) {
                    if ((*it).toLower() == currentStyle) {
                        kDebug() << "Updating the style to " << currentStyle;
                        style->setCurrentItem(idx);
                        break;
                    }
                }
#endif

                resetCreatedConnection();
                m_active = form;

                emit dirty(form, false);
                emitNoFormSelected();
                showPropertySet(0);
                return;
            }
        }
    }
    //m_active = 0;
}

Form*
FormManager::activeForm() const
{
    return m_active;
}

Form*
FormManager::formForWidget(QWidget *w)
{
    foreach (Form *form, m_forms) {
        if (form->toplevelContainer() && form->widget() == w)
            return form;
    }

    return 0; // not one of toplevel widgets
}

void
FormManager::deleteForm(Form *form)
{
    if (!form)
        return;
    if (m_forms.contains(form))
        m_forms.removeOne(form);
    else
        m_preview.removeOne(form);

    if (m_forms.isEmpty()) {
        m_active = 0;
        emit propertySetSwitched(0);
    }
}

void
FormManager::importForm(Form *form, bool preview)
{
    if (!preview)
        initForm(form);
    else {
        m_preview.append(form);
        form->setDesignMode(false);
    }
}

void
FormManager::initForm(Form *form)
{
    m_forms.append(form);

    if (m_treeview)
        m_treeview->setForm(form);

    m_active = form;

    connect(form, SIGNAL(selectionChanged(QWidget*, bool, bool)),
            m_propSet, SLOT(setSelectedWidgetWithoutReload(QWidget*, bool, bool)));
    if (m_treeview) {
        connect(form, SIGNAL(selectionChanged(QWidget*, bool, bool)),
                m_treeview, SLOT(setSelectedWidget(QWidget*, bool)));
        connect(form, SIGNAL(childAdded(ObjectTreeItem*)), m_treeview, SLOT(addItem(ObjectTreeItem*)));
        connect(form, SIGNAL(childRemoved(ObjectTreeItem*)), m_treeview, SLOT(removeItem(ObjectTreeItem*)));
    }
    connect(m_propSet, SIGNAL(widgetNameChanged(const QByteArray&, const QByteArray&)),
            form, SLOT(changeName(const QByteArray&, const QByteArray&)));

    form->setSelectedWidget(form->widget());
    windowChanged(form->widget());
}

void
FormManager::previewForm(Form *form, QWidget *container, Form *toForm)
{
    if (!form || !container || !form->objectTree())
        return;
    QDomDocument domDoc;
    if (!FormIO::saveFormToDom(form, domDoc))
        return;

    Form *myform;
    if (!toForm) {
        myform = new Form(form->library(), false/*!designMode, we need to set it early enough*/);
        myform->setObjectName(form->objectTree()->name());
    } else
        myform = toForm;
    myform->createToplevel(container);
    container->setStyle(form->widget()->style());

    if (!FormIO::loadFormFromDom(myform, container, domDoc)) {
        delete myform;
        return;
    }

    myform->setDesignMode(false);
    m_preview.append(myform);
    container->show();
}

/*
bool
FormManager::loadFormFromDomInternal(Form *form, QWidget *container, QDomDocument &inBuf)
{
  return FormIO::loadFormFromDom(myform, container, domDoc);
}

bool
FormManager::saveFormToStringInternal(Form *form, QString &dest, int indent)
{
  return KFormDesigner::FormIO::saveFormToString(form, dest, indent);
}*/

bool
FormManager::isTopLevel(QWidget *w)
{
    if (!activeForm() || !activeForm()->objectTree())
        return false;

// kDebug() << "FormManager::isTopLevel(): for: " << w->objectName() << " = "
//  << activeForm()->objectTree()->lookup(w->name());

    ObjectTreeItem *item = activeForm()->objectTree()->lookup(w->objectName());
    if (!item)
        return true;

    return (!item->parent());
}

void
FormManager::deleteWidget()
{
    if (!activeForm() || !activeForm()->objectTree())
        return;

    QWidgetList *list = activeForm()->selectedWidgets();
    if (list->isEmpty())
        return;

    if (activeForm()->widget() == list->first()) {
        //toplevel form is selected, cannot delete it
        return;
    }

    K3Command *com = new DeleteWidgetCommand(*list, activeForm());
    activeForm()->addCommand(com, true);
}

void
FormManager::copyWidget()
{
    if (!activeForm() || !activeForm()->objectTree())
        return;

    QWidgetList *list = activeForm()->selectedWidgets();
    if (list->isEmpty())
        return;

    removeChildrenFromList(*list);

    // We clear the current clipboard
    m_domDoc.setContent(QString(), true);
    QDomElement parent = m_domDoc.createElement("UI");
    m_domDoc.appendChild(parent);

    foreach (QWidget *w, *list) {
        ObjectTreeItem *it = activeForm()->objectTree()->lookup(w->objectName());
        if (!it)
            continue;

        FormIO::saveWidget(it, parent, m_domDoc);
    }

    FormIO::cleanClipboard(parent);

    activeForm()->emitActionSignals(); // to update 'Paste' item state
}

void
FormManager::cutWidget()
{
    if (!activeForm() || !activeForm()->objectTree())
        return;

    QWidgetList *list = activeForm()->selectedWidgets();
    if (list->isEmpty())
        return;

    K3Command *com = new CutWidgetCommand(*list, activeForm());
    activeForm()->addCommand(com, true);
}

void
FormManager::pasteWidget()
{
    if (!m_domDoc.namedItem("UI").hasChildNodes())
        return;
    if (!activeForm() || !activeForm()->objectTree())
        return;

    K3Command *com = new PasteWidgetCommand(m_domDoc, activeForm()->activeContainer(), m_insertPoint);
    activeForm()->addCommand(com, true);
}

void
FormManager::setInsertPoint(const QPoint &p)
{
    m_insertPoint = p;
}

void
FormManager::createSignalMenu(QWidget *w)
{
    m_sigSlotMenu = new KMenu();
    m_sigSlotMenu->addTitle(SmallIcon("connection"), i18n("Signals"));

    const QList<QMetaMethod> list(
        KexiUtils::methodsForMetaObjectWithParents(w->metaObject(),
                QMetaMethod::Signal, QMetaMethod::Public));
//qt3: Q3StrList list = w->metaObject()->signalNames(true);
    foreach(QMetaMethod method, list) {
        m_sigSlotMenu->addAction(QString::fromLatin1(method.signature()));
    }
    QAction* result = m_sigSlotMenu->exec(QCursor::pos());
    if (result)
        menuSignalChosen(result);
    else
        resetCreatedConnection();

    delete m_sigSlotMenu;
    m_sigSlotMenu = 0;
}

void
FormManager::createSlotMenu(QWidget *w)
{
    m_sigSlotMenu = new KMenu();
    m_sigSlotMenu->addTitle(SmallIcon("connection"), i18n("Slots"));

    QString signalArg(m_connection->signal().remove(QRegExp(".*[(]|[)]")));

    const QList<QMetaMethod> list(
        KexiUtils::methodsForMetaObjectWithParents(w->metaObject(),
                QMetaMethod::Slot, QMetaMethod::Public));
//qt3: Q3StrList list = w->metaObject()->slotNames(true);
    foreach(QMetaMethod method, list) {
        QString slotArg(method.signature());
        slotArg = slotArg.remove(QRegExp(".*[(]|[)]"));
        if (!signalArg.startsWith(slotArg))
            continue; // args not compatible
        m_sigSlotMenu->addAction(slotArg);
    }

    QAction* result = m_sigSlotMenu->exec(QCursor::pos());
    if (result)
        menuSignalChosen(result);
    else
        resetCreatedConnection();

    delete m_sigSlotMenu;
    m_sigSlotMenu = 0;
}

void
FormManager::createContextMenu(QWidget *w, Container *container, bool popupAtCursor)
{
    if (!activeForm() || !activeForm()->widget())
        return;
    const bool toplevelWidgetSelected = activeForm()->widget() == w;
    const uint widgetsCount = container->form()->selectedWidgets()->count();
    const bool multiple = widgetsCount > 1;
    //const bool enableRemove = w != m_active->widget();
    // We only enablelayout creation if more than one widget with the same parent are selected
    const bool enableLayout = multiple || w == container->widget();

    m_menuWidget = w;
    QString n = container->form()->library()->displayName(w->metaObject()->className());
// QValueVector<int> menuIds();

    if (!m_popup) {
        m_popup = new KMenu();
    } else {
        m_popup->clear();
    }

    //set title
    QIcon icon;
    QString titleText;
    if (!multiple) {
        if (w == container->form()->widget()) {
            icon = SmallIcon("form");
            titleText = i18n("%1 : Form", w->objectName());
        } else {
            icon = SmallIcon(
                       container->form()->library()->iconName(w->metaObject()->className()));
            titleText = QString(w->objectName()) + " : " + n;
        }
    } else {
        icon = SmallIcon("multiple_obj");
        titleText = i18n("Multiple Widgets") + QString(" (%1)").arg(widgetsCount);
    }

    m_popup->addTitle(icon, titleText);

    QAction *a;
#define PLUG_ACTION(_name, forceVisible) \
    { a = action(_name); \
        if (a && (forceVisible || a->isEnabled())) { \
            if (separatorNeeded) \
                m_popup->addSeparator(); \
            separatorNeeded = false; \
            m_popup->addAction(a); \
        } \
    }

    bool separatorNeeded = false;

    PLUG_ACTION("edit_cut", !toplevelWidgetSelected);
    PLUG_ACTION("edit_copy", !toplevelWidgetSelected);
    PLUG_ACTION("edit_paste", true);
    PLUG_ACTION("edit_delete", !toplevelWidgetSelected);
    separatorNeeded = true;
    PLUG_ACTION("layout_menu", enableLayout);
    PLUG_ACTION("break_layout", enableLayout);
    separatorNeeded = true;
    PLUG_ACTION("align_menu", !toplevelWidgetSelected);
    PLUG_ACTION("adjust_size_menu", !toplevelWidgetSelected);
    separatorNeeded = true;

    // We create the buddy menu
    if (!multiple && w->inherits("QLabel")
            && ((QLabel*)w)->text().contains("&")
            && (((QLabel*)w)->textFormat() != Qt::RichText)) {
        if (separatorNeeded)
            m_popup->addSeparator();

        KMenu *sub = new KMenu(w);
        QWidget *buddy = ((QLabel*)w)->buddy();

        m_menuNoBuddy = sub->addAction(i18n("No Buddy"));
        if (!buddy)
            m_menuNoBuddy->setChecked(true);
        sub->addSeparator();

        // add all the widgets that can have focus
        foreach (ObjectTreeItem *item, *container->form()->tabStops()) {
            QAction* action = sub->addAction(
                KIcon(
                    container->form()->library()->iconName(item->className().toLatin1())),
                item->name()
            );
            if (item->widget() == buddy)
                action->setChecked(true);
        }

        QAction *subAction = m_popup->addMenu(sub);
        subAction->setText(i18n("Choose Buddy..."));
//  menuIds->append(id);
        connect(sub, SIGNAL(triggered(QAction*)), this, SLOT(buddyChosen(QAction*)));
        separatorNeeded = true;
    }

    //int sigid=0;
#ifdef KEXI_DEBUG_GUI
    if (!multiple && !(m_options & HideEventsInPopupMenu)) {
        if (separatorNeeded)
            m_popup->addSeparator();

        // We create the signals menu
        KMenu *sigMenu = new KMenu();
//ported Q3StrList list = w->metaObject()->signalNames(true);
        QList<QMetaMethod> list(
            KexiUtils::methodsForMetaObjectWithParents(w->metaObject(), QMetaMethod::Signal,
                    QMetaMethod::Public));
        foreach(QMetaMethod m, list) {
            sigMenu->addAction(m.signature());
        }
        QAction *eventsSubMenuAction = m_popup->addMenu(sigMenu);
        eventsSubMenuAction->setText(i18n("Events"));
//  menuIds->append(id);
        if (list.isEmpty())
            eventsSubMenuAction->setEnabled(false);
        connect(sigMenu, SIGNAL(triggered(QAction*)),
                this, SLOT(menuSignalChosen(QAction*)));
        separatorNeeded = true;
    }
#endif

    // Other items
    if (!multiple) {
        QAction* lastAction = 0;
        if (separatorNeeded) {
            lastAction = m_popup->addSeparator();
        }
        const uint oldIndex = m_popup->actions().count() - 1;
        container->form()->library()
        ->createMenuActions(w->metaObject()->className(), w, m_popup, container);
        if (oldIndex == uint(m_popup->actions().count() - 1)) {
//   for (uint i=oldIndex; i<m_popup->count(); i++) {
//    int id = m_popup->idAt( i );
//    if (id!=-1)
//     menuIds->append( id );
//   }
            //nothing added
            if (separatorNeeded) {
                m_popup->removeAction(lastAction);
            }
        }
    }

    //show the popup at the selected widget
    QPoint popupPos;
    if (popupAtCursor) {
        popupPos = QCursor::pos();
    } else {
        QWidgetList *lst = container->form()->selectedWidgets();
        QWidget * sel_w = lst ? lst->first() : container->form()->selectedWidget();
        popupPos = sel_w ? sel_w->mapToGlobal(QPoint(sel_w->width() / 2, sel_w->height() / 2)) : QCursor::pos();
    }
    m_insertPoint = container->widget()->mapFromGlobal(popupPos);
    m_popup->exec(popupPos);//QCursor::pos());
    m_insertPoint = QPoint();

// QValueVector<int>::iterator it;
// for(it = menuIds->begin(); it != menuIds->end(); ++it)
//  m_popup->removeItem(*it);
}

void
FormManager::buddyChosen(QAction *action)
{
    if (!m_menuWidget || !action)
        return;
    QLabel *label = static_cast<QLabel*>((QWidget*)m_menuWidget);

    if (action == m_menuNoBuddy) {
        label->setBuddy(0);
        return;
    }

    ObjectTreeItem *item = activeForm()->objectTree()->lookup(action->text());
    if (!item || !item->widget())
        return;
    label->setBuddy(item->widget());
}

void
FormManager::menuSignalChosen(QAction* action)
{
    if (m_options & HideSignalSlotConnections)
        return;

    //if(!m_menuWidget)
    // return;
    if (m_drawingSlot && m_sigSlotMenu && action) {
        if (m_connection->receiver().isNull())
            m_connection->setSignal(action->text());
        else {
            m_connection->setSlot(action->text());
            kDebug() << "Finished creating the connection: sender=" << m_connection->sender() << "; signal=" << m_connection->signal() <<
            "; receiver=" << m_connection->receiver() << "; slot=" << m_connection->slot();
            emit connectionCreated(activeForm(), *m_connection);
            stopCreatingConnection();
        }
    } else if (m_menuWidget)
        emit createFormSlot(m_active, m_menuWidget->objectName(), action->text());
}

void
FormManager::slotConnectionCreated(Form *form, Connection &connection)
{
    if (m_options & HideSignalSlotConnections)
        return;
    if (!form)
        return;

    Connection *c = new Connection(connection);
    form->connectionBuffer()->append(c);
}

void
FormManager::layoutHBox()
{
    createLayout(Container::HBox);
}

void
FormManager::layoutVBox()
{
    createLayout(Container::VBox);
}

void
FormManager::layoutGrid()
{
    createLayout(Container::Grid);
}

void
FormManager::layoutHSplitter()
{
    createLayout(Container::HSplitter);
}

void
FormManager::layoutVSplitter()
{
    createLayout(Container::VSplitter);
}

void
FormManager::layoutHFlow()
{
    createLayout(Container::HFlow);
}

void
FormManager::layoutVFlow()
{
    createLayout(Container::VFlow);
}

void
FormManager::createLayout(int layoutType)
{
    QWidgetList *list = m_active->selectedWidgets();
    // if only one widget is selected (a container), we modify its layout
    if (list->isEmpty()) {//sanity check
        kWarning() << "FormManager::createLayout(): list is empty!";
        return;
    }
    if (list->count() == 1) {
        ObjectTreeItem *item = m_active->objectTree()->lookup(list->first()->objectName());
        if (!item || !item->container() || !m_propSet->contains("layout"))
            return;
        (*m_propSet)["layout"] = Container::layoutTypeToString(layoutType);
        return;
    }

    QWidget *parent = list->first()->parentWidget();
    foreach (QWidget *w, *list) {
        kDebug() << "comparing widget " << w->objectName() << " whose parent is " << w->parentWidget()->objectName() << " insteaed of " << parent->objectName();
        if (w->parentWidget() != parent) {
            KMessageBox::sorry(m_active->widget()->topLevelWidget(), i18n("<b>Cannot create the layout.</b>\n"
                               "All selected widgets must have the same parent."));
            kDebug() << "FormManager::createLayout() widgets don't have the same parent widget";
            return;
        }
    }

    K3Command *com = new CreateLayoutCommand(layoutType, *list, m_active);
    m_active->addCommand(com, true);
}

void
FormManager::breakLayout()
{
    if (!activeForm() || !activeForm()->objectTree())
        return;

    Container *container = activeForm()->activeContainer();
    QByteArray c(container->widget()->metaObject()->className());

    if ((c == "Grid") || (c == "VBox") || (c == "HBox") || (c == "HFlow") || (c == "VFlow")) {
        K3Command *com = new BreakLayoutCommand(container);
        m_active->addCommand(com, true);
    } else { // normal container
        if (activeForm()->selectedWidgets()->count() == 1)
            (*m_propSet)["layout"] = "NoLayout";
        else
            container->setLayout(Container::NoLayout);
    }
}

void
FormManager::showPropertySet(WidgetPropertySet *set, bool forceReload,
                             const QByteArray& propertyToSelect)
{
    if (m_objectBlockingPropertyEditorUpdating)
        return;

    /*unused if(m_editor) {
        if (propertyToSelect.isEmpty() && forceReload)
          m_editor->changeSet(set ? set->set() : 0, propertyToSelect);
        else
          m_editor->changeSet(set ? set->set() : 0);
      }*/

    emit propertySetSwitched(set ? set->set() : 0, /*preservePrevSelection*/forceReload, propertyToSelect);
}

void
FormManager::blockPropertyEditorUpdating(void *blockingObject)
{
    if (!blockingObject || m_objectBlockingPropertyEditorUpdating)
        return;
    m_objectBlockingPropertyEditorUpdating = blockingObject;
}

void
FormManager::unblockPropertyEditorUpdating(void *blockingObject, WidgetPropertySet *set)
{
    if (!blockingObject || m_objectBlockingPropertyEditorUpdating != blockingObject)
        return;

    m_objectBlockingPropertyEditorUpdating = 0;
    showPropertySet(set, true/*forceReload*/);
}

void
FormManager::editTabOrder()
{
    if (!activeForm() || !activeForm()->objectTree())
        return;
    QWidget *topLevel = m_active->widget()->topLevelWidget();
    TabStopDialog dlg(topLevel);
    //const bool oldAutoTabStops = m_active->autoTabStops();
    if (dlg.exec(m_active) == QDialog::Accepted) {
        //inform about changing "autoTabStop" property
        // -- this will be received eg. by Kexi, so custom "autoTabStop" property can be updated
        emit autoTabStopsSet(m_active, dlg.autoTabStops());
        //force set dirty
        emit dirty(m_active, true);
    }
}

void
FormManager::slotStyle()
{
    if (!activeForm())
        return;

    KSelectAction *styleAction = qobject_cast<KSelectAction*>(
                                     m_collection->action("change_style"));
    const QString styleName = styleAction->currentText().toLower();
    if (styleName == activeForm()->widget()->style()->objectName().toLower())
        return;
    delete m_style;
    m_style = QStyleFactory::create(styleName);
    if (m_style) {
        activeForm()->widget()->setStyle(m_style);
        const QList<QWidget*> l(activeForm()->widget()->findChildren<QWidget*>());
        foreach(QWidget *w, l) {
            w->setStyle(m_style);
        }
    }
}

void
FormManager::editFormPixmapCollection()
{
    if (!activeForm() || !activeForm()->objectTree())
        return;

#ifdef __GNUC__
#warning pixmapcollection
#endif
#ifndef KEXI_NO_PIXMAPCOLLECTION
    PixmapCollectionEditor dialog(activeForm()->pixmapCollection(), activeForm()->widget()->topLevelWidget());
    dialog.exec();
#endif
}

void
FormManager::editConnections()
{
    if (m_options & HideSignalSlotConnections)
        return;
    if (!activeForm() || !activeForm()->objectTree())
        return;

    ConnectionDialog dialog(activeForm()->widget()->topLevelWidget());
    dialog.exec(activeForm());
}

void
FormManager::alignWidgets(int type)
{
    if (!activeForm() || !activeForm()->objectTree() || (activeForm()->selectedWidgets()->count() < 2))
        return;

    QWidget *parentWidget = activeForm()->selectedWidgets()->first()->parentWidget();

    foreach (QWidget *w, *activeForm()->selectedWidgets()) {
        if (w->parentWidget() != parentWidget) {
            kDebug() << "type ==" << type <<  " widgets don't have the same parent widget";
            return;
        }
    }

    K3Command *com = new AlignWidgetsCommand(type, *(activeForm()->selectedWidgets()), activeForm());
    activeForm()->addCommand(com, true);
}

void
FormManager::alignWidgetsToLeft()
{
    alignWidgets(AlignWidgetsCommand::AlignToLeft);
}

void
FormManager::alignWidgetsToRight()
{
    alignWidgets(AlignWidgetsCommand::AlignToRight);
}

void
FormManager::alignWidgetsToTop()
{
    alignWidgets(AlignWidgetsCommand::AlignToTop);
}

void
FormManager::alignWidgetsToBottom()
{
    alignWidgets(AlignWidgetsCommand::AlignToBottom);
}

void
FormManager::adjustWidgetSize()
{
    if (!activeForm() || !activeForm()->objectTree())
        return;

    K3Command *com = new AdjustSizeCommand(AdjustSizeCommand::SizeToFit, *(activeForm()->selectedWidgets()), activeForm());
    activeForm()->addCommand(com, true);
}

void
FormManager::alignWidgetsToGrid()
{
    if (!activeForm() || !activeForm()->objectTree())
        return;

    K3Command *com = new AlignWidgetsCommand(AlignWidgetsCommand::AlignToGrid, *(activeForm()->selectedWidgets()), activeForm());
    activeForm()->addCommand(com, true);
}

void
FormManager::adjustSizeToGrid()
{
    if (!activeForm() || !activeForm()->objectTree())
        return;

    K3Command *com = new AdjustSizeCommand(AdjustSizeCommand::SizeToGrid, *(activeForm()->selectedWidgets()), activeForm());
    activeForm()->addCommand(com, true);
}

void
FormManager::adjustWidthToSmall()
{
    if (!activeForm() || !activeForm()->objectTree())
        return;

    K3Command *com = new AdjustSizeCommand(AdjustSizeCommand::SizeToSmallWidth, *(activeForm()->selectedWidgets()), activeForm());
    activeForm()->addCommand(com, true);
}

void
FormManager::adjustWidthToBig()
{
    if (!activeForm() || !activeForm()->objectTree())
        return;

    K3Command *com = new AdjustSizeCommand(AdjustSizeCommand::SizeToBigWidth, *(activeForm()->selectedWidgets()), activeForm());
    activeForm()->addCommand(com, true);
}

void
FormManager::adjustHeightToSmall()
{
    if (!activeForm() || !activeForm()->objectTree())
        return;

    K3Command *com = new AdjustSizeCommand(AdjustSizeCommand::SizeToSmallHeight, *(activeForm()->selectedWidgets()), activeForm());
    activeForm()->addCommand(com, true);
}

void
FormManager::adjustHeightToBig()
{
    if (!activeForm() || !activeForm()->objectTree())
        return;

    K3Command *com = new AdjustSizeCommand(AdjustSizeCommand::SizeToBigHeight, *(activeForm()->selectedWidgets()), activeForm());
    activeForm()->addCommand(com, true);
}

void
FormManager::bringWidgetToFront()
{
    if (!activeForm() || !activeForm()->objectTree())
        return;

    foreach (QWidget *w, *activeForm()->selectedWidgets()) {
        w->raise();
    }
}

void
FormManager::sendWidgetToBack()
{
    if (!activeForm() || !activeForm()->objectTree())
        return;

    foreach (QWidget *w, *activeForm()->selectedWidgets()) {
        w->lower();
    }
}

void
FormManager::selectAll()
{
    if (!activeForm() || !activeForm()->objectTree())
        return;

    activeForm()->selectFormWidget();
    uint count = activeForm()->objectTree()->children()->count();
    foreach (ObjectTreeItem *titem, *activeForm()->objectTree()->children()) {
        activeForm()->setSelectedWidget(
            titem->widget(), /*add*/true, /*raise*/false, 
            /*moreWillBeSelected*/count > 1
        );
        count--;
    }
}

void
FormManager::clearWidgetContent()
{
    if (!activeForm() || !activeForm()->objectTree())
        return;

    foreach (QWidget *w, *activeForm()->selectedWidgets()) {
        activeForm()->library()->clearWidgetContent(w->metaObject()->className(), w);
    }
}

void
FormManager::deleteWidgetLater(QWidget *w)
{
    w->hide();
    w->setParent(0, Qt::Window);
    m_deleteWidgetLater_list.append(w);
    m_deleteWidgetLater_timer.setSingleShot(true);
    m_deleteWidgetLater_timer.start(100);
}

void
FormManager::deleteWidgetLaterTimeout()
{
    qDeleteAll(m_deleteWidgetLater_list);
    m_deleteWidgetLater_list.clear();
}

void
FormManager::showFormUICode()
{
#ifdef KEXI_DEBUG_GUI
    if (!activeForm())
        return;

    QString uiCode;
    if (!FormIO::saveFormToString(activeForm(), uiCode, 3)) {
        //! @todo show err?
        return;
    }

    if (!m_uiCodeDialog) {
        m_uiCodeDialog = new KPageDialog();
        m_uiCodeDialog->setObjectName("ui_dialog");
        m_uiCodeDialog->setFaceType(KPageDialog::Tabbed);
        m_uiCodeDialog->setModal(true);
        m_uiCodeDialog->setCaption(i18n("Form's UI Code"));
        m_uiCodeDialog->setButtons(KDialog::Close);
//kde4: needed?  m_uiCodeDialog->resize(700, 600);

        m_currentUICodeDialogEditor = new KTextEdit(m_uiCodeDialog);
        m_uiCodeDialog->addPage(m_currentUICodeDialogEditor, i18n("Current"));
        m_currentUICodeDialogEditor->setReadOnly(true);
        QFont f(m_currentUICodeDialogEditor->font());
        f.setFamily("courier");
        m_currentUICodeDialogEditor->setFont(f);
        //Qt3: m_currentUICodeDialogEditor->setTextFormat(Qt::PlainText);

        m_originalUICodeDialogEditor = new KTextEdit(m_uiCodeDialog);
        m_uiCodeDialog->addPage(m_originalUICodeDialogEditor, i18n("Original"));
        m_originalUICodeDialogEditor->setReadOnly(true);
        m_originalUICodeDialogEditor->setFont(f);
        //Qt3: m_originalUICodeDialogEditor->setTextFormat(Qt::PlainText);
    }
    m_currentUICodeDialogEditor->setPlainText(uiCode);
    //indent and set our original doc as well:
    QDomDocument doc;
    doc.setContent(activeForm()->m_recentlyLoadedUICode);
    m_originalUICodeDialogEditor->setPlainText(doc.toString(3));
    m_uiCodeDialog->show();
#endif
}

void
FormManager::slotSettingsChanged(int category)
{
    if (category == KGlobalSettings::SETTINGS_SHORTCUTS) {
        m_contextMenuKey = KGlobalSettings::contextMenuKey();
    }
}

void
FormManager::emitWidgetSelected(KFormDesigner::Form* form, bool multiple)
{
    enableFormActions();
    // Enable edit actions
    enableAction("edit_copy", true);
    enableAction("edit_cut", true);
    enableAction("edit_delete", true);
    enableAction("clear_contents", true);

    // 'Align Widgets' menu
    enableAction("align_menu", multiple);
    enableAction("align_to_left", multiple);
    enableAction("align_to_right", multiple);
    enableAction("align_to_top", multiple);
    enableAction("align_to_bottom", multiple);

    enableAction("adjust_size_menu", true);
    enableAction("adjust_width_small", multiple);
    enableAction("adjust_width_big", multiple);
    enableAction("adjust_height_small", multiple);
    enableAction("adjust_height_big", multiple);

    enableAction("format_raise", true);
    enableAction("format_lower", true);

    QWidgetList *wlist = form->selectedWidgets();
    bool fontEnabled = false;
    foreach (QWidget* w, *wlist) {
        if (-1 != KexiUtils::indexOfPropertyWithSuperclasses(w, "font")) {
            fontEnabled = true;
            break;
        }
    }
    enableAction("format_font", fontEnabled);

    // If the widgets selected is a container, we enable layout actions
    bool containerSelected = false;
    if (!multiple) {
        KFormDesigner::ObjectTreeItem *item = 0;
        if (form->selectedWidgets()->first())
            form->objectTree()->lookup(form->selectedWidgets()->first()->objectName());
        if (item && item->container())
            containerSelected = true;
    }
    const bool twoSelected = form->selectedWidgets()->count() == 2;
    // Layout actions
    enableAction("layout_menu", multiple || containerSelected);
    enableAction("layout_hbox", multiple || containerSelected);
    enableAction("layout_vbox", multiple || containerSelected);
    enableAction("layout_grid", multiple || containerSelected);
    enableAction("layout_hsplitter", twoSelected);
    enableAction("layout_vsplitter", twoSelected);

    KFormDesigner::Container *container = activeForm() ? activeForm()->activeContainer() : 0;
    if (container)
        enableAction("break_layout", (container->layoutType() != KFormDesigner::Container::NoLayout));

    emit widgetSelected(form, true);
}

void
FormManager::emitFormWidgetSelected(KFormDesigner::Form* form)
{
// disableWidgetActions();
    enableAction("edit_copy", false);
    enableAction("edit_cut", false);
    enableAction("edit_delete", false);
    enableAction("clear_contents", false);

    // Disable format functions
    enableAction("align_menu", false);
    enableAction("align_to_left", false);
    enableAction("align_to_right", false);
    enableAction("align_to_top", false);
    enableAction("align_to_bottom", false);
    enableAction("adjust_size_menu", false);
    enableAction("format_raise", false);
    enableAction("format_lower", false);

    enableAction("format_font", false);

    enableFormActions();

    const bool twoSelected = form->selectedWidgets()->count() == 2;
    const bool hasChildren = !form->objectTree()->children()->isEmpty();

    // Layout actions
    enableAction("layout_menu", hasChildren);
    enableAction("layout_hbox", hasChildren);
    enableAction("layout_vbox", hasChildren);
    enableAction("layout_grid", hasChildren);
    enableAction("layout_hsplitter", twoSelected);
    enableAction("layout_vsplitter", twoSelected);
    enableAction("break_layout", (form->toplevelContainer()->layoutType() != KFormDesigner::Container::NoLayout));

    emit formWidgetSelected(form);
}

void
FormManager::emitNoFormSelected()
{
    disableWidgetActions();

    // Disable edit actions
// enableAction("edit_paste", false);
// enableAction("edit_undo", false);
// enableAction("edit_redo", false);

    // Disable 'Tools' actions
    enableAction("pixmap_collection", false);
    if (!(m_options & HideSignalSlotConnections))
        enableAction("form_connections", false);
    enableAction("taborder", false);
    enableAction("change_style", activeForm() != 0);

    // Disable items in 'File'
    if (!(m_options & SkipFileActions)) {
        enableAction("file_save", false);
        enableAction("file_save_as", false);
        enableAction("preview_form", false);
    }

    emit noFormSelected();
}

void
FormManager::enableFormActions()
{
    // Enable 'Tools' actions
    enableAction("pixmap_collection", true);
    if (!(m_options & HideSignalSlotConnections))
        enableAction("form_connections", true);
    enableAction("taborder", true);
    enableAction("change_style", true);

    // Enable items in 'File'
    if (!(m_options & SkipFileActions)) {
        enableAction("file_save", true);
        enableAction("file_save_as", true);
        enableAction("preview_form", true);
    }

    enableAction("edit_paste", isPasteEnabled());
    enableAction("edit_select_all", true);
}

void
FormManager::disableWidgetActions()
{
    // Disable edit actions
    enableAction("edit_copy", false);
    enableAction("edit_cut", false);
    enableAction("edit_delete", false);
    enableAction("clear_contents", false);

    // Disable format functions
    enableAction("align_menu", false);
    enableAction("align_to_left", false);
    enableAction("align_to_right", false);
    enableAction("align_to_top", false);
    enableAction("align_to_bottom", false);
    enableAction("adjust_size_menu", false);
    enableAction("format_raise", false);
    enableAction("format_lower", false);

    enableAction("layout_menu", false);
    enableAction("layout_hbox", false);
    enableAction("layout_vbox", false);
    enableAction("layout_grid", false);
    enableAction("layout_hsplitter", false);
    enableAction("layout_vsplitter", false);
    enableAction("break_layout", false);
}

void
FormManager::emitUndoEnabled(bool enabled, const QString &text)
{
    enableAction("edit_undo", enabled);
    emit undoEnabled(enabled, text);
}

void
FormManager::emitRedoEnabled(bool enabled, const QString &text)
{
    enableAction("edit_redo", enabled);
    emit redoEnabled(enabled, text);
}

void
FormManager::changeFont()
{
    if (!m_active)
        return;
    QWidgetList *wlist = m_active->selectedWidgets();
    QWidgetList widgetsWithFontProperty;
    QFont font;
    bool oneFontSelected = true;
    foreach (QWidget* widget, *wlist) {
        if (m_active->library()->isPropertyVisible(widget->metaObject()->className(), widget, "font")) {
            widgetsWithFontProperty.append(widget);
            if (oneFontSelected) {
                if (widgetsWithFontProperty.count() == 1)
                    font = widget->font();
                else if (font != widget->font())
                    oneFontSelected = false;
            }
        }
    }
    if (widgetsWithFontProperty.isEmpty())
        return;
    if (!oneFontSelected) //many different fonts selected: pick a font from toplevel conatiner
        font = m_active->widget()->font();

    if (1 == widgetsWithFontProperty.count()) {
        //single widget's settings
//?        QWidget *widget = widgetsWithFontProperty.first();
        KoProperty::Property &fontProp = m_propSet->property("font");
        if (QDialog::Accepted != KFontDialog::getFont(font, false, m_active->widget()))
            return;
        fontProp = font;
        return;
    }
    //multiple widgets
    QFlags<KFontChooser::FontDiff> diffFlags = KFontChooser::NoFontDiffFlags;
    if (QDialog::Accepted != KFontDialog::getFontDiff(
                font, diffFlags, KFontChooser::NoDisplayFlags, m_active->widget())
            || 0 == diffFlags) {
        return;
    }
    //update font
    foreach (QWidget* widget, widgetsWithFontProperty) {
        QFont prevFont(widget->font());
        if (diffFlags & KFontChooser::FontDiffFamily)
            prevFont.setFamily(font.family());
        if (diffFlags & KFontChooser::FontDiffStyle) {
            prevFont.setBold(font.bold());
            prevFont.setItalic(font.italic());
        }
        if (diffFlags & KFontChooser::FontDiffSize)
            prevFont.setPointSize(font.pointSize());
        /*! @todo this modification is not added to UNDO BUFFER:
                  do it when KoProperty::Set supports multiple selections */
        widget->setFont(prevFont);
        //temporary fix for dirty flag:
        emit dirty(m_active, true);
    }
}

#include "formmanager.moc"
