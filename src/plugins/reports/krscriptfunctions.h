/*
 * Kexi Report Plugin
 * Copyright (C) 2007-2008 by Adam Pigg (adam@piggz.co.uk)
 * Copyright (C) 2012-2018 Jarosław Staniek <staniek@kde.org>
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

#ifndef KRSCRIPTFUNCTIONS_H
#define KRSCRIPTFUNCTIONS_H

#include <KReportGroupTracker>

#include <KDbEscapedString>

#include <QMap>

class KexiDBReportDataSource;
class KDbConnection;
class KDbCursor;

/**
*/
class KRScriptFunctions : public KReportGroupTracker
{
    Q_OBJECT
public:
    KRScriptFunctions(KexiDBReportDataSource *dataSource);

    ~KRScriptFunctions();

private:
    KexiDBReportDataSource * const m_dataSource;
    QString m_source;

    //! @todo Move SQL aggregate functions to KDb
    qreal math(const QString &, const QString &);

    QMap<QString, QVariant> m_groupData;

public Q_SLOTS:
    virtual void setGroupData(const QMap<QString, QVariant> &groupData) override;

    qreal sum(const QString &);
    qreal avg(const QString &);
    qreal min(const QString &);
    qreal max(const QString &);
    qreal count(const QString &);
    QVariant value(const QString &);
};

#endif
