/* This file is part of the KDE project
   Copyright (C) 2004 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2004-2016 Jarosław Staniek <staniek@kde.org>

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
#include "kexiqueryview.h"
#include "kexiquerydesignerguieditor.h"
#include "kexiquerydesignersql.h"
#include <KexiMainWindowIface.h>
#include <KexiWindow.h>
#include <kexiproject.h>
#include <kexipartinfo.h>

#include <KDbCursor>
#include <KDbParser>
#include <KDbQuerySchema>

#include <QDebug>

KEXI_PLUGIN_FACTORY(KexiQueryPart, "kexi_queryplugin.json")

KexiQueryPart::KexiQueryPart(QObject *parent, const QVariantList &l)
  : KexiPart::Part(parent,
        xi18nc("Translate this word using only lowercase alphanumeric characters (a..z, 0..9). "
              "Use '_' character instead of spaces. First character should be a..z character. "
              "If you cannot use latin characters in your language, use english word.",
              "query"),
        xi18nc("tooltip", "Create new query"),
        xi18nc("what's this", "Creates new query."),
        l)
{
    setInternalPropertyValue("textViewModeCaption", xi18n("SQL"));
}

KexiQueryPart::~KexiQueryPart()
{
}

KexiWindowData* KexiQueryPart::createWindowData(KexiWindow* window)
{
    KexiQueryPartTempData *data = new KexiQueryPartTempData(
        window, KexiMainWindowIface::global()->project()->dbConnection());
    data->setName(xi18nc("@info Object \"objectname\"", "%1 <resource>%2</resource>",
                         window->part()->info()->name(), window->partItem()->name()));
    return data;
}

KexiView* KexiQueryPart::createView(QWidget *parent, KexiWindow* window, KexiPart::Item *item,
                                    Kexi::ViewMode viewMode, QMap<QString, QVariant>*)
{
    Q_ASSERT(item);
    Q_UNUSED(item);
    Q_UNUSED(window);
    //qDebug();

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
        connect(prj, SIGNAL(newItemStored(KexiPart::Item*)),
                view, SLOT(slotNewItemStored(KexiPart::Item*)));
        connect(prj, SIGNAL(itemRemoved(KexiPart::Item)),
                view, SLOT(slotItemRemoved(KexiPart::Item)));
        connect(prj, SIGNAL(itemRenamed(KexiPart::Item,QString)),
                view, SLOT(slotItemRenamed(KexiPart::Item,QString)));
    }
    else if (viewMode == Kexi::TextViewMode) {
        view = new KexiQueryDesignerSqlView(parent);
        view->setObjectName("sqldesigner");
    }
    return view;
}

tristate KexiQueryPart::remove(KexiPart::Item *item)
{
    if (!KexiMainWindowIface::global()->project()
            || !KexiMainWindowIface::global()->project()->dbConnection())
        return false;
    KDbConnection *conn = KexiMainWindowIface::global()->project()->dbConnection();
    KDbQuerySchema *sch = conn->querySchema(item->identifier());
    if (sch)
        return conn->dropQuery(sch);
    //last chance: just remove item
    return conn->removeObject(item->identifier());
}

void KexiQueryPart::initPartActions()
{
}

void KexiQueryPart::initInstanceActions()
{
}

KDbObject* KexiQueryPart::loadSchemaObject(
    KexiWindow *window, const KDbObject& object, Kexi::ViewMode viewMode,
    bool *ownedByWindow)
{
    KexiQueryPartTempData * temp = static_cast<KexiQueryPartTempData*>(window->data());
    QString sql;
    if (!loadDataBlock(window, &sql, "sql")) {
        return 0;
    }
    KDbEscapedString sqlText(sql);
    KDbParser *parser = KexiMainWindowIface::global()->project()->sqlParser();
    KDbQuerySchema *query = 0;
    if (parser->parse(sqlText)) {
        query = parser->query();
    }
    //error?
    if (!query) {
        if (viewMode == Kexi::TextViewMode) {
            //for SQL view, no parsing is initially needed:
            //-just make a copy:
            return KexiPart::Part::loadSchemaObject(window, object, viewMode, ownedByWindow);
        }
        /* Set this to true on data loading loadSchemaObject() to indicate that TextView mode
         could be used instead of DataView or DesignView, because there are problems
         with opening object. */
        temp->proposeOpeningInTextViewModeBecauseOfProblems = true;
        //! @todo
        return 0;
    }
    qDebug() << KDbConnectionAndQuerySchema(
        KexiMainWindowIface::global()->project()->dbConnection(), *query);
    (KDbObject&)*query = object; //copy main attributes

    temp->registerTableSchemaChanges(query);
    if (ownedByWindow)
        *ownedByWindow = false;

    qDebug() << KDbConnectionAndQuerySchema(
        KexiMainWindowIface::global()->project()->dbConnection(), *query);
    return query;
}

