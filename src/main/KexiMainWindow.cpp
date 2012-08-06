/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2003-2011 Jarosław Staniek <staniek@kde.org>

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

#include "KexiMainWindow.h"
#include <config-kexi.h>
#include <unistd.h>

#include <QApplication>
#include <QEventLoop>
#include <QFile>
#include <QTimer>
#include <QObject>
#include <QProcess>
#include <QToolButton>

#include <QMutex>
#include <QWaitCondition>
#include <QFileDialog>
#include <QPixmap>
#include <QFocusEvent>
#include <QTextStream>
#include <QEvent>
#include <QKeyEvent>
#include <QHash>
#include <QDockWidget>
#include <QMenuBar>
#include <QShortcut>

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kaction.h>
#include <KActionCollection>
#include <kactionmenu.h>
#include <ktoggleaction.h>
#include <klocale.h>
#include <kstandardshortcut.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kdebug.h>
#include <kshortcutsdialog.h>
#include <kedittoolbar.h>
#include <ktogglefullscreenaction.h>

#include <kglobalsettings.h>
#include <ktip.h>
#include <kstandarddirs.h>
#include <kpushbutton.h>
#include <ktextbrowser.h>
#include <kiconloader.h>
#include <ktabwidget.h>
#include <kimageio.h>
#include <khelpmenu.h>
#include <kfiledialog.h>
#include <KMenu>
#include <KXMLGUIFactory>
#include <KMultiTabBar>

#include <db/connection.h>
#include <db/utils.h>
#include <db/cursor.h>
#include <db/admin.h>
#include <kexidb/dbobjectnamevalidator.h>
#include <kexiutils/utils.h>

#include <core/KexiWindow.h>
#include <core/KexiRecentProjects.h>

#include "kexiactionproxy.h"
#include "kexipartmanager.h"
#include "kexipart.h"
#include "kexipartinfo.h"
#include "kexipartguiclient.h"
#include "kexiproject.h"
#include "kexiprojectdata.h"
#include "kexi.h"
#include "kexistatusbar.h"
#include "kexiinternalpart.h"
#include "kexiactioncategories.h"
#include "kexifinddialog.h"
#include "kexisearchandreplaceiface.h"
#include <kexi_global.h>

#include <widget/properties/KexiPropertyEditorView.h>
#include <widget/utils/kexirecordnavigator.h>
#include <widget/utils/KexiDockableWidget.h>
#include <widget/navigator/KexiProjectNavigator.h>
#include <widget/navigator/KexiProjectModel.h>
#include <widget/KexiFileWidget.h>
#include <koproperty/EditorView.h>
#include <koproperty/Set.h>

#include "startup/KexiStartup.h"
#include "startup/KexiNewProjectAssistant.h"
#include "startup/KexiOpenProjectAssistant.h"
#include "startup/KexiWelcomeAssistant.h"
#include "startup/KexiImportExportAssistant.h"
#include "startup/KexiStartupDialog.h"
#include "kexinamedialog.h"

#include <KoIcon.h>

//2.x #include "printing/kexisimpleprintingpart.h"
//2.x #include "printing/kexisimpleprintingpagesetup.h"

#if !defined(KexiVDebug)
# define KexiVDebug if (0) kDebug()
#endif

//first fix the geometry
//#define KEXI_NO_CTXT_HELP 1

#ifdef HAVE_KNEWSTUFF
#include <knewstuff/downloaddialog.h>
#include "kexinewstuff.h"
#endif

//temporary fix to manage layout
//2.0: #include "ksplitter.h"
//2.0: #define KDOCKWIDGET_P 1

#ifndef KEXI_NO_FEEDBACK_AGENT
#ifdef FEEDBACK_INCLUDE
#include FEEDBACK_INCLUDE
#endif
#include <kaboutdata.h>
#include <ktoolinvocation.h>
#endif

//-------------------------------------------------

//! @internal
class KexiDockWidget : public QDockWidget
{
public:
    KexiDockWidget(const QString & title, QWidget *parent)
            : QDockWidget(title, parent) {
    }
    void setSizeHint(const QSize& hint) {
        m_hint = hint;
    }
    QSize sizeHint() const {
        return m_hint.isValid() ? m_hint : QDockWidget::sizeHint();
    }
private:
    QSize m_hint;
};

//-------------------------------------------------

#include "KexiMainWindow_p.h"

//-------------------------------------------------

KexiMainWindowTabWidget::KexiMainWindowTabWidget(QWidget *parent, KexiMainWidget* mainWidget)
        : KTabWidget(parent)
        , m_mainWidget(mainWidget)
        , m_tabIndex(-1)
{
    m_closeAction = new KAction(koIcon("tab-close"), i18n("&Close Tab"), this);
    m_closeAction->setToolTip(i18n("Close the current tab"));
    m_closeAction->setWhatsThis(i18n("Closes the current tab."));
    connect(m_closeAction, SIGNAL(triggered()), this, SLOT(closeTab()));
//! @todo  insert window list in the corner widget as in firefox
#if 0
    // close-tab button:
    QToolButton* rightWidget = new QToolButton(this);
    rightWidget->setDefaultAction(m_closeAction);
    rightWidget->setText(QString());
    rightWidget->setAutoRaise(true);
    rightWidget->adjustSize();
    setCornerWidget(rightWidget, Qt::TopRightCorner);
#endif
    setMovable(true);
    setDocumentMode(true);
    tabBar()->setExpanding(true);
}

KexiMainWindowTabWidget::~KexiMainWindowTabWidget()
{
}

void KexiMainWindowTabWidget::paintEvent(QPaintEvent * event)
{
    if (count() > 0)
        KTabWidget::paintEvent(event);
    else
        QWidget::paintEvent(event);
}

void KexiMainWindowTabWidget::closeTab()
{
    dynamic_cast<KexiMainWindow*>(KexiMainWindowIface::global())->closeWindowForTab(m_tabIndex);
}

void KexiMainWindowTabWidget::tabInserted(int index)
{
    KTabWidget::tabInserted(index);
    m_mainWidget->slotCurrentTabIndexChanged(index);
}

void KexiMainWindowTabWidget::contextMenu(int index, const QPoint& point)
{
    QMenu menu;
    menu.addAction(m_closeAction);
//! @todo add "&Detach Tab"
    setTabIndexFromContextMenu(index);
    menu.exec(point);
    KTabWidget::contextMenu(index, point);
}
    
void KexiMainWindowTabWidget::setTabIndexFromContextMenu(int clickedIndex)
{
    if (currentIndex() == -1) {
        m_tabIndex = -1;
        return;
    }
    m_tabIndex = clickedIndex;
}

//-------------------------------------------------

//static
/*KexiMainWindow* KexiMainWindow::self()
{
  return kexiMainWindow;
}*/

//static
int KexiMainWindow::create(int argc, char *argv[], const KexiAboutData &aboutData)
{
    Kexi::initCmdLineArgs(argc, argv, aboutData);

    bool GUIenabled = true;
//! @todo switch GUIenabled off when needed
    KApplication* app = new KApplication(GUIenabled);

    KGlobal::locale()->insertCatalog("calligra");
    KGlobal::locale()->insertCatalog("koproperty");

    tristate res = Kexi::startupHandler().init(argc, argv);
    if (!res || ~res) {
        delete app;
        return (~res) ? 0 : 1;
    }

    kDebug() << "startupActions OK";

    /* Exit requested, e.g. after database removing. */
    if (Kexi::startupHandler().action() == KexiStartupData::Exit) {
        delete app;
        return 0;
    }

    KexiMainWindow *win = new KexiMainWindow();
#ifdef KEXI_DEBUG_GUI
    QWidget* debugWindow = 0;
    if (GUIenabled) {
        KConfigGroup generalGroup = KGlobal::config()->group("General");
        if (generalGroup.readEntry("ShowInternalDebugger", false)) {
            debugWindow = KexiUtils::createDebugWindow(win);
            debugWindow->show();
        }
    }
#endif

#ifndef KEXI_MOBILE
    //QApplication::setMainWidget(win);
#endif

    if (true != win->startup()) {
        delete win;
        delete app;
        return 1;
    }

    //app->processEvents();//allow refresh our app
    win->restoreSettings();
    win->show();
#ifdef KEXI_DEBUG_GUI
    win->raise();
    static_cast<QWidget*>(win)->activateWindow();
#endif
    /*foreach (QWidget *widget, QApplication::topLevelWidgets()) {
        kDebug() << widget;
    }*/
    return 0;
}

//-------------------------------------------------

KexiMainWindow::KexiMainWindow(QWidget *parent)
        : KexiMainWindowSuper(parent)
        , KexiMainWindowIface()
        , KexiGUIMessageHandler(this)
        , d(new KexiMainWindow::Private(this))
{
    setObjectName("KexiMainWindow");

//kde4: removed  KImageIO::registerFormats();

    if (d->userMode)
        kDebug() << "KexiMainWindow::KexiMainWindow(): starting up in the User Mode";

    setAsDefaultHost(); //this is default host now.
    KIconLoader *globalIconLoader = KIconLoader::global();
    globalIconLoader->addAppDir(QLatin1String("kexi"));
    globalIconLoader->addAppDir(QLatin1String("calligra"));

    //get informed
    connect(&Kexi::partManager(), SIGNAL(partLoaded(KexiPart::Part*)),
            this, SLOT(slotPartLoaded(KexiPart::Part*)));
    connect(&Kexi::partManager(), SIGNAL(newObjectRequested(KexiPart::Info*)),
            this, SLOT(newObject(KexiPart::Info*)));

    setAcceptDrops(true);
    setupActions();
    setupMainWidget();
    updateAppCaption();

    (void)KexiUtils::smallFont(this/*init*/);

    if (!d->userMode) {
        setupContextHelp();
        setupPropertyEditor();
    }

    d->tabbedToolBar->hideTab("form");//temporalily until createToolbar is split
    d->tabbedToolBar->hideTab("report");//temporalily until createToolbar is split

    invalidateActions();
    d->timer.singleShot(0, this, SLOT(slotLastActions()));
    connect(d->mainWidget, SIGNAL(currentTabIndexChanged(int)), this, SLOT(showTabIfNeeded()));
    if (Kexi::startupHandler().forcedFullScreen()) {
        toggleFullScreen(true);
    }

    // --- global config
    //! @todo move to specialized KexiConfig class
    KConfigGroup tablesGroup(d->config->group("Tables"));
    const int defaultMaxLengthForTextFields = tablesGroup.readEntry("DefaultMaxLengthForTextFields", int(-1));
    if (defaultMaxLengthForTextFields >= 0) {
        KexiDB::Field::setDefaultMaxLength(defaultMaxLengthForTextFields);
    }
    // --- /global config
}

KexiMainWindow::~KexiMainWindow()
{
    d->forceWindowClosing = true;
    closeProject();
    delete d;
}

KexiProject *KexiMainWindow::project()
{
    return d->prj;
}

KXMLGUIClient* KexiMainWindow::guiClient() const
{
    return d->dummy_KXMLGUIClient;
}

KXMLGUIFactory* KexiMainWindow::guiFactory()
{
    return d->dummy_KXMLGUIFactory;
}

QList<QAction*> KexiMainWindow::allActions() const
{
    return actionCollection()->actions();
}

KActionCollection *KexiMainWindow::actionCollection() const
{
    return d->actionCollection;
}

KexiWindow* KexiMainWindow::currentWindow() const
{
    return windowForTab(d->mainWidget->tabWidget()->currentIndex());
}

KexiWindow* KexiMainWindow::windowForTab(int tabIndex) const
{
    if (!d->mainWidget->tabWidget())
        return 0;
    KexiWindowContainer *windowContainer
        = dynamic_cast<KexiWindowContainer*>(d->mainWidget->tabWidget()->widget(tabIndex));
    if (!windowContainer)
        return 0;
    return windowContainer->window;
}

void KexiMainWindow::setupMainMenuActionShortcut(KAction* action, const char* slot)
{
    if (!action->shortcut().isEmpty()) {
        (void)new QShortcut(action->shortcut().primary(), this, slot);
        (void)new QShortcut(action->shortcut().alternate(), this, slot);
    }
}

static void addThreeDotsToActionText(QAction* action)
{
    action->setText(i18nc("Action name with three dots...", "%1...", action->text()));
}

KAction* KexiMainWindow::addAction(const char *name, const KIcon &icon, const QString& text,
                                   const char *shortcut)
{
    KAction *action = icon.isNull() ? new KAction(text, this) : new KAction(icon, text, this);
    actionCollection()->addAction(name, action);
    if (shortcut) {
        action->setShortcut(QKeySequence(shortcut));
        QShortcut *s = new QShortcut(action->shortcut().primary(), this);
        connect(s, SIGNAL(activated()), action, SLOT(trigger()));
    }
    return action;
}

KAction* KexiMainWindow::addAction(const char *name, const QString& text, const char *shortcut)
{
    return addAction(name, KIcon(), text, shortcut);
}

