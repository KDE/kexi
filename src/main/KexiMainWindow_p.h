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

#ifndef KEXIMAINWINDOW_P_H
#define KEXIMAINWINDOW_P_H

#include <QKeyEvent>
#include <QScopedPointer>
#include <QTabWidget>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QStackedLayout>
#include <QProxyStyle>
#include <QTabBar>
#include <QTimer>
#include <QDockWidget>
#include <QShortcut>
#include <QStackedWidget>
#include <QMenu>

#include <KToolBar>
#include <KHelpMenu>
#include <KAboutData>
#include <KActionCollection>
#include <KMultiTabBar>
#include <KActionMenu>
#include <KSharedConfig>

#include "KexiMainWindow.h"
#include "KexiSearchLineEdit.h"
#include "KexiUserFeedbackAgent.h"
#include "KexiMenuWidget.h"
#include "kexifinddialog.h"
#include "KexiStartup.h"
#include "KexiGlobalViewModeSelector.h"
#include <kexiutils/utils.h>
#include <widget/utils/KexiDockableWidget.h>
#include <widget/KexiNameDialog.h>
#include <core/kexi.h>
#include <core/KexiWindow.h>
#include <core/kexipartinfo.h>
#include <KexiFadeWidgetEffect.h>

#define KEXI_NO_PROCESS_EVENTS

#ifdef KEXI_NO_PROCESS_EVENTS
# define KEXI_NO_PENDING_DIALOGS
#endif

#define PROJECT_NAVIGATOR_TABBAR_ID 0
#define PROPERTY_EDITOR_TABBAR_ID 1
#define KEXITABBEDTOOLBAR_SPACER_TAB_INDEX 1

class QPainter;
class KexiAssistantPage;
class KexiProjectNavigator;
class KPropertyEditorView;

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

    QRect tabRect(int index) const;

    void addSearchableModel(KexiSearchableModel *model);

    void removeSearchableModel(KexiSearchableModel *model);

    KToolBar *createToolBar(const char *name, const QString& caption);

    void setCurrentTab(const QString& name);

    //! Sets current tab to @a index, counting from first visible (non-Kexi) tab.
    //! In non-user mode, the first visible tab is "create" tab.
    void setCurrentTab(int index);

    void hideTab(const QString& name);

    void showTab(const QString& name);

    bool isTabVisible(const QString& name) const;

    bool isRolledUp();

    const QWidget* mainMenuContent();

public Q_SLOTS:
    void setMainMenuContent(QWidget *w);
    void selectMainMenuItem(const char *actionName);
    void showMainMenu(const char* actionName = 0);
    void hideMainMenu();
    void toggleMainMenu();
    void activateSearchLineEdit();
    void toggleRollDown();

protected:
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void leaveEvent(QEvent* event) override;
    virtual bool eventFilter(QObject* watched, QEvent* event) override;

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

class EmptyMenuContentWidget : public QWidget
{
    Q_OBJECT
public:
    explicit EmptyMenuContentWidget(QWidget* parent = nullptr);

    void alterBackground();

    virtual void changeEvent(QEvent *e) override;
};

//! @todo KEXI3 is KexiMenuWidgetStyle needed?
#if 0
//! A style proxy for KexiMenuWidget
class KexiMenuWidgetStyle : public KexiUtils::StyleProxy
{
public:
    explicit KexiMenuWidgetStyle(QStyle *style, QObject *parent = 0);

    virtual ~KexiMenuWidgetStyle();

    virtual void drawControl(ControlElement element, const QStyleOption *option,
                             QPainter *painter, const QWidget *widget = 0) const;
};
#endif

//! Main menu
class KexiMainMenu : public QWidget
{
    Q_OBJECT
public:
    explicit KexiMainMenu(KexiTabbedToolBar *toolBar, QWidget* parent = nullptr);

    ~KexiMainMenu();

    virtual bool eventFilter(QObject * watched, QEvent* event) override;

    void setContent(QWidget *contentWidget);

    const QWidget *contentWidget() const;

    void setPersistentlySelectedAction(KexiMenuWidgetAction* action, bool set);

/*    void setActiveAction(QAction* action = 0);*/

    void selectFirstItem();

    tristate showProjectMigrationWizard(
        const QString& mimeType, const QString& databaseName, const KDbConnectionData *cdata);

Q_SIGNALS:
    void contentAreaPressed();
    void hideContentsRequested();

protected Q_SLOTS:
    //void contentWidgetDestroyed();

protected:
    virtual void showEvent(QShowEvent * event) override;

private:
    QPointer<KexiMenuWidget> m_menuWidget;
    KexiTabbedToolBar* m_toolBar;
    bool m_initialized;
    EmptyMenuContentWidget *m_content;
    QStackedLayout *m_contentLayout = nullptr;
    QPointer<QWidget> m_contentWidget;
    QVBoxLayout* m_mainContentLayout = nullptr;
    QPointer<KexiMenuWidgetAction> m_persistentlySelectedAction;
    bool m_selectFirstItem;
};

