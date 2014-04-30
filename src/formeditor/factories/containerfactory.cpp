/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2006-2010 Jarosław Staniek <staniek@kde.org>

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

#include <KoIcon.h>

#include <QStackedWidget>
#include <QFrame>
#include <QDomDocument>
#include <QTabBar>
#include <QSplitter>
#include <QLayout>
#include <QPainter>
#include <QPaintEvent>
#include <QFileInfo>

#include <kpluginfactory.h>
#include <ktextedit.h>
#include <klineedit.h>
#include <klocale.h>
#include <kdebug.h>

#include <kinputdialog.h>
#include <kaction.h>

#include "containerfactory.h"
#include "container.h"
#include "form.h"
#include "formIO.h"
#include "objecttree.h"
#include "widgetlibrary.h"
#include "utils.h"

#include <kexiutils/utils.h>

ContainerWidget::ContainerWidget(QWidget *parent)
        : QWidget(parent)
{
}

ContainerWidget::~ContainerWidget()
{
}

QSize ContainerWidget::sizeHint() const
{
    return QSize(30, 30); //default
}

void ContainerWidget::dragMoveEvent(QDragMoveEvent *e)
{
    QWidget::dragMoveEvent(e);
    emit handleDragMoveEvent(e);
}

void ContainerWidget::dropEvent(QDropEvent *e)
{
    QWidget::dropEvent(e);
    emit handleDropEvent(e);
}

////////////////////////

GroupBox::GroupBox(const QString & title, QWidget *parent)
        : QGroupBox(title, parent)
{
}

GroupBox::~GroupBox()
{
}

void GroupBox::dragMoveEvent(QDragMoveEvent *e)
{
    QGroupBox::dragMoveEvent(e);
    emit handleDragMoveEvent(e);
}

void GroupBox::dropEvent(QDropEvent *e)
{
    QGroupBox::dropEvent(e);
    emit handleDropEvent(e);
}

QSize GroupBox::sizeHint() const
{
    return title().isEmpty() ? QGroupBox::sizeHint() : QSize(fontMetrics().width(title()), fontMetrics().height()*3);
}

////////////////////////

KFDTabWidget::KFDTabWidget(KFormDesigner::Container *container, QWidget *parent)
        : KFormDesigner::TabWidget(parent), m_container(container)
{
}

KFDTabWidget::~KFDTabWidget()
{
}

QSize
KFDTabWidget::sizeHint() const
{
    QSize s(30, 30); // default min size
    for (int i = 0; i < count(); i++)
        s = s.expandedTo(KFormDesigner::getSizeFromChildren(widget(i)));

    return s + QSize(10/*margin*/, tabBar()->height() + 20/*margin*/);
}

void KFDTabWidget::dragMoveEvent(QDragMoveEvent *e)
{
    TabWidgetBase::dragMoveEvent(e);
    if (dynamic_cast<ContainerWidget*>(currentWidget()))
        emit dynamic_cast<ContainerWidget*>(currentWidget())->handleDragMoveEvent(e);
    emit handleDragMoveEvent(e);
}

void KFDTabWidget::dropEvent(QDropEvent *e)
{
    TabWidgetBase::dropEvent(e);
    if (dynamic_cast<ContainerWidget*>(currentWidget()))
        emit dynamic_cast<ContainerWidget*>(currentWidget())->handleDropEvent(e);
    emit handleDropEvent(e);
}

/// Various layout widgets /////////////////:

HBox::HBox(QWidget *parent)
        : QFrame(parent)
{
}

HBox::~HBox()
{
}

void
HBox::paintEvent(QPaintEvent *)
{
    if (!designMode())
        return;
    QPainter p(this);
    p.setPen(QPen(Qt::red, 2, Qt::DashLine));
    p.drawRect(1, 1, width() - 1, height() - 1);
}

VBox::VBox(QWidget *parent)
        : QFrame(parent)
{
}

VBox::~VBox()
{
}

void
VBox::paintEvent(QPaintEvent *)
{
    if (!designMode())
        return;
    QPainter p(this);
    p.setPen(QPen(Qt::blue, 2, Qt::DashLine));
    p.drawRect(1, 1, width() - 1, height() - 1);
}

Grid::Grid(QWidget *parent)
        : QFrame(parent)
{
}