void KexiMainWindow::setupActions()
{
    //kde4
#ifdef __GNUC__
#warning TODO setupGUI(KMainWindow::Keys|KMainWindow::StatusBar|KMainWindow::Save|KMainWindow::Create, "kexiui.rc" );
#else
#pragma WARNING( TODO setupGUI(KMainWindow::Keys|KMainWindow::StatusBar|KMainWindow::Save|KMainWindow::Create, "kexiui.rc" ); )
#endif

// d->actionMapper = new QSignalMapper(this, "act_map");
// connect(d->actionMapper, SIGNAL(mapped(const QString &)), this, SLOT(slotAction(const QString &)));

    KActionCollection *ac = actionCollection();

    // PROJECT MENU
    KAction *action;
    
    ac->addAction("project_new",
        action = new KexiMenuWidgetAction(KStandardAction::New, this));
    addThreeDotsToActionText(action);
    action->setShortcut(KStandardShortcut::openNew());
    action->setToolTip(i18n("Create a new project"));
    action->setWhatsThis(
        i18n("Creates a new project. Currently opened project is not affected."));
    connect(action, SIGNAL(triggered()), this, SLOT(slotProjectNew()));
    setupMainMenuActionShortcut(action, SLOT(slotProjectNew()));

    ac->addAction("project_open",
            action = new KexiMenuWidgetAction(KStandardAction::Open, this));
    action->setToolTip(i18n("Open an existing project"));
    action->setWhatsThis(
        i18n("Opens an existing project. Currently opened project is not affected."));
    connect(action, SIGNAL(triggered()), this, SLOT(slotProjectOpen()));
    setupMainMenuActionShortcut(action, SLOT(slotProjectOpen()));

#ifdef HAVE_KNEWSTUFF
    action = addAction("project_download_examples", koIcon("go-down"),
                       i18n("&Download Example Databases..."));
    action->setToolTip(i18n("Download example databases from the Internet"));
    action->setWhatsThis(i18n("Downloads example databases from the Internet."));
    connect(action, SIGNAL(triggered()), this, SLOT(slotGetNewStuff()));
#endif

    {
        ac->addAction("project_welcome",
            action = d->action_project_welcome = new KexiMenuWidgetAction(
                KIcon(), i18n("Welcome"), this));
            addThreeDotsToActionText(action);
        connect(action, SIGNAL(triggered()), this, SLOT(slotProjectWelcome()));
        setupMainMenuActionShortcut(action, SLOT(slotProjectWelcome()));
        action->setToolTip(i18n("Show Welcome page"));
        action->setWhatsThis(
            i18n("Shows Welcome page with list of recently opened projects and other information. "));
    }

    ac->addAction("project_save",
                  d->action_save = KStandardAction::save(this, SLOT(slotProjectSave()), this));
    d->action_save->setToolTip(i18n("Save object changes"));
    d->action_save->setWhatsThis(i18n("Saves object changes from currently selected window."));

#ifdef KEXI_SHOW_UNIMPLEMENTED
    d->action_save_as = addAction("project_saveas", koIcon("document-save-as"),
                                  i18n("Save &As..."));
    d->action_save_as->setToolTip(i18n("Save object as"));
    d->action_save_as->setWhatsThis(
        i18n("Saves object changes from currently selected window under a new name "
             "(within the same project)."));
    connect(d->action_save_as, SIGNAL(triggered()), this, SLOT(slotProjectSaveAs()));

    ac->addAction("project_properties",
        action = d->action_project_properties = new KexiMenuWidgetAction(
            koIcon("document-properties"), i18n("Project Properties"), this));
    connect(action, SIGNAL(triggered()), this, SLOT(slotProjectProperties()));
    setupMainMenuActionShortcut(action, SLOT(slotProjectProperties()));
#else
    d->action_save_as = d->dummy_action;
    d->action_project_properties = d->dummy_action;
#endif

#ifdef __GNUC__
#warning replace document-import icon with something other
#else
#pragma WARNING( replace document-import icon with something other )
#endif
    ac->addAction("project_import_export_send",
        action = d->action_project_import_export_send = new KexiMenuWidgetAction(
            koIcon("document-import"), i18n("&Import, Export or Send..."), this));
    action->setToolTip(i18n("Import, export or send project"));
    action->setWhatsThis(
        i18n("Imports, exports or sends project."));
    connect(action, SIGNAL(triggered()), this, SLOT(slotProjectImportExportOrSend()));
    setupMainMenuActionShortcut(action, SLOT(slotProjectImportExportOrSend()));

    ac->addAction("project_close",
        action = d->action_close = new KexiMenuWidgetAction(
            koIcon("window-close"), i18nc("Close Project", "&Close"), this));
    action->setToolTip(i18n("Close the current project"));
    action->setWhatsThis(i18n("Closes the current project."));
    connect(action, SIGNAL(triggered()), this, SLOT(slotProjectClose()));
    setupMainMenuActionShortcut(action, SLOT(slotProjectClose()));

    ac->addAction("quit",
                  action = new KexiMenuWidgetAction(KStandardAction::Quit, this));
    connect(action, SIGNAL(triggered()), this, SLOT(slotProjectQuit()));
    action->setWhatsThis(i18n("Quits Kexi application."));
    setupMainMenuActionShortcut(action, SLOT(slotProjectQuit()));

#ifdef KEXI_SHOW_UNIMPLEMENTED
    d->action_project_relations = addAction("project_relations", koIcon("relation"),
                                            i18n("&Relationships..."), "Ctrl+R");
    d->action_project_relations->setToolTip(i18n("Project relationships"));
    d->action_project_relations->setWhatsThis(i18n("Shows project relationships."));
    connect(d->action_project_relations, SIGNAL(triggered()),
            this, SLOT(slotProjectRelations()));

#else
    d->action_project_relations = d->dummy_action;
#endif
    d->action_tools_import_project = addAction("tools_import_project", koIcon("document_import_database"),
                                               i18n("&Import Database..."));
    d->action_tools_import_project->setToolTip(i18n("Import entire database as a Kexi project"));
    d->action_tools_import_project->setWhatsThis(
        i18n("Imports entire database as a Kexi project."));
    connect(d->action_tools_import_project, SIGNAL(triggered()),
            this, SLOT(slotToolsImportProject()));

    d->action_tools_data_import = addAction("tools_import_tables", koIcon("document-import"),
                                            i18n("Import Tables"));
    d->action_tools_data_import->setToolTip(i18n("Import data from an external source into this database"));
    d->action_tools_data_import->setWhatsThis(i18n("Import data from an external source into this database"));
    connect(d->action_tools_data_import, SIGNAL(triggered()), this, SLOT(slotToolsImportTables()));

    d->action_tools_compact_database = addAction("tools_compact_database",
//! @todo icon
                                                 koIcon("kexi"), i18n("&Compact Database..."));
    d->action_tools_compact_database->setToolTip(i18n("Compact the current database project"));
    d->action_tools_compact_database->setWhatsThis(
        i18n("Compacts the current database project, so it will take less space and work faster."));
    connect(d->action_tools_compact_database, SIGNAL(triggered()),
            this, SLOT(slotToolsCompactDatabase()));

    if (d->userMode)
        d->action_project_import_data_table = 0;
    else {
        d->action_project_import_data_table = addAction("project_import_data_table",
            koIcon("table"), 
            /*! @todo: change to "file_import" with a table or so */
            i18nc("Import->Table Data From File...", "Import Data From &File..."));
        d->action_project_import_data_table->setToolTip(i18n("Import table data from a file"));
        d->action_project_import_data_table->setWhatsThis(i18n("Imports table data from a file."));
        connect(d->action_project_import_data_table, SIGNAL(triggered()),
                this, SLOT(slotProjectImportDataTable()));
    }

    d->action_project_export_data_table = addAction("project_export_data_table",
        koIcon("table"),
        /*! @todo: change to "file_export" with a table or so */
        i18nc("Export->Table or Query Data to File...", "Export Data to &File..."));
    d->action_project_export_data_table->setToolTip(
        i18n("Export data from the active table or query to a file"));
    d->action_project_export_data_table->setWhatsThis(
        i18n("Exports data from the active table or query to a file."));
    connect(d->action_project_export_data_table, SIGNAL(triggered()),
            this, SLOT(slotProjectExportDataTable()));

//TODO new KAction(i18n("From File..."), "document-open", 0,
//TODO  this, SLOT(slotImportFile()), actionCollection(), "project_import_file");
//TODO new KAction(i18n("From Server..."), "network-server-database", 0,
//TODO  this, SLOT(slotImportServer()), actionCollection(), "project_import_server");

#ifndef KEXI_NO_QUICK_PRINTING
    ac->addAction("project_print",
                  d->action_project_print = KStandardAction::print(this, SLOT(slotProjectPrint()), this));
    d->action_project_print->setToolTip(i18n("Print data from the active table or query"));
    d->action_project_print->setWhatsThis(i18n("Prints data from the active table or query."));

    ac->addAction("project_print_preview",
                  d->action_project_print_preview = KStandardAction::printPreview(
                                                        this, SLOT(slotProjectPrintPreview()), this));
    d->action_project_print_preview->setToolTip(
        i18n("Show print preview for the active table or query"));
    d->action_project_print_preview->setWhatsThis(
        i18n("Shows print preview for the active table or query."));

    d->action_project_print_setup = addAction("project_print_setup",
        koIcon("document-page-setup"), i18n("Page Set&up..."));
    d->action_project_print_setup->setToolTip(
        i18n("Show page setup for printing the active table or query"));
    d->action_project_print_setup->setWhatsThis(
        i18n("Shows page setup for printing the active table or query."));
    connect(d->action_project_print_setup, SIGNAL(triggered()),
            this, SLOT(slotProjectPageSetup()));
#endif

    //EDIT MENU
    d->action_edit_cut = createSharedAction(KStandardAction::Cut, "edit_cut");
    d->action_edit_copy = createSharedAction(KStandardAction::Copy, "edit_copy");
    d->action_edit_paste = createSharedAction(KStandardAction::Paste, "edit_paste");

    if (d->userMode)
        d->action_edit_paste_special_data_table = 0;
    else {
        d->action_edit_paste_special_data_table = addAction(
            "edit_paste_special_data_table",
            koIcon("table"), i18nc("Paste Special->As Data &Table...", "Paste Special..."));
        d->action_edit_paste_special_data_table->setToolTip(
            i18n("Paste clipboard data as a table"));
        d->action_edit_paste_special_data_table->setWhatsThis(
            i18n("Pastes clipboard data to a table."));
        connect(d->action_edit_paste_special_data_table, SIGNAL(triggered()),
                this, SLOT(slotEditPasteSpecialDataTable()));
    }

    d->action_edit_copy_special_data_table = addAction(
        "edit_copy_special_data_table",
        koIcon("table"), i18nc("Copy Special->Table or Query Data...", "Copy Special..."));
    d->action_edit_copy_special_data_table->setToolTip(
        i18n("Copy selected table or query data to clipboard"));
    d->action_edit_copy_special_data_table->setWhatsThis(
        i18n("Copies selected table or query data to clipboard."));
    connect(d->action_edit_copy_special_data_table, SIGNAL(triggered()),
            this, SLOT(slotEditCopySpecialDataTable()));

    d->action_edit_undo = createSharedAction(KStandardAction::Undo, "edit_undo");
    d->action_edit_undo->setWhatsThis(i18n("Reverts the most recent editing action."));
    d->action_edit_redo = createSharedAction(KStandardAction::Redo, "edit_redo");
    d->action_edit_redo->setWhatsThis(i18n("Reverts the most recent undo action."));

#if 0 //old
    d->action_edit_find = createSharedAction(KStandardAction::Find, "edit_find");
    d->action_edit_findnext = createSharedAction(KStandardAction::FindNext, "edit_findnext");
    d->action_edit_findprev = createSharedAction(KStandardAction::FindPrev, "edit_findprevious");
//! @todo d->action_edit_paste = createSharedAction( KStandardAction::Replace, "edit_replace");
#endif

    ac->addAction("edit_find",
                  d->action_edit_find = KStandardAction::find(
                                            this, SLOT(slotEditFind()), this));
    d->action_edit_find->setToolTip(i18n("Find text"));
    d->action_edit_find->setWhatsThis(i18n("Looks up the first occurrence of a piece of text."));
// d->action_edit_find = createSharedAction( KStandardAction::Find, "edit_find");
    ac->addAction("edit_findnext",
                  d->action_edit_findnext = KStandardAction::findNext(
                                                this, SLOT(slotEditFindNext()), this));
    ac->addAction("edit_findprevious",
                  d->action_edit_findprev = KStandardAction::findPrev(
                                                this, SLOT(slotEditFindPrevious()), this));
    d->action_edit_replace = 0;
//! @todo d->action_edit_replace = KStandardAction::replace(
//!  this, SLOT(slotEditReplace()), actionCollection(), "project_print_preview" );
    d->action_edit_replace_all = 0;
//! @todo d->action_edit_replace_all = new KAction( i18n("Replace All"), "", 0,
//!   this, SLOT(slotEditReplaceAll()), actionCollection(), "edit_replaceall");

    d->action_edit_select_all =  createSharedAction(KStandardAction::SelectAll,
                                 "edit_select_all");

    d->action_edit_delete = createSharedAction(i18n("&Delete"), "edit-delete",
                            KShortcut(), "edit_delete");
    d->action_edit_delete->setToolTip(i18n("Delete selected object"));
    d->action_edit_delete->setWhatsThis(i18n("Deletes currently selected object."));

    d->action_edit_delete_row = createSharedAction(i18n("Delete Record"), "delete_table_row",
                                KShortcut(Qt::CTRL + Qt::Key_Delete), "edit_delete_row");
    d->action_edit_delete_row->setToolTip(i18n("Delete currently selected record"));
    d->action_edit_delete_row->setWhatsThis(i18n("Deletes currently selected record."));

    d->action_edit_clear_table = createSharedAction(i18n("Clear Table Contents"),
                                 "clear_table_contents", KShortcut(), "edit_clear_table");
    d->action_edit_clear_table->setToolTip(i18n("Clear table contents"));
    d->action_edit_clear_table->setWhatsThis(i18n("Clears table contents."));
    setActionVolatile(d->action_edit_clear_table, true);

    d->action_edit_edititem = createSharedAction(i18n("Edit Item"), 0,
                              KShortcut(), /* CONFLICT in TV: Qt::Key_F2,  */
                              "edit_edititem");
    d->action_edit_edititem->setToolTip(i18n("Edit currently selected item"));
    d->action_edit_edititem->setWhatsThis(i18n("Edits currently selected item."));

    d->action_edit_insert_empty_row = createSharedAction(i18n("&Insert Empty Row"),
                                      "insert_table_row", KShortcut(Qt::SHIFT | Qt::CTRL | Qt::Key_Insert),
                                      "edit_insert_empty_row");
    setActionVolatile(d->action_edit_insert_empty_row, true);
    d->action_edit_insert_empty_row->setToolTip(i18n("Insert one empty row above"));
    d->action_edit_insert_empty_row->setWhatsThis(
        i18n("Inserts one empty row above currently selected table row."));

    //VIEW MENU
    /* UNUSED, see KexiToggleViewModeAction
      if (!d->userMode) {
        d->action_view_mode = new QActionGroup(this);
        ac->addAction( "view_data_mode",
          d->action_view_data_mode = new KToggleAction(
            koIcon("state_data"), i18n("&Data View"), d->action_view_mode) );
    //  d->action_view_data_mode->setObjectName("view_data_mode");
        d->action_view_data_mode->setShortcut(QKeySequence("F6"));
        //d->action_view_data_mode->setExclusiveGroup("view_mode");
        d->action_view_data_mode->setToolTip(i18n("Switch to data view"));
        d->action_view_data_mode->setWhatsThis(i18n("Switches to data view."));
        d->actions_for_view_modes.insert( Kexi::DataViewMode, d->action_view_data_mode );
        connect(d->action_view_data_mode, SIGNAL(triggered()),
          this, SLOT(slotViewDataMode()));
      }
      else {
        d->action_view_mode = 0;
        d->action_view_data_mode = 0;
      }

      if (!d->userMode) {
        ac->addAction( "view_design_mode",
          d->action_view_design_mode = new KToggleAction(
            koIcon("state_edit"), i18n("D&esign View"), d->action_view_mode) );
    //  d->action_view_design_mode->setObjectName("view_design_mode");
        d->action_view_design_mode->setShortcut(QKeySequence("F7"));
        //d->action_view_design_mode->setExclusiveGroup("view_mode");
        d->action_view_design_mode->setToolTip(i18n("Switch to design view"));
        d->action_view_design_mode->setWhatsThis(i18n("Switches to design view."));
        d->actions_for_view_modes.insert( Kexi::DesignViewMode, d->action_view_design_mode );
        connect(d->action_view_design_mode, SIGNAL(triggered()),
          this, SLOT(slotViewDesignMode()));
      }
      else
        d->action_view_design_mode = 0;

      if (!d->userMode) {
        ac->addAction( "view_text_mode",
          d->action_view_text_mode = new KToggleAction(
            koIcon("state_sql"), i18n("&Text View"), d->action_view_mode) );
        d->action_view_text_mode->setObjectName("view_text_mode");
        d->action_view_text_mode->setShortcut(QKeySequence("F8"));
        //d->action_view_text_mode->setExclusiveGroup("view_mode");
        d->action_view_text_mode->setToolTip(i18n("Switch to text view"));
        d->action_view_text_mode->setWhatsThis(i18n("Switches to text view."));
        d->actions_for_view_modes.insert( Kexi::TextViewMode, d->action_view_text_mode );
        connect(d->action_view_text_mode, SIGNAL(triggered()),
          this, SLOT(slotViewTextMode()));
      }
      else
        d->action_view_text_mode = 0;
    */
    if (d->isProjectNavigatorVisible) {
        d->action_view_nav = addAction("view_navigator",
                                       i18n("Switch to Project Navigator"),
                                       "Alt+1");
        d->action_view_nav->setToolTip(i18n("Switch to Project Navigator panel"));
        d->action_view_nav->setWhatsThis(i18n("Switches to Project Navigator panel."));
        connect(d->action_view_nav, SIGNAL(triggered()),
                this, SLOT(slotViewNavigator()));
    } else
        d->action_view_nav = 0;

    d->action_view_mainarea = addAction("view_mainarea",
                                        i18n("Switch to Main Area"), "Alt+2");
    d->action_view_mainarea->setToolTip(i18n("Switch to main area"));
    d->action_view_mainarea->setWhatsThis(i18n("Switches to main area."));
    connect(d->action_view_mainarea, SIGNAL(triggered()),
            this, SLOT(slotViewMainArea()));

    if (!d->userMode) {
        d->action_view_propeditor = addAction("view_propeditor",
                                              i18n("Switch to Property Editor"), "Alt+3");
        d->action_view_propeditor->setToolTip(i18n("Switch to Property Editor panel"));
        d->action_view_propeditor->setWhatsThis(i18n("Switches to Property Editor panel."));
        connect(d->action_view_propeditor, SIGNAL(triggered()),
                this, SLOT(slotViewPropertyEditor()));
    } else
        d->action_view_propeditor = 0;

    d->action_view_global_search = addAction("view_global_search",
                                             i18n("Switch to Global Search"), "Ctrl+K");
    d->action_view_global_search->setToolTip(i18n("Switch to Global Search box"));
    d->action_view_global_search->setWhatsThis(i18n("Switches to Global Search box."));
    // (connection is added elsewhere)

    //DATA MENU
    d->action_data_save_row = createSharedAction(i18n("&Save Record"), "dialog-ok",
                              KShortcut(Qt::SHIFT + Qt::Key_Return), "data_save_row");
    d->action_data_save_row->setToolTip(i18n("Save changes made to the current record"));
    d->action_data_save_row->setWhatsThis(i18n("Saves changes made to the current record."));
//temp. disable because of problems with volatile actions setActionVolatile( d->action_data_save_row, true );

    d->action_data_cancel_row_changes = createSharedAction(i18n("&Cancel Record Changes"),
                                        "dialog-cancel", KShortcut(), "data_cancel_row_changes");
    d->action_data_cancel_row_changes->setToolTip(
        i18n("Cancel changes made to the current record"));
    d->action_data_cancel_row_changes->setWhatsThis(
        i18n("Cancels changes made to the current record."));
//temp. disable because of problems with volatile actions setActionVolatile( d->action_data_cancel_row_changes, true );

    d->action_data_execute = createSharedAction(
                                 i18n("&Execute"), "media-playback-start", KShortcut(), "data_execute");
    //d->action_data_execute->setToolTip(i18n("")); //TODO
    //d->action_data_execute->setWhatsThis(i18n("")); //TODO

#ifndef KEXI_SHOW_UNIMPLEMENTED
    action = createSharedAction(i18n("&Filter"), "view-filter", KShortcut(), "data_filter");
    setActionVolatile(action, true);
#endif
// action->setToolTip(i18n("")); //todo
// action->setWhatsThis(i18n("")); //todo

// setSharedMenu("data_sort");
    /* moved to KexiStandardAction
      action = createSharedAction(i18n("&Ascending"), "sort_az", KShortcut(), "data_sort_az");
    //temp. disable because of problems with volatile actions setActionVolatile( action, true );
      action->setToolTip(i18n("Sort data in ascending order"));
      action->setWhatsThis(i18n("Sorts data in ascending order (from A to Z and from 0 to 9). Data from selected column is used for sorting."));

      action = createSharedAction(i18n("&Descending"), "sort_za", KShortcut(), "data_sort_za");
    //temp. disable because of problems with volatile actions setActionVolatile( action, true );
      action->setToolTip(i18n("Sort data in descending order"));
      action->setWhatsThis(i18n("Sorts data in descending (from Z to A and from 9 to 0). Data from selected column is used for sorting."));
    */
    // - record-navigation related actions
    createSharedAction(KexiRecordNavigator::Actions::moveToFirstRecord(), KShortcut(), "data_go_to_first_record");
    createSharedAction(KexiRecordNavigator::Actions::moveToPreviousRecord(), KShortcut(), "data_go_to_previous_record");
    createSharedAction(KexiRecordNavigator::Actions::moveToNextRecord(), KShortcut(), "data_go_to_next_record");
    createSharedAction(KexiRecordNavigator::Actions::moveToLastRecord(), KShortcut(), "data_go_to_last_record");
    createSharedAction(KexiRecordNavigator::Actions::moveToNewRecord(), KShortcut(), "data_go_to_new_record");

    //FORMAT MENU
    d->action_format_font = createSharedAction(i18n("&Font..."), "fonts",
                            KShortcut(), "format_font");
    d->action_format_font->setToolTip(i18n("Change font for selected object"));
    d->action_format_font->setWhatsThis(i18n("Changes font for selected object."));

    //TOOLS MENU

    //WINDOW MENU
    //additional 'Window' menu items
    d->action_window_next = addAction("window_next",
                                      i18n("&Next Window"), 
#ifdef Q_WS_WIN
        "Ctrl+Tab"
#else
        "Alt+Right"
#endif
    );
    d->action_window_next->setToolTip(i18n("Next window"));
    d->action_window_next->setWhatsThis(i18n("Switches to the next window."));
    connect(d->action_window_next, SIGNAL(triggered()),
            this, SLOT(activateNextWindow()));

    d->action_window_previous = addAction("window_previous",
                                          i18n("&Previous Window"),
#ifdef Q_WS_WIN
        "Ctrl+Shift+Tab"
#else
        "Alt+Left"
#endif
    );
    d->action_window_previous->setToolTip(i18n("Previous window"));
    d->action_window_previous->setWhatsThis(i18n("Switches to the previous window."));
    connect(d->action_window_previous, SIGNAL(triggered()),
            this, SLOT(activatePreviousWindow()));

    d->action_window_fullscreen = KStandardAction::fullScreen(this, SLOT(toggleFullScreen(bool)), this, ac);
    ac->addAction("full_screen", d->action_window_fullscreen);
    QList<QKeySequence> shortcuts;
    KShortcut *shortcut = new KShortcut(d->action_window_fullscreen->shortcut().primary(), QKeySequence("F11"));
    shortcuts = shortcut->toList();
    d->action_window_fullscreen->setShortcuts(shortcuts);
    QShortcut *s = new QShortcut(d->action_window_fullscreen->shortcut().primary(), this);
    connect(s, SIGNAL(activated()), d->action_window_fullscreen, SLOT(trigger()));
    QShortcut *sa = new QShortcut(d->action_window_fullscreen->shortcut().alternate(), this);
    connect(sa, SIGNAL(activated()), d->action_window_fullscreen, SLOT(trigger()));

    //SETTINGS MENU
#ifdef __GNUC__
#warning put 'configure keys' into settings view
#else
#pragma WARNING( put 'configure keys' into settings view )
#endif

#if 0 // moved to settings
    action = KStandardAction::keyBindings(this, SLOT(slotConfigureKeys()), this);
    ac->addAction(action->objectName(), action);
    action->setWhatsThis(i18n("Lets you configure shortcut keys."));
#endif

#ifdef KEXI_SHOW_UNIMPLEMENTED
    /*! @todo 2.0 - toolbars configuration will be handled in a special way
      action = KStandardAction::configureToolbars( this, SLOT( slotConfigureToolbars() ),
        actionCollection() );
      action->setWhatsThis(i18n("Lets you configure toolbars."));

      d->action_show_other = new KActionMenu(i18n("Other"),
        actionCollection(), "options_show_other");
        */
#endif

#ifdef KEXI_MACROS_SUPPORT
    Kexi::tempShowMacros() = true;
#else
    Kexi::tempShowMacros() = false;
#endif

#ifdef KEXI_SCRIPTS_SUPPORT
    Kexi::tempShowScripts() = true;
#else
    Kexi::tempShowScripts() = false;
#endif

#ifdef KEXI_SHOW_UNIMPLEMENTED
//! @todo 2.0 - implement settings window in a specific way
    ac->addAction("settings",
                  action = d->action_settings = new KexiMenuWidgetAction(
                    KStandardAction::Preferences, this));
    action->setObjectName("settings");
    action->setText(i18n("Settings..."));
    action->setToolTip(i18n("Kexi settings"));
    action->setWhatsThis(i18n("Lets you to view and change Kexi settings."));
    connect(action, SIGNAL(triggered()), this, SLOT(slotSettings()));
    setupMainMenuActionShortcut(action, SLOT(slotSettings()));
#else
    d->action_settings = d->dummy_action;
#endif

//! @todo reenable 'tip of the day' later
#if 0
    KStandardAction::tipOfDay(this, SLOT(slotTipOfTheDayAction()), actionCollection())
    ->setWhatsThis(i18n("This shows useful tips on the use of this application."));
#endif

#ifndef KEXI_NO_FEEDBACK_AGENT
#ifdef FEEDBACK_CLASS
    action = addAction("help_start_feedback_agent", koIcon("dialog-information"),
                       i18n("Give Feedback..."));
    connect(action, SIGNAL(triggered()),
            this, SLOT(slotStartFeedbackAgent()));
#endif
#endif

    // GLOBAL
    d->action_show_help_menu = addAction("help_show_menu", i18n("Show Help Menu"), "Alt+H");
    d->action_show_help_menu->setToolTip(i18n("Show Help menu"));
    d->action_show_help_menu->setWhatsThis(i18n("Shows Help menu."));
    // (connection is added elsewhere)

// KAction *actionSettings = new KAction(i18n("Configure Kexi..."), "configure", 0,
//  actionCollection(), "kexi_settings");
// actionSettings->setWhatsThis(i18n("Lets you configure Kexi."));
// connect(actionSettings, SIGNAL(activated()), this, SLOT(slotSettings()));

    // ----- declare action categories, so form's "assign action to button"
    //       (and macros in the future) will be able to recognize category
    //       of actions and filter them -----------------------------------
//! @todo shouldn't we move this to core?
    Kexi::ActionCategories *acat = Kexi::actionCategories();
    acat->addAction("data_execute", Kexi::PartItemActionCategory);

    //! @todo unused for now
    acat->addWindowAction("data_filter",
                          KexiPart::TableObjectType, KexiPart::QueryObjectType, KexiPart::FormObjectType);

    acat->addWindowAction("data_save_row",
                          KexiPart::TableObjectType, KexiPart::QueryObjectType, KexiPart::FormObjectType);

    acat->addWindowAction("data_cancel_row_changes",
                          KexiPart::TableObjectType, KexiPart::QueryObjectType, KexiPart::FormObjectType);

    acat->addWindowAction("delete_table_row",
                          KexiPart::TableObjectType, KexiPart::QueryObjectType, KexiPart::FormObjectType);

    //! @todo support this in KexiPart::FormObjectType as well
    acat->addWindowAction("data_sort_az",
                          KexiPart::TableObjectType, KexiPart::QueryObjectType);

    //! @todo support this in KexiPart::FormObjectType as well
    acat->addWindowAction("data_sort_za",
                          KexiPart::TableObjectType, KexiPart::QueryObjectType);

    //! @todo support this in KexiPart::FormObjectType as well
    acat->addWindowAction("edit_clear_table",
                          KexiPart::TableObjectType, KexiPart::QueryObjectType);

    //! @todo support this in KexiPart::FormObjectType as well
    acat->addWindowAction("edit_copy_special_data_table",
                          KexiPart::TableObjectType, KexiPart::QueryObjectType);

    // GlobalActions, etc.
    acat->addAction("edit_copy", Kexi::GlobalActionCategory | Kexi::PartItemActionCategory);

    acat->addAction("edit_cut", Kexi::GlobalActionCategory | Kexi::PartItemActionCategory);

    acat->addAction("edit_paste", Kexi::GlobalActionCategory | Kexi::PartItemActionCategory);

    acat->addAction("edit_delete", Kexi::GlobalActionCategory | Kexi::PartItemActionCategory | Kexi::WindowActionCategory,
                    KexiPart::TableObjectType, KexiPart::QueryObjectType, KexiPart::FormObjectType);

    acat->addAction("edit_delete_row", Kexi::GlobalActionCategory | Kexi::WindowActionCategory,
                    KexiPart::TableObjectType, KexiPart::QueryObjectType, KexiPart::FormObjectType);

    acat->addAction("edit_edititem", Kexi::PartItemActionCategory | Kexi::WindowActionCategory,
                    KexiPart::TableObjectType, KexiPart::QueryObjectType);

    acat->addAction("edit_find", Kexi::GlobalActionCategory | Kexi::WindowActionCategory,
                    KexiPart::TableObjectType, KexiPart::QueryObjectType, KexiPart::FormObjectType);

    acat->addAction("edit_findnext", Kexi::GlobalActionCategory | Kexi::WindowActionCategory,
                    KexiPart::TableObjectType, KexiPart::QueryObjectType, KexiPart::FormObjectType);

    acat->addAction("edit_findprevious", Kexi::GlobalActionCategory | Kexi::WindowActionCategory,
                    KexiPart::TableObjectType, KexiPart::QueryObjectType, KexiPart::FormObjectType);

    acat->addAction("edit_replace", Kexi::GlobalActionCategory | Kexi::WindowActionCategory,
                    KexiPart::TableObjectType, KexiPart::QueryObjectType, KexiPart::FormObjectType);

    acat->addAction("edit_paste_special_data_table", Kexi::GlobalActionCategory);

    acat->addAction("help_about_app", Kexi::GlobalActionCategory);

    acat->addAction("help_about_kde", Kexi::GlobalActionCategory);

    acat->addAction("help_contents", Kexi::GlobalActionCategory);

    acat->addAction("help_report_bug", Kexi::GlobalActionCategory);

    acat->addAction("help_whats_this", Kexi::GlobalActionCategory);

    acat->addAction("options_configure_keybinding", Kexi::GlobalActionCategory);

    acat->addAction("project_close", Kexi::GlobalActionCategory);

    //! @todo support this in FormObjectType as well
    acat->addAction("project_export_data_table", Kexi::GlobalActionCategory | Kexi::WindowActionCategory,
                    KexiPart::TableObjectType, KexiPart::QueryObjectType);

    acat->addAction("project_import_data_table", Kexi::GlobalActionCategory);

    acat->addAction("project_new", Kexi::GlobalActionCategory);

    acat->addAction("project_open", Kexi::GlobalActionCategory);

#ifndef KEXI_NO_QUICK_PRINTING
    //! @todo support this in FormObjectType, ReportObjectType as well as others
    acat->addAction("project_print", Kexi::WindowActionCategory,
                    KexiPart::TableObjectType, KexiPart::QueryObjectType);

    //! @todo support this in FormObjectType, ReportObjectType as well as others
    acat->addAction("project_print_preview", Kexi::WindowActionCategory,
                    KexiPart::TableObjectType, KexiPart::QueryObjectType);

    //! @todo support this in FormObjectType, ReportObjectType as well as others
    acat->addAction("project_print_setup", Kexi::WindowActionCategory,
                    KexiPart::TableObjectType, KexiPart::QueryObjectType);
#endif

    acat->addAction("quit", Kexi::GlobalActionCategory);

    acat->addAction("tools_compact_database", Kexi::GlobalActionCategory);

    acat->addAction("tools_import_project", Kexi::GlobalActionCategory);

    acat->addAction("tools_import_tables", Kexi::GlobalActionCategory);
    
    acat->addAction("view_data_mode", Kexi::GlobalActionCategory);

    acat->addAction("view_design_mode", Kexi::GlobalActionCategory);

    acat->addAction("view_text_mode", Kexi::GlobalActionCategory);

    acat->addAction("view_mainarea", Kexi::GlobalActionCategory);

    acat->addAction("view_navigator", Kexi::GlobalActionCategory);

    acat->addAction("view_propeditor", Kexi::GlobalActionCategory);

    acat->addAction("window_close", Kexi::GlobalActionCategory | Kexi::WindowActionCategory);
    acat->setAllObjectTypesSupported("window_close", true);

    acat->addAction("window_next", Kexi::GlobalActionCategory);

    acat->addAction("window_previous", Kexi::GlobalActionCategory);

    //skipped - design view only
    acat->addAction("format_font", Kexi::NoActionCategory);
    acat->addAction("project_save", Kexi::NoActionCategory);
    acat->addAction("edit_insert_empty_row", Kexi::NoActionCategory);
    //! @todo support this in KexiPart::TableObjectType, KexiPart::QueryObjectType later
    acat->addAction("edit_select_all", Kexi::NoActionCategory);
    //! @todo support this in KexiPart::TableObjectType, KexiPart::QueryObjectType, KexiPart::FormObjectType later
    acat->addAction("edit_redo", Kexi::NoActionCategory);
    //! @todo support this in KexiPart::TableObjectType, KexiPart::QueryObjectType, KexiPart::FormObjectType later
    acat->addAction("edit_undo", Kexi::NoActionCategory);

    //record-navigation related actions
    acat->addAction("data_go_to_first_record", Kexi::WindowActionCategory,
                    KexiPart::TableObjectType, KexiPart::QueryObjectType, KexiPart::FormObjectType);
    acat->addAction("data_go_to_previous_record", Kexi::WindowActionCategory,
                    KexiPart::TableObjectType, KexiPart::QueryObjectType, KexiPart::FormObjectType);
    acat->addAction("data_go_to_next_record", Kexi::WindowActionCategory,
                    KexiPart::TableObjectType, KexiPart::QueryObjectType, KexiPart::FormObjectType);
    acat->addAction("data_go_to_last_record", Kexi::WindowActionCategory,
                    KexiPart::TableObjectType, KexiPart::QueryObjectType, KexiPart::FormObjectType);
    acat->addAction("data_go_to_new_record", Kexi::WindowActionCategory,
                    KexiPart::TableObjectType, KexiPart::QueryObjectType, KexiPart::FormObjectType);

    //skipped - internal:
    acat->addAction("tablepart_create", Kexi::NoActionCategory);
    acat->addAction("querypart_create", Kexi::NoActionCategory);
    acat->addAction("formpart_create", Kexi::NoActionCategory);
    acat->addAction("reportpart_create", Kexi::NoActionCategory);
    acat->addAction("macropart_create", Kexi::NoActionCategory);
    acat->addAction("scriptpart_create", Kexi::NoActionCategory);
}

