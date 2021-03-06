/* This file is part of the KDE project
   Copyright (C) 2004 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2004, 2006 Jarosław Staniek <staniek@kde.org>

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

#include "kexiqueryview.h"
#include "kexiquerydesignersql.h"
#include "kexiquerydesignerguieditor.h"
#include "kexiquerypart.h"
#include <widget/tableview/KexiTableScrollArea.h>
#include <widget/kexiqueryparameters.h>
#include <kexiproject.h>
#include <KexiMainWindowIface.h>
#include <kexiutils/utils.h>
#include <KexiWindow.h>

#include <KDbConnection>
#include <KDbCursor>
#include <KDbParser>
#include <KDbQuerySchemaParameter>
#include <KDbTableViewColumn>

//! @internal
class Q_DECL_HIDDEN KexiQueryView::Private
{
public:
    Private()
            : cursor(0),
              currentParams()
    {}
    ~Private() {}
    KDbQuerySchema* query = nullptr;
    KDbCursor *cursor;
    QList<QVariant> currentParams;
    /*! Used in storeNewData(), storeData() to decide whether
     we should ask other view to save changes.
     Stores information about view mode. */
};

//---------------------------------------------------------------------------------

KexiQueryView::KexiQueryView(QWidget *parent)
        : KexiDataTableView(parent)
        , d(new Private())
{
    // setup main menu actions
    QList<QAction*> mainMenuActions;
    mainMenuActions
            << sharedAction("project_export_data_table");
    setMainMenuActions(mainMenuActions);

    tableView()->setInsertingEnabled(false); //default
}

KexiQueryView::~KexiQueryView()
{
    if (d->cursor)
        d->cursor->connection()->deleteCursor(d->cursor);
    delete d;
}

tristate KexiQueryView::setQuery(KDbQuerySchema *query)
{
    if (d->query == query) {
        return true;
    }
    KDbCursor* newCursor;
    if (query) {
        KexiUtils::WaitCursor wait;
        KDbConnection * conn = KexiMainWindowIface::global()->project()->dbConnection();
        //qDebug() << query->parameters(conn);
        bool ok;
        {
            KexiUtils::WaitCursorRemover remover;
            d->currentParams = KexiQueryParameters::getParameters(this, conn, query, &ok);
        }
        if (!ok) {//input cancelled
            return cancelled;
        }
        newCursor = conn->executeQuery(query, d->currentParams);
        if (!newCursor) {
            window()->setStatus(conn, xi18n("Query executing failed."));
            //! @todo also provide server result and sql statement
            return false;
        }
    } else {
        newCursor = nullptr;
    }

    if (d->cursor) {
        d->cursor->connection()->deleteCursor(d->cursor);
    }
    d->cursor = newCursor;
    d->query = query;
    setData(d->cursor);

//! @todo remove close() when dynamic cursors arrive
    if (d->cursor && !d->cursor->close()) {
        return false;
    }

//! @todo maybe allow writing and inserting for single-table relations?
    tableView()->setReadOnly(true);
//! @todo maybe allow writing and inserting for single-table relations?
    //set data model itself read-only too
    if (tableView()->data()) {
        tableView()->data()->setReadOnly(true);
    }
    tableView()->setInsertingEnabled(false);
    return true;
}

KDbQuerySchema* KexiQueryView::query()
{
    return d->query;
}

tristate KexiQueryView::afterSwitchFrom(Kexi::ViewMode mode)
{
    if (mode == Kexi::NoViewMode) {
        KDbQuerySchema *querySchema = static_cast<KDbQuerySchema *>(window()->schemaObject());
        const tristate result = setQuery(querySchema);
        if (true != result)
            return result;
    } else if (mode == Kexi::DesignViewMode || mode == Kexi::TextViewMode) {
        KexiQueryPartTempData * temp = static_cast<KexiQueryPartTempData*>(window()->data());
        const tristate result = setQuery(temp->query());
        if (true != result)
            return result;
    }
    return true;
}

KDbObject* KexiQueryView::storeNewData(const KDbObject& object,
                                                KexiView::StoreNewDataOptions options,
                                                bool *cancel)
{
    KexiView * view = window()->viewThatRecentlySetDirtyFlag();
    KexiQueryDesignerGuiEditor *guiView = dynamic_cast<KexiQueryDesignerGuiEditor*>(view);
    if (guiView) {
        return guiView->storeNewData(object, options, cancel);
    }
    KexiQueryDesignerSqlView *sqlView = dynamic_cast<KexiQueryDesignerSqlView*>(view);
    if (sqlView) {
        return sqlView->storeNewData(object, options, cancel);
    }
    return 0;
}

tristate KexiQueryView::storeData(bool dontAsk)
{
    KexiView * view = window()->viewThatRecentlySetDirtyFlag();
    KexiQueryDesignerGuiEditor *guiView = dynamic_cast<KexiQueryDesignerGuiEditor*>(view);
    if (guiView) {
        return guiView->storeData(dontAsk);
    }
    KexiQueryDesignerSqlView *sqlView = dynamic_cast<KexiQueryDesignerSqlView*>(view);
    if (sqlView) {
        return sqlView->storeData(dontAsk);
    }
    return false;
}

QList<QVariant> KexiQueryView::currentParameters() const
{
    return d->currentParams;
}