Grid::~Grid()
{
}

void
Grid::paintEvent(QPaintEvent *)
{
    if (!designMode())
        return;
    QPainter p(this);
    p.setPen(QPen(Qt::darkGreen, 2, Qt::DashLine));
    p.drawRect(1, 1, width() - 1, height() - 1);
}

HFlow::HFlow(QWidget *parent)
        : QFrame(parent)
{
}

HFlow::~HFlow()
{
}

void
HFlow::paintEvent(QPaintEvent *)
{
    if (!designMode())
        return;
    QPainter p(this);
    p.setPen(QPen(Qt::magenta, 2, Qt::DashLine));
    p.drawRect(1, 1, width() - 1, height() - 1);
}

VFlow::VFlow(QWidget *parent)
        : QFrame(parent)
{
}

VFlow::~VFlow()
{
}

void
VFlow::paintEvent(QPaintEvent *)
{
    if (!designMode())
        return;
    QPainter p(this);
    p.setPen(QPen(Qt::cyan, 2, Qt::DashLine));
    p.drawRect(1, 1, width() - 1, height() - 1);
}

QSize
VFlow::sizeHint() const
{
    if (layout())
        return layout()->sizeHint();
    else
        return QSize(700, 50); // default
}

/////// Sub forms ////////////////////////:

#if 0
SubForm::SubForm(KFormDesigner::Form *parentForm, QWidget *parent)
        : QScrollArea(parent), m_parentForm(parentForm), m_form(0), m_widget(0)
{
    setFrameStyle(QFrame::WinPanel | QFrame::Sunken);
    viewport()->setPaletteBackgroundColor(colorGroup().mid());
}

SubForm::~SubForm()
{
}

void
SubForm::setFormName(const QString &name)
{
    if (name.isEmpty())
        return;

    QFileInfo info(name);
    if (!info.exists() || (m_parentForm && info.fileName() == m_parentForm->filename())) {
//! @todo ???
        return; // we check if this form is valid
    }

    // we create the container widget
    delete m_widget;
    m_widget = new QWidget(viewport());
    m_widget->setObjectName("subform_widget");
    addChild(m_widget);
    m_form = new KFormDesigner::Form(m_parentForm);
    m_form->setObjectName(this->objectName());
    m_form->createToplevel(m_widget);

    // and load the sub form
    KFormDesigner::FormIO::loadFormFromFile(m_form, m_widget, name);
    m_form->setMode(KFormDesigner::Form::DesignMode);

    m_formName = name;
}
#endif //0

/////   Internal actions

AddTabAction::AddTabAction(KFormDesigner::Container *container,
                           TabWidgetBase *receiver, QObject *parent)
    : KAction(koIcon("tab-new"), i18nc("Add page to tab widget", "Add Page"),
              parent)
    , m_container(container)
    , m_receiver(receiver)
{
    connect(this, SIGNAL(triggered()), this, SLOT(slotTriggered()));
}

void AddTabAction::slotTriggered()
{
    if (!qobject_cast<QTabWidget*>(m_receiver))
        return;
    KFormDesigner::Command *command = new KFormDesigner::InsertPageCommand(m_container, m_receiver);
    if (m_receiver->count() == 0) {
        command->execute();
        delete command;
    }
    else {
        m_container->form()->addCommand(command);
    }
}

RemoveTabAction::RemoveTabAction(KFormDesigner::Container *container,
                                 TabWidgetBase *receiver, QObject *parent)
    : KAction(koIcon("tab-close-other"), i18nc("Remove tab widget's page", "Remove Page"),
              parent)
    , m_container(container)
    , m_receiver(receiver)
{
    connect(this, SIGNAL(triggered()), this, SLOT(slotTriggered()));
    if (m_receiver->count() <= 1) {
        setEnabled(false);
    }
}

void RemoveTabAction::slotTriggered()
{
    if (!qobject_cast<QTabWidget*>(m_receiver) || m_receiver->count() == 0)
        return;
    KFormDesigner::Command *com = new KFormDesigner::RemovePageCommand(m_container, m_receiver);
    m_container->form()->addCommand(com);
}