void KexiMainWindow::invalidateActions()
{
    invalidateProjectWideActions();
    invalidateSharedActions();
}

void KexiMainWindow::invalidateSharedActions(QObject *o)
{
    //TODO: enabling is more complex...
    /* d->action_edit_cut->setEnabled(true);
      d->action_edit_copy->setEnabled(true);
      d->action_edit_paste->setEnabled(true);*/

    if (!o)
        o = focusWindow();
    KexiSharedActionHost::invalidateSharedActions(o);
}

void KexiMainWindow::invalidateSharedActions()
{
    invalidateSharedActions(0);
}

// unused, I think
void KexiMainWindow::invalidateSharedActionsLater()
{
    QTimer::singleShot(1, this, SLOT(invalidateSharedActions()));
}

void KexiMainWindow::invalidateProjectWideActions()
{
// stateChanged("project_opened",d->prj ? StateNoReverse : StateReverse);

    const bool has_window = currentWindow();
    const bool window_dirty = currentWindow() && currentWindow()->isDirty();
    const bool readOnly = d->prj && d->prj->dbConnection() && d->prj->dbConnection()->isReadOnly();

    //PROJECT MENU
    d->action_save->setEnabled(has_window && window_dirty && !readOnly);
    d->action_save_as->setEnabled(has_window && !readOnly);
    d->action_project_properties->setEnabled(d->prj);
    d->action_close->setEnabled(d->prj);
    d->action_project_relations->setEnabled(d->prj);

    //DATA MENU
    if (d->action_project_import_data_table)
        d->action_project_import_data_table->setEnabled(d->prj && !readOnly);
    if (d->action_tools_data_import)
        d->action_tools_data_import->setEnabled(d->prj && !readOnly);
    d->action_project_export_data_table->setEnabled(
        currentWindow() && currentWindow()->part()->info()->isDataExportSupported()
        && !currentWindow()->neverSaved());
    if (d->action_edit_paste_special_data_table)
        d->action_edit_paste_special_data_table->setEnabled(d->prj && !readOnly);

#ifndef KEXI_NO_QUICK_PRINTING
    const bool printingActionsEnabled =
        currentWindow() && currentWindow()->part()->info()->isPrintingSupported()
        && !currentWindow()->neverSaved();
    d->action_project_print->setEnabled(printingActionsEnabled);
    d->action_project_print_preview->setEnabled(printingActionsEnabled);
    d->action_project_print_setup->setEnabled(printingActionsEnabled);
#endif

    //EDIT MENU
//! @todo "copy special" is currently enabled only for data view mode;
//!  what about allowing it to enable in design view for "kexi/table" ?
    if (currentWindow() && currentWindow()->currentViewMode() == Kexi::DataViewMode) {
        KexiPart::Info *activePartInfo = currentWindow()->part()->info();
        d->action_edit_copy_special_data_table->setEnabled(
            activePartInfo ? activePartInfo->isDataExportSupported() : false);
    } else
        d->action_edit_copy_special_data_table->setEnabled(false);
    d->action_edit_find->setEnabled(d->prj);

    //VIEW MENU
    if (d->action_view_nav)
        d->action_view_nav->setEnabled(d->prj);
    d->action_view_mainarea->setEnabled(d->prj);
    if (d->action_view_propeditor)
        d->action_view_propeditor->setEnabled(d->prj);
    /* UNUSED, see KexiToggleViewModeAction
      if (d->action_view_data_mode) {
        d->action_view_data_mode->setEnabled(
          has_window && currentWindow()->supportsViewMode(Kexi::DataViewMode) );
        if (!d->action_view_data_mode->isEnabled())
          d->action_view_data_mode->setChecked(false);
      }
      if (d->action_view_design_mode) {
        d->action_view_design_mode->setEnabled(
          has_window && currentWindow()->supportsViewMode(Kexi::DesignViewMode) );
        if (!d->action_view_design_mode->isEnabled())
          d->action_view_design_mode->setChecked(false);
      }
      if (d->action_view_text_mode) {
        d->action_view_text_mode->setEnabled(
          has_window && currentWindow()->supportsViewMode(Kexi::TextViewMode) );
        if (!d->action_view_text_mode->isEnabled())
          d->action_view_text_mode->setChecked(false);
      }*/
#ifndef KEXI_NO_CTXT_HELP
    d->action_show_helper->setEnabled(d->prj);
#endif

    //CREATE MENU
    /*replaced by d->tabbedToolBar
      if (d->createMenu)
        d->createMenu->setEnabled(d->prj);*/
    if (d->tabbedToolBar && d->tabbedToolBar->createWidgetToolBar())
        d->tabbedToolBar->createWidgetToolBar()->setEnabled(d->prj);

    // DATA MENU
    //d->action_data_execute->setEnabled( currentWindow() && currentWindow()->part()->info()->isExecuteSupported() );

    //TOOLS MENU
    // "compact db" supported if there's no db or the current db supports compacting and is opened r/w:
    d->action_tools_compact_database->setEnabled(
        !d->prj
        || (!readOnly && d->prj && d->prj->dbConnection()
            && (d->prj->dbConnection()->driver()->features() & KexiDB::Driver::CompactingDatabaseSupported))
    );

    //WINDOW MENU
    if (d->action_window_next) {
#ifdef __GNUC__
#warning TODO  d->action_window_next->setEnabled(!m_pDocumentViews->isEmpty());
#else
#pragma WARNING( TODO  d->action_window_next->setEnabled(!m_pDocumentViews->isEmpty()); )
#endif
#ifdef __GNUC__
#warning TODO  d->action_window_previous->setEnabled(!m_pDocumentViews->isEmpty());
#else
#pragma WARNING( TODO  d->action_window_previous->setEnabled(!m_pDocumentViews->isEmpty()); )
#endif
    }

    //DOCKS
    if (d->navigator) {
        d->navigator->setEnabled(d->prj);
    }
    if (d->propEditor)
        d->propEditorTabWidget->setEnabled(d->prj);
}

/* UNUSED, see KexiToggleViewModeAction
void KexiMainWindow::invalidateViewModeActions()
{
  if (currentWindow()) {
    //update toggle action
    if (currentWindow()->currentViewMode()==Kexi::DataViewMode) {
      if (d->action_view_data_mode)
        d->action_view_data_mode->setChecked( true );
    }
    else if (currentWindow()->currentViewMode()==Kexi::DesignViewMode) {
      if (d->action_view_design_mode)
        d->action_view_design_mode->setChecked( true );
    }
    else if (currentWindow()->currentViewMode()==Kexi::TextViewMode) {
      if (d->action_view_text_mode)
        d->action_view_text_mode->setChecked( true );
    }
  }
}*/

tristate KexiMainWindow::startup()
{
    tristate result = true;
    switch (Kexi::startupHandler().action()) {
    case KexiStartupHandler::CreateBlankProject:
        d->updatePropEditorVisibility(Kexi::NoViewMode);
#ifdef __GNUC__
#warning todo modern startup:        result = createBlankProject();
#else
#pragma WARNING( todo modern startup:        result = createBlankProject(); )
#endif
        break;
    case KexiStartupHandler::CreateFromTemplate:
        result = createProjectFromTemplate(*Kexi::startupHandler().projectData());
        break;
    case KexiStartupHandler::OpenProject:
        result = openProject(*Kexi::startupHandler().projectData());
        break;
    case KexiStartupHandler::ImportProject:
        result = showProjectMigrationWizard(
                   Kexi::startupHandler().importActionData().mimeType,
                   Kexi::startupHandler().importActionData().fileName
               );
        break;
    case KexiStartupHandler::ShowWelcomeScreen:
        //! @todo show welcome screen as soon as is available
        QTimer::singleShot(1, this, SLOT(slotProjectWelcome()));
        break;
    default:
        d->updatePropEditorVisibility(Kexi::NoViewMode);
    }
    
    //    d->mainWidget->setAutoSaveSettings(QLatin1String("MainWindow"), /*saveWindowSize*/false);
    return result;
}

static QString internalReason(KexiDB::Object *obj)
{
    const QString &s = obj->errorMsg();
    if (s.isEmpty())
        return s;
    return QString("<br>(%1) ").arg(i18n("reason:") + " <i>" + s + "</i>");
}

tristate KexiMainWindow::openProject(const KexiProjectData& projectData)
{
    kDebug() << projectData;
    KexiProjectData *newProjectData = new KexiProjectData(projectData);
// if (d->userMode) {
    //TODO: maybe also auto allow to open objects...
//  return setupUserModeMode(newProjectData);
// }
    createKexiProject(*newProjectData);
    if (!newProjectData->connectionData()->savePassword
            && newProjectData->connectionData()->password.isEmpty()
            && newProjectData->connectionData()->fileName().isEmpty() //! @todo temp.: change this if there are file-based drivers requiring a password
       ) {
        //ask for password
        KexiDBPasswordDialog pwdDlg(this, *newProjectData->connectionData(),
                                    false /*!showDetailsButton*/);
        if (QDialog::Accepted != pwdDlg.exec()) {
            delete d->prj;
            d->prj = 0;
            return cancelled;
        }
    }
    bool incompatibleWithKexi;
    tristate res = d->prj->open(incompatibleWithKexi);
    if (~res) {
        delete d->prj;
        d->prj = 0;
        return cancelled;
    }
    else if (!res) {
        delete d->prj;
        d->prj = 0;
        if (incompatibleWithKexi) {
            if (KMessageBox::Yes == KMessageBox::questionYesNo(this,
                    i18n("Database project %1 does not appear to have been created using Kexi."
                         "<p>Do you want to import it as a new Kexi project?",
                         projectData.infoString()),
                    0, KGuiItem(i18nc("Import Database", "&Import..."), "database_import"),
                    KStandardGuiItem::cancel()))
            {
                const bool anotherProjectAlreadyOpened = d->prj;
                tristate res = showProjectMigrationWizard("application/x-kexi-connectiondata",
                               projectData.databaseName(), projectData.constConnectionData());

                if (!anotherProjectAlreadyOpened) //the project could have been opened within this instance
                    return res;

                //always return cancelled because even if migration succeeded, new Kexi instance
                //will be started if user wanted to open the imported db
                return cancelled;
            }
            return cancelled;
        }
        return false;
    }
    setupProjectNavigator();
    newProjectData->setLastOpened(QDateTime::currentDateTime());
    Kexi::recentProjects()->addProjectData(newProjectData);
    updateReadOnlyState();
    invalidateActions();
// d->disableErrorMessages = true;
    enableMessages(false);

    QTimer::singleShot(1, this, SLOT(slotAutoOpenObjectsLater()));
    d->tabbedToolBar->showTab("create");// not needed since create toolbar already shows toolbar! move when kexi starts
    d->tabbedToolBar->showTab("data");
    d->tabbedToolBar->showTab("external");
    d->tabbedToolBar->showTab("tools");
    d->tabbedToolBar->hideTab("form");//temporalily until createToolbar is split
    d->tabbedToolBar->hideTab("report");//temporalily until createToolbar is split
    //d->tabbedToolBar->showTab("form");
    //d->tabbedToolBar->showTab("report");

    // make sure any tab is activated
    d->tabbedToolBar->setCurrentTab(0);
    return true;
}

tristate KexiMainWindow::openProject(const KexiProjectData& data, const QString& shortcutPath,
                                     bool *opened)
{
    if (!shortcutPath.isEmpty() && d->prj) {
        const tristate result = openProjectInExternalKexiInstance(
            shortcutPath, QString(), QString());
        if (result == true) {
            *opened = true;
        }
        return result;
    }
    return openProject(data);
}

tristate KexiMainWindow::createProjectFromTemplate(const KexiProjectData& projectData)
{
    QStringList mimetypes;
    mimetypes.append(KexiDB::defaultFileBasedDriverMimeType());
    QString fname;
    const QString startDir("kfiledialog:///OpenExistingOrCreateNewProject"/*as in KexiNewProjectWizard*/);
    const QString caption(i18n("Select New Project's Location"));

    while (true) {
#ifdef __GNUC__
#warning TODO - remove win32 case
#else
#pragma WARNING( TODO - remove win32 case )
#endif
        Q_UNUSED(projectData);
        if (fname.isEmpty() &&
                !projectData.constConnectionData()->dbFileName().isEmpty()) {
            //propose filename from db template name
            fname = projectData.constConnectionData()->dbFileName();
        }
        const bool specialDir = fname.isEmpty();
        kDebug() << fname << ".............";
        KFileDialog dlg(specialDir ? KUrl(startDir) : KUrl(),
                        QString(), this);
        dlg.setModal(true);
        dlg.setMimeFilter(mimetypes);
        if (!specialDir)
            dlg.setSelection(fname);   // may also be a filename
        dlg.setOperationMode(KFileDialog::Saving);
        dlg.setWindowTitle(caption);
        dlg.exec();
        fname = dlg.selectedFile();
        if (fname.isEmpty())
            return cancelled;
        if (KexiFileWidget::askForOverwriting(fname, this))
            break;
    }

    QFile sourceFile(projectData.constConnectionData()->fileName());
    if (!sourceFile.copy(fname)) {
//! @todo show error from with QFile::FileError
        return false;
    }

    return openProject(fname, 0, QString(), projectData.autoopenObjects/*copy*/);
}

void KexiMainWindow::updateReadOnlyState()
{
    const bool readOnly = d->prj && d->prj->dbConnection() && d->prj->dbConnection()->isReadOnly();
    d->statusBar->setReadOnlyFlag(readOnly);
    if (d->navigator) {
        d->navigator->setReadOnly(readOnly);
    }
    
// update "insert ....." actions for every part
    KActionCollection *ac = actionCollection();
    KexiPart::PartInfoList *plist = Kexi::partManager().infoList();
    if (plist) {
        foreach(KexiPart::Info *info, *plist) {
            QAction *a = ac->action(KexiPart::nameForCreateAction(*info));
            if (a)
                a->setEnabled(!readOnly);
        }
    }
}

