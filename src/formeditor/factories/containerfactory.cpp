/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2006-2009 Jarosław Staniek <staniek@kde.org>

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

#include <QStackedWidget>
#include <QFrame>
#include <qdom.h>
#include <Q3ScrollView>
#include <QTabBar>
#include <QSplitter>
#include <QLayout>
#include <QPainter>
#include <QPaintEvent>
#include <QFileInfo>

#include <KGenericFactory>
#include <KTextEdit>
#include <KLineEdit>
#include <KLocale>
#include <KDebug>
#include <kdeversion.h>
#include <KInputDialog>
#include <KIcon>
#include <KAction>

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
        , m_preview(false)
{
}

HBox::~HBox()
{
}

void
HBox::paintEvent(QPaintEvent *)
{
    if (m_preview) return;
    QPainter p(this);
    p.setPen(QPen(Qt::red, 2, Qt::DashLine));
    p.drawRect(1, 1, width() - 1, height() - 1);
}

VBox::VBox(QWidget *parent)
        : QFrame(parent)
        , m_preview(false)
{
}

VBox::~VBox()
{
}

void
VBox::paintEvent(QPaintEvent *)
{
    if (m_preview) return;
    QPainter p(this);
    p.setPen(QPen(Qt::blue, 2, Qt::DashLine));
    p.drawRect(1, 1, width() - 1, height() - 1);
}

Grid::Grid(QWidget *parent)
        : QFrame(parent)
        , m_preview(false)
{
}

Grid::~Grid()
{
}

void
Grid::paintEvent(QPaintEvent *)
{
    if (m_preview) return;
    QPainter p(this);
    p.setPen(QPen(Qt::darkGreen, 2, Qt::DashLine));
    p.drawRect(1, 1, width() - 1, height() - 1);
}

HFlow::HFlow(QWidget *parent)
        : QFrame(parent)
        , m_preview(false)
{
}

HFlow::~HFlow()
{
}

void
HFlow::paintEvent(QPaintEvent *)
{
    if (m_preview) return;
    QPainter p(this);
    p.setPen(QPen(Qt::magenta, 2, Qt::DashLine));
    p.drawRect(1, 1, width() - 1, height() - 1);
}

VFlow::VFlow(QWidget *parent)
        : QFrame(parent)
        , m_preview(false)
{
}

VFlow::~VFlow()
{
}

