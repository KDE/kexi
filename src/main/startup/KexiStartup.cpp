/* This file is part of the KDE project
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

#include "KexiStartup.h"

#include "kexi.h"
#include "kexiproject.h"
#include "kexiprojectdata.h"
#include "kexiprojectset.h"
#include "kexiguimsghandler.h"
#include "KexiStartupDialog.h"

#include <db/utils.h>
#include <db/driver.h>
#include <db/drivermanager.h>
#include <core/kexipartmanager.h>
#include <widget/KexiConnectionSelectorWidget.h>
#include <widget/KexiProjectSelectorWidget.h>
#include <kexidbconnectionwidget.h>
#include <kexidbshortcutfile.h>

#include <KoIcon.h>

#include <kdebug.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kmessagebox.h>
#include <kcmdlineargs.h>

#include <ktextedit.h>
#include <kuser.h>
#include <KProgressDialog>

#include <unistd.h>

#include <QApplication>
#include <QLayout>

//! @todo enable this when we need sqlite3-to-someting-newer migration
// #define KEXI_SQLITE_MIGRATION

#ifdef KEXI_SQLITE_MIGRATION
# include "KexiStartup_p.h"
#endif

namespace Kexi
{
K_GLOBAL_STATIC(KexiStartupHandler, _startupHandler)

KexiStartupHandler& startupHandler()
{
    return *_startupHandler;
}
}

class KexiStartupData;
//---------------------------------

//! @internal
class KexiStartupHandler::Private
{
public:
    Private()
            : passwordDialog(0)//, showConnectionDetailsExecuted(false)
            , connShortcutFile(0), connDialog(0), startupDialog(0) {
    }

    ~Private() {
        destroyGui();
    }
    void destroyGui() {
        delete passwordDialog;
        passwordDialog = 0;
        delete connDialog;
        connDialog = 0;
        delete startupDialog;
        startupDialog = 0;
    }

    KexiDBPasswordDialog* passwordDialog;
    QString shortcutFileName;
    KexiDBConnShortcutFile *connShortcutFile;
    KexiDBConnectionDialog *connDialog;
    QString shortcutFileGroupKey;
    KexiStartupDialog *startupDialog;
};

//---------------------------------

static bool stripQuotes(const QString &item, QString &name)
{
    if (item.left(1) == "\"" && item.right(1) == "\"") {
        name = item.mid(1, item.length() - 2);
        return true;
    }
    name = item;
    return false;
}

void updateProgressBar(KProgressDialog *pd, char *buffer, int buflen)
{
    char *p = buffer;
    QByteArray line;
    line.reserve(80);
    for (int i = 0; i < buflen; i++, p++) {
        if ((i == 0 || buffer[i-1] == '\n') && buffer[i] == '%') {
            bool ok;
            int j = 0;
            ++i;
            line.clear();
            for (;i<buflen && *p >= '0' && *p <= '9'; j++, i++, p++)
                line += *p;
            --i; --p;
            const int percent = line.toInt(&ok);
            if (ok && percent >= 0 && percent <= 100 && pd->progressBar()->value() < percent) {
//    kDebug() << percent;
                pd->progressBar()->setValue(percent);
                qApp->processEvents(QEventLoop::AllEvents, 100);
            }
        }
    }
}

//---------------------------------

class KexiDBPasswordDialog::Private
{
 public:
    Private(KexiDB::ConnectionData* data);
    ~Private();

    KexiDB::ConnectionData *cdata;
    bool showConnectionDetailsRequested;
};

KexiDBPasswordDialog::Private::Private(KexiDB::ConnectionData* data)
    : cdata(data)
    , showConnectionDetailsRequested(false)
{
}

KexiDBPasswordDialog::Private::~Private()
{

}
#include <kexiutils/utils.h>

KexiDBPasswordDialog::KexiDBPasswordDialog(QWidget *parent, KexiDB::ConnectionData& cdata, bool showDetailsButton)
        : KPasswordDialog(parent, ShowUsernameLine | ShowDomainLine,
                          showDetailsButton ? KDialog::User1 : KDialog::None)
        , d(new Private(&cdata))
{
    setCaption(i18nc("@title:window", "Opening Database"));
    setPrompt(i18nc("@info", "Supply a password below."));

    QString srv = cdata.serverInfoString(false);
    QLabel *domainLabel = KexiUtils::findFirstChild<QLabel*>(this, "QLabel", "domainLabel");
    if (domainLabel) {
        domainLabel->setText(i18n("Database server:"));
    }
    setDomain(srv);

    QString usr;
    if (cdata.userName.isEmpty())
        usr = i18nc("unspecified user", "(unspecified)");
    else
        usr = cdata.userName;
    setUsernameReadOnly(true);
    setUsername(usr);

    if (showDetailsButton) {
        connect(this, SIGNAL(user1Clicked()),
                this, SLOT(slotShowConnectionDetails()));
        setButtonText(KDialog::User1, i18n("&Details") + " >>");
    }
    setButtonText(KDialog::Ok, i18n("&Open"));
    setButtonIcon(KDialog::Ok, koIcon("document-open"));
}

KexiDBPasswordDialog::~KexiDBPasswordDialog()
{
    delete d;
}

bool KexiDBPasswordDialog::showConnectionDetailsRequested() const
{
    return d->showConnectionDetailsRequested;
}

void KexiDBPasswordDialog::slotButtonClicked(int button)
{
    if (button == KDialog::Ok || button == KDialog::User1) {
        d->cdata->password = password();
        QLineEdit *userEdit = KexiUtils::findFirstChild<QLineEdit*>(this, "QLineEdit", "userEdit");
        if (!userEdit->isReadOnly()) {
            d->cdata->userName = userEdit->text();
        }
    }
    KPasswordDialog::slotButtonClicked(button);
}

void KexiDBPasswordDialog::slotShowConnectionDetails()
{
    d->showConnectionDetailsRequested = true;
    close();
}

//static
bool KexiDBPasswordDialog::getPasswordIfNeeded(KexiDB::ConnectionData *data, QWidget *parent)
{
    if (data->passwordNeeded() && data->password.isNull() /* null means missing password */) {
        //ask for password
        KexiDBPasswordDialog pwdDlg(parent, *data, false /*!showDetailsButton*/);
        if (QDialog::Accepted != pwdDlg.exec()) {
            return false;
        }
    }
    return true;
}


