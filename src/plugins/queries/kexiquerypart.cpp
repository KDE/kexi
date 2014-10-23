/* This file is part of the KDE project
   Copyright (C) 2004 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2004-2010 Jarosław Staniek <staniek@kde.org>

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

#include "kexiquerypart.h"

#include <kdebug.h>

#include <KexiMainWindowIface.h>
#include <KexiWindow.h>
#include <kexiproject.h>
#include <kexipartinfo.h>

#include <db/cursor.h>
#include <db/parser/parser.h>

#include "kexiqueryview.h"
#include "kexiquerydesignerguieditor.h"
#include "kexiquerydesignersql.h"

//------------------------------------------------

KexiQueryPart::KexiQueryPart(QObject *parent, const QVariantList &l)
  : KexiPart::Part(parent,
        i18nc("Translate this word using only lowercase alphanumeric characters (a..z, 0..9). "
              "Use '_' character instead of spaces. First character should be a..z character. "
              "If you cannot use latin characters in your language, use english word.",
              "query"),
        i18nc("tooltip", "Create new query"),
        i18nc("what's this", "Creates new query."),
        l)
{
    setInternalPropertyValue("textViewModeCaption", i18n("SQL"));
}

KexiQueryPart::~KexiQueryPart()
{
}

KexiWindowData* KexiQueryPart::createWindowData(KexiWindow* window)
{
    KexiQueryPart::TempData *data = new KexiQueryPart::TempData(
        window, KexiMainWindowIface::global()->project()->dbConnection());
    data->listenerInfoString = i18nc("@info Object \"objectname\"", "%1 <resource>%2</resource>",
                                     window->part()->info()->instanceCaption(),
                                     window->partItem()->name());
    return data;
}

KexiView* KexiQueryPart::createView(QWidget *parent, KexiWindow* window, KexiPart::Item &item,
                                    Kexi::ViewMode viewMode, QMap<QString, QVariant>*)
{
    Q_UNUSED(item);
    Q_UNUSED(window);
    //kDebug();

    KexiView* view = 0;
    if (viewMode == Kexi::DataViewMode) {
        view = new KexiQueryView(parent);
        view->setObjectName("dataview");
    }
    else if (viewMode == Kexi::DesignViewMode) {
        view = new KexiQueryDesignerGuiEditor(parent);
        view->setObjectName("guieditor");
        //needed for updating tables combo box:
        KexiProject *prj = KexiMainWindowIface::global()->project();
        connect(prj, SIGNAL(newItemStored(KexiPart::Item&)),
                view, SLOT(slotNewItemStored(KexiPart::Item&)));
        connect(prj, SIGNAL(itemRemoved(KexiPart::Item)),
                view, SLOT(slotItemRemoved(KexiPart::Item)));
        connect(prj, SIGNAL(itemRenamed(KexiPart::Item,QString)),
                view, SLOT(slotItemRenamed(KexiPart::Item,QString)));
    }
    else if (viewMode == Kexi::TextViewMode) {
        view = new KexiQueryDesignerSQLView(parent);
        view->setObjectName("sqldesigner");
    }
    return view;
}

tristate KexiQueryPart::remove(KexiPart::Item &item)
{
    if (!KexiMainWindowIface::global()->project()
            || !KexiMainWindowIface::global()->project()->dbConnection())
        return false;
    KexiDB::Connection *conn = KexiMainWindowIface::global()->project()->dbConnection();
    KexiDB::QuerySchema *sch = conn->querySchema(item.identifier());
    if (sch)
        return conn->dropQuery(sch);
    //last chance: just remove item
    return conn->removeObject(item.identifier());
}

void KexiQueryPart::initPartActions()
{
}

void KexiQueryPart::initInstanceActions()
{
}

KexiDB::SchemaData* KexiQueryPart::loadSchemaData(
    KexiWindow *window, const KexiDB::SchemaData& sdata, Kexi::ViewMode viewMode,
    bool *ownedByWindow)
{
    KexiQueryPart::TempData * temp = static_cast<KexiQueryPart::TempData*>(window->data());
    QString sqlText;
    if (!loadDataBlock(window, sqlText, "sql")) {
        return 0;
    }
    KexiDB::Parser *parser = KexiMainWindowIface::global()->project()->sqlParser();
    parser->parse(sqlText);
    KexiDB::QuerySchema *query = parser->query();
    //error?
    if (!query) {
        if (viewMode == Kexi::TextViewMode) {
            //for SQL view, no parsing is initially needed:
            //-just make a copy:
            return KexiPart::Part::loadSchemaData(window, sdata, viewMode, ownedByWindow);
        }
        /* Set this to true on data loading loadSchemaData() to indicate that TextView mode
         could be used instead of DataView or DesignView, because there are problems
         with opening object. */
        temp->proposeOpeningInTextViewModeBecauseOfProblems = true;
        //! @todo
        return 0;
    }
    query->debug();
    (KexiDB::SchemaData&)*query = sdata; //copy main attributes

    temp->registerTableSchemaChanges(query);
    if (ownedByWindow)
        *ownedByWindow = false;

    query->debug();
    return query;
}

