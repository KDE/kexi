/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2003-2018 Jarosław Staniek <staniek@kde.org>

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

#include "KexiMainWindow_p.h"
#include "KexiObjectViewWidget.h"

#include <KToggleAction>

#include <QPainter>
#include <QDebug>
#include <QSplitter>

#include <KConfigGroup>

#include <KDbUtils>

#include <KexiPropertyPaneWidget.h>
#include <kexiutils/SmallToolButton.h>
#include <kexiutils/KexiTester.h>
#include <kexiutils/KexiFadeWidgetEffect.h>
#include <KexiIcon.h>
#include <KexiStyle.h>
#include <KexiProjectNavigator.h>
#include <core/kexipartmanager.h>
#include <KexiAssistantWidget.h>

EmptyMenuContentWidget::EmptyMenuContentWidget(QWidget* parent)
 : QWidget(parent)
{
    setAutoFillBackground(true);
    alterBackground();
}

void EmptyMenuContentWidget::alterBackground()
{
    QPalette pal(palette());
    QColor bg(pal.color(QPalette::Window));
    bg.setAlpha(200);
    pal.setColor(QPalette::Window, bg);
    setPalette(pal);
}

void EmptyMenuContentWidget::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::PaletteChange) {
        alterBackground();
    }
    QWidget::changeEvent(e);
}

//! @todo KEXI3 is KexiMenuWidgetStyle needed?
#if 0
KexiMenuWidgetStyle::KexiMenuWidgetStyle(QStyle *style, QObject *parent)
    : KexiUtils::StyleProxy(style, parent)
{
}

KexiMenuWidgetStyle::~KexiMenuWidgetStyle()
{
}

void KexiMenuWidgetStyle::drawControl(ControlElement element, const QStyleOption *option,
                         QPainter *painter, const QWidget *widget = 0) const
{
    if (element == QStyle::CE_MenuItem
        && (option->state & QStyle::State_Selected) && (option->state & QStyle::State_Enabled)
        && parentStyle()->objectName() == QLatin1String("oxygen"))
    {
        // Ugly fix for visual glitch of oxygen; no chance for improvement since
        // we've forked QMenu and oxygen checks for qobject_cast<QMenu*> directly.
        QColor c(option->palette.color(QPalette::Window));
        int h, s, v, a;
        c.getHsv(&h, &s, &v, &a);
        // Why 0.91208791? I knew you're curious. There are some algorithms in Oxygen
        // to make color a bit lighter. They are not in the public API nor they are simple.
        // So the number was computed by me to find the proper value for the color
        // (the accuracy is quite OK).
        // It's also related to the fact that Oxygen's menus have gradient background.
        // A lot of computation happens under the mask...
        c.setHsv(h, s, v * 0.91208791, a);
        painter->fillRect(option->rect.x() + 6, option->rect.y() + 6,
                          option->rect.width() - 12, option->rect.height() - 12,
                          c);
    }
    KexiUtils::StyleProxy::drawControl(element, option, painter, widget);
}
#endif

KexiMainMenu::KexiMainMenu(KexiTabbedToolBar *toolBar, QWidget* parent)
 : QWidget(parent),
    m_toolBar(toolBar), m_initialized(false)
{
    m_content = 0;
    m_selectFirstItem = false;
}

KexiMainMenu::~KexiMainMenu()
{
    delete (QWidget*)m_contentWidget;
}