RenameTabAction::RenameTabAction(KFormDesigner::Container *container,
                                 TabWidgetBase *receiver, QObject *parent)
    : KAction(koIcon("edit-rename"), i18nc("Rename tab widget's page", "Rename Page..."),
              parent)
    , m_container(container)
    , m_receiver(receiver)
{
    connect(this, SIGNAL(triggered()), this, SLOT(slotTriggered()));
}

void RenameTabAction::slotTriggered()
{
    if (!qobject_cast<QTabWidget*>(m_receiver))
        return;
    QWidget *w = m_receiver->currentWidget();
    bool ok;
    QString name = KInputDialog::getText(i18n("New Page Title"),
                                         i18n("Enter a new title for the current page:"),
                                         m_receiver->tabText(m_receiver->indexOf(w)), &ok, w->topLevelWidget());
    if (ok)
        m_receiver->setTabText(m_receiver->indexOf(w), name);
}

AddStackPageAction::AddStackPageAction(KFormDesigner::Container *container,
                                       QWidget *receiver, QObject *parent)
    : KAction(koIcon("tab-new"), i18nc("Add page to a stacked widget", "Add Page..."),
              parent)
    , m_container(container)
    , m_receiver(receiver)
{
    connect(this, SIGNAL(triggered()), this, SLOT(slotTriggered()));
}

void AddStackPageAction::slotTriggered()
{
    if (   !KexiUtils::objectIsA(m_receiver, "QStackedWidget")
        && /* compat */ !KexiUtils::objectIsA(m_receiver, "QWidgetStack"))
    {
        return;
    }
    KFormDesigner::Command *command = new KFormDesigner::InsertPageCommand(m_container, m_receiver);
    if (!dynamic_cast<QStackedWidget*>(m_receiver)->currentWidget()) {
        command->execute();
        delete command;
    }
    else {
        m_container->form()->addCommand(command);
    }
}

RemoveStackPageAction::RemoveStackPageAction(KFormDesigner::Container *container,
                                             QWidget *receiver, QObject *parent)
    : KAction(koIcon("tab-close-other"), i18nc("Remove page from a stacked widget", "Remove Page"),
              parent)
    , m_container(container)
    , m_receiver(receiver)
{
    connect(this, SIGNAL(triggered()), this, SLOT(slotTriggered()));
    if (dynamic_cast<TabWidgetBase*>(m_receiver)->count() <= 1) {
        setEnabled(false);
    }
}

void RemoveStackPageAction::slotTriggered()
{
    if (   !KexiUtils::objectIsA(m_receiver, "QStackedWidget")
        && /* compat */ !KexiUtils::objectIsA(m_receiver, "QWidgetStack"))
    {
        return;
    }
    QStackedWidget *stack = dynamic_cast<QStackedWidget*>(m_receiver);
    QWidget *page = stack->currentWidget();

    QWidgetList list;
    list.append(page);
    KFormDesigner::Command *com = new KFormDesigner::DeleteWidgetCommand(*m_container->form(), list);

    // raise prev/next widget
    int index = stack->indexOf(page);
    if (index > 0) {
        index--;
    }
    else if (index < (stack->count()-1)) {
        index++;
    }
    else {
        index = -1;
    }
    if (index >= 0) {
        stack->setCurrentIndex(index);
    }
    stack->removeWidget(page);
    m_container->form()->addCommand(com);
}

GoToStackPageAction::GoToStackPageAction(Direction direction,
                                         KFormDesigner::Container *container,
                                         QWidget *receiver, QObject *parent)
    : KAction(KIcon(direction == Previous ? koIconName("go-previous") : koIconName("go-next")),
              direction == Previous ? i18nc("Go to Previous Page of a Stacked Widget", "Go to Previous Page")
                                    : i18nc("Go to Next Page of a Stacked Widget", "Go to Next Page"),
              parent)
    , m_direction(direction)
    , m_container(container)
    , m_receiver(receiver)
{
    connect(this, SIGNAL(triggered()), this, SLOT(slotTriggered()));
    QStackedWidget *stack = dynamic_cast<QStackedWidget*>(m_receiver);
    if (!stack || !stack->widget(nextWidgetIndex())) {
        setEnabled(false);
    }
}

int GoToStackPageAction::nextWidgetIndex() const
{
    QStackedWidget *stack = dynamic_cast<QStackedWidget*>(m_receiver);
    if (!stack)
        return -1;
    return stack->currentIndex() + (m_direction == Previous ? -1 : 1);
}

