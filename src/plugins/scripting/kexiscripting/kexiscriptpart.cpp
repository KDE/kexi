/* This file is part of the KDE project
   Copyright (C) 2004 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2005 Sebastian Sauer <mail@dipe.org>

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

#include "kexiscriptpart.h"
#include "kexiscriptdesignview.h"
#include "kexiscriptadaptor.h"
#include "../kexidb/kexidbmodule.h"
#include "KexiScriptingDebug.h"

#include <kexipart.h>
#include <kexipartitem.h>
#include <KexiIcon.h>
#include <KexiView.h>
#include <KexiWindow.h>
#include <KexiMainWindowIface.h>
#include <kexiproject.h>
#include <KDbConnection>

#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KMessageBox>

#include <QDebug>
#include <QJSEngine>
#include <QJSValue>
#include <QJSValueIterator>
#include <QDomDocument>

KEXI_PLUGIN_FACTORY(KexiScriptPart, "kexi_scriptplugin.json")

/// \internal
class Q_DECL_HIDDEN KexiScriptPart::Private
{
public:
    explicit Private(KexiScriptPart* p)
            : p(p) {}
    ~Private() {
    }

    QJSEngine engine;
    KexiScriptPart* p;
    KexiScriptAdaptor adaptor;
    Scripting::KexiDBModule kexidbmodule;
};

KexiScriptPart::KexiScriptPart(QObject *parent, const QVariantList& l)
  : KexiPart::Part(parent,
        xi18nc("Translate this word using only lowercase alphanumeric characters (a..z, 0..9). "
              "Use '_' character instead of spaces. First character should be a..z character. "
              "If you cannot use latin characters in your language, use english word.",
              "script"),
        xi18nc("tooltip", "Create new script"),
        xi18nc("what's this", "Creates new script."),
        l)
  , d(new Private(this))
{
    d->engine.installExtensions(QJSEngine::ConsoleExtension);
    
    registerMetaObjects();
    
    QJSValueIterator it(d->engine.globalObject());
    while (it.hasNext()) {
        it.next();
        KexiScriptingDebug() << it.name() << ": " << it.value().toString();
    }
}

KexiScriptPart::~KexiScriptPart()
{
    delete d;
}

bool KexiScriptPart::execute(KexiPart::Item* item, QObject* sender)
{
    Q_UNUSED(sender);
    if (!item) {
        qWarning() << "Invalid item.";
        return false;
    }

    QString p = loadData(item);

    QJSValue result = execute(p);
    
    if (result.isError()) {
            QString errormessage = result.toString();
            long lineno = result.property("lineNumber").toInt();
            QMessageBox mb(xi18n("KEXI Script"), xi18n("Error executing script at line %1:\n%2").arg(lineno).arg(errormessage),  QMessageBox::Critical, QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
            mb.exec();
            return false;
    }
    return true; 
}

QJSValue KexiScriptPart::execute(const QString& program)
{
    QJSValue val;
    
    if (!d->engine.globalObject().hasProperty("KDb")) {
        val = d->engine.newQObject(&d->kexidbmodule);
        d->engine.globalObject().setProperty("KDb", val);
    }
    
    if (!program.isEmpty()) {
        return  d->engine.evaluate(program);
    }  
    return QJSValue();
}

void KexiScriptPart::initPartActions()
{

}

void KexiScriptPart::initInstanceActions()
{
    createSharedAction(Kexi::TextViewMode, xi18n("Configure Editor..."),
                       koIconName("configure"), QKeySequence(), "script_config_editor");
}

KexiView* KexiScriptPart::createView(QWidget *parent,
                                     KexiWindow *window,
                                     KexiPart::Item *item,
                                     Kexi::ViewMode viewMode,
                                     QMap<QString, QVariant>* staticObjectArgs)
{
    Q_ASSERT(item);
    Q_UNUSED(window);
    Q_UNUSED(staticObjectArgs);

    QString partname = item->name();
    if (! partname.isNull()) {
        if (viewMode == Kexi::TextViewMode) {
            return new KexiScriptDesignView(parent);
        }
    }
    return 0;
}

KLocalizedString KexiScriptPart::i18nMessage(
    const QString& englishMessage, KexiWindow* window) const
{
    if (englishMessage == "Design of object <resource>%1</resource> has been modified.")
        return kxi18nc(I18NC_NOOP("@info", "Design of script <resource>%1</resource> has been modified."));
    if (englishMessage == "Object <resource>%1</resource> already exists.")
        return kxi18nc(I18NC_NOOP("@info", "Script <resource>%1</resource> already exists."));
    return Part::i18nMessage(englishMessage, window);
}

QString KexiScriptPart::loadData(KexiPart::Item* item)
{
    QString data;
    if (!item) {
        return QString();
    }

    if (true != KexiMainWindowIface::global()->project()->dbConnection()->loadDataBlock(
                item->identifier(), &data))
    {
        return QString();
    }

    QString errMsg;
    int errLine;
    int errCol;

    QString scriptType;

    QDomDocument domdoc;
    bool parsed = domdoc.setContent(data, false, &errMsg, &errLine, &errCol);

    if (!parsed) {
        KexiScriptingWarning() << "XML parsing error line: " << errLine << " col: " << errCol << " message: " << errMsg;
        return QString();
    }

    QDomElement scriptelem = domdoc.namedItem("script").toElement();
    if (scriptelem.isNull()) {
        KexiScriptingWarning() << "script domelement is null";
        return QString();
    }

    scriptType = scriptelem.attribute("scripttype");
    if (scriptType.isEmpty()) {
        scriptType = "executable";
    }

    if (scriptType == "executable") {
        return scriptelem.text().toUtf8();
    } else {
        return QString();
    }
}

void KexiScriptPart::registerMetaObjects()
{
     QJSValue meta = d->engine.newQMetaObject(&KexiScriptAdaptor::staticMetaObject);
     d->engine.globalObject().setProperty("KexiScriptAdaptor", meta);
     
     meta = d->engine.newQMetaObject(&Scripting::KexiDBModule::staticMetaObject);
     d->engine.globalObject().setProperty("KDb", meta);
}

#include "kexiscriptpart.moc"