bool KexiMainMenu::eventFilter(QObject * watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress && watched == m_content && !m_contentWidget) {
        emit contentAreaPressed();
    }
    else if (event->type() == QEvent::KeyPress) {
        QKeyEvent* ke = static_cast<QKeyEvent*>(event);
        if ((ke->key() == Qt::Key_Escape) && ke->modifiers() == Qt::NoModifier) {
            emit hideContentsRequested();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void KexiMainMenu::setContent(QWidget *contentWidget)
{
    if (m_menuWidget && m_persistentlySelectedAction) {
        m_menuWidget->setPersistentlySelectedAction(
            m_persistentlySelectedAction,
            m_persistentlySelectedAction->persistentlySelected());
    }
    /*if (m_menuWidget->persistentlySelectedAction())
        qDebug() << "****" << m_menuWidget->persistentlySelectedAction()->objectName();*/
    KexiFadeWidgetEffect *fadeEffect = 0;

    if (m_contentWidget && contentWidget) {
        fadeEffect = new KexiFadeWidgetEffect(m_content);
    }
    if (m_contentWidget)
        m_contentWidget->deleteLater();
    m_contentWidget = contentWidget;
    if (m_contentWidget) {
        QPalette contentWidgetPalette(m_contentWidget->palette());
        contentWidgetPalette.setBrush(QPalette::Active, QPalette::Window, contentWidgetPalette.brush(QPalette::Active, QPalette::Base));
        contentWidgetPalette.setBrush(QPalette::Inactive, QPalette::Window, contentWidgetPalette.brush(QPalette::Inactive, QPalette::Base));
        contentWidgetPalette.setBrush(QPalette::Disabled, QPalette::Window, contentWidgetPalette.brush(QPalette::Disabled, QPalette::Base));
        contentWidgetPalette.setBrush(QPalette::Active, QPalette::WindowText, contentWidgetPalette.brush(QPalette::Active, QPalette::Text));
        contentWidgetPalette.setBrush(QPalette::Inactive, QPalette::WindowText, contentWidgetPalette.brush(QPalette::Inactive, QPalette::Text));
        contentWidgetPalette.setBrush(QPalette::Disabled, QPalette::WindowText, contentWidgetPalette.brush(QPalette::Disabled, QPalette::Text));
        const QColor highlightDisabled(KexiUtils::blendedColors(
                                     contentWidgetPalette.color(QPalette::Active, QPalette::Highlight),
                                     contentWidgetPalette.color(QPalette::Disabled, QPalette::Window), 1, 2));
        contentWidgetPalette.setBrush(QPalette::Disabled, QPalette::Highlight, highlightDisabled);
        const QColor highlightedTextDisabled(KexiUtils::blendedColors(
                                     contentWidgetPalette.color(QPalette::Active, QPalette::HighlightedText),
                                     contentWidgetPalette.color(QPalette::Disabled, QPalette::WindowText), 1, 2));
        contentWidgetPalette.setBrush(QPalette::Disabled, QPalette::HighlightedText, highlightedTextDisabled);
        m_contentWidget->setPalette(contentWidgetPalette);
        for(QAbstractScrollArea *area : m_contentWidget->findChildren<QAbstractScrollArea*>()) {
            QPalette pal(area->viewport()->palette());
            pal.setBrush(QPalette::Disabled, QPalette::Base, contentWidgetPalette.brush(QPalette::Disabled, QPalette::Base));
            area->viewport()->setPalette(pal);
        }

        m_contentWidget->setAutoFillBackground(true);
        m_contentWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
        m_contentWidget->setContentsMargins(0, 0, 0, 0);
        m_contentLayout->addWidget(m_contentWidget);
        m_contentLayout->setCurrentWidget(m_contentWidget);
        m_contentWidget->setFocus();
        m_contentWidget->installEventFilter(this);
        //connect(m_contentWidget, SIGNAL(destroyed()), this, SLOT(contentWidgetDestroyed()));
    }
    if (fadeEffect) {
        if (m_contentWidget)
            m_contentLayout->update();

        QTimer::singleShot(10, fadeEffect, SLOT(start()));
    }
}

const QWidget *KexiMainMenu::contentWidget() const
{
    return m_contentWidget;
}

void KexiMainMenu::setPersistentlySelectedAction(KexiMenuWidgetAction* action, bool set)
{
    m_persistentlySelectedAction = action;
    m_persistentlySelectedAction->setPersistentlySelected(set);
}

/*
void KexiMainMenu::setActiveAction(QAction* action = 0)
{
    if (!action && !m_menuWidget->actions().isEmpty()) {
        action = actions().first();
    }
    if (action) {
        m_menuWidget->setActiveAction(action);
    }
}
*/

void KexiMainMenu::selectFirstItem()
{
    m_selectFirstItem = true;
}

void KexiMainMenu::showEvent(QShowEvent * event)
{
    if (!m_initialized) {
        m_initialized = true;
        KActionCollection *ac = KexiMainWindowIface::global()->actionCollection();
        QHBoxLayout *hlyr = new QHBoxLayout(this);

        hlyr->setSpacing(0);
        hlyr->setMargin(0);

        m_menuWidget = new KexiMenuWidget;
//! @todo KEXI3 is KexiMenuWidgetStyle needed?
#if 0
        QString styleName(m_menuWidget->style()->objectName());
        if (KDE::version() < KDE_MAKE_VERSION(4, 8, 0) // a fix is apparently needed for glitch in KDE < 4.8
            && styleName == "oxygen")
        {
            KexiMenuWidgetStyle *customStyle = new KexiMenuWidgetStyle(m_menuWidget->style()->objectName(), this);
            m_menuWidget->setStyle(customStyle);
        }
#endif
        m_menuWidget->installEventFilter(this);
        m_menuWidget->setFocusPolicy(Qt::StrongFocus);
        setFocusProxy(m_menuWidget);
        m_menuWidget->setFrame(false);
        m_menuWidget->setAutoFillBackground(true);

        m_menuWidget->addAction(ac->action("project_welcome"));
        m_menuWidget->addAction(ac->action("project_open"));
        m_menuWidget->addAction(ac->action("project_close"));
        m_menuWidget->addSeparator();
        m_menuWidget->addAction(ac->action("project_new"));
        m_menuWidget->addAction(ac->action("project_import_export_send"));
#ifdef KEXI_SHOW_UNIMPLEMENTED
        m_menuWidget->addAction(ac->action("project_properties"));
        //! @todo project information
        m_menuWidget->addAction(ac->action("settings"));
#endif
        m_menuWidget->addSeparator();
        m_menuWidget->addAction(ac->action("quit"));
        hlyr->addWidget(m_menuWidget);

        m_content = new EmptyMenuContentWidget;
        m_content->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
        m_content->installEventFilter(this);
        m_mainContentLayout = new QVBoxLayout;
        hlyr->addLayout(m_mainContentLayout);
        m_contentLayout = new QStackedLayout(m_content);
        m_contentLayout->setStackingMode(QStackedLayout::StackAll);
        m_contentLayout->setContentsMargins(0, 0, 0, 0);
        m_mainContentLayout->addWidget(m_content);
        hlyr->setStretchFactor(m_mainContentLayout, 1);
    }
    QWidget::showEvent(event);
    if (m_selectFirstItem && !m_menuWidget->actions().isEmpty()) {
        QAction* action = m_menuWidget->actions().first();
        m_menuWidget->setActiveAction(action);
        m_selectFirstItem = false;
    }
}

// ---

KexiTabbedToolBar::Private::Private(KexiTabbedToolBar *t)
            : q(t), createWidgetToolBar(0)
#ifdef KEXI_AUTORISE_TABBED_TOOLBAR
            , tabToRaise(-1)
#endif
            , rolledUp(false)
{
#ifdef KEXI_AUTORISE_TABBED_TOOLBAR
    tabRaiseTimer.setSingleShot(true);
    tabRaiseTimer.setInterval(300);
#endif
    tabBarAnimation.setPropertyName("opacity");
    tabBarAnimation.setDuration(500);
    connect(&tabBarAnimation, SIGNAL(finished()), q, SLOT(tabBarAnimationFinished()));
    tabIndex = 0;
    lowestIndex = 2;
}

//! @return true if @a style name is specific regarding tab styling
static bool isSpecificTabStyle(const QString &styleName)
{
    return styleName == "oxygen" || styleName == "qtcurve" || styleName == "gtk+"
           || styleName == "gtk2";
}

KexiTabbedToolBarStyle::KexiTabbedToolBarStyle(const QString &baseStyleName)
  : QProxyStyle(baseStyleName)
{
}

KexiTabbedToolBarStyle::~KexiTabbedToolBarStyle()
{
}

void KexiTabbedToolBarStyle::drawControl(ControlElement element, const QStyleOption *option,
                                         QPainter *painter, const QWidget *widget) const
{
    const QString styleName(baseStyle()->objectName());
    qreal origOpacity = -1.0;
    if (element == CE_TabBarTab) {
        const QStyleOptionTab* opt
            = qstyleoption_cast<const QStyleOptionTab*>(option);
        const QTabBar* tabBar = qobject_cast<const QTabBar*>(widget);
        KexiTabbedToolBar* tbar = tabBar
            ? qobject_cast<KexiTabbedToolBar*>(tabBar->parentWidget()) : 0;
        if (opt && tbar) {
            const int index = tabBar->tabAt(opt->rect.center());
            if (index == KEXITABBEDTOOLBAR_SPACER_TAB_INDEX)
                return;
            bool mouseOver = opt->state & QStyle::State_MouseOver;
            bool unselectedOrMenuVisible
                = !(opt->state & State_Selected) || tbar->mainMenuVisible();
            if (unselectedOrMenuVisible) {
                if (styleName == "bespin") {
                    unselectedOrMenuVisible = false;
                }
            }

            QStyleOptionTab newOpt(*opt);
            const bool specificStyle = isSpecificTabStyle(styleName);
            newOpt.text = (specificStyle ? " " : "")
                    + tabBar->tabText(index)
                    + (specificStyle ? " " : "");
            if (!mouseOver
                && unselectedOrMenuVisible
                && index > 0)
            {
                if (tbar->mainMenuVisible())
                    newOpt.state &= ~QStyle::State_HasFocus;
                QProxyStyle::drawControl(CE_TabBarTabLabel, &newOpt, painter, widget);
                return;
            }
            else if (index == 0) {
                QBrush bg;
                newOpt.state |= State_Selected;
                if (tbar->mainMenuVisible()) {
                    bg = newOpt.palette.brush(QPalette::Active, QPalette::Highlight);
                    if (!specificStyle) {
                        newOpt.palette.setBrush(QPalette::WindowText,
                                                newOpt.palette.brush(QPalette::Active, QPalette::HighlightedText));
                        newOpt.palette.setBrush(QPalette::ButtonText,
                                                newOpt.palette.brush(QPalette::Active, QPalette::HighlightedText));
                    }
                }
                else {
                    if (styleName == "fusion") {
                        bg = newOpt.palette.brush(QPalette::Active, QPalette::Button);
                    } else {
                        bg = Qt::transparent;
                    }
                }
                QFont origFont(painter->font());
                QFont f(origFont);
                f.setBold(true);
                painter->setFont(f);
                newOpt.palette.setBrush(QPalette::Window, bg);
                newOpt.palette.setBrush(QPalette::Button, // needed e.g. for Plastique style
                                        bg);
                QProxyStyle::drawControl(element, &newOpt, painter, widget);
                painter->setFont(origFont);
                if (!mouseOver || tbar->mainMenuVisible() || styleName == "gtk+") {
                    return;
                }
            }
            if (index > 0 || mouseOver) {
                const QPalette::ColorGroup hbGroup =  (styleName == "oxygen")
                        ? QPalette::Active : QPalette::Inactive;
                const QBrush hb(newOpt.palette.brush(hbGroup, QPalette::Highlight));
                newOpt.palette.setBrush(QPalette::Window, hb);
                newOpt.palette.setBrush(QPalette::Button, hb); // needed e.g. for Plastique style
                if (mouseOver && (index != tbar->currentIndex() || tbar->mainMenuVisible())) {
                    // use lower opacity for diplaying hovered tabs
                    origOpacity = painter->opacity();
                    painter->setOpacity(styleName == "qtcurve" ? 0.2 : 0.3);
                    newOpt.state |= State_Selected;
                }
                else {
                    if (!specificStyle) {
                        newOpt.palette.setBrush(QPalette::WindowText,
                                                newOpt.palette.brush(QPalette::Inactive, QPalette::HighlightedText));
                        newOpt.palette.setBrush(QPalette::ButtonText,
                                                newOpt.palette.brush(QPalette::Inactive, QPalette::HighlightedText));
                    }
                }
                if (index == tbar->currentIndex() && styleName == "qtcurve") {
                    origOpacity = painter->opacity();
                    painter->setOpacity(0.5);
                }
                (newOpt.state |= State_Active) ^= State_Active;
                QProxyStyle::drawControl(element, &newOpt, painter, widget);
                if (origOpacity != -1.0) {
                    // restore opacity and draw labels using full this opacity
                    painter->setOpacity(origOpacity);
                    if (index > 0) {
                        QProxyStyle::drawControl(CE_TabBarTabLabel, &newOpt, painter, widget);
                    }
                }
                return;
            }
        }
    }
    else if (element == CE_ToolBar) {
        return;
    }
    QProxyStyle::drawControl(element, option, painter, widget);
}

void KexiTabbedToolBarStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                                           QPainter *painter, const QWidget *widget) const
{
    const QString styleName(baseStyle()->objectName());
    if (element == PE_FrameTabWidget) {
        return;
    }
    if (element == PE_FrameTabBarBase) {
        const QTabBar* tabBar = qobject_cast<const QTabBar*>(widget);
        KexiTabbedToolBar* tbar = tabBar
            ? qobject_cast<KexiTabbedToolBar*>(tabBar->parentWidget()) : 0;
        if (tbar && tbar->mainMenuVisible() && styleName != "bespin") {
            return;
        }
    }
    if (element == QStyle::PE_PanelToolBar || element == QStyle::PE_FrameMenu) {
        return;
    }
    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

int KexiTabbedToolBarStyle::pixelMetric(PixelMetric metric, const QStyleOption* option,
                                        const QWidget* widget) const
{
    if (metric == QStyle::PM_SmallIconSize)
        return KIconLoader::SizeMedium;
    return QProxyStyle::pixelMetric(metric, option, widget);
}

// ---

KexiTabbedToolBarTabBar::KexiTabbedToolBarTabBar(QWidget *parent)
    : QTabBar(parent)
{
    setObjectName("tabbar");
    customStyle = new KexiTabbedToolBarStyle(style()->objectName());
    customStyle->setParent(this);
    setStyle(customStyle);
    installEventFilter(parent);
    QWidget *mainWindow = KexiMainWindowIface::global()->thisWidget();
    mainWindow->installEventFilter(parent);
    setAttribute(Qt::WA_Hover, true);
}

QSize KexiTabbedToolBarTabBar::originalTabSizeHint(int index) const
{
    return QTabBar::tabSizeHint(index);
}

QSize KexiTabbedToolBarTabBar::tabSizeHint(int index) const
{
    QSize s = QTabBar::tabSizeHint(index);
    QStyleOptionTab ot;
    ot.initFrom(this);
    QFont f(font());
    f.setBold(true);
    ot.text = (isSpecificTabStyle(customStyle->baseStyle()->objectName()) ? "  " : "") + tabText(index);
    ot.fontMetrics = QFontMetrics(f);
    int w = customStyle->pixelMetric(QStyle::PM_TabBarTabHSpace, &ot, this);
    if (w <= 0) { // needed e.g. for oxygen
        w = fontMetrics().width("   ");
    }
    if (index == 0) {
        s.setWidth(QFontMetrics(f).width(ot.text) + w * 2);
        return s;
    }
    else if (index == KEXITABBEDTOOLBAR_SPACER_TAB_INDEX) {
        // fix width of the spacer tab
        s.setWidth(w);
    }
    return s;
}

void KexiTabbedToolBar::Private::toggleMainMenu()
{
    if (q->mainMenuVisible())
        hideMainMenu();
    else
        showMainMenu();
}

void KexiTabbedToolBar::Private::showMainMenu(const char* actionName)
{
    QWidget *mainWindow = KexiMainWindowIface::global()->thisWidget();
    if (!mainMenu) {
        mainMenu = new KexiMainMenu(q, mainWindow);
        connect(mainMenu, SIGNAL(contentAreaPressed()), this, SLOT(hideMainMenu()));
        connect(mainMenu, SIGNAL(hideContentsRequested()), this, SLOT(hideContentsOrMainMenu()));
    }
    updateMainMenuGeometry();
    if (actionName) {
        q->selectMainMenuItem(actionName);
    }
    else {
        mainMenu->selectFirstItem();
    }
    mainMenu->show();
    mainMenu->setFocus();
    q->update(); // tab bar could have a line that should be repainted
}

void KexiTabbedToolBar::Private::updateMainMenuGeometry()
{
    if (!mainMenu)
        return;
    QWidget *mainWindow = KexiMainWindowIface::global()->thisWidget();
    KexiTabbedToolBarTabBar *tabBar = static_cast<KexiTabbedToolBarTabBar*>(q->tabBar());
    QPoint pos = q->mapToGlobal(QPoint(0, tabBar->originalTabSizeHint(0).height() - 1));
//     qDebug() << "1." << pos;
    pos = mainWindow->mapFromGlobal(pos);
//     qDebug() << "2." << pos;
//     qDebug() << "3." << q->pos();

    QStyleOptionTab ot;
    ot.initFrom(tabBar);
    int overlap = tabBar->style()->pixelMetric(QStyle::PM_TabBarBaseOverlap, &ot, tabBar)
                  - tabBar->style()->pixelMetric(QStyle::PM_TabBarBaseHeight, &ot, tabBar);
//     qDebug() << "4. overlap=" << overlap;

    mainMenu->setGeometry(0, pos.y() - overlap /*- q->y()*/,
                          mainWindow->width(),
                          mainWindow->height() - pos.y() + overlap /*+ q->y()*/);
}

void KexiTabbedToolBar::Private::initSearchLineEdit()
{
    //! @todo use KexiConfig
    KConfigGroup mainWindowGroup(KSharedConfig::openConfig()->group("MainWindow"));
    const bool enabled = mainWindowGroup.readEntry("GlobalSearchBoxEnabled", true);
    if (enabled && !searchLineEdit) {
        searchLineEdit = new KexiSearchLineEdit;
        kexiTester() << KexiTestObject(searchLineEdit, "globalSearch.lineEdit");
        searchLineEdit->installEventFilter(q);
        helpLayer->addWidget(searchLineEdit);
    } else if (!enabled && searchLineEdit) {
        helpLayer->removeWidget(searchLineEdit);
        delete searchLineEdit;
        searchLineEdit = nullptr;
    }
}

void KexiTabbedToolBar::activateSearchLineEdit()
{
    if (!d->searchLineEdit) {
        return;
    }
    d->searchLineEdit->selectAll();
    d->searchLineEdit->setFocus();
}

void KexiTabbedToolBar::Private::hideMainMenu()
{
    if (!mainMenu || !mainMenu->isVisible())
        return;
    mainMenu->hide();
    mainMenu->setContent(0);
    q->update();  // tab bar could have a line that should be repainted
}

void KexiTabbedToolBar::Private::hideContentsOrMainMenu()
{
    if (!mainMenu || !mainMenu->isVisible())
        return;
    if (mainMenu->contentWidget()) {
        mainMenu->setContent(0);
    }
    else {
        hideMainMenu();
    }
}

KToolBar *KexiTabbedToolBar::Private::createToolBar(const char *name, const QString& caption)
{
    KToolBar *tbar = new KToolBar(q, true /*main toolbar*/, false /*read config*/);
    tbar->setIconDimensions(22); //!< @todo make configurable or system-dependent?
    // needed e.g. for Windows style to remove the toolbar's frame
    tbar->setStyle(customTabBar->customStyle);
    toolbarsForName.insert(name, tbar);
    tbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    tbar->setObjectName(name);
    toolbarsCaptionForName.insert(name, caption);
    tabIndex = q->addTab(tbar, caption);
    toolbarsVisibleForIndex.append(true);
    toolbarsIndexForName.insert(name, tabIndex);
    return tbar;
}

KexiTabbedToolBar::KexiTabbedToolBar(QWidget *parent)
        : QTabWidget(parent)
        , d(new Private(this))
{
    d->customTabBar = new KexiTabbedToolBarTabBar(this);
    setTabBar(d->customTabBar);
    setStyle(d->customTabBar->customStyle);

    // from ktabwidget.cpp
    //! @todo QTabWidget::setTabBar() should be added with this code
    //! @todo KEXI3 Are these tabBar() connections useful to port?
#if 0
    connect(tabBar(), SIGNAL(contextMenu(int,QPoint)), SLOT(contextMenu(int,QPoint&)));
    connect(tabBar(), SIGNAL(tabDoubleClicked(int)), SLOT(mouseDoubleClick(int)));
    connect(tabBar(), SIGNAL(newTabRequest()), this, SIGNAL(mouseDoubleClick())); // #185487
    connect(tabBar(), SIGNAL(mouseMiddleClick(int)), SLOT(mouseMiddleClick(int)));
    connect(tabBar(), SIGNAL(initiateDrag( int )), SLOT(initiateDrag( int )));
    connect(tabBar(), SIGNAL(testCanDecode(QDragMoveEvent*,bool*)), SIGNAL(testCanDecode(QDragMoveEvent*,bool*)));
    connect(tabBar(), SIGNAL(receivedDropEvent(int,QDropEvent*)), SLOT(receivedDropEvent(int,QDropEvent*)));
    connect(tabBar(), SIGNAL(moveTab(int,int)), SLOT(moveTab(int,int)));
    connect(tabBar(), SIGNAL(tabCloseRequested(int)), SLOT(closeRequest(int)));
#endif

    setMouseTracking(true); // for mouseMoveEvent()
    setWhatsThis(xi18n("Task-oriented toolbar. Groups commands using tabs."));
#ifdef KEXI_AUTORISE_TABBED_TOOLBAR
    connect(&d->tabRaiseTimer, SIGNAL(timeout()), this, SLOT(slotDelayedTabRaise()));
#endif
    connect(tabBar(), SIGNAL(tabBarDoubleClicked(int)), this, SLOT(slotTabDoubleClicked(int)));

    d->ac = KexiMainWindowIface::global()->actionCollection();
    const bool userMode = KexiMainWindowIface::global()->userMode();
    KToolBar *tbar;

    slotSettingsChanged(0);//KGlobalSettings::FontChanged
    //! @todo KEXI3 port from KGlobalSettings::Private::_k_slotNotifyChange:
    //! connect(KGlobalSettings::self(), SIGNAL(settingsChanged(int)), this, SLOT(slotSettingsChanged(int)));

    // help area
    QWidget *helpWidget = new QWidget(this);
    helpWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    d->helpLayer = new QHBoxLayout(helpWidget);
    d->helpLayer->setContentsMargins(0, 0, 0, 0);
    setCornerWidget(helpWidget, Qt::TopRightCorner);
    d->initSearchLineEdit();

    // needed e.g. for Windows style to remove the toolbar's frame
    QWidget *dummyWidgetForMainMenu = new QWidget(this);
    dummyWidgetForMainMenu->setObjectName("kexi");
    addTab(dummyWidgetForMainMenu, KAboutData::applicationData().displayName());
    d->toolbarsVisibleForIndex.append(true);
    addTab(new QWidget(this), QString()); // dummy for spacer
    d->toolbarsVisibleForIndex.append(true);

    if (!userMode) {
        d->createWidgetToolBar = d->createToolBar("create", xi18n("Create"));
    }

//! @todo move to form plugin
    tbar = d->createToolBar("form", xi18n("Form Design"));

//! @todo move to report plugin
    tbar = d->createToolBar("report", xi18n("Report Design"));
    Q_UNUSED(tbar)

    connect(this, SIGNAL(currentChanged(int)), this, SLOT(slotCurrentChanged(int)));
    setCurrentWidget(widget(KEXITABBEDTOOLBAR_SPACER_TAB_INDEX + 1)); // the default
    setFocusPolicy(Qt::NoFocus);
}

void KexiTabbedToolBar::Private::setCurrentTab(const QString& name)
{
    q->setCurrentWidget(q->toolBar(name));
}

void KexiTabbedToolBar::Private::hideTab(const QString& name)
{
    q->removeTab(q->indexOf(toolbarsForName.value(name)));
    toolbarsVisibleForIndex[toolbarsIndexForName.value(name)] = false;
}

bool KexiTabbedToolBar::Private::isTabVisible(const QString& name) const
{
    return q->indexOf(toolbarsForName.value(name)) != -1
           && toolbarsVisibleForIndex[toolbarsIndexForName.value(name)];
}

#ifndef NDEBUG
void KexiTabbedToolBar::Private::debugToolbars() const
{
    qDebug() << "QHash<QString, KToolBar*> toolbarsForName:";
    for (QHash<QString, KToolBar*>::ConstIterator it(toolbarsForName.constBegin());
         it!=toolbarsForName.constEnd(); ++it)
    {
        qDebug() << it.key() << "->" << it.value();
    }
    qDebug() << "QHash<QString, int> toolbarsIndexForName:";
    for (QHash<QString, int>::ConstIterator it(toolbarsIndexForName.constBegin());
         it!=toolbarsIndexForName.constEnd(); ++it)
    {
        qDebug() << it.key() << "->" << it.value();
    }
    qDebug() << "QHash<QString, QString> toolbarsCaptionForName:";
    for (QHash<QString, QString>::ConstIterator it(toolbarsCaptionForName.constBegin());
         it!=toolbarsCaptionForName.constEnd(); ++it)
    {
        qDebug() << it.key() << "->" << it.value();
    }
    qDebug() << "QVector<bool> toolbarsVisibleForIndex:";
    for (int i = 0; i < toolbarsVisibleForIndex.size(); i++) {
        qDebug() << i << "->" << toolbarsVisibleForIndex[i];
    }
}
#endif

void KexiTabbedToolBar::Private::showTab(const QString& name)
{
//    qDebug() << "name:" << name;
//    qDebug() << "toolbarsForName.value(name):" << toolbarsForName.value(name);
//    qDebug() << "toolbarsIndexForName.value(name):" << toolbarsIndexForName.value(name);
//    qDebug() << "q->indexOf(toolbarsForName.value(name))" << q->indexOf(toolbarsForName.value(name));
#ifndef NDEBUG
    //debugToolbars();
#endif
    if (q->indexOf(toolbarsForName.value(name)) == -1) {
        int h = 0;
        // count h = invisible tabs before this
        for (int i = lowestIndex; i < toolbarsIndexForName.value(name); i++) {
            if (!toolbarsVisibleForIndex.at(i))
                h++;
        }
        q->insertTab(toolbarsIndexForName.value(name) - h,
                     toolbarsForName.value(name), toolbarsCaptionForName.value(name));
        toolbarsVisibleForIndex[toolbarsIndexForName.value(name)] = true;
    }
}

// ---

KexiTabbedToolBar::~KexiTabbedToolBar()
{
    delete d;
}

bool KexiTabbedToolBar::mainMenuVisible() const
{
    return d->mainMenu && d->mainMenu->isVisible();
}

QRect KexiTabbedToolBar::tabRect(int index) const
{
    return tabBar()->tabRect(index);
}

void KexiTabbedToolBar::slotSettingsChanged(int category)
{
    Q_UNUSED(category);
    //! @todo if (category == KGlobalSettings::FontChanged) {
        //! @todo KEXI3 KGlobalSettings::menuFont() not available (using QFontDatabase::systemFont(QFontDatabase::GeneralFont) for now)
        //!       https://community.kde.org/Frameworks/Porting_Notes#Global_Settings
        setFont(QFontDatabase::systemFont(QFontDatabase::GeneralFont)); // the toolbar acts like a menu
    //}
}

KToolBar* KexiTabbedToolBar::createWidgetToolBar() const
{
    return d->createWidgetToolBar;
}

void KexiTabbedToolBar::mouseMoveEvent(QMouseEvent* event)
{
#ifdef KEXI_AUTORISE_TABBED_TOOLBAR
    QPoint p = event->pos();
    int tab = tabBar()->tabAt(p);
    if (d->tabToRaise != -1 && (tab == -1 || tab == currentIndex())) {
        d->tabRaiseTimer.stop();
        d->tabToRaise = -1;
    } else if (d->tabToRaise != tab) {
        d->tabRaiseTimer.start();

        d->tabToRaise = tab;
    }
#endif
    QTabWidget::mouseMoveEvent(event);
}

void KexiTabbedToolBar::leaveEvent(QEvent* event)
{
#ifdef KEXI_AUTORISE_TABBED_TOOLBAR
    d->tabRaiseTimer.stop();
    d->tabToRaise = -1;
#endif
    QTabWidget::leaveEvent(event);
}

bool KexiTabbedToolBar::eventFilter(QObject* watched, QEvent* event)
{
    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        QWidget *mainWin = KexiMainWindowIface::global()->thisWidget();
        // qDebug() << "MouseButtonPress: watched:" << watched << "window()->focusWidget():" << window()->focusWidget();
        if (d->searchLineEdit && watched == d->searchLineEdit) {
            activateSearchLineEdit(); // custom setFocus() for search box, so it's possible to focus
                                      // back on Escape key press
            return false;
        }
        else if (watched == tabBar()) {
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            QPoint p = me->pos();
            KexiTabbedToolBarTabBar *tb = static_cast<KexiTabbedToolBarTabBar*>(tabBar());
            int index = tb->tabAt(p);
            if (index == 0) {
                d->toggleMainMenu();
                return true;
            }
            d->hideMainMenu();
            if (index == KEXITABBEDTOOLBAR_SPACER_TAB_INDEX) {
                return true;
            }
        }
        else if (watched == mainWin && d->mainMenu) {
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            if (!QRect(d->mainMenu->mapToGlobal(QPoint(0,0)), d->mainMenu->size())
                    .contains(mainWin->mapToGlobal(me->pos())))
            {
                // hide if clicked outside of the menu
                d->hideMainMenu();
            }
        }
        }
        break;
    case QEvent::KeyPress: {
        QKeyEvent* ke = static_cast<QKeyEvent*>(event);
//         qDebug() << "**********" << QString::number(ke->key(), 16)
//                  << QKeySequence::mnemonic(tabText(0))[0];
        if (QKeySequence::mnemonic(tabText(0)) == QKeySequence(ke->key())) {
//             qDebug() << "eat the &File accel";
            if (!d->mainMenu || !d->mainMenu->isVisible()) {
                d->showMainMenu();
            }
            /*this could be unexpected:
            else if (d->mainMenu && d->mainMenu->isVisible()) {
                d->hideMainMenu();
            }*/
            return true;
        }
        if (d->mainMenu && d->mainMenu->isVisible() && (ke->key() == Qt::Key_Escape) && ke->modifiers() == Qt::NoModifier) {
            d->hideContentsOrMainMenu();
            return true;
        }
        break;
    }
    case QEvent::Resize:
        if (watched == KexiMainWindowIface::global()->thisWidget()) {
            d->updateMainMenuGeometry();
        }
        break;
    case QEvent::Shortcut: {
        QShortcutEvent *se = static_cast<QShortcutEvent*>(event);
        if (watched == tabBar() && QKeySequence::mnemonic(tabText(0)) == se->key()) {
//             qDebug() << "eat the &File accel";
            if (!d->mainMenu || !d->mainMenu->isVisible()) {
                d->showMainMenu();
                return true;
            }
        }
        break;
    }
    default:;
    }
    return QTabWidget::eventFilter(watched, event);
}

