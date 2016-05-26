/*
 * Kexi Report Plugin
 * Copyright (C) 2007-2008 by Adam Pigg <adam@piggz.co.uk>
 * Copyright (C) 2012 Jarosław Staniek <staniek@kde.org>
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

#include <KDbConnection>
#include <KDbCursor>

#include <QDebug>

KRScriptFunctions::KRScriptFunctions(const KReportDataSource* kodata, KDbConnection* conn)
{
    m_cursor = kodata;
    m_connection = conn;

    if (kodata) {
        m_source = kodata->sourceName();
    }
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
    QString ret = QLatin1String("0.0");

    if (!m_connection) {
        return 0.0;
    }

    KDbEscapedString sql = KDbEscapedString("SELECT " + function + "(" + field + ") FROM (" + m_source + ")");

    if (!m_groupData.isEmpty()) {
        sql += " WHERE(" + where() + ')';
    }

    qDebug() << sql;

    m_connection->querySingleString(sql,&ret);

    return ret.toDouble();
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
    if (!m_cursor) {
        qDebug() << "No cursor to get value of field" << field;
        return QVariant();
    }

    QStringList fields = m_cursor->fieldNames();
    QVariant val = m_cursor->value(fields.indexOf(field));
    if (val.type() == QVariant::String) {
        // UTF-8 values are expected so convert this
        return val.toString().toUtf8();
    }

    return val;
}

KDbEscapedString KRScriptFunctions::where()
{
    QByteArray w;
    QMap<QString, QVariant>::const_iterator i = m_groupData.constBegin();
    while (i != m_groupData.constEnd()) {
        w += '(' + i.key().toUtf8() + QByteArrayLiteral(" = '") + i.value().toString().toUtf8() + QByteArrayLiteral("') AND ");
        ++i;
    }
    w.chop(4);
    //kreportDebug() << w;
    return KDbEscapedString(w);
}
