/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2004-2012 Jarosław Staniek <staniek@kde.org>
   Copyright (C) 2005 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2005 Sebastian Sauer <mail@dipe.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include "kexiscriptdesignview.h"
#include "kexiscripteditor.h"
#include "kexiscriptadaptor.h"
#include "../kexidb/kexidbmodule.h"
#include "KexiScriptingDebug.h"
#include "kexiscriptpart.h"

#include <KexiIcon.h>
#include <KexiMainWindowIface.h>
#include <kexipart.h>

#include <KDbConnection>
#include <KPropertyListData>

#include <KActionMenu>
#include <KMessageBox>

#include <QSplitter>
#include <QTimer>
#include <QDomDocument>
#include <QTextBrowser>
#include <QFileDialog>
#include <QMenu>
#include <QJSEngine>
#include <QJSValue>
#include <QJSValueIterator>

static QString _stdout;

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type)
    if (QString(context.category) == "js") {
        _stdout += msg + "\n";
    } else {
        fprintf(stderr, "%s\n", msg.toLocal8Bit().constData());
        fflush(stderr);
    }
}

/// @internal
class KexiScriptDesignViewPrivate
{
public:

    QSplitter* splitter;
        
    /// The \a KexiScriptEditor to edit the scripting code.
    KexiScriptEditor* editor;

    /// The \a KPropertySet used in the propertyeditor.
    KPropertySet* properties;

    /// Boolean flag to avoid infinite recursion.
    bool updatesProperties;

    /// Used to display statusmessages.
    QTextBrowser* statusbrowser;

    /** The type of script
     *  executable = regular script that can be executed by the user
     *  module = a script which doesn't contain a 'main', only
     *           functions that can be used by other scripts
     *  object = a script which contains code to be loaded into another
                 object such as a report or form
     */
    QString scriptType;
    
    QString factoryConstructors;
};

KexiScriptDesignView::KexiScriptDesignView(
    QWidget *parent)
        : KexiView(parent)
        , d(new KexiScriptDesignViewPrivate())
{
    setObjectName("KexiScriptDesignView");

    d->updatesProperties = false;

    d->splitter = new QSplitter(this);
    d->splitter->setOrientation(Qt::Vertical);

    d->editor = new KexiScriptEditor(d->splitter);
    d->splitter->addWidget(d->editor);
    d->editor->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    d->splitter->setStretchFactor(d->splitter->indexOf(d->editor), 3);
    d->splitter->setFocusProxy(d->editor);
    //addChildView(d->editor);
    setViewWidget(d->splitter);

    d->statusbrowser = new QTextBrowser(d->splitter);
    d->splitter->addWidget(d->statusbrowser);
    d->splitter->setStretchFactor(d->splitter->indexOf(d->statusbrowser), 1);
    d->statusbrowser->setObjectName("ScriptStatusBrowser");
    d->statusbrowser->setReadOnly(true);
    //d->browser->setWordWrap(QTextEdit::WidgetWidth);
    d->statusbrowser->installEventFilter(this);

    /*
    plugSharedAction( "data_execute", this, SLOT(execute()) );
    if(KexiEditor::isAdvancedEditor()) // the configeditor is only in advanced mode avaiable.
        plugSharedAction( "script_config_editor", d->editor, SLOT(slotConfigureEditor()) );
    */

    // setup local actions
    QList<QAction*> viewActions;

    {
        QAction *a = new QAction(koIcon("system-run"), xi18n("Execute"), this);
        a->setObjectName("script_execute");
        a->setToolTip(xi18n("Execute the scripting code"));
        a->setWhatsThis(xi18n("Executes the scripting code."));
        connect(a, SIGNAL(triggered()), this, SLOT(execute()));
        viewActions << a;
    }

    QAction *a = new QAction(this);
    a->setSeparator(true);
    viewActions << a;

    KActionMenu *menu = new KActionMenu(koIcon("document-properties"), xi18n("Edit"), this);
    menu->setObjectName("script_edit_menu");
    menu->setToolTip(xi18n("Edit actions"));
    menu->setWhatsThis(xi18n("Provides Edit menu."));
    menu->setDelayed(false);
    foreach(QAction *a, d->editor->defaultContextMenu()->actions()) {
        menu->addAction(a);
    }
    if (KexiEditor::isAdvancedEditor()) { // the configeditor is only in advanced mode available.
        menu->addSeparator();
        QAction *a = new QAction(koIcon("configure"), xi18n("Configure Editor..."), this);
        a->setObjectName("script_config_editor");
        a->setToolTip(xi18n("Configure the scripting editor"));
        a->setWhatsThis(xi18n("Configures the scripting editor."));
        connect(a, SIGNAL(triggered()), d->editor, SLOT(slotConfigureEditor()));
        menu->addAction(a);
    }
    viewActions << menu;
    setViewActions(viewActions);

    // setup main menu actions
    QList<QAction*> mainMenuActions;
    a = new QAction(koIcon("document-import"), xi18n("&Import..."), this);
    a->setObjectName("script_import");
    a->setToolTip(xi18n("Import script"));
    a->setWhatsThis(xi18n("Imports script from a file."));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(slotImport()));
    mainMenuActions << a;

    a = new QAction(this);
    a->setSeparator(true);
    mainMenuActions << a;

    //a = new QAction(this);
    //a->setObjectName("project_saveas"); // placeholder for real?
    a = sharedAction("project_saveas");
    mainMenuActions << a;

    a = new QAction(koIcon("document-export"), xi18n("&Export..."), this);
    a->setObjectName("script_export");
    a->setToolTip(xi18n("Export script"));
    a->setWhatsThis(xi18n("Exports script to a file."));
    connect(a, SIGNAL(triggered(bool)), this, SLOT(slotExport()));
    mainMenuActions << a;

    setMainMenuActions(mainMenuActions);

    d->properties = new KPropertySet(this);
    connect(d->properties, SIGNAL(propertyChanged(KPropertySet&,KProperty&)),
            this, SLOT(slotPropertyChanged(KPropertySet&,KProperty&)));

    initialize("");
    loadData();
}