void KexiTabbedToolBar::slotCurrentChanged(int index)
{
    if (index == indexOf(d->createWidgetToolBar) && index != -1) {
        if (d->createWidgetToolBar->actions().isEmpty()) {
            QTimer::singleShot(10, this, SLOT(setupCreateWidgetToolbar()));
        }
    }
    if (d->rolledUp) { // switching the tab rolls down
        slotTabDoubleClicked(index);
    }
    if (index == 0) { // main menu
        d->showMainMenu();
    }
    else {
        d->hideMainMenu();
    }
}

void KexiTabbedToolBar::slotTabDoubleClicked(int index)
{
    if (index <= 0) {
        return; // main item does not count here
    }
    d->rolledUp = !d->rolledUp;
    d->tabBarAnimation.stop();
    QWidget *w = widget(index);
    if (!w) {
        return;
    }
    w->setGraphicsEffect(&d->tabBarOpacityEffect);
    if (d->rolledUp) {
        d->tabBarOpacityEffect.setOpacity(1.0);
        d->tabBarAnimation.setTargetObject(&d->tabBarOpacityEffect);
        d->tabBarAnimation.setStartValue(1.0);
        d->tabBarAnimation.setEndValue(0.0);
        d->tabBarAnimation.start();
    }
    else { // roll down
        d->tabBarOpacityEffect.setOpacity(0.0);
        setMaximumHeight(QWIDGETSIZE_MAX);
        widget(d->rolledUpIndex)->show();
        widget(d->rolledUpIndex)->setMaximumHeight(QWIDGETSIZE_MAX);
        w->setMaximumHeight(QWIDGETSIZE_MAX);
        w->show();
        d->tabBarAnimation.setTargetObject(&d->tabBarOpacityEffect);
        d->tabBarAnimation.setStartValue(0.0);
        d->tabBarAnimation.setEndValue(1.0);
        d->tabBarAnimation.start();
    }
}

