/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2002, 2003 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2004-2017 Jarosław Staniek <staniek@kde.org>

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

#include "kexitablepart.h"
#include <KexiIcon.h>
#include <core/KexiMainWindowIface.h>
#include <core/kexiproject.h>
#include <core/kexipartinfo.h>
#include <kexiutils/utils.h>
#include <widget/properties/KexiCustomPropertyFactory.h>
#include <widget/tableview/KexiDataTableView.h>
#include <widget/tableview/KexiDataTableScrollArea.h>
#include "kexitabledesignerview.h"
#include "kexitabledesigner_dataview.h"
#include "kexilookupcolumnpage.h"
#include <KexiWindow.h>
#include <kexiutils/utils.h>

#include <KDbConnection>

#include <KMessageBox>

#include <QDebug>
#include <QTabWidget>

KEXI_PLUGIN_FACTORY(KexiTablePart, "kexi_tableplugin.json")

//! @internal
class Q_DECL_HIDDEN KexiTablePart::Private
{
public:
    Private() {
    }
    ~Private() {
        delete static_cast<KexiLookupColumnPage*>(lookupColumnPage);
    }
    QPointer<KexiLookupColumnPage> lookupColumnPage;
};

KexiTablePart::KexiTablePart(QObject *parent, const QVariantList& l)
  : KexiPart::Part(parent,
        xi18nc("Translate this word using only lowercase alphanumeric characters (a..z, 0..9). "
              "Use '_' character instead of spaces. First character should be a..z character. "
              "If you cannot use latin characters in your language, use english word.",
              "table"),
        xi18nc("tooltip", "Create new table"),
        xi18nc("what's this", "Creates new table."),
        l)
  , d(new Private)
{
    // needed for custom "identifier" property editor widget
    KexiCustomPropertyFactory::init();

//! @todo js: also add Kexi::TextViewMode when we'll have SQL ALTER TABLE EDITOR!!!
}

KexiTablePart::~KexiTablePart()
{
    delete d;
}

void KexiTablePart::initPartActions()
{
}

void KexiTablePart::initInstanceActions()
{
}

KexiWindowData* KexiTablePart::createWindowData(KexiWindow* window)
{
    KexiMainWindowIface *win = KexiMainWindowIface::global();
    return new KexiTablePartTempData(window, win->project()->dbConnection());
}

KexiView* KexiTablePart::createView(QWidget *parent, KexiWindow* window,
                                    KexiPart::Item *item, Kexi::ViewMode viewMode, QMap<QString, QVariant>*)
{
    Q_ASSERT(item);
    KexiMainWindowIface *win = KexiMainWindowIface::global();
    if (!win || !win->project() || !win->project()->dbConnection())
        return 0;


    KexiTablePartTempData *temp
        = static_cast<KexiTablePartTempData*>(window->data());
    if (!temp->table()) {
        temp->setTable(win->project()->dbConnection()->tableSchema(item->name()));
        qDebug() << "schema is " << temp->table();
    }

    if (viewMode == Kexi::DesignViewMode) {
        KexiTableDesignerView *t = new KexiTableDesignerView(parent);
        return t;
    } else if (viewMode == Kexi::DataViewMode) {
        if (!temp->table()) {
            return 0; //!< @todo message
        }
        //we're not setting table schema here -it will be forced to set
        // in KexiTableDesigner_DataView::afterSwitchFrom()
        KexiTableDesigner_DataView *t = new KexiTableDesigner_DataView(parent);
        return t;
    }
    return 0;
}

tristate KexiTablePart::remove(KexiPart::Item *item)
{
    KexiProject *project = KexiMainWindowIface::global()->project();
    if (!project || !project->dbConnection())
        return false;

    KDbConnection *conn = project->dbConnection();
    KDbTableSchema *sch = conn->tableSchema(item->identifier());

    if (sch) {
        const tristate res = KexiTablePart::askForClosingObjectsUsingTableSchema(
            KexiMainWindowIface::global()->openedWindowFor(item->identifier()), conn, sch,
            kxi18nc("@info",
                    "<para>You are about to delete table <resource>%1</resource> but it is used by "
                    "following opened windows:</para>")
                .subs(sch->name()));
        if (res != true) {
            return res;
        }
        return conn->dropTable(sch);
    }
    //last chance: just remove item
    return conn->removeObject(item->identifier());
}

