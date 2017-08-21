/*
 * Kexi Report Plugin
 * Copyright (C) 2007-2008 by Adam Pigg <adam@piggz.co.uk>
 * Copyright (C) 2011-2015 Jarosław Staniek <staniek@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "kexireportpart.h"

#include <QTabWidget>
#include <QDebug>

#include <KDbConnection>

#include <KLocalizedString>

#include <KexiIcon.h>
#include <core/KexiWindow.h>
#include "kexireportview.h"
#include "kexireportdesignview.h"
#include <core/KexiMainWindowIface.h>
#include "kexisourceselector.h"
#include <widget/properties/KexiCustomPropertyFactory.h>

//! @internal
class Q_DECL_HIDDEN KexiReportPart::Private
{
public:
    Private() : toolboxActionGroup(0)
    {
        sourceSelector = 0;
    }
    ~Private() {
    }
    KexiSourceSelector *sourceSelector;
    QActionGroup toolboxActionGroup;
    QMap<QString, QAction*> toolboxActionsByName;
};

static bool isInterpreterSupported(const QString &interpreterName)
{
    return 0 == interpreterName.compare(QLatin1String("javascript"), Qt::CaseInsensitive)
           || 0 == interpreterName.compare(QLatin1String("qtscript"), Qt::CaseInsensitive);
}

KexiReportPart::KexiReportPart(QObject *parent, const QVariantList &l)
  : KexiPart::Part(parent,
        xi18nc("Translate this word using only lowercase alphanumeric characters (a..z, 0..9). "
              "Use '_' character instead of spaces. First character should be a..z character. "
              "If you cannot use latin characters in your language, use english word.",
              "report"),
        xi18nc("tooltip", "Create new report"),
        xi18nc("what's this", "Creates new report."),
        l)
  , d(new Private)
{
    setInternalPropertyValue("newObjectsAreDirty", true);
    // needed for custom "pixmap" property editor widget
    KexiCustomPropertyFactory::init();
}

KexiReportPart::~KexiReportPart()
{
    delete d;
}

KLocalizedString KexiReportPart::i18nMessage(
    const QString& englishMessage, KexiWindow* window) const
{
    Q_UNUSED(window);
    if (englishMessage == "Design of object <resource>%1</resource> has been modified.")
        return kxi18nc(I18NC_NOOP("@info", "Design of report <resource>%1</resource> has been modified."));
    if (englishMessage == "Object <resource>%1</resource> already exists.")
        return kxi18nc(I18NC_NOOP("@info", "Report <resource>%1</resource> already exists."));

    return Part::i18nMessage(englishMessage, window);
}

KexiView* KexiReportPart::createView(QWidget *parent, KexiWindow* window,
                                     KexiPart::Item *item, Kexi::ViewMode viewMode, QMap<QString, QVariant>*)
{
    Q_ASSERT(item);
    Q_UNUSED(window);
    Q_UNUSED(item);

    KexiView* view = 0;

    if (viewMode == Kexi::DataViewMode) {
        view = new KexiReportView(parent);

    } else if (viewMode == Kexi::DesignViewMode) {
        view = new KexiReportDesignView(parent, d->sourceSelector);
        connect(d->sourceSelector, &KexiSourceSelector::sourceDataChanged, qobject_cast<KexiReportDesignView*>(view), &KexiReportDesignView::slotSourceDataChanged);
        connect(view, SIGNAL(itemInserted(QString)), this, SLOT(slotItemInserted(QString)));
    }
    return view;
}

void KexiReportPart::initPartActions()
{
    KexiMainWindowIface *win = KexiMainWindowIface::global();
    QList<QAction*> reportActions = KReportDesigner::itemActions(&d->toolboxActionGroup);

    foreach(QAction* action, reportActions) {
        connect(action, SIGNAL(triggered(bool)), this, SLOT(slotToolboxActionTriggered(bool)));
        win->addToolBarAction("report", action);
        d->toolboxActionsByName.insert(action->objectName(), action);
    }

}

KDbObject* KexiReportPart::loadSchemaObject(
    KexiWindow *window, const KDbObject& object, Kexi::ViewMode viewMode,
    bool *ownedByWindow)
{
    QString layout;
    if (   !loadDataBlock(window, &layout, "layout") == true
        && !loadDataBlock(window, &layout, "pgzreport_layout") == true /* compat */)
    {
        return 0;
    }

    QDomDocument doc;
    if (!doc.setContent(layout)) {
        return 0;
    }

    KexiReportPartTempData * temp = static_cast<KexiReportPartTempData*>(window->data());
    const QDomElement root = doc.documentElement();
    temp->reportDefinition = root.firstChildElement("report:content");
    if (temp->reportDefinition.isNull()) {
        qWarning() << "no report report:content element found in report" << window->partItem()->name();
        return 0;
    }
    temp->connectionDefinition = root.firstChildElement("connection");
    if (temp->connectionDefinition.isNull()) {
        qWarning() << "no report report:content element found in report" << window->partItem()->name();
        return 0;
    }
    return KexiPart::Part::loadSchemaObject(window, object, viewMode, ownedByWindow);
}