void KexiMainWindow::slotAutoOpenObjectsLater()
{
    QString not_found_msg;
    bool openingCancelled;
    //ok, now open "autoopen: objects
    if (d->prj) {
        foreach(KexiProjectData::ObjectInfo* info, d->prj->data()->autoopenObjects) {
            KexiPart::Info *i = Kexi::partManager().infoForClass(info->value("type"));
            if (!i) {
                not_found_msg += "<li>";
                if (!info->value("name").isEmpty())
                    not_found_msg += (QString("\"") + info->value("name") + "\" - ");
                if (info->value("action") == "new")
                    not_found_msg += i18n("cannot create object - unknown object type \"%1\"", info->value("type"));
                else
                    not_found_msg += i18n("unknown object type \"%1\"", info->value("type"));
                not_found_msg += internalReason(&Kexi::partManager()) + "<br></li>";
                continue;
            }
            // * NEW
            if (info->value("action") == "new") {
                if (!newObject(i, openingCancelled) && !openingCancelled) {
                    not_found_msg += "<li>";
                    not_found_msg += (i18n("cannot create object of type \"%1\"", info->value("type")) +
                                      internalReason(d->prj) + "<br></li>");
                } else
                    d->wasAutoOpen = true;
                continue;
            }

            KexiPart::Item *item = d->prj->item(i, info->value("name"));

            if (!item) {
                QString taskName;
                if (info->value("action") == "execute")
                    taskName = i18nc("\"executing object\" action", "executing");
#ifndef KEXI_NO_QUICK_PRINTING
                else if (info->value("action") == "print-preview")
                    taskName = i18n("making print preview for");
                else if (info->value("action") == "print")
                    taskName = i18n("printing");
#endif
                else
                    taskName = i18n("opening");

                not_found_msg += (QString("<li>") + taskName + " \"" + info->value("name") + "\" - ");
                if ("table" == info->value("type").toLower())
                    not_found_msg += i18n("table not found");
                else if ("query" == info->value("type").toLower())
                    not_found_msg += i18n("query not found");
                else if ("macro" == info->value("type").toLower())
                    not_found_msg += i18n("macro not found");
                else if ("script" == info->value("type").toLower())
                    not_found_msg += i18n("script not found");
                else
                    not_found_msg += i18n("object not found");
                not_found_msg += (internalReason(d->prj) + "<br></li>");
                continue;
            }
            // * EXECUTE, PRINT, PRINT PREVIEW
            if (info->value("action") == "execute") {
                tristate res = executeItem(item);
                if (false == res) {
                    not_found_msg += (QString("<li>\"") + info->value("name") + "\" - " + i18n("cannot execute object") +
                                      internalReason(d->prj) + "<br></li>");
                }
                continue;
            }
#ifndef KEXI_NO_QUICK_PRINTING
            else if (info->value("action") == "print") {
                tristate res = printItem(item);
                if (false == res) {
                    not_found_msg += (QString("<li>\"") + info->value("name") + "\" - " + i18n("cannot print object") +
                                      internalReason(d->prj) + "<br></li>");
                }
                continue;
            }
            else if (info->value("action") == "print-preview") {
                tristate res = printPreviewForItem(item);
                if (false == res) {
                    not_found_msg += (QString("<li>\"") + info->value("name") + "\" - " + i18n("cannot make print preview of object") +
                                      internalReason(d->prj) + "<br></li>");
                }
                continue;
            }
#endif

            Kexi::ViewMode viewMode;
            if (info->value("action") == "open")
                viewMode = Kexi::DataViewMode;
            else if (info->value("action") == "design")
                viewMode = Kexi::DesignViewMode;
            else if (info->value("action") == "edittext")
                viewMode = Kexi::TextViewMode;
            else
                continue; //sanity

            QString openObjectMessage;
            if (!openObject(item, viewMode, openingCancelled, 0, &openObjectMessage)
                    && (!openingCancelled || !openObjectMessage.isEmpty())) {
                not_found_msg += (QString("<li>\"") + info->value("name") + "\" - ");
                if (openObjectMessage.isEmpty())
                    not_found_msg += i18n("cannot open object");
                else
                    not_found_msg += openObjectMessage;
                not_found_msg += internalReason(d->prj) + "<br></li>";
                continue;
            } else {
                d->wasAutoOpen = true;
            }
        }
    }
    enableMessages(true);
// d->disableErrorMessages = false;

    if (!not_found_msg.isEmpty())
        showErrorMessage(i18n("You have requested selected objects to be automatically opened "
                              "or processed on startup. Several objects cannot be opened or processed."),
                         QString("<ul>%1</ul>").arg(not_found_msg));

    d->updatePropEditorVisibility(currentWindow() ? currentWindow()->currentViewMode() : Kexi::NoViewMode);
#if defined(KDOCKWIDGET_P)
    if (d->propEditor) {
        KDockWidget *dw = (KDockWidget *)d->propEditorTabWidget->parentWidget();
        KDockSplitter *ds = (KDockSplitter *)dw->parentWidget();
        if (ds)
            ds->setSeparatorPosInPercent(d->config->readEntry("RightDockPosition", 80/* % */));
    }
#endif

    updateAppCaption();
// d->navToolWindow->wrapperWidget()->setFixedWidth(200);
    d->tabbedToolBar->hideMainMenu();

    qApp->processEvents();
    emit projectOpened();
}

tristate KexiMainWindow::closeProject()
{
    if (d->tabbedToolBar)
        d->tabbedToolBar->hideMainMenu();

#ifndef KEXI_NO_PENDING_DIALOGS
    if (d->pendingWindowsExist()) {
        kDebug() << "KexiMainWindow::closeProject() pendingWindowsExist...";
        d->actionToExecuteWhenPendingJobsAreFinished = Private::CloseProjectAction;
        return cancelled;
    }
#endif

    //only save nav. visibility setting if there is project opened
    d->saveSettingsForShowProjectNavigator = d->prj && d->isProjectNavigatorVisible;

    if (!d->prj)
        return true;

    {
        // make sure the project can be closed
        bool cancel = false;
        emit acceptProjectClosingRequested(cancel);
        if (cancel)
            return cancelled;
    }

    d->windowExistedBeforeCloseProject = currentWindow();

#if defined(KDOCKWIDGET_P)
    //remember docks position - will be used on storeSettings()
    if (d->propEditor) {
        KDockWidget *dw = (KDockWidget *)d->propEditorTabWidget->parentWidget();
        KDockSplitter *ds = (KDockSplitter *)dw->parentWidget();
        if (ds)
            d->propEditorDockSeparatorPos = ds->separatorPosInPercent();
    }
    if (d->nav) {
//  makeDockInvisible( manager()->findWidgetParentDock(d->propEditor) );

        if (d->propEditor) {
#ifdef __GNUC__
#warning TODO   if (d->openedWindowsCount() == 0)
#else
#pragma WARNING( TODO   if (d->openedWindowsCount() == 0) )
#endif
#ifdef __GNUC__
#warning TODO    makeWidgetDockVisible(d->propEditorTabWidget);
#else
#pragma WARNING( TODO    makeWidgetDockVisible(d->propEditorTabWidget); )
#endif
            KDockWidget *dw = (KDockWidget *)d->propEditorTabWidget->parentWidget();
            KDockSplitter *ds = (KDockSplitter *)dw->parentWidget();
            if (ds)
                ds->setSeparatorPosInPercent(80);
        }

        KDockWidget *dw = (KDockWidget *)d->nav->parentWidget();
        KDockSplitter *ds = (KDockSplitter *)dw->parentWidget();
        int dwWidth = dw->width();
        if (ds) {
            if (d->openedWindowsCount() != 0 && d->propEditorTabWidget && d->propEditorTabWidget->isVisible())
                d->navDockSeparatorPos = ds->separatorPosInPercent();
            else
                d->navDockSeparatorPos = (100 * dwWidth) / width();

//    int navDockSeparatorPosWithAutoOpen = (100 * dw->width()) / width() + 4;
//    d->navDockSeparatorPos = (100 * dw->width()) / width() + 1;
        }
    }
#endif

    //close each window, optionally asking if user wants to close (if data changed)
    while (currentWindow()) {
        tristate res = closeWindow(currentWindow());
        if (!res || ~res)
            return res;
    }

    // now we will close for sure
    emit beforeProjectClosing();

    if (!d->prj->closeConnection())
        return false;

    if (d->navigator) {
        d->navWasVisibleBeforeProjectClosing = d->navDockWidget->isVisible();
        d->navDockWidget->hide();
        d->navigator->setProject(0);
        slotProjectNavigatorVisibilityChanged(true); // hide side tab
        //d->navigator->clear();
    }
    
    if (d->propEditorDockWidget)
        d->propEditorDockWidget->hide();

    d->clearWindows(); //sanity!
    delete d->prj;
    d->prj = 0;

// Kexi::partManager().unloadAllParts();

    updateReadOnlyState();
    invalidateActions();
    updateAppCaption();

    emit projectClosed();
/*
    d->tabbedToolBar->hideTab("create");
    d->tabbedToolBar->hideTab("data");
    d->tabbedToolBar->hideTab("external");
    d->tabbedToolBar->hideTab("tools");
    d->tabbedToolBar->hideTab("form");
    d->tabbedToolBar->hideTab("report");*/
    return true;
}

void KexiMainWindow::setupContextHelp()
{
#ifndef KEXI_NO_CTXT_HELP
    d->ctxHelp = new KexiContextHelp(d->mainWidget, this);
    /*todo
      d->ctxHelp->setContextHelp(i18n("Welcome"),i18n("The <B>KEXI team</B> wishes you a lot of productive work, "
        "with this product. <BR><HR><BR>If you have found a <B>bug</B> or have a <B>feature</B> request, please don't "
        "hesitate to report it at our <A href=\"http://www.kexi-project.org/cgi-bin/bug.pl\"> issue "
        "tracking system </A>.<BR><HR><BR>If you would like to <B>join</B> our effort, the <B>development</B> documentation "
        "at <A href=\"http://www.kexi-project.org\">www.kexi-project.org</A> is a good starting point."),0);
    */
    addToolWindow(d->ctxHelp, KDockWidget::DockBottom | KDockWidget::DockLeft, getMainDockWidget(), 20);
#endif
}

void KexiMainWindow::setupMainWidget()
{
    QVBoxLayout *vlyr = new QVBoxLayout(this);
    vlyr->setContentsMargins(0, 0, 0, 0);
    vlyr->setSpacing(0);

    if (d->isMainMenuVisible) {
        QWidget *tabbedToolBarContainer = new QWidget(this);
        vlyr->addWidget(tabbedToolBarContainer);
        QVBoxLayout *tabbedToolBarContainerLyr = new QVBoxLayout(tabbedToolBarContainer);
        tabbedToolBarContainerLyr->setSpacing(0);
        tabbedToolBarContainerLyr->setContentsMargins(
            KDialog::marginHint() / 2, KDialog::marginHint() / 2,
            KDialog::marginHint() / 2, KDialog::marginHint() / 2);

        d->tabbedToolBar = new KexiTabbedToolBar(tabbedToolBarContainer);
        Q_ASSERT(d->action_view_global_search);
        connect(d->action_view_global_search, SIGNAL(triggered()),
                d->tabbedToolBar, SLOT(activateSearchLineEdit()));
        tabbedToolBarContainerLyr->addWidget(d->tabbedToolBar);
    }
    else {
        d->tabbedToolBar = 0;
    }

    QWidget *mainWidgetContainer = new QWidget();
    vlyr->addWidget(mainWidgetContainer, 1);
    QHBoxLayout *mainWidgetContainerLyr = new QHBoxLayout(mainWidgetContainer);
    mainWidgetContainerLyr->setContentsMargins(0, 0, 0, 0);
    mainWidgetContainerLyr->setSpacing(0);

    KMultiTabBar *mtbar = new KMultiTabBar(KMultiTabBar::Left);
    mtbar->setStyle(KMultiTabBar::KDEV3ICON);
    mainWidgetContainerLyr->addWidget(mtbar, 0);
    d->multiTabBars.insert(mtbar->position(), mtbar);

    d->mainWidget = new KexiMainWidget();
    d->mainWidget->setParent(this);
    
    KConfigGroup mainWindowGroup(d->config->group("MainWindow"));
    d->mainWidget->tabWidget()->setTabsClosable(true);
    connect(d->mainWidget->tabWidget(), SIGNAL(tabCloseRequested(int)),
            this, SLOT(closeWindowForTab(int)));
    mainWidgetContainerLyr->addWidget(d->mainWidget, 1);

    mtbar = new KMultiTabBar(KMultiTabBar::Right);
    mtbar->setStyle(KMultiTabBar::KDEV3ICON);
    mainWidgetContainerLyr->addWidget(mtbar, 0);
    d->multiTabBars.insert(mtbar->position(), mtbar);

    d->statusBar = new KexiStatusBar(this);
#if 0 // still disabled, see KexiStatusBar
    connect(d->statusBar->m_showNavigatorAction, SIGNAL(triggered(bool)),
        this, SLOT(slotSetProjectNavigatorVisible(bool)));
    connect(d->statusBar->m_showPropertyEditorAction, SIGNAL(triggered(bool)),
        this, SLOT(slotSetPropertyEditorVisible(bool)));
#endif
    vlyr->addWidget(d->statusBar);
}

void KexiMainWindow::slotSetProjectNavigatorVisible(bool set)
{
    if (d->navDockWidget)
        d->navDockWidget->setVisible(set);
}

void KexiMainWindow::slotSetPropertyEditorVisible(bool set)
{
    if (d->propEditorDockWidget)
        d->propEditorDockWidget->setVisible(set);
}

void KexiMainWindow::slotProjectNavigatorVisibilityChanged(bool visible)
{
    d->setTabBarVisible(KMultiTabBar::Left, PROJECT_NAVIGATOR_TABBAR_ID,
                        d->navDockWidget, !visible);
}

void KexiMainWindow::slotPropertyEditorVisibilityChanged(bool visible)
{
    if (!d->enable_slotPropertyEditorVisibilityChanged)
        return;
    d->setPropertyEditorTabBarVisible(!visible);
    if (!visible)
        d->propertyEditorCollapsed = true;
}

void KexiMainWindow::slotMultiTabBarTabClicked(int id)
{
    if (id == PROJECT_NAVIGATOR_TABBAR_ID) {
        slotProjectNavigatorVisibilityChanged(true);
        d->navDockWidget->show();
    }
    else if (id == PROPERTY_EDITOR_TABBAR_ID) {
        slotPropertyEditorVisibilityChanged(true);
        d->propEditorDockWidget->show();
        d->propertyEditorCollapsed = false;
    }
}

static Qt::DockWidgetArea loadDockAreaSetting(KConfigGroup& group, const char* configEntry, Qt::DockWidgetArea defaultArea)
{
        const QString areaName = group.readEntry(configEntry).toLower();
        if (areaName == "left")
            return Qt::LeftDockWidgetArea;
        else if (areaName == "right")
            return Qt::RightDockWidgetArea;
        else if (areaName == "top")
            return Qt::TopDockWidgetArea;
        else if (areaName == "bottom")
            return Qt::BottomDockWidgetArea;
        return defaultArea;
}

static void saveDockAreaSetting(KConfigGroup& group, const char* configEntry, Qt::DockWidgetArea area)
{
    QString areaName;
    switch (area) {
    case Qt::LeftDockWidgetArea: areaName = "left"; break;
    case Qt::RightDockWidgetArea: areaName = "right"; break;
    case Qt::TopDockWidgetArea: areaName = "top"; break;
    case Qt::BottomDockWidgetArea: areaName = "bottom"; break;
    default: areaName = "left"; break;
    }
    if (areaName.isEmpty())
        group.deleteEntry(configEntry);
    else
        group.writeEntry(configEntry, areaName);
}

void KexiMainWindow::setupProjectNavigator()
{
    if (!d->isProjectNavigatorVisible)
        return;

    if (d->navigator) {
        d->navDockWidget->show();
    }
    else {
        d->navDockWidget = new KexiDockWidget(QString(), d->mainWidget);
        d->navDockWidget->setObjectName("ProjectNavigatorDockWidget");
//        d->navDockWidget->setMinimumWidth(300);
        KConfigGroup mainWindowGroup(d->config->group("MainWindow"));
        d->mainWidget->addDockWidget(
            loadDockAreaSetting(mainWindowGroup, "ProjectNavigatorArea", Qt::LeftDockWidgetArea),
            d->navDockWidget
//            static_cast<Qt::Orientation>(mainWindowGroup.readEntry("PropertyEditorOrientation", (int)Qt::Vertical))
        );

        KexiDockableWidget* navDockableWidget = new KexiDockableWidget(d->navDockWidget);
        d->navigator = new KexiProjectNavigator(navDockableWidget);
        
        //navDockableWidget->setWidget(d->nav);

               
        //TODO temp
//TODO remove        QWidget *navi = new QWidget(navDockableWidget);
//TODO remove        QVBoxLayout *navi_layout = new QVBoxLayout();
//TODO remove
//TODO remove        navi_layout->addWidget(d->navigator);
//TODO remove        navi->setLayout(navi_layout);

        navDockableWidget->setWidget(d->navigator);
        //End TODO

        d->navDockWidget->setWindowTitle(d->navigator->windowTitle());
        d->navDockWidget->setWidget(navDockableWidget);

//        const bool showProjectNavigator = mainWindowGroup.readEntry("ShowProjectNavigator", true);
        const QSize projectNavigatorSize = mainWindowGroup.readEntry<QSize>("ProjectNavigatorSize", QSize());
        if (!projectNavigatorSize.isNull()) {
            navDockableWidget->setSizeHint(projectNavigatorSize);
        }

#ifdef __GNUC__
#warning TODO d->navToolWindow = addToolWindow(d->nav, KDockWidget::DockLeft, getMainDockWidget(), 20/*, lv, 35, "2"*/);
#else
#pragma WARNING( TODO d->navToolWindow = addToolWindow(d->nav, KDockWidget::DockLeft, getMainDockWidget(), 20/*, lv, 35, "2"*/); )
#endif
//  d->navToolWindow->hide();

        connect(d->navDockWidget, SIGNAL(visibilityChanged(bool)),
            this, SLOT(slotProjectNavigatorVisibilityChanged(bool)));

        //Nav2 Signals
        connect(d->navigator, SIGNAL(openItem(KexiPart::Item*, Kexi::ViewMode)),
                this, SLOT(openObject(KexiPart::Item*, Kexi::ViewMode)));
        connect(d->navigator, SIGNAL(openOrActivateItem(KexiPart::Item*, Kexi::ViewMode)),
                this, SLOT(openObjectFromNavigator(KexiPart::Item*, Kexi::ViewMode)));
        connect(d->navigator, SIGNAL(newItem(KexiPart::Info*)),
                this, SLOT(newObject(KexiPart::Info*)));
        connect(d->navigator, SIGNAL(removeItem(KexiPart::Item*)),
                this, SLOT(removeObject(KexiPart::Item*)));
        connect(d->navigator->model(), SIGNAL(renameItem(KexiPart::Item*, const QString&, bool&)),
                this, SLOT(renameObject(KexiPart::Item*, const QString&, bool&)));
        connect(d->navigator, SIGNAL(executeItem(KexiPart::Item*)),
                this, SLOT(executeItem(KexiPart::Item*)));
        connect(d->navigator, SIGNAL(exportItemToClipboardAsDataTable(KexiPart::Item*)),
                this, SLOT(copyItemToClipboardAsDataTable(KexiPart::Item*)));
        connect(d->navigator, SIGNAL(exportItemToFileAsDataTable(KexiPart::Item*)),
                this, SLOT(exportItemAsDataTable(KexiPart::Item*)));
#ifndef KEXI_NO_QUICK_PRINTING
        connect(d->navigator, SIGNAL(printItem(KexiPart::Item*)),
                this, SLOT(printItem(KexiPart::Item*)));
        connect(d->navigator, SIGNAL(pageSetupForItem(KexiPart::Item*)),
                this, SLOT(showPageSetupForItem(KexiPart::Item*)));
#endif
        connect(d->navigator, SIGNAL(selectionChanged(KexiPart::Item*)),
                this, SLOT(slotPartItemSelectedInNavigator(KexiPart::Item*)));
        if (d->prj) {//connect to the project
            connect(d->prj, SIGNAL(itemRemoved(const KexiPart::Item&)),
                    d->navigator->model(), SLOT(slotRemoveItem(const KexiPart::Item&)));
        }

        
        //  d->restoreNavigatorWidth();
    }
    if (d->prj->isConnected()) {
        QString partManagerErrorMessages;

        if (!partManagerErrorMessages.isEmpty()) {
            showWarningContinueMessage(partManagerErrorMessages, QString(),
                                       "dontShowWarningsRelatedToPluginsLoading");
        }
        d->navigator->setProject(d->prj, QString()/*all classes*/, &partManagerErrorMessages);
        
    }

    connect(d->prj, SIGNAL(newItemStored(KexiPart::Item&)), d->navigator->model(), SLOT(slotAddItem(KexiPart::Item&)));

    d->navigator->setFocus();
    
    if (d->forceShowProjectNavigatorOnCreation) {
        slotViewNavigator();
        d->forceShowProjectNavigatorOnCreation = false;
    } else if (d->forceHideProjectNavigatorOnCreation) {
#ifdef __GNUC__
#warning TODO d->navToolWindow->hide();
#else
#pragma WARNING( TODO d->navToolWindow->hide(); )
#endif
//  makeDockInvisible( manager()->findWidgetParentDock(d->nav) );
        d->forceHideProjectNavigatorOnCreation = false;
    }

    invalidateActions();
}

void KexiMainWindow::slotLastActions()
{
}

