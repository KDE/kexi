/*
* Kexi Report Plugin
* Copyright (C) 2007-2017 by Adam Pigg <adam@piggz.co.uk>
* Copyright (C) 2017-2018 Jarosław Staniek <staniek@kde.org>
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

#ifndef __KEXIDBREPORTDATA_H__
#define __KEXIDBREPORTDATA_H__

#include <QString>
#include <QStringList>

#include <KReportDataSource>

class KexiReportPartTempData;

//! @brief Implementation of database report data source
class KexiDBReportDataSource : public KReportDataSource
{
public:
    /*!
     * @a pluginId specifies type of @a objectName, a table or query.
     * Types accepted:
     * -"org.kexi-project.table"
     * -"org.kexi-project.query"
     * -empty QString() - attempt to resolve @a objectName
     */
    KexiDBReportDataSource(const QString &objectName, const QString &pluginId,
                           KexiReportPartTempData *data);
    virtual ~KexiDBReportDataSource();

    virtual QStringList fieldNames() const override;
    virtual void setSorting(const QList<SortedField>& sorting) override;

    //! Adds a condition <field> <relation> <value> to the data source.
    //! @note Only single-character relation operators such as "=" or ">" are supported now.
    //! @todo Use KDb parser to support all relation operators such as ">=".
    virtual void addCondition(const QString &field, const QVariant &value,
                              const QString &relation = QLatin1String("=")) override;

    virtual QString sourceName() const override;
    virtual int fieldNumber(const QString &field) const override;
    virtual QVariant value(int) const override;
    virtual QVariant value(const QString &field) const override;

    virtual bool open() override;
    virtual bool close() override;
    virtual bool moveNext() override;
    virtual bool movePrevious() override;
    virtual bool moveFirst() override;
    virtual bool moveLast() override;

    virtual qint64 at() const override;
    virtual qint64 recordCount() const override;

    /**
     * Runs aggregate function @a function on the data source
     *
     * @param function name such as max, min, avg
     * @param field name of field for which the aggregation should be executed
     * @param conditions optional conditions that limit the record set
     * @return value of the function, 0.0 on failure
     *
     * @warning SQL injection warning: validity of @a function name is not checked, this should not
     *          be part of a public API.
     * @todo Move SQL aggregate functions to KDb. Current code depends on support for subqueries.
     */
    double runAggregateFunction(const QString &function, const QString &field,
                                const QMap<QString, QVariant> &conditions);

    //Utility Functions
    virtual QStringList dataSourceNames() const override;
    virtual Q_REQUIRED_RESULT KReportDataSource *create(const QString &source) const override;

private:
    class Private;
    Private * const d;

    bool getSchema(const QString& pluginId);
};

#endif