void GoToStackPageAction::slotTriggered()
{
    QStackedWidget *stack = dynamic_cast<QStackedWidget*>(m_receiver);
    if (stack && stack->widget(nextWidgetIndex())) {
        stack->setCurrentIndex(nextWidgetIndex());
    }
}

/////   The factory /////////////////////////

ContainerFactory::ContainerFactory(QObject *parent, const QVariantList &)
        : KFormDesigner::WidgetFactory(parent, "containers")
{
#if 0 // not needed?
    KFormDesigner::WidgetInfo *wBtnGroup = new KFormDesigner::WidgetInfo(this);
    wBtnGroup->setIconName(koIconName("frame"));
    wBtnGroup->setClassName("QButtonGroup");
    wBtnGroup->setName(i18n("Button Group"));
    wBtnGroup->setNamePrefix(
        i18nc("Widget name. This string will be used to name widgets of this class. It must _not_ contain white spaces and non latin1 characters.", "buttonGroup"));
    wBtnGroup->setDescription(i18n("A simple container to group buttons"));
    addClass(wBtnGroup);
#endif

    KFormDesigner::WidgetInfo *wTabWidget = new KFormDesigner::WidgetInfo(this);
    wTabWidget->setIconName(koIconName("tabwidget"));
    wTabWidget->setClassName("KFDTabWidget");
    wTabWidget->addAlternateClassName("KTabWidget");
    wTabWidget->addAlternateClassName("QTabWidget");
//tmp: wTabWidget->setSavingName("QTabWidget");
    wTabWidget->setSavingName("KTabWidget");
    wTabWidget->setIncludeFileName("ktabwidget.h");
    wTabWidget->setName(i18n("Tab Widget"));
    wTabWidget->setNamePrefix(
        i18nc("Widget name. This string will be used to name widgets of this class. It must _not_ contain white spaces and non latin1 characters.", "tabWidget"));
    wTabWidget->setDescription(i18n("A widget to display multiple pages using tabs"));
    addClass(wTabWidget);

    KFormDesigner::WidgetInfo *wWidget = new KFormDesigner::WidgetInfo(this);
    wWidget->setIconName(koIconName("frame"));
    wWidget->setClassName("QWidget");
    wWidget->addAlternateClassName("ContainerWidget");
    wWidget->setName(i18n("Basic container"));
    wWidget->setNamePrefix(
        i18nc("Widget name. This string will be used to name widgets of this class. It must _not_ contain white spaces and non latin1 characters.", "container"));
    wWidget->setDescription(i18n("An empty container with no frame"));
    addClass(wWidget);

    KFormDesigner::WidgetInfo *wGroupBox = new KFormDesigner::WidgetInfo(this);
    wGroupBox->setIconName(koIconName("groupbox"));
    wGroupBox->setClassName("QGroupBox");
    wGroupBox->addAlternateClassName("GroupBox");
    wGroupBox->setName(i18n("Group Box"));
    wGroupBox->setNamePrefix(
        i18nc("Widget name. This string will be used to name widgets of this class. It must _not_ contain white spaces and non latin1 characters.", "groupBox"));
    wGroupBox->setDescription(i18n("A container to group some widgets"));
    addClass(wGroupBox);

    KFormDesigner::WidgetInfo *wFrame = new KFormDesigner::WidgetInfo(this);
    wFrame->setIconName(koIconName("frame"));
    wFrame->setClassName("QFrame");
    wFrame->setName(i18n("Frame"));
    wFrame->setNamePrefix(
        i18nc("Widget name. This string will be used to name widgets of this class. It must _not_ contain white spaces and non latin1 characters.", "frame"));
    wFrame->setDescription(i18n("A simple frame container"));
    addClass(wFrame);

    KFormDesigner::WidgetInfo *wWidgetStack = new KFormDesigner::WidgetInfo(this);
    wWidgetStack->setIconName(koIconName("widgetstack"));
    wWidgetStack->setClassName("QStackedWidget");
    wWidgetStack->addAlternateClassName("QWidgetStack");
    wWidgetStack->setName(i18n("Widget Stack"));
    wWidgetStack->setNamePrefix(
        i18nc("Widget name. This string will be used to name widgets of this class. It must _not_ contain white spaces and non latin1 characters.", "widgetStack"));
    wWidgetStack->setDescription(i18n("A container with multiple pages"));
    addClass(wWidgetStack);

    KFormDesigner::WidgetInfo *wHBox = new KFormDesigner::WidgetInfo(this);
    wHBox->setIconName(koIconName("frame"));
    wHBox->setClassName("HBox");
    wHBox->setName(i18n("Horizontal Box"));
    wHBox->setNamePrefix(
        i18nc("Widget name. This string will be used to name widgets of this class. It must _not_ contain white spaces and non latin1 characters.", "horizontalBox"));
    wHBox->setDescription(i18n("A simple container to group widgets horizontally"));
    addClass(wHBox);

    KFormDesigner::WidgetInfo *wVBox = new KFormDesigner::WidgetInfo(this);
    wVBox->setIconName(koIconName("frame"));
    wVBox->setClassName("VBox");
    wVBox->setName(i18n("Vertical Box"));
    wVBox->setNamePrefix(
        i18nc("Widget name. This string will be used to name widgets of this class. It must _not_ contain white spaces and non latin1 characters.", "verticalBox"));
    wVBox->setDescription(i18n("A simple container to group widgets vertically"));
    addClass(wVBox);

    KFormDesigner::WidgetInfo *wGrid = new KFormDesigner::WidgetInfo(this);
    wGrid->setIconName(koIconName("frame"));
    wGrid->setClassName("Grid");
    wGrid->setName(i18n("Grid Box"));
    wGrid->setNamePrefix(
        i18nc("Widget name. This string will be used to name widgets of this class. It must _not_ contain white spaces and non latin1 characters.", "gridBox"));
    wGrid->setDescription(i18n("A simple container to group widgets in a grid"));
    addClass(wGrid);

    KFormDesigner::WidgetInfo *wSplitter = new KFormDesigner::WidgetInfo(this);
    wSplitter->setIconName(koIconNameNeededWithSubs("an icon for a container to allow user to resize its children",
                                                    "splitter","frame"));
    wSplitter->setClassName("Splitter");
    wSplitter->addAlternateClassName("QSplitter");
    wSplitter->setName(i18n("Splitter"));
    wSplitter->setNamePrefix(
        i18nc("Widget name. This string will be used to name widgets of this class. It must _not_ contain white spaces and non latin1 characters.", "splitter"));
    wSplitter->setDescription(i18n("A container that enables user to resize its children"));
    wSplitter->setAutoSaveProperties(QList<QByteArray>() << "orientation");
    addClass(wSplitter);

    KFormDesigner::WidgetInfo *wHFlow = new KFormDesigner::WidgetInfo(this);
    wHFlow->setIconName(koIconNameNeededWithSubs("icon for a container to group widgets by rows","hflow","frame"));
    wHFlow->setClassName("HFlow");
    wHFlow->setName(i18n("Row Layout"));
    wHFlow->setNamePrefix(
        i18nc("Widget name. This string will be used to name widgets of this class. It must _not_ contain white spaces and non latin1 characters.", "rowLayout"));
    wHFlow->setDescription(i18n("A simple container to group widgets by rows"));
    addClass(wHFlow);

    KFormDesigner::WidgetInfo *wVFlow = new KFormDesigner::WidgetInfo(this);
    wVFlow->setIconName(koIconNameNeededWithSubs("icon for a container to group widgets by columns","vflow","frame"));
    wVFlow->setClassName("VFlow");
    wVFlow->setName(i18n("Column Layout"));
    wVFlow->setNamePrefix(
        i18nc("Widget name. This string will be used to name widgets of this class. It must _not_ contain white spaces and non latin1 characters.", "columnLayout"));
    wVFlow->setDescription(i18n("A simple container to group widgets by columns"));
    addClass(wVFlow);

//todo
#if 0
    KFormDesigner::WidgetInfo *wSubForm = new KFormDesigner::WidgetInfo(this);
    wSubForm->setIconName(koIconName("form"));
    wSubForm->setClassName("SubForm");
    wSubForm->setName(i18n("Sub Form"));
    wSubForm->setNamePrefix(
        i18nc("Widget name. This string will be used to name widgets of this class. It must _not_ contain white spaces and non latin1 characters.", "subForm"));
    wSubForm->setDescription(i18n("A form widget included in another Form"));
    wSubForm->setAutoSyncForProperty("formName", false);
    addClass(wSubForm);
#endif

    //groupbox
    setPropertyDescription("title", i18n("Title"));
    setPropertyDescription("flat", i18n("Flat"));

    //tab widget
    setPropertyDescription("tabPosition", i18n("Tab Position"));
    setPropertyDescription("currentIndex", i18n("Current Page"));
    setPropertyDescription("tabShape", i18n("Tab Shape"));
    setPropertyDescription("elideMode", i18nc("Tab Widget's Elide Mode", "Elide Mode"));
    setPropertyDescription("usesScrollButtons", i18nc("Tab Widget uses scroll buttons", "Scroll Buttons"));

    setPropertyDescription("tabPosition", i18n("Tab Position"));
    setPropertyDescription("tabPosition", i18n("Tab Position"));
    setPropertyDescription("tabsClosable", i18n("Closable Tabs"));
    setPropertyDescription("movable", i18n("Movable Tabs"));
    setPropertyDescription("documentMode", i18n("Document Mode"));

    setValueDescription("Rounded", i18nc("for Tab Shape", "Rounded"));
    setValueDescription("Triangular", i18nc("for Tab Shape", "Triangular"));
}