KLocalizedString KexiQueryPart::i18nMessage(const QString& englishMessage, KexiWindow* window) const
{
    if (englishMessage == "Design of object <resource>%1</resource> has been modified.")
        return ki18n(I18N_NOOP("Design of query <resource>%1</resource> has been modified."));
    if (englishMessage == "Object <resource>%1</resource> already exists.")
        return ki18n(I18N_NOOP("Query <resource>%1</resource> already exists."));

    return Part::i18nMessage(englishMessage, window);
}

tristate KexiQueryPart::rename(KexiPart::Item &item, const QString& newName)
{
    Q_UNUSED(newName);
    if (!KexiMainWindowIface::global()->project()->dbConnection())
        return false;
    KexiMainWindowIface::global()->project()->dbConnection()
    ->setQuerySchemaObsolete(item.name());
    return true;
}

//----------------

KexiQueryPart::TempData::TempData(KexiWindow* window, KexiDB::Connection *conn)
        : KexiWindowData(window)
        , KexiDB::Connection::TableSchemaChangeListenerInterface()
        , m_query(0)
        , m_queryChangedInPreviousView(false)
{
    this->conn = conn;
}

KexiQueryPart::TempData::~TempData()
{
    conn->unregisterForTablesSchemaChanges(*this);
}

void KexiQueryPart::TempData::clearQuery()
{
    if (!m_query)
        return;
    unregisterForTablesSchemaChanges();
    m_query->clear();
}

void KexiQueryPart::TempData::unregisterForTablesSchemaChanges()
{
    conn->unregisterForTablesSchemaChanges(*this);
}

void KexiQueryPart::TempData::registerTableSchemaChanges(KexiDB::QuerySchema *q)
{
    if (!q)
        return;
    foreach(KexiDB::TableSchema* table, *q->tables()) {
        conn->registerForTableSchemaChanges(*this, *table);
    }
}

tristate KexiQueryPart::TempData::closeListener()
{
    KexiWindow* window = static_cast<KexiWindow*>(parent());
    return KexiMainWindowIface::global()->closeWindow(window);
}

KexiDB::QuerySchema *KexiQueryPart::TempData::takeQuery()
{
    KexiDB::QuerySchema *query = m_query;
    m_query = 0;
    return query;
}

void KexiQueryPart::TempData::setQuery(KexiDB::QuerySchema *query)
{
    if (m_query && m_query == query)
        return;
    if (m_query
            /* query not owned by window */
            && (static_cast<KexiWindow*>(parent())->schemaData() != static_cast<KexiDB::SchemaData*>(m_query)))
    {
        delete m_query;
    }
    m_query = query;
}

bool KexiQueryPart::TempData::queryChangedInPreviousView() const
{
    return m_queryChangedInPreviousView;
}

void KexiQueryPart::TempData::setQueryChangedInPreviousView(bool set)
{
    m_queryChangedInPreviousView = set;
}

//----------------

K_EXPORT_KEXIPART_PLUGIN( KexiQueryPart, query )

#include "kexiquerypart.moc"
