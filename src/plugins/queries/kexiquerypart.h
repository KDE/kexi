/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@kde.org>
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

#ifndef KEXIQUERYPART_H
#define KEXIQUERYPART_H

#include <QMap>

#include <kexipart.h>
#include <kexipartitem.h>
#include <KexiWindowData.h>

#include <KDbTableSchemaChangeListener>

//! @short Temporary data kept in memory while switching between Query Window's views
class KexiQueryPartTempData : public KexiWindowData,
                              public KDbTableSchemaChangeListener
{
    Q_OBJECT
public:
    KexiQueryPartTempData(KexiWindow* parent, KDbConnection *conn);
    virtual ~KexiQueryPartTempData();
    virtual tristate closeListener();
    void clearQuery();
    void unregisterForTablesSchemaChanges();
    void registerTableSchemaChanges(KDbQuerySchema *q);

    /*! Assigns query \a query for this data.
     Existing query (available using query()) is deleted but only
     if it is not owned by parent window (i.e. != KexiWindow::schemaObject()).
     \a query can be 0.
     If \a query is equal to existing query, nothing is performed.
    */
    void setQuery(KDbQuerySchema *query);

    //! \return query associated with this data
    KDbQuerySchema *query() const {
        return m_query;
    }

    //! Takes query associated with this data (without deleting) and returns it.
    //! After this call query() == 0
    KDbQuerySchema *takeQuery();

    //! Connection used for retrieving definition of the query
    KDbConnection *conn;

    /*! @return view mode if which the query member has changed.
     It's possibly one of previously visited views. Kexi::NoViewMode is the default,
     what means that query was not changed.
     Used on view switching. We're checking this flag to see if we should
     rebuild internal structure for DesignViewMode of regenerated sql text
     in TextViewMode after switch from other view. */
    Kexi::ViewMode queryChangedInView() const;

    /*! Sets the queryChangedInView flag. If @a set is true, then the flag is changed
     to the current view mode. If @a set is false, the flag is changed to Kexi::NoViewMode.
     @see queryChangedInView() */
    void setQueryChangedInView(bool set);

private:
    KDbQuerySchema *m_query;
    Kexi::ViewMode m_queryChangedInView;
};

//! @short Kexi Query Designer plugin
class KexiQueryPart : public KexiPart::Part
{
    Q_OBJECT

public:
    KexiQueryPart(QObject *parent, const QVariantList &);
    virtual ~KexiQueryPart();

    virtual tristate remove(KexiPart::Item *item);

    //! Implemented for KexiPart::Part.
    virtual KDbQuerySchema* currentQuery(KexiView* view);

    virtual KLocalizedString i18nMessage(const QString& englishMessage,
                                         KexiWindow* window) const;

    /*! Renames stored data pointed by \a item to \a newName.
     Reimplemented to mark the query obsolete by using KDbConnection::setQuerySchemaObsolete(). */
    virtual tristate rename(KexiPart::Item *item, const QString& newName);

protected:
    KexiWindowData* createWindowData(KexiWindow* window) override Q_REQUIRED_RESULT;

    KexiView *createView(QWidget *parent, KexiWindow *window, KexiPart::Item *item,
                         Kexi::ViewMode viewMode = Kexi::DataViewMode,
                         QMap<QString, QVariant> *staticObjectArgs = nullptr) override Q_REQUIRED_RESULT;

    virtual void initPartActions();
    virtual void initInstanceActions();

    virtual KDbObject* loadSchemaObject(KexiWindow *window,
            const KDbObject& object, Kexi::ViewMode viewMode, bool *ownedByWindow);
};

#endif
