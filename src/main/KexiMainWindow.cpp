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

#include "KexiMainWindow.h"
#include "KexiMainWindow_p.h"
#include "kexiactionproxy.h"
#include "kexipartmanager.h"
#include "kexipart.h"
#include "kexipartinfo.h"
#include "kexipartguiclient.h"
#include "kexiproject.h"
#include "kexiprojectdata.h"
#include "kexi.h"
#include "kexiinternalpart.h"
#include "kexiactioncategories.h"
#include "kexifinddialog.h"
#include "kexisearchandreplaceiface.h"
#include "KexiBugReportDialog.h"
#define KEXI_SKIP_REGISTERICONSRESOURCE
#define KEXI_SKIP_SETUPPRIVATEICONSRESOURCE
#include "KexiRegisterResource_p.h"
#include "KexiObjectViewWidget.h"
#include "KexiObjectViewTabWidget.h"
#include <kexiutils/utils.h>
#include <kexiutils_global.h>
#include <KexiStyle.h>
#include <kexiutils/KexiCloseButton.h>
#include <kexiutils/KexiTester.h>
#include <KexiVersion.h>
#include <core/KexiWindow.h>
#include <core/KexiRecentProjects.h>
#include <core/kexiaboutdata.h>
#include <core/KexiCommandLineOptions.h>
#include <KexiIcon.h>
#include <kexi_global.h>
#include <KexiPropertyPaneWidget.h>
#include <widget/utils/kexirecordnavigator.h>
#include <widget/utils/KexiDockableWidget.h>
#include <widget/navigator/KexiProjectNavigator.h>
#include <widget/navigator/KexiProjectModel.h>
#include <widget/KexiNameDialog.h>
#include <widget/KexiNameWidget.h>
#include <widget/KexiDBPasswordDialog.h>
#include "startup/KexiStartup.h"
#include "startup/KexiNewProjectAssistant.h"
#include "startup/KexiOpenProjectAssistant.h"
#include "startup/KexiWelcomeAssistant.h"
#include "startup/KexiImportExportAssistant.h"
#include <KexiAssistantPage.h>

#include <KDbConnection>
#include <KDbConnectionOptions>
#include <KDbCursor>
#include <KDbAdmin>
#include <KDbDriverManager>
#include <KDbObjectNameValidator>

#include <KPropertyEditorView>
#include <KPropertySet>

#include <KActionCollection>
#include <KActionMenu>
#include <KToggleAction>
#include <KStandardShortcut>
#include <KStandardGuiItem>
#include <KConfig>
#include <KShortcutsDialog>
#include <KEditToolBar>
#include <KToggleFullScreenAction>
#include <KIconLoader>
#include <KHelpMenu>
#include <KMultiTabBar>
#include <KLocalizedString>
#include <KMessageBox>
#include <KConfigGroup>
#include <KAcceleratorManager>

#include <QApplication>
#include <QFile>
#include <QProcess>
#include <QToolButton>
#include <QDebug>
#include <QHash>
#include <QStylePainter>
#include <QStyleFactory>
#include <QDesktopWidget>
#include <QResource>
#include <QMenuBar>

#if !defined(KexiVDebug)
# define KexiVDebug if (0) qDebug()
#endif

#ifdef HAVE_KCRASH
#include <kcrash.h>
//! @todo else, add Breakpad? https://phabricator.kde.org/T1642
#endif

KexiDockWidgetStyle::KexiDockWidgetStyle(const QString &baseStyleName)
 : QProxyStyle(baseStyleName)
{
}

KexiDockWidgetStyle::~KexiDockWidgetStyle()
{
}

void KexiDockWidgetStyle::polish(QWidget* widget)
{
    baseStyle()->polish(widget);
    widget->setContentsMargins(0, 0, 0, 0);
}

class Q_DECL_HIDDEN KexiDockWidget::Private
{
public:
    Private() {}
    QSize hint;
};

KexiDockWidget::KexiDockWidget(const QString &_tabText, QWidget *parent)
        : QDockWidget(parent), tabText(_tabText), d(new Private)
{
    // No floatable dockers, Dolphin had problems, we don't want the same...
    // https://bugs.kde.org/show_bug.cgi?id=288629
    // https://bugs.kde.org/show_bug.cgi?id=322299
    setFeatures(QDockWidget::NoDockWidgetFeatures);//DockWidgetClosable);
    setAllowedAreas(Qt::LeftDockWidgetArea|Qt::RightDockWidgetArea);
    setFocusPolicy(Qt::NoFocus);
    if (style()->objectName().compare("windowsvista", Qt::CaseInsensitive) == 0) {
        // windowsvista style has broken accelerator visualization support
        KAcceleratorManager::setNoAccel(this);
    }
    KexiDockWidgetStyle *customStyle = new KexiDockWidgetStyle(style()->objectName());
    customStyle->setParent(this);
    setStyle(customStyle);
    setTitleBarWidget(new QWidget(this)); // hide the title
    layout()->setContentsMargins(0, 0, 0, 0);
    layout()->setSpacing(0);
}

KexiDockWidget::~KexiDockWidget()
{
    delete d;
}

void KexiDockWidget::paintEvent(QPaintEvent *pe)
{
    Q_UNUSED(pe);
    QStylePainter p(this);
    if (isFloating()) {
        QStyleOptionFrame framOpt;
        framOpt.initFrom(this);
        p.drawPrimitive(QStyle::PE_FrameDockWidget, framOpt);
    }

    // Title must be painted after the frame, since the areas overlap, and
    // the title may wish to extend out to all sides (eg. XP style)
    QStyleOptionDockWidget titleOpt;
    initStyleOption(&titleOpt);
    p.drawControl(QStyle::CE_DockWidgetTitle, titleOpt);
}

void KexiDockWidget::setSizeHint(const QSize& hint)
{
    d->hint = hint;
}

QSize KexiDockWidget::sizeHint() const
{
    return d->hint.isValid() ? d->hint : QDockWidget::sizeHint();
}

//-------------------------------------------------

static bool setupIconTheme(KLocalizedString *errorMessage, KLocalizedString *detailsErrorMessage)
{
    // Register kexi resource first to have priority over the standard breeze theme.
    // For example "table" icon exists in both resources.
    if (!registerResource("icons/kexi_breeze.rcc", QStandardPaths::AppDataLocation,
                          QString(), QString(), errorMessage, detailsErrorMessage)
        || !registerGlobalBreezeIconsResource(errorMessage, detailsErrorMessage))
    {
        return false;
    }
    setupBreezeIconTheme();

    // tell KIconLoader an co. about the theme
    KConfigGroup cg(KSharedConfig::openConfig(), "Icons");
    cg.writeEntry("Theme", "breeze");
    cg.sync();
    return true;
}

//! @todo 3.1 replace with KexiStyle
bool setupApplication()
{
#if defined Q_OS_WIN || defined Q_OS_MACOS
    // Only this style matches current KEXI theme and can be supported/tested
    const char *name = "breeze";
    QScopedPointer<QStyle> style(QStyleFactory::create(name));
    if (!style || style->objectName() != name) {
        qWarning() <<
             qPrintable(QString("Could not find application style %1. "
                                "KEXI will not start. Please check if KEXI is properly installed. ")
                                .arg(name));
         return false;
     }
     qApp->setStyle(style.take());
#endif
     return true;
}

//static
int KexiMainWindow::create(const QStringList &arguments, const QString &componentName,
                           const QList<QCommandLineOption> &extraOptions)
{
    qApp->setQuitOnLastWindowClosed(false);

    KLocalizedString::setApplicationDomain("kexi");
    //! @todo KEXI3 app->setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    KexiAboutData aboutData;
    if (!componentName.isEmpty()) {
        aboutData.setComponentName(componentName);
    }
    KAboutData::setApplicationData(aboutData);

    if (!setupApplication()) {
        return 1;
    }

#ifdef HAVE_KCRASH
    KCrash::initialize();
#endif

    KLocalizedString errorMessage;
    KLocalizedString detailsErrorMessage;
    if (!setupIconTheme(&errorMessage, &detailsErrorMessage)
        || !KexiStyle::setupApplication(&errorMessage))
    {
        if (detailsErrorMessage.isEmpty()) {
            KMessageBox::error(nullptr, errorMessage.toString());
        } else {
            KMessageBox::detailedError(nullptr, errorMessage.toString(), detailsErrorMessage.toString());
        }
        qWarning() << qPrintable(errorMessage.toString(Kuit::PlainText));
        return 1;
    }
    QApplication::setWindowIcon(koIcon("kexi"));

    const tristate res = KexiStartupHandler::global()->init(arguments, extraOptions);
    if (!res || ~res) {
        return (~res) ? 0 : 1;
    }
    //qDebug() << "startupActions OK";

    /* Exit requested, e.g. after database removing. */
    if (KexiStartupHandler::global()->action() == KexiStartupData::Exit) {
        return 0;
    }

    KexiMainWindow *win = new KexiMainWindow();
#ifdef KEXI_DEBUG_GUI
    KConfigGroup generalGroup = KSharedConfig::openConfig()->group("General");
    if (generalGroup.readEntry("ShowInternalDebugger", false)) {
        QWidget* debugWindow = KexiUtils::createDebugWindow(win);
        debugWindow->show();
    }
#endif

    if (true != win->startup()) {
        delete win;
        return 1;
    }

    win->restoreSettings();
    win->show();
#ifdef KEXI_DEBUG_GUI
    win->raise();
    static_cast<QWidget*>(win)->activateWindow();
#endif
    /*foreach (QWidget *widget, QApplication::topLevelWidgets()) {
        qDebug() << widget;
    }*/
    return 0;
}

//-------------------------------------------------

KexiMainMenuActionShortcut::KexiMainMenuActionShortcut(const QKeySequence& key,
                                                       QAction *action, QWidget *parent)
    : QShortcut(key, parent)
    , m_action(action)
{
    connect(this, SIGNAL(activated()), this, SLOT(slotActivated()));
}

KexiMainMenuActionShortcut::~KexiMainMenuActionShortcut()
{
}

void KexiMainMenuActionShortcut::slotActivated()
{
    if (!m_action->isEnabled()) {
        return;
    }
    m_action->trigger();
}

//-------------------------------------------------

KexiMainWindow::KexiMainWindow(QWidget *parent)
        : QMainWindow(parent)
        , KexiMainWindowIface()
        , KexiGUIMessageHandler(this)
        , d(new KexiMainWindow::Private(this))
{
    setObjectName("KexiMainWindow");
    setAttribute(Qt::WA_DeleteOnClose);
    kexiTester() << KexiTestObject(this);

    if (d->userMode) {
        //qDebug() << "starting up in the User Mode";
    }
    setAsDefaultHost(); //this is default host now.

    //get informed
    connect(&Kexi::partManager(), SIGNAL(partLoaded(KexiPart::Part*)),
            this, SLOT(slotPartLoaded(KexiPart::Part*)));
    connect(&Kexi::partManager(), SIGNAL(newObjectRequested(KexiPart::Info*)),
            this, SLOT(newObject(KexiPart::Info*)));

    setAcceptDrops(true);
    setupActions();
    setupMainWidget();
    setupMainMenu();
    updateAppCaption();

    if (!d->userMode) {
        setupContextHelp();
        //setupPropertyEditor();
    }

    invalidateActions();
    d->timer.singleShot(0, this, SLOT(slotLastActions()));
    if (KexiStartupHandler::global()->forcedFullScreen()) {
        toggleFullScreen(true);
    }

    // --- global config
    //! @todo move to specialized KexiConfig class
    KConfigGroup tablesGroup(d->config->group("Tables"));
    const int defaultMaxLengthForTextFields = tablesGroup.readEntry("DefaultMaxLengthForTextFields", int(-1));
    if (defaultMaxLengthForTextFields >= 0) {
        KDbField::setDefaultMaxLength(defaultMaxLengthForTextFields);
    }
    // --- /global config
}

KexiMainWindow::~KexiMainWindow()
{
    d->forceWindowClosing = true;
    closeProject();
    delete d;
    Kexi::deleteGlobalObjects();
}

KexiProject *KexiMainWindow::project()
{
    return d->prj;
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
    if (!d->objectViewWidget || !d->objectViewWidget->tabWidget()) {
        return 0;
    }
    return windowForTab(d->objectViewWidget->tabWidget()->currentIndex());
}

KexiWindow* KexiMainWindow::windowForTab(int tabIndex) const
{
    if (!d->objectViewWidget || !d->objectViewWidget->tabWidget())
        return 0;
    return d->objectViewWidget->tabWidget()->window(tabIndex);
}

void KexiMainWindow::setupMainMenuActionShortcut(QAction * action)
{
    if (!action->shortcut().isEmpty()) {
        //foreach(const QKeySequence &shortcut, action->shortcuts()) {
            //(void)new KexiMainMenuActionShortcut(shortcut, action, this);
        //}
    }
}

static void addThreeDotsToActionText(QAction* action)
{
    action->setText(xi18nc("Action name with three dots...", "%1...", action->text()));
}

QAction * KexiMainWindow::addAction(const char *name, const QIcon &icon, const QString& text,
                                   const char *shortcut)
{
    QAction *action = icon.isNull() ? new QAction(text, this) : new QAction(icon, text, this);
    actionCollection()->addAction(name, action);
    if (shortcut) {
        action->setShortcut(QKeySequence(shortcut));
        action->setShortcutContext(Qt::ApplicationShortcut);
        //QShortcut *s = new QShortcut(action->shortcut(), this);
        //connect(s, SIGNAL(activated()), action, SLOT(trigger()));
    }
    return action;
}

QAction * KexiMainWindow::addAction(const char *name, const QString& text, const char *shortcut)
{
    return addAction(name, QIcon(), text, shortcut);
}

