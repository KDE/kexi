/* This file is part of the KDE project
   Copyright (C) 2003-2017 Jarosław Staniek <staniek@kde.org>

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
#include <core/kexipartmanager.h>
#include <core/kexipartinfo.h>
#include <core/KexiCommandLineOptions.h>
#include <kexiutils/utils.h>
#include <widget/KexiConnectionSelectorWidget.h>
#include <widget/KexiProjectSelectorWidget.h>
#include <widget/KexiDBPasswordDialog.h>
#include <kexidbconnectionwidget.h>
#include <kexidbshortcutfile.h>
#include <KexiIcon.h>

#include <KDbUtils>
#include <KDbDriver>
#include <KDbDriverManager>
#include <KDbDriverMetaData>

#include <KMessageBox>
#include <KLocalizedString>

#include <QDebug>
#include <QApplication>
#include <QMimeDatabase>
#include <QMimeType>
#include <QProgressDialog>

static void destroyStartupHandler()
{
    if (!KexiStartupData::global()) {
        return;
    }
    KexiStartupHandler* startupHandler = static_cast<KexiStartupHandler*>(KexiStartupHandler::global());
    delete startupHandler; // resets KexiStartup::global()
}

class KexiStartupData;
//---------------------------------

static bool stripQuotes(const QString &item, QString *name)
{
    if (item.left(1) == "\"" && item.right(1) == "\"") {
        *name = item.mid(1, item.length() - 2);
        return true;
    }
    *name = item;
    return false;
}

//! @internal
class Q_DECL_HIDDEN KexiStartupHandler::Private
{
public:
    Private(KexiStartupHandler *handler)
        : q(handler)
    {
    }

    ~Private() {
        destroyGui();
    }
    void destroyGui() {
        delete passwordDialog;
        passwordDialog = 0;
        delete connDialog;
        connDialog = 0;
    }

    bool findAutoopenObjects()
    {
        bool atLeastOneFound = false;
        KexiProjectData::AutoOpenObjects *objects
            = q->projectData() ? &q->projectData()->autoopenObjects : nullptr;
        for (const QCommandLineOption &option : q->options().autoopeningObjectsOptions()) {
            if (findAutoopenObjects(option, objects)) {
                atLeastOneFound = true;
            }
        }
        return atLeastOneFound;
    }

    bool findAutoopenObjects(const QCommandLineOption &option,
                             KexiProjectData::AutoOpenObjects *objects)
    {
        bool atLeastOneFound = false;
        for (const QString &value : q->values(option)) {
            QString typeName;
            QString objectName;
            int idx;
            bool nameRequired = true;
            if (option.names() == q->options().newObject.names()) {
                objectName.clear();
                stripQuotes(value, &typeName);
                nameRequired = false;
            } else {//open, design, text...
                QString defaultType;
                if (option.names() == q->options().execute.names()) {
#ifdef KEXI_MACROS_SUPPORT
                    defaultType = "macro";
#else
                    defaultType = "script";
#endif
                } else {
                    defaultType = "table";
                }

                //option with " " (set default type)
                if (stripQuotes(value, &objectName)) {
                    typeName = defaultType;
                } else if ((idx = value.indexOf(':')) != -1) {
                    //option with type name specified:
                    typeName = value.left(idx).toLower();
                    objectName = value.mid(idx + 1);
                    (void)stripQuotes(objectName, &objectName); // remove optional quotes
                } else {
                    //just obj. name: set default type name
                    objectName = value;
                    typeName = defaultType;
                }
            }
            if (typeName.isEmpty()) {
                continue;
            }
            if (nameRequired && objectName.isEmpty()) {
                continue;
            }
            atLeastOneFound = true;
            if (objects) {
                KexiProjectData::ObjectInfo info;
                info.insert("name", objectName);
                info.insert("type", typeName);
                info.insert("action", option.names().first());
                //ok, now add info for this object
                objects->append(info);
            } else {
                return true; //no need to find more because we do not have project anyway
            }
        } //for
        return atLeastOneFound;
    }

    KexiDBPasswordDialog* passwordDialog = nullptr;
    QString shortcutFileName;
    KexiDBConnShortcutFile *connShortcutFile = nullptr;
    KexiDBConnectionDialog *connDialog = nullptr;
    QString shortcutFileGroupKey;
private:
    KexiStartupHandler* const q;
};

//---------------------------------

void updateProgressBar(QProgressDialog *pd, char *buffer, int buflen)
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
            if (ok && percent >= 0 && percent <= 100 && pd->value() < percent) {
//    qDebug() << percent;
                pd->setValue(percent);
                qApp->processEvents(QEventLoop::AllEvents, 100);
            }
        }
    }
}


//---------------------------------
KexiStartupHandler::KexiStartupHandler()
        : QObject(0)
        , KexiStartupData()
        , d(new Private(this))
{
    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(slotAboutToAppQuit()));
}

KexiStartupHandler::~KexiStartupHandler()
{
    qRemovePostRoutine(destroyStartupHandler); // post routine is installed!
    delete d;
}

KexiStartupHandler* KexiStartupHandler::global()
{
    if (!KexiStartupData::global()) {
        (void)new KexiStartupHandler; // sets KexiStartup::global()
        qAddPostRoutine(destroyStartupHandler);
    }
    return static_cast<KexiStartupHandler*>(KexiStartupData::global());
}

void KexiStartupHandler::slotAboutToAppQuit()
{
    d->destroyGui();
}

void prettyPrintPluginMetaData(int maxWidth, const QStringList &labels, QTextStream *out,
                               const KPluginMetaData& metaData)
{
#define LABEL(n) labels[n] << QString(maxWidth - labels[n].length() + 1, ' ')
    *out << " * " << metaData.pluginId() << endl
         << "     " << LABEL(0) << metaData.name() << endl
         << "     " << LABEL(1) << metaData.description() << endl
         << "     " << LABEL(2) << metaData.version() << endl
         << "     " << LABEL(3) << metaData.fileName() << endl;
#undef LABEL
}

//! Nicely output a list of plugins
static void prettyPrintListOfPlugins()
{
    QTextStream out(stdout);

    QStringList labels;
    labels << i18nc("Plugin name", "Name:")
           << i18nc("Plugin description", "Description:")
           << i18nc("Plugin version", "Version:")
           << i18nc("Plugin fileName", "File:");
    int maxWidth = -1;
    foreach(const QString &label, labels) {
        maxWidth = qMax(maxWidth, label.length());
    }

    // 1. Kexi plugins
    if (Kexi::partManager().infoList()->isEmpty()) {
        out << i18n("No KEXI plugins found.") << endl;
    }
    else {
        out << i18n("KEXI plugins (%1):", Kexi::partManager().infoList()->count()) << endl;
        foreach(const KexiPart::Info *info, *Kexi::partManager().infoList()) {
            prettyPrintPluginMetaData(maxWidth, labels, &out, *info);
        }
    }

    // 2. KDb drivers
    KDbDriverManager driverManager;
    if (driverManager.driverIds().isEmpty()) {
        out << i18n("No KDb database driver plugins found.") << endl;
    }
    else {
        out << i18n("KDb database driver plugins (%1):", driverManager.driverIds().count()) << endl;
        foreach(const QString &pluginId, driverManager.driverIds()) {
            const KDbDriverMetaData *metaData = driverManager.driverMetaData(pluginId);
            if (metaData) {
                prettyPrintPluginMetaData(maxWidth, labels, &out, *metaData);
            }
        }
    }
}

tristate KexiStartupHandler::handleHighPriorityOptions()
{
    if (isSet(options().listPlugins)) {
        prettyPrintListOfPlugins();
        setAction(Exit);
        return true;
    }
    // option not found:
    return cancelled;
}

tristate KexiStartupHandler::init(const QStringList &arguments,
                                  const QList<QCommandLineOption> &extraOptions)
{
    setAction(DoNothing);

    tristate res = parseOptions(arguments, extraOptions);
    if (res != true) {
        setAction(Exit);
        return res;
    }

//    if (isSet(options().help)) {
//        helpText
//    }

    res = handleHighPriorityOptions();
    if (res == true || res == false) {
        setAction(Exit);
        return res;
    }

    // Other options
    KDbConnectionData cdata;

    if (isSet(options().connectionShortcut)) {
        KexiDBConnShortcutFile connectionShortcut(value(options().connectionShortcut));
        if (!connectionShortcut.loadConnectionData(&cdata)) {
//! @todo Show error message from KexiDBConnShortcutFile when there's one implemented.
//!       For we're displaying generic error msg.
            KMessageBox::sorry(0,
                               xi18nc("@info",
                                     "<para>Could not read connection information from connection shortcut "
                                     "file <filename>%1</filename>.</para>"
                                     "<para><note>Check whether the file has valid contents.</note></para>",
                                     QDir::toNativeSeparators(connectionShortcut.fileName())));
            return false;
        }
    }

    // Set to true if user explicitly sets conn data options from command line.
    // In this case display login dialog and skip the standard Welcome Wizard.
    bool connDataOptionsSpecified = false;

    if (isSet(options().dbDriver)) {
        cdata.setDriverId(value(options().dbDriver));
        connDataOptionsSpecified = true;
    }

    QString fileType;
    if (isSet(options().fileType)) {
        fileType = value(options().fileType);
    }
    if (!positionalArguments().isEmpty() && !fileType.isEmpty()) {
        if (fileType != "project" && fileType != "shortcut" && fileType != "connection") {
            KMessageBox::sorry(0,
                xi18nc("Please don't translate the \"type\" word, it's constant.",
                       "<icode>%1</icode> is not valid value for <icode>--type</icode> "
                       "command-line option. Possible value can be <icode>%2</icode>, "
                       "<icode>%3</icode> or <icode>%4</icode>",
                       fileType, "project", "shortcut", "connection"));
            return false;
        }
    }

    if (isSet(options().host)) {
        cdata.setHostName(value(options().host));
        connDataOptionsSpecified = true;
    }
    if (isSet(options().localSocket)) {
        cdata.setLocalSocketFileName(value(options().localSocket));
        connDataOptionsSpecified = true;
    }
    if (isSet(options().user)) {
        cdata.setUserName(value(options().user));
        connDataOptionsSpecified = true;
    }
    bool fileDriverSelected;
    if (cdata.driverId().isEmpty())
        fileDriverSelected = true;
    else {
        KDbDriverManager manager;
        const KDbDriverMetaData *driverMetaData = manager.driverMetaData(cdata.driverId());
        if (!driverMetaData) {
            //driver id provided explicitly, but not found
            KMessageBox::sorry(0, manager.result().message());
            return false;
        }
        fileDriverSelected = driverMetaData->isFileBased();
    }

    bool projectFileExists = false;

    if (isSet(options().port)) {
        bool ok;
        const int p = value(options().port).toInt(&ok);
        if (ok && p > 0) {
            cdata.setPort(p);
            connDataOptionsSpecified = true;
        }
        else {
            KMessageBox::sorry(0,
                xi18n("Invalid port number <icode>%1</icode> specified.", value(options().port)));
            return false;
        }
    }
    if (connDataOptionsSpecified && cdata.driverId().isEmpty()) {
        KMessageBox::sorry(0, xi18n("Could not open database. No database driver specified."));
        return false;
    }

    KexiStartupData::setForcedUserMode(isSet(options().userMode));
    KexiStartupData::setForcedDesignMode(isSet(options().designMode));
    KexiStartupData::setProjectNavigatorVisible(isSet(options().showNavigator));
    KexiStartupData::setMainMenuVisible(!isSet(options().hideMenu));
    KexiStartupData::setForcedFullScreen(isSet(options().fullScreen));
    bool createDB = isSet(options().createDb) || isSet(options().createAndOpenDb);
    const bool openExisting = !createDB && !isSet(options().dropDb);
    bool readOnly = isSet(options().readOnly);
    const QString couldnotMsg = QString::fromLatin1("\n")
        + xi18nc("@info", "Could not start <application>%1</application> application this way.",
                QApplication::applicationDisplayName());

    if (createDB && isSet(options().dropDb)) {
        KMessageBox::sorry(0,
            xi18nc("Please don't translate the \"createdb\" and \"dropdb\" words, these are constants.",
                   "Both <icode>createdb</icode> and <icode>dropdb</icode> used in startup options.") + couldnotMsg);
        return false;
    };

    if (createDB || isSet(options().dropDb)) {
        if (positionalArguments().isEmpty()) {
            KMessageBox::sorry(0, xi18n("No project name specified."));
            return false;
        }
        KexiStartupData::setAction(Exit);
    }

//! @todo add option for non-gui; integrate with KWallet; move to static KexiProject method
    if (!fileDriverSelected && !cdata.driverId().isEmpty() && cdata.password().isEmpty()) {

        if (cdata.password().isEmpty()) {
            delete d->passwordDialog;
            d->passwordDialog = new KexiDBPasswordDialog(0, cdata, KexiDBPasswordDialog::ShowDetailsButton);
            if (connDataOptionsSpecified) {
                if (cdata.userName().isEmpty()) {
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

    /* qDebug() << "ARGC==" << args->count();
      for (int i=0;i<args->count();i++) {
        qDebug() << "ARG" <<i<< "= " << args->arg(i);
      }*/

    if (KexiStartupData::forcedUserMode() && KexiStartupData::forcedDesignMode()) {
        KMessageBox::sorry(0, xi18nc("Please don't translate the <icode>user-mode</icode> and <icode>design-mode</icode> words, these are constants.",
                                    "Both <icode>user-mode</icode> and <icode>design-mode</icode> used in startup options.") + couldnotMsg);
        return false;
    }

    //database filenames, shortcut filenames or db names on a server
    if (!positionalArguments().isEmpty()) {
        QString prjName;
        QString fileName;
        if (fileDriverSelected) {
            fileName = positionalArguments().first();
        } else {
            prjName = positionalArguments().first();
        }

        if (fileDriverSelected) {
            QFileInfo finfo(fileName);
            prjName = finfo.fileName(); //filename only, to avoid messy names like when Kexi is started with "../../db" arg
            cdata.setDatabaseName(finfo.absoluteFilePath());
            projectFileExists = finfo.exists();

            if (isSet(options().dropDb) && !projectFileExists) {
                KMessageBox::sorry(0,
                                   xi18nc("@info",
                                          "Could not delete project. The file "
                                          "<filename>%1</filename> does not exist.",
                                          QDir::toNativeSeparators(cdata.databaseName())));
                return 0;
            }
        }

        if (createDB) {
            if (cdata.driverId().isEmpty())
                cdata.setDriverId(KDb::defaultFileBasedDriverId());
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

                if (isSet(options().dropDb))
                    detectOptions |= DontConvert;
                if (readOnly)
                    detectOptions |= OpenReadOnly;

                QString detectedDriverId;
                KexiStartupData::Import importData = KexiStartupData::importActionData();
                bool forceReadOnly;
                const tristate res = detectActionForFile(&importData, &detectedDriverId,
                                     cdata.driverId(), cdata.databaseName(), 0, detectOptions,
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
                cdata.setDriverId(detectedDriverId);
                if (cdata.driverId() == "shortcut") {
                    //get information for a shortcut file
                    KexiStartupData::setProjectData(new KexiProjectData());
                    d->shortcutFileName = cdata.databaseName();
                    if (!KexiStartupData::projectData()->load(d->shortcutFileName, &d->shortcutFileGroupKey)) {
                        KMessageBox::sorry(0, xi18nc("@info",
                                                     "Could not open shortcut file <filename>%1</filename>.",
                                                     QDir::toNativeSeparators(cdata.databaseName())));
                        delete KexiStartupData::projectData();
                        KexiStartupData::setProjectData(0);
                        return false;
                    }
                    if (KexiStartupData::projectData()->databaseName().isEmpty()) {
                        d->connDialog = new KexiDBConnectionDialog(0,
                                                                   *KexiStartupData::projectData(), d->shortcutFileName);
                        connect(d->connDialog, SIGNAL(saveChanges()),
                                this, SLOT(slotSaveShortcutFileChanges()));
                        int dialogRes = d->connDialog->exec();
                        if (dialogRes == QDialog::Accepted) {
                            //get (possibly changed) prj data
                            KexiStartupData::setProjectData(
                                new KexiProjectData(d->connDialog->currentProjectData()));
                        }

                        delete d->connDialog;
                        d->connDialog = 0;

                        if (dialogRes == QDialog::Rejected) {
                            delete KexiStartupData::projectData();
                            KexiStartupData::setProjectData(0);
                            return cancelled;
                        }
                    }
                } else if (cdata.driverId() == "connection") {
                    //get information for a connection file
                    d->connShortcutFile = new KexiDBConnShortcutFile(cdata.databaseName());
                    if (!d->connShortcutFile->loadConnectionData(&cdata, &d->shortcutFileGroupKey)) {
                        KMessageBox::sorry(0, xi18nc("@info",
                                                     "Could not open connection data file <filename>%1</filename>.",
                                                     QDir::toNativeSeparators(cdata.databaseName())));
                        delete d->connShortcutFile;
                        d->connShortcutFile = 0;
                        return false;
                    }
                    bool cancel = false;
                    while (true) {
                        if (isSet(options().skipConnDialog)) {
                            //show connection dialog, so user can change parameters
                            if (!d->connDialog) {
                                d->connDialog = new KexiDBConnectionDialog(0,
                                        cdata, d->connShortcutFile->fileName());
                                connect(d->connDialog, SIGNAL(saveChanges()),
                                        this, SLOT(slotSaveShortcutFileChanges()));
                            }
                            const int dialogRes = d->connDialog->exec();
                            if (dialogRes == QDialog::Accepted) {
                                //get (possibly changed) prj data
                                cdata = *d->connDialog->currentProjectData().connectionData();
                            } else {
                                cancel = true;
                                break;
                            }
                        }
                        KexiStartupData::setProjectData(selectProject(&cdata, &cancel));
                        if (KexiStartupData::projectData() || cancel || !isSet(options().skipConnDialog))
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
    if (positionalArguments().count() > 1) {
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
        int dialogRes = d->connDialog->exec();

        if (dialogRes == QDialog::Accepted) {
            //get (possibly changed) prj data
            KexiStartupData::setProjectData(new KexiProjectData(d->connDialog->currentProjectData()));
        }

        delete d->connDialog;
        d->connDialog = 0;

        if (dialogRes == QDialog::Rejected) {
            delete KexiStartupData::projectData();
            KexiStartupData::setProjectData(0);
            return cancelled;
        }
    }

    if (positionalArguments().isEmpty() && connDataOptionsSpecified) {
        bool cancel = false;
        KexiStartupData::setProjectData(selectProject(&cdata, &cancel));
        if (!KexiStartupData::projectData() || cancel) {
            KexiStartupData::setProjectData(0);
            return false;
        }
    }

    //---autoopen objects:
    const bool atLeastOneAOOFound = d->findAutoopenObjects();

    if (atLeastOneAOOFound && !openExisting) {
        KMessageBox::information(0,
                                 xi18n("You have specified a few database objects to be opened automatically, "
                                       "using startup options.\n"
                                       "These options will be ignored because they are not available while creating "
                                       "or dropping projects."));
    }

    if (createDB) {
        bool creationCancelled;
        KexiGUIMessageHandler gui;
        KexiProject *prj = KexiProject::createBlankProject(&creationCancelled, *projectData(), &gui);
        bool ok = prj != 0;
        delete prj;
        if (creationCancelled)
            return cancelled;
        if (!isSet(options().createAndOpenDb)) {
            if (ok) {
                KMessageBox::information(0, xi18nc("@info",
                                                   "Project <resource>%1</resource> created successfully.",
                                                   QDir::toNativeSeparators(projectData()->databaseName())));
            }
            return ok;
        }
    } else if (isSet(options().dropDb)) {
        KexiGUIMessageHandler gui;
        res = KexiProject::dropProject(*projectData(), &gui, false/*ask*/);
        if (res == true)
            KMessageBox::information(0, xi18nc("@info", "Project <resource>%1</resource> dropped successfully.",
                                             QDir::toNativeSeparators(projectData()->databaseName())));
        return res != false;
    }

    //------

    KexiPart::PartInfoList *partInfoList = Kexi::partManager().infoList();
    if (!partInfoList || partInfoList->isEmpty()) {
        KexiGUIMessageHandler msgh;
        msgh.showErrorMessage(Kexi::partManager().result());
        KexiStartupData::setProjectData(0);
        return false;
    }

    if (!KexiStartupData::projectData()) {
        cdata = KDbConnectionData(); //clear

        KexiStartupData::setAction(ShowWelcomeScreen);
        return true;
//! @todo remove startup dialog code
#if 0
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
        }
#ifdef KEXI_PROJECT_TEMPLATES
        else if (r == KexiStartupDialog::CreateFromTemplateResult) {
            const QString selFile(d->startupDialog->selectedFile());
            cdata.setDatabaseName(selFile);
            QString detectedDriverId;
            KexiStartupData::Import importData = KexiStartupData::importActionData();
            res = detectActionForFile(&importData, &detectedDriverId,
                                 cdata.driverId(), selFile);
            if (true != res)
                return res;
            KexiStartupData::setImportActionData(importData);
            if (KexiStartupData::importActionData() || detectedDriverId.isEmpty())
                return false;
            cdata.setDriverId(detectedDriverId);
            KexiStartupData::setProjectData(new KexiProjectData(cdata, selFile));
            KexiStartupData::projectData()->autoopenObjects = d->startupDialog->autoopenObjects();
            KexiStartupData::setAction(CreateFromTemplate);
            return true;
        }
#endif
        else if (r == KexiStartupDialog::OpenExistingResult) {
            const QString selectedFile(d->startupDialog->selectedFile());
            if (!selectedFile.isEmpty()) {
                //file-based project
                cdata.setDatabaseName(selectedFile);
                QString detectedDriverId;
                KexiStartupData::Import importData = KexiStartupData::importActionData();
                res = detectActionForFile(&importData, &detectedDriverId,
                                     cdata.driverId(), selectedFile);
                if (true != res)
                    return res;
                KexiStartupData::setImportActionData(importData);
                if (KexiStartupData::importActionData()) { //importing action
                    KexiStartupData::setAction(ImportProject);
                    return true;
                }

                if (detectedDriverId.isEmpty())
                    return false;
                cdata.setDriverId(detectedDriverId);
                KexiStartupData::setProjectData(new KexiProjectData(cdata, selectedFile));
            } else if (d->startupDialog->selectedExistingConnection()) {
                KDbConnectionData *cdata = d->startupDialog->selectedExistingConnection();
                //ok, now we will try to show projects for this connection to the user
                bool cancelled;
                KexiStartupData::setProjectData(selectProject(cdata, &cancelled));
                if ((!KexiStartupData::projectData() && !cancelled) || cancelled) {
                    //try again
                    return init();
                }
                //not needed anymore
                delete d->startupDialog;
                d->startupDialog = 0;
            }
        }

        if (!KexiStartupData::projectData())
            return true;
#endif
    }

    if (KexiStartupData::projectData() && (openExisting || (createDB && isSet(options().createAndOpenDb)))) {
        KexiStartupData::projectData()->setReadOnly(readOnly);
        KexiStartupData::setAction(OpenProject);
    }
    return true;
}

tristate KexiStartupHandler::detectActionForFile(
        KexiStartupData::Import* detectedImportAction, QString *detectedDriverId,
        const QString& _suggestedDriverId, const QString &databaseName, QWidget *parent,
        int options, bool *forceReadOnly)
{
    Q_ASSERT(detectedDriverId);
    *detectedImportAction = KexiStartupData::Import(); //clear
    if (forceReadOnly) {
        *forceReadOnly = false;
    }
    QString suggestedDriverId(_suggestedDriverId); //safe
    detectedDriverId->clear();
    QFileInfo finfo(databaseName);
    if (databaseName.isEmpty()) {
        if (!(options & SkipMessages)) {
            KMessageBox::sorry(parent, xi18nc("@info", "Could not open file. Missing filename."),
                               xi18nc("@title:window", "Could Not Open File"));
        }
        return false;
    }
    if (!finfo.exists()) {
        if (!(options & SkipMessages)) {
            KMessageBox::sorry(parent, xi18nc("@info", "Could not open file. "
                                             "The file <filename>%1</filename> does not exist.",
                                             QDir::toNativeSeparators(databaseName)),
                                       xi18nc("@title:window", "Could Not Open File" ));
        }
        return false;
    }
    if (!finfo.isReadable()) {
        if (!(options & SkipMessages)) {
            KMessageBox::sorry(parent, xi18nc("@info",
                                             "<para>Could not open file <filename>%1</filename> for reading.</para>"
                                             "<para><note>Check the file's permissions and whether it is "
                                             "already opened and locked by another application.</note></para>",
                                             QDir::toNativeSeparators(databaseName)),
                                       xi18nc("@title:window", "Could Not Open File" ));
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

    QMimeType mime;
    QString mimename;

    const bool thisIsShortcut = (options & ThisIsAShortcutToAProjectFile)
                                || (options & ThisIsAShortcutToAConnectionData);

    if ((options & ThisIsAProjectFile) || !thisIsShortcut) {
        //try this detection if "project file" mode is forced or no type is forced:
        QMimeDatabase db;
        mime = db.mimeTypeForFile(databaseName, QMimeDatabase::MatchContent);
        if (mime.isValid()) {
            mimename = mime.name();
        }
        //qDebug() << "found mime is:" << mimename;
        if (mimename.isEmpty() || mimename == "application/octet-stream" || mimename == "text/plain") {
            //try by URL:
            mime = db.mimeTypeForUrl(QUrl::fromLocalFile(databaseName));
            mimename = mime.name();
        }
    }
    if (mimename.isEmpty() || mimename == "application/octet-stream") {
        // perhaps the file is locked
        QFile f(databaseName);
        if (!f.open(QIODevice::ReadOnly)) {
            // BTW: similar error msg is provided in SQLiteConnection::drv_useDatabase()
            if (!(options & SkipMessages)) {
                KMessageBox::sorry(parent, xi18nc("@info", "<para>Could not open project.</para>"
                                   "<para>The file <filename>%1</filename> is not readable. "
                                   "Check the file's permissions and whether it is already opened "
                                   "and locked by another application.</para>",
                                   QDir::toNativeSeparators(databaseName)));
            }
            return false;
        }
    }
    if ((options & ThisIsAShortcutToAProjectFile) || mimename == "application/x-kexiproject-shortcut") {
        *detectedDriverId = "shortcut";
        return true;
    }

    if ((options & ThisIsAShortcutToAConnectionData) || mimename == "application/x-kexi-connectiondata") {
        *detectedDriverId = "connection";
        return true;
    }

    //! @todo rather check this use migration drivers' X-KexiSupportedMimeTypes [strlist] property
    if (mime.isValid()) {
        if (mimename == "application/vnd.ms-access") {
            if ((options & SkipMessages) || KMessageBox::Yes != KMessageBox::questionYesNo(
                parent, xi18nc("@info",
                               "<para><filename>%1</filename> is an external file of type <resource>%2</resource>.</para>"
                               "<para>Do you want to import the file as a KEXI project?</para>",
                               QDir::toNativeSeparators(databaseName), mime.comment()),
                               xi18n("Open External File"),
                               KGuiItem(xi18nc("@action:button Import File", "Import..."),
                                        KexiIconName("database-import")),
                               KStandardGuiItem::cancel()))
            {
                return cancelled;
            }
            detectedImportAction->mimeType = mimename;
            detectedImportAction->fileName = databaseName;
            return true;
        }
    }

    if (!finfo.isWritable()) {
        //! @todo if file is ro: change project mode (but do not care if we're jsut importing)
    }

    // "application/x-kexiproject-sqlite", etc.:
    const QStringList driverIdsForMimeType = Kexi::driverManager().driverIdsForMimeType(mimename);
    QString compatibleDatabaseDriverId;
    if (!driverIdsForMimeType.isEmpty()) {
        //! @todo if there are more drivers to pick, ask which to use
        compatibleDatabaseDriverId = driverIdsForMimeType.first();
    }
    bool useDetectedDriver = suggestedDriverId.isEmpty() || suggestedDriverId.compare(*detectedDriverId, Qt::CaseInsensitive) == 0;
    if (!useDetectedDriver) {
        if (compatibleDatabaseDriverId.isEmpty()) {
            //! @todo error message "Could not detect database driver to use"
            return false;
        }
        KMessageBox::ButtonCode res = KMessageBox::Yes;
        if (!(options & SkipMessages))
            res = KMessageBox::warningYesNoCancel(parent, xi18nc("@info",
                  "The project file <filename>%1</filename> is recognized as compatible with "
                  "<resource>%2</resource> database driver, "
                  "while you have asked for <resource>%3</resource> database driver to be used.\n"
                  "Do you want to use <resource>%4</resource> database driver?",
                  QDir::toNativeSeparators(databaseName),
                  compatibleDatabaseDriverId, suggestedDriverId, compatibleDatabaseDriverId));
        if (KMessageBox::Yes == res)
            useDetectedDriver = true;
        else if (KMessageBox::Cancel == res)
            return cancelled;
    }
    if (useDetectedDriver) {
        *detectedDriverId = compatibleDatabaseDriverId;
    } else { //use the suggested driver
        *detectedDriverId = suggestedDriverId;
    }
// qDebug() << "driver id:" << detectedDriverName;

    if (detectedDriverId->isEmpty()) {
        QString possibleProblemsMessage(Kexi::driverManager().possibleProblemsMessage());
        if (!possibleProblemsMessage.isEmpty()) {
            possibleProblemsMessage = xi18n("Possible problems: %1", possibleProblemsMessage);
        }
        if (!(options & SkipMessages)) {
            const QString comment(mime.comment().isEmpty() ? QString() : QString::fromLatin1(" (%1)").arg(mime.comment()));
            KMessageBox::detailedSorry(parent,
               xi18nc("@info",
                      "The file <filename>%1</filename> is not recognized as being supported by "
                      "<application>%2</application>.",
                      QDir::toNativeSeparators(databaseName), QApplication::applicationDisplayName()),
                      possibleProblemsMessage.isEmpty()
                       ? xi18nc("@info",
                                "<para>Could not find plugin supporting for this file type.</para>"
                                "<para>Detected MIME type is <resource>%1</resource>%2.</para>",
                                mimename, comment)
                       : xi18nc("@info",
                                "<para>Could not find plugin supporting for this file type.</para>"
                                "<para>Detected MIME type is <resource>%1</resource>%2.</para><para>%3</para>",
                                mimename, comment, possibleProblemsMessage));
        }
        return false;
    }
    return true;
}

KexiProjectData*
KexiStartupHandler::selectProject(KDbConnectionData *cdata, bool *cancelled, QWidget *parent)
{
    clearStatus();
    *cancelled = false;
    if (!cdata)
        return 0;
    if (!cdata->savePassword() && cdata->password().isEmpty()) {
        if (!d->passwordDialog)
            d->passwordDialog = new KexiDBPasswordDialog(0, *cdata);
        const int ret = d->passwordDialog->exec();
        if (d->passwordDialog->showConnectionDetailsRequested() || ret == QDialog::Accepted) {

        } else {
            *cancelled = true;
            return 0;
        }
    }
    KexiProjectData* projectData = 0;
    //dialog for selecting a project
    KexiProjectSelectorDialog prjdlg(parent, *cdata, true, false);
    if (!prjdlg.projectSet() || prjdlg.projectSet()->result().isError()) {
        KexiGUIMessageHandler msgh;
        QString msg(xi18n("Could not load list of available projects for <resource>%1</resource> database server.",
                         cdata->toUserVisibleString()));
        if (prjdlg.projectSet()) {
            msgh.showErrorMessage(msg, prjdlg.projectSet());
        }
        else {
            msgh.showErrorMessage(msg, QString());
        }
        return 0;
    }
    if (prjdlg.exec() != QDialog::Accepted) {
        *cancelled = true;
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
        KMessageBox::sorry(0, xi18n("Failed saving connection data to <filename>%1</filename> file.",
                           QDir::toNativeSeparators(fileName)));
    }
}