void KexiMainWindow::setupPropertyEditor()
{
    if (!d->propEditor) {
        KConfigGroup mainWindowGroup(d->config->group("MainWindow"));
//TODO: FIX LAYOUT PROBLEMS
        d->propEditorDockWidget = new KexiDockWidget(i18n("Property Editor"), d->mainWidget);
        d->propEditorDockWidget->setObjectName("PropertyEditorDockWidget");
        d->mainWidget->addDockWidget(
            loadDockAreaSetting(mainWindowGroup, "PropertyEditorArea", Qt::RightDockWidgetArea),
            d->propEditorDockWidget,
            Qt::Vertical
        );
        connect(d->propEditorDockWidget, SIGNAL(visibilityChanged(bool)),
            this, SLOT(slotPropertyEditorVisibilityChanged(bool)));

        d->propEditorDockableWidget = new KexiDockableWidget(d->propEditorDockWidget);
        d->propEditorDockWidget->setWidget(d->propEditorDockableWidget);
        const QSize propertyEditorSize = mainWindowGroup.readEntry<QSize>("PropertyEditorSize", QSize());
        if (!propertyEditorSize.isNull()) {
            d->propEditorDockableWidget->setSizeHint(propertyEditorSize);
        }

        QWidget *propEditorDockWidgetContents = new QWidget(d->propEditorDockableWidget);
        d->propEditorDockableWidget->setWidget(propEditorDockWidgetContents);
        QVBoxLayout *propEditorDockWidgetContentsLyr = new QVBoxLayout(propEditorDockWidgetContents);
        KexiUtils::setMargins(propEditorDockWidgetContentsLyr, KDialog::marginHint() / 2);

        d->propEditorTabWidget = new KTabWidget(propEditorDockWidgetContents);
        d->propEditorTabWidget->setDocumentMode(true);
        propEditorDockWidgetContentsLyr->addWidget(d->propEditorTabWidget);
//  d->propEditorTabWidget->hide();
        d->propEditor = new KexiPropertyEditorView(d->propEditorTabWidget);
        d->propEditorTabWidget->setWindowTitle(d->propEditor->windowTitle());
        d->propEditorTabWidget->addTab(d->propEditor, i18n("Properties"));
//TODO REMOVE?  d->propEditor->installEventFilter(this);

        KConfigGroup propertyEditorGroup(d->config->group("PropertyEditor"));
        int size = propertyEditorGroup.readEntry("FontSize", -1);
        QFont f(KexiUtils::smallFont());
        if (size > 0)
            f.setPixelSize(size);
        d->propEditorTabWidget->setFont(f);

        d->enable_slotPropertyEditorVisibilityChanged = false;
        d->propEditorDockWidget->setVisible(false);
        d->enable_slotPropertyEditorVisibilityChanged = true;
    }
}

void KexiMainWindow::slotPartLoaded(KexiPart::Part* p)
{
    if (!p)
        return;
    p->createGUIClients();//this);
}

void KexiMainWindow::updateAppCaption()
{
//! @todo allow to set custom "static" app caption

    d->appCaptionPrefix.clear();
    if (d->prj && d->prj->data()) {//add project name
        d->appCaptionPrefix = d->prj->data()->caption();
        if (d->appCaptionPrefix.isEmpty())
            d->appCaptionPrefix = d->prj->data()->databaseName();
    }

    setWindowTitle((d->appCaptionPrefix.isEmpty() ? QString() : (d->appCaptionPrefix + QString::fromLatin1(" - ")))
                   + KGlobal::mainComponent().aboutData()->programName());
}

bool KexiMainWindow::queryClose()
{
#ifndef KEXI_NO_PENDING_DIALOGS
    if (d->pendingWindowsExist()) {
        kDebug() << "KexiMainWindow::queryClose() pendingWindowsExist...";
        d->actionToExecuteWhenPendingJobsAreFinished = Private::QuitAction;
        return false;
    }
#endif
// storeSettings();
    const tristate res = closeProject();
    if (~res)
        return false;

    if (res == true)
        storeSettings();

    if (! ~res) {
        qApp->quit();
    }
    return ! ~res;
}

bool KexiMainWindow::queryExit()
{
    //storeSettings();
    return true;
}

void KexiMainWindow::closeEvent(QCloseEvent *ev)
{
    d->mainWidget->closeEvent(ev);
/*    if (queryClose()) {
        ev->accept();
    }*/
}

void
KexiMainWindow::restoreSettings()
{
    KConfigGroup mainWindowGroup(d->config->group("MainWindow"));
// restoreWindowSize( mainWindowGroup );
// d->mainWidget->applyMainWindowSettings( mainWindowGroup );
// saveState()
    const bool maximize = mainWindowGroup.readEntry("Maximized", false);
    const QRect geometry(mainWindowGroup.readEntry("Geometry", QRect()));
    if (geometry.isValid())
        setGeometry(geometry);
    else if (maximize)
        setWindowState(windowState() | Qt::WindowMaximized);
    else {
        QRect desk = QApplication::desktop()->screenGeometry(
            QApplication::desktop()->screenNumber(this));
        if (desk.width() <= 1024 || desk.height() < 768)
            setWindowState(windowState() | Qt::WindowMaximized);
        else
            resize(1024, 768);
    }
    // Saved settings

#if 0
    if (showProjectNavigator) {
        //it's invisible by default but we want to show it on navigator creation
        d->forceShowProjectNavigatorOnCreation = true;
    }
#endif
}

void
KexiMainWindow::storeSettings()
{
    kDebug() << "KexiMainWindow::storeSettings()";
    KConfigGroup mainWindowGroup(d->config->group("MainWindow"));
    //saveWindowSize( mainWindowGroup );

    if (isMaximized()) {
        mainWindowGroup.writeEntry("Maximized", true);
        mainWindowGroup.deleteEntry("Geometry");
    } else {
        mainWindowGroup.deleteEntry("Maximized");
        mainWindowGroup.writeEntry("Geometry", geometry());
    }

    saveDockAreaSetting(mainWindowGroup, "ProjectNavigatorArea", d->mainWidget->dockWidgetArea(d->navDockWidget));
    saveDockAreaSetting(mainWindowGroup, "PropertyEditorArea", d->mainWidget->dockWidgetArea(d->propEditorDockWidget));

// mainWindowGroup.writeEntry("PropertyEditor", mb->isHidden() ? "Disabled" : "Enabled");
// d->mainWidget->saveMainWindowSettings( mainWindowGroup );
// d->mainWidget->saveState();

    if (d->navigator)
        mainWindowGroup.writeEntry("ProjectNavigatorSize", d->navigator->parentWidget()->size());
    
    if (d->propEditorDockableWidget)
        mainWindowGroup.writeEntry("PropertyEditorSize", d->propEditorDockableWidget->size());

    d->config->sync();
#if 0

    if (d->saveSettingsForShowProjectNavigator) {
        if (d->navWasVisibleBeforeProjectClosing)
            mainWindowGroup.deleteEntry("ShowProjectNavigator");
        else
            mainWindowGroup.writeEntry("ShowProjectNavigator", false);
    }

    if (d->propEditor) {
        KConfigGroup propertyEditorGroup(d->config->group("PropertyEditor"));
        propertyEditorGroup.writeEntry("FontSize", d->propEditorTabWidget->font().pixelSize());
    }
#endif
}

void
KexiMainWindow::registerChild(KexiWindow *window)
{
    kDebug();
    connect(window, SIGNAL(dirtyChanged(KexiWindow*)), this, SLOT(slotDirtyFlagChanged(KexiWindow*)));

    if (window->id() != -1) {
        d->insertWindow(window);
    }
    kDebug() << "ID=" << window->id();
}

void
KexiMainWindow::updateWindowViewGUIClient(KXMLGUIClient *viewClient)
{
    Q_UNUSED(viewClient);
}

void KexiMainWindow::updateCustomPropertyPanelTabs(KexiWindow *prevWindow,
        Kexi::ViewMode prevViewMode)
{
    updateCustomPropertyPanelTabs(
        prevWindow ? prevWindow->part() : 0,
        prevWindow ? prevWindow->currentViewMode() : prevViewMode,
        currentWindow() ? currentWindow()->part() : 0,
        currentWindow() ? currentWindow()->currentViewMode() : Kexi::NoViewMode
    );
}

void KexiMainWindow::updateCustomPropertyPanelTabs(
    KexiPart::Part *prevWindowPart, Kexi::ViewMode prevViewMode,
    KexiPart::Part *curWindowPart, Kexi::ViewMode curViewMode)
{
    if (!d->propEditorTabWidget)
        return;

    if (   !curWindowPart
        || (/*prevWindowPart &&*/ curWindowPart
             && (prevWindowPart != curWindowPart || prevViewMode != curViewMode)
           )
       )
    {
        if (d->partForPreviouslySetupPropertyPanelTabs) {
            //remember current page number for this part
            if ((   prevViewMode == Kexi::DesignViewMode
                 && static_cast<KexiPart::Part*>(d->partForPreviouslySetupPropertyPanelTabs) != curWindowPart) //part changed
                || curViewMode != Kexi::DesignViewMode)
            { //..or switching to other view mode
                d->recentlySelectedPropertyPanelPages.insert(
                        d->partForPreviouslySetupPropertyPanelTabs,
                        d->propEditorTabWidget->currentIndex());
            }
        }

        //delete old custom tabs (other than 'property' tab)
        const uint count = d->propEditorTabWidget->count();
        for (uint i = 1; i < count; i++)
            d->propEditorTabWidget->removeTab(1);
    }

    //don't change anything if part is not switched nor view mode changed
    if ((!prevWindowPart && !curWindowPart)
            || (prevWindowPart == curWindowPart && prevViewMode == curViewMode)
            || (curWindowPart && curViewMode != Kexi::DesignViewMode))
    {
        //new part for 'previously setup tabs'
        d->partForPreviouslySetupPropertyPanelTabs = curWindowPart;
        return;
    }

    if (curWindowPart) {
        //recreate custom tabs
        curWindowPart->setupCustomPropertyPanelTabs(d->propEditorTabWidget);

        //restore current page number for this part
        if (d->recentlySelectedPropertyPanelPages.contains(curWindowPart)) {
            d->propEditorTabWidget->setCurrentIndex(
                d->recentlySelectedPropertyPanelPages[ curWindowPart ]
            );
        }
    }

    //new part for 'previously setup tabs'
    d->partForPreviouslySetupPropertyPanelTabs = curWindowPart;
}

void KexiMainWindow::activeWindowChanged(KexiWindow *window, KexiWindow *prevWindow)
{
    kDebug() << "to=" << (window ? window->windowTitle() : "<none>");
    bool windowChanged = prevWindow != window;

    if (windowChanged) {
        if (prevWindow) {
            //inform previously activated dialog about deactivation
            prevWindow->deactivate();
        }
    }

    updateCustomPropertyPanelTabs(prevWindow, prevWindow ? prevWindow->currentViewMode() : Kexi::NoViewMode);

    // inform the current view of the new dialog about property switching
    // (this will also call KexiMainWindow::propertySetSwitched() to update the current property editor's set
    if (windowChanged && currentWindow())
        currentWindow()->selectedView()->propertySetSwitched();

    if (windowChanged) {
        if (currentWindow() && currentWindow()->currentViewMode() != 0 && window) //on opening new dialog it can be 0; we don't want this
            d->updatePropEditorVisibility(currentWindow()->currentViewMode());
    }

    invalidateActions();
    d->updateFindDialogContents();
    if (window)
        window->setFocus();
}

bool
KexiMainWindow::activateWindow(int id)
{
    kDebug();
#ifndef KEXI_NO_PENDING_DIALOGS
    Private::PendingJobType pendingType;
    return activateWindow(*d->openedWindowFor(id, pendingType));
#else
    return activateWindow(*d->openedWindowFor(id));
#endif
}

bool
KexiMainWindow::activateWindow(KexiWindow& window)
{
    kDebug() << "KexiMainWindow::activateWindow(KexiWindow&)";

    d->focus_before_popup = &window;
    d->mainWidget->tabWidget()->setCurrentWidget(window.parentWidget()/*container*/);
    window.activate();
    return true;
}

void KexiMainWindow::activateNextWindow()
{
#ifdef __GNUC__
#warning TODO activateNextWindow()
#else
#pragma WARNING( TODO activateNextWindow() )
#endif
}

void KexiMainWindow::activatePreviousWindow()
{
#ifdef __GNUC__
#warning TODO activatePreviousWindow()
#else
#pragma WARNING( TODO activatePreviousWindow() )
#endif
}

void
KexiMainWindow::slotSettings()
{
    if (d->tabbedToolBar) {
        d->tabbedToolBar->showMainMenu("settings");
        // dummy
        QLabel *dummy = KEXI_UNFINISHED_LABEL(actionCollection()->action("settings")->text());
        d->tabbedToolBar->setMainMenuContent(dummy);
    }
}

void
KexiMainWindow::slotConfigureKeys()
{
    /*    KShortcutsDialog dlg;
        dlg.insert( actionCollection() );
        dlg.configure();*/
    KShortcutsDialog::configure(actionCollection(),
                                KShortcutsEditor::LetterShortcutsDisallowed, this);
}

void
KexiMainWindow::slotConfigureToolbars()
{
    KEditToolBar edit(actionCollection());//factory());
//    connect(&edit,SIGNAL(newToolbarConfig()),this,SLOT(slotNewToolbarConfig()));
    (void) edit.exec();
}

void KexiMainWindow::slotProjectNew()
{
    createNewProject();
#if 0
    if (!d->prj) {
        //create within this instance
        createBlankProject();
        return;
    }

    bool cancel;
    QString fileName;
    KexiProjectData *new_data = createBlankProjectData(
                                    cancel,
                                    false, /* do not confirm prj overwrites: user will be asked on process startup */
                                    &fileName //shortcut fname
                                );
    if (!new_data)
        return;

    QStringList args;
    args  << "-create-opendb";
    if (new_data->connectionData()->fileName().isEmpty()) {
        //server based - pass .kexic file
        if (fileName.isEmpty())
            return;
        args << new_data->databaseName() << fileName;
        //args << "--skip-conn-dialog"; //user does not expect conn. dialog to be shown here
    } else {
        //file based
        fileName = new_data->connectionData()->fileName();
        args << fileName;
    }
//todo:   pass new_data->caption()
    //start new instance
//! @todo use KProcess?
#ifdef __GNUC__
#warning untested
#else
#pragma WARNING( untested )
#endif
    QProcess proc(this);
//    proc.setCommunication((Q3Process::Communication)0);
//  proc.setWorkingDirectory( QFileInfo(new_data->connectionData()->fileName()).dir(true) );
    proc.setWorkingDirectory(QFileInfo(fileName).absoluteDir().absolutePath());
    proc.start(qApp->applicationFilePath(), args);
    if (!proc.waitForStarted()) {
        d->showStartProcessMsg(args);
    }
    delete new_data;
#endif
}

void
KexiMainWindow::createKexiProject(const KexiProjectData& new_data)
{
    d->prj = new KexiProject(new_data, this);
// d->prj = ::createKexiProject(new_data);
//provided by KexiMessageHandler connect(d->prj, SIGNAL(error(const QString&,KexiDB::Object*)), this, SLOT(showErrorMessage(const QString&,KexiDB::Object*)));
//provided by KexiMessageHandler connect(d->prj, SIGNAL(error(const QString&,const QString&)), this, SLOT(showErrorMessage(const QString&,const QString&)));
    connect(d->prj, SIGNAL(itemRenamed(const KexiPart::Item&, const QString&)), this, SLOT(slotObjectRenamed(const KexiPart::Item&, const QString&)));

    if (d->navigator){
        connect(d->prj, SIGNAL(itemRemoved(const KexiPart::Item&)), d->navigator, SLOT(slotRemoveItem(const KexiPart::Item&)));
    }
    
}

//unused
KexiProjectData* KexiMainWindow::createBlankProjectData(bool &cancelled, bool confirmOverwrites,
                                       QString* shortcutFileName)
{
    Q_UNUSED(shortcutFileName);
    Q_UNUSED(confirmOverwrites);

    //KexiNewProjectWizard *wiz = new KexiNewProjectWizard(Kexi::connset(), 0);
    //wiz->setConfirmOverwrites(confirmOverwrites);

#ifdef __GNUC__
#warning todo
#else
#pragma WARNING( todo )
#endif
    cancelled = false;
    KexiProjectData *new_data = 0;
#if 0 // before MODERN
    if (wiz.exec() != QDialog::Accepted) {
        cancelled = true;
        return 0;
    }

    if (shortcutFileName)
        shortcutFileName->clear();
    if (wiz.projectConnectionData()) {
        //server-based project
        KexiDB::ConnectionData *cdata = wiz->projectConnectionData();
        kDebug() << "DBNAME: " << wiz.projectDBName() << " SERVER: " << cdata->serverInfoString();
        new_data = new KexiProjectData(*cdata, wiz.projectDBName(), wiz.projectCaption());
        if (shortcutFileName)
            *shortcutFileName = Kexi::connset().fileNameForConnectionData(cdata);
    } else if (!wiz.projectDBName().isEmpty()) {
        //file-based project
        KexiDB::ConnectionData cdata;
        cdata.caption = wiz.projectCaption();
        cdata.driverName = KexiDB::defaultFileBasedDriverName();
        cdata.setFileName(wiz.projectDBName());
        new_data = new KexiProjectData(cdata, wiz.projectDBName(), wiz.projectCaption());
    } else {
        cancelled = true;
        return 0;
    }
#endif
    return new_data;
}

void KexiMainWindow::createNewProject()
{
    if (!d->tabbedToolBar)
        return;
    d->tabbedToolBar->showMainMenu("project_new");
    KexiNewProjectAssistant* assistant = new KexiNewProjectAssistant;
    connect(assistant, SIGNAL(createProject(KexiProjectData*)), 
            this, SLOT(createNewProject(KexiProjectData*)));

    d->tabbedToolBar->setMainMenuContent(assistant);
#if 0   
    
    bool cancel;
    KexiProjectData *new_data = createBlankProjectData(cancel);
    if (cancel)
        return cancelled;
    if (!new_data)
        return false;

    createKexiProject(new_data);

    tristate res = d->prj->create(true /*overwrite*/);
    if (res != true) {
        delete d->prj;
        d->prj = 0;
        return res;
    }
    kDebug() << "new project created ---";
    setupProjectNavigator();
    Kexi::recentProjects().addProjectData(new_data);

    invalidateActions();
    updateAppCaption();
    return true;
#endif
}

tristate KexiMainWindow::createNewProject(KexiProjectData* projectData)
{
    createKexiProject(*projectData);
    tristate res = d->prj->create(true /*overwrite*/);
    if (res != true) {
        delete d->prj;
        d->prj = 0;
        return res;
    }
    d->tabbedToolBar->hideMainMenu();
    kDebug() << "new project created ---";
    setupProjectNavigator();
    projectData->setLastOpened(QDateTime::currentDateTime());
    Kexi::recentProjects()->addProjectData(projectData);

    invalidateActions();
    updateAppCaption();
    return true;
}

void KexiMainWindow::slotProjectOpen()
{
    if (!d->tabbedToolBar)
        return;
    d->tabbedToolBar->showMainMenu("project_open");
    KexiOpenProjectAssistant* assistant = new KexiOpenProjectAssistant;
    connect(assistant, SIGNAL(openProject(KexiProjectData)), 
            this, SLOT(openProject(KexiProjectData)));
    connect(assistant, SIGNAL(openProject(QString)), 
            this, SLOT(openProject(QString)));
    d->tabbedToolBar->setMainMenuContent(assistant);
}

tristate KexiMainWindow::openProject(const QString& aFileName)
{
    return openProject(aFileName, QString(), QString());
}

tristate KexiMainWindow::openProject(const QString& aFileName,
                                     const QString& fileNameForConnectionData, const QString& dbName)
{
    if (d->prj)
        return openProjectInExternalKexiInstance(aFileName, fileNameForConnectionData, dbName);

    KexiDB::ConnectionData *cdata = 0;
    if (!fileNameForConnectionData.isEmpty()) {
        cdata = Kexi::connset().connectionDataForFileName(fileNameForConnectionData);
        if (!cdata) {
            kWarning() << "KexiMainWindow::openProject() cdata?";
            return false;
        }
    }
    return openProject(aFileName, cdata, dbName);
}

tristate KexiMainWindow::openProject(const QString& aFileName,
                                     KexiDB::ConnectionData *cdata, const QString& dbName,
                                     const KexiProjectData::AutoOpenObjects& autoopenObjects)
{
    if (d->prj) {
        return openProjectInExternalKexiInstance(aFileName, cdata, dbName);
    }

    KexiProjectData* projectData = 0;
    bool deleteAfterOpen = false;
    if (cdata) {
        //server-based project
        if (dbName.isEmpty()) {//no database name given, ask user
            bool cancel;
            projectData = Kexi::startupHandler().selectProject(cdata, cancel, this);
            if (cancel)
                return cancelled;
        } else {
//! @todo caption arg?
            projectData = new KexiProjectData(*cdata, dbName);
            deleteAfterOpen = true;
        }
    } else {
//  QString selFile = dlg.selectedExistingFile();
        if (aFileName.isEmpty()) {
            kWarning() << "KexiMainWindow::openProject(): aFileName.isEmpty()";
            return false;
        }
        //file-based project
        kDebug() << "Project File: " << aFileName;
        KexiDB::ConnectionData cdata;
        cdata.setFileName(aFileName);
//   cdata.driverName = KexiStartupHandler::detectDriverForFile( cdata.driverName, fileName, this );
        QString detectedDriverName;
        KexiStartupData::Import importActionData;
        const tristate res = KexiStartupHandler::detectActionForFile(
                                 importActionData, detectedDriverName, cdata.driverName, aFileName, this);
        if (true != res)
            return res;

        if (importActionData) { //importing requested
            return showProjectMigrationWizard(importActionData.mimeType, importActionData.fileName);
        }
        cdata.driverName = detectedDriverName;

        if (cdata.driverName.isEmpty())
            return false;

        //opening requested
        projectData = new KexiProjectData(cdata);
        deleteAfterOpen = true;
    }
    if (!projectData)
        return false;
    projectData->autoopenObjects = autoopenObjects;
    const tristate res = openProject(*projectData);
    if (deleteAfterOpen) //projectData object has been copied
        delete projectData;
    return res;
}