void KexiTabbedToolBar::tabBarAnimationFinished()
{
    if (d->rolledUp) {
        // hide and collapse the area
        widget(currentIndex())->hide();
        KexiTabbedToolBarTabBar *tb = static_cast<KexiTabbedToolBarTabBar*>(tabBar());
        setFixedHeight(tb->tabSizeHint(currentIndex()).height());
        widget(currentIndex())->setFixedHeight(0);
        d->rolledUpIndex = currentIndex();
    }
}

void KexiTabbedToolBar::setupCreateWidgetToolbar()
{
    if (!d->createWidgetToolBar->actions().isEmpty())
        return;
//! @todo separate core object types from custom....
    KexiPart::PartInfoList *plist = Kexi::partManager().infoList(); //this list is properly sorted
    if (plist) {
        foreach(KexiPart::Info *info, *plist) {
            QAction* a = info->newObjectAction();
            if (a) {
                d->createWidgetToolBar->addAction(a);
            } else {
                //! @todo err
            }
        }
    }
}

void KexiTabbedToolBar::slotDelayedTabRaise()
{
#ifdef KEXI_AUTORISE_TABBED_TOOLBAR
    QPoint p = mapFromGlobal(QCursor::pos()); // make sure cursor is still over the tab
    int tab = tabBar()->tabAt(p);
    if (tab != d->tabToRaise) {
        d->tabToRaise = -1;
    } else if (d->tabToRaise != -1) {
        setCurrentIndex(d->tabToRaise);
        d->tabToRaise = -1;
    }
#endif
}