class KexiTabbedToolBarTabBar;

//! @internal
class Q_DECL_HIDDEN KexiTabbedToolBar::Private : public QObject
{
    Q_OBJECT
public:
    explicit Private(KexiTabbedToolBar *t);

    KToolBar *createToolBar(const char *name, const QString& caption);

    int tabIndex;

public Q_SLOTS:
    void showMainMenu(const char* actionName = 0);
    void hideMainMenu();
    void hideContentsOrMainMenu();
    void toggleMainMenu();
    void updateMainMenuGeometry();

    //! Initializes global search line edit. If it is enabled, it's created, if disabled, it's deleted.
    void initSearchLineEdit();

public:
    KexiTabbedToolBarTabBar *customTabBar;
    QPointer<KexiMainMenu> mainMenu;

    KexiTabbedToolBar *q;
    KActionCollection *ac;
    int createId;
    KToolBar *createWidgetToolBar;
    QHBoxLayout *helpLayer;
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
    KexiSearchLineEdit *searchLineEdit = nullptr;
    void setCurrentTab(const QString& name);
    void hideTab(const QString& name);
    void showTab(const QString& name);
    bool isTabVisible(const QString& name) const;
#ifndef NDEBUG
    void debugToolbars() const;
#endif
    int lowestIndex;
};

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
    virtual QSize tabSizeHint(int index) const override;

    KexiTabbedToolBarStyle* customStyle;
};

//! Style proxy for KexiTabbedToolBar, to get the "Kexi" tab style right.
class KexiTabbedToolBarStyle : public QProxyStyle
{
    Q_OBJECT
public:
    explicit KexiTabbedToolBarStyle(const QString &baseStyleName);

    virtual ~KexiTabbedToolBarStyle();

    virtual void drawControl(ControlElement element, const QStyleOption *option,
                             QPainter *painter, const QWidget *widget = 0) const override;

    virtual void drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                               QPainter *painter, const QWidget *widget = 0) const override;

    virtual int pixelMetric(PixelMetric metric, const QStyleOption* option = 0,
                            const QWidget* widget = 0) const override;
};

//! Style proxy for KexiTabbedToolBar, to fix the hardcoded margins (e.g. for Breeze).
class KexiDockWidgetStyle : public QProxyStyle
{
    Q_OBJECT
public:
    explicit KexiDockWidgetStyle(const QString &baseStyleName);

    virtual ~KexiDockWidgetStyle();

    using QProxyStyle::polish;
    void polish(QWidget* widget) override;
};

//------------------------------------------

//! @internal Dock widget with floating disabled but still collapsible
class KexiDockWidget : public QDockWidget
{
    Q_OBJECT
public:
    KexiDockWidget(const QString &tabText, QWidget *parent);

    virtual ~KexiDockWidget();

    virtual void setSizeHint(const QSize& hint);

    virtual QSize sizeHint() const override;

    const QString tabText; //!< for tab bar tabs

protected:
    virtual void paintEvent(QPaintEvent *pe) override;

private:
    class Private;
    Private * const d;
};

//------------------------------------------

//! @internal safer dictionary
typedef QMap< int, KexiWindow* > KexiWindowDict;

//! @internal
class Q_DECL_HIDDEN KexiMainWindow::Private
{
public:
    explicit Private(KexiMainWindow* w);

    ~Private();

#ifndef KEXI_NO_PENDING_DIALOGS
    //! Job type. Currently used for marking items as being opened or closed.
    enum PendingJobType {
        NoJob = 0,
        WindowOpeningJob,
        WindowClosingJob
    };

    KexiWindow *openedWindowFor(const KexiPart::Item* item, PendingJobType &pendingType);

    KexiWindow *openedWindowFor(int identifier, PendingJobType &pendingType);

    void addItemToPendingWindows(const KexiPart::Item* item, PendingJobType jobType);

    bool pendingWindowsExist();

    void removePendingWindow(int identifier);
#else
    KexiWindow *openedWindowFor(int identifier);
#endif

    void insertWindow(KexiWindow *window);

    bool windowContainerExistsFor(int identifier) const;

    void setWindowContainerExistsFor(int identifier, bool set);

    void updateWindowId(KexiWindow *window, int oldItemID);

    void removeWindow(int identifier);

    int openedWindowsCount();

    //! Used in KexiMainWindowe::closeProject()
    void clearWindows();

    void showStartProcessMsg(const QStringList& args);

    //! Updates Property Editor Pane's visibility for the current window and the @a viewMode view mode.
    /*! @a info can be provided to hadle cases when current window is not yet defined (in openObject()). */
    void updatePropEditorVisibility(Kexi::ViewMode viewMode, KexiPart::Info *info = 0);

    //void setTabBarVisible(KMultiTabBar::KMultiTabBarPosition position, int id,
    //                      KexiDockWidget *dockWidget, bool visible);