tristate KexiMainWindow::openProjectInExternalKexiInstance(const QString& aFileName,
        KexiDB::ConnectionData *cdata, const QString& dbName)
{
    QString fileNameForConnectionData;
    if (aFileName.isEmpty()) { //try .kexic file
        if (cdata)
            fileNameForConnectionData = Kexi::connset().fileNameForConnectionData(cdata);
    }
    return openProjectInExternalKexiInstance(aFileName, fileNameForConnectionData, dbName);
}

tristate KexiMainWindow::openProjectInExternalKexiInstance(const QString& aFileName,
        const QString& fileNameForConnectionData, const QString& dbName)
{
    QString fileName(aFileName);
    QStringList args;
 
    // open a file-based project or a server connection provided as a .kexic file
    // (we have no other simple way to provide the startup data to a new process)
    if (fileName.isEmpty()) { //try .kexic file
        if (!fileNameForConnectionData.isEmpty())
            args << "--skip-conn-dialog"; //user does not expect conn. dialog to be shown here

        if (dbName.isEmpty()) { //use 'kexi --skip-conn-dialog file.kexic'
            fileName = fileNameForConnectionData;
        } else { //use 'kexi --skip-conn-dialog --connection file.kexic dbName'
            args << "--connection" << fileNameForConnectionData;
            fileName = dbName;
        }
    }
    if (fileName.isEmpty()) {
        kWarning() << "KexiMainWindow::openProjectInExternalKexiInstance() fileName?";
        return false;
    }
//! @todo use KRun
//Can arguments be supplied to KRun like is used here? AP
#ifdef __GNUC__
#warning untested
#else
#pragma WARNING( untested )
#endif
    args << fileName;
    const bool ok = QProcess::startDetached(
        qApp->applicationFilePath(), args,
        QFileInfo(fileName).absoluteDir().absolutePath());
    if (!ok) {
        d->showStartProcessMsg(args);
    }
    d->tabbedToolBar->hideMainMenu();
    return ok;
}

void KexiMainWindow::slotProjectWelcome()
{
    if (!d->tabbedToolBar)
        return;
    d->tabbedToolBar->showMainMenu("project_welcome");
    KexiWelcomeAssistant* assistant = new KexiWelcomeAssistant(
        Kexi::recentProjects());
    connect(assistant, SIGNAL(openProject(KexiProjectData,QString,bool*)), 
            this, SLOT(openProject(KexiProjectData,QString,bool*)));
    d->tabbedToolBar->setMainMenuContent(assistant);
}

void
KexiMainWindow::slotProjectSave()
{
    if (!currentWindow())
        return;
    saveObject(currentWindow());
    updateAppCaption();
    invalidateActions();
}

void
KexiMainWindow::slotProjectSaveAs()
{
    KEXI_UNFINISHED(i18n("Save object as"));
}

void
KexiMainWindow::slotProjectPrint()
{
#ifndef KEXI_NO_QUICK_PRINTING
    if (currentWindow() && currentWindow()->partItem())
        printItem(currentWindow()->partItem());
#endif
}

void
KexiMainWindow::slotProjectPrintPreview()
{
#ifndef KEXI_NO_QUICK_PRINTING
    if (currentWindow() && currentWindow()->partItem())
        printPreviewForItem(currentWindow()->partItem());
#endif
}

void
KexiMainWindow::slotProjectPageSetup()
{
#ifndef KEXI_NO_QUICK_PRINTING
    if (currentWindow() && currentWindow()->partItem())
        showPageSetupForItem(currentWindow()->partItem());
#endif
}

void KexiMainWindow::slotProjectExportDataTable()
{
    if (currentWindow() && currentWindow()->partItem())
        exportItemAsDataTable(currentWindow()->partItem());
}

void KexiMainWindow::slotProjectProperties()
{
    if (!d->tabbedToolBar)
        return;
    d->tabbedToolBar->showMainMenu("project_properties");
    // dummy
    QLabel *dummy = KEXI_UNFINISHED_LABEL(actionCollection()->action("project_properties")->text());
    d->tabbedToolBar->setMainMenuContent(dummy);
    //TODO: load the implementation not the ui :)
// ProjectSettingsUI u(this);
// u.exec();
}

void KexiMainWindow::slotProjectImportExportOrSend()
{
    if (!d->tabbedToolBar)
        return;
    d->tabbedToolBar->showMainMenu("project_import_export_send");
    KexiImportExportAssistant* assistant = new KexiImportExportAssistant(
        d->action_project_import_export_send,
        d->action_tools_import_project);
    connect(assistant, SIGNAL(importProject()), this, SLOT(slotToolsImportProject()));
    d->tabbedToolBar->setMainMenuContent(assistant);
}

void
KexiMainWindow::slotProjectClose()
{
    closeProject();
}

void KexiMainWindow::slotProjectRelations()
{
    if (!d->prj)
        return;
    KexiWindow *w = KexiInternalPart::createKexiWindowInstance("relation", this);
    activateWindow(*w);
}

void KexiMainWindow::slotImportFile()
{
    KEXI_UNFINISHED("Import: " + i18n("From File..."));
}

void KexiMainWindow::slotImportServer()
{
    KEXI_UNFINISHED("Import: " + i18n("From Server..."));
}

void
KexiMainWindow::slotProjectQuit()
{
    if (~ closeProject())
        return;
    close();
}

void KexiMainWindow::slotViewNavigator()
{
    if (!d->navigator) {
        return;
    }
    d->navigator->setFocus();
}

void KexiMainWindow::slotViewMainArea()
{
    if (currentWindow())
        currentWindow()->setFocus();
}

void KexiMainWindow::slotViewPropertyEditor()
{
    if (!d->propEditor) {
        return;
    }

    if (d->propEditorTabWidget->currentWidget())
        d->propEditorTabWidget->currentWidget()->setFocus();
}

tristate KexiMainWindow::switchToViewMode(KexiWindow& window, Kexi::ViewMode viewMode)
{
    const Kexi::ViewMode prevViewMode = currentWindow()->currentViewMode();
    if (prevViewMode == viewMode)
        return true;
    if (!activateWindow(window))
        return false;
    if (!currentWindow()) {
        /* UNUSED, see KexiToggleViewModeAction
            d->toggleLastCheckedMode();*/
        return false;
    }
    if (&window != currentWindow())
        return false;
    if (!currentWindow()->supportsViewMode(viewMode)) {
        showErrorMessage(i18n("Selected view is not supported for \"%1\" object.",
                              currentWindow()->partItem()->name()),
                         i18n("Selected view (%1) is not supported by this object type (%2).",
                              Kexi::nameForViewMode(viewMode),
                              currentWindow()->part()->info()->instanceCaption()));
        /* UNUSED, see KexiToggleViewModeAction
            d->toggleLastCheckedMode();*/
        return false;
    }
    updateCustomPropertyPanelTabs(currentWindow()->part(), prevViewMode,
                                  currentWindow()->part(), viewMode);
    tristate res = currentWindow()->switchToViewMode(viewMode);
    if (!res) {
        updateCustomPropertyPanelTabs(0, Kexi::NoViewMode); //revert
        showErrorMessage(i18n("Switching to other view failed (%1).",
                              Kexi::nameForViewMode(viewMode)), currentWindow());
        /* UNUSED, see KexiToggleViewModeAction
            d->toggleLastCheckedMode();*/
        return false;
    }
    if (~res) {
        updateCustomPropertyPanelTabs(0, Kexi::NoViewMode); //revert
        /* UNUSED, see KexiToggleViewModeAction
            d->toggleLastCheckedMode();*/
        return cancelled;
    }

    activateWindow(window);
    //view changed: switch to this view's gui client
    KXMLGUIClient *viewClient = currentWindow()->guiClient();
    updateWindowViewGUIClient(viewClient);
    if (d->curWindowViewGUIClient && !viewClient)
        guiFactory()->removeClient(d->curWindowViewGUIClient);
    d->curWindowViewGUIClient = viewClient; //remember

    invalidateSharedActions();
    invalidateProjectWideActions();
    d->updateFindDialogContents();
    d->updatePropEditorVisibility(viewMode);
    showDesignTabIfNeeded(currentWindow()->partItem()->partClass(), viewMode);
    return true;
}

void KexiMainWindow::slotViewDataMode()
{
    if (currentWindow())
        switchToViewMode(*currentWindow(), Kexi::DataViewMode);
}

void KexiMainWindow::slotViewDesignMode()
{
    if (currentWindow())
        switchToViewMode(*currentWindow(), Kexi::DesignViewMode);
}

void KexiMainWindow::slotViewTextMode()
{
    if (currentWindow())
        switchToViewMode(*currentWindow(), Kexi::TextViewMode);
}

tristate KexiMainWindow::getNewObjectInfo(
    KexiPart::Item *partItem, KexiPart::Part *part,
    bool& allowOverwriting, const QString& messageWhenAskingForName)
{
    //data was never saved in the past -we need to create a new object at the backend
    KexiPart::Info *info = part->info();
    if (!d->nameDialog) {
        d->nameDialog = new KexiNameDialog(
            messageWhenAskingForName, this);
        //check if that name is allowed
        d->nameDialog->widget()->addNameSubvalidator(
            new KexiDB::ObjectNameValidator(project()->dbConnection()->driver()));
    } else {
        d->nameDialog->widget()->setMessageText(messageWhenAskingForName);
    }
    d->nameDialog->widget()->setCaptionText(partItem->caption());
    d->nameDialog->widget()->setNameText(partItem->name());
    d->nameDialog->setWindowTitle(i18n("Save Object As"));
    d->nameDialog->setDialogIcon(info->itemIconName());
    allowOverwriting = false;
    bool found;
    do {
        if (d->nameDialog->exec() != QDialog::Accepted)
            return cancelled;
        //check if that name already exists
        KexiDB::SchemaData tmp_sdata;
        tristate result = project()->dbConnection()->loadObjectSchemaData(
                              project()->idForClass(info->partClass()),
                              d->nameDialog->widget()->nameText(), tmp_sdata);
        if (!result)
            return false;
        found = result == true;
        if (found) {
            if (allowOverwriting) {
                int res = KMessageBox::warningYesNoCancel(this,
                          "<p>"
                          + part->i18nMessage("Object \"%1\" already exists.", 0)
                          .subs(d->nameDialog->widget()->nameText()).toString()
                          + "</p><p>" + i18n("Do you want to replace it?") + "</p>",
                          QString(),
                          KGuiItem(i18n("&Replace"), "button_yes"),
                          KGuiItem(i18n("&Choose Other Name...")),
                          KStandardGuiItem::cancel(),
                          QString(),
                          KMessageBox::Notify | KMessageBox::Dangerous);
                if (res == KMessageBox::No)
                    continue;
                else if (res == KMessageBox::Cancel)
                    return cancelled;
                else {//yes
                    allowOverwriting = true;
                    break;
                }
            } else {
                KMessageBox::information(this,
                                         "<p>"
                                         + part->i18nMessage("Object \"%1\" already exists.", 0)
                                         .subs(d->nameDialog->widget()->nameText()).toString()
                                         + "</p><p>" + i18n("Please choose other name.") + "</p>");
//    " For example: Table \"my_table\" already exists" ,
//    "%1 \"%2\" already exists.\nPlease choose other name.")
//    .arg(dlg->part().componentName()).arg(d->nameDialog->widget()->nameText()));
                continue;
            }
        }
    } while (found);

    //update name and caption
    partItem->setName(d->nameDialog->widget()->nameText());
    partItem->setCaption(d->nameDialog->widget()->captionText());
    return true;
}

tristate KexiMainWindow::saveObject(KexiWindow *window, const QString& messageWhenAskingForName,
                                    bool dontAsk)
{
    tristate res;
    if (!window->neverSaved()) {
        //data was saved in the past -just save again
        res = window->storeData(dontAsk);
        if (!res)
            showErrorMessage(i18n("Saving \"%1\" object failed.", window->partItem()->name()),
                             currentWindow());
        return res;
    }

    const int oldItemID = window->partItem()->identifier();

    bool allowOverwriting = false;
    res = getNewObjectInfo(window->partItem(), window->part(), allowOverwriting,
                           messageWhenAskingForName);
    if (res != true)
        return res;

    res = window->storeNewData();
    if (~res)
        return cancelled;
    if (!res) {
        showErrorMessage(i18n("Saving new \"%1\" object failed.", window->partItem()->name()),
                         currentWindow());
        return false;
    }

    //update navigator
//this is alreday done in KexiProject::addStoredItem(): d->nav->addItem(window->partItem());
    //item id changed to final one: update association in dialogs' dictionary
// d->dialogs.take(oldItemID);
    d->updateWindowId(window, oldItemID);
    invalidateProjectWideActions();
    return true;
}

tristate KexiMainWindow::closeWindow(KexiWindow *window)
{
    return closeWindow(window ? window : currentWindow(), true);
}

tristate KexiMainWindow::closeCurrentWindow()
{
    return closeWindow(0);
}

tristate KexiMainWindow::closeWindowForTab(int tabIndex)
{
    KexiWindow* window = windowForTab(tabIndex);
    if (!window)
        return false;
    return closeWindow(window);
}

tristate KexiMainWindow::closeWindow(KexiWindow *window, bool layoutTaskBar, bool doNotSaveChanges)
{
#ifdef __GNUC__
#warning TODO KexiMainWindow::closeWindow()
#else
#pragma WARNING( TODO KexiMainWindow::closeWindow() )
#endif
    ///@note Q_UNUSED layoutTaskBar
    Q_UNUSED(layoutTaskBar);
    
    if (!window)
        return true;
    if (d->insideCloseWindow)
        return true;

#ifndef KEXI_NO_PENDING_DIALOGS
    d->addItemToPendingWindows(window->partItem(), Private::WindowClosingJob);
#endif

    d->insideCloseWindow = true;

    if (window == currentWindow() && !window->isAttached()) {
        if (d->propEditor) {
            // ah, closing detached window - better switch off property buffer right now...
            d->propertySet = 0;
            d->propEditor->editor()->changeSet(0);
        }
    }

    bool remove_on_closing = window->partItem() ? window->partItem()->neverSaved() : false;
    if (window->isDirty() && !d->forceWindowClosing && !doNotSaveChanges) {
        //more accurate tool tips and what's this
        KGuiItem saveChanges(KStandardGuiItem::save());
        saveChanges.setToolTip(i18n("Save changes"));
        saveChanges.setWhatsThis(
            i18n("Pressing this button will save all recent changes made in \"%1\" object.",
                 window->partItem()->name()));
        KGuiItem discardChanges(KStandardGuiItem::discard());
        discardChanges.setWhatsThis(
            i18n("Pressing this button will discard all recent changes made in \"%1\" object.",
                 window->partItem()->name()));

        //dialog's data is dirty:
        //--adidional message, e.g. table designer will return
        //  "Note: This table is already filled with data which will be removed."
        //  if the window is in design view mode.
        const KLocalizedString additionalMessage(
            window->part()->i18nMessage(":additional message before saving design", window));
        QString additionalMessageString;
        if (!additionalMessage.isEmpty())
            additionalMessageString = additionalMessage.toString();

        if (additionalMessageString.startsWith(":"))
            additionalMessageString.clear();
        if (!additionalMessageString.isEmpty())
            additionalMessageString = "<p>" + additionalMessageString + "</p>";

        const int questionRes = KMessageBox::warningYesNoCancel(this,
                                "<p>"
                                + window->part()->i18nMessage("Design of object \"%1\" has been modified.", window)
                                .subs(window->partItem()->name()).toString()
                                + "</p><p>" + i18n("Do you want to save changes?") + "</p>"
                                + additionalMessageString /*may be empty*/,
                                QString(),
                                saveChanges,
                                discardChanges);
        if (questionRes == KMessageBox::Cancel) {
#ifndef KEXI_NO_PENDING_DIALOGS
            d->removePendingWindow(window->id());
#endif
            d->insideCloseWindow = false;
            d->windowsToClose.clear(); //give up with 'close all'
            return cancelled;
        }
        if (questionRes == KMessageBox::Yes) {
            //save it
//   if (!window->storeData())
            tristate res = saveObject(window, QString(), true /*dontAsk*/);
            if (!res || ~res) {
//js:TODO show error info; (retry/ignore/cancel)
#ifndef KEXI_NO_PENDING_DIALOGS
                d->removePendingWindow(window->id());
#endif
                d->insideCloseWindow = false;
                d->windowsToClose.clear(); //give up with 'close all'
                return res;
            }
            remove_on_closing = false;
        }
    }

    const int window_id = window->id(); //remember now, because removeObject() can destruct partitem object
    const QString window_partClass = window->partItem()->partClass();
    if (remove_on_closing) {
        //we won't save this object, and it was never saved -remove it
        if (!removeObject(window->partItem(), true)) {
#ifndef KEXI_NO_PENDING_DIALOGS
            d->removePendingWindow(window->id());
#endif
            //msg?
            //TODO: ask if we'd continue and return true/false
            d->insideCloseWindow = false;
            d->windowsToClose.clear(); //give up with 'close all'
            return false;
        }
    } else {
        //not dirty now
        if (d->navigator) {
            d->navigator->updateItemName(*window->partItem(), false);
        }
    }

    d->removeWindow(window_id);
    QWidget *windowContainer = window->parentWidget();
    d->mainWidget->tabWidget()->removeTab(
        d->mainWidget->tabWidget()->indexOf(windowContainer));

#ifndef KEXI_NO_QUICK_PRINTING
    //also remove from 'print setup dialogs' cache, if needed
    int printedObjectID = 0;
    if (d->pageSetupWindowItemID2dataItemID_map.contains(window_id))
        printedObjectID = d->pageSetupWindowItemID2dataItemID_map[ window_id ];
    d->pageSetupWindows.remove(printedObjectID);
#endif

    KXMLGUIClient *client = window->commonGUIClient();
    KXMLGUIClient *viewClient = window->guiClient();
    if (d->curWindowGUIClient == client) {
        d->curWindowGUIClient = 0;
    }
    if (d->curWindowViewGUIClient == viewClient) {
        d->curWindowViewGUIClient = 0;
    }
    if (client) {
        //sanity: ouch, it is not removed yet? - do it now
        if (d->closedWindowGUIClient && d->closedWindowGUIClient != client)
            guiFactory()->removeClient(d->closedWindowGUIClient);
        if (d->openedWindowsCount() == 0) {//now there is no dialogs - remove client RIGHT NOW!
            d->closedWindowGUIClient = 0;
            guiFactory()->removeClient(client);
        } else {
            //remember this - and MAYBE remove later, if needed
            d->closedWindowGUIClient = client;
        }
    }
    if (viewClient) {
        //sanity: ouch, it is not removed yet? - do it now
        if (d->closedWindowViewGUIClient && d->closedWindowViewGUIClient != viewClient)
            guiFactory()->removeClient(d->closedWindowViewGUIClient);
        if (d->openedWindowsCount() == 0) {//now there is no dialogs - remove client RIGHT NOW!
            d->closedWindowViewGUIClient = 0;
            guiFactory()->removeClient(viewClient);
        } else {
            //remember this - and MAYBE remove later, if needed
            d->closedWindowViewGUIClient = viewClient;
        }
    }

    delete windowContainer;

    //focus navigator if nothing else available
    if (d->openedWindowsCount() == 0) {
        if (d->navigator) {
            d->navigator->setFocus();
        }
        d->updatePropEditorVisibility(Kexi::NoViewMode);
    }

    invalidateActions();
    d->insideCloseWindow = false;
    if (!d->windowsToClose.isEmpty()) {//continue 'close all'
        KexiWindow* w = d->windowsToClose.takeAt(0);
        closeWindow(w, true);
    }

#ifndef KEXI_NO_PENDING_DIALOGS
    d->removePendingWindow(window_id);

    //perform pending global action that was suspended:
    if (!d->pendingWindowsExist()) {
        d->executeActionWhenPendingJobsAreFinished();
    }
#endif
    showTabIfNeeded();
    return true;
}

QWidget* KexiMainWindow::findWindow(QWidget *w)
{
    while (w && !acceptsSharedActions(w)) {
        if (w == d->propEditorDockWidget)
            return currentWindow();
        w = w->parentWidget();
    }
    return w;
}

bool KexiMainWindow::acceptsSharedActions(QObject *w)
{
    return w->inherits("KexiWindow") || w->inherits("KexiView");
}

#if 0 // remove?
bool KexiMainWindow::eventFilter(QObject *obj, QEvent * e)
{
    return KexiMainWindowSuper::eventFilter(obj, e);
}
#endif

bool KexiMainWindow::openingAllowed(KexiPart::Item* item, Kexi::ViewMode viewMode, QString* errorMessage)
{
    kDebug() << viewMode;
    //! @todo this can be more complex once we deliver ACLs...
    if (!d->userMode)
        return true;
    KexiPart::Part * part = Kexi::partManager().partForClass(item->partClass());
    if (!part) {
        if (errorMessage) {
            *errorMessage = Kexi::partManager().errorMsg();
        }
    }
    kDebug() << part << item->partClass();
    if (part)
        kDebug() << item->partClass() << part->info()->supportedUserViewModes();
    return part && (part->info()->supportedUserViewModes() & viewMode);
}

KexiWindow *
KexiMainWindow::openObject(const QString& partClass, const QString& name,
                           Kexi::ViewMode viewMode, bool &openingCancelled, QMap<QString, QVariant>* staticObjectArgs)
{
    KexiPart::Item *item = d->prj->itemForClass(partClass, name);
    if (!item)
        return 0;
    return openObject(item, viewMode, openingCancelled, staticObjectArgs);
}

