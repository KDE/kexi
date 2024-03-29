/* This file is part of the KDE project
   Copyright (C) 2005-2011 Jarosław Staniek <staniek@kde.org>

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

#include "kexiformmanager.h"
#include "kexidbform.h"
#ifdef KEXI_AUTOFIELD_FORM_WIDGET_SUPPORT
#include "kexidbautofield.h"
#endif
#include "kexiformscrollview.h"
#include "kexiformview.h"
#include "kexidatasourcepage.h"

#include <KexiIcon.h>

#include <QDomDocument>
#include <QAction>
#include <QDebug>

#include <KToggleAction>
#include <KActionCollection>
#include <KPageDialog>
#include <KTextEdit>
#include <KToolBar>
#include <KSharedConfig>
#include <KConfigGroup>

#include <formeditor/form.h>
#include <formeditor/widgetlibrary.h>
#include <formeditor/commands.h>
#include <formeditor/objecttree.h>
#include <formeditor/formIO.h>
#include <formeditor/kexiactionselectiondialog.h>
#include <formeditor/WidgetTreeWidget.h>

#include <KPropertySet>
#include <KProperty>
#include <widget/properties/KexiCustomPropertyFactory.h>
#include <core/KexiMainWindowIface.h>
#include <kexiutils/SmallToolButton.h>
#include <kexiutils/utils.h>
#include <config-kexi.h>

class KexiFormManagerPrivate {
public:
    explicit KexiFormManagerPrivate(KexiFormManager *qq) : part(nullptr), lib(nullptr), q(qq)
    {
        features = KFormDesigner::Form::NoFeatures;
        widgetActionGroup = new KFormDesigner::ActionGroup(q);
    }
    ~KexiFormManagerPrivate() {
    }
    KexiFormPart* part;
    KFormDesigner::WidgetLibrary* lib;
    QPointer<KFormDesigner::ActionGroup> widgetActionGroup;
    QPointer<KFormDesigner::WidgetTreeWidget> widgetTree;
    QPointer<KActionCollection> collection;
    KFormDesigner::Form::Features features;
    QPointer<KToggleAction> pointerAction;
#ifdef KFD_SIGSLOTS
    QPointer<KToggleAction> dragConnectionAction;
#endif
    QPointer<KToggleAction> snapToGridAction;

    KexiFormManager *q;
};

Q_GLOBAL_STATIC(KexiFormManager, g_manager)

KexiFormManager* KexiFormManager::self()
{
    return g_manager;
}

KexiFormManager::KexiFormManager()
        : QObject()
        , d(new KexiFormManagerPrivate(this))
{
    // needed for custom "pixmap" property editor widget
    KexiCustomPropertyFactory::init();
}

KexiFormManager::~KexiFormManager()
{
    delete d;
}

void KexiFormManager::init(KexiFormPart *part, KFormDesigner::WidgetTreeWidget *widgetTree)
{
/*! @todo add configuration for supported factory groups */
    QStringList supportedFactoryGroups;
    supportedFactoryGroups += "kexi";
    d->lib = new KFormDesigner::WidgetLibrary(this, supportedFactoryGroups);
    d->lib->setAdvancedPropertiesVisible(false);

    connect(d->lib, SIGNAL(widgetCreated(QWidget*)),
            this, SLOT(slotWidgetCreatedByFormsLibrary(QWidget*)));
    connect(d->lib, SIGNAL(widgetActionToggled(QByteArray)),
        this, SLOT(slotWidgetActionToggled(QByteArray)));

    d->part = part;
    KActionCollection *col = /*tmp*/ new KActionCollection(this);
    if (col) {
        createActions( col );
        //connect actions provided by widget factories
        connect(col->action("widget_assign_action"), SIGNAL(triggered()),
                this, SLOT(slotAssignAction()));
    }

    d->widgetTree = widgetTree;
    if (d->widgetTree) {
//! @todo KEXI3 Port this: connect()
//! @todo KEXI3 Port code related to KFormDesigner::FormManager::m_treeview here
//! @todo        connect(m_propSet, SIGNAL(widgetNameChanged(QByteArray,QByteArray)),
//! @todo                m_treeview, SLOT(renameItem(QByteArray,QByteArray)));
    }
}