KToolBar *KexiTabbedToolBar::toolBar(const QString& name) const
{
    return d->toolbarsForName[name];
}

void KexiTabbedToolBar::addAction(KToolBar *tbar, const char* actionName)
{
    QAction *a = d->ac->action(actionName);
    if (a)
        tbar->addAction(a);
}

void KexiTabbedToolBar::addAction(const QString& toolBarName, QAction *action)
{
    if (!action)
        return;
    KToolBar *tbar = d->toolbarsForName[toolBarName];
    if (!tbar)
        return;
    tbar->addAction(action);
}

void KexiTabbedToolBar::addSeparatorAndAction(KToolBar *tbar, const char* actionName)
{
    QAction *a = d->ac->action(actionName);
    if (a) {
        tbar->addSeparator();
        tbar->addAction(a);
    }
}

void KexiTabbedToolBar::appendWidgetToToolbar(const QString& name, QWidget* widget)
{
    KToolBar *tbar = d->toolbarsForName[name];
    if (!tbar) {
        return;
    }
    QAction *action = tbar->addWidget(widget);
    d->extraActions.insert(widget, action);
}

void KexiTabbedToolBar::setWidgetVisibleInToolbar(QWidget* widget, bool visible)
{
    QAction *action = d->extraActions[widget];
    if (!action) {
        return;
    }
    action->setVisible(visible);
}

