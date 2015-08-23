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

#ifndef KEXIMAINWINDOW_P_H
#define KEXIMAINWINDOW_P_H

#define KEXI_NO_PROCESS_EVENTS

#ifdef KEXI_NO_PROCESS_EVENTS
# define KEXI_NO_PENDING_DIALOGS
#endif

#define PROJECT_NAVIGATOR_TABBAR_ID 0
#define PROPERTY_EDITOR_TABBAR_ID 1

#include <QHBoxLayout>
#include <QPainter>
#include <QKeyEvent>
#include <QScopedPointer>
#include <QTabWidget>

#include <KToolBar>
#include <KHelpMenu>

#include <KexiIcon.h>

#include "KexiSearchLineEdit.h"
#include "KexiUserFeedbackAgent.h"
#include <kexiutils/SmallToolButton.h>
#include <kexiutils/styleproxy.h>
#include <kexiutils/KexiTester.h>
#include <kexiutils/utils.h>
#include <core/kexi.h>

class KexiProjectNavigator;

static const int KEXITABBEDTOOLBAR_SPACER_TAB_INDEX = 1;

//! @short Main application's tabbed toolbar
class KexiTabbedToolBar : public QTabWidget
{
    Q_OBJECT
public:
    explicit KexiTabbedToolBar(QWidget *parent);
    virtual ~KexiTabbedToolBar();

    KToolBar *createWidgetToolBar() const;

    KToolBar *toolBar(const QString& name) const;

    void appendWidgetToToolbar(const QString& name, QWidget* widget);

    void setWidgetVisibleInToolbar(QWidget* widget, bool visible);

//! @todo replace with the final Actions API
    void addAction(const QString& toolBarName, QAction *action);

    bool mainMenuVisible() const;

    QRect tabRect(int index) const { return tabBar()->tabRect(index); }

    KHelpMenu *helpMenu() const;

    void addSearchableModel(KexiSearchableModel *model);

    KToolBar *createToolBar(const char *name, const QString& caption);

    void setCurrentTab(const QString& name);

    //! Sets current tab to @a index, counting from first visible (non-Kexi) tab.
    //! In non-user mode, the first visible tab is "create" tab.
    void setCurrentTab(int index);

    void hideTab(const QString& name);

    void showTab(const QString& name);

    bool isTabVisible(const QString& name) const;

    bool isRolledUp();

public Q_SLOTS:
    void setMainMenuContent(QWidget *w);
    void selectMainMenuItem(const char *actionName);
    void showMainMenu(const char* actionName = 0);
    void hideMainMenu();
    void toggleMainMenu();
    void activateSearchLineEdit();
    void toggleRollDown();

protected:
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void leaveEvent(QEvent* event);
    virtual bool eventFilter(QObject* watched, QEvent* event);

protected Q_SLOTS:
    void slotCurrentChanged(int index);
    void slotDelayedTabRaise();
    void slotSettingsChanged(int category);
    //! Used for delayed loading of the "create" toolbar. Called only once.
    void setupCreateWidgetToolbar();
    void slotTabDoubleClicked(int index);
    void tabBarAnimationFinished();

private:
    void addAction(KToolBar *tbar, const char* actionName);
    void addSeparatorAndAction(KToolBar *tbar, const char* actionName);

    class Private;
    Private * const d;
};

//! @internal window container created to speedup opening new tabs
class KexiWindowContainer : public QWidget
{
public:
    explicit KexiWindowContainer(QWidget* parent)
            : QWidget(parent)
            , window(0)
            , lyr(new QVBoxLayout(this)) {
        lyr->setContentsMargins(0, 0, 0, 0);
    }
    ~KexiWindowContainer() {
        //! @todo warning if saveSettings() failed?
        if (window) {
            window->saveSettings();
            delete (KexiWindow*)window;
        }
    }
    void setWindow(KexiWindow* w) {
        window = w;
        if (w)
            lyr->addWidget(w);
    }
    QPointer<KexiWindow> window;
private:
    QVBoxLayout *lyr;
};

#include <KFadeWidgetEffect>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QStackedLayout>
#include "KexiMenuWidget.h"

class EmptyMenuContentWidget : public QWidget
{
public:
    explicit EmptyMenuContentWidget(QWidget* parent = 0)
     : QWidget(parent)
    {
        setAutoFillBackground(true);
        alterBackground();
    }
    void alterBackground()
    {
        QPalette pal(palette());
        QColor bg(pal.color(QPalette::Window));
        bg.setAlpha(200);
        pal.setColor(QPalette::Window, bg);
        setPalette(pal);
    }
    virtual void changeEvent(QEvent *e) {
        if (e->type() == QEvent::PaletteChange) {
            alterBackground();
        }
        QWidget::changeEvent(e);
    }
};

class KexiFadeWidgetEffect : public KFadeWidgetEffect
{
    Q_OBJECT
public:
    explicit KexiFadeWidgetEffect(QWidget *destWidget, int duration = 250)
    : KFadeWidgetEffect(destWidget)
    , m_duration(duration)
    {
    }
public Q_SLOTS:
    void start() { KFadeWidgetEffect::start(m_duration); }
private:
    int m_duration;
};