KFormDesigner::ActionGroup* KexiFormManager::widgetActionGroup() const
{
    return d->widgetActionGroup;
}

void KexiFormManager::createActions(KActionCollection* collection)
{
    d->collection = collection;
    d->lib->createWidgetActions(d->widgetActionGroup);
//! @todo insertWidget() slot?

#ifdef KFD_SIGSLOTS
    if (d->features & KFormDesigner::Form::EnableConnections) {
        // nothing
    }
    else {
        d->dragConnectionAction = new KToggleAction(
            KexiIcon("signalslot"), futureI18n("Connect Signals/Slots"), d->collection);
        d->dragConnectionAction->setObjectName("drag_connection");
        connect(d->dragConnectionAction, SIGNAL(triggered()),
                this, SLOT(startCreatingConnection()));
        d->dragConnectionAction->setChecked(false);
    }
#endif

    d->pointerAction = new KToggleAction(
        koIcon("tool-pointer"), xi18n("Pointer"), d->collection);
    d->pointerAction->setObjectName("edit_pointer");
    d->widgetActionGroup->addAction(d->pointerAction);
    connect(d->pointerAction, SIGNAL(triggered()),
            this, SLOT(slotPointerClicked()));
    d->pointerAction->setChecked(true);

    d->snapToGridAction = new KToggleAction(
        xi18n("Snap to Grid"), d->collection);
    d->snapToGridAction->setObjectName("snap_to_grid");

//! @todo
#if 0
    // Create the Style selection action (with a combo box in toolbar and submenu items)
    KSelectAction *styleAction = new KSelectAction(
        xi18n("Style"), d->collection);
    styleAction->setObjectName("change_style");
    connect(styleAction, SIGNAL(triggered()),
            this, SLOT(slotStyle()));
    styleAction->setEditable(false);

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
    styleAction->setToolTip(xi18n("Set the current view style."));
    styleAction->setMenuAccelsEnabled(true);
#endif

    d->lib->addCustomWidgetActions(d->collection);

#ifdef KEXI_DEBUG_GUI
    KConfigGroup generalGroup(KSharedConfig::openConfig()->group("General"));
    if (generalGroup.readEntry("ShowInternalDebugger", false)) {
        QAction *a = new QAction(koIcon("run-build-file"), xi18n("Show Form UI Code"), this);
        d->collection->addAction("show_form_ui", a);
        a->setShortcut(Qt::CTRL + Qt::Key_U);
        connect(a, SIGNAL(triggered()), this, SLOT(showFormUICode()));
    }
#endif

//! @todo move elsewhere
    {
        // (from obsolete kexiformpartinstui.rc)
        QStringList formActions;
        formActions
            << "edit_pointer"
            << QString() //sep
#ifndef KEXI_AUTOFIELD_FORM_WIDGET_SUPPORT
            << "library_widget_KexiDBAutoField"
#endif
            << "library_widget_KexiDBLabel"
            << "library_widget_KexiDBLineEdit"
            << "library_widget_KexiDBTextEdit"
            << "library_widget_KexiDBComboBox"
            << "library_widget_KexiDBCheckBox"
            << "library_widget_KexiDBImageBox"
            << QString() //sep
            << "library_widget_KexiDBPushButton"
            << QString() //sep
            << "library_widget_KexiFrame"
            << "library_widget_QGroupBox"
            << "library_widget_KFDTabWidget"
            << "library_widget_KexiLineWidget"
            << QString() //sep
#ifdef HAVE_QTWEBKITWIDGETS
            << "library_widget_WebBrowserWidget"
#endif
#ifdef HAVE_MARBLE
            << "library_widget_MapBrowserWidget"
#endif
            << "library_widget_KexiDBSlider"
            << "library_widget_KexiDBProgressBar"
            << "library_widget_KexiDBCommandLinkButton"
            << "library_widget_KexiDBDatePicker"
            << QString() //sep
            ;
        KexiMainWindowIface *win = KexiMainWindowIface::global();
        foreach( const QString& actionName_, formActions ) {
            QAction *a;
            const QString actionName(actionName_.startsWith(':') ? actionName_.mid(1) : actionName_);
            if (actionName.isEmpty()) {
                a = new QAction(this);
                a->setSeparator(true);
            }
            else {
                a = d->widgetActionGroup->action(actionName);
            }
            if (actionName_.startsWith(':')) {  // icon only
                KexiSmallToolButton *btn = new KexiSmallToolButton(a, win->toolBar("form"));
                btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
                win->appendWidgetToToolbar("form", btn);
            }
            else {
                win->addToolBarAction("form", a);
            }
        }

        QSet<QString> iconOnlyActions;
        const QList<QAction*> actions( d->collection->actions() );
        foreach( QAction *a, actions ) {
            if (iconOnlyActions.contains(a->objectName())) { // icon only
                KexiSmallToolButton *btn = new KexiSmallToolButton(a, win->toolBar("form"));
                btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
                win->appendWidgetToToolbar("form", btn);
                win->setWidgetVisibleInToolbar(btn, true);
            }
            else {
                win->addToolBarAction("form", a);
            }
        }
    }
}