void KexiTabbedToolBar::showMainMenu(const char* actionName)
{
    d->showMainMenu(actionName);
}

void KexiTabbedToolBar::hideMainMenu()
{
    d->hideMainMenu();
}

void KexiTabbedToolBar::toggleMainMenu()
{
    d->toggleMainMenu();
}

void KexiTabbedToolBar::setMainMenuContent(QWidget *w)
{
    d->mainMenu->setContent(w);
}

const QWidget* KexiTabbedToolBar::mainMenuContent()
{
    return d->mainMenu->contentWidget();
}

void KexiTabbedToolBar::selectMainMenuItem(const char *actionName)
{
    if (actionName) {
        KActionCollection *ac = KexiMainWindowIface::global()->actionCollection();
        KexiMenuWidgetAction *a = qobject_cast<KexiMenuWidgetAction*>(ac->action(actionName));
        if (a) {
            d->mainMenu->setPersistentlySelectedAction(a, true);
        }
    }
}

void KexiTabbedToolBar::addSearchableModel(KexiSearchableModel *model)
{
    if (!d->searchLineEdit) {
        return;
    }
    d->searchLineEdit->addSearchableModel(model);
}

void KexiTabbedToolBar::removeSearchableModel(KexiSearchableModel *model)
{
    if (!d->searchLineEdit) {
        return;
    }
    d->searchLineEdit->removeSearchableModel(model);
}

KToolBar* KexiTabbedToolBar::createToolBar(const char* name, const QString& caption)
{
    return d->createToolBar(name, caption);
}

void KexiTabbedToolBar::setCurrentTab(const QString& name)
{
    //qDebug() << name;
    d->setCurrentTab(name);
}

void KexiTabbedToolBar::setCurrentTab(int index)
{
    setCurrentIndex(d->lowestIndex + index);
}

void KexiTabbedToolBar::hideTab(const QString& name)
{
    //qDebug() << name;
    d->hideTab(name);
}

void KexiTabbedToolBar::showTab(const QString& name)
{
    //qDebug() << name;
    d->showTab(name);
}

bool KexiTabbedToolBar::isTabVisible(const QString& name) const
{
    return d->isTabVisible(name);
}

bool KexiTabbedToolBar::isRolledUp()
{
    return d->rolledUp;
}

void KexiTabbedToolBar::toggleRollDown()
{
    slotTabDoubleClicked(-1);//use -1 just to rolldown/up the tabbar
}

//------------------------------------------