//! A style proxy for KexiMenuWidget
class KexiMenuWidgetStyle : public KexiUtils::StyleProxy
{
public:
    explicit KexiMenuWidgetStyle(QStyle *style, QObject *parent = 0)
        : KexiUtils::StyleProxy(style, parent)
    {
    }
    virtual ~KexiMenuWidgetStyle() {
    }
    virtual void drawControl(ControlElement element, const QStyleOption *option,
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
};

//! Main menu
class KexiMainMenu : public QWidget
{
    Q_OBJECT
public:
    explicit KexiMainMenu(KexiTabbedToolBar *toolBar, QWidget* parent = 0) : QWidget(parent),
        m_toolBar(toolBar), m_initialized(false)
    {
        m_content = 0;
        m_selectFirstItem = false;
    }
    ~KexiMainMenu() {
        delete (QWidget*)m_contentWidget;
    }
    virtual bool eventFilter(QObject * watched, QEvent* event) {
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

    void setContent(QWidget *contentWidget) {
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
            m_contentWidget->setPalette(contentWidgetPalette);

            m_contentWidget->setAutoFillBackground(true);
            m_contentWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
            m_contentWidget->setContentsMargins(0, 0, 0, 0);
            m_contentLayout->addWidget(m_contentWidget);
            m_contentLayout->setCurrentWidget(m_contentWidget);
            m_contentWidget->setFocus();
            m_contentWidget->installEventFilter(this);
            connect(m_contentWidget, SIGNAL(destroyed()), this, SLOT(contentWidgetDestroyed()));
        }
        if (fadeEffect) {
            if (m_contentWidget)
                m_contentLayout->update();

            QTimer::singleShot(10, fadeEffect, SLOT(start()));
        }
    }

    const QWidget *contentWidget() const {
        return m_contentWidget;
    }

    void setPersistentlySelectedAction(KexiMenuWidgetAction* action, bool set)
    {
        m_persistentlySelectedAction = action;
        m_persistentlySelectedAction->setPersistentlySelected(set);
    }

/*    void setActiveAction(QAction* action = 0) {
        if (!action && !m_menuWidget->actions().isEmpty()) {
            action = actions().first();
        }
        if (action) {
            m_menuWidget->setActiveAction(action);
        }
    }*/