void KexiFormManager::slotWidgetCreatedByFormsLibrary(QWidget* widget)
{
    QList<QMetaMethod> _signals(KexiUtils::methodsForMetaObject(
                                    widget->metaObject(), QMetaMethod::Signal));

    if (!_signals.isEmpty()) {
        QByteArray handleDragMoveEventSignal = "handleDragMoveEvent(QDragMoveEvent*)";
        QByteArray handleDropEventSignal = "handleDropEvent(QDropEvent*)";
        KexiFormView *formView = KDbUtils::findParent<KexiFormView*>(widget);

        foreach(const QMetaMethod& method, _signals) {
            if (0 == qstrcmp(method.methodSignature(), handleDragMoveEventSignal)) {
                //qDebug() << method.methodSignature();
                if (formView) {
                    connect(widget, SIGNAL(handleDragMoveEvent(QDragMoveEvent*)),
                            formView, SLOT(slotHandleDragMoveEvent(QDragMoveEvent*)));
                }
            } else if (0 == qstrcmp(method.methodSignature(), handleDropEventSignal)) {
                //qDebug() << method.methodSignature();
                if (formView) {
                    connect(widget, SIGNAL(handleDropEvent(QDropEvent*)),
                            formView, SLOT(slotHandleDropEvent(QDropEvent*)));
                }
            }
        }
    }
}

void KexiFormManager::slotWidgetActionToggled(const QByteArray& action)
{
    KexiFormView* fv = activeFormViewWidget();
    if (fv) {
        fv->form()->enterWidgetInsertingState(action);
    }
}

KFormDesigner::WidgetLibrary* KexiFormManager::library() const
{
    return d->lib;
}

QAction* KexiFormManager::action(const char* name)
{
    KActionCollection *col = d->part->actionCollectionForMode(Kexi::DesignViewMode);
    if (!col)
        return 0;
    QString n(translateName(name));
    QAction *a = col->action(n);
    if (a)
        return a;
    if (activeFormViewWidget()) {
        a = KexiMainWindowIface::global()->actionCollection()->action(n);
        if (a)
            return a;
    }
    return d->collection->action(name);
}

KexiFormView* KexiFormManager::activeFormViewWidget() const
{
    KexiWindow *currentWindow = KexiMainWindowIface::global()->currentWindow();
    if (!currentWindow)
        return 0;
    KexiFormView *currentView = dynamic_cast<KexiFormView*>(currentWindow->selectedView());
    if (!currentView || currentView->viewMode()!=Kexi::DesignViewMode) {
        return 0;
    }
    KFormDesigner::Form *form = currentView->form();
    if (!form) {
        return 0;
    }
    KexiDBForm *dbform = dynamic_cast<KexiDBForm*>(form->formWidget());
    if (!dbform) {
        return 0;
    }
    KexiFormScrollView *scrollViewWidget = dynamic_cast<KexiFormScrollView*>(dbform->dataAwareObject());
    if (!scrollViewWidget)
        return 0;
    return dynamic_cast<KexiFormView*>(scrollViewWidget->parent());
}