void KexiMainWindow::setupActions()
{
    KActionCollection *ac = actionCollection();

    // PROJECT MENU
    QAction *action;

    ac->addAction("project_new",
        action = new KexiMenuWidgetAction(KStandardAction::New, this));
    action->setText(xi18n("&New Project..."));
    action->setShortcuts(KStandardShortcut::openNew());
    action->setToolTip(xi18n("Create a new project"));
    action->setWhatsThis(
        xi18n("Creates a new project. Currently opened project is not affected."));
    connect(action, SIGNAL(triggered()), this, SLOT(slotProjectNew()));
    setupMainMenuActionShortcut(action);

    ac->addAction("project_open",
            action = new KexiMenuWidgetAction(KStandardAction::Open, this));
    action->setText(xi18n("&Open Project..."));
    action->setIcon(koIcon("project-open"));
    action->setToolTip(xi18n("Open an existing project"));
    action->setWhatsThis(
        xi18n("Opens an existing project. Currently opened project is not affected."));
    connect(action, SIGNAL(triggered()), this, SLOT(slotProjectOpen()));
    setupMainMenuActionShortcut(action);

    ac->addAction("project_open_recent",
                  action = KStandardAction::openRecent(this, SLOT(slotProjectOpen()), this));
    action->setToolTip(xi18n("Open a project that was recently opened."));
    action->setWhatsThis(xi18n("Opens a project that was recently opened."));

    {
        ac->addAction("project_welcome",
            action = d->action_project_welcome = new KexiMenuWidgetAction(
                QIcon(), xi18n("Welcome"), this));
            addThreeDotsToActionText(action);
        connect(action, SIGNAL(triggered()), this, SLOT(slotProjectWelcome()));
        setupMainMenuActionShortcut(action);
        action->setToolTip(xi18n("Show Welcome page"));
        action->setWhatsThis(
            xi18n("Shows Welcome page with list of recently opened projects and other information. "));
    }

    ac->addAction("project_save",
                  d->action_save = KStandardAction::save(this, SLOT(slotProjectSave()), this));
    d->action_save->setToolTip(xi18n("Save object changes"));
    d->action_save->setWhatsThis(xi18n("Saves object changes from currently selected window."));
    setupMainMenuActionShortcut(d->action_save);

    d->action_save_as = addAction("project_saveas", koIcon("document-save-as"),
                                  xi18n("Save &As..."));
    d->action_save_as->setToolTip(xi18n("Save object as"));
    d->action_save_as->setWhatsThis(
        xi18n("Saves object from currently selected window under a new name (within the same project)."));
    connect(d->action_save_as, SIGNAL(triggered()), this, SLOT(slotProjectSaveAs()));

#ifdef KEXI_SHOW_UNIMPLEMENTED
    ac->addAction("project_properties",
        action = d->action_project_properties = new KexiMenuWidgetAction(
            koIcon("document-properties"), futureI18n("Project Properties"), this));
    connect(action, SIGNAL(triggered()), this, SLOT(slotProjectProperties()));
    setupMainMenuActionShortcut(action);
#else
    d->action_project_properties = d->dummy_action;
#endif

    //! @todo replace document-import icon with something other
    ac->addAction("project_import_export_send",
        action = d->action_project_import_export_send = new KexiMenuWidgetAction(
            koIcon("document-import"), xi18n("&Import, Export or Send..."), this));
    action->setToolTip(xi18n("Import, export or send project"));
    action->setWhatsThis(
        xi18n("Imports, exports or sends project."));
    connect(action, SIGNAL(triggered()), this, SLOT(slotProjectImportExportOrSend()));
    setupMainMenuActionShortcut(action);

    ac->addAction("project_close",
        action = d->action_close = new KexiMenuWidgetAction(
            koIcon("project-development-close"), xi18n("&Close Project"), this));
    action->setToolTip(xi18n("Close the current project"));
    action->setWhatsThis(xi18n("Closes the current project."));
    connect(action, SIGNAL(triggered()), this, SLOT(slotProjectClose()));
    setupMainMenuActionShortcut(action);

    ac->addAction("quit",
                  action = new KexiMenuWidgetAction(KStandardAction::Quit, this));
    connect(action, SIGNAL(triggered()), this, SLOT(slotProjectQuit()));
    action->setWhatsThis(xi18nc("@info:whatsthis", "Quits <application>%1</application> application.",
                               QApplication::applicationDisplayName()));
    setupMainMenuActionShortcut(action);

#ifdef KEXI_SHOW_UNIMPLEMENTED
    d->action_project_relations = addAction("project_relations", KexiIcon("database-relations"),
                                            futureI18n("&Relationships..."));
    d->action_project_relations->setToolTip(futureI18n("Project relationships"));
    d->action_project_relations->setWhatsThis(futureI18n("Shows project relationships."));
    connect(d->action_project_relations, SIGNAL(triggered()),
            this, SLOT(slotProjectRelations()));

#else
    d->action_project_relations = d->dummy_action;
#endif
    d->action_tools_import_project = addAction("tools_import_project", KexiIcon("database-import"),
                                               xi18n("&Import Database..."));
    d->action_tools_import_project->setToolTip(xi18n("Import entire database as a KEXI project"));
    d->action_tools_import_project->setWhatsThis(
        xi18n("Imports entire database as a KEXI project."));
    connect(d->action_tools_import_project, SIGNAL(triggered()),
            this, SLOT(slotToolsImportProject()));

    d->action_tools_data_import = addAction("tools_import_tables", koIcon("document-import"),
                                            xi18n("Import Tables..."));
    d->action_tools_data_import->setToolTip(xi18n("Import data from an external source into this project"));
    d->action_tools_data_import->setWhatsThis(xi18n("Imports data from an external source into this project."));
    connect(d->action_tools_data_import, SIGNAL(triggered()), this, SLOT(slotToolsImportTables()));

    d->action_tools_compact_database = addAction("tools_compact_database",
//! @todo icon
                                                 koIcon("application-x-compress"),
                                                 xi18n("&Compact Database..."));
    d->action_tools_compact_database->setToolTip(xi18n("Compact the current database project"));
    d->action_tools_compact_database->setWhatsThis(
        xi18n("Compacts the current database project, so it will take less space and work faster."));
    connect(d->action_tools_compact_database, SIGNAL(triggered()),
            this, SLOT(slotToolsCompactDatabase()));

    if (d->userMode)
        d->action_project_import_data_table = 0;
    else {
        d->action_project_import_data_table = addAction("project_import_data_table",
            KexiIcon("document-empty"),
            /*! @todo: change to "file_import" with a table or so */
            xi18nc("Import->Table Data From File...", "Import Data From &File..."));
        d->action_project_import_data_table->setToolTip(xi18n("Import table data from a file"));
        d->action_project_import_data_table->setWhatsThis(xi18n("Imports table data from a file."));
        connect(d->action_project_import_data_table, SIGNAL(triggered()),
                this, SLOT(slotProjectImportDataTable()));
    }

    d->action_project_export_data_table = addAction("project_export_data_table",
        KexiIcon("table"),
        /*! @todo: change to "file_export" with a table or so */
        xi18nc("Export->Table or Query Data to File...", "Export Data to &File..."));
    d->action_project_export_data_table->setToolTip(
        xi18n("Export data from the active table or query to a file"));
    d->action_project_export_data_table->setWhatsThis(
        xi18n("Exports data from the active table or query to a file."));
    connect(d->action_project_export_data_table, SIGNAL(triggered()),
            this, SLOT(slotProjectExportDataTable()));

//! @todo new QAction(xi18n("From File..."), "document-open", 0,
//!          this, SLOT(slotImportFile()), actionCollection(), "project_import_file");
//! @todo new QAction(xi18n("From Server..."), "network-server-database", 0,
//!          this, SLOT(slotImportServer()), actionCollection(), "project_import_server");

#ifdef KEXI_SHOW_UNIMPLEMENTED
    ac->addAction("project_print",
                  d->action_project_print = KStandardAction::print(this, SLOT(slotProjectPrint()), this));
    d->action_project_print->setToolTip(futureI18n("Print data from the active table or query"));
    d->action_project_print->setWhatsThis(futureI18n("Prints data from the active table or query."));

    ac->addAction("project_print_preview",
                  d->action_project_print_preview = KStandardAction::printPreview(
                                                        this, SLOT(slotProjectPrintPreview()), this));
    d->action_project_print_preview->setToolTip(
        futureI18n("Show print preview for the active table or query"));
    d->action_project_print_preview->setWhatsThis(
        futureI18n("Shows print preview for the active table or query."));

    d->action_project_print_setup = addAction("project_print_setup",
        koIcon("configure"), futureI18n("Print Set&up...")); //!< @todo document-page-setup could be a better icon
    d->action_project_print_setup->setToolTip(
        futureI18n("Show print setup for the active table or query"));
    d->action_project_print_setup->setWhatsThis(
        futureI18n("Shows print setup for the active table or query."));
    connect(d->action_project_print_setup, SIGNAL(triggered()),
            this, SLOT(slotProjectPageSetup()));
#else
    d->action_project_print = d->dummy_action;
    d->action_project_print_preview = d->dummy_action;
    d->action_project_print_setup = d->dummy_action;
#endif

    //EDIT MENU
    d->action_edit_cut = createSharedAction(KStandardAction::Cut);
    d->action_edit_copy = createSharedAction(KStandardAction::Copy);
    d->action_edit_paste = createSharedAction(KStandardAction::Paste);

    if (d->userMode)
        d->action_edit_paste_special_data_table = 0;
    else {
        d->action_edit_paste_special_data_table = addAction(
            "edit_paste_special_data_table",
            d->action_edit_paste->icon(), xi18nc("Paste Special->As Data &Table...", "Paste Special..."));
        d->action_edit_paste_special_data_table->setToolTip(
            xi18n("Paste clipboard data as a table"));
        d->action_edit_paste_special_data_table->setWhatsThis(
            xi18n("Pastes clipboard data as a table."));
        connect(d->action_edit_paste_special_data_table, SIGNAL(triggered()),
                this, SLOT(slotEditPasteSpecialDataTable()));
    }

    d->action_edit_copy_special_data_table = addAction(
        "edit_copy_special_data_table",
        KexiIcon("table"), xi18nc("Copy Special->Table or Query Data...", "Copy Special..."));
    d->action_edit_copy_special_data_table->setToolTip(
        xi18n("Copy selected table or query data to clipboard"));
    d->action_edit_copy_special_data_table->setWhatsThis(
        xi18n("Copies selected table or query data to clipboard."));
    connect(d->action_edit_copy_special_data_table, SIGNAL(triggered()),
            this, SLOT(slotEditCopySpecialDataTable()));

    d->action_edit_undo = createSharedAction(KStandardAction::Undo);
    d->action_edit_undo->setWhatsThis(xi18n("Reverts the most recent editing action."));
    d->action_edit_redo = createSharedAction(KStandardAction::Redo);
    d->action_edit_redo->setWhatsThis(xi18n("Reverts the most recent undo action."));

    ac->addAction("edit_find",
                  d->action_edit_find = KStandardAction::find(
                                            this, SLOT(slotEditFind()), this));
    d->action_edit_find->setToolTip(xi18n("Find text"));
    d->action_edit_find->setWhatsThis(xi18n("Looks up the first occurrence of a piece of text."));
    ac->addAction("edit_findnext",
                  d->action_edit_findnext = KStandardAction::findNext(
                                                this, SLOT(slotEditFindNext()), this));
    ac->addAction("edit_findprevious",
                  d->action_edit_findprev = KStandardAction::findPrev(
                                                this, SLOT(slotEditFindPrevious()), this));
    ac->addAction("edit_replace",
                  d->action_edit_replace = KStandardAction::replace(
                                                this, SLOT(slotEditReplace()), this));
    d->action_edit_replace_all = addAction("edit_replace_all", xi18n("Replace All"));

    d->action_edit_select_all =  createSharedAction(KStandardAction::SelectAll);

    d->action_edit_delete = createSharedAction(xi18n("&Delete"), koIconName("edit-delete"),
                            QKeySequence(), "edit_delete");
    d->action_edit_delete->setToolTip(xi18n("Delete selected object"));
    d->action_edit_delete->setWhatsThis(xi18n("Deletes currently selected object."));

    d->action_edit_delete_row = createSharedAction(xi18n("Delete Record"), KexiIconName("edit-table-delete-row"),
                                QKeySequence(Qt::CTRL + Qt::Key_Delete), "edit_delete_row");
    d->action_edit_delete_row->setToolTip(xi18n("Delete the current record"));
    d->action_edit_delete_row->setWhatsThis(xi18n("Deletes the current record."));

    d->action_edit_clear_table = createSharedAction(xi18n("Clear Table Contents..."),
                                 KexiIconName("edit-table-clear"), QKeySequence(), "edit_clear_table");
    d->action_edit_clear_table->setToolTip(xi18n("Clear table contents"));
    d->action_edit_clear_table->setWhatsThis(xi18n("Clears table contents."));
    setActionVolatile(d->action_edit_clear_table, true);

    d->action_edit_edititem = createSharedAction(xi18n("Edit Item"), QString(),
                              QKeySequence(), /* CONFLICT in TV: Qt::Key_F2,  */
                              "edit_edititem");
    d->action_edit_edititem->setToolTip(xi18n("Edit currently selected item"));
    d->action_edit_edititem->setWhatsThis(xi18n("Edits currently selected item."));

    d->action_edit_insert_empty_row = createSharedAction(xi18n("&Insert Empty Row"),
                                      KexiIconName("edit-table-insert-row"), QKeySequence(Qt::SHIFT | Qt::CTRL | Qt::Key_Insert),
                                      "edit_insert_empty_row");
    setActionVolatile(d->action_edit_insert_empty_row, true);
    d->action_edit_insert_empty_row->setToolTip(xi18n("Insert one empty row above"));
    d->action_edit_insert_empty_row->setWhatsThis(
        xi18n("Inserts one empty row above currently selected table row."));

    //VIEW MENU
    /* UNUSED, see KexiToggleViewModeAction
      if (!d->userMode) {
        d->action_view_mode = new QActionGroup(this);
        ac->addAction( "view_data_mode",
          d->action_view_data_mode = new KToggleAction(
            KexiIcon("data-view"), xi18n("&Data View"), d->action_view_mode) );
    //  d->action_view_data_mode->setObjectName("view_data_mode");
        d->action_view_data_mode->setShortcut(QKeySequence("F6"));
        //d->action_view_data_mode->setExclusiveGroup("view_mode");
        d->action_view_data_mode->setToolTip(xi18n("Switch to data view"));
        d->action_view_data_mode->setWhatsThis(xi18n("Switches to data view."));
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
            KexiIcon("design-view"), xi18n("D&esign View"), d->action_view_mode) );
    //  d->action_view_design_mode->setObjectName("view_design_mode");
        d->action_view_design_mode->setShortcut(QKeySequence("F7"));
        //d->action_view_design_mode->setExclusiveGroup("view_mode");
        d->action_view_design_mode->setToolTip(xi18n("Switch to design view"));
        d->action_view_design_mode->setWhatsThis(xi18n("Switches to design view."));
        d->actions_for_view_modes.insert( Kexi::DesignViewMode, d->action_view_design_mode );
        connect(d->action_view_design_mode, SIGNAL(triggered()),
          this, SLOT(slotViewDesignMode()));
      }
      else
        d->action_view_design_mode = 0;

      if (!d->userMode) {
        ac->addAction( "view_text_mode",
          d->action_view_text_mode = new KToggleAction(
            KexiIcon("sql-view"), xi18n("&Text View"), d->action_view_mode) );
        d->action_view_text_mode->setObjectName("view_text_mode");
        d->action_view_text_mode->setShortcut(QKeySequence("F8"));
        //d->action_view_text_mode->setExclusiveGroup("view_mode");
        d->action_view_text_mode->setToolTip(xi18n("Switch to text view"));
        d->action_view_text_mode->setWhatsThis(xi18n("Switches to text view."));
        d->actions_for_view_modes.insert( Kexi::TextViewMode, d->action_view_text_mode );
        connect(d->action_view_text_mode, SIGNAL(triggered()),
          this, SLOT(slotViewTextMode()));
      }
      else
        d->action_view_text_mode = 0;
    */
    if (d->isProjectNavigatorVisible) {
        ac->addAction("view_navigator",
            d->action_show_nav = new KToggleAction(xi18n("Show Project Navigator"), this));
        d->action_show_nav->setChecked(true);
        d->action_show_nav->setShortcut(QKeySequence("Alt+0"));
        d->action_show_nav->setToolTip(xi18n("Show the Project Navigator pane"));
        d->action_show_nav->setWhatsThis(xi18n("Shows the Project Navigator pane."));
        connect(d->action_show_nav, SIGNAL(triggered()),
                this, SLOT(slotToggleProjectNavigator()));
    } else {
        d->action_show_nav = 0;
    }

    if (d->isProjectNavigatorVisible) {
        // Shortcut taken from "Activate Projects pane" https://doc.qt.io/qtcreator/creator-keyboard-shortcuts.html
        d->action_activate_nav = addAction("activate_navigator",
                                       xi18n("Activate Project Navigator"),
                                       "Alt+X");
        d->action_activate_nav->setToolTip(xi18n("Activate the Project Navigator pane"));
        d->action_activate_nav->setWhatsThis(xi18n("Activates the Project Navigator pane. If it is hidden, shows it first."));
        connect(d->action_activate_nav, SIGNAL(triggered()),
                this, SLOT(slotActivateNavigator()));
    } else {
        d->action_activate_nav = 0;
    }

    d->action_activate_mainarea = addAction("activate_mainarea",
                                        xi18n("Activate main area")
// , "Alt+2"?
//! @todo activate_mainarea: pressing Esc in project nav or propeditor should move back to the main area
                                        );
    d->action_activate_mainarea->setToolTip(xi18n("Activate the main area"));
    d->action_activate_mainarea->setWhatsThis(xi18n("Activates the main area."));
    connect(d->action_activate_mainarea, SIGNAL(triggered()),
            this, SLOT(slotActivateMainArea()));

    //! @todo windows with "_3" prefix have conflicting auto shortcut set to Alt+3 -> remove that!
    if (!d->userMode) {
        ac->addAction("view_propeditor",
            d->action_show_propeditor = new KToggleAction(xi18n("Show Property Pane"), this));
        d->action_show_propeditor->setShortcut(QKeySequence("Alt+3"));
        d->action_show_propeditor->setToolTip(xi18n("Show the Property pane"));
        d->action_show_propeditor->setWhatsThis(xi18n("Shows the Property pane."));
        connect(d->action_show_propeditor, SIGNAL(triggered()),
                this, SLOT(slotTogglePropertyEditor()));
    } else {
        d->action_show_propeditor = 0;
    }

    if (!d->userMode) {
        d->action_activate_propeditor = addAction("activate_propeditor",
                                              xi18n("Activate Property Pane"), "Alt+-");
        d->action_activate_propeditor->setToolTip(xi18n("Activate the Property pane"));
        d->action_activate_propeditor->setWhatsThis(xi18n("Activates the Property pane. If it is hidden, shows it first."));
        connect(d->action_activate_propeditor, SIGNAL(triggered()),
                this, SLOT(slotActivatePropertyPane()));
    } else {
        d->action_activate_propeditor = 0;
    }

    d->action_tools_locate = addAction("tools_locate",
                                             xi18n("Locate..."), "Ctrl+K");
    d->action_tools_locate->setToolTip(xi18n("Switch to Global Locate box"));
    d->action_tools_locate->setWhatsThis(xi18n("Switches to Global Locate box."));
    // (connection is added elsewhere)

    //DATA MENU
    d->action_data_save_row = createSharedAction(xi18n("&Accept"), koIconName("dialog-ok"),
                              QKeySequence(Qt::SHIFT + Qt::Key_Return), "data_save_row");
    d->action_data_save_row->setToolTip(xi18n("Accept changes made to the current record"));
    d->action_data_save_row->setWhatsThis(xi18n("Accepts changes made to the current record."));
//temp. disable because of problems with volatile actions setActionVolatile( d->action_data_save_row, true );

    d->action_data_cancel_row_changes = createSharedAction(xi18n("&Cancel"),
                                        koIconName("dialog-cancel"), QKeySequence(Qt::Key_Escape), "data_cancel_row_changes");
    d->action_data_cancel_row_changes->setToolTip(
        xi18n("Cancel changes made to the current record"));
    d->action_data_cancel_row_changes->setWhatsThis(
        xi18n("Cancels changes made to the current record."));
//temp. disable because of problems with volatile actions setActionVolatile( d->action_data_cancel_row_changes, true );

    d->action_data_execute = createSharedAction(
                                 xi18n("&Execute"), koIconName("media-playback-start"), QKeySequence(), "data_execute");
    //! @todo d->action_data_execute->setToolTip(xi18n(""));
    //! @todo d->action_data_execute->setWhatsThis(xi18n(""));

#ifdef KEXI_SHOW_UNIMPLEMENTED
    action = createSharedAction(futureI18n("&Filter"), koIconName("view-filter"), QKeySequence(), "data_filter");
    setActionVolatile(action, true);
#endif
//! @todo action->setToolTip(xi18n(""));
//! @todo action->setWhatsThis(xi18n(""));

    // - record-navigation related actions
    createSharedAction(KexiRecordNavigator::Actions::moveToFirstRecord(), QKeySequence(), "data_go_to_first_record");
    createSharedAction(KexiRecordNavigator::Actions::moveToPreviousRecord(), QKeySequence(), "data_go_to_previous_record");
    createSharedAction(KexiRecordNavigator::Actions::moveToNextRecord(), QKeySequence(), "data_go_to_next_record");
    createSharedAction(KexiRecordNavigator::Actions::moveToLastRecord(), QKeySequence(), "data_go_to_last_record");
    createSharedAction(KexiRecordNavigator::Actions::moveToNewRecord(), QKeySequence(), "data_go_to_new_record");

    //FORMAT MENU
#ifdef KEXI_SHOW_UNIMPLEMENTED
    d->action_format_font = createSharedAction(xi18n("&Font..."), koIconName("fonts-package"),
                            QKeySequence(), "format_font");
    d->action_format_font->setToolTip(xi18n("Change font for selected object"));
    d->action_format_font->setWhatsThis(xi18n("Changes font for selected object."));
#else
    d->action_format_font = d->dummy_action;
#endif

    //TOOLS MENU

    //WINDOW MENU
    // additional 'Window' menu items
    d->action_window_next = addAction("window_next", xi18n("&Next Window"), "Alt+Right");
    d->action_window_next->setToolTip(xi18n("Next window"));
    d->action_window_next->setWhatsThis(xi18n("Switches to the next window."));
    connect(d->action_window_next, SIGNAL(triggered()), this, SLOT(activateNextWindow()));

    d->action_window_previous = addAction("window_previous", xi18n("&Previous Window"), "Alt+Left");
    d->action_window_previous->setToolTip(xi18n("Previous window"));
    d->action_window_previous->setWhatsThis(xi18n("Switches to the previous window."));
    connect(d->action_window_previous, SIGNAL(triggered()), this, SLOT(activatePreviousWindow()));

    d->action_close_tab = addAction("close_tab", koIcon("tab-close"), xi18n("&Close Tab"), "Ctrl+W");
    d->action_close_tab->setToolTip(xi18n("Close the current tab"));
    d->action_close_tab->setWhatsThis(xi18n("Closes the current tab."));
    connect(d->action_close_tab, SIGNAL(triggered()), this, SLOT(closeCurrentWindow()));

    d->action_close_all_tabs = addAction("close_all_tabs", QIcon(), xi18n("Cl&ose All Tabs"));
    d->action_close_all_tabs->setToolTip(xi18n("Close all tabs"));
    d->action_close_all_tabs->setWhatsThis(xi18n("Closes all tabs."));
    connect(d->action_close_all_tabs, SIGNAL(triggered()), this, SLOT(closeAllWindows()));

    d->action_tab_next = addAction("tab_next", koIcon("go-next"),
                                   KStandardShortcut::label(KStandardShortcut::TabNext));
    d->action_tab_next->setWhatsThis(KStandardShortcut::whatsThis(KStandardShortcut::TabNext));
    d->action_tab_next->setShortcuts(KStandardShortcut::shortcut(KStandardShortcut::TabNext));
    connect(d->action_tab_next, &QAction::triggered, this, &KexiMainWindow::activateNextTab);

    d->action_tab_previous = addAction("tab_previous", koIcon("go-previous"),
                                       KStandardShortcut::label(KStandardShortcut::TabPrev));
    d->action_tab_previous->setWhatsThis(KStandardShortcut::whatsThis(KStandardShortcut::TabPrev));
    d->action_tab_previous->setShortcuts(KStandardShortcut::shortcut(KStandardShortcut::TabPrev));
    connect(d->action_tab_previous, &QAction::triggered, this, &KexiMainWindow::activatePreviousTab);

    d->action_window_fullscreen = KStandardAction::fullScreen(this, SLOT(toggleFullScreen(bool)), this, ac);
    ac->addAction("full_screen", d->action_window_fullscreen);

    //SETTINGS MENU
//! @todo put 'configure keys' into settings view

#ifdef KEXI_SHOW_UNIMPLEMENTED
    //! @todo toolbars configuration will be handled in a special way
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
//! @todo implement settings window in a specific way
    ac->addAction("settings",
                  action = d->action_settings = new KexiMenuWidgetAction(
                    KStandardAction::Preferences, this));
    action->setObjectName("settings");
    //action->setText(futureI18n("Settings..."));
    action->setToolTip(futureI18n("Show KEXI settings"));
    action->setWhatsThis(futureI18n("Shows KEXI settings."));
    connect(action, SIGNAL(triggered()), this, SLOT(slotSettings()));
    setupMainMenuActionShortcut(action);
#else
    d->action_settings = d->dummy_action;
#endif

//! @todo reenable 'tip of the day' later
#if 0
    KStandardAction::tipOfDay(this, SLOT(slotTipOfTheDayAction()), actionCollection())
    ->setWhatsThis(xi18n("This shows useful tips on the use of this application."));
#endif

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

    //! @todo support this in FormObjectType as well
    acat->addWindowAction("project_export_data_table",
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

    acat->addAction("help_donate", Kexi::GlobalActionCategory); // disabled for now

    acat->addAction("switch_application_language", Kexi::GlobalActionCategory);

    acat->addAction("options_configure_keybinding", Kexi::GlobalActionCategory);

    acat->addAction("project_close", Kexi::GlobalActionCategory);

    acat->addAction("project_import_data_table", Kexi::GlobalActionCategory);

    acat->addAction("project_new", Kexi::GlobalActionCategory);

    acat->addAction("project_open", Kexi::GlobalActionCategory);

#ifdef KEXI_QUICK_PRINTING_SUPPORT
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

    acat->addAction("activate_navigator", Kexi::GlobalActionCategory);

    acat->addAction("view_propeditor", Kexi::GlobalActionCategory);

    acat->addAction("activate_mainarea", Kexi::GlobalActionCategory);

    acat->addAction("activate_propeditor", Kexi::GlobalActionCategory);

    acat->addAction("close_tab", Kexi::GlobalActionCategory | Kexi::WindowActionCategory);
    acat->setAllObjectTypesSupported("close_tab", true);

    acat->addAction("close_all_tabs", Kexi::GlobalActionCategory | Kexi::WindowActionCategory);
    acat->setAllObjectTypesSupported("close_all_tabs", true);

    acat->addAction("tab_next", Kexi::GlobalActionCategory);

    acat->addAction("tab_previous", Kexi::GlobalActionCategory);

    acat->addAction("full_screen", Kexi::GlobalActionCategory);

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

void KexiMainWindow::setupMainMenu()
{
    KActionCollection *ac = actionCollection();
    QMenuBar *menu = menuBar();
    {
        QMenu *fileMenu = menu->addMenu(xi18n("&File"));
        if (!d->userMode) {
            d->addAction(fileMenu, "project_new");
            fileMenu->addSeparator();
            d->addAction(fileMenu, "project_open");
            d->addAction(fileMenu, "project_open_recent");
            fileMenu->addSeparator();
        }
        fileMenu->addAction(d->action_save);
        if (!d->userMode) {
            fileMenu->addAction(d->action_save_as);
        }
        fileMenu->addSeparator();
        if (!d->userMode) {
            fileMenu->addAction(d->action_tools_import_project);
            fileMenu->addSeparator();
        }
#ifdef KEXI_SHOW_UNIMPLEMENTED
        fileMenu->addAction(d->action_project_print);
        fileMenu->addAction(d->action_project_print_preview);
        fileMenu->addAction(d->action_project_print_setup);
        fileMenu->addSeparator();
#endif
#ifdef KEXI_SHOW_UNIMPLEMENTED
        fileMenu->addAction(d->action_project_properties);
        fileMenu->addSeparator();
#endif
        fileMenu->addAction(d->action_close_tab);
        fileMenu->addAction(d->action_close_all_tabs);
        if (!d->userMode) {
            fileMenu->addAction(d->action_close);
        }
        fileMenu->addSeparator();
        d->addAction(fileMenu, "quit");
    }
    {
        QMenu *editMenu = menu->addMenu(xi18n("&Edit"));
        editMenu->addAction(d->action_edit_undo);
        editMenu->addAction(d->action_edit_redo);
        editMenu->addSeparator();
        editMenu->addAction(d->action_edit_cut);
        editMenu->addAction(d->action_edit_copy);
        editMenu->addAction(d->action_edit_copy_special_data_table);
        editMenu->addAction(d->action_edit_paste);
        if (!d->userMode) {
            editMenu->addAction(d->action_edit_paste_special_data_table);
        }
        editMenu->addSeparator();
        editMenu->addAction(d->action_edit_select_all);
        editMenu->addSeparator();
        {
            QMenu *findReplaceMenu = editMenu->addMenu(xi18n("&Find/Replace"));
            findReplaceMenu->addAction(d->action_edit_find);
            findReplaceMenu->addAction(d->action_edit_findnext);
            findReplaceMenu->addAction(d->action_edit_findprev);
#ifdef KEXI_SHOW_UNIMPLEMENTED
            findReplaceMenu->addSeparator();
            findReplaceMenu->addAction(d->action_edit_replace);
            findReplaceMenu->addAction(d->action_edit_replace_all);
#endif
        }
        editMenu->addSeparator();
        editMenu->addAction(d->action_edit_delete);
        // in local toolbar already: editMenu->addAction(d->action_data_save_row);
        // in local toolbar already: editMenu->addAction(d->action_data_cancel_row_changes);
        //TODO move to local menu: editMenu->addAction(d->action_edit_edititem);
        //TODO move to local menu: editMenu->addAction(d->action_edit_insert_empty_row);
        //TODO move to local menu: editMenu->addAction(d->action_data_execute);
        //editMenu->addSeparator();
        //TODO move to local menu: editMenu->addAction(d->action_edit_delete);
        //TODO move to local menu: editMenu->addAction(d->action_edit_delete_row);
        //in local menu already editMenu->addAction(d->action_edit_clear_table);
    }
#ifdef KEXI_SHOW_UNIMPLEMENTED
    {
        QMenu *formatMenu = menu->addMenu(xi18n("F&ormat"));
        formatMenu->addAction(d->action_format_font);
    }
#endif
    {
        QMenu *dataMenu = menu->addMenu(xi18n("&Data"));
        if (!d->userMode) {
            dataMenu->addAction(d->action_project_import_data_table);
            dataMenu->addAction(d->action_tools_data_import);
            dataMenu->addSeparator();
        }
        dataMenu->addAction(d->action_project_export_data_table);
    }
    {
        QMenu *toolsMenu = menu->addMenu(xi18n("&Tools"));
        toolsMenu->addAction(d->action_tools_locate);
        toolsMenu->addSeparator();
#ifdef KEXI_SHOW_UNIMPLEMENTED
        toolsMenu->addAction(d->action_project_relations);
#endif
        toolsMenu->addAction(d->action_tools_compact_database);
    }
    {
        QMenu *settingsMenu = menu->addMenu(xi18n("&Settings"));
        settingsMenu->addAction(d->action_window_fullscreen);
        settingsMenu->addSeparator();
#ifdef KEXI_SHOW_UNIMPLEMENTED
        settingsMenu->addAction(d->action_settings);
#endif
    }
    {
        QMenu *windowMenu = menu->addMenu(xi18n("&Window"));
        windowMenu->addAction(d->action_tab_next);
        windowMenu->addAction(d->action_tab_previous);
        windowMenu->addSeparator();
        windowMenu->addAction(d->action_show_nav);
        windowMenu->addAction(d->action_show_propeditor);
    }
    {
        // add help menu actions... (KexiTabbedToolBar depends on them)
        KHelpMenu *helpMenu = new KHelpMenu(this, KAboutData::applicationData(),
                                    true/*showWhatsThis*/);
        QAction* help_report_bug_action = helpMenu->action(KHelpMenu::menuReportBug);
        ac->addAction(help_report_bug_action->objectName(), help_report_bug_action);
        QObject::disconnect(help_report_bug_action, 0, 0, 0);
        QObject::connect(help_report_bug_action, &QAction::triggered, this, &KexiMainWindow::slotReportBug);
        help_report_bug_action->setText(xi18nc("Report a bug or wish for KEXI application", "Report a &Bug or Wish..."));
        help_report_bug_action->setIcon(koIcon("tools-report-bug")); // good icon for toolbar
        help_report_bug_action->setWhatsThis(xi18n("Files a bug or wish for KEXI application."));
        QAction* help_whats_this_action =  helpMenu->action(KHelpMenu::menuWhatsThis);
        ac->addAction(help_whats_this_action->objectName(), help_whats_this_action);
        help_whats_this_action->setWhatsThis(xi18n("Activates a \"What's This?\" tool."));
        QAction* help_contents_action = helpMenu->action(KHelpMenu::menuHelpContents);
        ac->addAction(help_contents_action->objectName(), help_contents_action);
        help_contents_action->setText(xi18n("Help"));
        help_contents_action->setWhatsThis(xi18n("Shows KEXI Handbook."));
        QAction* help_about_app_action = helpMenu->action(KHelpMenu::menuAboutApp);
        ac->addAction(help_about_app_action->objectName(), help_about_app_action);
        help_about_app_action->setWhatsThis(xi18n("Shows information about KEXI application."));
        QAction* help_about_kde_action = helpMenu->action(KHelpMenu::menuAboutKDE);
        ac->addAction(help_about_kde_action->objectName(), help_about_kde_action);
        help_about_kde_action->setWhatsThis(xi18n("Shows information about KDE."));
        menu->addMenu(helpMenu->menu());
    }
}

void KexiMainWindow::invalidateActions()
{
    invalidateProjectWideActions();
    invalidateSharedActions();
}

void KexiMainWindow::invalidateSharedActions(QObject *o)
{
    //! @todo enabling is more complex...
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
    const bool has_window = currentWindow();
    const bool window_dirty = currentWindow() && currentWindow()->isDirty();
    const bool readOnly = d->prj && d->prj->dbConnection() && d->prj->dbConnection()->options()->isReadOnly();

    //PROJECT MENU
    d->action_save->setEnabled(has_window && window_dirty && !readOnly);
    d->action_save_as->setEnabled(has_window && !readOnly);
    d->action_project_properties->setEnabled(d->prj);
    d->action_close->setEnabled(d->prj);
    d->action_project_relations->setEnabled(d->prj);
    if (d->objectViewWidget) {
        d->action_close_tab->setEnabled(has_window);
        d->action_close_all_tabs->setEnabled(has_window);
    }

    //DATA MENU
    if (d->action_project_import_data_table)
        d->action_project_import_data_table->setEnabled(d->prj && !readOnly);
    if (d->action_tools_data_import)
        d->action_tools_data_import->setEnabled(d->prj && !readOnly);
    d->action_project_export_data_table->setEnabled(
        currentWindow() && currentWindow()->part()->info()->isDataExportSupported());
    if (d->action_edit_paste_special_data_table)
        d->action_edit_paste_special_data_table->setEnabled(d->prj && !readOnly);

#ifdef KEXI_SHOW_UNIMPLEMENTED
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
    } else {
        d->action_edit_copy_special_data_table->setEnabled(false);
    }
    d->action_edit_find->setEnabled(d->prj);

    //VIEW MENU
    if (d->action_show_nav)
        d->action_show_nav->setEnabled(d->prj);
    d->action_activate_mainarea->setEnabled(d->prj);

    //CREATE MENU
    if (d->tabbedToolBar && d->tabbedToolBar->createWidgetToolBar())
        d->tabbedToolBar->createWidgetToolBar()->setEnabled(d->prj);

    // DATA MENU

    // WINDOW MENU
    if (d->objectViewWidget) {
        d->action_tab_next->setEnabled(d->objectViewWidget->tabWidget()->count() > 1);
        d->action_tab_previous->setEnabled(d->objectViewWidget->tabWidget()->count() > 1);
    }

    //TOOLS MENU
    // "compact db" supported if there's no db or the current db supports compacting and is opened r/w:
    d->action_tools_compact_database->setEnabled(
        //! @todo Support compacting of non-opened projects
        /*!d->prj
        ||*/ (!readOnly && d->prj && d->prj->dbConnection()
            && (d->prj->dbConnection()->driver()->features() & KDbDriver::CompactingDatabaseSupported))
    );

    //DOCKS
    if (d->objectViewWidget && d->objectViewWidget->projectNavigator()) {
        d->objectViewWidget->projectNavigator()->setEnabled(d->prj);
    }
    if (d->objectViewWidget && d->objectViewWidget->propertyPane()) {
        d->objectViewWidget->propertyPane()->setEnabled(d->prj);
    }
}

tristate KexiMainWindow::startup()
{
    tristate result = true;
    switch (KexiStartupHandler::global()->action()) {
    case KexiStartupHandler::CreateBlankProject:
        d->updatePropEditorVisibility(Kexi::NoViewMode);
        break;
#ifdef KEXI_PROJECT_TEMPLATES
    case KexiStartupHandler::CreateFromTemplate:
        result = createProjectFromTemplate(*KexiStartupHandler::global()->projectData());
        break;
#endif
    case KexiStartupHandler::OpenProject:
        result = openProject(*KexiStartupHandler::global()->projectData());
        break;
    case KexiStartupHandler::ImportProject:
        result = showProjectMigrationWizard(
                   KexiStartupHandler::global()->importActionData().mimeType,
                   KexiStartupHandler::global()->importActionData().fileName
               );
        break;
    case KexiStartupHandler::ShowWelcomeScreen:
        //! @todo show welcome screen as soon as is available
        QTimer::singleShot(100, this, SLOT(slotProjectWelcome()));
        break;
    default:
        d->updatePropEditorVisibility(Kexi::NoViewMode);
    }
    return result;
}

static QString internalReason(const KDbResult &result)
{
    const QString msg = result.message();
    if (msg.isEmpty()) {
        return QString();
    }
    return xi18n("<br/>(reason: <i>%1</i>)", msg);
}

tristate KexiMainWindow::openProject(const KexiProjectData& projectData)
{
    //qDebug() << projectData;
    QScopedPointer<KexiProject> prj(createKexiProjectObject(projectData));
    if (~KexiDBPasswordDialog::getPasswordIfNeeded(prj->data()->connectionData(), this)) {
        return cancelled;
    }
    bool incompatibleWithKexi;
    tristate res = prj->open(&incompatibleWithKexi);

    if (prj->data()->connectionData()->isPasswordNeeded()) {
        // password was supplied in this session, and shouldn't be stored or reused afterwards,
        // so let's remove it
        prj->data()->connectionData()->setPassword(QString());
    }

    if (~res) {
        return cancelled;
    }
    else if (!res) {
        if (incompatibleWithKexi) {
            if (KMessageBox::PrimaryAction == KMessageBox::questionTwoActions(this,
                    xi18nc("@info (don't add tags around %1, it's done already)",
                           "Database project %1 does not appear to have been created using "
                           "<application>%2</application>.<nl/>"
                           "Do you want to import it as a new KEXI project?",
                           projectData.infoString(), QApplication::applicationDisplayName()),
                    QString(), KGuiItem(xi18nc("@action:button Import Database", "&Import..."), KexiIconName("database-import")),
                    KStandardGuiItem::cancel()))
            {
                const bool anotherProjectAlreadyOpened = prj;
                tristate res = showProjectMigrationWizard("application/x-kexi-connectiondata",
                                   projectData.databaseName(), *projectData.connectionData());

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

    // success
    d->prj = prj.take();
    d->modeSelector->setCurrentMode(Kexi::EditGlobalMode);
    d->prj->data()->setLastOpened(QDateTime::currentDateTime());
    Kexi::recentProjects()->addProjectData(*d->prj->data());
    updateReadOnlyState();
    invalidateActions();
    setMessagesEnabled(false);

    QTimer::singleShot(1, this, SLOT(slotAutoOpenObjectsLater()));
    if (d->tabbedToolBar) {
        d->tabbedToolBar->showTab("create");// not needed since create toolbar already shows toolbar! move when kexi starts
        d->tabbedToolBar->hideTab("form");//temporalily until createToolbar is split
        d->tabbedToolBar->hideTab("report");//temporalily until createToolbar is split

        // make sure any tab is activated
        d->tabbedToolBar->setCurrentTab(0);
    }
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
    Q_UNUSED(projectData);
#ifdef KEXI_PROJECT_TEMPLATES
    QStringList mimetypes;
    mimetypes.append(KDb::defaultFileBasedDriverMimeType());
    QString fname;
    //! @todo KEXI3 add equivalent of kfiledialog:///
    const QString startDir("kfiledialog:///OpenExistingOrCreateNewProject"/*as in KexiNewProjectWizard*/);
    const QString caption(xi18nc("@window:title", "Select New Project's Location"));

    while (true) {
        if (fname.isEmpty() &&
                !projectData.connectionData()->databaseName().isEmpty()) {
            //propose filename from db template name
            fname = projectData.connectionData()->databaseName();
        }
        const bool specialDir = fname.isEmpty();
        qDebug() << fname << ".............";
        QFileDialog dlg(specialDir ? QUrl(startDir) : QUrl(),
                        QString(), this);
        dlg.setModal(true);
        dlg.setMimeFilter(mimetypes);
        if (!specialDir)
            dlg.selectUrl(QUrl::fromLocalFile(fname);   // may also be a filename
        dlg.setFileMode(QFileDialog::ExistingFile);
        dlg.setFileMode(QFileDialog::AcceptOpen);
        dlg.setWindowTitle(caption);
        if (QDialog::Accepted != dlg.exec()) {
            return cancelled;
        }
        if (dlg.selectedFiles().isEmpty()) {
            return cancelled;
        }
        fname = dlg.selectedFiles().first();
        if (fname.isEmpty()) {
            return cancelled;
        }
        if (KexiUtils::askForFileOverwriting(fname, this)) {
            break;
        }
    }

    QFile sourceFile(projectData.connectionData()->fileName());
    if (!sourceFile.copy(fname)) {
//! @todo show error from with QFile::FileError
        return false;
    }

    return openProject(fname, 0, QString(), projectData.autoopenObjects/*copy*/);
#else
    return false;
#endif
}

void KexiMainWindow::updateReadOnlyState()
{
    const bool readOnly = d->prj && d->prj->dbConnection() && d->prj->dbConnection()->options()->isReadOnly();
    //! @todo KEXI3 show read-only flag in the GUI because we have no statusbar
    if (d->objectViewWidget && d->objectViewWidget->projectNavigator()) {
        d->objectViewWidget->projectNavigator()->setReadOnly(readOnly);
    }

    // update "insert ....." actions for every part
    KexiPart::PartInfoList *plist = Kexi::partManager().infoList();
    if (plist) {
        foreach(KexiPart::Info *info, *plist) {
            QAction *a = info->newObjectAction();
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
        for (const KexiProjectData::ObjectInfo &info : d->prj->data()->autoopenObjects) {
            KexiPart::Info *i = Kexi::partManager().infoForPluginId(info.value("type"));
            if (!i) {
                not_found_msg += "<li>";
                if (!info.value("name").isEmpty()) {
                    not_found_msg += (QString("\"") + info.value("name") + "\" - ");
                }
                if (info.value("action") == "new") {
                    not_found_msg += xi18n("cannot create object - unknown object type \"%1\"",
                                           info.value("type"));
                } else {
                    not_found_msg += xi18n("unknown object type \"%1\"", info.value("type"));
                }
                not_found_msg += internalReason(Kexi::partManager().result()) + "<br></li>";
                continue;
            }
            // * NEW
            if (info.value("action") == "new") {
                if (!newObject(i, &openingCancelled) && !openingCancelled) {
                    not_found_msg += "<li>";
                    not_found_msg
                        += (xi18n("cannot create object of type \"%1\"", info.value("type"))
                            + internalReason(d->prj->result()) + "<br></li>");
                } else {
                    d->wasAutoOpen = true;
                }
                continue;
            }

            KexiPart::Item *item = d->prj->item(i, info.value("name"));
            if (!item) {
                QString taskName;
                if (info.value("action") == "execute") {
                    taskName = xi18nc("\"executing object\" action", "executing");
#ifdef KEXI_QUICK_PRINTING_SUPPORT
                } else if (info->value("action") == "print-preview") {
                    taskName = futureI18n("making print preview for");
                } else if (info->value("action") == "print") {
                    taskName = futureI18n("printing");
#endif
                } else {
                    taskName = xi18n("opening");
                }

                not_found_msg += (QString("<li>") + taskName + " \"" + info.value("name") + "\" - ");
                if ("table" == info.value("type").toLower()) {
                    not_found_msg += xi18n("table not found");
                } else if ("query" == info.value("type").toLower()) {
                    not_found_msg += xi18n("query not found");
                } else if ("macro" == info.value("type").toLower()) {
                    not_found_msg += xi18n("macro not found");
                } else if ("script" == info.value("type").toLower()) {
                    not_found_msg += xi18n("script not found");
                } else {
                    not_found_msg += xi18n("object not found");
                }
                not_found_msg += (internalReason(d->prj->result()) + "<br></li>");
                continue;
            }
            // * EXECUTE, PRINT, PRINT PREVIEW
            if (info.value("action") == "execute") {
                tristate res = executeItem(item);
                if (false == res) {
                    not_found_msg += (QString("<li>\"") + info.value("name") + "\" - "
                                      + xi18n("cannot execute object")
                                      + internalReason(d->prj->result()) + "<br></li>");
                }
                continue;
            }
#ifdef KEXI_QUICK_PRINTING_SUPPORT
            else if (info.value("action") == "print") {
                tristate res = printItem(item);
                if (false == res) {
                    not_found_msg += (QString("<li>\"") + info.value("name") + "\" - " + futureI18n("cannot print object") +
                                      internalReason(d->prj->result()) + "<br></li>");
                }
                continue;
            }
            else if (info.value("action") == "print-preview") {
                tristate res = printPreviewForItem(item);
                if (false == res) {
                    not_found_msg += (QString("<li>\"") + info.value("name") + "\" - " + futureI18n("cannot make print preview of object") +
                                      internalReason(d->prj->result()) + "<br></li>");
                }
                continue;
            }
#endif

            Kexi::ViewMode viewMode;
            if (info.value("action") == "open") {
                viewMode = Kexi::DataViewMode;
            } else if (info.value("action") == "design") {
                viewMode = Kexi::DesignViewMode;
            } else if (info.value("action") == "edittext") {
                viewMode = Kexi::TextViewMode;
            } else {
                continue; //sanity
            }

            QString openObjectMessage;
            if (!openObject(item, viewMode, &openingCancelled, 0, &openObjectMessage)
                    && (!openingCancelled || !openObjectMessage.isEmpty()))
            {
                not_found_msg += (QString("<li>\"") + info.value("name") + "\" - ");
                if (openObjectMessage.isEmpty()) {
                    not_found_msg += xi18n("cannot open object");
                } else {
                    not_found_msg += openObjectMessage;
                }
                not_found_msg += internalReason(d->prj->result()) + "<br></li>";
                continue;
            } else {
                d->wasAutoOpen = true;
            }
        }
    }
    setMessagesEnabled(true);

    if (!not_found_msg.isEmpty()) {
        showErrorMessage(xi18n("You have requested selected objects to be automatically opened "
                              "or processed on startup. Several objects cannot be opened or processed."),
                         QString("<ul>%1</ul>").arg(not_found_msg));
    }
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
    if (d->tabbedToolBar) {
        d->tabbedToolBar->hideMainMenu();
    }

    qApp->processEvents();
    emit projectOpened();
}

tristate KexiMainWindow::closeProject()
{
    if (d->tabbedToolBar)
        d->tabbedToolBar->hideMainMenu();

#ifndef KEXI_NO_PENDING_DIALOGS
    if (d->pendingWindowsExist()) {
        qDebug() << "pendingWindowsExist...";
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
        emit acceptProjectClosingRequested(&cancel);
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
        if (d->propEditor) {
//! @todo KEXI3 if (d->openedWindowsCount() == 0)
//! @todo KEXI3 makeWidgetDockVisible(d->propEditorTabWidget);
            KDockWidget *dw = (KDockWidget *)d->propEditorTabWidget->parentWidget();
            KDockSplitter *ds = (KDockSplitter *)dw->parentWidget();
            if (ds)
                ds->setSeparatorPosInPercent(80);
        }

        KDockWidget *dw = (KDockWidget *)d->nav->parentWidget();
        KDockSplitter *ds = (KDockSplitter *)dw->parentWidget();
        int dwWidth = dw->width();
        if (ds) {
            if (d->openedWindowsCount() != 0 && d->propEditorTabWidget && d->propEditorTabWidget->isVisible()) {
                d->navDockSeparatorPos = ds->separatorPosInPercent();
            }
            else {
                d->navDockSeparatorPos = (100 * dwWidth) / width();
            }
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

    if (d->objectViewWidget && d->objectViewWidget->projectNavigator()) {
        d->navWasVisibleBeforeProjectClosing = d->objectViewWidget->projectNavigator()->isVisible();
        d->setProjectNavigatorVisible(false);
        d->objectViewWidget->projectNavigator()->setProject(0);
        //slotProjectNavigatorVisibilityChanged(true); // hide side tab
    }

    if (d->objectViewWidget && d->objectViewWidget->propertyPane()) {
        d->objectViewWidget->propertyPane()->hide();
        d->action_show_propeditor->setChecked(false);
    }
    d->clearWindows(); //sanity!
    delete d->prj;
    d->prj = 0;

    updateReadOnlyState();
    invalidateActions();
    updateAppCaption();
    if (d->userMode) {
        d->modeSelector->setCurrentMode(Kexi::WelcomeGlobalMode);
    }

    emit projectClosed();
    return true;
}

void KexiMainWindow::setupContextHelp()
{
#ifdef KEXI_SHOW_CONTEXT_HELP
    d->ctxHelp = new KexiContextHelp(d->mainWidget, this);
    //! @todo
    /*
      d->ctxHelp->setContextHelp(xi18n("Welcome"),xi18n("The <B>KEXI team</B> wishes you a lot of productive work, "
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
    QWidget *centralWidget = new QWidget;
    setCentralWidget(centralWidget);

    QVBoxLayout *vlyr = new QVBoxLayout(centralWidget);
    vlyr->setContentsMargins(0, 0, 0, 0);
    vlyr->setSpacing(0);

    if (d->isMainMenuVisible) {
        QWidget *tabbedToolBarContainer = new QWidget(this);
        vlyr->addWidget(tabbedToolBarContainer);
        QVBoxLayout *tabbedToolBarContainerLyr = new QVBoxLayout(tabbedToolBarContainer);
        tabbedToolBarContainerLyr->setSpacing(0);
        tabbedToolBarContainerLyr->setContentsMargins(
            KexiUtils::marginHint() / 2, KexiUtils::marginHint() / 2,
            KexiUtils::marginHint() / 2, KexiUtils::marginHint() / 2);

        d->tabbedToolBar = new KexiTabbedToolBar(tabbedToolBarContainer);
        Q_ASSERT(d->action_tools_locate);
        connect(d->action_tools_locate, SIGNAL(triggered()),
                d->tabbedToolBar, SLOT(activateSearchLineEdit()));
        tabbedToolBarContainerLyr->addWidget(d->tabbedToolBar);
        d->tabbedToolBar->hideTab("form"); //temporarily until createToolbar is split
        d->tabbedToolBar->hideTab("report"); //temporarily until createToolbar is split
    }
    else {
        d->tabbedToolBar = 0;
    }

    QWidget *mainWidgetContainer = new QWidget();
    vlyr->addWidget(mainWidgetContainer, 1);
    QHBoxLayout *mainWidgetContainerLyr = new QHBoxLayout(mainWidgetContainer);
    mainWidgetContainerLyr->setContentsMargins(0, 0, 0, 0);
    mainWidgetContainerLyr->setSpacing(0);

    d->modeSelector = new KexiGlobalViewModeSelector;
    connect(d->modeSelector, &KexiGlobalViewModeSelector::currentModeChanged,
            this, &KexiMainWindow::slotCurrentModeChanged);
    mainWidgetContainerLyr->addWidget(d->modeSelector);
    if (d->userMode) {
        d->modeSelector->hide();
    }

    d->globalViewStack = new QStackedWidget;
    mainWidgetContainerLyr->addWidget(d->globalViewStack, 1);
}

//void KexiMainWindow::slotSetProjectNavigatorVisible(bool set)
//{
//    if (d->navDockWidget)
//        d->navDockWidget->setVisible(set);
//}

//void KexiMainWindow::slotSetPropertyEditorVisible(bool set)
//{
//    if (d->propEditorDockWidget)
//        d->propEditorDockWidget->setVisible(set);
//}

//void KexiMainWindow::slotProjectNavigatorVisibilityChanged(bool visible)
//{
//    d->setTabBarVisible(KMultiTabBar::Left, PROJECT_NAVIGATOR_TABBAR_ID,
//                        d->navDockWidget, !visible);
//}

//void KexiMainWindow::slotPropertyEditorVisibilityChanged(bool visible)
//{
//    if (!d->enable_slotPropertyEditorVisibilityChanged)
//        return;
//    d->setPropertyEditorTabBarVisible(!visible);
//    if (!visible)
//        d->propertyEditorCollapsed = true;
//}

/*
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

static Qt::DockWidgetArea applyRightToLeftToDockArea(Qt::DockWidgetArea area)
{
    if (QApplication::layoutDirection() == Qt::RightToLeft) {
        if (area == Qt::LeftDockWidgetArea) {
            return Qt::RightDockWidgetArea;
        }
        else if (area == Qt::RightDockWidgetArea) {
            return Qt::LeftDockWidgetArea;
        }
    }
    return area;
}*/

void KexiMainWindow::setupObjectView()
{
    if (d->objectViewWidget) {
        return;
    }
    KexiObjectViewWidget::Flags flags;
    if (d->isProjectNavigatorVisible) {
        flags |= KexiObjectViewWidget::ProjectNavigatorEnabled;
    }
    if (!d->userMode) {
        flags |= KexiObjectViewWidget::PropertyPaneEnabled;
    }
    d->objectViewWidget = new KexiObjectViewWidget(flags);
    connect(d->objectViewWidget, &KexiObjectViewWidget::activeWindowChanged,
            this, &KexiMainWindow::activeWindowChanged);
    connect(d->objectViewWidget, &KexiObjectViewWidget::closeWindowRequested,
            this, &KexiMainWindow::closeWindowForTab);
    connect(d->objectViewWidget, &KexiObjectViewWidget::closeAllWindowsRequested,
            this, &KexiMainWindow::closeAllWindows);
    connect(d->objectViewWidget, &KexiObjectViewWidget::projectNavigatorAnimationFinished,
            this, &KexiMainWindow::slotProjectNavigatorVisibilityChanged);
    slotProjectNavigatorVisibilityChanged(d->objectViewWidget->projectNavigator());

    // Restore settings
    //! @todo FIX LAYOUT PROBLEMS
    KConfigGroup propertyEditorGroup(d->config->group("PropertyEditor"));
    QFont f(KexiStyle::propertyPane().font());
    const qreal pointSizeF = propertyEditorGroup.readEntry("FontPointSize", -1.0f); // points are more accurate
    if (pointSizeF > 0.0) {
        f.setPointSizeF(pointSizeF);
    } else {
        const int pixelSize = propertyEditorGroup.readEntry("FontSize", -1); // compatibility with Kexi 2.x
        if (pixelSize > 0) {
            f.setPixelSize(pixelSize);
        }
    }
    if (d->objectViewWidget->propertyPane()) {
        d->objectViewWidget->propertyPane()->setFont(f);

        KConfigGroup mainWindowGroup(d->config->group("MainWindow"));
        const QSize projectNavigatorSize = mainWindowGroup.readEntry<QSize>("ProjectNavigatorSize", QSize());
        const QSize propertyEditorSize = mainWindowGroup.readEntry<QSize>("PropertyEditorSize", QSize());
        d->objectViewWidget->setSidebarWidths(projectNavigatorSize.isValid() ? projectNavigatorSize.width() : -1,
                                              propertyEditorSize.isValid() ? propertyEditorSize.width() : -1);
    }

    d->globalViewStack->addWidget(d->objectViewWidget);
    KexiProjectNavigator* navigator = d->objectViewWidget->projectNavigator();
    if (navigator) {
        //connect(d->navDockWidget, SIGNAL(visibilityChanged(bool)),
        //    this, SLOT(slotProjectNavigatorVisibilityChanged(bool)));

        //Nav2 Signals
        connect(navigator, SIGNAL(openItem(KexiPart::Item*,Kexi::ViewMode)),
                this, SLOT(openObject(KexiPart::Item*,Kexi::ViewMode)));
        connect(navigator, SIGNAL(openOrActivateItem(KexiPart::Item*,Kexi::ViewMode)),
                this, SLOT(openObjectFromNavigator(KexiPart::Item*,Kexi::ViewMode)));
        connect(navigator, SIGNAL(newItem(KexiPart::Info*)),
                this, SLOT(newObject(KexiPart::Info*)));
        connect(navigator, SIGNAL(removeItem(KexiPart::Item*)),
                this, SLOT(removeObject(KexiPart::Item*)));
        connect(navigator->model(), SIGNAL(renameItem(KexiPart::Item*,QString,bool*)),
                this, SLOT(renameObject(KexiPart::Item*,QString,bool*)));
        connect(navigator->model(), SIGNAL(changeItemCaption(KexiPart::Item*,QString,bool*)),
                this, SLOT(setObjectCaption(KexiPart::Item*,QString,bool*)));
        connect(navigator, SIGNAL(executeItem(KexiPart::Item*)),
                this, SLOT(executeItem(KexiPart::Item*)));
        connect(navigator, SIGNAL(exportItemToClipboardAsDataTable(KexiPart::Item*)),
                this, SLOT(copyItemToClipboardAsDataTable(KexiPart::Item*)));
        connect(navigator, SIGNAL(exportItemToFileAsDataTable(KexiPart::Item*)),
                this, SLOT(exportItemAsDataTable(KexiPart::Item*)));
#ifdef KEXI_QUICK_PRINTING_SUPPORT
        connect(navigator, SIGNAL(printItem(KexiPart::Item*)),
                this, SLOT(printItem(KexiPart::Item*)));
        connect(navigator, SIGNAL(pageSetupForItem(KexiPart::Item*)),
                this, SLOT(showPageSetupForItem(KexiPart::Item*)));
#endif
        connect(navigator, SIGNAL(selectionChanged(KexiPart::Item*)),
                this, SLOT(slotPartItemSelectedInNavigator(KexiPart::Item*)));
    }

    if (navigator) {
        if (d->prj) {
            connect(d->prj, SIGNAL(newItemStored(KexiPart::Item*)),
                    navigator->model(), SLOT(slotAddItem(KexiPart::Item*)));
            connect(d->prj, SIGNAL(itemRemoved(KexiPart::Item)),
                    navigator->model(), SLOT(slotRemoveItem(KexiPart::Item)));
            navigator->setFocus();
        }

        /*if (d->forceShowProjectNavigatorOnCreation) {
            slotShowNavigator();
            d->forceShowProjectNavigatorOnCreation = false;
        } else if (d->forceHideProjectNavigatorOnCreation) {
            d->forceHideProjectNavigatorOnCreation = false;
        }*/
    }

    invalidateActions();
}

void KexiMainWindow::updateObjectView()
{
    setupObjectView();

    if (d->prj && d->prj->isConnected()) {
        KexiProjectNavigator* navigator = d->objectViewWidget->projectNavigator();
        if (navigator && !navigator->model()->project()) {
            QString partManagerErrorMessages;
            navigator->setProject(d->prj, QString()/*all classes*/, &partManagerErrorMessages);
            if (partManagerErrorMessages.isEmpty()) {
                d->setProjectNavigatorVisible(true);
            } else {
                showWarningContinueMessage(partManagerErrorMessages, QString(),
                                           "ShowWarningsRelatedToPluginsLoading");
            }
        }
    }
}

void KexiMainWindow::slotLastActions()
{
}

void KexiMainWindow::slotPartLoaded(KexiPart::Part* p)
{
    if (!p)
        return;
    p->createGUIClients();
}

void KexiMainWindow::updateAppCaption()
{
//! @todo allow to set custom "static" app caption

    d->appCaptionPrefix.clear();
    if (d->prj && d->prj->data()) {//add project name
        d->appCaptionPrefix = d->prj->data()->caption();
        if (d->appCaptionPrefix.isEmpty()) {
            d->appCaptionPrefix = d->prj->data()->databaseName();
        }
        if (d->prj->dbConnection()->options()->isReadOnly()) {
            d->appCaptionPrefix = xi18nc("<project-name> (read only)", "%1 (read only)", d->appCaptionPrefix);
        }
    }

    setWindowTitle(d->appCaptionPrefix);
}

bool KexiMainWindow::queryClose()
{
#ifndef KEXI_NO_PENDING_DIALOGS
    if (d->pendingWindowsExist()) {
        qDebug() << "pendingWindowsExist...";
        d->actionToExecuteWhenPendingJobsAreFinished = Private::QuitAction;
        return false;
    }
#endif
    const tristate res = closeProject();
    if (~res)
        return false;

    if (res == true)
        storeSettings();

    if (! ~res) {
        Kexi::deleteGlobalObjects();
        qApp->quit();
    }
    return ! ~res;
}

void KexiMainWindow::closeEvent(QCloseEvent *ev)
{
    if (queryClose()) {
        ev->accept();
    } else {
        ev->ignore();
    }
}

void KexiMainWindow::resizeEvent(QResizeEvent *e)
{
    QMainWindow::resizeEvent(e);
    //qDebug() << "===" << e->size() << size() << isVisible();
}

static const QSize KEXI_MIN_WINDOW_SIZE(1024, 768);

void
KexiMainWindow::restoreSettings()
{
    KConfigGroup mainWindowGroup(d->config->group("MainWindow"));
    const bool maximize = mainWindowGroup.readEntry("Maximized", false);
    const QRect geometry(mainWindowGroup.readEntry("Geometry", QRect()));
    if (geometry.isValid())
        setGeometry(geometry);
    else if (maximize)
        setWindowState(windowState() | Qt::WindowMaximized);
    else {
        QRect desk = QApplication::desktop()->screenGeometry(
            QApplication::desktop()->screenNumber(this));
        if (desk.width() <= KEXI_MIN_WINDOW_SIZE.width()
                || desk.height() <= KEXI_MIN_WINDOW_SIZE.height())
        {
            setWindowState(windowState() | Qt::WindowMaximized);
        } else {
            resize(KEXI_MIN_WINDOW_SIZE);
        }
    }
}

void
KexiMainWindow::storeSettings()
{
    //qDebug();
    KConfigGroup mainWindowGroup(d->config->group("MainWindow"));

    if (isMaximized()) {
        mainWindowGroup.writeEntry("Maximized", true);
        mainWindowGroup.deleteEntry("Geometry");
    } else {
        mainWindowGroup.deleteEntry("Maximized");
        mainWindowGroup.writeEntry("Geometry", geometry());
    }

    if (d->objectViewWidget) {
        int projectNavigatorWidth;
        int propertyEditorWidth;
        d->objectViewWidget->getSidebarWidths(&projectNavigatorWidth, &propertyEditorWidth);
        if (projectNavigatorWidth > 0) {
            mainWindowGroup.writeEntry("ProjectNavigatorSize", QSize(projectNavigatorWidth, 1));
        }
        if (propertyEditorWidth > 0) {
            mainWindowGroup.writeEntry("PropertyEditorSize", QSize(propertyEditorWidth, 1));
        }
    }
    d->config->sync();
}

void
KexiMainWindow::registerChild(KexiWindow *window)
{
    //qDebug();
    connect(window, SIGNAL(dirtyChanged(KexiWindow*)), this, SLOT(slotDirtyFlagChanged(KexiWindow*)));

    if (window->id() != -1) {
        d->insertWindow(window);
    }
    //qDebug() << "ID=" << window->id();
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
    if (!d->objectViewWidget || !d->objectViewWidget->propertyPane())
        return;

    if (   !curWindowPart
        || (/*prevWindowPart &&*/ curWindowPart
             && (prevWindowPart != curWindowPart || prevViewMode != curViewMode)
           )
       )
    {
#ifdef __GNUC__
#warning TODO KexiMainWindow::updateCustomPropertyPanelTabs()
#else
#pragma WARNING(TODO KexiMainWindow::updateCustomPropertyPanelTabs())
#endif
#if 0
        if (d->partForPreviouslySetupPropertyPanelTabs) {
            //remember current page number for this part
            if ((   prevViewMode == Kexi::DesignViewMode
                 && static_cast<KexiPart::Part*>(d->partForPreviouslySetupPropertyPanelTabs) != curWindowPart) //part changed
                || curViewMode != Kexi::DesignViewMode)
            { //..or switching to other view mode
                d->recentlySelectedPropertyPanelPages.insert(
                        d->partForPreviouslySetupPropertyPanelTabs,
                        d->objectViewWidget->propertyPane()->currentIndex());
            }
        }

        //delete old custom tabs (other than 'property' tab)
        const int count = d->objectViewWidget->propertyEditorTabWidget()->count();
        for (int i = 1; i < count; i++)
            d->objectViewWidget->propertyEditorTabWidget()->removeTab(1);
#endif
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
        d->objectViewWidget->propertyPane()->removeAllSections();
        curWindowPart->setupPropertyPane(d->objectViewWidget->propertyPane());

#ifdef __GNUC__
#warning TODO KexiMainWindow::updateCustomPropertyPanelTabs()
#else
#pragma WARNING(TODO KexiMainWindow::updateCustomPropertyPanelTabs())
#endif
#if 0
        //restore current page number for this part
        if (d->recentlySelectedPropertyPanelPages.contains(curWindowPart)) {
            d->objectViewWidget->propertyEditorTabWidget()->setCurrentIndex(
                d->recentlySelectedPropertyPanelPages[ curWindowPart ]
            );
        }
#endif
    }
//#endif
    //new part for 'previously setup tabs'
    d->partForPreviouslySetupPropertyPanelTabs = curWindowPart;
}

void KexiMainWindow::activeWindowChanged(KexiWindow *window, KexiWindow *prevWindow)
{
    //qDebug() << "to=" << (window ? window->windowTitle() : "<none>");
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
        if (currentWindow() && currentWindow()->currentViewMode() != 0 && window) {
            setCurrentMode(viewModeToGlobal(currentWindow()->currentViewMode()));

            //on opening new dialog it can be 0; we don't want this
            d->updatePropEditorVisibility(currentWindow()->currentViewMode());

            restoreDesignTabIfNeeded(window->partItem()->pluginId(), window->currentViewMode(),
                                     prevWindow ? prevWindow->partItem()->identifier() : 0);
            activateDesignTabIfNeeded(window->partItem()->pluginId(),
                                      window->currentViewMode());
        }
    }

    invalidateActions();
    d->updateFindDialogContents();
    if (window)
        window->setFocus();
}

bool
KexiMainWindow::activateWindow(int id)
{
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
    //qDebug();

    d->focus_before_popup = &window;
    d->objectViewWidget->tabWidget()->setCurrentWidget(window.parentWidget()/*container*/);
    window.activate();
    return true;
}

void KexiMainWindow::activateNextWindow()
{
    // Case 1: go to next assistant page
    KexiAssistantPage *page = d->visibleMainMenuWidgetPage();
    if (page) {
        page->next();
        return;
    }
    // Case 2: go to next tab
    activateNextTab();
}

void KexiMainWindow::activatePreviousWindow()
{
    // Case 1: go to previous assistant page
    KexiAssistantPage *page = d->visibleMainMenuWidgetPage();
    if (page) {
        page->tryBack();
        return;
    }
    // Case 2: go to previous tab
    activatePreviousTab();
}

void KexiMainWindow::activateNextTab()
{
    int index = d->objectViewWidget->tabWidget()->currentIndex() + 1;
    if (index >= d->objectViewWidget->tabWidget()->count()) {
        index = 0;
    }
    d->objectViewWidget->tabWidget()->setCurrentIndex(index);
}

void KexiMainWindow::activatePreviousTab()
{
    int index = d->objectViewWidget->tabWidget()->currentIndex() - 1;
    if (index < 0) {
        index = d->objectViewWidget->tabWidget()->count() - 1;
    }
    d->objectViewWidget->tabWidget()->setCurrentIndex(index);
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
    KShortcutsDialog::configure(actionCollection(),
                                KShortcutsEditor::LetterShortcutsDisallowed, this);
}

void
KexiMainWindow::slotConfigureToolbars()
{
    KEditToolBar edit(actionCollection());
    (void) edit.exec();
}

void KexiMainWindow::slotProjectNew()
{
    createNewProject();
}

KexiProject* KexiMainWindow::createKexiProjectObject(const KexiProjectData &data)
{
    KexiProject *prj = new KexiProject(data, this);
    connect(prj, SIGNAL(itemRenamed(KexiPart::Item,QString)), this, SLOT(slotObjectRenamed(KexiPart::Item,QString)));

    if (d->objectViewWidget && d->objectViewWidget->projectNavigator()){
        connect(prj, SIGNAL(itemRemoved(KexiPart::Item)),
                d->objectViewWidget->projectNavigator()->model(),
                SLOT(slotRemoveItem(KexiPart::Item)));
    }
    return prj;
}

void KexiMainWindow::createNewProject()
{
    if (!d->tabbedToolBar)
        return;
    d->tabbedToolBar->showMainMenu("project_new");
    KexiNewProjectAssistant* assistant = new KexiNewProjectAssistant;
    connect(assistant, SIGNAL(createProject(KexiProjectData)),
            this, SLOT(createNewProject(KexiProjectData)));

    d->tabbedToolBar->setMainMenuContent(assistant);
}

tristate KexiMainWindow::createNewProject(const KexiProjectData &projectData)
{
    QScopedPointer<KexiProject> prj(createKexiProjectObject(projectData));
    tristate res = prj->create(true /*overwrite*/);
    if (res != true) {
        return res;
    }
    //qDebug() << "new project created ---";
    if (d->prj) {
        res = openProjectInExternalKexiInstance(
                prj->data()->connectionData()->databaseName(),
                prj->data()->connectionData(),
                prj->data()->databaseName());
        Kexi::recentProjects()->addProjectData(*prj->data());
        if (d->tabbedToolBar) {
            d->tabbedToolBar->hideMainMenu();
        }
        return res;
    }
    if (d->tabbedToolBar) {
        d->tabbedToolBar->hideMainMenu();
    }
    d->prj = prj.take();
    d->prj->data()->setLastOpened(QDateTime::currentDateTime());
    Kexi::recentProjects()->addProjectData(*d->prj->data());
    d->modeSelector->setCurrentMode(Kexi::EditGlobalMode);

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

    KDbConnectionData *cdata = 0;
    if (!fileNameForConnectionData.isEmpty()) {
        cdata = Kexi::connset().connectionDataForFileName(fileNameForConnectionData);
        if (!cdata) {
            qWarning() << "cdata?";
            return false;
        }
    }
    return openProject(aFileName, cdata, dbName);
}

tristate KexiMainWindow::openProject(const QString& aFileName,
                                     KDbConnectionData *cdata, const QString& dbName,
                                     const KexiProjectData::AutoOpenObjects& autoopenObjects)
{
    if (d->prj) {
        return openProjectInExternalKexiInstance(aFileName, cdata, dbName);
    }

    KexiProjectData* projectData = 0;
    const KexiStartupHandler *h = KexiStartupHandler::global();
    bool readOnly = h->isSet(h->options().readOnly);
    bool deleteAfterOpen = false;
    if (cdata) {
        //server-based project
        if (dbName.isEmpty()) {//no database name given, ask user
            bool cancel;
            projectData = KexiStartupHandler::global()->selectProject(cdata, &cancel, this);
            if (cancel)
                return cancelled;
        } else {
//! @todo caption arg?
            projectData = new KexiProjectData(*cdata, dbName);
            deleteAfterOpen = true;
        }
    } else {
        if (aFileName.isEmpty()) {
            qWarning() << "aFileName.isEmpty()";
            return false;
        }
        //file-based project
        //qDebug() << "Project File:" << aFileName;
        KDbConnectionData fileConnData;
        fileConnData.setDatabaseName(aFileName);
        QString detectedDriverId;
        int detectOptions = 0;
        if (readOnly) {
            detectOptions |= KexiStartupHandler::OpenReadOnly;
        }
        KexiStartupData::Import importActionData;
        bool forceReadOnly;
        const tristate res = KexiStartupHandler::detectActionForFile(
                                 &importActionData, &detectedDriverId, fileConnData.driverId(),
                                 aFileName, this, detectOptions, &forceReadOnly);
        if (forceReadOnly) {
            readOnly = true;
        }
        if (true != res)
            return res;

        if (importActionData) { //importing requested
            return showProjectMigrationWizard(importActionData.mimeType, importActionData.fileName);
        }
        fileConnData.setDriverId(detectedDriverId);

        if (fileConnData.driverId().isEmpty())
            return false;

        //opening requested
        projectData = new KexiProjectData(fileConnData);
        deleteAfterOpen = true;
    }
    if (!projectData)
        return false;
    projectData->setReadOnly(readOnly);
    projectData->autoopenObjects = autoopenObjects;
    const tristate res = openProject(*projectData);
    if (deleteAfterOpen) //projectData object has been copied
        delete projectData;
    return res;
}

tristate KexiMainWindow::openProjectInExternalKexiInstance(const QString& aFileName,
        KDbConnectionData *cdata, const QString& dbName)
{
    QString fileNameForConnectionData;
    if (aFileName.isEmpty()) { //try .kexic file
        if (cdata)
            fileNameForConnectionData = Kexi::connset().fileNameForConnectionData(*cdata);
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
            if (fileNameForConnectionData.isEmpty()) {
                qWarning() << "fileNameForConnectionData?";
                return false;
            }
            args << "--connection" << fileNameForConnectionData;
            fileName = dbName;
        }
    }
    if (fileName.isEmpty()) {
        qWarning() << "fileName?";
        return false;
    }
//! @todo use KRun
//! @todo untested
//Can arguments be supplied to KRun like is used here? AP
    args << fileName;
    const bool ok = QProcess::startDetached(
        qApp->applicationFilePath(), args,
        QFileInfo(fileName).absoluteDir().absolutePath());
    if (!ok) {
        d->showStartProcessMsg(args);
    }
    if (d->tabbedToolBar) {
        d->tabbedToolBar->hideMainMenu();
    }
    return ok;
}

void KexiMainWindow::slotProjectWelcome()
{
    if (!d->tabbedToolBar)
        return;
    d->tabbedToolBar->showMainMenu("project_welcome");
    KexiWelcomeAssistant* assistant = new KexiWelcomeAssistant(
        Kexi::recentProjects(), this);
    connect(assistant, SIGNAL(openProject(KexiProjectData,QString,bool*)),
            this, SLOT(openProject(KexiProjectData,QString,bool*)));
    d->tabbedToolBar->setMainMenuContent(assistant);
}

void
KexiMainWindow::slotProjectSave()
{
    if (!currentWindow() || currentWindow()->currentViewMode() == Kexi::DataViewMode) {
        return;
    }
    saveObject(currentWindow());
    updateAppCaption();
    invalidateActions();
}

void
KexiMainWindow::slotProjectSaveAs()
{
    if (!currentWindow() || currentWindow()->currentViewMode() == Kexi::DataViewMode) {
        return;
    }
    saveObject(currentWindow(), QString(), SaveObjectAs);
    updateAppCaption();
    invalidateActions();
}

void
KexiMainWindow::slotProjectPrint()
{
#ifdef KEXI_QUICK_PRINTING_SUPPORT
    if (currentWindow() && currentWindow()->partItem())
        printItem(currentWindow()->partItem());
#endif
}

void
KexiMainWindow::slotProjectPrintPreview()
{
#ifdef KEXI_QUICK_PRINTING_SUPPORT
    if (currentWindow() && currentWindow()->partItem())
        printPreviewForItem(currentWindow()->partItem());
#endif
}

void
KexiMainWindow::slotProjectPageSetup()
{
#ifdef KEXI_QUICK_PRINTING_SUPPORT
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
    //! @todo load the implementation not the ui :)
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
    KexiWindow *w = KexiInternalPart::createKexiWindowInstance("org.kexi-project.relations", this);
    if (w) {
        activateWindow(*w);
    }
}

void KexiMainWindow::slotImportFile()
{
    KEXI_UNFINISHED("Import: " + xi18n("From File..."));
}

void KexiMainWindow::slotImportServer()
{
    KEXI_UNFINISHED("Import: " + xi18n("From Server..."));
}

void
KexiMainWindow::slotProjectQuit()
{
    if (~ closeProject())
        return;
    close();
}

void KexiMainWindow::slotActivateNavigator()
{
    if (!d->objectViewWidget || !d->objectViewWidget->projectNavigator()) {
        return;
    }
    d->objectViewWidget->projectNavigator()->setFocus();
}

void KexiMainWindow::slotActivateMainArea()
{
    if (currentWindow())
        currentWindow()->setFocus();
}

void KexiMainWindow::slotActivatePropertyPane()
{
    if (!d->objectViewWidget || !d->objectViewWidget->propertyPane()) {
        return;
    }

    d->objectViewWidget->propertyPane()->setFocus();
//    if (d->objectViewWidget->propertyPane()->currentWidget()) {
//        d->objectViewWidget->propertyPane()->currentWidget()->setFocus();
//    }
}

void KexiMainWindow::slotToggleProjectNavigator()
{
    if (d->objectViewWidget && d->objectViewWidget->projectNavigator()) {
        d->setProjectNavigatorVisible(!d->objectViewWidget->projectNavigator()->isVisible(), Private::ShowAnimated);
    }
}

void KexiMainWindow::slotTogglePropertyEditor()
{
    if (d->objectViewWidget && d->objectViewWidget->propertyPane()) {
        d->objectViewWidget->setPropertyPaneVisible(!d->objectViewWidget->propertyPane()->isVisible());
    }
}

tristate KexiMainWindow::switchToViewMode(KexiWindow& window, Kexi::ViewMode viewMode)
{
    const Kexi::ViewMode prevViewMode = currentWindow()->currentViewMode();
    if (prevViewMode == viewMode)
        return true;
    if (!activateWindow(window))
        return false;
    if (!currentWindow()) {
        return false;
    }
    if (&window != currentWindow())
        return false;
    if (!currentWindow()->supportsViewMode(viewMode)) {
        showErrorMessage(xi18nc("@info",
                                "Selected view is not supported for <resource>%1</resource> object.",
                                currentWindow()->partItem()->name()),
                         xi18nc("@info",
                                "Selected view (%1) is not supported by this object type (%2).",
                                Kexi::nameForViewMode(viewMode),
                                currentWindow()->part()->info()->name()));
        return false;
    }
    setCurrentMode(viewModeToGlobal(viewMode));

    updateCustomPropertyPanelTabs(currentWindow()->part(), prevViewMode,
                                  currentWindow()->part(), viewMode);
    tristate res = currentWindow()->switchToViewMode(viewMode);
    if (!res) {
        updateCustomPropertyPanelTabs(0, Kexi::NoViewMode); //revert
        showErrorMessage(xi18n("Switching to other view failed (%1).",
                              Kexi::nameForViewMode(viewMode)), currentWindow());
        return false;
    }
    if (~res) {
        updateCustomPropertyPanelTabs(0, Kexi::NoViewMode); //revert
        return cancelled;
    }

    activateWindow(window);

    invalidateSharedActions();
    invalidateProjectWideActions();
    d->updateFindDialogContents();
    d->updatePropEditorVisibility(viewMode);
    QString origTabToActivate;
    if (viewMode == Kexi::DesignViewMode) {
        // Save the orig tab: we want to back to design tab
        // when user moved to data view and then immediately to design view.
        origTabToActivate = d->tabsToActivateOnShow.value(currentWindow()->partItem()->identifier());
    }
    restoreDesignTabIfNeeded(currentWindow()->partItem()->pluginId(), viewMode,
                             currentWindow()->partItem()->identifier());
    if (viewMode == Kexi::DesignViewMode) {
        activateDesignTab(currentWindow()->partItem()->pluginId());
        // Restore the saved tab to the orig one. restoreDesignTabIfNeeded() saved tools tab probably.
        d->tabsToActivateOnShow.insert(currentWindow()->partItem()->identifier(), origTabToActivate);
    }

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

//! Used to control if we're not Saving-As object under the original name
class SaveAsObjectNameValidator : public KexiNameDialogValidator
{
public:
    explicit SaveAsObjectNameValidator(const QString &originalObjectName)
     : m_originalObjectName(originalObjectName)
    {
    }
    virtual bool validate(KexiNameDialog *dialog) const override {
        if (dialog->widget()->nameText() == m_originalObjectName) {
            KMessageBox::information(dialog,
                                     xi18nc("Could not save object under the original name.",
                                           "Could not save under the original name."));
            return false;
        }
        return true;
    }
private:
    QString m_originalObjectName;
};

tristate KexiMainWindow::getNewObjectInfo(
    KexiPart::Item *partItem, const QString &originalName, KexiPart::Part *part,
    bool allowOverwriting, bool *overwriteNeeded, const QString& messageWhenAskingForName)
{
    //data was never saved in the past -we need to create a new object at the backend
    KexiPart::Info *info = part->info();
    if (!d->nameDialog) {
        d->nameDialog = new KexiNameDialog(
            messageWhenAskingForName, this);
        //check if that name is allowed
        d->nameDialog->widget()->addNameSubvalidator(
            new KDbObjectNameValidator(project()->dbConnection()->driver()));
        d->nameDialog->buttonBox()->button(QDialogButtonBox::Ok)->setText(xi18nc("@action:button Save object", "Save"));
    } else {
        d->nameDialog->widget()->setMessageText(messageWhenAskingForName);
    }
    d->nameDialog->widget()->setCaptionText(partItem->caption());
    d->nameDialog->widget()->setNameText(partItem->name());
    d->nameDialog->setWindowTitle(xi18nc("@title:window", "Save Object As"));
    d->nameDialog->setDialogIcon(info->iconName());
    d->nameDialog->setAllowOverwriting(allowOverwriting);
    if (!originalName.isEmpty()) {
        d->nameDialog->setValidator(new SaveAsObjectNameValidator(originalName));
    }
    if (d->nameDialog->execAndCheckIfObjectExists(*project(), *part, overwriteNeeded)
        != QDialog::Accepted)
    {
        return cancelled;
    }

    // close window of object that will be overwritten
    if (*overwriteNeeded) {
        KexiPart::Item* overwrittenItem = project()->item(info, d->nameDialog->widget()->nameText());
        if (overwrittenItem) {
            KexiWindow * openedWindow = d->openedWindowFor(overwrittenItem->identifier());
            if (openedWindow) {
                const tristate res = closeWindow(openedWindow);
                if (res != true) {
                    return res;
                }
            }
        }
    }

    //update name and caption
    partItem->setName(d->nameDialog->widget()->nameText());
    partItem->setCaption(d->nameDialog->widget()->captionText());
    return true;
}

//! Used to delete part item on exit from block
class PartItemDeleter : public QScopedPointer<KexiPart::Item>
{
    public:
        explicit PartItemDeleter(KexiProject *prj) : m_prj(prj) {}
        ~PartItemDeleter() {
            if (!isNull()) {
                m_prj->deleteUnstoredItem(take());
            }
        }
    private:
        KexiProject *m_prj;
};

static void showSavingObjectFailedErrorMessage(KexiMainWindow *wnd, KexiPart::Item *item)
{
    wnd->showErrorMessage(
        xi18nc("@info Saving object failed",
              "Saving <resource>%1</resource> object failed.", item->name()),
              wnd->currentWindow());
}

tristate KexiMainWindow::saveObject(KexiWindow *window, const QString& messageWhenAskingForName,
                                    SaveObjectOptions options)
{
    tristate res;
    bool saveAs = options & SaveObjectAs;
    if (!saveAs && !window->neverSaved()) {
        //data was saved in the past -just save again
        res = window->storeData(options & DoNotAsk);
        if (!res) {
            showSavingObjectFailedErrorMessage(this, window->partItem());
        }
        return res;
    }
    if (saveAs && window->neverSaved()) {
        //if never saved, saveAs == save
        saveAs = false;
    }

    const int oldItemID = window->partItem()->identifier();

    KexiPart::Item *partItem;
    KexiView::StoreNewDataOptions storeNewDataOptions;
    PartItemDeleter itemDeleter(d->prj);
    if (saveAs) {
        partItem = d->prj->createPartItem(window->part());
        if (!partItem) {
            //! @todo error
            return false;
        }
        itemDeleter.reset(partItem);
    }
    else {
        partItem = window->partItem();
    }

    bool overwriteNeeded;
    res = getNewObjectInfo(partItem, saveAs ? window->partItem()->name() : QString(),
                           window->part(), true /*allowOverwriting*/,
                           &overwriteNeeded, messageWhenAskingForName);
    if (res != true)
        return res;
    if (overwriteNeeded) {
        storeNewDataOptions |= KexiView::OverwriteExistingData;
    }

    if (saveAs) {
        res = window->storeDataAs(partItem, storeNewDataOptions);
    }
    else {
        res = window->storeNewData(storeNewDataOptions);
    }

    if (~res)
        return cancelled;
    if (!res) {
        showSavingObjectFailedErrorMessage(this, partItem);
        return false;
    }

    d->updateWindowId(window, oldItemID);
    invalidateProjectWideActions();
    itemDeleter.take();
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

tristate KexiMainWindow::closeAllWindows()
{
    if (!d->objectViewWidget || !d->objectViewWidget->tabWidget())
        return true;
    QList<KexiWindow*> windowList;
    for (int i = 0; i < d->objectViewWidget->tabWidget()->count(); ++i) {
        KexiWindow *window = windowForTab(i);
        if (window) {
            windowList.append(window);
        }
    }
    tristate alternateResult = true;
    for (KexiWindow *window : windowList) {
        const tristate result = closeWindow(window);
        if (~result) {
            return result;
        }
        if (result == false) {
            alternateResult = false;
        }
    }
    return alternateResult;
}

tristate KexiMainWindow::closeWindow(KexiWindow *window, bool layoutTaskBar, bool doNotSaveChanges)
{
//! @todo KEXI3 KexiMainWindow::closeWindow()
    ///@note Q_UNUSED layoutTaskBar
    Q_UNUSED(layoutTaskBar);

    if (!window)
        return true;
    if (d->insideCloseWindow)
        return true;

    const int previousItemId = window->partItem()->identifier();

#ifndef KEXI_NO_PENDING_DIALOGS
    d->addItemToPendingWindows(window->partItem(), Private::WindowClosingJob);
#endif

    d->insideCloseWindow = true;

    if (window == currentWindow() && !window->isAttached()) {
        if (d->propertyEditor()) {
            // ah, closing detached window - better switch off property buffer right now...
            d->propertySet = 0;
            d->propertyEditor()->changeSet(0);
        }
    }

    bool remove_on_closing = window->partItem() ? window->partItem()->neverSaved() : false;
    if (window->isDirty() && !d->forceWindowClosing && !doNotSaveChanges) {
        //more accurate tool tips and what's this
        KGuiItem saveChanges(KStandardGuiItem::save());
        saveChanges.setToolTip(xi18n("Save changes"));
        saveChanges.setWhatsThis(
            xi18nc("@info", "Saves all recent changes made in <resource>%1</resource> object.",
                   window->partItem()->name()));
        KGuiItem discardChanges(KStandardGuiItem::discard());
        discardChanges.setWhatsThis(
            xi18nc("@info", "Discards all recent changes made in <resource>%1</resource> object.",
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

        if (additionalMessageString.startsWith(':'))
            additionalMessageString.clear();
        if (!additionalMessageString.isEmpty())
            additionalMessageString = "<p>" + additionalMessageString + "</p>";

        const KMessageBox::ButtonCode questionRes = KMessageBox::warningTwoActionsCancel(this,
                                "<p>"
                                + window->part()->i18nMessage("Design of object <resource>%1</resource> has been modified.", window)
                                .subs(window->partItem()->name()).toString()
                                + "</p><p>" + xi18n("Do you want to save changes?") + "</p>"
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
        if (questionRes == KMessageBox::PrimaryAction) {
            //save it
            tristate res = saveObject(window, QString(), DoNotAsk);
            if (!res || ~res) {
//! @todo show error info; (retry/ignore/cancel)
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
    if (remove_on_closing) {
        //we won't save this object, and it was never saved -remove it
        if (!removeObject(window->partItem(), true)) {
#ifndef KEXI_NO_PENDING_DIALOGS
            d->removePendingWindow(window->id());
#endif
            //msg?
            //! @todo ask if we'd continue and return true/false
            d->insideCloseWindow = false;
            d->windowsToClose.clear(); //give up with 'close all'
            return false;
        }
    } else {
        //not dirty now
        if (d->objectViewWidget && d->objectViewWidget->projectNavigator()) {
            d->objectViewWidget->projectNavigator()->updateItemName(*window->partItem(), false);
        }
    }

    hideDesignTab(previousItemId, QString());

    d->removeWindow(window_id);
    d->setWindowContainerExistsFor(window->partItem()->identifier(), false);
    QWidget *windowContainer = window->parentWidget();
    d->objectViewWidget->tabWidget()->removeTab(
        d->objectViewWidget->tabWidget()->indexOf(windowContainer));

#ifdef KEXI_QUICK_PRINTING_SUPPORT
    //also remove from 'print setup dialogs' cache, if needed
    int printedObjectID = 0;
    if (d->pageSetupWindowItemID2dataItemID_map.contains(window_id))
        printedObjectID = d->pageSetupWindowItemID2dataItemID_map[ window_id ];
    d->pageSetupWindows.remove(printedObjectID);
#endif

    delete windowContainer;

    //focus navigator if nothing else available
    if (d->openedWindowsCount() == 0) {
        if (d->objectViewWidget && d->objectViewWidget->projectNavigator()) {
            d->objectViewWidget->projectNavigator()->setFocus();
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
    //d->objectViewWidget->slotCurrentTabIndexChanged(d->objectViewWidget->tabWidget()->currentIndex());
    showDesignTabIfNeeded(0);

    if (currentWindow()) {
        restoreDesignTabIfNeeded(currentWindow()->partItem()->pluginId(),
                                 currentWindow()->currentViewMode(),
                                 0);
    }
    d->tabsToActivateOnShow.remove(previousItemId);
    return true;
}

QWidget* KexiMainWindow::findWindow(QWidget *w)
{
    while (w && !acceptsSharedActions(w)) {
        if (w == d->objectViewWidget->propertyPane()) {
            return currentWindow();
        }
        w = w->parentWidget();
    }
    return w;
}

KexiWindow* KexiMainWindow::openedWindowFor(int identifier)
{
    return d->openedWindowFor(identifier);
}

KexiWindow* KexiMainWindow::openedWindowFor(const KexiPart::Item* item)
{
    return item ? openedWindowFor(item->identifier()) : 0;
}

KDbQuerySchema* KexiMainWindow::unsavedQuery(int queryId)
{
    KexiWindow * queryWindow = openedWindowFor(queryId);
    if (!queryWindow || !queryWindow->isDirty()) {
        return 0;
    }

    return queryWindow->part()->currentQuery(queryWindow->viewForMode(Kexi::DataViewMode));
}

QList<QVariant> KexiMainWindow::currentParametersForQuery(int queryId) const
{
    KexiWindow *queryWindow = d->openedWindowFor(queryId);
    if (!queryWindow) {
        return QList<QVariant>();
    }

    KexiView *view = queryWindow->viewForMode(Kexi::DataViewMode);
    if (!view) {
        return QList<QVariant>();
    }

    return view->currentParameters();
}

bool KexiMainWindow::acceptsSharedActions(QObject *w)
{
    return w->inherits("KexiWindow") || w->inherits("KexiView");
}

bool KexiMainWindow::openingAllowed(KexiPart::Item* item, Kexi::ViewMode viewMode, QString* errorMessage)
{
    //qDebug() << viewMode;
    //! @todo this can be more complex once we deliver ACLs...
    if (!d->userMode)
        return true;
    KexiPart::Part * part = Kexi::partManager().partForPluginId(item->pluginId());
    if (!part) {
        if (errorMessage) {
            *errorMessage = Kexi::partManager().result().message();
        }
    }
    //qDebug() << part << item->pluginId();
    //if (part)
    //    qDebug() << item->pluginId() << part->info()->supportedUserViewModes();
    return part && (part->info()->supportedUserViewModes() & viewMode);
}

KexiWindow *
KexiMainWindow::openObject(const QString& pluginId, const QString& name,
                           Kexi::ViewMode viewMode, bool *openingCancelled, QMap<QString, QVariant>* staticObjectArgs)
{
    KexiPart::Item *item = d->prj->itemForPluginId(pluginId, name);
    if (!item)
        return 0;
    return openObject(item, viewMode, openingCancelled, staticObjectArgs);
}

KexiWindow *
KexiMainWindow::openObject(KexiPart::Item* item, Kexi::ViewMode viewMode, bool *openingCancelled,
                           QMap<QString, QVariant>* staticObjectArgs, QString* errorMessage)
{
    Q_ASSERT(openingCancelled);
    if (!d->prj || !item) {
        return 0;
    }

    if (!openingAllowed(item, viewMode, errorMessage)) {
        if (errorMessage)
            *errorMessage = xi18nc(
                                "opening is not allowed in \"data view/design view/text view\" mode",
                                "opening is not allowed in \"%1\" mode", Kexi::nameForViewMode(viewMode));
        *openingCancelled = true;
        return 0;
    }
    //qDebug() << d->prj << item;

    KexiWindow *prevWindow = currentWindow();

    KexiUtils::WaitCursor wait;
#ifndef KEXI_NO_PENDING_DIALOGS
    Private::PendingJobType pendingType;
    KexiWindow *window = d->openedWindowFor(item, pendingType);
    if (pendingType != Private::NoJob) {
        *openingCancelled = true;
        return 0;
    }
#else
    KexiWindow *window = openedWindowFor(item);
#endif
    int previousItemId = currentWindow() ? currentWindow()->partItem()->identifier() : 0;
    *openingCancelled = false;

    bool alreadyOpened = false;
    QWidget *windowContainer = 0;

    if (window) {
        if (viewMode != window->currentViewMode()) {
            if (true != switchToViewMode(*window, viewMode))
                return 0;
        } else
            activateWindow(*window);
        alreadyOpened = true;
    } else {
        if (d->windowContainerExistsFor(item->identifier())) {
            // window not yet present but window container exists: return 0 and wait
            return 0;
        }
        KexiPart::Part *part = Kexi::partManager().partForPluginId(item->pluginId());
        d->updatePropEditorVisibility(viewMode, part ? part->info() : 0);
        //update tabs before opening
        updateCustomPropertyPanelTabs(currentWindow() ? currentWindow()->part() : 0,
                                      currentWindow() ? currentWindow()->currentViewMode() : Kexi::NoViewMode,
                                      part, viewMode);

        const int tabIndex = d->objectViewWidget->tabWidget()->addEmptyContainerTab(
                    part ? part->info()->icon() : koIcon("object"),
                    KexiWindow::windowTitleForItem(*item));
        // open new tab earlier
        d->setWindowContainerExistsFor(item->identifier(), true);
        d->objectViewWidget->tabWidget()->setTabToolTip(tabIndex, KexiPart::fullCaptionForItem(item, part));
        QString whatsThisText;
        if (part) {
            whatsThisText = xi18nc("@info",
                                   "Tab for <resource>%1</resource> (%2).",
                                   item->captionOrName(), part->info()->name());
        }
        else {
            whatsThisText = xi18nc("@info",
                                   "Tab for <resource>%1</resource>.", item->captionOrName());
        }
        d->objectViewWidget->tabWidget()->setTabWhatsThis(tabIndex, whatsThisText);
        d->objectViewWidget->tabWidget()->setCurrentIndex(tabIndex);

#ifndef KEXI_NO_PENDING_DIALOGS
        d->addItemToPendingWindows(item, Private::WindowOpeningJob);
#endif
        windowContainer = d->objectViewWidget->tabWidget()->widget(tabIndex);
        window = d->prj->openObject(windowContainer, item, viewMode, staticObjectArgs);
        if (window) {
            d->objectViewWidget->tabWidget()->setWindowForTab(tabIndex, window);
            // update text and icon
            d->objectViewWidget->tabWidget()->setTabText(tabIndex, window->windowTitle());
            d->objectViewWidget->tabWidget()->setTabIcon(tabIndex, window->windowIcon());
        }
    }

    if (!window || !activateWindow(*window)) {
#ifndef KEXI_NO_PENDING_DIALOGS
        d->removePendingWindow(item->identifier());
#endif
        d->setWindowContainerExistsFor(item->identifier(), false);
        d->objectViewWidget->tabWidget()->removeTab(
            d->objectViewWidget->tabWidget()->indexOf(windowContainer));
        delete windowContainer;
        updateCustomPropertyPanelTabs(0, Kexi::NoViewMode); //revert
        //! @todo add error msg...
        return 0;
    }

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
        // Call switchToViewMode() and propertySetSwitched() again here because
        // this is the time when then new window is the current one - previous call did nothing.
        switchToViewMode(*window, window->currentViewMode());
        currentWindow()->selectedView()->propertySetSwitched();
    }
    invalidateProjectWideActions();
    restoreDesignTabIfNeeded(item->pluginId(), viewMode, previousItemId);
    activateDesignTabIfNeeded(item->pluginId(), viewMode);
    QString origTabToActivate;
    if (prevWindow) {
        // Save the orig tab for prevWindow that was stored in the restoreDesignTabIfNeeded() call above
        origTabToActivate = d->tabsToActivateOnShow.value(prevWindow->partItem()->identifier());
    }
    activeWindowChanged(window, prevWindow);
    if (prevWindow) {
        // Restore the orig tab
        d->tabsToActivateOnShow.insert(prevWindow->partItem()->identifier(), origTabToActivate);
    }
    return window;
}

KexiWindow *
KexiMainWindow::openObjectFromNavigator(KexiPart::Item* item, Kexi::ViewMode viewMode)
{
    bool openingCancelled;
    return openObjectFromNavigator(item, viewMode, &openingCancelled);
}

KexiWindow *
KexiMainWindow::openObjectFromNavigator(KexiPart::Item* item, Kexi::ViewMode viewMode,
                                        bool *openingCancelled)
{
    Q_ASSERT(openingCancelled);
    if (!openingAllowed(item, viewMode)) {
        *openingCancelled = true;
        return 0;
    }
    if (!d->prj || !item)
        return 0;
#ifndef KEXI_NO_PENDING_DIALOGS
    Private::PendingJobType pendingType;
    KexiWindow *window = d->openedWindowFor(item, pendingType);
    if (pendingType != Private::NoJob) {
        *openingCancelled = true;
        return 0;
    }
#else
    KexiWindow *window = openedWindowFor(item);
#endif
    *openingCancelled = false;
    if (window) {
        if (activateWindow(*window)) {
            return window;
        }
    }
    //if DataViewMode is not supported, try Design, then Text mode (currently useful for script part)
    KexiPart::Part *part = Kexi::partManager().partForPluginId(item->pluginId());
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
    KexiWindow *window = openedWindowFor(item);
#endif
    if (!window)
        return cancelled;
    return closeWindow(window);
}

bool KexiMainWindow::newObject(KexiPart::Info *info, bool* openingCancelled)
{
    Q_ASSERT(openingCancelled);
    if (d->userMode) {
        *openingCancelled = true;
        return false;
    }
    *openingCancelled = false;
    if (!d->prj || !info)
        return false;
    KexiPart::Part *part = Kexi::partManager().part(info);
    if (!part)
        return false;

    KexiPart::Item *it = d->prj->createPartItem(info);
    if (!it) {
        //! @todo error
        return false;
    }

    if (!it->neverSaved()) { //only add stored objects to the browser
        d->objectViewWidget->projectNavigator()->model()->slotAddItem(it);
    }
    Kexi::ViewMode viewMode = (info->supportedViewModes() & Kexi::DesignViewMode)
            ? Kexi::DesignViewMode
            : Kexi::TextViewMode;
    return openObject(it, viewMode, openingCancelled);
}

tristate KexiMainWindow::removeObject(KexiPart::Item *item, bool dontAsk)
{
    if (d->userMode)
        return cancelled;
    if (!d->prj || !item)
        return false;

    KexiPart::Part *part = Kexi::partManager().partForPluginId(item->pluginId());
    if (!part)
        return false;

    if (!dontAsk) {
        if (KMessageBox::SecondaryAction == KMessageBox::questionTwoActions(this,
                xi18nc("@info Delete <objecttype> <objectname>?",
                      "<para>Do you want to permanently delete the following object?<nl/>"
                      "<nl/>%1 <resource>%2</resource></para>"
                      "<para><note>If you click <interface>Delete</interface>, "
                      "you will not be able to undo the deletion.</note></para>",
                      part->info()->name(), item->name()),
                xi18nc("@title:window Delete Object %1.",
                      "Delete <resource>%1</resource>?", item->name()),
                KStandardGuiItem::del(),
                KStandardGuiItem::cancel(), QString(), KMessageBox::Notify | KMessageBox::Dangerous))
        {
            return cancelled;
        }
    }

    tristate res = true;
#ifdef KEXI_QUICK_PRINTING_SUPPORT
    //also close 'print setup' dialog for this item, if any
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
    KexiWindow *window = openedWindowFor(item);
#endif

    if (window) {//close existing window
        const bool tmp = d->forceWindowClosing;
        d->forceWindowClosing = true;
        res = closeWindow(window);
        d->forceWindowClosing = tmp; //restore
        if (!res || ~res) {
            return res;
        }
    }

#ifdef KEXI_QUICK_PRINTING_SUPPORT
    //in case the dialog is a 'print setup' dialog, also update d->pageSetupWindows
    int dataItemID = d->pageSetupWindowItemID2dataItemID_map[item->identifier()];
    d->pageSetupWindowItemID2dataItemID_map.remove(item->identifier());
    d->pageSetupWindows.remove(dataItemID);
#endif

    if (!d->prj->removeObject(item)) {
        //! @todo better msg
        showSorryMessage(xi18n("Could not delete object."));
        return false;
    }
    return true;
}

void KexiMainWindow::renameObject(KexiPart::Item *item, const QString& _newName, bool *success)
{
    Q_ASSERT(success);
    if (d->userMode) {
        *success = false;
        return;
    }
    QString newName = _newName.trimmed();
    if (newName.isEmpty()) {
        showSorryMessage(xi18n("Could not set empty name for this object."));
        *success = false;
        return;
    }

    KexiWindow *window = openedWindowFor(item);
    if (window) {
        QString msg = xi18nc("@info",
                            "<para>Before renaming object <resource>%1</resource> it should be closed.</para>"
                            "<para>Do you want to close it?</para>",
                            item->name());
        KGuiItem closeAndRenameItem(KStandardGuiItem::closeWindow());
        closeAndRenameItem.setText(xi18n("Close Window and Rename"));
        const int r = KMessageBox::questionTwoActions(this, msg, QString(), closeAndRenameItem,
                                           KStandardGuiItem::cancel());
        if (r != KMessageBox::PrimaryAction) {
            *success = false;
            return;
        }
        const tristate closeResult = closeWindow(window);
        if (closeResult != true) {
            *success = false;
            return;
        }
    }
    setMessagesEnabled(false); //to avoid double messages
    const bool res = d->prj->renameObject(item, newName);
    setMessagesEnabled(true);
    if (!res) {
        showErrorMessage(xi18nc("@info", "Renaming object <resource>%1</resource> failed.",
                                newName), d->prj);
        *success = false;
        return;
    }
    *success = true;
}

void KexiMainWindow::setObjectCaption(KexiPart::Item *item, const QString& _newCaption, bool *success)
{
    Q_ASSERT(success);
    if (d->userMode) {
        *success = false;
        return;
    }
    QString newCaption = _newCaption.trimmed();
    setMessagesEnabled(false); //to avoid double messages
    const bool res = d->prj->setObjectCaption(item, newCaption);
    setMessagesEnabled(true);
    if (!res) {
        showErrorMessage(xi18nc("@info",
                                "Setting caption for object <resource>%1</resource> failed.",
                                newCaption), d->prj);
        *success = false;
        return;
    }
    *success = true;
}

void KexiMainWindow::slotObjectRenamed(const KexiPart::Item &item, const QString& oldName)
{
    Q_UNUSED(oldName);
#ifndef KEXI_NO_PENDING_DIALOGS
    Private::PendingJobType pendingType;
    KexiWindow *window = d->openedWindowFor(&item, pendingType);
    if (pendingType != Private::NoJob)
        return;
#else
    KexiWindow *window = openedWindowFor(&item);
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
    if (d->propertyEditor()) {
        d->propertyEditor()->acceptInput();
    }
}

void KexiMainWindow::propertySetSwitched(KexiWindow *window, bool force,
        bool preservePrevSelection, bool sortedProperties, const QByteArray& propertyToSelect)
{
    KexiWindow* _currentWindow = currentWindow();
    //qDebug() << "currentWindow(): "
    //    << (_currentWindow ? _currentWindow->windowTitle() : QString("NULL"))
    //    << "window:" << (window ? window->windowTitle() : QString("NULL"));
    if (_currentWindow && _currentWindow != window) {
        d->propertySet = 0; //we'll need to move to another prop. set
        return;
    }
    if (d->propertyEditor()) {
        KPropertySet *newSet = _currentWindow ? _currentWindow->propertySet() : 0;
        if (!newSet || (force || static_cast<KPropertySet*>(d->propertySet) != newSet)) {
            d->propertySet = newSet;
            if (preservePrevSelection || force) {
                KPropertyEditorView::SetOptions options;
                if (preservePrevSelection) {
                    options |= KPropertyEditorView::SetOption::PreservePreviousSelection;
                }
                if (sortedProperties) {
                    options |= KPropertyEditorView::SetOption::AlphabeticalOrder;
                }

                d->objectViewWidget->propertyPane()->changePropertySet(
                            d->propertySet, propertyToSelect, options,
                            (_currentWindow && _currentWindow->selectedView())
                                ? _currentWindow->selectedView()->textToDisplayForNullSet()
                                : QString());
            }
        }
    }
}

void KexiMainWindow::slotDirtyFlagChanged(KexiWindow* window)
{
    KexiPart::Item *item = window->partItem();
    //update text in navigator and app. caption
    if (!d->userMode) {
        d->objectViewWidget->projectNavigator()->updateItemName(*item, window->isDirty());
    }

    invalidateActions();
    updateAppCaption();
    d->objectViewWidget->tabWidget()->setTabText(
        d->objectViewWidget->tabWidget()->indexOf(window->parentWidget()),
        window->windowTitle());
}

void KexiMainWindow::slotTipOfTheDay()
{
    //! @todo
}

void KexiMainWindow::slotReportBug()
{
    KexiBugReportDialog bugReport(this);
    bugReport.exec();
}

bool KexiMainWindow::userMode() const
{
    return d->userMode;
}

void
KexiMainWindow::setupUserActions()
{
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
        QMap<QString, QString> args;
        QDialog *dlg = KexiInternalPart::createModalDialogInstance("org.kexi-project.migration", "importtable", this, 0, &args);
        if (!dlg)
            return; //error msg has been shown by KexiInternalPart

        const int result = dlg->exec();
        delete dlg;
        if (result != QDialog::Accepted)
            return;

        QString destinationTableName(args["destinationTableName"]);
        if (!destinationTableName.isEmpty()) {
            QString pluginId = "org.kexi-project.table";
            bool openingCancelled;
            KexiMainWindow::openObject(pluginId, destinationTableName, Kexi::DataViewMode, &openingCancelled);
        }
    }
}

void KexiMainWindow::slotToolsCompactDatabase()
{
    KexiProjectData *data = 0;
    KDbDriver *drv = 0;
    const bool projectWasOpened = d->prj;

    if (!d->prj) {
        //! @todo Support compacting of non-opened projects
        return;
#if 0
        KexiStartupDialog dlg(
            KexiStartupDialog::OpenExisting, 0, Kexi::connset(), this);

        if (dlg.exec() != QDialog::Accepted)
            return;

        if (dlg.selectedFile().isEmpty()) {
//! @todo add support for server based if needed?
            return;
        }
        KDbConnectionData cdata;
        cdata.setDatabaseName(dlg.selectedFile());

        //detect driver name for the selected file
        KexiStartupData::Import detectedImportAction;
        QString detectedDriverId;
        tristate res = KexiStartupHandler::detectActionForFile(
                           &detectedImportAction, &detectedDriverId,
                           QString() /*suggestedDriverId*/, cdata.databaseName(), 0,
                           KexiStartupHandler::SkipMessages | KexiStartupHandler::ThisIsAProjectFile
                           | KexiStartupHandler::DontConvert);

        if (true == res && !detectedImportAction) {
            cdata.setDriverId(detectedDriverId);
            drv = Kexi::driverManager().driver(cdata.driverId());
        }
        if (!drv || !(drv->features() & KDbDriver::CompactingDatabaseSupported)) {
            KMessageBox::information(this,
                                     xi18n("Compacting database file <filename>%1</filename> is not supported.",
                                           QDir::toNativeSeparators(cdata.databaseName())));
            return;
        }
        data = new KexiProjectData(cdata);
#endif
    } else {
        //sanity
        if (!(d->prj && d->prj->dbConnection()
                && (d->prj->dbConnection()->driver()->features() & KDbDriver::CompactingDatabaseSupported)))
            return;

        KGuiItem yesItem(KStandardGuiItem::cont());
        yesItem.setText(xi18nc("@action:button Compact database", "Compact"));
        if (KMessageBox::PrimaryAction != KMessageBox::questionTwoActions(this,
                xi18n("The current project has to be closed before compacting the database. "
                     "It will be open again after compacting.\n\nDo you want to continue?"),
                QString(), yesItem, KStandardGuiItem::cancel()))
        {
            return;
        }
        data = new KexiProjectData(*d->prj->data()); // a copy
        drv = d->prj->dbConnection()->driver();
        const tristate res = closeProject();
        if (~res || !res) {
            delete data;
            return;
        }
    }

    if (!drv->adminTools().vacuum(*data->connectionData(), data->databaseName())) {
      showErrorMessage(QString(), &drv->adminTools());
    }

    if (projectWasOpened)
      openProject(*data);

    delete data;
}

tristate KexiMainWindow::showProjectMigrationWizard(const QString& mimeType, const QString& databaseName)
{
    return d->showProjectMigrationWizard(mimeType, databaseName, 0);
}

tristate KexiMainWindow::showProjectMigrationWizard(
    const QString& mimeType, const QString& databaseName, const KDbConnectionData &cdata)
{
    return d->showProjectMigrationWizard(mimeType, databaseName, &cdata);
}

tristate KexiMainWindow::executeItem(KexiPart::Item* item)
{
    KexiPart::Info *info = item ? Kexi::partManager().infoForPluginId(item->pluginId()) : 0;
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
                       "org.kexi-project.importexport.csv", "KexiCSVImportDialog", this, 0, &args);
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

    qWarning() << "no such action:" << actionName;
    return false;
}

tristate KexiMainWindow::exportItemAsDataTable(KexiPart::Item* item)
{
    if (!item)
        return false;

    QMap<QString, QString> args;

    if (!checkForDirtyFlagOnExport(item, &args)) {
            return false;
    }

    //! @todo: accept record changes...
    args.insert("destinationType", "file");
    args.insert("itemId", QString::number(item->identifier()));
    QDialog *dlg = KexiInternalPart::createModalDialogInstance(
                       "org.kexi-project.importexport.csv", "KexiCSVExportWizard", this, 0, &args);
    if (!dlg)
        return false; //error msg has been shown by KexiInternalPart
    int result = dlg->exec();
    delete dlg;
    return result == QDialog::Rejected ? tristate(cancelled) : tristate(true);
}

bool KexiMainWindow::checkForDirtyFlagOnExport(KexiPart::Item *item, QMap<QString, QString> *args)
{
    //! @todo: handle tables
    if (item->pluginId() != "org.kexi-project.query") {
        return true;
    }

    KexiWindow * itemWindow = openedWindowFor(item);
    if (itemWindow && itemWindow->isDirty()) {
        tristate result;
        if (item->neverSaved()) {
            result = true;
        } else {
            int prevWindowId = 0;
            if (!itemWindow->isVisible()) {
                prevWindowId = currentWindow()->id();
                activateWindow(itemWindow->id());
            }
            result = askOnExportingChangedQuery(item);

            if (prevWindowId != 0) {
                activateWindow(prevWindowId);
            }
        }

        if (~result) {
            return false;
        } else if (true == result) {
            args->insert("useTempQuery","1");
        }
    }

    return true;
}

tristate KexiMainWindow::askOnExportingChangedQuery(KexiPart::Item *item) const
{
    const KMessageBox::ButtonCode result = KMessageBox::warningTwoActionsCancel(const_cast<KexiMainWindow*>(this),
        xi18nc("@info", "Design of query <resource>%1</resource> that you want to export data"
                                         " from is changed and has not yet been saved. Do you want to use data"
                                         " from the changed query for exporting or from its original (saved)"
                                         " version?", item->captionOrName()),
        QString(),
        KGuiItem(xi18nc("@action:button Export query data", "Use the Changed Query")),
        KGuiItem(xi18nc("@action:button Export query data", "Use the Original Query")),
        KStandardGuiItem::cancel(),
        QString(),
        KMessageBox::Notify | KMessageBox::Dangerous);
    if (result == KMessageBox::PrimaryAction) {
        return true;
    } else if (result == KMessageBox::No) {
        return false;
    }

    return cancelled;
}

bool KexiMainWindow::printItem(KexiPart::Item* item, const QString& titleText)
{
    //! @todo printItem(item, KexiSimplePrintingSettings::load(), titleText);
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
//! @todo printPreviewForItem(item, KexiSimplePrintingSettings::load(), titleText, reload);
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
//! @todo check if changes to this object's design are saved, if not: ask for saving
//! @todo accept record changes...
//! @todo printActionForItem(item, PageSetupForItem);
    return false;
}

//! @todo reenable printItem() when ported
#if 0
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
    KexiPart::Info *info = Kexi::partManager().infoForPluginId(item->pluginId());
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
    KexiWindow *window = openedWindowFor(item);
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
            saveChanges.setToolTip(futureI18n("Save changes"));
            saveChanges.setWhatsThis(
                futureI18n("Pressing this button will save all recent changes made in \"%1\" object.",
                     item->name()));
            KGuiItem doNotSave(KStandardGuiItem::cancel());
            doNotSave.setWhatsThis(
                futureI18n("Pressing this button will ignore all unsaved changes made in \"%1\" object.",
                     window->partItem()->name()));

            QString question;
            if (action == PrintItem)
                question = futureI18n("Do you want to save changes before printing?");
            else if (action == PreviewItem)
                question = futureI18n("Do you want to save changes before making print preview?");
            else if (action == PageSetupForItem)
                question = futureI18n("Do you want to save changes before showing page setup?");
            else
                return false;

            const KMessageBox::ButtonCode questionRes = KMessageBox::warningTwoActionsCancel(this,
                                    "<p>"
                                    + window->part()->i18nMessage("Design of object <resource>%1</resource> has been modified.", window)
                                    .subs(item->name())
                                    + "</p><p>" + question + "</p>",
                                    QString(),
                                    saveChanges,
                                    doNotSave);
            if (KMessageBox::Cancel == questionRes)
                return cancelled;
            if (KMessageBox::PrimaryAction == questionRes) {
                tristate savingRes = saveObject(window, QString(), DoNotAsk);
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
                                &openingCancelled, &staticObjectArgs);
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
    KexiPart::Item* item = d->objectViewWidget->projectNavigator()->selectedPartItem();
    if (item)
        copyItemToClipboardAsDataTable(item);
}

tristate KexiMainWindow::copyItemToClipboardAsDataTable(KexiPart::Item* item)
{
    if (!item)
        return false;

    QMap<QString, QString> args;

    if (!checkForDirtyFlagOnExport(item, &args)) {
            return false;
    }

    args.insert("destinationType", "clipboard");
    args.insert("itemId", QString::number(item->identifier()));
    QDialog *dlg = KexiInternalPart::createModalDialogInstance(
                       "org.kexi-project.importexport.csv", "KexiCSVExportWizard", this, 0, &args);
    if (!dlg)
        return false; //error msg has been shown by KexiInternalPart
    const int result = dlg->exec();
    delete dlg;
    return result == QDialog::Rejected ? tristate(cancelled) : tristate(true);
}

void KexiMainWindow::slotEditPasteSpecialDataTable()
{
//! @todo allow data appending (it is not possible now)
    if (d->userMode)
        return;
    QMap<QString, QString> args;
    args.insert("sourceType", "clipboard");
    QDialog *dlg = KexiInternalPart::createModalDialogInstance(
                       "org.kexi-project.importexport.csv", "KexiCSVImportDialog", this, 0, &args);
    if (!dlg)
        return; //error msg has been shown by KexiInternalPart
    dlg->exec();
    delete dlg;
}

void KexiMainWindow::slotEditFind()
{
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

void KexiMainWindow::highlightObject(const QString& pluginId, const QString& name)
{
    if (!d->prj)
        return;
    KexiPart::Item *item = d->prj->itemForPluginId(pluginId, name);
    if (!item)
        return;
    if (d->objectViewWidget && d->objectViewWidget->projectNavigator()) {
        d->setProjectNavigatorVisible(true, Private::ShowAnimated);
        d->objectViewWidget->projectNavigator()->selectItem(*item);
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

void KexiMainWindow::updatePropertyEditorInfoLabel()
{
    if (d->objectViewWidget && d->objectViewWidget->propertyPane()) {
        d->objectViewWidget->propertyPane()->updateInfoLabelForPropertySet(
            (currentWindow() && currentWindow()->selectedView())
                    ? currentWindow()->selectedView()->textToDisplayForNullSet()
                    : QString());
    }
}

void KexiMainWindow::beginPropertyPaneUpdate()
{
    if (d->propertyPaneAnimation) {
        d->propertyPaneAnimation->hide();
        d->propertyPaneAnimation->deleteLater();
    }
    d->propertyPaneAnimation = new KexiFadeWidgetEffect(d->objectViewWidget->propertyPane());
}

void KexiMainWindow::endPropertyPaneUpdate()
{
    if (d->propertyPaneAnimation) {
        d->objectViewWidget->propertyPane()->repaint();
        d->propertyPaneAnimation->start(150);
    }
}

void KexiMainWindow::addSearchableModel(KexiSearchableModel *model)
{
    if (d->tabbedToolBar) {
        d->tabbedToolBar->addSearchableModel(model);
    }
}

void KexiMainWindow::removeSearchableModel(KexiSearchableModel *model)
{
    if (d->tabbedToolBar) {
        d->tabbedToolBar->removeSearchableModel(model);
    }
}

void KexiMainWindow::setReasonableDialogSize(QDialog *dialog)
{
    dialog->setMinimumSize(600, 400);
    dialog->resize(size() * 0.8);
}

void KexiMainWindow::restoreDesignTabAndActivateIfNeeded(const QString &tabName)
{
    if (!d->tabbedToolBar) {
        return;
    }
    d->tabbedToolBar->showTab(tabName);
    if (currentWindow() && currentWindow()->partItem()
        && currentWindow()->partItem()->identifier() != 0) // for unstored items id can be < 0
    {
        const QString tabToActivate = d->tabsToActivateOnShow.value(
                                          currentWindow()->partItem()->identifier());
        //qDebug() << "tabToActivate:" << tabToActivate << "tabName:" << tabName;
        if (tabToActivate == tabName) {
            d->tabbedToolBar->setCurrentTab(tabToActivate);
        }
    }
}

void KexiMainWindow::restoreDesignTabIfNeeded(const QString &pluginId, Kexi::ViewMode viewMode,
                                              int previousItemId)
{
    //qDebug() << pluginId << viewMode << previousItemId;
    if (viewMode == Kexi::DesignViewMode) {
        switch (d->prj->typeIdForPluginId(pluginId)) {
        case KexiPart::FormObjectType: {
            hideDesignTab(previousItemId, "org.kexi-project.report");
            restoreDesignTabAndActivateIfNeeded("form");
            break;
        }
        case KexiPart::ReportObjectType: {
            hideDesignTab(previousItemId, "org.kexi-project.form");
            restoreDesignTabAndActivateIfNeeded("report");
            break;
        }
        default:
            hideDesignTab(previousItemId);
        }
    }
    else {
        hideDesignTab(previousItemId);
    }
}

void KexiMainWindow::activateDesignTab(const QString &pluginId)
{
    if (!d->tabbedToolBar) {
        return;
    }
    switch (d->prj->typeIdForPluginId(pluginId)) {
    case KexiPart::FormObjectType:
        d->tabbedToolBar->setCurrentTab("form");
        break;
    case KexiPart::ReportObjectType:
        d->tabbedToolBar->setCurrentTab("report");
        break;
    default:;
    }
}

void KexiMainWindow::activateDesignTabIfNeeded(const QString &pluginId, Kexi::ViewMode viewMode)
{
    if (!d->tabbedToolBar) {
        return;
    }
    const QString tabToActivate = d->tabsToActivateOnShow.value(currentWindow()->partItem()->identifier());
    //qDebug() << pluginId << viewMode << tabToActivate;

    if (viewMode == Kexi::DesignViewMode && tabToActivate.isEmpty()) {
        activateDesignTab(pluginId);
    }
    else {
        d->tabbedToolBar->setCurrentTab(tabToActivate);
    }
}

void KexiMainWindow::hideDesignTab(int itemId, const QString &pluginId)
{
    if (!d->tabbedToolBar) {
        return;
    }
    //qDebug() << itemId << pluginId;
    if (   itemId > 0
        && d->tabbedToolBar->currentWidget())
    {
        const QString currentTab = d->tabbedToolBar->currentWidget()->objectName();
        //qDebug() << "d->tabsToActivateOnShow.insert" << itemId << currentTab;
        d->tabsToActivateOnShow.insert(itemId, currentTab);
    }
    switch (d->prj->typeIdForPluginId(pluginId)) {
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

void KexiMainWindow::showDesignTabIfNeeded(int previousItemId)
{
    if (d->insideCloseWindow && d->tabbedToolBar)
        return;
    if (currentWindow()) {
        restoreDesignTabIfNeeded(currentWindow()->partItem()->pluginId(),
                                 currentWindow()->currentViewMode(), previousItemId);
    } else {
        hideDesignTab(previousItemId);
    }
}

KexiUserFeedbackAgent* KexiMainWindow::userFeedbackAgent() const
{
    return &d->userFeedback;
}

KexiMigrateManagerInterface* KexiMainWindow::migrateManager()
{
    if (!d->migrateManager) {
        d->migrateManager = dynamic_cast<KexiMigrateManagerInterface*>(
                    KexiInternalPart::createObjectInstance(
                        "org.kexi-project.migration", "manager", this, this, nullptr));
    }
    return d->migrateManager;
}

void KexiMainWindow::toggleFullScreen(bool isFullScreen)
{
    if (d->tabbedToolBar) {
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
    }
    const Qt::WindowStates s = windowState() & Qt::WindowMaximized;
    if (isFullScreen) {
        setWindowState(windowState() | Qt::WindowFullScreen | s);
    } else {
        setWindowState((windowState() & ~Qt::WindowFullScreen));
        showMaximized();
    }
}

Kexi::GlobalViewMode KexiMainWindow::currentMode() const
{
    return d->modeSelector->currentMode();
}

void KexiMainWindow::setCurrentMode(Kexi::GlobalViewMode mode)
{
    d->modeSelector->setCurrentMode(mode);
}

void KexiMainWindow::slotCurrentModeChanged(Kexi::GlobalViewMode previousMode)
{
    const Kexi::ViewMode viewMode = currentWindow()
            ? currentWindow()->currentViewMode() : Kexi::NoViewMode;
    switch (d->modeSelector->currentMode()) {
    case Kexi::WelcomeGlobalMode:
        break;
    case Kexi::ProjectGlobalMode:
        break;
    case Kexi::EditGlobalMode:
        updateObjectView();
        d->globalViewStack->setCurrentWidget(d->objectViewWidget);
        if (viewMode == Kexi::DesignViewMode || viewMode == Kexi::TextViewMode) {
            if (true != switchToViewMode(*currentWindow(), Kexi::DataViewMode)) {
                setCurrentMode(previousMode);
            }
        }
        break;
    case Kexi::DesignGlobalMode:
        updateObjectView();
        d->globalViewStack->setCurrentWidget(d->objectViewWidget);
        if (viewMode == Kexi::DataViewMode) {
            Kexi::ViewMode newViewMode;
            if (currentWindow()->supportsViewMode(Kexi::TextViewMode)
                && d->modeSelector->keyboardModifiers() == Qt::CTRL)
            {
                newViewMode = Kexi::TextViewMode;
            } else {
                newViewMode = Kexi::DesignViewMode;
            }
            if (true != switchToViewMode(*currentWindow(), newViewMode)) {
                setCurrentMode(previousMode);
            }
        }
        break;
    case Kexi::HelpGlobalMode:
        break;
    case Kexi::NoGlobalMode:
        break;
    }
}

void KexiMainWindow::slotProjectNavigatorVisibilityChanged(bool visible)
{
    if (d->objectViewWidget && d->objectViewWidget->projectNavigator()) {
        d->modeSelector->setArrowColor(visible
            ? d->objectViewWidget->projectNavigator()->palette().color(QPalette::Window)
            : d->objectViewWidget->tabWidget()->palette().color(QPalette::Window));
    }
}