    //void setPropertyEditorTabBarVisible(bool visible);

    QObject *openedCustomObjectsForItem(KexiPart::Item* item, const char* name);

    void addOpenedCustomObjectForItem(KexiPart::Item* item, QObject* object, const char* name);

    KexiFindDialog *findDialog();

    /*! Updates the find/replace dialog depending on the active view.
     Nothing is performed if the dialog is not instantiated yet or is invisible. */
    void updateFindDialogContents(bool createIfDoesNotExist = false);

    //! \return the current view if it supports \a actionName, otherwise returns 0.
    KexiView *currentViewSupportingAction(const char* actionName) const;

    //! \return the current view if it supports KexiSearchAndReplaceViewInterface.
    KexiSearchAndReplaceViewInterface* currentViewSupportingSearchAndReplaceInterface() const;

    tristate showProjectMigrationWizard(
        const QString& mimeType, const QString& databaseName, const KDbConnectionData *cdata);

    KPropertyEditorView *propertyEditor() const;

    //! Show mode for panes
    enum ShowMode {
        ShowImmediately,
        ShowAnimated
    };

    //! Sets visibility of the project navigator without or without animating it.
    //! Related action is checked/unchecked accordingly.
    void setProjectNavigatorVisible(bool set, ShowMode mode = ShowImmediately);

    inline void addAction(QMenu *menu, const char* actionName) {
        menu->addAction(actionCollection->action(QLatin1String(actionName)));
    }

    /**
     * Returns current page of active visible main menu widget or @c nullptr if there is no visible
     * menu widget or menu widget contains no page.
     */
    KexiAssistantPage *visibleMainMenuWidgetPage();

    KexiMainWindow *wnd;
    QStackedWidget *globalViewStack;
    KexiObjectViewWidget *objectViewWidget;
    QPointer<KexiFadeWidgetEffect> propertyPaneAnimation;
    KActionCollection *actionCollection;
    KexiGlobalViewModeSelector *modeSelector;
    KHelpMenu *helpMenu;
    KexiProject *prj;
    KSharedConfig::Ptr config;
#ifdef KEXI_SHOW_CONTEXT_HELP
    KexiContextHelp *ctxHelp;
#endif
    KexiTabbedToolBar *tabbedToolBar;
    QMap<int, QString> tabsToActivateOnShow;
    //! poits to kexi part which has been previously used to setup proppanel's tabs using
    //! KexiPart::setupCustomPropertyPanelTabs(), in updateCustomPropertyPanelTabs().
    QPointer<KexiPart::Part> partForPreviouslySetupPropertyPanelTabs;
    QMap<KexiPart::Part*, int> recentlySelectedPropertyPanelPages;
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
    QAction *action_project_print, *action_project_print_preview,
        *action_project_print_setup;
    QAction *action_project_welcome;
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

    //! data menu
    QAction *action_data_save_row;
    QAction *action_data_cancel_row_changes;
    QAction *action_data_execute;

    //! format menu
    QAction *action_format_font;

    //! tools menu
    QAction *action_tools_import_project, *action_tools_compact_database, *action_tools_data_import;
    QAction *action_tools_locate;

    //! window menu
    KToggleAction *action_show_nav;
    KToggleAction *action_show_propeditor;
    QAction *action_activate_nav;
    QAction *action_activate_mainarea;
    QAction *action_activate_propeditor;
    QAction *action_window_next, *action_window_previous, *action_window_fullscreen;
    QAction *action_close_tab, *action_close_all_tabs;
    QAction *action_tab_next, *action_tab_previous;

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

    void executeActionWhenPendingJobsAreFinished();
#endif

    //! Used for delayed windows closing for 'close all'
    QList<KexiWindow*> windowsToClose;

#ifdef KEXI_QUICK_PRINTING_SUPPORT
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

    //! Used by openedCustomObjectsForItem() and addOpenedCustomObjectForItem()
    QHash<QByteArray, QObject*> m_openedCustomObjectsForItem;

    int propEditorDockSeparatorPos, navDockSeparatorPos;

    bool wasAutoOpen;
    bool windowExistedBeforeCloseProject;

    bool propertyEditorCollapsed;

    bool enable_slotPropertyEditorVisibilityChanged;

    KexiUserFeedbackAgent userFeedback;

    KexiMigrateManagerInterface* migrateManager;

private:
    //! @todo move to KexiProject
    KexiWindowDict windows;
    //! A set of item identifiers for whose there are KexiWindowContainer instances already.
    //! This lets to verify that KexiWindow is about to be constructed and opened so multiple
    //! opening can be avoided.
    QSet<int> windowContainers;
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
    KexiMainMenuActionShortcut(const QKeySequence& key, QAction *action, QWidget *parent);

    virtual ~KexiMainMenuActionShortcut();

protected Q_SLOTS:
    //! Triggers associated action only when this action is enabled
    void slotActivated();

private:
    QPointer<QAction> m_action;
};

#endif
