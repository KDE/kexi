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

#ifndef KEXIQUERYDESIGNERSQL_H
#define KEXIQUERYDESIGNERSQL_H

#include "kexiquerypart.h"

#include <KexiView.h>

class KexiQueryDesignerSqlEditor;

//! The KexiQueryDesignerSqlView class for editing Queries in text mode.
/*! It is a view containing SQL text editor
 and SQL status widget split vertically. */
class KexiQueryDesignerSqlView : public KexiView
{
    Q_OBJECT

public:
    explicit KexiQueryDesignerSqlView(QWidget *parent);
    virtual ~KexiQueryDesignerSqlView();

    QString sqlText() const;
    KexiQueryDesignerSqlEditor *editor() const;

protected:
    KexiQueryPartTempData * tempData() const;

    virtual tristate beforeSwitchTo(Kexi::ViewMode mode, bool *dontStore) override;
    virtual tristate afterSwitchFrom(Kexi::ViewMode mode) override;
    virtual KDbObject* storeNewData(const KDbObject& object,
                                             KexiView::StoreNewDataOptions options,
                                             bool *cancel) override;
    virtual tristate storeData(bool dontAsk = false) override;

    void setStatusOk();
    void setStatusError(const QString& msg);
    void setStatusEmpty();
    void setStatusText(const QString& text);

    virtual void updateActions(bool activated) override;

protected Q_SLOTS:
    /*! Performs query checking (by text parsing). \return true and sets d->parsedQuery
     to the new query schema object on success. */
    bool slotCheckQuery();
    void slotTextChanged();

Q_SIGNALS:
    void queryShortcut();

private:
    class Private;
    Private * const d;

    friend class KexiQueryView; // for storeNewData() and storeData() only
};

#endif