//---------------------------------
KexiStartupHandler::KexiStartupHandler()
        : QObject(0)
        , KexiStartupData()
        , d(new Private())
{
    // K_GLOBAL_STATIC is cleaned up *after* QApplication is gone
    // but we have to cleanup before -> use qAddPostRoutine
    qAddPostRoutine(Kexi::_startupHandler.destroy);

    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(slotAboutToAppQuit()));
}

KexiStartupHandler::~KexiStartupHandler()
{
    qRemovePostRoutine(Kexi::_startupHandler.destroy); // post routine is installed!
    delete d;
}

void KexiStartupHandler::slotAboutToAppQuit()
{
    d->destroyGui();
}

bool KexiStartupHandler::getAutoopenObjects(KCmdLineArgs *args, const QByteArray &action_name)
{
    QStringList list = args->getOptionList(action_name);
    bool atLeastOneFound = false;
    foreach(const QString &option, list) {
        QString type_name, obj_name, item = option;
        int idx;
        bool name_required = true;
        if (action_name == "new") {
            obj_name.clear();
            stripQuotes(item, type_name);
            name_required = false;
        } else {//open, design, text...
            QString defaultType;
            if (action_name == "execute")
                defaultType = "macro";
            else
                defaultType = "table";

            //option with " " (set default type)
            if (stripQuotes(item, obj_name)) {
                type_name = defaultType;
            } else if ((idx = item.indexOf(':')) != -1) {
                //option with type name specified:
                type_name = item.left(idx).toLower();
                obj_name = item.mid(idx + 1);
                //optional: remove ""
                if (obj_name.left(1) == "\"" && obj_name.right(1) == "\"")
                    obj_name = obj_name.mid(1, obj_name.length() - 2);
            } else {
                //just obj. name: set default type name
                obj_name = item;
                type_name = defaultType;
            }
        }
        if (type_name.isEmpty())
            continue;
        if (name_required && obj_name.isEmpty())
            continue;

        atLeastOneFound = true;
        if (projectData()) {
            KexiProjectData::ObjectInfo* info = new KexiProjectData::ObjectInfo();
            info->insert("name", obj_name);
            info->insert("type", type_name);
            info->insert("action", action_name);
            //ok, now add info for this object
            projectData()->autoopenObjects.append(info);
        } else
            return true; //no need to find more because we do not have projectData() anyway
    } //for
    return atLeastOneFound;
}