ContainerFactory::~ContainerFactory()
{
}

QWidget* 
ContainerFactory::createWidget(const QByteArray &c, QWidget *p, const char *n,
                                  KFormDesigner::Container *container,
                                  CreateWidgetOptions options)
{
    kDebug() << c;
    QWidget *w = 0;
    bool createContainer = false;
    if (c == "KFDTabWidget") {
        KFDTabWidget *tab = new KFDTabWidget(container, p);
        w = tab;
#if defined(USE_KTabWidget)
        tab->setTabReorderingEnabled(true);
        connect(tab, SIGNAL(movedTab(int,int)), this, SLOT(reorderTabs(int,int)));
#endif
        kDebug() << "Creating ObjectTreeItem:";
        container->form()->objectTree()->addItem(container->objectTree(),
                new KFormDesigner::ObjectTreeItem(
                    container->form()->library()->displayName(c), n, tab, container));
    } else if (c == "QWidget") {
        w = new ContainerWidget(p);
        w->setObjectName(n);
        new KFormDesigner::Container(container, w, p);
        return w;
    } else if (c == "QGroupBox") {
        QString text = container->form()->library()->textForWidgetName(n, c);
        w = new GroupBox(text, p);
        createContainer = true;
    } else if (c == "QFrame") {
        QFrame *frm = new QFrame(p);
        w = frm;
        frm->setLineWidth(2);
        frm->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
        createContainer = true;
    } else if (c == "QStackedWidget" || /* compat */ c == "QWidgetStack") {
        QStackedWidget *stack = new QStackedWidget(p);
        w = stack;
        stack->setLineWidth(2);
        stack->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
        kDebug() << "Creating ObjectTreeItem:";
        container->form()->objectTree()->addItem(container->objectTree(),
                new KFormDesigner::ObjectTreeItem(
                    container->form()->library()->displayName(c), n, stack, container));

        if (container->form()->interactiveMode()) {
            AddStackPageAction(container, stack, 0).trigger(); // addStackPage();
        }
    } else if (c == "HBox") {
        w = new HBox(p);
        createContainer = true;
    } else if (c == "VBox") {
        w = new VBox(p);
        createContainer = true;
    } else if (c == "Grid") {
        w = new Grid(p);
        createContainer = true;
    } else if (c == "HFlow") {
        w = new HFlow(p);
        createContainer = true;
    } else if (c == "VFlow") {
        w = new VFlow(p);
        createContainer = true;
#if 0 //todo
    } else if (c == "SubForm") {
        w = new SubForm(container->form(), p);
#endif
    } else if (c == "QSplitter") {
        QSplitter *split = new QSplitter(p);
        w = split;
        if (0 == (options & WidgetFactory::AnyOrientation)) {
            split->setOrientation(
                (options & WidgetFactory::VerticalOrientation) ? Qt::Vertical : Qt::Horizontal);
        }
        createContainer = true;
    }

    if (w) {
        w->setObjectName(n);
        kDebug() << w << w->objectName() << "created";
    }
    if (createContainer) {
        (void)new KFormDesigner::Container(container, w, container);
    }

    if (c == "KFDTabWidget") {
        // if we are loading, don't add this tab
        if (container->form()->interactiveMode()) {
            TabWidgetBase *tab = dynamic_cast<TabWidgetBase*>(w);
            AddTabAction(container, tab, 0).slotTriggered(); // addTabPage();
        }
    }
    return w;
}