KexiWindow *
KexiMainWindow::openObject(KexiPart::Item* item, Kexi::ViewMode viewMode, bool &openingCancelled,
                           QMap<QString, QVariant>* staticObjectArgs, QString* errorMessage)
{
    if (!openingAllowed(item, viewMode, errorMessage)) {
        if (errorMessage)
            *errorMessage = i18nc(
                                "opening is not allowed in \"data view/design view/text view\" mode",
                                "opening is not allowed in \"%1\" mode", Kexi::nameForViewMode(viewMode));
        openingCancelled = true;
        return 0;
    }
    kDebug() << d->prj << item;

    if (!d->prj || !item)
        return 0;
    KexiUtils::WaitCursor wait;
#ifndef KEXI_NO_PENDING_DIALOGS
    Private::PendingJobType pendingType;
    KexiWindow *window = d->openedWindowFor(item, pendingType);
    if (pendingType != Private::NoJob) {
        openingCancelled = true;
        return 0;
    }
#else
    KexiWindow *window = d->openedWindowFor(item);
#endif
    openingCancelled = false;

    bool needsUpdateViewGUIClient = true;
    bool alreadyOpened = false;
    KexiWindowContainer *windowContainer = 0;
    if (window) {
        //window->activate();
        if (viewMode != window->currentViewMode()) {
            if (true != switchToViewMode(*window, viewMode))
                return 0;
        } else
            activateWindow(*window);
        needsUpdateViewGUIClient = false;
        alreadyOpened = true;
    } else {
        KexiPart::Part *part = Kexi::partManager().partForClass(item->partClass());
        d->updatePropEditorVisibility(viewMode, part ? part->info() : 0);
        //update tabs before opening
        updateCustomPropertyPanelTabs(currentWindow() ? currentWindow()->part() : 0,
                                      currentWindow() ? currentWindow()->currentViewMode() : Kexi::NoViewMode,
                                      part, viewMode);

        // open new tab earlier
        windowContainer = new KexiWindowContainer(d->mainWidget->tabWidget());
        const int tabIndex = d->mainWidget->tabWidget()->addTab(
            windowContainer,
            KIcon(part ? part->info()->itemIconName() : QString()),
            item->captionOrName());
        d->mainWidget->tabWidget()->setTabToolTip(tabIndex, KexiPart::fullCaptionForItem(*item, part));
        QString whatsThisText;
        if (part) {
            whatsThisText = i18n("Tab for \"%1\" (%2).",
                                 item->captionOrName(), part->info()->instanceCaption());
        }
        else {
            whatsThisText = i18n("Tab for \"%1\".", item->captionOrName());
        }
        d->mainWidget->tabWidget()->setTabWhatsThis(tabIndex, whatsThisText);
        d->mainWidget->tabWidget()->setCurrentWidget(windowContainer);

#ifndef KEXI_NO_PENDING_DIALOGS
        d->addItemToPendingWindows(item, Private::WindowOpeningJob);
#endif
//  window = d->prj->openObject(d->mainWidget->tabWidget(), *item, viewMode, staticObjectArgs);
        window = d->prj->openObject(windowContainer, *item, viewMode, staticObjectArgs);
        if (window) {
            windowContainer->setWindow(window);
            // update text and icon
            d->mainWidget->tabWidget()->setTabText(
                d->mainWidget->tabWidget()->indexOf(windowContainer),
                window->windowTitle());
            d->mainWidget->tabWidget()->setTabIcon(
                d->mainWidget->tabWidget()->indexOf(windowContainer),
                window->windowIcon());
        }
    }

    if (!window || !activateWindow(*window)) {
#ifndef KEXI_NO_PENDING_DIALOGS
        d->removePendingWindow(item->identifier());
#endif
        d->mainWidget->tabWidget()->removeTab(
            d->mainWidget->tabWidget()->indexOf(windowContainer));
        delete windowContainer;
        updateCustomPropertyPanelTabs(0, Kexi::NoViewMode); //revert
        //js TODO: add error msg...
        return 0;
    }

    if (needsUpdateViewGUIClient /*&& !d->userMode*/) {
        //view changed: switch to this view's gui client
        KXMLGUIClient *viewClient = window->guiClient();
        updateWindowViewGUIClient(viewClient);
        if (d->curWindowViewGUIClient && !viewClient)
            guiFactory()->removeClient(d->curWindowViewGUIClient);
        d->curWindowViewGUIClient = viewClient; //remember
    }

//UNUSED invalidateViewModeActions();
    if (viewMode != window->currentViewMode())
        invalidateSharedActions();

#ifndef KEXI_NO_PENDING_DIALOGS
    d->removePendingWindow(window->id());

    //perform pending global action that was suspended:
    if (!d->pendingWindowsExist()) {
        d->executeActionWhenPendingJobsAreFinished();
    }
#endif
    if (window && !alreadyOpened) {
//  window->setParent(d->tabWidget);
//  KexiWindow* previousWindow = currentWindow();
//moved  d->mainWidget->tabWidget()->addTab(window, window->windowIcon(), window->windowTitle());
//moved  d->mainWidget->tabWidget()->setCurrentWidget(window);
        // Call switchToViewMode() and propertySetSwitched() again here because
        // this is the time when then new window is the current one - previous call did nothing.
        switchToViewMode(*window, window->currentViewMode());
        currentWindow()->selectedView()->propertySetSwitched();
//  activeWindowChanged(window, previousWindow);
    }
    invalidateProjectWideActions();
    showDesignTabIfNeeded(item->partClass(), viewMode);
    setDesignTabIfNeeded(item->partClass());
    return window;
}

KexiWindow *
KexiMainWindow::openObjectFromNavigator(KexiPart::Item* item, Kexi::ViewMode viewMode)
{
    bool openingCancelled;
    return openObjectFromNavigator(item, viewMode, openingCancelled);
}

KexiWindow *
KexiMainWindow::openObjectFromNavigator(KexiPart::Item* item, Kexi::ViewMode viewMode,
                                        bool &openingCancelled)
{
    if (!openingAllowed(item, viewMode)) {
        openingCancelled = true;
        return 0;
    }
    if (!d->prj || !item)
        return 0;
#ifndef KEXI_NO_PENDING_DIALOGS
    Private::PendingJobType pendingType;
    KexiWindow *window = d->openedWindowFor(item, pendingType);
    if (pendingType != Private::NoJob) {
        openingCancelled = true;
        return 0;
    }
#else
    KexiWindow *window = d->openedWindowFor(item);
#endif
    openingCancelled = false;
    if (window) {
        if (activateWindow(*window)) {//item->identifier())) {//just activate
//UNUSED   invalidateViewModeActions();
            return window;
        }
    }
    //if DataViewMode is not supported, try Design, then Text mode (currently useful for script part)
    KexiPart::Part *part = Kexi::partManager().partForClass(item->partClass());
    if (!part)
        return 0;
    if (viewMode == Kexi::DataViewMode && !(part->info()->supportedViewModes() & Kexi::DataViewMode)) {
        if (part->info()->supportedViewModes() & Kexi::DesignViewMode)
            return openObjectFromNavigator(item, Kexi::DesignViewMode, openingCancelled);
        else if (part->info()->supportedViewModes() & Kexi::TextViewMode)
            return openObjectFromNavigator(item, Kexi::TextViewMode, openingCancelled);
    }
    //do the same as in openObject()
    return openObject(item, viewMode, openingCancelled);
}

tristate KexiMainWindow::closeObject(KexiPart::Item* item)
{
#ifndef KEXI_NO_PENDING_DIALOGS
    Private::PendingJobType pendingType;
    KexiWindow *window = d->openedWindowFor(item, pendingType);
    if (pendingType == Private::WindowClosingJob)
        return true;
    else if (pendingType == Private::WindowOpeningJob)
        return cancelled;
#else
    KexiWindow *window = d->openedWindowFor(item);
#endif
    if (!window)
        return cancelled;
    return closeWindow(window);
}

bool KexiMainWindow::newObject(KexiPart::Info *info, bool& openingCancelled)
{
    if (d->userMode) {
        openingCancelled = true;
        return false;
    }
    openingCancelled = false;
    if (!d->prj || !info)
        return false;
    KexiPart::Part *part = Kexi::partManager().partForClass(info->partClass());
    if (!part)
        return false;

    KexiPart::Item *it = d->prj->createPartItem(info);
    if (!it) {
        //! @todo error
        return false;
    }

    if (!it->neverSaved()) { //only add stored objects to the browser
        d->navigator->model()->slotAddItem(*it);
    }
    return openObject(it, Kexi::DesignViewMode, openingCancelled);
}

tristate KexiMainWindow::removeObject(KexiPart::Item *item, bool dontAsk)
{
    if (d->userMode)
        return cancelled;
    if (!d->prj || !item)
        return false;

    KexiPart::Part *part = Kexi::partManager().partForClass(item->partClass());
    if (!part)
        return false;

    if (!dontAsk) {
        if (KMessageBox::No == KMessageBox::warningYesNo(this,
                "<p>" + i18n("Do you want to permanently delete:\n"
                             "%1\n"
                             "If you click \"Delete\", you will not be able to undo the deletion.",
                             "</p><p>" + part->info()->instanceCaption() + " \"" + item->name() + "\"?</p>"),
                0, KGuiItem(i18n("Delete"), "edit-delete"), KStandardGuiItem::no())) {
            return cancelled;
        }
    }

    tristate res = true;
#ifndef KEXI_NO_QUICK_PRINTING
    //also close 'print setup' dialog for this item, if any
// int printedObjectID = 0;
// if (d->pageSetupWindowItemID2dataItemID_map.contains(item->identifier()))
//  printedObjectID = d->pageSetupWindowItemID2dataItemID_map[ item->identifier() ];
    KexiWindow * pageSetupWindow = d->pageSetupWindows[ item->identifier()];
    const bool oldInsideCloseWindow = d->insideCloseWindow;
    {
        d->insideCloseWindow = false;
        if (pageSetupWindow)
            res = closeWindow(pageSetupWindow);
    }
    d->insideCloseWindow = oldInsideCloseWindow;
    if (!res || ~res) {
        return res;
    }
#endif

#ifndef KEXI_NO_PENDING_DIALOGS
    Private::PendingJobType pendingType;
    KexiWindow *window = d->openedWindowFor(item, pendingType);
    if (pendingType != Private::NoJob) {
        return cancelled;
    }
#else
    KexiWindow *window = d->openedWindowFor(item);
#endif

    if (window) {//close existing window
        const bool tmp = d->forceWindowClosing;
        /*const bool remove_on_closing = */window->partItem()->neverSaved();
        d->forceWindowClosing = true;
        res = closeWindow(window);
        d->forceWindowClosing = tmp; //restore
        if (!res || ~res) {
            return res;
        }
    }

#ifndef KEXI_NO_QUICK_PRINTING
    //in case the dialog is a 'print setup' dialog, also update d->pageSetupWindows
    int dataItemID = d->pageSetupWindowItemID2dataItemID_map[item->identifier()];
    d->pageSetupWindowItemID2dataItemID_map.remove(item->identifier());
    d->pageSetupWindows.remove(dataItemID);
#endif

    if (!d->prj->removeObject(*item)) {
        //TODO(js) better msg
        showSorryMessage(i18n("Could not remove object."));
        return false;
    }
    return true;
}

void KexiMainWindow::renameObject(KexiPart::Item *item, const QString& _newName, bool &success)
{
    if (d->userMode) {
        success = false;
        return;
    }
    QString newName = _newName.trimmed();
    if (newName.isEmpty()) {
        showSorryMessage(i18n("Could not set empty name for this object."));
        success = false;
        return;
    }
    enableMessages(false); //to avoid double messages
    const bool res = d->prj->renameObject(*item, newName);
    enableMessages(true);
    if (!res) {
        showErrorMessage(d->prj, i18n("Renaming object \"%1\" failed.", newName));
        success = false;
        return;
    }
}

void KexiMainWindow::slotObjectRenamed(const KexiPart::Item &item, const QString& /*oldName*/)
{
#ifndef KEXI_NO_PENDING_DIALOGS
    Private::PendingJobType pendingType;
    KexiWindow *window = d->openedWindowFor(&item, pendingType);
    if (pendingType != Private::NoJob)
        return;
#else
    KexiWindow *window = d->openedWindowFor(&item);
#endif
    if (!window)
        return;

    //change item
    window->updateCaption();
    if (static_cast<KexiWindow*>(currentWindow()) == window)//optionally, update app. caption
        updateAppCaption();
}

void KexiMainWindow::acceptPropertySetEditing()
{
    if (d->propEditor)
        d->propEditor->editor()->acceptInput();
}

void KexiMainWindow::propertySetSwitched(KexiWindow *window, bool force,
        bool preservePrevSelection, bool sortedProperties, const QByteArray& propertyToSelect)
{
    KexiWindow* _currentWindow = currentWindow();
    kDebug() << "currentWindow(): "
    << (_currentWindow ? _currentWindow->windowTitle() : QString("NULL"))
    << " window: " << (window ? window->windowTitle() : QString("NULL"));
    if (_currentWindow && _currentWindow != window) {
        d->propertySet = 0; //we'll need to move to another prop. set
        return;
    }
    if (d->propEditor) {
        KoProperty::Set *newSet = _currentWindow ? _currentWindow->propertySet() : 0;
        if (!newSet || (force || static_cast<KoProperty::Set*>(d->propertySet) != newSet)) {
            d->propertySet = newSet;
            if (preservePrevSelection || force) {
                KoProperty::EditorView::SetOptions options = KoProperty::EditorView::ExpandChildItems;
                if (preservePrevSelection) {
                    options |= KoProperty::EditorView::PreservePreviousSelection;
                }
                if (sortedProperties) {
                    options |= KoProperty::EditorView::AlphabeticalOrder;
                }

                if (propertyToSelect.isEmpty()) {
                    d->propEditor->editor()->changeSet(d->propertySet, options);
                }
                else {
                    d->propEditor->editor()->changeSet(d->propertySet, propertyToSelect, options);
                }
            }
        }
/*moved to d->updatePropEditorVisibility()
        const bool inDesignMode = _currentWindow && _currentWindow->currentViewMode() == Kexi::DesignViewMode;
        if (   (newSet && inDesignMode)
            || (!newSet && inDesignMode && _currentWindow->part()->info()->isPropertyEditorAlwaysVisibleInDesignMode()))
        {
            d->propEditorDockWidget->setVisible(true);
        }
        else if (!inDesignMode) {
            d->propEditorDockWidget->setVisible(false);
        }*/
    }
}

void KexiMainWindow::slotDirtyFlagChanged(KexiWindow* window)
{
    KexiPart::Item *item = window->partItem();
    //update text in navigator and app. caption
    if (!d->userMode) {
        d->navigator->updateItemName(*item, window->isDirty());
    }

    invalidateActions();
    updateAppCaption();
    d->mainWidget->tabWidget()->setTabText(
        d->mainWidget->tabWidget()->indexOf(window->parentWidget()),
        window->windowTitle());
}

void KexiMainWindow::slotTipOfTheDay()
{
    //todo
}

void KexiMainWindow::slotImportantInfo()
{
    importantInfo(false);
}

void KexiMainWindow::slotStartFeedbackAgent()
{
#ifndef KEXI_NO_FEEDBACK_AGENT
#ifdef FEEDBACK_CLASS
    const KAboutData* about = KApplication::kApplication()->aboutData();
    FEEDBACK_CLASS* wizard = new FEEDBACK_CLASS(about->programName(),
            about->version(), 0, 0, 0, FEEDBACK_CLASS::AllPages);

    if (wizard->exec()) {
        KToolInvocation::invokeMailer("kexi-reports-dummy@kexi.org",
                                      QString(), QString(),
                                      about->appName() + QString::fromLatin1(" [feedback]"),
                                      wizard->feedbackDocument().toString(2).local8Bit());
    }

    delete wizard;
#endif
#endif
}

void KexiMainWindow::importantInfo(bool /*onStartup*/)
{
#if 0
    if (onStartup && !d->showImportantInfoOnStartup)
        return;

    QString key = QString("showImportantInfo %1").arg(KEXI_VERSION_STRING);
    d->config->setGroup("Startup");
    bool show = d->config->readBoolEntry(key, true);

    if (show || !onStartup) { //if !onStartup - dialog is always shown
        d->config->setGroup("TipOfDay");
        if (!d->config->hasKey("RunOnStart"))
            d->config->writeEntry("RunOnStart", true);

        QString lang = KGlobal::locale()->language();
        QString fname = locate("data", QString("kexi/readme_") + lang);
        if (fname.isEmpty())//back to default
            fname = locate("data", "kexi/readme_en");
        KTipDialog tipDialog(new KTipDatabase(QString()), 0);
        tipDialog.setWindowTitle(i18n("Important Information"));
        QObjectList *l = tipDialog.queryList("KPushButton");  //hack: hide <- -> buttons
        int i = 0;
        for (QObjectListIt it(*l); it.current() && i < 2; ++it, i++)
            static_cast<KPushButton*>(it.current())->hide();
        QFile f(fname);
        if (f.open(QIODevice::ReadOnly)) {
            QTextStream ts(&f);
            ts.setCodec(KGlobal::locale()->codecForEncoding());
            QTextBrowser *tb = KexiUtils::findFirstChild<KTextBrowser>(&tipDialog, "KTextBrowser");
            if (tb) {
                tb->setText(QString("<qt>%1</qt>").arg(ts.read()));
            }
            f.close();
        }

        tipDialog.adjustSize();
        QRect desk = QApplication::desktop()->screenGeometry(QApplication::desktop()->screenNumber(this));
        tipDialog.resize(qMax(tipDialog.width(), desk.width()*3 / 5), qMax(tipDialog.height(), desk.height()*3 / 5));
        KDialog::centerOnScreen(&tipDialog);
        tipDialog.setModal(true);
        tipDialog.exec();
        //a hack: get user's settings
        d->config->setGroup("TipOfDay");
        show = d->config->readBoolEntry("RunOnStart", show);
    }

    //write our settings back
    d->config->setGroup("Startup");
    d->config->writeEntry(key, show);
    d->showImportantInfoOnStartup = false;
#endif
}

bool KexiMainWindow::userMode() const
{
    return d->userMode;
}

bool
KexiMainWindow::setupUserMode(KexiProjectData *projectData)
{
// Kexi::tempShowMacros() = true;
// Kexi::tempShowScripts() = true;
    if (!projectData)
        return false;

    createKexiProject(*projectData); //initialize project
// d->prj->setFinal(true);         //announce that we are in fianl mode

    tristate res = d->prj->open();             //try to open database
    if (!res || ~res) {
        delete d->prj;
        d->prj = 0;
        return false;
    }

#if 0 //todo reenable; autoopen objects are handled elsewhere
    KexiDB::TableSchema *sch = d->prj->dbConnection()->tableSchema("kexi__final");
    QString err_msg = i18n("Could not start project \"%1\" in Final Mode.",
                           static_cast<KexiDB::SchemaData*>(projectData)->name());
    if (!sch) {
        hide();
        showErrorMessage(err_msg, i18n("No Final Mode data found."));
        return false;
    }

    KexiDB::Cursor *c = d->prj->dbConnection()->executeQuery(*sch);
    if (!c) {
        hide();
        showErrorMessage(err_msg, i18n("Error reading Final Mode data."));
        return false;
    }

    QString startupPart;
    QString startupItem;
    while (c->moveNext()) {
        kDebug() << "KexiMainWinImpl::setupUserMode(): property: [" << c->value(1).toString() << "] " << c->value(2).toString();
        if (c->value(1).toString() == "startup-part")
            startupPart = c->value(2).toString();
        else if (c->value(1).toString() == "startup-item")
            startupItem = c->value(2).toString();
        else if (c->value(1).toString() == "mainxmlui")
            setXML(c->value(2).toString());
    }
    d->prj->dbConnection()->deleteCursor(c);

    kDebug() << "KexiMainWinImpl::setupUserMode(): part: " << startupPart;
    kDebug() << "KexiMainWinImpl::setupUserMode(): item: " << startupItem;

    setupActions();
    setupUserActions();
    guiFactory()->addClient(this);
    setStandardToolBarMenuEnabled(false);
    setHelpMenuEnabled(false);

    KexiPart::Info *i = Kexi::partManager().infoForClass(startupPart);
    if (!i) {
        hide();
        showErrorMessage(err_msg, i18n("Specified plugin does not exist."));
        return false;
    }

    Kexi::partManager().part(i);
    KexiPart::Item *item = d->prj->item(i, startupItem);
    bool openingCancelled;
    if (!openObject(item, Kexi::DataViewMode, openingCancelled) && !openingCancelled) {
        hide();
        showErrorMessage(err_msg, i18n("Specified object could not be opened."));
        return false;
    }

    QWidget::setWindowTitle("MyApp");//TODO
#endif
    return true;
}

void
KexiMainWindow::setupUserActions()
{
#if 0 //unused for now
    KexiDB::Cursor *c = d->prj->dbConnection()->executeQuery("SELECT p_id, name, text, icon, method, arguments FROM kexi__useractions WHERE scope = 0");
    if (!c)
        return;

    while (c->moveNext()) {
        KexiUserAction::fromCurrentRecord(this, actionCollection(), c);
    }
    d->prj->dbConnection()->deleteCursor(c);
    /*
      KexiUserAction *a1 = new KexiUserAction(this, actionCollection(), "user_dataview", "Change to dataview", "table");
      Arguments args;
      args.append(QVariant("kexi/table"));
      args.append(QVariant("persons"));
      a1->setMethod(KexiUserAction::OpenObject, args);
    */
#endif
}