    void selectFirstItem() {
        m_selectFirstItem = true;
    }

Q_SIGNALS:
    void contentAreaPressed();
    void hideContentsRequested();

protected Q_SLOTS:
    void contentWidgetDestroyed();

protected:
    virtual void showEvent(QShowEvent * event) {
        if (!m_initialized) {
            m_initialized = true;
            KActionCollection *ac = KexiMainWindowIface::global()->actionCollection();
            QHBoxLayout *hlyr = new QHBoxLayout(this);
            hlyr->setSpacing(0);
            hlyr->setMargin(0);
            m_menuWidget = new KexiMenuWidget;
            QString styleName(m_menuWidget->style()->objectName());
            if (KDE::version() < KDE_MAKE_VERSION(4, 8, 0) // a fix is apparently needed for glitch in KDE < 4.8
                && styleName == "oxygen")
            {
                KexiMenuWidgetStyle *customStyle = new KexiMenuWidgetStyle(m_menuWidget->style(), this);
                m_menuWidget->setStyle(customStyle);
            }
            m_menuWidget->installEventFilter(this);
            m_menuWidget->setFocusPolicy(Qt::StrongFocus);
            setFocusProxy(m_menuWidget);
            m_menuWidget->setFrame(false);
            m_menuWidget->setAutoFillBackground(true);
            int leftmargin, topmargin, rightmargin, bottommargin;
            m_menuWidget->getContentsMargins(&leftmargin, &topmargin, &rightmargin, &bottommargin);
            m_menuWidget->setContentsMargins(leftmargin, topmargin, rightmargin, bottommargin);

            m_menuWidget->addAction(ac->action("project_welcome"));
            m_menuWidget->addAction(ac->action("project_open"));
            m_menuWidget->addAction(ac->action("project_close"));
            m_menuWidget->addSeparator();
            m_menuWidget->addAction(ac->action("project_new"));
            m_menuWidget->addAction(ac->action("project_import_export_send"));
            m_menuWidget->addAction(ac->action("project_properties"));
            //! @todo project information
            m_menuWidget->addAction(ac->action("settings"));
            m_menuWidget->addSeparator();
            m_menuWidget->addAction(ac->action("quit"));
            hlyr->addWidget(m_menuWidget);
            m_content = new EmptyMenuContentWidget;
            m_content->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
            m_content->installEventFilter(this);
            m_mainContentLayout = new QVBoxLayout;
            m_mainContentLayout->addSpacing(styleName == "oxygen" ? 0 : 3);
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

private:
    QPointer<KexiMenuWidget> m_menuWidget;
    KexiTabbedToolBar* m_toolBar;
    bool m_initialized;
    EmptyMenuContentWidget *m_content;
    QStackedLayout *m_contentLayout;
    QPointer<QWidget> m_contentWidget;
    QVBoxLayout* m_mainContentLayout;
    QPointer<KexiMenuWidgetAction> m_persistentlySelectedAction;
    bool m_selectFirstItem;
};

void KexiMainMenu::contentWidgetDestroyed()
{
}

class KexiTabbedToolBarTabBar;

//! @internal
class KexiTabbedToolBar::Private : public QObject
{
    Q_OBJECT
public:
    explicit Private(KexiTabbedToolBar *t)
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

    KToolBar *createToolBar(const char *name, const QString& caption);
    int tabIndex;

public Q_SLOTS:
    void showMainMenu(const char* actionName = 0);
    void hideMainMenu();
    void hideContentsOrMainMenu();
    void toggleMainMenu();
    void updateMainMenuGeometry();

public:
    KexiTabbedToolBarTabBar *customTabBar;
    QPointer<KexiMainMenu> mainMenu;

    KexiTabbedToolBar *q;
    KActionCollection *ac;
    int createId;
    KToolBar *createWidgetToolBar;
#ifdef KEXI_AUTORISE_TABBED_TOOLBAR
    //! Used for delayed tab raising
    int tabToRaise;
    //! Used for delayed tab raising
    QTimer tabRaiseTimer;
#endif
    //! Toolbars for name
    QHash<QString, KToolBar*> toolbarsForName;
    QHash<QString, int> toolbarsIndexForName;
    QHash<QString, QString> toolbarsCaptionForName;
    QVector<bool> toolbarsVisibleForIndex;
    QHash<QWidget*, QAction*> extraActions;
    bool rolledUp;
    QPropertyAnimation tabBarAnimation;
    QGraphicsOpacityEffect tabBarOpacityEffect;
    int rolledUpIndex;
    KHelpMenu *helpMenu;
    KexiSearchLineEdit *searchLineEdit;
    void setCurrentTab(const QString& name);
    void hideTab(const QString& name);
    void showTab(const QString& name);
    bool isTabVisible(const QString& name) const;
#ifndef NDEBUG
    void debugToolbars() const;
#endif
    int lowestIndex;
};

#include <KSharedConfig>

#include <QTabBar>
#include <QDebug>

class KexiTabbedToolBarStyle;

//! Tab bar reimplementation for KexiTabbedToolBar.
/*! The main its purpose is to alter the width of "Kexi" tab.
*/
class KexiTabbedToolBarTabBar : public QTabBar
{
    Q_OBJECT
public:
    explicit KexiTabbedToolBarTabBar(QWidget *parent = 0);
    virtual QSize originalTabSizeHint(int index) const;
    virtual QSize tabSizeHint(int index) const;

    KexiTabbedToolBarStyle* customStyle;
};

//! @return true is @a style name is specific regarding tab styling
static bool isSpecificTabStyle(const QString &styleName)
{
    return styleName == "oxygen" || styleName == "qtcurve" || styleName == "gtk+";
}

//! Style proxy for KexiTabbedToolBar, to get the "Kexi" tab style right.
class KexiTabbedToolBarStyle : public KexiUtils::StyleProxy
{
public:
    explicit KexiTabbedToolBarStyle(QStyle *style, QObject *parent = 0)
      : KexiUtils::StyleProxy(style, parent)
    {
    }
    virtual ~KexiTabbedToolBarStyle() {
    }
    virtual void drawControl(ControlElement element, const QStyleOption *option,
                             QPainter *painter, const QWidget *widget = 0) const
    {
        const QString styleName(parentStyle()->objectName());
        qreal origOpacity = -1.0;
        if (element == CE_TabBarTab) {
            const QStyleOptionTabV2* opt
                = qstyleoption_cast<const QStyleOptionTabV2*>(option);
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

                QStyleOptionTabV2 newOpt(*opt);
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
                    KexiUtils::StyleProxy::drawControl(CE_TabBarTabLabel, &newOpt, painter, widget);
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
                        bg = Qt::transparent;
                    }
                    QFont origFont(painter->font());
                    QFont f(origFont);
                    f.setBold(true);
                    painter->setFont(f);
                    newOpt.palette.setBrush(QPalette::Window, bg);
                    newOpt.palette.setBrush(QPalette::Button, // needed e.g. for Plastique style
                                            bg);
                    KexiUtils::StyleProxy::drawControl(element, &newOpt, painter, widget);
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
                    KexiUtils::StyleProxy::drawControl(element, &newOpt, painter, widget);
                    if (origOpacity != -1.0) {
                        // restore opacity and draw labels using full this opacity
                        painter->setOpacity(origOpacity);
                        if (index > 0) {
                            KexiUtils::StyleProxy::drawControl(CE_TabBarTabLabel, &newOpt, painter, widget);
                        }
                    }
                    return;
                }
            }
        }
        else if (element == CE_ToolBar) {
            return;
        }
        KexiUtils::StyleProxy::drawControl(element, option, painter, widget);
    }

    virtual void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                          QPainter *painter, const QWidget *widget = 0) const
    {
        const QString styleName(parentStyle()->objectName());
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
        KexiUtils::StyleProxy::drawPrimitive(element, option, painter, widget);
    }

    virtual int pixelMetric(PixelMetric metric, const QStyleOption* option = 0,
                            const QWidget* widget = 0) const
    {
        if (metric == QStyle::PM_SmallIconSize)
            return KIconLoader::SizeMedium;
        return KexiUtils::StyleProxy::pixelMetric(metric, option, widget);
    }
};

