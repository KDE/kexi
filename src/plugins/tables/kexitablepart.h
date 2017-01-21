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

#include <KDbFieldList>
#include <KDbTableSchemaChangeListener>

class KexiLookupColumnPage;

//! @short Temporary data kept in memory while switching between Table Window's views
class KexiTablePartTempData : public KexiWindowData, public KDbTableSchemaChangeListener
{
    Q_OBJECT
public:
    explicit KexiTablePartTempData(QObject* parent, KDbConnection *conn);

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

protected:
    //! Closes listener - this temp-data acts as a listener for tracking changes in table schema
    //! that is displayed in the window's data view.
    //! It just calls KexiDataTableView::setData(nullptr) is there's data set for the view
    //! (i.e. if KexiDataTableView::tableView()->data() is not @c nullptr).
    tristate closeListener() override;

private:
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

    //! Close objects that listenen to changes of the table schema @a table.
    //! Asks the user for approval if there is at least one object that listens for changes
    //! of the schema. If there is no approval, returns @c cancelled.
    //! On failure returns @c false.
    //! Special case: listener that is equal to window->data() will be silently closed
    //! without asking for confirmation. It is not counted when looking for objects that
    //! are "blocking" changes of @a table.
    //! This exception is needed because the listener handles the data view's lifetime
    //! and the data view should be reset silently without bothering the user.
    //! See KexiTablePartTempData::closeListener()
    static tristate askForClosingObjectsUsingTableSchema(
        KexiWindow *window, KDbConnection *conn,
        KDbTableSchema *table, const QString& msg);

    virtual KLocalizedString i18nMessage(const QString& englishMessage,
                                         KexiWindow* window) const;

    KexiLookupColumnPage* lookupColumnPage() const;

protected:
    virtual KexiWindowData* createWindowData(KexiWindow* window);

    virtual KexiView* createView(QWidget *parent, KexiWindow* window,
                                 KexiPart::Item *item, Kexi::ViewMode viewMode = Kexi::DataViewMode,
                                 QMap<QString, QVariant>* staticObjectArgs = 0);

    virtual void initPartActions();
    virtual void initInstanceActions();

    virtual void setupCustomPropertyPanelTabs(QTabWidget *tab);

    virtual KDbObject* loadSchemaObject(KexiWindow *window, const KDbObject& object,
            Kexi::ViewMode viewMode, bool *ownedByWindow);

private:
    class Private;
    Private* const d;
};

#endif