void KexiFormManager::enableAction(const char* name, bool enable)
{
    KexiFormView* formViewWidget = activeFormViewWidget();
    if (!formViewWidget)
        return;
    formViewWidget->setAvailable(translateName(name).toLatin1(), enable);
}

void KexiFormManager::setFormDataSource(const QString& pluginId, const QString& name)
{
    KexiFormView* formViewWidget = activeFormViewWidget();
    if (!formViewWidget)
        return;
    KexiDBForm* formWidget = dynamic_cast<KexiDBForm*>(formViewWidget->form()->widget());
    if (!formWidget)
        return;

    QString oldDataSourcePartClass(formWidget->dataSourcePluginId());
    QString oldDataSource(formWidget->dataSource());
    if (pluginId != oldDataSourcePartClass || name != oldDataSource) {
        QHash<QByteArray, QVariant> propValues;
        propValues.insert("dataSource", name);
        propValues.insert("dataSourcePartClass", pluginId);
        KFormDesigner::PropertyCommandGroup *group = new KFormDesigner::PropertyCommandGroup(
            xi18n("Set form's data source to <resource>%1</resource>", name));
        formViewWidget->form()->createPropertyCommandsInDesignMode(
            formWidget, propValues, group, true /*addToActiveForm*/);
    }
}

void KexiFormManager::setDataSourceFieldOrExpression(
    const QString& string, const QString& caption, KDbField::Type type)
{
    KexiFormView* formViewWidget = activeFormViewWidget();
    if (!formViewWidget)
        return;

    KPropertySet* set = formViewWidget->form()->propertySet();
    if (!set->contains("dataSource"))
        return;

    set->property("dataSource").setValue(string);

    if (set->propertyValue("autoCaption", false).toBool()) {
        set->changePropertyIfExists("fieldCaptionInternal", caption);
    }
    if (set->propertyValue("widgetType").toString() == "Auto") {
        set->changePropertyIfExists("fieldTypeInternal", type);
    }
}

void KexiFormManager::insertAutoFields(const QString& sourcePartClass, const QString& sourceName,
                                       const QStringList& fields)
{
#ifdef KEXI_AUTOFIELD_FORM_WIDGET_SUPPORT
    Q_UNUSED(sourcePartClass);
    Q_UNUSED(sourceName);
    Q_UNUSED(fields);
#else
    KexiFormView* formViewWidget = activeFormViewWidget();
    if (!formViewWidget || !formViewWidget->form() || !formViewWidget->form()->activeContainer())
        return;
    formViewWidget->insertAutoFields(sourcePartClass, sourceName, fields,
                                     formViewWidget->form()->activeContainer());
#endif
}

void KexiFormManager::slotHistoryCommandExecuted(KFormDesigner::Command *command)
{
    if (command->childCount() == 2) {
        KexiFormView* formViewWidget = activeFormViewWidget();
        if (!formViewWidget)
            return;
        KexiDBForm* formWidget = dynamic_cast<KexiDBForm*>(formViewWidget->form()->widget());
        if (!formWidget)
            return;
        const KFormDesigner::PropertyCommand* pc1
            = dynamic_cast<const KFormDesigner::PropertyCommand*>(command->child(0));
        const KFormDesigner::PropertyCommand* pc2
            = dynamic_cast<const KFormDesigner::PropertyCommand*>(command->child(1));
        if (pc1 && pc2 && pc1->propertyName() == "dataSource" && pc2->propertyName() == "dataSourcePartClass") {
            const QHash<QByteArray, QVariant>::const_iterator it1(pc1->oldValues().constBegin());
            const QHash<QByteArray, QVariant>::const_iterator it2(pc2->oldValues().constBegin());
            if (it1.key() == formWidget->objectName() && it2.key() == formWidget->objectName())
                d->part->dataSourcePage()->setFormDataSource(
                    formWidget->dataSourcePluginId(), formWidget->dataSource());
        }
    }
}