tristate KexiTablePart::rename(KexiPart::Item *item, const QString& newName)
{
    Q_ASSERT(item);
    KDbConnection *conn = KexiMainWindowIface::global()->project()->dbConnection();
    KDbTableSchema *schema = conn->tableSchema(item->identifier());
    if (!schema)
        return false;
    const tristate res = KexiTablePart::askForClosingObjectsUsingTableSchema(
        KexiMainWindowIface::global()->openedWindowFor(item->identifier()), conn, schema,
        kxi18nc("@info",
                "<para>You are about to rename table <resource>%1</resource> but it is used by "
                "following opened windows:</para>")
            .subs(schema->name()));
    if (res != true) {
        return res;
    }
    return conn->alterTableName(schema, newName);
}

KDbObject* KexiTablePart::loadSchemaObject(KexiWindow *window, const KDbObject& object,
                              Kexi::ViewMode viewMode, bool *ownedByWindow)
{
    Q_UNUSED(window);
    Q_UNUSED(viewMode);
    Q_ASSERT(ownedByWindow);
    *ownedByWindow = false;
    return KexiMainWindowIface::global()->project()->dbConnection()->tableSchema(object.name());
}

// static
tristate KexiTablePart::askForClosingObjectsUsingTableSchema(KexiWindow *window,
                                                             KDbConnection *conn,
                                                             KDbTableSchema *table,
                                                             const KLocalizedString &msg)
{
    Q_ASSERT(conn);
    Q_ASSERT(table);
    if (!window) {
        return true;
    }
    QList<KDbTableSchemaChangeListener*> listeners
            = KDbTableSchemaChangeListener::listeners(conn, table);
    KexiTablePartTempData *temp = static_cast<KexiTablePartTempData*>(window->data());
    // Special case: listener that is equal to window->data() will be silently closed
    // without asking for confirmation. It is not counted when looking for objects that
    // are "blocking" changes of the table.
    const bool tempListenerExists = listeners.removeAll(temp) > 0;
    // Immediate success if there's no temp-data's listener to close nor other listeners to close
    if (!tempListenerExists && listeners.isEmpty()) {
        return true;
    }

    if (!listeners.isEmpty()) {
        QString openedObjectsStr = "<p><ul>";
        for(const KDbTableSchemaChangeListener* listener : listeners) {
            openedObjectsStr += QString("<li>%1</li>").arg(listener->name());
        }
        openedObjectsStr += "</ul></p>";
        QString message = "<html>"
            + i18nc("@info/plain Sentence1 Sentence2 Sentence3", "%1%2%3",
                    KexiUtils::localizedStringToHtmlSubstring(msg), openedObjectsStr,
                    KexiUtils::localizedStringToHtmlSubstring(
                        kxi18nc("@info", "<para>Do you want to close these windows and save the "
                                         "design or cancel saving?</para>")))
            + "</html>";
        KGuiItem closeAndSaveItem(KStandardGuiItem::save());
        closeAndSaveItem.setText(
            xi18nc("@action:button Close all windows and save", "Close Windows and Save"));
        closeAndSaveItem.setToolTip(xi18nc("@info:tooltip Close all windows and save design",
                                           "Close all windows and save design"));
        const int r = KMessageBox::questionYesNo(window, message, QString(), closeAndSaveItem,
                                                 KStandardGuiItem::cancel(), QString(),
                                                 KMessageBox::Notify | KMessageBox::Dangerous);
        if (r != KMessageBox::Yes) {
            return cancelled;
        }
    }
    // try to close every window depending on the table (if present) and also the temp-data's
    // listener (if present)
    const tristate res = KDbTableSchemaChangeListener::closeListeners(conn, table, { temp });
    if (res != true) { //do not expose closing errors twice; just cancel
        return cancelled;
    }
    return true;
}

KLocalizedString KexiTablePart::i18nMessage(
    const QString& englishMessage, KexiWindow* window) const
{
    Q_UNUSED(window);
    if (englishMessage == "Design of object <resource>%1</resource> has been modified.")
        return kxi18nc(I18NC_NOOP("@info", "Design of table <resource>%1</resource> has been modified."));

    if (englishMessage == "Object <resource>%1</resource> already exists.")
        return kxi18nc(I18NC_NOOP("@info", "Table <resource>%1</resource> already exists."));

    if (window->currentViewMode() == Kexi::DesignViewMode && !window->neverSaved()
            && englishMessage == ":additional message before saving design")
        return kxi18nc(I18NC_NOOP("@info", "<warning>Any data in this table will be deleted upon design's saving!</warning>"));

    return Part::i18nMessage(englishMessage, window);
}