KexiWindowData* KexiReportPart::createWindowData(KexiWindow* window)
{
    return new KexiReportPartTempData(window);
}

KexiReportPartTempData::KexiReportPartTempData(KexiWindow* parent)
        : KexiWindowData(parent)
        , reportSchemaChangedInPreviousView(true /*to force reloading on startup*/)
{
}

void KexiReportPart::setupCustomPropertyPanelTabs(QTabWidget *tab)
{
    if (!d->sourceSelector) {
        d->sourceSelector = new KexiSourceSelector(KexiMainWindowIface::global()->project(), tab);
    }
    tab->addTab(d->sourceSelector, koIcon("server-database"), QString());
    tab->setTabToolTip(tab->indexOf(d->sourceSelector), xi18n("Data Source"));
}

void KexiReportPart::slotToolboxActionTriggered(bool checked)
{
    if (!checked)
        return;
    QObject *theSender = sender();
    if (!theSender)
        return;

    QString senderName = sender()->objectName();
    KexiMainWindowIface *mainwin = KexiMainWindowIface::global();

    KexiWindow *win = mainwin->currentWindow();

    if (!win)
        return;

    KexiView *designView = win->viewForMode(Kexi::DesignViewMode);

    if (designView) {
        KexiReportDesignView *dv = dynamic_cast<KexiReportDesignView*>(designView);
        if (!dv)
            return;
        dv->triggerAction(senderName);
    }
}

void KexiReportPart::slotItemInserted(const QString& entity)
{
    Q_UNUSED(entity);
    // uncheck toolbox action after it is used
    QAction * a = d->toolboxActionGroup.checkedAction();
    if (a) {
        a->setChecked(false);
    }
}

QStringList KexiReportPart::scriptList() const
{
    QStringList scripts;

    KexiMainWindowIface *win = KexiMainWindowIface::global();

    if (win->project() && win->project()->dbConnection()) {
        QList<int> scriptids = win->project()->dbConnection()->objectIds(KexiPart::ScriptObjectType);
        QStringList scriptnames = win->project()->dbConnection()->objectNames(KexiPart::ScriptObjectType);

        qDebug() << scriptids << scriptnames;

        int i = 0;
        foreach(int id, scriptids) {
            qDebug() << "ID:" << id;
            tristate res;
            QString script;
            res = win->project()->dbConnection()->loadDataBlock(id, &script, QString());
            if (res == true) {
                QDomDocument domdoc;
                bool parsed = domdoc.setContent(script, false);

                QDomElement scriptelem = domdoc.namedItem("script").toElement();
                if (parsed && !scriptelem.isNull()) {
                    if (scriptelem.attribute("scripttype") == "object"
                        && isInterpreterSupported(scriptelem.attribute("language")))
                    {
                        scripts << scriptnames[i];
                    }
                } else {
                    qWarning() << "Unable to parse script";
                }
            } else {
                qWarning() << "Unable to loadDataBlock";
            }
            ++i;
        }

        qDebug() << scripts;
    }
    return scripts;
}

QString KexiReportPart::scriptCode(const QString& scriptname) const
{
    QString scripts;

    KexiMainWindowIface *win = KexiMainWindowIface::global();

    if (win->project() && win->project()->dbConnection()) {
        QList<int> scriptids = win->project()->dbConnection()->objectIds(KexiPart::ScriptObjectType);
        QStringList scriptnames = win->project()->dbConnection()->objectNames(KexiPart::ScriptObjectType);

        int i = 0;
        foreach(int id, scriptids) {
            qDebug() << "ID:" << id;
            tristate res;
            QString script;
            res = win->project()->dbConnection()->loadDataBlock(id, &script, QString());
            if (res == true) {
                QDomDocument domdoc;
                bool parsed = domdoc.setContent(script, false);

                if (! parsed) {
                    qWarning() << "XML parsing error";
                    return QString();
                }

                QDomElement scriptelem = domdoc.namedItem("script").toElement();
                if (scriptelem.isNull()) {
                    qWarning() << "script domelement is null";
                    return QString();
                }

                QString interpretername = scriptelem.attribute("language");
                qDebug() << scriptelem.attribute("scripttype");
                qDebug() << scriptname << scriptnames[i];

                if ((isInterpreterSupported(interpretername) && scriptelem.attribute("scripttype") == "module") || scriptname == scriptnames[i])
                {
                    scripts += '\n' + scriptelem.text().toUtf8();
                }
                ++i;
            } else {
                qWarning() << "Unable to loadDataBlock";
            }
        }
    }
    return scripts;
}