void KexiFormManager::showFormUICode()
{
#ifdef KEXI_DEBUG_GUI
    KexiFormView* formView = activeFormViewWidget();
    if (!formView)
        return;
    formView->form()->resetInlineEditor();

    QString uiCode;
    const int indent = 2;
    if (!KFormDesigner::FormIO::saveFormToString(formView->form(), uiCode, indent)) {
        //! @todo show err?
        return;
    }

    KPageDialog uiCodeDialog;
    uiCodeDialog.setFaceType(KPageDialog::Tabbed);
    uiCodeDialog.setModal(true);
    uiCodeDialog.setWindowTitle(xi18nc("@title:window", "Form's UI Code"));
    uiCodeDialog.resize(700, 600);
    KTextEdit *currentUICodeDialogEditor = new KTextEdit(&uiCodeDialog);
    uiCodeDialog.addPage(currentUICodeDialogEditor, xi18n("Current"));
    currentUICodeDialogEditor->setReadOnly(true);
    QFont f(currentUICodeDialogEditor->font());
    f.setFamily("courier");
    currentUICodeDialogEditor->setFont(f);

    KTextEdit *originalUICodeDialogEditor = new KTextEdit(&uiCodeDialog);
    uiCodeDialog.addPage(originalUICodeDialogEditor, xi18n("Original"));
    originalUICodeDialogEditor->setReadOnly(true);
    originalUICodeDialogEditor->setFont(f);
    currentUICodeDialogEditor->setPlainText(uiCode);
    //indent and set our original doc as well:
    QDomDocument doc;
    doc.setContent(formView->form()->m_recentlyLoadedUICode);
    originalUICodeDialogEditor->setPlainText(doc.toString(indent));
    uiCodeDialog.exec();
#endif
}

void KexiFormManager::slotAssignAction()
{
    KexiFormView* formView = activeFormViewWidget();
    if (!formView)
        return;
    KFormDesigner::Form *form = formView->form();
    KexiDBForm *dbform = 0;
    if (form->mode() != KFormDesigner::Form::DesignMode
        || !(dbform = dynamic_cast<KexiDBForm*>(form->formWidget())))
    {
        return;
    }

    KPropertySet* set = form->propertySet();

    KexiFormEventAction::ActionData data;
    const KProperty &onClickActionProp = set->property("onClickAction");
    if (!onClickActionProp.isNull())
        data.string = onClickActionProp.value().toString();

    const KProperty &onClickActionOptionProp = set->property("onClickActionOption");
    if (!onClickActionOptionProp.isNull())
        data.option = onClickActionOptionProp.value().toString();

    KexiFormScrollView *scrollViewWidget
        = dynamic_cast<KexiFormScrollView*>(dbform->dataAwareObject());
    if (!scrollViewWidget)
        return;
    KexiFormView* formViewWidget = dynamic_cast<KexiFormView*>(scrollViewWidget->parent());
    if (!formViewWidget)
        return;

    KexiActionSelectionDialog dlg(dbform, data,
                                  set->property("objectName").value().toString());

    if (dlg.exec() == QDialog::Accepted) {
        data = dlg.currentAction();
        //update property value
        set->changeProperty("onClickAction", data.string);
        set->changeProperty("onClickActionOption", data.option);
    }
}

void KexiFormManager::slotPointerClicked()
{
    KexiFormView* formView = activeFormViewWidget();
    if (!formView)
        return;
    formView->form()->enterWidgetSelectingState();
}

QString KexiFormManager::translateName(const char* name) const
{
    QString n(QString::fromLatin1(name));
    // translate to our name space:
    if (n.startsWith("align_") || n.startsWith("adjust_")
            || n == "format_raise" || n == "format_lower" || n == "taborder")
    {
        n.prepend("formpart_");
    }
    return n;
}