KexiScriptDesignView::~KexiScriptDesignView()
{
    delete d->properties;
    delete d;
}

#if 0
QString KexiScriptDesignView::scriptAction() const
{
    return d->scriptaction;
}
#endif

void KexiScriptDesignView::initialize(const QString &program)
{
    setDirty(false);
    updateProperties();
    d->editor->initialize(program);
    connect(d->editor, SIGNAL(textChanged()), this, SLOT(setDirty()));
    d->splitter->setSizes( QList<int>() << height() * 2 / 3 << height() * 1 / 3 );
}

void KexiScriptDesignView::slotImport()
{
    //QUrl("kfiledialog:///kexiscriptingdesigner"),
    const QUrl result = QFileDialog::getOpenFileUrl(this, xi18nc("@title:window", "Import Script"),
                                                    QUrl(), "Javascript (*.js)");
    if (!result.isValid()) {
        return;
    }
    //! @todo support remote files?
    QFile f(result.toLocalFile());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        KMessageBox::sorry(this,
            xi18nc("@info", "Could not read <filename>%1</filename>.", result.toLocalFile()));
        return;
    }
    d->editor->setText(f.readAll());
    f.close();
}

void KexiScriptDesignView::slotExport()
{
    const QUrl result = QFileDialog::getSaveFileUrl(this, xi18nc("@title:window", "Export Script"),
                                        QUrl("kfiledialog:///kexiscriptingdesigner"), "Javascript (*.js)" );
    if (!result.isValid())
        return;
    QFile f(result.toLocalFile());
    if (! f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        KMessageBox::sorry(this,
            xi18nc("@info", "Could not write <filename>%1</filename>.", result.toLocalFile()));
        return;
    }
    f.write(d->editor->text().toUtf8());
    f.close();
}

void KexiScriptDesignView::updateProperties()
{
    if (d->updatesProperties)
        return;
    d->updatesProperties = true;

    d->properties->clear();

    QStringList types;
    types << "executable" << "module" << "object";
    KPropertyListData* typelist = new KPropertyListData(types, types);
    KProperty* t = new KProperty(
        "type", // name
        typelist, // ListData
        (d->scriptType.isEmpty() ? "executable" : d->scriptType), // value
        xi18n("Script Type"), // caption
        xi18n("The type of script"), // description
        KProperty::List // type
    );
    d->properties->addProperty(t);

    //propertySetSwitched();
    propertySetReloaded(true);
    d->updatesProperties = false;
}