KexiMainWindow::Private::Private(KexiMainWindow* w)
    : wnd(w)
{
    objectViewWidget = 0;
    actionCollection = new KActionCollection(w);
    KexiProjectData *pdata = KexiStartupHandler::global()->projectData();
    userMode = KexiStartupHandler::global()->forcedUserMode() /* <-- simply forced the user mode */
               /* project has 'user mode' set as default and not 'design mode' override is found: */
               || (pdata && pdata->userMode() && !KexiStartupHandler::global()->forcedDesignMode());
    isProjectNavigatorVisible = KexiStartupHandler::global()->isProjectNavigatorVisible();
    isMainMenuVisible = KexiStartupHandler::global()->isMainMenuVisible();
    prj = 0;
    config = KSharedConfig::openConfig();
    nameDialog = 0;
    m_findDialog = 0;
    focus_before_popup = 0;
    action_show_nav = 0;
    action_show_propeditor = 0;
    action_activate_nav = 0;
    action_activate_propeditor = 0;
    forceWindowClosing = false;
    insideCloseWindow = false;
#ifndef KEXI_NO_PENDING_DIALOGS
    actionToExecuteWhenPendingJobsAreFinished = NoAction;
#endif
    propEditorDockSeparatorPos = -1;
    navDockSeparatorPos = -1;
    wasAutoOpen = false;
    windowExistedBeforeCloseProject = false;
#ifndef KEXI_SHOW_UNIMPLEMENTED
    dummy_action = new KActionMenu(QString(), wnd);
#endif
    forceShowProjectNavigatorOnCreation = false;
    forceHideProjectNavigatorOnCreation = false;
    navWasVisibleBeforeProjectClosing = false;
    saveSettingsForShowProjectNavigator = true;
    propertyEditorCollapsed = false;
    enable_slotPropertyEditorVisibilityChanged = true;
    migrateManager = 0;
}

KexiMainWindow::Private::~Private()
{
    qDeleteAll(m_openedCustomObjectsForItem);
}

void KexiMainWindow::Private::insertWindow(KexiWindow *window)
{
//! @todo (threads)  QMutexLocker dialogsLocker( &dialogsMutex );
    windows.insert(window->id(), window);
#ifndef KEXI_NO_PENDING_DIALOGS
    pendingWindows.remove(window->id());
#endif
}

bool KexiMainWindow::Private::windowContainerExistsFor(int identifier) const
{
    return windowContainers.contains(identifier);
}

void KexiMainWindow::Private::setWindowContainerExistsFor(int identifier, bool set)
{
    if (set) {
        windowContainers.insert(identifier);
    }
    else {
        windowContainers.remove(identifier);
    }
}

void KexiMainWindow::Private::updateWindowId(KexiWindow *window, int oldItemID)
{
//! @todo (threads)  QMutexLocker dialogsLocker( &dialogsMutex );
    windows.remove(oldItemID);
#ifndef KEXI_NO_PENDING_DIALOGS
    pendingWindows.remove(oldItemID);
#endif
    windows.insert(window->id(), window);
}

void KexiMainWindow::Private::removeWindow(int identifier)
{
//! @todo (threads)  QMutexLocker dialogsLocker( &dialogsMutex );
    windows.remove(identifier);
}

int KexiMainWindow::Private::openedWindowsCount()
{
//! @todo (threads)  QMutexLocker dialogsLocker( &dialogsMutex );
    return windows.count();
}

//! Used in KexiMainWindowe::closeProject()
void KexiMainWindow::Private::clearWindows()
{
//! @todo (threads)  QMutexLocker dialogsLocker( &dialogsMutex );
    windows.clear();
#ifndef KEXI_NO_PENDING_DIALOGS
    pendingWindows.clear();
#endif
}

void KexiMainWindow::Private::showStartProcessMsg(const QStringList& args)
{
    wnd->showErrorMessage(xi18nc("@info", "Could not start <application>%1</application> application.",
                                 QApplication::applicationDisplayName()),
                          xi18nc("@info",
                                 "Command <command>%1</command> failed.", args.join(" ")));
}

void KexiMainWindow::Private::updatePropEditorVisibility(Kexi::ViewMode viewMode, KexiPart::Info *info)
{
    if (!objectViewWidget || !objectViewWidget->propertyPane()) {
        return;
    }
    KexiWindow *currentWindow = wnd->currentWindow();
    if (!info && currentWindow) {
        info = currentWindow->part()->info();
    }
    const bool visible = (viewMode == Kexi::DesignViewMode)
        && ((currentWindow && currentWindow->propertySet()) || (info && info->isPropertyEditorAlwaysVisibleInDesignMode()));
    //qDebug() << "visible ==" << visible;
    enable_slotPropertyEditorVisibilityChanged = false;
    bool set;
    if (visible && propertyEditorCollapsed) { // used when we're switching back to a window with propeditor available but collapsed
        set = !visible;
    }
    else {
        set = visible;
    }
    objectViewWidget->propertyPane()->setVisible(set);
    action_show_propeditor->setChecked(set);
    action_show_propeditor->setEnabled(set);
    objectViewWidget->updateSidebarWidths();
    enable_slotPropertyEditorVisibilityChanged = true;
}

/*
void KexiMainWindow::Private::setTabBarVisible(KMultiTabBar::KMultiTabBarPosition position, int id,
                                               KexiDockWidget *dockWidget, bool visible)
{
    KMultiTabBar *mtbar = multiTabBars.value(position);
    if (!mtbar) {
        return;
    }
    if (!visible) {
        mtbar->removeTab(id);
    }
    else if (!mtbar->tab(id)) {
        mtbar->appendTab(koIcon("document-properties"), id, dockWidget->tabText);
        KMultiTabBarTab *tab = mtbar->tab(id);
        QObject::connect(tab, SIGNAL(clicked(int)),
                         wnd, SLOT(slotMultiTabBarTabClicked(int)),
                         Qt::UniqueConnection);
    }
}

void KexiMainWindow::Private::setPropertyEditorTabBarVisible(bool visible)
{
    setTabBarVisible(KMultiTabBar::Right, PROPERTY_EDITOR_TABBAR_ID,
                     objectViewWidget->propertyEditorTabWidget(), visible);
}*/

QObject *KexiMainWindow::Private::openedCustomObjectsForItem(KexiPart::Item* item, const char* name)
{
    if (!item || !name) {
        qWarning() << "!item || !name";
        return 0;
    }
    QByteArray key(QByteArray::number(item->identifier()) + name);
    return m_openedCustomObjectsForItem.value(key);
}

void KexiMainWindow::Private::addOpenedCustomObjectForItem(KexiPart::Item* item, QObject* object, const char* name)
{
    QByteArray key(QByteArray::number(item->identifier()) + name);
    m_openedCustomObjectsForItem.insert(key, object);
}

KexiFindDialog *KexiMainWindow::Private::findDialog()
{
    if (!m_findDialog) {
        m_findDialog = new KexiFindDialog(wnd);
        m_findDialog->setActions(action_edit_findnext, action_edit_findprev,
                                 action_edit_replace, action_edit_replace_all);
    }
    return m_findDialog;
}

