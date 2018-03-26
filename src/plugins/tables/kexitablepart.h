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

#ifndef KEXITABLEPART_H
#define KEXITABLEPART_H

#include <kexi.h>
#include <kexipart.h>
#include <KexiWindow.h>
#include <KexiWindowData.h>
#include <kexipartitem.h>

#include <KDbTableSchemaChangeListener>

class KexiLookupColumnPage;

//! @short Temporary data kept in memory while switching between Table Window's views
class KexiTablePartTempData : public KexiWindowData, public KDbTableSchemaChangeListener
{
    Q_OBJECT
public:
    KexiTablePartTempData(KexiWindow* parent, KDbConnection *conn);

    ~KexiTablePartTempData();

    //! Table used for this data
    KDbTableSchema* table();

    //! Sets table used for this data
    //! If the previous table differs from @a table and is not @c nullptr, listener for
    //! it will be unregistered.
    //! If @a table is not @c nullptr, this temp-data object will be registered as a listener
    //! for it.
    void setTable(KDbTableSchema *table);

    //! Connection used for retrieving definition of the query
    KDbConnection* connection();

    /*! true, if \a table member has changed in previous view. Used on view switching.
     We're checking this flag to see if we should refresh data for DataViewMode. */
    bool tableSchemaChangedInPreviousView;

    //! @c true indicates that closeListener() should close the table designer window.
    //! This is disabled in one case: upon saving of the design of this table.
    //! @see KexiTableDesignerView::storeData()
    bool closeWindowOnCloseListener = true;

protected:
    //! Closes listener - this temp-data acts as a listener for tracking changes in table schema
    //! that is displayed in the window's data view.
    //! It just calls KexiDataTableView::setData(nullptr) is there's data set for the view
    //! (i.e. if KexiDataTableView::tableView()->data() is not @c nullptr).
    tristate closeListener() override;

private:
    void closeDataInDataView();

    Q_DISABLE_COPY(KexiTablePartTempData)
    class Private;
    Private * const d;
};

//! @short Kexi Table Designer plugin
class KexiTablePart : public KexiPart::Part
{
    Q_OBJECT

public:
    KexiTablePart(QObject *parent, const QVariantList &);
    virtual ~KexiTablePart();

    virtual tristate remove(KexiPart::Item *item);

    virtual tristate rename(KexiPart::Item *item, const QString& newName);

    /**
     * Closes objects that listenen to changes of the table schema @a table, i.e. use it.
     *
     * These objects can be currently:
     * - lookup fields of other tables
     * - queries using the table directly or via lookup fields
     * - forms and reports that use the table directly as data source or via query.
     *
     * Scripts referencing the table programatically are not analyzed, so they can fail on next
     * execution.
     *
     * This method asks the user for approval if there is at least one object that listens for
     * changes of the schema (altering, renaming or removal). If there is no approval, returns
     * @c cancelled. On failure @c false is returned. If @a window is @c nullptr, @c true is
     * returned immediately because there is no window to care about.
     *
     * Special case: listener for the table @a table will be silently closed without asking for
     * confirmation. It is ignored when looking for objects that are "blocking" changes
     * of @a table. This exception is needed because the listener handles the data view's lifetime
     * and the data view should be reset silently without bothering the user.
     *
     * @see KexiTablePartTempData::closeListener()
     * @see KexiQueryPart::askForClosingObjectsUsingQuerySchema()
     */
    static tristate askForClosingObjectsUsingTableSchema(KexiWindow *window, KDbConnection *conn,
                                                         KDbTableSchema *table,
                                                         const KLocalizedString &msg);

    virtual KLocalizedString i18nMessage(const QString& englishMessage,
                                         KexiWindow* window) const;

    KexiLookupColumnPage* lookupColumnPage() const;

protected:
    KexiWindowData* createWindowData(KexiWindow* window) override Q_REQUIRED_RESULT;

    KexiView *createView(QWidget *parent, KexiWindow *window, KexiPart::Item *item,
                         Kexi::ViewMode viewMode = Kexi::DataViewMode,
                         QMap<QString, QVariant> *staticObjectArgs = nullptr) override Q_REQUIRED_RESULT;

    virtual void initPartActions();
    virtual void initInstanceActions();

    virtual void setupPropertyPane(KexiPropertyPaneWidget *pane);

    virtual KDbObject* loadSchemaObject(KexiWindow *window, const KDbObject& object,
            Kexi::ViewMode viewMode, bool *ownedByWindow);

private:
    class Private;
    Private* const d;
};

#endif