void KexiMainWindow::slotToolsImportProject()
{
    if (d->tabbedToolBar)
        d->tabbedToolBar->hideMainMenu();
    showProjectMigrationWizard(QString(), QString());
}

void KexiMainWindow::slotToolsImportTables()
{
    if (project()) {
        QDialog *dlg = KexiInternalPart::createModalDialogInstance("migration", "importtable", this, 0);
        if (!dlg)
            return; //error msg has been shown by KexiInternalPart
            
            const int result = dlg->exec();
        delete dlg;
        //raise();
        if (result != QDialog::Accepted)
            return;
    }
}

void KexiMainWindow::slotToolsCompactDatabase()
{
    KexiProjectData *data = 0;
    KexiDB::Driver *drv = 0;
    const bool projectWasOpened = d->prj;

    if (!d->prj) {
        KexiProjectSet fake;
        KexiStartupDialog dlg(
            KexiStartupDialog::OpenExisting, 0, Kexi::connset(), fake,
            this);

        if (dlg.exec() != QDialog::Accepted)
            return;

        if (dlg.selectedFileName().isEmpty()) {
//! @todo add support for server based if needed?
            return;
        }
        KexiDB::ConnectionData cdata;
        cdata.setFileName(dlg.selectedFileName());

        //detect driver name for the selected file
        KexiStartupData::Import detectedImportAction;
        tristate res = KexiStartupHandler::detectActionForFile(
                           detectedImportAction, cdata.driverName,
                           "" /*suggestedDriverName*/, cdata.fileName(), 0,
                           KexiStartupHandler::SkipMessages | KexiStartupHandler::ThisIsAProjectFile
                           | KexiStartupHandler::DontConvert);

        if (true == res && !detectedImportAction)
            drv = Kexi::driverManager().driver(cdata.driverName);
        if (!drv || !(drv->features() & KexiDB::Driver::CompactingDatabaseSupported)) {
            KMessageBox::information(this, "<qt>" +
                                     i18n("Compacting database file <nobr>\"%1\"</nobr> is not supported.",
                                          QDir::convertSeparators(cdata.fileName())));
            return;
        }
        data = new KexiProjectData(cdata);
    } else {
        //sanity
        if (!(d->prj && d->prj->dbConnection()
                && (d->prj->dbConnection()->driver()->features() & KexiDB::Driver::CompactingDatabaseSupported)))
            return;

        if (KMessageBox::Continue != KMessageBox::warningContinueCancel(this,
                i18n("The current project has to be closed before compacting the database. "
                     "It will be open again after compacting.\n\nDo you want to continue?")))
            return;

        data = new KexiProjectData(*d->prj->data()); // a copy
        drv = d->prj->dbConnection()->driver();
        const tristate res = closeProject();
        if (~res || !res) {
            delete data;
            return;
        }
    }

    if (!drv->adminTools().vacuum(*data->connectionData(), data->databaseName())) {
      //err msg
      showErrorMessage(&drv->adminTools());
    }

    if (projectWasOpened)
      openProject(*data);

    delete data;
}

tristate KexiMainWindow::showProjectMigrationWizard(
    const QString& mimeType, const QString& databaseName, const KexiDB::ConnectionData *cdata)
{
    //pass arguments
    QMap<QString, QString> args;
    args.insert("mimeType", mimeType);
    args.insert("databaseName", databaseName);
    if (cdata) { //pass ConnectionData serialized as a string...
        QString str;
        KexiUtils::serializeMap(KexiDB::toMap(*cdata), str);
        args.insert("connectionData", str);
    }

    QDialog *dlg = KexiInternalPart::createModalDialogInstance("migration", "migration", this, 0, &args);
    if (!dlg)
        return false; //error msg has been shown by KexiInternalPart

    const int result = dlg->exec();
    delete dlg;
    //raise();
    if (result != QDialog::Accepted)
        return cancelled;

    //open imported project in a new Kexi instance
    QString destinationDatabaseName(args["destinationDatabaseName"]);
    QString fileName, destinationConnectionShortcut, dbName;
    if (!destinationDatabaseName.isEmpty()) {
        if (args.contains("destinationConnectionShortcut")) {
            // server-based
            destinationConnectionShortcut = args["destinationConnectionShortcut"];
        } else {
            // file-based
            fileName = destinationDatabaseName;
            destinationDatabaseName.clear();
        }
        tristate res = openProject(fileName, destinationConnectionShortcut,
                                   destinationDatabaseName);
        raise();
        return res;
//   KexiDB::ConnectionData *connData = new KexiDB::ConnectionData();
//   KexiDB::fromMap( KexiUtils::deserializeMap( args["destinationConnectionData"] ), *connData );
//  return openProject(destinationFileName, 0);
    }
    return true;
}

tristate KexiMainWindow::executeItem(KexiPart::Item* item)
{
    KexiPart::Info *info = item ? Kexi::partManager().infoForClass(item->partClass()) : 0;
    if ((! info) || (! info->isExecuteSupported()))
        return false;
    KexiPart::Part *part = Kexi::partManager().part(info);
    if (!part)
        return false;
    return part->execute(item);
}

void KexiMainWindow::slotProjectImportDataTable()
{
//! @todo allow data appending (it is not possible now)
    if (d->userMode)
        return;
    QMap<QString, QString> args;
    args.insert("sourceType", "file");
    QDialog *dlg = KexiInternalPart::createModalDialogInstance(
                       "csv_importexport", "KexiCSVImportDialog", this, 0, &args);
    if (!dlg)
        return; //error msg has been shown by KexiInternalPart
    dlg->exec();
    delete dlg;
}

tristate KexiMainWindow::executeCustomActionForObject(KexiPart::Item* item,
        const QString& actionName)
{
    if (actionName == "exportToCSV")
        return exportItemAsDataTable(item);
    else if (actionName == "copyToClipboardAsCSV")
        return copyItemToClipboardAsDataTable(item);

    kWarning() << "KexiMainWindow::executeCustomActionForObject(): no such action: "
    << actionName;
    return false;
}

tristate KexiMainWindow::exportItemAsDataTable(KexiPart::Item* item)
{
    if (!item)
        return false;
//! @todo: check if changes to this are saved, if not: ask for saving
//! @todo: accept record changes...

    QMap<QString, QString> args;
    args.insert("destinationType", "file");
    args.insert("itemId", QString::number(item->identifier()));
    QDialog *dlg = KexiInternalPart::createModalDialogInstance(
                       "csv_importexport", "KexiCSVExportWizard", this, 0, &args);
    if (!dlg)
        return false; //error msg has been shown by KexiInternalPart
    int result = dlg->exec();
    delete dlg;
    return result == QDialog::Rejected ? cancelled : true;
}

bool KexiMainWindow::printItem(KexiPart::Item* item, const QString& titleText)
{
#ifdef __GNUC__
#warning TODO printItem(item, KexiSimplePrintingSettings::load(), titleText);
#else
#pragma WARNING( TODO printItem(item, KexiSimplePrintingSettings::load(), titleText); )
#endif
    Q_UNUSED(item)
    Q_UNUSED(titleText)
    return false;
}

tristate KexiMainWindow::printItem(KexiPart::Item* item)
{
    return printItem(item, QString());
}

bool KexiMainWindow::printPreviewForItem(KexiPart::Item* item, const QString& titleText, bool reload)
{
#ifdef __GNUC__
#warning TODO printPreviewForItem(item, KexiSimplePrintingSettings::load(), titleText, reload);
#else
#pragma WARNING( TODO printPreviewForItem(item, KexiSimplePrintingSettings::load(), titleText, reload); )
#endif
    Q_UNUSED(item)
    Q_UNUSED(titleText)
    Q_UNUSED(reload)
    return false;
}

tristate KexiMainWindow::printPreviewForItem(KexiPart::Item* item)
{
    return printPreviewForItem(item, QString(),
//! @todo store cached record data?
                               true/*reload*/);
}

tristate KexiMainWindow::showPageSetupForItem(KexiPart::Item* item)
{
    Q_UNUSED(item)
//! @todo: check if changes to this object's design are saved, if not: ask for saving
//! @todo: accept record changes...
#ifdef __GNUC__
#warning TODO printActionForItem(item, PageSetupForItem);
#else
#pragma WARNING( TODO printActionForItem(item, PageSetupForItem); )
#endif
    return false;
}

#ifdef __GNUC__
#warning TODO reenable printItem() when ported
#else
#pragma WARNING( TODO reenable printItem() when ported )
#endif
#if 0//TODO
bool KexiMainWindow::printItem(KexiPart::Item* item, const KexiSimplePrintingSettings& settings,
                               const QString& titleText)
{
//! @todo: check if changes to this object's design are saved, if not: ask for saving
//! @todo: accept record changes...
    KexiSimplePrintingCommand cmd(this, item->identifier());
    //modal
    return cmd.print(settings, titleText);
}

bool KexiMainWindow::printPreviewForItem(KexiPart::Item* item,
        const KexiSimplePrintingSettings& settings, const QString& titleText, bool reload)
{
//! @todo: check if changes to this object's design are saved, if not: ask for saving
//! @todo: accept record changes...
    KexiSimplePrintingCommand* cmd = d->openedCustomObjectsForItem<KexiSimplePrintingCommand>(
                                         item, "KexiSimplePrintingCommand");
    if (!cmd) {
        d->addOpenedCustomObjectForItem(
            item,
            cmd = new KexiSimplePrintingCommand(this, item->identifier()),
            "KexiSimplePrintingCommand"
        );
    }
    return cmd->showPrintPreview(settings, titleText, reload);
}

tristate KexiMainWindow::printActionForItem(KexiPart::Item* item, PrintActionType action)
{
    if (!item)
        return false;
    KexiPart::Info *info = Kexi::partManager().infoForClass(item->partClass());
    if (!info->isPrintingSupported())
        return false;

    KexiWindow *printingWindow = d->pageSetupWindows[ item->identifier()];
    if (printingWindow) {
        if (!activateWindow(*printingWindow))
            return false;
        if (action == PreviewItem || action == PrintItem) {
            QTimer::singleShot(0, printingWindow->selectedView(),
                               (action == PreviewItem) ? SLOT(printPreview()) : SLOT(print()));
        }
        return true;
    }

#ifndef KEXI_NO_PENDING_DIALOGS
    Private::PendingJobType pendingType;
    KexiWindow *window = d->openedWindowFor(item, pendingType);
    if (pendingType != Private::NoJob)
        return cancelled;
#else
    KexiWindow *window = d->openedWindowFor(item);
#endif

    if (window) {
        // accept record changes
        QWidget *prevFocusWidget = focusWidget();
        window->setFocus();
        d->action_data_save_row->activate(QAction::Trigger);
        if (prevFocusWidget)
            prevFocusWidget->setFocus();

        // opened: check if changes made to this dialog are saved, if not: ask for saving
        if (window->neverSaved()) //sanity check
            return false;
        if (window->isDirty()) {
            KGuiItem saveChanges(KStandardGuiItem::save());
            saveChanges.setToolTip(i18n("Save changes"));
            saveChanges.setWhatsThis(
                i18n("Pressing this button will save all recent changes made in \"%1\" object.",
                     item->name()));
            KGuiItem doNotSave(KStandardGuiItem::no());
            doNotSave.setWhatsThis(
                i18n("Pressing this button will ignore all unsaved changes made in \"%1\" object.",
                     window->partItem()->name()));

            QString question;
            if (action == PrintItem)
                question = i18n("Do you want to save changes before printing?");
            else if (action == PreviewItem)
                question = i18n("Do you want to save changes before making print preview?");
            else if (action == PageSetupForItem)
                question = i18n("Do you want to save changes before showing page setup?");
            else
                return false;

            const int questionRes = KMessageBox::warningYesNoCancel(this,
                                    "<p>"
                                    + window->part()->i18nMessage("Design of object \"%1\" has been modified.", window)
                                    .subs(item->name())
                                    + "</p><p>" + question + "</p>",
                                    QString(),
                                    saveChanges,
                                    doNotSave);
            if (KMessageBox::Cancel == questionRes)
                return cancelled;
            if (KMessageBox::Yes == questionRes) {
                tristate savingRes = saveObject(window, QString(), true /*dontAsk*/);
                if (true != savingRes)
                    return savingRes;
            }
        }
    }
    KexiPart::Part * printingPart = Kexi::partManager().partForClass("org.kexi-project.simpleprinting");
    if (!printingPart)
        printingPart = new KexiSimplePrintingPart(); //hardcoded as there're no .desktop file
    KexiPart::Item* printingPartItem = d->prj->createPartItem(
                                           printingPart, item->name() //<-- this will look like "table1 : printing" on the window list
                                       );
    QMap<QString, QVariant> staticObjectArgs;
    staticObjectArgs["identifier"] = QString::number(item->identifier());
    if (action == PrintItem)
        staticObjectArgs["action"] = "print";
    else if (action == PreviewItem)
        staticObjectArgs["action"] = "printPreview";
    else if (action == PageSetupForItem)
        staticObjectArgs["action"] = "pageSetup";
    else
        return false;
    bool openingCancelled;
    printingWindow = openObject(printingPartItem, Kexi::DesignViewMode,
                                openingCancelled, &staticObjectArgs);
    if (openingCancelled)
        return cancelled;
    if (!printingWindow) //sanity
        return false;
    d->pageSetupWindows.insert(item->identifier(), printingWindow);
    d->pageSetupWindowItemID2dataItemID_map.insert(
        printingWindow->partItem()->identifier(), item->identifier());

    return true;
}
#endif

void KexiMainWindow::slotEditCopySpecialDataTable()
{
    KexiPart::Item* item = d->navigator->selectedPartItem();
    if (item)
        copyItemToClipboardAsDataTable(item);
}

tristate KexiMainWindow::copyItemToClipboardAsDataTable(KexiPart::Item* item)
{
    if (!item)
        return false;

    QMap<QString, QString> args;
    args.insert("destinationType", "clipboard");
    args.insert("itemId", QString::number(item->identifier()));
    QDialog *dlg = KexiInternalPart::createModalDialogInstance(
                       "csv_importexport", "KexiCSVExportWizard", this, 0, &args);
    if (!dlg)
        return false; //error msg has been shown by KexiInternalPart
    const int result = dlg->exec();
    delete dlg;
    return result == QDialog::Rejected ? cancelled : true;
}

void KexiMainWindow::slotEditPasteSpecialDataTable()
{
//! @todo allow data appending (it is not possible now)
    if (d->userMode)
        return;
    QMap<QString, QString> args;
    args.insert("sourceType", "clipboard");
    QDialog *dlg = KexiInternalPart::createModalDialogInstance(
                       "csv_importexport", "KexiCSVImportDialog", this, 0, &args);
    if (!dlg)
        return; //error msg has been shown by KexiInternalPart
    dlg->exec();
    delete dlg;
}

void KexiMainWindow::slotEditFind()
{
// KexiView *view = d->currentViewSupportingAction("edit_findnext");
    KexiSearchAndReplaceViewInterface* iface = d->currentViewSupportingSearchAndReplaceInterface();
    if (!iface)
        return;
    d->updateFindDialogContents(true/*create if does not exist*/);
    d->findDialog()->setReplaceMode(false);

    d->findDialog()->show();
    d->findDialog()->activateWindow();
    d->findDialog()->raise();
}

void KexiMainWindow::slotEditFind(bool next)
{
    KexiSearchAndReplaceViewInterface* iface = d->currentViewSupportingSearchAndReplaceInterface();
    if (!iface)
        return;
    tristate res = iface->find(
                       d->findDialog()->valueToFind(), d->findDialog()->options(), next);
    if (~res)
        return;
    d->findDialog()->updateMessage(true == res);
//! @todo result
}

void KexiMainWindow::slotEditFindNext()
{
    slotEditFind(true);
}

void KexiMainWindow::slotEditFindPrevious()
{
    slotEditFind(false);
}

void KexiMainWindow::slotEditReplace()
{
    KexiSearchAndReplaceViewInterface* iface = d->currentViewSupportingSearchAndReplaceInterface();
    if (!iface)
        return;
    d->updateFindDialogContents(true/*create if does not exist*/);
    d->findDialog()->setReplaceMode(true);
//! @todo slotEditReplace()
    d->findDialog()->show();
    d->findDialog()->activateWindow();
}

void KexiMainWindow::slotEditReplaceNext()
{
    slotEditReplace(false);
}

void KexiMainWindow::slotEditReplace(bool all)
{
    KexiSearchAndReplaceViewInterface* iface = d->currentViewSupportingSearchAndReplaceInterface();
    if (!iface)
        return;
//! @todo add question: "Do you want to replace every occurrence of \"%1\" with \"%2\"?
//!       You won't be able to undo this." + "Do not ask again".
    tristate res = iface->findNextAndReplace(
                       d->findDialog()->valueToFind(), d->findDialog()->valueToReplaceWith(),
                       d->findDialog()->options(), all);
    d->findDialog()->updateMessage(true == res);
//! @todo result
}

void KexiMainWindow::slotEditReplaceAll()
{
    slotEditReplace(true);
}

/// TMP (until there's true template support)
void KexiMainWindow::slotGetNewStuff()
{
#ifdef HAVE_KNEWSTUFF
    if (!d->newStuff)
        d->newStuff = new KexiNewStuff(this);
    d->newStuff->download();

    //KNS::DownloadDialog::open(newstuff->customEngine(), "kexi/template");
#endif
}

void KexiMainWindow::highlightObject(const QString& partClass, const QString& name)
{
    slotViewNavigator();
    if (!d->prj)
        return;
    KexiPart::Item *item = d->prj->itemForClass(partClass, name);
    if (!item)
        return;
    if (d->navigator) {
        d->navigator->selectItem(*item);
    }
}

void KexiMainWindow::slotPartItemSelectedInNavigator(KexiPart::Item* item)
{
    Q_UNUSED(item);
}

KToolBar *KexiMainWindow::toolBar(const QString& name) const
{
    return d->tabbedToolBar ? d->tabbedToolBar->toolBar(name) : 0;
}

void KexiMainWindow::appendWidgetToToolbar(const QString& name, QWidget* widget)
{
    if (d->tabbedToolBar)
        d->tabbedToolBar->appendWidgetToToolbar(name, widget);
}

void KexiMainWindow::setWidgetVisibleInToolbar(QWidget* widget, bool visible)
{
    if (d->tabbedToolBar)
        d->tabbedToolBar->setWidgetVisibleInToolbar(widget, visible);
}

void KexiMainWindow::addToolBarAction(const QString& toolBarName, QAction *action)
{
    if (d->tabbedToolBar)
        d->tabbedToolBar->addAction(toolBarName, action);
}

void KexiMainWindow::updatePropertyEditorInfoLabel(const QString& textToDisplayForNullSet)
{
    d->propEditor->updateInfoLabelForPropertySet(d->propertySet, textToDisplayForNullSet);
}

void KexiMainWindow::addSearchableModel(KexiSearchableModel *model)
{
    d->tabbedToolBar->addSearchableModel(model);
}

void KexiMainWindow::showDesignTabIfNeeded(const QString &partClass, const Kexi::ViewMode viewMode)
{
    closeTab("");
    if (viewMode == Kexi::DesignViewMode) {
        switch (d->prj->idForClass(partClass)) {
        case KexiPart::FormObjectType: 
            d->tabbedToolBar->showTab("form");
            break;
        case KexiPart::ReportObjectType: 
            d->tabbedToolBar->showTab("report");
            break;
        default: ;
        }
    }
}

void KexiMainWindow::setDesignTabIfNeeded(const QString &partClass)
{
    switch (d->prj->idForClass(partClass)) {
    case KexiPart::FormObjectType: 
        d->tabbedToolBar->setCurrentTab("form"); 
        break;
    case KexiPart::ReportObjectType: 
        d->tabbedToolBar->setCurrentTab("report"); 
        break;
    default:;
    }  
}

void KexiMainWindow::closeTab(const QString &partClass)
{
    switch (d->prj->idForClass(partClass)) {
    case KexiPart::FormObjectType: 
        d->tabbedToolBar->hideTab("form");
        break;
    case KexiPart::ReportObjectType: 
        d->tabbedToolBar->hideTab("report");
        break;
    default:
        d->tabbedToolBar->hideTab("form");
        d->tabbedToolBar->hideTab("report");
    }
}

void KexiMainWindow::showTabIfNeeded()
{
    if (currentWindow()) {
        showDesignTabIfNeeded(currentWindow()->partItem()->partClass(), currentWindow()->currentViewMode());
    } else {
        closeTab("");
    }
}

KexiUserFeedbackAgent* KexiMainWindow::userFeedbackAgent() const
{
    return &d->userFeedback;
}

void KexiMainWindow::toggleFullScreen(bool isFullScreen)
{
    static bool isTabbarRolledDown;

    if (isFullScreen) {
        isTabbarRolledDown = !d->tabbedToolBar->isRolledUp();
        if (isTabbarRolledDown) {
            d->tabbedToolBar->toggleRollDown();
        }
    } else {
        if (isTabbarRolledDown && d->tabbedToolBar->isRolledUp()) {
            d->tabbedToolBar->toggleRollDown();
        }
    }

    KToggleFullScreenAction::setFullScreen(this, isFullScreen);
}

#include "KexiMainWindow.moc"
#include "KexiMainWindow_p.moc"