void KexiTablePart::setupCustomPropertyPanelTabs(QTabWidget *tab)
{
    if (!d->lookupColumnPage) {
        d->lookupColumnPage = new KexiLookupColumnPage(0);
        connect(d->lookupColumnPage,
                SIGNAL(jumpToObjectRequested(QString,QString)),
                KexiMainWindowIface::global()->thisWidget(),
                SLOT(highlightObject(QString,QString)));

//! @todo add "Table" tab
        /*
          connect(d->dataSourcePage, SIGNAL(formDataSourceChanged(QCString,QCString)),
            KFormDesigner::FormManager::self(), SLOT(setFormDataSource(QCString,QCString)));
          connect(d->dataSourcePage, SIGNAL(dataSourceFieldOrExpressionChanged(QString,QString,KDbField::Type)),
            KFormDesigner::FormManager::self(), SLOT(setDataSourceFieldOrExpression(QString,QString,KDbField::Type)));
          connect(d->dataSourcePage, SIGNAL(insertAutoFields(QString,QString,QStringList)),
            KFormDesigner::FormManager::self(), SLOT(insertAutoFields(QString,QString,QStringList)));*/
    }

    KexiProject *prj = KexiMainWindowIface::global()->project();
    d->lookupColumnPage->setProject(prj);

//! @todo add lookup field icon
    tab->addTab(d->lookupColumnPage, KexiIcon("combobox"), QString());
    tab->setTabToolTip(tab->indexOf(d->lookupColumnPage), xi18n("Lookup column"));
}

KexiLookupColumnPage* KexiTablePart::lookupColumnPage() const
{
    return d->lookupColumnPage;
}

//----------------

class Q_DECL_HIDDEN KexiTablePartTempData::Private
{
public:
    Private()
        : table(nullptr)
    {
    }
    KDbTableSchema *table;
    KDbConnection *conn;
};

KexiTablePartTempData::KexiTablePartTempData(KexiWindow* parent, KDbConnection *conn)
        : KexiWindowData(parent)
        , KDbTableSchemaChangeListener()
        , tableSchemaChangedInPreviousView(true /*to force reloading on startup*/)
        , d(new Private)
{
    d->conn = conn;
    setName(KexiUtils::localizedStringToHtmlSubstring(
        kxi18nc("@info", "Table <resource>%1</resource>").subs(parent->partItem()->name())));
}

KexiTablePartTempData::~KexiTablePartTempData()
{
    KDbTableSchemaChangeListener::unregisterForChanges(d->conn, this);
    delete d;
}

KDbTableSchema* KexiTablePartTempData::table()
{
    return d->table;
}

KDbConnection* KexiTablePartTempData::connection()
{
    return d->conn;
}

void KexiTablePartTempData::closeDataInDataView()
{
    const KexiWindow* window = static_cast<KexiWindow*>(parent());
    if (window->currentViewMode() != Kexi::DataViewMode) {
        KexiTableDesigner_DataView *dataView
            = qobject_cast<KexiTableDesigner_DataView*>(window->viewForMode(Kexi::DataViewMode));
        if (dataView && dataView->tableView()->data()) {
            dataView->setData(nullptr);
        }
    }
}

void KexiTablePartTempData::setTable(KDbTableSchema *table)
{
    if (d->table == table) {
        return;
    }
    if (d->table) {
        KDbTableSchemaChangeListener::unregisterForChanges(d->conn, d->table);
    }
    closeDataInDataView();
    d->table = table;
    if (d->table) {
        KDbTableSchemaChangeListener::registerForChanges(d->conn, this, d->table);
    }
}

tristate KexiTablePartTempData::closeListener()
{
    KexiWindow* window = static_cast<KexiWindow*>(parent());
    qDebug() << window->partItem()->name();
    closeDataInDataView();
    if (closeWindowOnCloseListener) {
        return KexiMainWindowIface::global()->closeWindow(window);
    }
    return true;
}

#include "kexitablepart.moc"