bool
ContainerFactory::previewWidget(const QByteArray &classname, 
    QWidget *widget, KFormDesigner::Container *container)
{
    if (classname == "QStackedWidget" || /* compat */ classname == "QWidgetStack") {
        QStackedWidget *stack = dynamic_cast<QStackedWidget*>(widget);
        KFormDesigner::ObjectTreeItem *tree = container->form()->objectTree()->lookup(
            widget->objectName());
        if (!tree->modifiedProperties()->contains("frameShape"))
            stack->setFrameStyle(QFrame::NoFrame);
    }
    return true;
}

bool
ContainerFactory::createMenuActions(const QByteArray &classname, QWidget *w,
                                    QMenu *menu, KFormDesigner::Container *container)
{
    QWidget *pw = w->parentWidget();
    //kDebug() << classname << w->metaObject()->className();

    if (classname == "KFDTabWidget" || pw->parentWidget()->inherits("QTabWidget")) {
#ifdef __GNUC__
#warning port this: setWidget(pw->parentWidget(), m_container->toplevel());
#else
#pragma WARNING( port this: setWidget(pw->parentWidget(), m_container->toplevel()); )
#endif
#if 0
        if (pw->parentWidget()->inherits("QTabWidget")) {
            setWidget(pw->parentWidget(), m_container->toplevel());
        }
#endif

        TabWidgetBase *tab = dynamic_cast<TabWidgetBase*>(w);
        if (tab) {
            menu->addAction( new AddTabAction(container, tab, menu) );
            menu->addAction( new RenameTabAction(container, tab, menu) );
            menu->addAction( new RemoveTabAction(container, tab, menu) );
        }
        return true;
    }
    else if (    (KexiUtils::objectIsA(pw, "QStackedWidget") || /* compat */ KexiUtils::objectIsA(pw, "QWidgetStack"))
              && !pw->parentWidget()->inherits("QTabWidget")
            )
    {
        QStackedWidget *stack = dynamic_cast<QStackedWidget*>(pw);
#ifdef __GNUC__
#warning port this: setWidget( pw, container->form()->objectTree()->lookup(stack->objectName())->parent()->container() );
#else
#pragma WARNING( port this: setWidget( pw, container->form()->objectTree()->lookup(stack->objectName())->parent()->container() ); )
#endif
#if 0
        setWidget(
            pw,
            container->form()->objectTree()->lookup(stack->objectName())->parent()->container()
        );
#endif
        KFormDesigner::Container *parentContainer
            = container->form()->objectTree()->lookup(stack->objectName())->parent()->container();
        menu->addAction( new AddStackPageAction(parentContainer, pw, menu) );
        menu->addAction( new RemoveStackPageAction(parentContainer, pw, menu) );
        menu->addAction( new GoToStackPageAction(GoToStackPageAction::Previous, parentContainer, pw, menu) );
        menu->addAction( new GoToStackPageAction(GoToStackPageAction::Next, parentContainer, pw, menu) );
        return true;
    }
    return false;
}