//! Tab bar reimplementation for KexiTabbedToolBar.
/*! The main its purpose is to alter the width of "Kexi" tab.
*/
KexiTabbedToolBarTabBar::KexiTabbedToolBarTabBar(QWidget *parent)
    : QTabBar(parent)
{
    setObjectName("tabbar");
    customStyle = new KexiTabbedToolBarStyle(style(), this);
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
    ot.text = (isSpecificTabStyle(style()->objectName()) ? "  " : "") + tabText(index);
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

void KexiTabbedToolBar::activateSearchLineEdit()
{
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
    KToolBar *tbar = new KToolBar(q);
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
    connect(tabBar(), SIGNAL(contextMenu(int,QPoint)), SLOT(contextMenu(int,QPoint&)));
    connect(tabBar(), SIGNAL(tabDoubleClicked(int)), SLOT(mouseDoubleClick(int)));
    connect(tabBar(), SIGNAL(newTabRequest()), this, SIGNAL(mouseDoubleClick())); // #185487
    connect(tabBar(), SIGNAL(mouseMiddleClick(int)), SLOT(mouseMiddleClick(int)));
    connect(tabBar(), SIGNAL(initiateDrag( int )), SLOT(initiateDrag( int )));
    connect(tabBar(), SIGNAL(testCanDecode(QDragMoveEvent*,bool*)), SIGNAL(testCanDecode(QDragMoveEvent*,bool*)));
    connect(tabBar(), SIGNAL(receivedDropEvent(int,QDropEvent*)), SLOT(receivedDropEvent(int,QDropEvent*)));
    connect(tabBar(), SIGNAL(moveTab(int,int)), SLOT(moveTab(int,int)));
    connect(tabBar(), SIGNAL(tabCloseRequested(int)), SLOT(closeRequest(int)));

    setMouseTracking(true); // for mouseMoveEvent()
    setWhatsThis(xi18n("Task-oriented toolbar. Groups commands using tabs."));
#ifdef KEXI_AUTORISE_TABBED_TOOLBAR
    connect(&d->tabRaiseTimer, SIGNAL(timeout()), this, SLOT(slotDelayedTabRaise()));
#endif
    connect(tabBar(), SIGNAL(tabDoubleClicked(int)), this, SLOT(slotTabDoubleClicked(int)));

    d->ac = KexiMainWindowIface::global()->actionCollection();
    QWidget *mainWin = KexiMainWindowIface::global()->thisWidget();
    const bool userMode = KexiMainWindowIface::global()->userMode();
    KToolBar *tbar;

    KexiUtils::smallFont(this/*init*/);
    slotSettingsChanged(KGlobalSettings::FontChanged);
    connect(KGlobalSettings::self(), SIGNAL(settingsChanged(int)), this, SLOT(slotSettingsChanged(int)));

    // help area
    QWidget *helpWidget = new QWidget(this);
    helpWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    QHBoxLayout *helpLyr = new QHBoxLayout(helpWidget);
    helpLyr->setContentsMargins(0, 0, 0, 0);

    // * HELP MENU
    // add help menu actions... (KexiTabbedToolBar depends on them)
    d->helpMenu = new KHelpMenu(this, KGlobal::mainComponent().aboutData(),
                                true/*showWhatsThis*/, d->ac);
    QAction* help_report_bug_action = d->ac->action("help_report_bug");
    QObject::disconnect(help_report_bug_action, 0, 0, 0);
    QObject::connect(help_report_bug_action, SIGNAL(triggered()), mainWin, SLOT(slotReportBug()));
    help_report_bug_action->setText(xi18nc("Report a bug or wish for Kexi application", "Report a &Bug or Wish..."));
    help_report_bug_action->setIcon(koIcon("tools-report-bug")); // good icon for toolbar
    help_report_bug_action->setWhatsThis(xi18n("Files a bug or wish for Kexi application."));
    QAction* help_whats_this_action =  d->ac->action("help_whats_this");
    help_whats_this_action->setWhatsThis(xi18n("Activates a \"What's This?\" tool."));
    QAction* help_contents_action = d->ac->action("help_contents");
    help_contents_action->setText(xi18n("Help"));
    help_contents_action->setWhatsThis(xi18n("Shows Kexi Handbook."));
    QAction* help_about_app_action = d->ac->action("help_about_app");
    help_about_app_action->setWhatsThis(xi18n("Shows information about Kexi application."));
    QAction* help_about_kde_action = d->ac->action("help_about_kde");
    help_about_kde_action->setWhatsThis(xi18n("Shows information about KDE."));

    QAction *action_show_help_menu = d->ac->action("help_show_menu");
    KexiSmallToolButton *btn = new KexiSmallToolButton(koIcon("help-contextual"), QString(), helpWidget);
    btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
    btn->setPopupMode(QToolButton::InstantPopup);
    btn->setToolTip(action_show_help_menu->toolTip());
    btn->setWhatsThis(action_show_help_menu->whatsThis());
    btn->setFocusPolicy(Qt::NoFocus);
    QStyleOptionToolButton opt;
    opt.initFrom(btn);
    int w = btn->sizeHint().width();
    int wAdd = btn->style()->pixelMetric(QStyle::PM_MenuButtonIndicator, &opt, btn);
    if (w <= (2 * (wAdd + 1))) {
        w += wAdd + 2;
    }
    btn->setMinimumWidth(w);
    connect(action_show_help_menu, SIGNAL(triggered()), btn, SLOT(showMenu()));
    helpLyr->addWidget(btn);
    btn->setMenu(d->helpMenu->menu());
    setCornerWidget(helpWidget, Qt::TopRightCorner);
    d->searchLineEdit = new KexiSearchLineEdit;
    kexiTester() << KexiTestObject(d->searchLineEdit, "globalSearch.lineEdit");
    d->searchLineEdit->installEventFilter(this);
    helpLyr->addWidget(d->searchLineEdit);

    // needed e.g. for Windows style to remove the toolbar's frame
    QWidget *dummyWidgetForMainMenu = new QWidget(this);
    dummyWidgetForMainMenu->setObjectName("kexi");
    addTab(dummyWidgetForMainMenu, KGlobal::mainComponent().aboutData()->programName());
    d->toolbarsVisibleForIndex.append(true);
    addTab(new QWidget(this), QString()); // dummy for spacer
    d->toolbarsVisibleForIndex.append(true);

    if (!userMode) {
        d->createWidgetToolBar = d->createToolBar("create", xi18n("Create"));
    }

    tbar = d->createToolBar("data", xi18n("Data"));
    addAction(tbar, "edit_cut");
    addAction(tbar, "edit_copy");
    addAction(tbar, "edit_paste");
    if (!userMode)
        addAction(tbar, "edit_paste_special_data_table");
//! @todo move undo/redo to quickbar:

    tbar = d->createToolBar("external", xi18n("External Data"));
    if (!userMode) {
        addAction(tbar, "project_import_data_table");
        addAction(tbar, "tools_import_tables");
    }

    tbar = d->createToolBar("tools", xi18n("Tools"));
    addAction(tbar, "tools_compact_database");

//! @todo move to form plugin
    tbar = d->createToolBar("form", xi18n("Form Design"));

//! @todo move to report plugin
    tbar = d->createToolBar("report", xi18n("Report Design"));

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

KexiTabbedToolBar::~KexiTabbedToolBar()
{
    delete d;
}

bool KexiTabbedToolBar::mainMenuVisible() const
{
    return d->mainMenu && d->mainMenu->isVisible();
}

KHelpMenu* KexiTabbedToolBar::helpMenu() const
{
    return d->helpMenu;
}

void KexiTabbedToolBar::slotSettingsChanged(int category)
{
    if (category == KGlobalSettings::FontChanged) {
        setFont(KGlobalSettings::menuFont());   // toolbar acts like a menu
    }
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
        if (watched == d->searchLineEdit) {
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
    if (index == 0)
        return; // main item does not count here
    d->rolledUp = !d->rolledUp;
    d->tabBarAnimation.stop();
    QWidget *w = widget(currentIndex());
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
    d->searchLineEdit->addSearchableModel(model);
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


//! @short A widget being main part of KexiMainWindow
class KexiMainWidget : public KMainWindow
{
    Q_OBJECT
public:
    KexiMainWidget();
    virtual ~KexiMainWidget();

    void setParent(KexiMainWindow* mainWindow) {
        KMainWindow::setParent(mainWindow);
        m_mainWindow = mainWindow;
    }

    KexiMainWindowTabWidget* tabWidget() const {
        return m_tabWidget;
    }
protected:
    virtual bool queryClose();
protected Q_SLOTS:
    void slotCurrentTabIndexChanged(int index);
Q_SIGNALS:
    void currentTabIndexChanged(int index);

private:
    void setupCentralWidget();

    KexiMainWindowTabWidget* m_tabWidget;
    KexiMainWindow *m_mainWindow;
    QPointer<KexiWindow> m_previouslyActiveWindow;

    friend class KexiMainWindow;
    friend class KexiMainWindowTabWidget;
};

KexiMainWidget::KexiMainWidget()
        : KMainWindow(0, Qt::Widget)
        , m_mainWindow(0)
{
    setupCentralWidget();
}

KexiMainWidget::~KexiMainWidget()
{
}

void KexiMainWidget::setupCentralWidget()
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *centralWidgetLyr = new QVBoxLayout(centralWidget);
    m_tabWidget = new KexiMainWindowTabWidget(centralWidget, this);
    connect(m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(slotCurrentTabIndexChanged(int)));
    centralWidgetLyr->setContentsMargins(0, 0, 0, 0);
    centralWidgetLyr->addWidget(m_tabWidget);
    setCentralWidget(centralWidget);
}

bool KexiMainWidget::queryClose()
{
    return m_mainWindow ? m_mainWindow->queryClose() : true;
}

void KexiMainWidget::slotCurrentTabIndexChanged(int index)
{
    KexiWindowContainer* cont = dynamic_cast<KexiWindowContainer*>(m_tabWidget->widget(index));
    if (! cont || (KexiWindow*)m_previouslyActiveWindow == cont->window)
        return;
    if (m_mainWindow)
        m_mainWindow->activeWindowChanged(cont->window, (KexiWindow*)m_previouslyActiveWindow);
    m_previouslyActiveWindow = cont->window;
    emit currentTabIndexChanged(index);
}

//------------------------------------------

//! @internal Dock widget with floating disabled but still collapsible
class KexiDockWidget : public QDockWidget
{
    Q_OBJECT
public:
    KexiDockWidget(const QString &title, QWidget *parent);

    virtual ~KexiDockWidget();

    virtual void setSizeHint(const QSize& hint);

    virtual QSize sizeHint() const;

    QToolButton* closeButton() const;

protected:
    virtual void paintEvent(QPaintEvent *pe);

protected Q_SLOTS:
    void slotCloseClicked();

private:
    class Private;
    Private * const d;
};

//------------------------------------------

//! @internal safer dictionary
typedef QMap< int, KexiWindow* > KexiWindowDict;

//! @internal
class KexiMainWindow::Private
{
public:
    explicit Private(KexiMainWindow* w)
            : wnd(w) {
        actionCollection = new KActionCollection(w);
        propEditor = 0;
        propEditorDockWidget = 0;
        navDockWidget = 0;
        propEditorTabWidget = 0;
        KexiProjectData *pdata = Kexi::startupHandler().projectData();
        userMode = Kexi::startupHandler().forcedUserMode() /* <-- simply forced the user mode */
                   /* project has 'user mode' set as default and not 'design mode' override is found: */
                   || (pdata && pdata->userMode() && !Kexi::startupHandler().forcedDesignMode());
        isProjectNavigatorVisible = Kexi::startupHandler().isProjectNavigatorVisible();
        isMainMenuVisible = Kexi::startupHandler().isMainMenuVisible();
        navigator = 0;
        prj = 0;
        config = KSharedConfig::openConfig();
        nameDialog = 0;
        m_findDialog = 0;
        focus_before_popup = 0;
        action_view_nav = 0;
        action_view_propeditor = 0;
        action_view_mainarea = 0;
        action_welcome_projects_title_id = -1;
        action_welcome_connections_title_id = -1;
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
#ifdef HAVE_KNEWSTUFF
        newStuff = 0;
#endif
        forceShowProjectNavigatorOnCreation = false;
        forceHideProjectNavigatorOnCreation = false;
        navWasVisibleBeforeProjectClosing = false;
        saveSettingsForShowProjectNavigator = true;
        propertyEditorCollapsed = false;
        enable_slotPropertyEditorVisibilityChanged = true;
    }
    ~Private() {
        qDeleteAll(m_openedCustomObjectsForItem);
    }

#ifndef KEXI_NO_PENDING_DIALOGS
    //! Job type. Currently used for marking items as being opened or closed.
    enum PendingJobType {
        NoJob = 0,
        WindowOpeningJob,
        WindowClosingJob
    };

    KexiWindow *openedWindowFor(const KexiPart::Item* item, PendingJobType &pendingType) {
        return openedWindowFor(item->identifier(), pendingType);
    }

    KexiWindow *openedWindowFor(int identifier, PendingJobType &pendingType) {
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
#else
    KexiWindow *openedWindowFor(int identifier) {
//! @todo (threads)  QMutexLocker dialogsLocker( &dialogsMutex );
        return windows.contains(identifier) ? (KexiWindow*)windows.value(identifier) : 0;
    }
#endif

    void insertWindow(KexiWindow *window) {
//! @todo (threads)  QMutexLocker dialogsLocker( &dialogsMutex );
        windows.insert(window->id(), window);
#ifndef KEXI_NO_PENDING_DIALOGS
        pendingWindows.remove(window->id());
#endif
    }

#ifndef KEXI_NO_PENDING_DIALOGS
    void addItemToPendingWindows(const KexiPart::Item* item, PendingJobType jobType) {
//! @todo (threads)  QMutexLocker dialogsLocker( &dialogsMutex );
        pendingWindows.insert(item->identifier(), jobType);
    }

    bool pendingWindowsExist() {
        if (pendingWindows.begin() != pendingWindows.end())
            qDebug() <<  pendingWindows.constBegin().key() << " " << (int)pendingWindows.constBegin().value();
//! @todo (threads)  QMutexLocker dialogsLocker( &dialogsMutex );
        return !pendingWindows.isEmpty();
    }
#endif

    void updateWindowId(KexiWindow *window, int oldItemID) {
//! @todo (threads)  QMutexLocker dialogsLocker( &dialogsMutex );
        windows.remove(oldItemID);
#ifndef KEXI_NO_PENDING_DIALOGS
        pendingWindows.remove(oldItemID);
#endif
        windows.insert(window->id(), window);
    }

    void removeWindow(int identifier) {
//! @todo (threads)  QMutexLocker dialogsLocker( &dialogsMutex );
        windows.remove(identifier);
    }

#ifndef KEXI_NO_PENDING_DIALOGS
    void removePendingWindow(int identifier) {
//! @todo (threads)  QMutexLocker dialogsLocker( &dialogsMutex );
        pendingWindows.remove(identifier);
    }
#endif

    uint openedWindowsCount() {
//! @todo (threads)  QMutexLocker dialogsLocker( &dialogsMutex );
        return windows.count();
    }

    //! Used in KexiMainWindowe::closeProject()
    void clearWindows() {
//! @todo (threads)  QMutexLocker dialogsLocker( &dialogsMutex );
        windows.clear();
#ifndef KEXI_NO_PENDING_DIALOGS
        pendingWindows.clear();
#endif
    }

    void showStartProcessMsg(const QStringList& args) {
        wnd->showErrorMessage(xi18n("Could not start %1 application.", QString(KEXI_APP_NAME)),
                              xi18n("Command \"%1\" failed.", args.join(" ")));
    }

    /* void hideMenuItem(const QString& menuName, const QString& itemText, bool alsoSeparator)
      {
        Q3PopupMenu *pm = popups[menuName.toLatin1()];
        if (!pm)
          return;
        const QList<QAction*> actions( pm->actions() );
        bool nowHideSeparator = false;
        foreach( QAction *action, actions ) {
          qDebug() << action->text();
          if (nowHideSeparator) {
            if (action->isSeparator())
              action->setVisible(false);
            break;
          }
          else if (action->text().toLower().trimmed() == itemText.toLower().trimmed()) {
//! @todo KEXI3 OK? this should only hide menu item, not global action... action->setVisible(false);
            action->setVisible(false);
            if (alsoSeparator)
              nowHideSeparator = true; //continue look to find separtator, if present
            else
              break;
          }
        }
      }

      void disableMenuItem(const QString& menuName, const QString& itemText)
      {
//! @todo KEXI3 reimplement disableMenuItem()
        Q3PopupMenu *pm = popups[menuName.toLatin1()];
        if (!pm)
          return;
        const QList<QAction*> actions( pm->actions() );
        foreach( QAction *action, actions ) {
          if (action->text().toLower().trimmed()==itemText.toLower().trimmed()) {
            action->setEnabled(false);
            break;
          }
        }
      }*/

    //! Updates Property Editor Pane's visibility for the current window and the @a viewMode view mode.
    /*! @a info can be provided to hadle cases when current window is not yet defined (in openObject()). */
    void updatePropEditorVisibility(Kexi::ViewMode viewMode, KexiPart::Info *info = 0) {
        if (!propEditorDockWidget)
            return;
        KexiWindow *currentWindow = wnd->currentWindow();
        if (!info && currentWindow) {
            info = currentWindow->part()->info();
        }
        const bool visible = (viewMode == Kexi::DesignViewMode)
            && ((currentWindow && currentWindow->propertySet()) || (info && info->isPropertyEditorAlwaysVisibleInDesignMode()));
        //qDebug() << "visible == " << visible;
        enable_slotPropertyEditorVisibilityChanged = false;
        if (visible && propertyEditorCollapsed) { // used when we're switching back to a window with propeditor available but collapsed
            propEditorDockWidget->setVisible(!visible);
            setPropertyEditorTabBarVisible(true);
        }
        else {
            propEditorDockWidget->setVisible(visible);
            setPropertyEditorTabBarVisible(false);
        }
        enable_slotPropertyEditorVisibilityChanged = true;
    }

    void setTabBarVisible(KMultiTabBar::KMultiTabBarPosition position, int id,
                          KexiDockWidget *dockWidget, bool visible)
    {
        KMultiTabBar *mtbar = multiTabBars.value(position);
        if (!visible) {
            mtbar->removeTab(id);
        }
        else if (!mtbar->tab(id)) {
            QString t(dockWidget->windowTitle());
            t.remove('&');
            mtbar->appendTab(QPixmap(), id, t);
            KMultiTabBarTab *tab = mtbar->tab(id);
            QObject::connect(tab, SIGNAL(clicked(int)),
                             wnd, SLOT(slotMultiTabBarTabClicked(int)),
                             Qt::UniqueConnection);
        }
    }

    void setPropertyEditorTabBarVisible(bool visible) {
        setTabBarVisible(KMultiTabBar::Right, PROPERTY_EDITOR_TABBAR_ID,
                         propEditorDockWidget, visible);
    }

    template<class type>
    type *openedCustomObjectsForItem(KexiPart::Item* item, const char* name) {
        if (!item || !name) {
            qWarning() << "!item || !name";
            return 0;
        }
        QByteArray key(QByteArray::number(item->identifier()) + name);
        return qobject_cast<type*>(m_openedCustomObjectsForItem.value(key));
    }

    void addOpenedCustomObjectForItem(KexiPart::Item* item, QObject* object, const char* name) {
        QByteArray key(QByteArray::number(item->identifier()) + name);
        m_openedCustomObjectsForItem.insert(key, object);
    }

    KexiFindDialog *findDialog() {
        if (!m_findDialog) {
            m_findDialog = new KexiFindDialog(wnd);
            m_findDialog->setActions(action_edit_findnext, action_edit_findprev,
                                     action_edit_replace, action_edit_replace_all);
        }
        return m_findDialog;
    }

    /*! Updates the find/replace dialog depending on the active view.
     Nothing is performed if the dialog is not instantiated yet or is invisible. */
    void updateFindDialogContents(bool createIfDoesNotExist = false) {
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

    //! \return the current view if it supports \a actionName, otherwise returns 0.
    KexiView *currentViewSupportingAction(const char* actionName) const {
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

    //! \return the current view if it supports KexiSearchAndReplaceViewInterface.
    KexiSearchAndReplaceViewInterface* currentViewSupportingSearchAndReplaceInterface() const {
        if (!wnd->currentWindow())
            return 0;
        KexiView *view = wnd->currentWindow()->selectedView();
        if (!view)
            return 0;
        return dynamic_cast<KexiSearchAndReplaceViewInterface*>(view);
    }

    KexiMainWindow *wnd;
    KexiMainWidget *mainWidget;
    KActionCollection *actionCollection;
    KexiStatusBar *statusBar;
    KHelpMenu *helpMenu;
    KexiProject *prj;
    KSharedConfig::Ptr config;
#ifndef KEXI_NO_CTXT_HELP
    KexiContextHelp *ctxHelp;
#endif
    KexiProjectNavigator *navigator;
    KexiTabbedToolBar *tabbedToolBar;
    QMap<int, QString> tabsToActivateOnShow;
    KexiDockWidget *navDockWidget;
    QTabWidget *propEditorTabWidget;
    KexiDockWidget *propEditorDockWidget;
    QPointer<KexiDockableWidget> propEditorDockableWidget;
    //! poits to kexi part which has been previously used to setup proppanel's tabs using
    //! KexiPart::setupCustomPropertyPanelTabs(), in updateCustomPropertyPanelTabs().
    QPointer<KexiPart::Part> partForPreviouslySetupPropertyPanelTabs;
    QMap<KexiPart::Part*, int> recentlySelectedPropertyPanelPages;
    QPointer<KexiPropertyEditorView> propEditor;
    QPointer<KPropertySet> propertySet;

    KexiNameDialog *nameDialog;
    QTimer timer; //!< helper timer
    QString appCaptionPrefix; //<! application's caption prefix - prj name (if opened), else: null

#ifndef KEXI_SHOW_UNIMPLEMENTED
    KActionMenu *dummy_action;
#endif

    //! Kexi menu
    QAction *action_save, *action_save_as,
    *action_project_import_export_send, *action_close,
    *action_project_properties,
    *action_project_relations, *action_project_import_data_table,
    *action_project_export_data_table;
#ifndef KEXI_NO_QUICK_PRINTING
    QAction *action_project_print, *action_project_print_preview,
        *action_project_print_setup;
#endif
    QAction *action_project_welcome;
    QAction *action_show_other;
    int action_welcome_projects_title_id,
    action_welcome_connections_title_id;
    QAction *action_settings;

    //! edit menu
    QAction *action_edit_delete, *action_edit_delete_row,
    *action_edit_cut, *action_edit_copy, *action_edit_paste,
    *action_edit_find, *action_edit_findnext, *action_edit_findprev,
    *action_edit_replace, *action_edit_replace_all,
    *action_edit_select_all,
    *action_edit_undo, *action_edit_redo,
    *action_edit_insert_empty_row,
    *action_edit_edititem, *action_edit_clear_table,
    *action_edit_paste_special_data_table,
    *action_edit_copy_special_data_table;

    //! view menu
    QAction *action_view_nav, *action_view_propeditor, *action_view_mainarea;
#ifndef KEXI_NO_CTXT_HELP
    KToggleAction *action_show_helper;
#endif
    //! data menu
    QAction *action_data_save_row;
    QAction *action_data_cancel_row_changes;
    QAction *action_data_execute;

    //! format menu
    QAction *action_format_font;

    //! tools menu
    QAction *action_tools_import_project, *action_tools_compact_database, *action_tools_data_import;
    KActionMenu *action_tools_scripts;

    //! window menu
    QAction *action_window_next, *action_window_previous, *action_window_fullscreen;

    //! global
    QAction *action_show_help_menu;
    QAction *action_view_global_search;
    //! for dock windows

    QPointer<QWidget> focus_before_popup;

    //! Set to true only in destructor, used by closeWindow() to know if
    //! user can cancel window closing. If true user even doesn't see any messages
    //! before closing a window. This is for extremely sanity... and shouldn't be even needed.
    bool forceWindowClosing;

    //! Indicates that we're inside closeWindow() method - to avoid inf. recursion
    //! on window removing
    bool insideCloseWindow;

#ifndef KEXI_NO_PENDING_DIALOGS
    //! Used in executeActionWhenPendingJobsAreFinished().
    enum ActionToExecuteWhenPendingJobsAreFinished {
        NoAction,
        QuitAction,
        CloseProjectAction
    };
    ActionToExecuteWhenPendingJobsAreFinished actionToExecuteWhenPendingJobsAreFinished;

    void executeActionWhenPendingJobsAreFinished() {
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
#endif

    //! Used for delayed windows closing for 'close all'
    QList<KexiWindow*> windowsToClose;

#ifndef KEXI_NO_QUICK_PRINTING
    //! Opened page setup dialogs, used by printOrPrintPreviewForItem().
    QHash<int, KexiWindow*> pageSetupWindows;

    /*! A map from Kexi dialog to "print setup" part item's ID of the data item
     used by closeWindow() to find an ID of the data item, so the entry
     can be removed from pageSetupWindows dictionary. */
    QMap<int, int> pageSetupWindowItemID2dataItemID_map;
#endif

    //! Indicates if project is started in User Mode
    bool userMode;

    //! Indicates if project navigator should be visible
    bool isProjectNavigatorVisible;

    //! Indicates if the main menu should be visible
    bool isMainMenuVisible;

    //! Set in restoreSettings() and used in initNavigator()
    //! to customize navigator visibility on startup
    bool forceShowProjectNavigatorOnCreation;
    bool forceHideProjectNavigatorOnCreation;

    bool navWasVisibleBeforeProjectClosing;
    bool saveSettingsForShowProjectNavigator;
#ifdef HAVE_KNEWSTUFF
    KexiNewStuff  *newStuff;
#endif

    //! Used by openedCustomObjectsForItem() and addOpenedCustomObjectForItem()
    QHash<QByteArray, QObject*> m_openedCustomObjectsForItem;

    int propEditorDockSeparatorPos, navDockSeparatorPos;

    bool wasAutoOpen;
    bool windowExistedBeforeCloseProject;

    QMap<KMultiTabBar::KMultiTabBarPosition, KMultiTabBar*> multiTabBars;
    bool propertyEditorCollapsed;

    bool enable_slotPropertyEditorVisibilityChanged;

    KexiUserFeedbackAgent userFeedback;

    //! @todo KEXI3 QScopedPointer<KexiMigration::MigrateManager> migrateManager;

private:
    //! @todo move to KexiProject
    KexiWindowDict windows;
#ifndef KEXI_NO_PROCESS_EVENTS
    QHash<int, PendingJobType> pendingWindows; //!< part item identifiers for windows whoose opening has been started
    //! @todo QMutex dialogsMutex; //!< used for locking windows and pendingWindows dicts
#endif
    KexiFindDialog *m_findDialog;
};

//------------------------------------------

//! Action shortcut used by KexiMainWindow::setupMainMenuActionShortcut(QAction *)
//! Activates action only if enabled.
class KexiMainMenuActionShortcut : public QShortcut
{
    Q_OBJECT
public:
    KexiMainMenuActionShortcut(const QKeySequence& key, QWidget *parent, QAction *action);

protected Q_SLOTS:
    //! Triggers associated action only when this action is enabled
    void slotActivated();

private:
    QPointer<QAction> m_action;
};

#endif