tristate KexiStartupHandler::init(int /*argc*/, char ** /*argv*/)
{
    setAction(DoNothing);
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs(0);
    if (!args)
        return true;

    KexiDB::ConnectionData cdata;

    const QString connectionShortcutFileName(args->getOption("connection"));
    if (!connectionShortcutFileName.isEmpty()) {
        KexiDBConnShortcutFile connectionShortcut(connectionShortcutFileName);
        if (!connectionShortcut.loadConnectionData(cdata)) {
//! @todo Show error message from KexiDBConnShortcutFile when there's one implemented.
//!       For we're displaying generic error msg.
            KMessageBox::sorry(0,
                               i18nc("@info",
                                     "Could not read connection information from connection shortcut "
                                     "file <filename>%1</filename>."
                                     "<note>Check whether the file has valid contents.</note>",
                                     QDir::convertSeparators(connectionShortcut.fileName())));
            return false;
        }
    }

    // Set to true if user explicitly sets conn data options from command line.
    // In this case display login dialog and skip the standard Welcome Wizard.
    bool connDataOptionsSpecified = false;

    if (!args->getOption("dbdriver").isEmpty()) {
        cdata.driverName = args->getOption("dbdriver");
        connDataOptionsSpecified = true;
    }

    QString fileType(args->getOption("type").toLower());
    if (args->count() > 0 && (!fileType.isEmpty() && fileType != "project" && fileType != "shortcut" && fileType != "connection")) {
        KMessageBox::sorry(0,
                           i18nc("Please don't translate the \"type\" word, it's constant.",
                                 "Invalid argument <icode>%1</icode> specified for <icode>type</icode> command-line option.",
                                fileType));
        return false;
    }

    if (!args->getOption("host").isEmpty()) {
        cdata.hostName = args->getOption("host");
        connDataOptionsSpecified = true;
    }
    if (!args->getOption("local-socket").isEmpty()) {
        cdata.localSocketFileName = args->getOption("local-socket");
        connDataOptionsSpecified = true;
    }
    if (!args->getOption("user").isEmpty()) {
        cdata.userName = args->getOption("user");
        connDataOptionsSpecified = true;
    }
    bool fileDriverSelected;
    if (cdata.driverName.isEmpty())
        fileDriverSelected = true;
    else {
        KexiDB::DriverManager dm;
        KexiDB::Driver::Info dinfo = dm.driverInfo(cdata.driverName);
        if (dinfo.name.isEmpty()) {
            //driver name provided explicitly, but not found
            KMessageBox::sorry(0, dm.errorMsg());
            return false;
        }
        fileDriverSelected = dinfo.fileBased;
    }

    bool projectFileExists = false;
    const QString portStr = args->getOption("port");

    if (!portStr.isEmpty()) {
        bool ok;
        const int p = portStr.toInt(&ok);
        if (ok && p > 0) {
            cdata.port = p;
            connDataOptionsSpecified = true;
        }
        else {
            KMessageBox::sorry(0,
                               i18n("Invalid port number <icode>%1</icode> specified.", portStr));
            return false;
        }
    }
    if (connDataOptionsSpecified && cdata.driverName.isEmpty()) {
        KMessageBox::sorry(0, i18n("Could not open database. No database driver specified."));
        return false;
    }

    KexiStartupData::setForcedUserMode(args->isSet("user-mode"));
    KexiStartupData::setForcedDesignMode(args->isSet("design-mode"));
    KexiStartupData::setProjectNavigatorVisible(args->isSet("show-navigator"));
    KexiStartupData::setMainMenuVisible(!args->isSet("hide-menu"));
    KexiStartupData::setForcedFullScreen(args->isSet("fullscreen"));
    bool createDB = args->isSet("createdb");
    const bool alsoOpenDB = args->isSet("create-opendb");
    if (alsoOpenDB)
        createDB = true;
    const bool dropDB = args->isSet("dropdb");
    const bool openExisting = !createDB && !dropDB;
    bool readOnly = args->isSet("readonly");
    const QString couldnotMsg = QString::fromLatin1("\n")
                                + i18n("Could not start Kexi application this way.");

    if (createDB && dropDB) {
        KMessageBox::sorry(0, i18nc("Please don't translate the \"createdb\" and \"dropdb\" words, these are constants.",
                                    "Both <icode>createdb</icode> and <icode>dropdb</icode> used in startup options.") + couldnotMsg);
        return false;
    };

    if (createDB || dropDB) {
        if (args->count() < 1) {
            KMessageBox::sorry(0, i18n("No project name specified."));
            return false;
        }
        KexiStartupData::setAction(Exit);
    }

//! @todo add option for non-gui; integrate with KWallet; move to static KexiProject method
    if (!fileDriverSelected && !cdata.driverName.isEmpty() && cdata.password.isEmpty()) {

        if (cdata.password.isEmpty()) {
            delete d->passwordDialog;
            d->passwordDialog = new KexiDBPasswordDialog(0, cdata, true);
            if (connDataOptionsSpecified) {
                if (cdata.userName.isEmpty()) {
                    d->passwordDialog->setUsername(QString());
                    d->passwordDialog->setUsernameReadOnly(false);
                    QLineEdit *userEdit = KexiUtils::findFirstChild<QLineEdit*>(d->passwordDialog, "QLineEdit", "userEdit");
                    if (userEdit) {
                        userEdit->setFocus();
                    }
                }
            }
            const int ret = d->passwordDialog->exec();
            if (d->passwordDialog->showConnectionDetailsRequested() || ret == QDialog::Accepted) {
            }
            else {
                KexiStartupData::setAction(Exit);
                return true;
            }
        }
    }

    /* kDebug() << "ARGC==" << args->count();
      for (int i=0;i<args->count();i++) {
        kDebug() << "ARG" <<i<< "= " << args->arg(i);
      }*/

    if (KexiStartupData::forcedUserMode() && KexiStartupData::forcedDesignMode()) {
        KMessageBox::sorry(0, i18nc("Please don't translate the <icode>user-mode</icode> and <icode>design-mode</icode> words, these are constants.",
                                    "Both <icode>user-mode</icode> and <icode>design-mode</icode> used in startup options.") + couldnotMsg);
        return false;
    }

    //database filenames, shortcut filenames or db names on a server
    if (args->count() >= 1) {
        QString prjName;
        QString fileName;
        if (fileDriverSelected) {
            fileName = args->arg(0);
        } else {
            prjName = args->arg(0);
        }

        if (fileDriverSelected) {
            QFileInfo finfo(fileName);
            prjName = finfo.fileName(); //filename only, to avoid messy names like when Kexi is started with "../../db" arg
            cdata.setFileName(finfo.absoluteFilePath());
            projectFileExists = finfo.exists();

            if (dropDB && !projectFileExists) {
                KMessageBox::sorry(0,
                                   i18n("Could not remove project.\nThe file \"%1\" does not exist.",
                                        QDir::convertSeparators(cdata.dbFileName())));
                return 0;
            }
        }

        if (createDB) {
            if (cdata.driverName.isEmpty())
                cdata.driverName = KexiDB::defaultFileBasedDriverName();
            KexiStartupData::setProjectData(new KexiProjectData(cdata, prjName)); //dummy
        } else {
            if (fileDriverSelected) {
                int detectOptions = 0;
                if (fileType == "project")
                    detectOptions |= ThisIsAProjectFile;
                else if (fileType == "shortcut")
                    detectOptions |= ThisIsAShortcutToAProjectFile;
                else if (fileType == "connection")
                    detectOptions |= ThisIsAShortcutToAConnectionData;

                if (dropDB)
                    detectOptions |= DontConvert;
                if (readOnly)
                    detectOptions |= OpenReadOnly;

                QString detectedDriverName;
                KexiStartupData::Import importData = KexiStartupData::importActionData();
                bool forceReadOnly;
                const tristate res = detectActionForFile(&importData, &detectedDriverName,
                                     cdata.driverName, cdata.fileName(), 0, detectOptions,
                                     &forceReadOnly);
                if (true != res)
                    return res;
                if (forceReadOnly) {
                    readOnly = true;
                }
                KexiStartupData::setImportActionData(importData);
                if (KexiStartupData::importActionData()) { //importing action
                    KexiStartupData::setAction(ImportProject);
                    return true;
                }

                //opening action
                cdata.driverName = detectedDriverName;
                if (cdata.driverName == "shortcut") {
                    //get information for a shortcut file
                    KexiStartupData::setProjectData(new KexiProjectData());
                    d->shortcutFileName = cdata.fileName();
                    if (!KexiStartupData::projectData()->load(d->shortcutFileName, &d->shortcutFileGroupKey)) {
                        KMessageBox::sorry(0, i18n("Could not open shortcut file\n\"%1\".",
                                                   QDir::convertSeparators(cdata.fileName())));
                        delete KexiStartupData::projectData();
                        KexiStartupData::setProjectData(0);
                        return false;
                    }
                    if (KexiStartupData::projectData()->databaseName().isEmpty()) {
                        d->connDialog = new KexiDBConnectionDialog(0,
                                                                   *KexiStartupData::projectData(), d->shortcutFileName);
                        connect(d->connDialog, SIGNAL(saveChanges()),
                                this, SLOT(slotSaveShortcutFileChanges()));
                        int res = d->connDialog->exec();
                        if (res == QDialog::Accepted) {
                            //get (possibly changed) prj data
                            KexiStartupData::setProjectData(
                                new KexiProjectData(d->connDialog->currentProjectData()));
                        }

                        delete d->connDialog;
                        d->connDialog = 0;

                        if (res == QDialog::Rejected) {
                            delete KexiStartupData::projectData();
                            KexiStartupData::setProjectData(0);
                            return cancelled;
                        }
                    }
                } else if (cdata.driverName == "connection") {
                    //get information for a connection file
                    d->connShortcutFile = new KexiDBConnShortcutFile(cdata.fileName());
                    if (!d->connShortcutFile->loadConnectionData(cdata, &d->shortcutFileGroupKey)) {
                        KMessageBox::sorry(0, i18n("Could not open connection data file\n\"%1\".",
                                                   QDir::convertSeparators(cdata.fileName())));
                        delete d->connShortcutFile;
                        d->connShortcutFile = 0;
                        return false;
                    }
                    bool cancel = false;
                    const bool showConnectionDialog = !args->isSet("skip-conn-dialog");
                    while (true) {
                        if (showConnectionDialog) {
                            //show connection dialog, so user can change parameters
                            if (!d->connDialog) {
                                d->connDialog = new KexiDBConnectionDialog(0,
                                        cdata, d->connShortcutFile->fileName());
                                connect(d->connDialog, SIGNAL(saveChanges()),
                                        this, SLOT(slotSaveShortcutFileChanges()));
                            }
                            const int res = d->connDialog->exec();
                            if (res == QDialog::Accepted) {
                                //get (possibly changed) prj data
                                cdata = *d->connDialog->currentProjectData().constConnectionData();
                            } else {
                                cancel = true;
                                break;
                            }
                        }
                        KexiStartupData::setProjectData(selectProject(&cdata, cancel));
                        if (KexiStartupData::projectData() || cancel || !showConnectionDialog)
                            break;
                    }

                    delete d->connShortcutFile;
                    d->connShortcutFile = 0;
                    delete d->connDialog;
                    d->connDialog = 0;

                    if (cancel)
                        return cancelled;
                }
                else { // !shortcut && !connection
                    KexiStartupData::setProjectData(new KexiProjectData(cdata, prjName));
                }
            }
            else { // !fileDriverSelected
                KexiStartupData::setProjectData(new KexiProjectData(cdata, prjName));
            }

        }
    }
    if (args->count() > 1) {
        //! @todo KRun another Kexi instance
    }

    //let's show connection details, user asked for that in the "password dialog"
    if (d->passwordDialog && d->passwordDialog->showConnectionDetailsRequested()) {
        if (KexiStartupData::projectData()) {
            d->connDialog = new KexiDBConnectionDialog(0, *KexiStartupData::projectData());
        }
        else {
            d->connDialog = new KexiDBConnectionDialog(0, cdata);
        }
        int res = d->connDialog->exec();

        if (res == QDialog::Accepted) {
            //get (possibly changed) prj data
            KexiStartupData::setProjectData(new KexiProjectData(d->connDialog->currentProjectData()));
        }

        delete d->connDialog;
        d->connDialog = 0;

        if (res == QDialog::Rejected) {
            delete KexiStartupData::projectData();
            KexiStartupData::setProjectData(0);
            return cancelled;
        }
    }

    if (args->count() < 1 && connDataOptionsSpecified) {
        bool cancel = false;
        KexiStartupData::setProjectData(selectProject(&cdata, cancel));
        if (!KexiStartupData::projectData() || cancel) {
            KexiStartupData::setProjectData(0);
            return false;
        }
    }

    //---autoopen objects:
    const bool atLeastOneAOOFound = getAutoopenObjects(args, "open")
                                    || getAutoopenObjects(args, "design")
                                    || getAutoopenObjects(args, "edittext")
                                    || getAutoopenObjects(args, "execute")
                                    || getAutoopenObjects(args, "new")
                                    || getAutoopenObjects(args, "print")
                                    || getAutoopenObjects(args, "print-preview");

    if (atLeastOneAOOFound && !openExisting) {
        KMessageBox::information(0,
                                 i18n("You have specified a few database objects to be opened automatically, "
                                      "using startup options.\n"
                                      "These options will be ignored because they are not available while creating "
                                      "or dropping projects."));
    }

    if (createDB) {
        bool creationNancelled;
        KexiGUIMessageHandler gui;
        KexiProject *prj = KexiProject::createBlankProject(creationNancelled, *projectData(), &gui);
        bool ok = prj != 0;
        delete prj;
        if (creationNancelled)
            return cancelled;
        if (!alsoOpenDB) {
            if (ok) {
                KMessageBox::information(0, i18n("Project \"%1\" created successfully.",
                                                 QDir::convertSeparators(projectData()->databaseName())));
            }
            return ok;
        }
    } else if (dropDB) {
        KexiGUIMessageHandler gui;
        tristate res = KexiProject::dropProject(*projectData(), &gui, false/*ask*/);
        if (res == true)
            KMessageBox::information(0, i18n("Project \"%1\" dropped successfully.",
                                             QDir::convertSeparators(projectData()->databaseName())));
        return res != false;
    }

    //------

    KexiPart::PartInfoList *partInfoList = Kexi::partManager().infoList();
    if (!partInfoList || partInfoList->isEmpty()) {
        KexiGUIMessageHandler msgh;
        msgh.showErrorMessage(&Kexi::partManager());
        KexiStartupData::setProjectData(0);
        return false;
    }

    if (!KexiStartupData::projectData()) {
        cdata = KexiDB::ConnectionData(); //clear

        KexiStartupData::setAction(ShowWelcomeScreen);
        return true;
//! @todo remove startup dialog code
        if (args->isSet("skip-startup-dialog") || !KexiStartupDialog::shouldBeShown())
            return true;

        if (!d->startupDialog) {
            //create startup dialog for reuse because it can be used again after conn err.
            d->startupDialog = new KexiStartupDialog(
                KexiStartupDialog::Everything, KexiStartupDialog::CheckBoxDoNotShowAgain,
                Kexi::connset(), 0);
        }
        if (d->startupDialog->exec() != QDialog::Accepted)
            return true;

        const int r = d->startupDialog->result();
        if (r == KexiStartupDialog::CreateBlankResult) {
            KexiStartupData::setAction(CreateBlankProject);
            return true;
        } else if (r == KexiStartupDialog::ImportResult) {
            KexiStartupData::setAction(ImportProject);
            return true;
        } else if (r == KexiStartupDialog::CreateFromTemplateResult) {
            const QString selFile(d->startupDialog->selectedFileName());
            cdata.setFileName(selFile);
            QString detectedDriverName;
            KexiStartupData::Import importData = KexiStartupData::importActionData();
            const tristate res = detectActionForFile(&importData, &detectedDriverName,
                                 cdata.driverName, selFile);
            if (true != res)
                return res;
            KexiStartupData::setImportActionData(importData);
            if (KexiStartupData::importActionData() || detectedDriverName.isEmpty())
                return false;
            cdata.driverName = detectedDriverName;
            KexiStartupData::setProjectData(new KexiProjectData(cdata, selFile));
#ifdef KEXI_PROJECT_TEMPLATES
            KexiStartupData::projectData()->autoopenObjects = d->startupDialog->autoopenObjects();
#endif
            KexiStartupData::setAction(CreateFromTemplate);
            return true;
        } else if (r == KexiStartupDialog::OpenExistingResult) {
            const QString selFile(d->startupDialog->selectedFileName());
            if (!selFile.isEmpty()) {
                //file-based project
                cdata.setFileName(selFile);
                QString detectedDriverName;
                KexiStartupData::Import importData = KexiStartupData::importActionData();
                const tristate res = detectActionForFile(&importData, &detectedDriverName,
                                     cdata.driverName, selFile);
                if (true != res)
                    return res;
                KexiStartupData::setImportActionData(importData);
                if (KexiStartupData::importActionData()) { //importing action
                    KexiStartupData::setAction(ImportProject);
                    return true;
                }

                if (detectedDriverName.isEmpty())
                    return false;
                cdata.driverName = detectedDriverName;
                KexiStartupData::setProjectData(new KexiProjectData(cdata, selFile));
            } else if (d->startupDialog->selectedExistingConnection()) {
                KexiDB::ConnectionData *cdata = d->startupDialog->selectedExistingConnection();
                //ok, now we will try to show projects for this connection to the user
                bool cancelled;
                KexiStartupData::setProjectData(selectProject(cdata, cancelled));
                if ((!KexiStartupData::projectData() && !cancelled) || cancelled) {
                    //try again
                    return init(0, 0);
                }
                //not needed anymore
                delete d->startupDialog;
                d->startupDialog = 0;
            }
        }

        if (!KexiStartupData::projectData())
            return true;
    }

    if (KexiStartupData::projectData() && (openExisting || (createDB && alsoOpenDB))) {
        KexiStartupData::projectData()->setReadOnly(readOnly);
        KexiStartupData::setAction(OpenProject);
    }
    return true;
}