bool
ContainerFactory::startInlineEditing(InlineEditorCreationArguments& /*args*/)
{
    return false;
}

bool
ContainerFactory::saveSpecialProperty(const QByteArray &, const QString &name, const QVariant &, QWidget *w, QDomElement &parentNode, QDomDocument &parent)
{
    if ((name == "title") && (w->parentWidget()->parentWidget()->inherits("QTabWidget"))) {
        TabWidgetBase *tab = dynamic_cast<TabWidgetBase*>(w->parentWidget()->parentWidget());
        KFormDesigner::FormIO::savePropertyElement(
            parentNode, parent, "attribute", "title", tab->tabText(tab->indexOf(w)));
    } else if ((name == "stackIndex") 
        && (KexiUtils::objectIsA(w->parentWidget(), "QStackedWidget") || /*compat*/ KexiUtils::objectIsA(w->parentWidget(), "QWidgetStack")))
    {
        QStackedWidget *stack = dynamic_cast<QStackedWidget*>(w->parentWidget());
        KFormDesigner::FormIO::savePropertyElement(
            parentNode, parent, "attribute", "stackIndex", stack->indexOf(w));
    } else
        return false;
    return true;
}

bool
ContainerFactory::readSpecialProperty(const QByteArray &, QDomElement &node, QWidget *w,
                                      KFormDesigner::ObjectTreeItem *item)
{
    KFormDesigner::Form *form = item->container() ? item->container()->form() : item->parent()->container()->form();
    const QString name( node.attribute("name") );
    if ((name == "title") && (item->parent()->widget()->inherits("QTabWidget"))) {
        TabWidgetBase *tab = dynamic_cast<TabWidgetBase*>(w->parentWidget());
        tab->addTab(w, node.firstChild().toElement().text());
        item->addModifiedProperty("title", node.firstChild().toElement().text());
        return true;
    }

    if ((name == "stackIndex")
        && (KexiUtils::objectIsA(w->parentWidget(), "QStackedWidget")
            || /*compat*/ KexiUtils::objectIsA(w->parentWidget(), "QWidgetStack")))
    {
        QStackedWidget *stack = dynamic_cast<QStackedWidget*>(w->parentWidget());
        int index = KFormDesigner::FormIO::readPropertyValue(form, node.firstChild(), w, name).toInt();
        stack->insertWidget(index, w);
        stack->setCurrentWidget(w);
        item->addModifiedProperty("stackIndex", index);
        return true;
    }

    return false;
}