KDbQuerySchema *KexiQueryPart::currentQuery(KexiView* view)
{
    if (!view)
        return 0;

    KexiQueryView *qvp = 0;
    if (!(qvp = qobject_cast<KexiQueryView*>(view))) {
        return 0;
    }

    return static_cast<KexiQueryPartTempData*>(qvp->window()->data())->query();
}

KLocalizedString KexiQueryPart::i18nMessage(const QString& englishMessage, KexiWindow* window) const
{
    if (englishMessage == "Design of object <resource>%1</resource> has been modified.")
        return kxi18nc(I18NC_NOOP("@info", "Design of query <resource>%1</resource> has been modified."));
    if (englishMessage == "Object <resource>%1</resource> already exists.")
        return kxi18nc(I18NC_NOOP("@info", "Query <resource>%1</resource> already exists."));

    return Part::i18nMessage(englishMessage, window);
}

tristate KexiQueryPart::rename(KexiPart::Item *item, const QString& newName)
{
    Q_ASSERT(item);
    Q_UNUSED(newName);
    if (!KexiMainWindowIface::global()->project()->dbConnection())
        return false;
    KexiMainWindowIface::global()->project()->dbConnection()
        ->setQuerySchemaObsolete(item->name());
    return true;
}

//----------------

KexiQueryPartTempData::KexiQueryPartTempData(KexiWindow* window, KDbConnection *conn)
        : KexiWindowData(window)
        , KDbTableSchemaChangeListener()
        , m_query(0)
        , m_queryChangedInView(Kexi::NoViewMode)
{
    this->conn = conn;
}

KexiQueryPartTempData::~KexiQueryPartTempData()
{
    KDbTableSchemaChangeListener::unregisterForChanges(conn, this);
}

void KexiQueryPartTempData::clearQuery()
{
    if (!m_query)
        return;
    unregisterForTablesSchemaChanges();
    m_query->clear();
}

void KexiQueryPartTempData::unregisterForTablesSchemaChanges()
{
    KDbTableSchemaChangeListener::unregisterForChanges(conn, this);
}

void KexiQueryPartTempData::registerTableSchemaChanges(KDbQuerySchema *q)
{
    if (!q)
        return;
    foreach(const KDbTableSchema* table, *q->tables()) {
        KDbTableSchemaChangeListener::registerForChanges(conn, this, table);
    }
}

tristate KexiQueryPartTempData::closeListener()
{
    KexiWindow* window = static_cast<KexiWindow*>(parent());
    return KexiMainWindowIface::global()->closeWindow(window);
}

KDbQuerySchema *KexiQueryPartTempData::takeQuery()
{
    KDbQuerySchema *query = m_query;
    m_query = 0;
    return query;
}

void KexiQueryPartTempData::setQuery(KDbQuerySchema *query)
{
    if (m_query && m_query == query)
        return;
    if (m_query
            /* query not owned by window */
            && (static_cast<KexiWindow*>(parent())->schemaObject() != static_cast<KDbObject*>(m_query)))
    {
        delete m_query;
    }
    m_query = query;
}

Kexi::ViewMode KexiQueryPartTempData::queryChangedInView() const
{
    return m_queryChangedInView;
}

void KexiQueryPartTempData::setQueryChangedInView(bool set)
{
    m_queryChangedInView = set ? qobject_cast<KexiWindow*>(parent())->currentViewMode()
                                       : Kexi::NoViewMode;
}

#include "kexiquerypart.moc"