tristate KexiStartupHandler::detectActionForFile(
        KexiStartupData::Import* detectedImportAction, QString *detectedDriverName,
        const QString& _suggestedDriverName, const QString &dbFileName, QWidget *parent,
        int options, bool *forceReadOnly)
{
    *detectedImportAction = KexiStartupData::Import(); //clear
    if (forceReadOnly) {
        *forceReadOnly = false;
    }
    QString suggestedDriverName(_suggestedDriverName); //safe
    detectedDriverName->clear();
    QFileInfo finfo(dbFileName);
    if (dbFileName.isEmpty()) {
        if (!(options & SkipMessages)) {
            KMessageBox::sorry(parent, i18nc("@info", "Could not open file. Missing filename."),
                               i18nc("@title:window", "Could Not Open File"));
        }
        return false;
    }
    if (!finfo.exists()) {
        if (!(options & SkipMessages)) {
            KMessageBox::sorry(parent, i18nc("@info", "Could not open file. "
                                             "The file <filename>%1</filename> does not exist.",
                                             QDir::convertSeparators(dbFileName)),
                                       i18nc("@title:window", "Could Not Open File" ));
        }
        return false;
    }
    if (!finfo.isReadable()) {
        if (!(options & SkipMessages)) {
            KMessageBox::sorry(parent, i18nc("@info",
                                             "Could not open file <filename>%1</filename> for reading. "
                                             "<note>Check the file's permissions and whether it is "
                                             "already opened and locked by another application.</note>",
                                             QDir::convertSeparators(dbFileName)),
                                       i18nc("@title:window", "Could Not Open File" ));
        }
        return false;
    }
    if (!(options & OpenReadOnly) && !finfo.isWritable()) {
        if (!KexiProject::askForOpeningNonWritableFileAsReadOnly(parent, finfo)) {
            return false;
        }
        if (forceReadOnly) {
            *forceReadOnly = true;
        }
    }

    KMimeType::Ptr ptr;
    QString mimename;

    const bool thisIsShortcut = (options & ThisIsAShortcutToAProjectFile)
                                || (options & ThisIsAShortcutToAConnectionData);

    if ((options & ThisIsAProjectFile) || !thisIsShortcut) {
        //try this detection if "project file" mode is forced or no type is forced:
        ptr = KMimeType::findByFileContent(dbFileName);
        mimename = ptr.data() ? ptr.data()->name() : QString();
        kDebug() << "found mime is:" << mimename;
        if (mimename.isEmpty() || mimename == "application/octet-stream" || mimename == "text/plain") {
            //try by URL:
            ptr = KMimeType::findByUrl(KUrl::fromPath(dbFileName));
            mimename = ptr.data()->name();
        }
    }
    if (mimename.isEmpty() || mimename == "application/octet-stream") {
        // perhaps the file is locked
        QFile f(dbFileName);
        if (!f.open(QIODevice::ReadOnly)) {
            // BTW: similar error msg is provided in SQLiteConnection::drv_useDatabase()
            if (!(options & SkipMessages))
                KMessageBox::sorry(parent, i18n("<p>Could not open project.</p>")
                                   + i18n("<p>The file <nobr>\"%1\"</nobr> is not readable.</p>",
                                          QDir::convertSeparators(dbFileName))
                                   + i18n("Check the file's permissions and whether it is already opened "
                                          "and locked by another application."));
            return false;
        }
    }
    if ((options & ThisIsAShortcutToAProjectFile) || mimename == "application/x-kexiproject-shortcut") {
        *detectedDriverName = "shortcut";
        return true;
    }

    if ((options & ThisIsAShortcutToAConnectionData) || mimename == "application/x-kexi-connectiondata") {
        *detectedDriverName = "connection";
        return true;
    }

    //! @todo rather check this using migration drivers'
    //! X-KexiSupportedMimeTypes [strlist] property
    if (ptr.data()) {
        if (mimename == "application/vnd.ms-access") {
            if ((options & SkipMessages) || KMessageBox::Yes != KMessageBox::questionYesNo(
                        parent, i18n("\"%1\" is an external file of type:\n\"%2\".\n"
                                     "Do you want to import the file as a Kexi project?",
                                     QDir::convertSeparators(dbFileName), ptr.data()->comment()),
                        i18n("Open External File"), KGuiItem(i18n("Import...")), KStandardGuiItem::cancel())) {
                return cancelled;
            }
            detectedImportAction->mimeType = mimename;
            detectedImportAction->fileName = dbFileName;
            return true;
        }
    }

    if (!finfo.isWritable()) {
        //! @todo if file is ro: change project mode (but do not care if we're jsut importing)
    }

    // "application/x-kexiproject-sqlite", etc.:
    QString tmpDriverName = Kexi::driverManager().lookupByMime(mimename).toLatin1();
//! @todo What about trying to reuse CALLIGRA FILTER CHAINS here?
    bool useDetectedDriver = suggestedDriverName.isEmpty() || suggestedDriverName.toLower() == detectedDriverName->toLower();
    if (!useDetectedDriver) {
        int res = KMessageBox::Yes;
        if (!(options & SkipMessages))
            res = KMessageBox::warningYesNoCancel(parent, i18n(
                                                      "The project file \"%1\" is recognized as compatible with \"%2\" database driver, "
                                                      "while you have asked for \"%3\" database driver to be used.\n"
                                                      "Do you want to use \"%4\" database driver?",
                                                      QDir::convertSeparators(dbFileName),
                                                      tmpDriverName, suggestedDriverName, tmpDriverName));
        if (KMessageBox::Yes == res)
            useDetectedDriver = true;
        else if (KMessageBox::Cancel == res)
            return cancelled;
    }
    if (useDetectedDriver) {
        *detectedDriverName = tmpDriverName;
    } else {//use suggested driver
        *detectedDriverName = suggestedDriverName;
    }
// kDebug() << "driver name:" << detectedDriverName;
//hardcoded for convenience:
    const QString newFileFormat = "SQLite3";

#ifdef KEXI_SQLITE_MIGRATION
    if (!(options & DontConvert || options & SkipMessages)
            && detectedDriverName.toLower() == "sqlite2" && detectedDriverName.toLower() != suggestedDriverName.toLower()
            && KMessageBox::Yes == KMessageBox::questionYesNo(parent, i18n(
                        "Previous version of database file format (\"%1\") is detected in the \"%2\" "
                        "project file.\nDo you want to convert the project to a new \"%3\" format (recommended)?",
                        detectedDriverName, QDir::convertSeparators(dbFileName), newFileFormat))) {
        SQLite2ToSQLite3Migration migr(finfo.absoluteFilePath());
        tristate res = migr.run();
//  kDebug() << "--- migr.run() END ---";
        if (!res) {
            KMessageBox::sorry(parent, i18n(
                                   "Failed to convert project file \"%1\" to a new \"%2\" format.\n"
                                   "The file format remains unchanged.",
                                   QDir::convertSeparators(dbFileName), newFileFormat));
            //continue...
        }
        if (res == true)
            detectedDriverName = newFileFormat;
    }
#endif
    if (detectedDriverName->isEmpty()) {
        QString possibleProblemsInfoMsg(Kexi::driverManager().possibleProblemsInfoMsg());
        if (!possibleProblemsInfoMsg.isEmpty()) {
            possibleProblemsInfoMsg.prepend(QString::fromLatin1("<p>") + i18n("Possible problems:"));
            possibleProblemsInfoMsg += QString::fromLatin1("</p>");
        }
        if (!(options & SkipMessages)) {
            KMessageBox::detailedSorry(parent,
                                       i18n("The file \"%1\" is not recognized as being supported by Kexi.",
                                            QDir::convertSeparators(dbFileName)),
                                       QString::fromLatin1("<p>")
                                       + i18n("Database driver for this file type not found.\nDetected MIME type is %1.",
                                              mimename)
                                       + (ptr.data()->comment().isEmpty()
                                          ? QString::fromLatin1(".") : QString::fromLatin1(" (%1).").arg(ptr.data()->comment()))
                                       + QString::fromLatin1("</p>")
                                       + possibleProblemsInfoMsg);
        }
        return false;
    }
    return true;
}