bool
ContainerFactory::isPropertyVisibleInternal(const QByteArray &classname,
        QWidget *w, const QByteArray &property, bool isTopLevel)
{
    bool ok = true;

    if (   classname == "HBox" || classname == "VBox" || classname == "Grid"
        || classname == "HFlow" || classname == "VFlow")
    {
        return property == "objectName" || property == "geometry";
    }
    else if (classname == "QGroupBox") {
        ok =
#ifndef KEXI_SHOW_UNFINISHED
            /*! @todo Hidden for now in Kexi. "checkable" and "checked" props need adding
            a fake properties which will allow to properly work in design mode, otherwise
            child widgets become frozen when checked==true */
            (KFormDesigner::WidgetFactory::advancedPropertiesVisible() || (property != "checkable" && property != "checked")) &&
#endif
            true
            ;
    } else if (classname == "KFDTabWidget") {
        ok = (KFormDesigner::WidgetFactory::advancedPropertiesVisible() || (property != "tabReorderingEnabled" && property != "hoverCloseButton" && property != "hoverCloseButtonDelayed"));
    }

    return ok && WidgetFactory::isPropertyVisibleInternal(classname, w, property, isTopLevel);
}

bool
ContainerFactory::changeInlineText(KFormDesigner::Form *form, QWidget *widget,
                                   const QString &text, QString &oldText)
{
    oldText = widget->property("text").toString();
    changeProperty(form, widget, "text", text);
    return true;
}

void
ContainerFactory::resizeEditor(QWidget *editor, QWidget *widget, const QByteArray &)
{
    QSize s = widget->size();
    editor->move(widget->x() + 2, widget->y() - 5);
    editor->resize(s.width() - 20, widget->fontMetrics().height() + 10);
}

// Widget-specific slots used in menu items

void ContainerFactory::reorderTabs(int oldpos, int newpos)
{
    KFDTabWidget *tabWidget = qobject_cast<KFDTabWidget*>(sender());
    KFormDesigner::ObjectTreeItem *tab = tabWidget->container()->form()->objectTree()->lookup(tabWidget->objectName());
    if (!tab)
        return;

    tab->children()->move(oldpos, newpos);
}

KFormDesigner::ObjectTreeItem* ContainerFactory::selectableItem(KFormDesigner::ObjectTreeItem* item)
{
    if (item->parent() && item->parent()->widget()) {
        if (qobject_cast<QTabWidget*>(item->parent()->widget())) {
            // tab widget's page
            return item->parent();
        }
    }
    return item;
}

K_EXPORT_KEXI_FORM_WIDGET_FACTORY_PLUGIN(ContainerFactory, containers)

#include "containerfactory.moc"