KPropertySet* KexiScriptDesignView::propertySet()
{
    return d->properties;
}

void KexiScriptDesignView::slotPropertyChanged(KPropertySet& /*set*/, KProperty& property)
{
    if (property.isNull())
        return;

    if (property.name() == "type") {
        d->scriptType = property.value().toString();
    }
    else {
        KexiScriptingWarning() << "unknown property:" << property.name();
        return;
    }

    setDirty(true);
}

void KexiScriptDesignView::execute()
{
    d->statusbrowser->clear();
    QTime time;
    time.start();
    d->statusbrowser->append(xi18nc("@info",
                                    "Execution of the script <resource>%1</resource> started.",
                                    part()->instanceName()));

    KexiScriptPart *pt = qobject_cast<KexiScriptPart*>(part());
    
    _stdout = QString();
    qInstallMessageHandler(myMessageOutput);
    if (pt) {
        QJSValue result = pt->execute(d->editor->text().toUtf8());
    
        d->statusbrowser->append(_stdout);
        if (result.isError()) {
            QString errormessage = result.toString();
            d->statusbrowser->append(QString("<b>%2</b><br>").arg(errormessage.toHtmlEscaped()));

            long lineno = result.property("lineNumber").toInt();
            if (lineno >= 0)
                d->editor->setLineNo(lineno);
        }
        else {
            // xgettext: no-c-format
            d->statusbrowser->append(xi18n("Successfully executed. Time elapsed: %1ms", time.elapsed()));
        }
    }
    qInstallMessageHandler(0);
}

bool KexiScriptDesignView::loadData()
{
    QString data;
    if (!loadDataBlock(&data)) {
        KexiScriptingDebug() << "no DataBlock";
        return false;
    }

    QString errMsg;
    int errLine;
    int errCol;

    QDomDocument domdoc;
    bool parsed = domdoc.setContent(data, false, &errMsg, &errLine, &errCol);

    if (! parsed) {
        KexiScriptingWarning() << "XML parsing error line: " << errLine << " col: " << errCol << " message: " << errMsg;
        return false;
    }

    QDomElement scriptelem = domdoc.namedItem("script").toElement();
    if (scriptelem.isNull()) {
        KexiScriptingWarning() << "script domelement is null";
        return false;
    }

    d->scriptType = scriptelem.attribute("scripttype");
    if (d->scriptType.isEmpty()) {
        d->scriptType = "executable";
    }

    initialize(scriptelem.text().toUtf8());

    return true;
}

KDbObject* KexiScriptDesignView::storeNewData(const KDbObject& object,
                                                       KexiView::StoreNewDataOptions options,
                                                       bool *cancel)
{
    KDbObject *s = KexiView::storeNewData(object, options, cancel);

    if (!s || *cancel) {
        delete s;
        return 0;
    }
    KexiScriptingDebug() << "new id:" << s->id();

    if (! storeData()) {
        qWarning() << "Failed to store the data.";
        //failure: remove object's object data to avoid garbage
        KDbConnection *conn = KexiMainWindowIface::global()->project()->dbConnection();
        conn->removeObject(s->id());
        delete s;
        return 0;
    }

    return s;
}

tristate KexiScriptDesignView::storeData(bool /*dontAsk*/)
{
    KexiScriptingDebug() << "Saving script" << d->editor->text().toUtf8();
    QDomDocument domdoc("script");
    QDomElement scriptelem = domdoc.createElement("script");
    domdoc.appendChild(scriptelem);

    scriptelem.setAttribute("language", "javascript");
    //! @todo move different types to their own part??
    scriptelem.setAttribute("scripttype", d->scriptType);

    QDomText scriptcode = domdoc.createTextNode(d->editor->text().toUtf8());
    scriptelem.appendChild(scriptcode);

    return storeDataBlock(domdoc.toString());
}