KexiProjectData*
KexiStartupHandler::selectProject(KexiDB::ConnectionData *cdata, bool& cancelled, QWidget *parent)
{
    clearStatus();
    cancelled = false;
    if (!cdata)
        return 0;
    if (!cdata->savePassword && cdata->password.isEmpty()) {
        if (!d->passwordDialog)
            d->passwordDialog = new KexiDBPasswordDialog(0, *cdata, false);
        const int ret = d->passwordDialog->exec();
        if (d->passwordDialog->showConnectionDetailsRequested() || ret == QDialog::Accepted) {

        } else {
            cancelled = true;
            return 0;
        }
    }
    KexiProjectData* projectData = 0;
    //dialog for selecting a project
    KexiProjectSelectorDialog prjdlg(parent, *cdata, true, false);
    if (!prjdlg.projectSet() || prjdlg.projectSet()->error()) {
        KexiGUIMessageHandler msgh;
        QString msg(i18n("Could not load list of available projects for <resource>%1</resource> database server.",
                         cdata->serverInfoString(true)));
        if (prjdlg.projectSet()) {
            msgh.showErrorMessage(prjdlg.projectSet(), msg);
        }
        else {
            msgh.showErrorMessage(msg);
        }
        return 0;
    }
    if (prjdlg.exec() != QDialog::Accepted) {
        cancelled = true;
        return 0;
    }
    if (prjdlg.selectedProjectData()) {
        //deep copy
        projectData = new KexiProjectData(*prjdlg.selectedProjectData());
    }
    return projectData;
}

void KexiStartupHandler::slotSaveShortcutFileChanges()
{
    bool ok = true;
    QString fileName;
    if (!d->shortcutFileName.isEmpty()) {
        fileName = d->shortcutFileName;
        ok = d->connDialog->currentProjectData().save(
            d->shortcutFileName,
            d->connDialog->savePasswordOptionSelected(),
            &d->shortcutFileGroupKey);
    }
    else if (d->connShortcutFile) {
        fileName = d->connShortcutFile->fileName();
        ok = d->connShortcutFile->saveConnectionData(
                 *d->connDialog->currentProjectData().connectionData(),
                 d->connDialog->savePasswordOptionSelected(),
                 &d->shortcutFileGroupKey);
    }

    if (!ok) {
        KMessageBox::sorry(0, i18n("Failed saving connection data to <filename>%1</filename> file.",
                           QDir::convertSeparators(fileName)));
    }
}

#include "KexiStartup.moc"
