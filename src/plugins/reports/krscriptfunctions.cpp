/*
 * Kexi Report Plugin
 * Copyright (C) 2007-2008 by Adam Pigg <adam@piggz.co.uk>
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

#include "krscriptfunctions.h"
#include "KexiDBReportDataSource.h"

#include <KDbConnection>
#include <KDbCursor>
#include <KDbNativeStatementBuilder>

#include <QDebug>

KRScriptFunctions::KRScriptFunctions(KexiDBReportDataSource *datasource)
    : m_dataSource(datasource)
{
    Q_ASSERT(m_dataSource);
}

KRScriptFunctions::~KRScriptFunctions()
{
}

void KRScriptFunctions::setGroupData(const QMap<QString, QVariant>& groupData)
{
    m_groupData = groupData;
}

qreal KRScriptFunctions::math(const QString &function, const QString &field)
{
    return m_dataSource->runAggregateFunction(function, field, m_groupData);
}

qreal KRScriptFunctions::sum(const QString &field)
{
    return math("SUM", field);
}

qreal KRScriptFunctions::avg(const QString &field)
{
    return math("AVG", field);
}

qreal KRScriptFunctions::min(const QString &field)
{
    return math("MIN", field);
}

qreal KRScriptFunctions::max(const QString &field)
{
    return math("MAX", field);
}

qreal KRScriptFunctions::count(const QString &field)
{
    return math("COUNT", field);
}

QVariant KRScriptFunctions::value(const QString &field)
{
    const QVariant val = m_dataSource->value(field);
    if (val.type() == QVariant::String) {
        // UTF-8 values are expected so convert this
        return val.toString().toUtf8();
    }
    return val;
}