void KexiMainWindow::Private::updateFindDialogContents(bool createIfDoesNotExist)
{
    if (!wnd->currentWindow())
        return;
    if (!createIfDoesNotExist && (!m_findDialog || !m_findDialog->isVisible()))
        return;
    KexiSearchAndReplaceViewInterface* iface = currentViewSupportingSearchAndReplaceInterface();
    if (!iface) {
        if (m_findDialog) {
            m_findDialog->setButtonsEnabled(false);
            m_findDialog->setLookInColumnList(QStringList(), QStringList());
        }
        return;
    }
//! @todo use ->caption() here, depending on global settings related to displaying captions
    findDialog()->setObjectNameForCaption(wnd->currentWindow()->partItem()->name());

    QStringList columnNames;
    QStringList columnCaptions;
    QString currentColumnName; // for 'look in'
    if (!iface->setupFindAndReplace(columnNames, columnCaptions, currentColumnName)) {
        m_findDialog->setButtonsEnabled(false);
        m_findDialog->setLookInColumnList(QStringList(), QStringList());
        return;
    }
    m_findDialog->setButtonsEnabled(true);
    const QString prevColumnName(m_findDialog->currentLookInColumnName());
    m_findDialog->setLookInColumnList(columnNames, columnCaptions);
    m_findDialog->setCurrentLookInColumnName(prevColumnName);
}

KexiView *KexiMainWindow::Private::currentViewSupportingAction(const char* actionName) const
{
    if (!wnd->currentWindow())
        return 0;
    KexiView *view = wnd->currentWindow()->selectedView();
    if (!view)
        return 0;
    QAction *action = view->sharedAction(actionName);
    if (!action || !action->isEnabled())
        return 0;
    return view;
}

KexiSearchAndReplaceViewInterface* KexiMainWindow::Private::currentViewSupportingSearchAndReplaceInterface() const
{
    if (!wnd->currentWindow())
        return 0;
    KexiView *view = wnd->currentWindow()->selectedView();
    if (!view)
        return 0;
    return dynamic_cast<KexiSearchAndReplaceViewInterface*>(view);
}

tristate KexiMainWindow::Private::showProjectMigrationWizard(
    const QString& mimeType, const QString& databaseName, const KDbConnectionData *cdata)
{
    //pass arguments
    QMap<QString, QString> args;
    args.insert("mimeType", mimeType);
    args.insert("databaseName", databaseName);
    if (cdata) { //pass KDbConnectionData serialized as a string...
        QString str;
        KDbUtils::serializeMap(cdata->toMap(), &str);
        args.insert("connectionData", str);
    }

    QDialog *dlg = KexiInternalPart::createModalDialogInstance("org.kexi-project.migration", "migration", wnd, 0, &args);
    if (!dlg)
        return false; //error msg has been shown by KexiInternalPart

    const int result = dlg->exec();
    delete dlg;
    if (result != QDialog::Accepted)
        return cancelled;

    //open imported project in a new Kexi instance
    QString destinationDatabaseName(args["destinationDatabaseName"]);
    QString fileName, destinationConnectionShortcut;
    if (!destinationDatabaseName.isEmpty()) {
        if (args.contains("destinationConnectionShortcut")) {
            // server-based
            destinationConnectionShortcut = args["destinationConnectionShortcut"];
        } else {
            // file-based
            fileName = destinationDatabaseName;
            destinationDatabaseName.clear();
        }
        tristate res = wnd->openProject(fileName, destinationConnectionShortcut,
                                        destinationDatabaseName);
        wnd->raise();
        return res;
    }
    return true;
}

KPropertyEditorView* KexiMainWindow::Private::propertyEditor() const
{
    return (objectViewWidget && objectViewWidget->propertyPane() && objectViewWidget->propertyPane()->editor())
            ? objectViewWidget->propertyPane()->editor() : 0;
}

void KexiMainWindow::Private::setProjectNavigatorVisible(bool set, ShowMode mode)
{
    if (objectViewWidget && objectViewWidget->projectNavigator()) {
        if (mode == ShowAnimated) {
            objectViewWidget->setProjectNavigatorVisible(set);
            if (set) { // on showing, arrow should be updated immediately
                wnd->slotProjectNavigatorVisibilityChanged(set);
            }
        } else {
            objectViewWidget->projectNavigator()->setVisible(set);
            action_show_nav->setChecked(set);
            wnd->slotProjectNavigatorVisibilityChanged(set);
        }
    }
}

KexiAssistantPage *KexiMainWindow::Private::visibleMainMenuWidgetPage()
{
    const KexiAssistantWidget *widget = qobject_cast<const KexiAssistantWidget*>(tabbedToolBar->mainMenuContent());
    if (widget && widget->isVisible()) {
        return widget->currentPage();
    }
    return nullptr;
}

#ifndef KEXI_NO_PENDING_DIALOGS
void KexiMainWindow::Private::executeActionWhenPendingJobsAreFinished()
{
    ActionToExecuteWhenPendingJobsAreFinished a = actionToExecuteWhenPendingJobsAreFinished;
    actionToExecuteWhenPendingJobsAreFinished = NoAction;
    switch (a) {
    case QuitAction:
        qApp->quit();
        break;
    case CloseProjectAction:
        wnd->closeProject();
        break;
    default:;
    }
}

KexiWindow *KexiMainWindow::Private::openedWindowFor(const KexiPart::Item* item, PendingJobType &pendingType)
{
    return openedWindowFor(item->identifier(), pendingType);
}

KexiWindow *KexiMainWindow::Private::openedWindowFor(int identifier, PendingJobType &pendingType)
{
//! @todo (threads)  QMutexLocker dialogsLocker( &dialogsMutex );
    QHash<int, PendingJobType>::ConstIterator it = pendingWindows.find(identifier);
    if (it == pendingWindows.end())
        pendingType = NoJob;
    else
        pendingType = it.value();

    if (pendingType == WindowOpeningJob) {
        return 0;
    }
    return windows.contains(identifier) ? (KexiWindow*)windows.value(identifier) : 0;
}

void KexiMainWindow::Private::addItemToPendingWindows(const KexiPart::Item* item, PendingJobType jobType)
{
//! @todo (threads)  QMutexLocker dialogsLocker( &dialogsMutex );
    pendingWindows.insert(item->identifier(), jobType);
}

bool KexiMainWindow::Private::pendingWindowsExist()
{
    if (pendingWindows.begin() != pendingWindows.end())
        qDebug() <<  pendingWindows.constBegin().key() << (int)pendingWindows.constBegin().value();
//! @todo (threads)  QMutexLocker dialogsLocker( &dialogsMutex );
    return !pendingWindows.isEmpty();
}

void KexiMainWindow::Private::removePendingWindow(int identifier)
{
//! @todo (threads)  QMutexLocker dialogsLocker( &dialogsMutex );
    pendingWindows.remove(identifier);
}

#else // KEXI_NO_PENDING_DIALOGS

KexiWindow *KexiMainWindow::Private::openedWindowFor(int identifier)
{
//! @todo (threads)  QMutexLocker dialogsLocker( &dialogsMutex );
    return windows.contains(identifier) ? (KexiWindow*)windows.value(identifier) : 0;
}
#endif