void
VFlow::paintEvent(QPaintEvent *)
{
    if (m_preview) return;
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

///////  Tab related command (to allow tab creation/deletion undoing)

InsertPageCommand::InsertPageCommand(KFormDesigner::Container *container, QWidget *parent)
        : KFormDesigner::Command()
{
    m_containername = container->widget()->objectName();
    m_form = container->form();
    m_parentname = parent->objectName();
    setText( i18n("Add Page") );
}

void
InsertPageCommand::execute()
{
    KFormDesigner::Container *container = m_form->objectTree()->lookup(m_containername)->container();
    QWidget *parent = m_form->objectTree()->lookup(m_parentname)->widget();
    if (m_name.isEmpty()) {
        m_name = container->form()->objectTree()->generateUniqueName(
                     container->form()->library()->displayName("QWidget").toLatin1(),
                     /*!numberSuffixRequired*/false);
    }

    QWidget *page = container->form()->library()->createWidget(
        "QWidget", parent, m_name.toLatin1(), container);
    page->setAutoFillBackground(true);
//    page->setPaletteBackgroundColor(Qt::red);
    KFormDesigner::ObjectTreeItem *item = container->form()->objectTree()->lookup(m_name);

    QByteArray classname = parent->metaObject()->className();
    if (classname == "KFDTabWidget") {
        TabWidgetBase *tab = dynamic_cast<TabWidgetBase*>(parent);
        QString n = i18n("Page %1", tab->count() + 1);
        tab->addTab(page, n);
        tab->setCurrentWidget(page);
        item->addModifiedProperty("title", n);
    } else if (classname == "QStackedWidget" || /* compat */ classname == "QWidgetStack") {
        QStackedWidget *stack = dynamic_cast<QStackedWidget*>(parent);
        stack->addWidget(page);
        stack->setCurrentWidget(page);
        item->addModifiedProperty("stackIndex", stack->indexOf(page));
    }
}

void InsertPageCommand::undo()
{
    QWidget *page = m_form->objectTree()->lookup(m_name)->widget();
    QWidget *parent = m_form->objectTree()->lookup(m_parentname)->widget();

    QWidgetList list;
    list.append(page);
    KFormDesigner::Command *com = new KFormDesigner::DeleteWidgetCommand(*m_form, list);

    QByteArray classname = parent->metaObject()->className();
    if (classname == "KFDTabWidget") {
        TabWidgetBase *tab = dynamic_cast<TabWidgetBase*>(parent);
        tab->removeTab(tab->indexOf(page));
    } else if (classname == "QStackedWidget" || /* compat */ classname == "QWidgetStack") {
        QStackedWidget *stack = dynamic_cast<QStackedWidget*>(parent);
        int index = stack->indexOf(page);
        if (index > 0)
            index--;
        else if (index < (stack->count()-1))
            index++;
        else
            index = -1;

        if (index >= 0)
            stack->setCurrentIndex(index);
        stack->removeWidget(page);
    }

    com->execute();
    delete com;
}

/////// Sub forms ////////////////////////:

#if 0
SubForm::SubForm(KFormDesigner::Form *parentForm, QWidget *parent)
        : Q3ScrollView(parent), m_parentForm(parentForm), m_form(0), m_widget(0)
{
    setFrameStyle(Q3Frame::WinPanel | Q3Frame::Sunken);
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

//! Action of adding tab to a tab widget
//! Keeps context expressed using container and receiver widget
class AddTabAction : public KAction
{
public:
    AddTabAction(KFormDesigner::Container *container,
                 QWidget *receiver, QObject *parent);
protected slots:
    void slotTriggered();
private:
    KFormDesigner::Container *m_container;
    QWidget *m_receiver;
};

AddTabAction::AddTabAction(KFormDesigner::Container *container, 
                           QWidget *receiver, QObject *parent)
    : KAction(KIcon("tab-new"), i18nc("Add page to tab widget", "Add Page"),
              parent)
    , m_container(container)
    , m_receiver(receiver)
{
    connect(this, SIGNAL(triggered()), this, SLOT(slotTriggered()));
}

void AddTabAction::slotTriggered()
{
    if (!m_receiver->inherits("QTabWidget"))
        return;
    KFormDesigner::Command *command = new InsertPageCommand(m_container, m_receiver);
    if (dynamic_cast<TabWidgetBase*>(m_receiver)->count() == 0) {
        command->execute();
        delete command;
    }
    else {
        m_container->form()->addCommand(command);
    }
}

//! Action of removing tab from a tab widget
//! Keeps context expressed using container and receiver widget
class RemoveTabAction : public KAction
{
public:
    RemoveTabAction(KFormDesigner::Container *container,
                    QWidget *receiver, QObject *parent);
protected slots:
    void slotTriggered();
private:
    KFormDesigner::Container *m_container;
    QWidget *m_receiver;
};

RemoveTabAction::RemoveTabAction(KFormDesigner::Container *container,
                                 QWidget *receiver, QObject *parent)
    : KAction(KIcon("tab-close-other"), i18nc("Remove tab widget's page", "Remove Page"),
              parent)
    , m_container(container)
    , m_receiver(receiver)
{
    connect(this, SIGNAL(triggered()), this, SLOT(slotTriggered()));
    if (dynamic_cast<TabWidgetBase*>(m_receiver)->count() <= 1) {
        setEnabled(false);
    }
}

void RemoveTabAction::slotTriggered()
{
    if (!m_receiver->inherits("QTabWidget"))
        return;
    TabWidgetBase *tab = dynamic_cast<TabWidgetBase*>(m_receiver);
    QWidget *w = tab->currentWidget();

    QWidgetList list;
    list.append(w);
    KFormDesigner::Command *com = new KFormDesigner::DeleteWidgetCommand(*m_container->form(), list);
    tab->removePage(w);
    m_container->form()->addCommand(com);
}

//! Action renaming tab widget's tab
//! Keeps context expressed using container and receiver widget
class RenameTabAction : public KAction
{
public:
    RenameTabAction(KFormDesigner::Container *container,
                    QWidget *receiver, QObject *parent);
protected slots:
    void slotTriggered();
private:
    KFormDesigner::Container *m_container;
    QWidget *m_receiver;
};

RenameTabAction::RenameTabAction(KFormDesigner::Container *container,
                                 QWidget *receiver, QObject *parent)
    : KAction(KIcon("edit-rename"), i18nc("Rename tab widget's page", "Rename Page..."),
              parent)
    , m_container(container)
    , m_receiver(receiver)
{
    connect(this, SIGNAL(triggered()), this, SLOT(slotTriggered()));
}

void RenameTabAction::slotTriggered()
{
    if (!m_receiver->inherits("QTabWidget"))
        return;
    TabWidgetBase *tab = dynamic_cast<TabWidgetBase*>(m_receiver);
    QWidget *w = tab->currentWidget();
    bool ok;
    QString name = KInputDialog::getText(i18n("New Page Title"),
                                         i18n("Enter a new title for the current page:"),
                                         tab->tabText(tab->indexOf(w)), &ok, w->topLevelWidget());
    if (ok)
        tab->changeTab(w, name);
}

//! Action of adding page to a stacked widget
//! Keeps context expressed using container and receiver widget
class AddStackPageAction : public KAction
{
public:
    AddStackPageAction(KFormDesigner::Container *container,
                       QWidget *receiver, QObject *parent);
protected slots:
    void slotTriggered();
private:
    KFormDesigner::Container *m_container;
    QWidget *m_receiver;
};

AddStackPageAction::AddStackPageAction(KFormDesigner::Container *container,
                                       QWidget *receiver, QObject *parent)
    : KAction(KIcon("tab-new"), i18nc("Add page to a stacked widget", "Add Page..."),
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
    KFormDesigner::Command *command = new InsertPageCommand(m_container, m_receiver);
    if (!dynamic_cast<QStackedWidget*>(m_receiver)->currentWidget()) {
        command->execute();
        delete command;
    }
    else {
        m_container->form()->addCommand(command);
    }
}

//! Action of removing page from a stacked widget
//! Keeps context expressed using container and receiver widget
class RemoveStackPageAction : public KAction
{
public:
    RemoveStackPageAction(KFormDesigner::Container *container,
                          QWidget *receiver, QObject *parent);
protected slots:
    void slotTriggered();
private:
    KFormDesigner::Container *m_container;
    QWidget *m_receiver;
};

RemoveStackPageAction::RemoveStackPageAction(KFormDesigner::Container *container,
                                             QWidget *receiver, QObject *parent)
    : KAction(KIcon("tab-close-other"), i18nc("Remove page from a stacked widget", "Remove Page"),
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

//! Action of moving between pages of a stacked widget
//! Keeps context expressed using container and receiver widget
class GoToStackPageAction : public KAction
{
public:
    enum Direction {
        Previous,
        Next
    };
    GoToStackPageAction(Direction direction, KFormDesigner::Container *container,
                        QWidget *receiver, QObject *parent);
protected slots:
    void slotTriggered();
private:
    int nextWidgetIndex() const;

    Direction m_direction;
    KFormDesigner::Container *m_container;
    QWidget *m_receiver;
};

GoToStackPageAction::GoToStackPageAction(Direction direction,
                                         KFormDesigner::Container *container,
                                         QWidget *receiver, QObject *parent)
    : KAction(KIcon(direction == Previous ? "go-previous" : "go-next"),
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

ContainerFactory::ContainerFactory(QObject *parent, const QStringList &)
        : KFormDesigner::WidgetFactory(parent, "containers")
{
#if 0 // not needed?
    KFormDesigner::WidgetInfo *wBtnGroup = new KFormDesigner::WidgetInfo(this);
    wBtnGroup->setPixmap("frame");
    wBtnGroup->setClassName("QButtonGroup");
    wBtnGroup->setName(i18n("Button Group"));
    wBtnGroup->setNamePrefix(
        i18nc("Widget name. This string will be used to name widgets of this class. It must _not_ contain white spaces and non latin1 characters.", "buttonGroup"));
    wBtnGroup->setDescription(i18n("A simple container to group buttons"));
    addClass(wBtnGroup);
#endif

    KFormDesigner::WidgetInfo *wTabWidget = new KFormDesigner::WidgetInfo(this);
    wTabWidget->setPixmap("tabwidget");
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
    wWidget->setPixmap("frame");
    wWidget->setClassName("QWidget");
    wWidget->addAlternateClassName("ContainerWidget");
    wWidget->setName(i18n("Basic container"));
    wWidget->setNamePrefix(
        i18nc("Widget name. This string will be used to name widgets of this class. It must _not_ contain white spaces and non latin1 characters.", "container"));
    wWidget->setDescription(i18n("An empty container with no frame"));
    addClass(wWidget);

    KFormDesigner::WidgetInfo *wGroupBox = new KFormDesigner::WidgetInfo(this);
    wGroupBox->setPixmap("groupbox");
    wGroupBox->setClassName("QGroupBox");
    wGroupBox->setName(i18n("Group Box"));
    wGroupBox->setNamePrefix(
        i18nc("Widget name. This string will be used to name widgets of this class. It must _not_ contain white spaces and non latin1 characters.", "groupBox"));
    wGroupBox->setDescription(i18n("A container to group some widgets"));
    addClass(wGroupBox);

    KFormDesigner::WidgetInfo *wFrame = new KFormDesigner::WidgetInfo(this);
    wFrame->setPixmap("frame");
    wFrame->setClassName("QFrame");
    wFrame->setName(i18n("Frame"));
    wFrame->setNamePrefix(
        i18nc("Widget name. This string will be used to name widgets of this class. It must _not_ contain white spaces and non latin1 characters.", "frame"));
    wFrame->setDescription(i18n("A simple frame container"));
    addClass(wFrame);

    KFormDesigner::WidgetInfo *wWidgetStack = new KFormDesigner::WidgetInfo(this);
    wWidgetStack->setPixmap("widgetstack");
    wWidgetStack->setClassName("QStackedWidget");
    wWidgetStack->addAlternateClassName("QWidgetStack");
    wWidgetStack->setName(i18n("Widget Stack"));
    wWidgetStack->setNamePrefix(
        i18nc("Widget name. This string will be used to name widgets of this class. It must _not_ contain white spaces and non latin1 characters.", "widgetStack"));
    wWidgetStack->setDescription(i18n("A container with multiple pages"));
    addClass(wWidgetStack);

    KFormDesigner::WidgetInfo *wHBox = new KFormDesigner::WidgetInfo(this);
    wHBox->setPixmap("frame");
    wHBox->setClassName("HBox");
    wHBox->setName(i18n("Horizontal Box"));
    wHBox->setNamePrefix(
        i18nc("Widget name. This string will be used to name widgets of this class. It must _not_ contain white spaces and non latin1 characters.", "horizontalBox"));
    wHBox->setDescription(i18n("A simple container to group widgets horizontally"));
    addClass(wHBox);

    KFormDesigner::WidgetInfo *wVBox = new KFormDesigner::WidgetInfo(this);
    wVBox->setPixmap("frame");
    wVBox->setClassName("VBox");
    wVBox->setName(i18n("Vertical Box"));
    wVBox->setNamePrefix(
        i18nc("Widget name. This string will be used to name widgets of this class. It must _not_ contain white spaces and non latin1 characters.", "verticalBox"));
    wVBox->setDescription(i18n("A simple container to group widgets vertically"));
    addClass(wVBox);

    KFormDesigner::WidgetInfo *wGrid = new KFormDesigner::WidgetInfo(this);
    wGrid->setPixmap("frame");
    wGrid->setClassName("Grid");
    wGrid->setName(i18n("Grid Box"));
    wGrid->setNamePrefix(
        i18nc("Widget name. This string will be used to name widgets of this class. It must _not_ contain white spaces and non latin1 characters.", "gridBox"));
    wGrid->setDescription(i18n("A simple container to group widgets in a grid"));
    addClass(wGrid);

    KFormDesigner::WidgetInfo *wSplitter = new KFormDesigner::WidgetInfo(this);
//! @todo horizontal/vertical splitter icons
    wSplitter->setPixmap("frame");
    wSplitter->setClassName("Splitter");
    wSplitter->addAlternateClassName("QSplitter");
    wSplitter->setName(i18n("Splitter"));
    wSplitter->setNamePrefix(
        i18nc("Widget name. This string will be used to name widgets of this class. It must _not_ contain white spaces and non latin1 characters.", "splitter"));
    wSplitter->setDescription(i18n("A container that enables user to resize its children"));
    addClass(wSplitter);

    KFormDesigner::WidgetInfo *wHFlow = new KFormDesigner::WidgetInfo(this);
//! @todo hflow icon
    wHFlow->setPixmap("frame");
    wHFlow->setClassName("HFlow");
    wHFlow->setName(i18n("Row Layout"));
    wHFlow->setNamePrefix(
        i18nc("Widget name. This string will be used to name widgets of this class. It must _not_ contain white spaces and non latin1 characters.", "rowLayout"));
    wHFlow->setDescription(i18n("A simple container to group widgets by rows"));
    addClass(wHFlow);

    KFormDesigner::WidgetInfo *wVFlow = new KFormDesigner::WidgetInfo(this);
//! @todo vflow icon
    wVFlow->setPixmap("frame");
    wVFlow->setClassName("VFlow");
    wVFlow->setName(i18n("Column Layout"));
    wVFlow->setNamePrefix(
        i18nc("Widget name. This string will be used to name widgets of this class. It must _not_ contain white spaces and non latin1 characters.", "columnLayout"));
    wVFlow->setDescription(i18n("A simple container to group widgets by columns"));
    addClass(wVFlow);

//todo
#if 0
    KFormDesigner::WidgetInfo *wSubForm = new KFormDesigner::WidgetInfo(this);
    wSubForm->setPixmap("form");
    wSubForm->setClassName("SubForm");
    wSubForm->setName(i18n("Sub Form"));
    wSubForm->setNamePrefix(
        i18nc("Widget name. This string will be used to name widgets of this class. It must _not_ contain white spaces and non latin1 characters.", "subForm"));
    wSubForm->setDescription(i18n("A form widget included in another Form"));
    wSubForm->setAutoSyncForProperty("formName", false);
    addClass(wSubForm);
#endif

    //groupbox
    m_propDesc["title"] = i18n("Title");
    m_propDesc["flat"] = i18n("Flat");

    //tab widget
    m_propDesc["tabPosition"] = i18n("Tab Position");
    m_propDesc["currentIndex"] = i18n("Current Page");
    m_propDesc["tabShape"] = i18n("Tab Shape");
    m_propDesc["elideMode"] = i18nc("Tab Widget's Elide Mode", "Elide Mode");
    m_propDesc["usesScrollButtons"] = i18nc("Tab Widget uses scroll buttons", "Scroll Buttons");

    m_propDesc["tabPosition"] = i18n("Tab Position");
    m_propDesc["tabPosition"] = i18n("Tab Position");

    m_propValDesc["Rounded"] = i18nc("for Tab Shape", "Rounded");
    m_propValDesc["Triangular"] = i18nc("for Tab Shape", "Triangular");
}

ContainerFactory::~ContainerFactory()
{
}

QWidget* 
ContainerFactory::createWidget(const QByteArray &c, QWidget *p, const char *n,
                                  KFormDesigner::Container *container,
                                  CreateWidgetOptions options)
{
    QWidget *w = 0;
#if 0 // needed?
    if (c == "QButtonGroup") {
        QString text = container->form()->library()->textForWidgetName(n, c);
        QButtonGroup *group = new Q3ButtonGroup(/*i18n("Button Group")*/text, p);
        w = group;
        w->setObjectName(n);
        new KFormDesigner::Container(container, group, container);
    } else
#endif
    if (c == "KFDTabWidget") {
        KFDTabWidget *tab = new KFDTabWidget(container, p);
        w = tab;
        w->setObjectName(n);
#if defined(USE_KTabWidget)
        tab->setTabReorderingEnabled(true);
        connect(tab, SIGNAL(movedTab(int, int)), this, SLOT(reorderTabs(int, int)));
#endif
        container->form()->objectTree()->addItem(container->objectTree(),
                new KFormDesigner::ObjectTreeItem(
                    container->form()->library()->displayName(c), n, tab, container));

        // if we are loading, don't add this tab
        if (container->form()->interactiveMode()) {
//2.0            setWidget(tab, container);
            AddTabAction(container, tab, 0).trigger(); // addTabPage();
        }
    } else if (c == "QWidget") {
        w = new ContainerWidget(p);
        w->setObjectName(n);
        new KFormDesigner::Container(container, w, p);
    } else if (c == "QGroupBox") {
        QString text = container->form()->library()->textForWidgetName(n, c);
        w = new GroupBox(text, p);
        w->setObjectName(n);
        new KFormDesigner::Container(container, w, container);
    } else if (c == "QFrame") {
        QFrame *frm = new QFrame(p);
        w = frm;
        frm->setLineWidth(2);
        frm->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
        new KFormDesigner::Container(container, w, container);
    } else if (c == "QStackedWidget" || /* compat */ c == "QWidgetStack") {
        QStackedWidget *stack = new QStackedWidget(p);
        w = stack;
        stack->setLineWidth(2);
        stack->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
        container->form()->objectTree()->addItem(container->objectTree(),
                new KFormDesigner::ObjectTreeItem(
                    container->form()->library()->displayName(c), n, stack, container));

        if (container->form()->interactiveMode()) {
//2.0            setWidget(stack, container);
            AddStackPageAction(container, stack, 0).trigger(); // addStackPage();
        }
    } else if (c == "HBox") {
        w = new HBox(p);
        new KFormDesigner::Container(container, w, container);
    } else if (c == "VBox") {
        w = new VBox(p);
        new KFormDesigner::Container(container, w, container);
    } else if (c == "Grid") {
        w = new Grid(p);
        new KFormDesigner::Container(container, w, container);
    } else if (c == "HFlow") {
        w = new HFlow(p);
        new KFormDesigner::Container(container, w, container);
    } else if (c == "VFlow") {
        w = new VFlow(p);
        new KFormDesigner::Container(container, w, container);
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
        (void)new KFormDesigner::Container(container, split, container);
    }

    if (w) {
        setObjectName(n);
        return w;
    }
    return 0;
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
    else if (classname == "HBox")
        dynamic_cast<HBox*>(widget)->setPreviewMode();
    else if (classname == "VBox")
        dynamic_cast<VBox*>(widget)->setPreviewMode();
    else if (classname == "Grid")
        dynamic_cast<Grid*>(widget)->setPreviewMode();
    else if (classname == "HFlow")
        dynamic_cast<HFlow*>(widget)->setPreviewMode();
    else if (classname == "VFlow")
        dynamic_cast<VFlow*>(widget)->setPreviewMode();
    else
        return false;
    return true;
}

bool
ContainerFactory::createMenuActions(const QByteArray &classname, QWidget *w,
                                    QMenu *menu, KFormDesigner::Container *container)
{
//2.0    setWidget(w, container);
    QWidget *pw = w->parentWidget();

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

        menu->addAction( new AddTabAction(container, w, menu) );
        menu->addAction( new RenameTabAction(container, w, menu) );
        menu->addAction( new RemoveTabAction(container, w, menu) );
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
ContainerFactory::startInlineEditing(InlineEditorCreationArguments& args)
{
//2.0    m_container = container;
#if 0 // needed?
    if (classname == "QButtonGroup") {
        Q3ButtonGroup *group = static_cast<Q3ButtonGroup*>(w);
        QRect r = QRect(group->x() + 2, group->y() - 5, group->width() - 10, w->fontMetrics().height() + 10);
        createEditor(classname, group->title(), group, container, r, Qt::AlignLeft);
        return true;
    }
#endif
    if (args.classname == "QGroupBox") {
        QGroupBox *group = static_cast<QGroupBox*>(args.widget);
        args.text = group->title();
        args.geometry = QRect(group->x() + 2, group->y() - 5, group->width() - 10, args.widget->fontMetrics().height() + 10);
        return true;
    }
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
ContainerFactory::readSpecialProperty(const QByteArray &, QDomElement &node, QWidget *w, KFormDesigner::ObjectTreeItem *item)
{
    const QString name( node.attribute("name") );
    if ((name == "title") && (item->parent()->widget()->inherits("QTabWidget"))) {
        TabWidgetBase *tab = dynamic_cast<TabWidgetBase*>(w->parentWidget());
        tab->addTab(w, node.firstChild().toElement().text());
        item->addModifiedProperty("title", node.firstChild().toElement().text());
        return true;
    }

    if ((name == "stackIndex")
        && (KexiUtils::objectIsA(w->parentWidget(), "QStackedWidget") || /*compat*/ KexiUtils::objectIsA(w->parentWidget(), "QWidgetStack")))
    {
        QStackedWidget *stack = dynamic_cast<QStackedWidget*>(w->parentWidget());
        int index = KFormDesigner::FormIO::readPropertyValue(node.firstChild(), w, name).toInt();
        stack->insertWidget(index, w);
        stack->setCurrentWidget(w);
        item->addModifiedProperty("stackIndex", index);
        return true;
    }

    return false;
}

QList<QByteArray>
ContainerFactory::autoSaveProperties(const QByteArray &c)
{
    QList<QByteArray> lst;
    if (c == "QSplitter")
        lst << "orientation";
    return lst;
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
#ifdef KEXI_NO_UNFINISHED
            /*! @todo Hidden for now in Kexi. "checkable" and "checked" props need adding
            a fake properties which will allow to properly work in design mode, otherwise
            child widgets become frozen when checked==true */
            (m_showAdvancedProperties || (property != "checkable" && property != "checked")) &&
#endif
            true
            ;
    } else if (classname == "KFDTabWidget") {
        ok = (m_showAdvancedProperties || (property != "tabReorderingEnabled" && property != "hoverCloseButton" && property != "hoverCloseButtonDelayed"));
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

// Widget Specific slots used in menu items

// moved to AddTabAction
#if 0
void ContainerFactory::addTabPage()
{
    if (!widget()->inherits("QTabWidget"))
        return;
    KFormDesigner::Command *com = new InsertPageCommand(m_container, widget());
    if (dynamic_cast<TabWidgetBase*>(widget())->count() == 0) {
        com->execute();
        delete com;
    }
    else {
        m_container->form()->addCommand(com);
    }
}
#endif

// moved to RemoveTabAction
#if 0
void ContainerFactory::removeTabPage()
{
    if (!widget()->inherits("QTabWidget"))
        return;
    TabWidgetBase *tab = dynamic_cast<TabWidgetBase*>(widget());
    QWidget *w = tab->currentWidget();

    QWidgetList list;
    list.append(w);
    KFormDesigner::Command *com = new KFormDesigner::DeleteWidgetCommand(*m_container->form(), list);
    tab->removePage(w);
    m_container->form()->addCommand(com);
}
#endif

// moved to RenameTabAction
#if 0
void ContainerFactory::renameTabPage()
{
    if (!widget()->inherits("QTabWidget"))
        return;
    TabWidgetBase *tab = dynamic_cast<TabWidgetBase*>(widget());
    QWidget *w = tab->currentWidget();
    bool ok;

    QString name = KInputDialog::getText(i18n("New Page Title"),
                                         i18n("Enter a new title for the current page:"),
                                         tab->tabText(tab->indexOf(w)), &ok, w->topLevelWidget());
    if (ok)
        tab->changeTab(w, name);
}
#endif

void ContainerFactory::reorderTabs(int oldpos, int newpos)
{
    KFDTabWidget *tabWidget = qobject_cast<KFDTabWidget*>(sender());
    KFormDesigner::ObjectTreeItem *tab = tabWidget->container()->form()->objectTree()->lookup(tabWidget->objectName());
    if (!tab)
        return;

    tab->children()->move(oldpos, newpos);
}

// moved to AddStackPageAction
#if 0
void ContainerFactory::addStackPage()
{
    if (   !KexiUtils::objectIsA(widget(), "QStackedWidget")
        && /* compat */ !KexiUtils::objectIsA(widget(), "QWidgetStack"))
    {
        return;
    }
    KFormDesigner::Command *com = new InsertPageCommand(m_container, widget());
    if (!dynamic_cast<QStackedWidget*>(widget())->currentWidget()) {
        com->execute();
        delete com;
    }
    else {
        m_container->form()->addCommand(com);
    }
}
#endif

// moved to RemoveStackPageAction
#if 0
void ContainerFactory::removeStackPage()
{
    if (   !KexiUtils::objectIsA(widget(), "QStackedWidget")
        && /* compat */ !KexiUtils::objectIsA(widget(), "QWidgetStack"))
    {
        return;
    }
    QStackedWidget *stack = dynamic_cast<QStackedWidget*>(widget());
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
#endif

// moved to GoToStackPageAction
#if 0
void ContainerFactory::nextStackPage()
{
    QStackedWidget *stack = dynamic_cast<QStackedWidget*>(widget());
    int index = stack->currentIndex() + 1;
    if (stack->widget(index))
        stack->setCurrentIndex(index);
}
#endif

// moved to GoToStackPageAction
#if 0
void ContainerFactory::prevStackPage()
{
    QStackedWidget *stack = dynamic_cast<QStackedWidget*>(widget());
    int index = stack->currentIndex() - 1;
    if (stack->widget(index))
        stack->setCurrentIndex(index);
}
#endif

KFORMDESIGNER_WIDGET_FACTORY(ContainerFactory, containers)

#include "containerfactory.moc"
