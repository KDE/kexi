/*
* Kexi Report Plugin
* Copyright (C) 2007-2009 by Adam Pigg (adam@piggz.co.uk)
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

#include <KDbCursor>
#include <KDbUtils>

#include <config-kreport.h>
#include <KReportDataSource>

/**

*/
class KexiDBReportData : public KReportDataSource
{
public:
    KexiDBReportData(const QString &objectName, KDbConnection *conn);

    /*!
     * @a pluginId specifies type of @a objectName, a table or query.
     * Types accepted:
     * -"org.kexi-project.table"
     * -"org.kexi-project.query"
     * -empty QString() - attempt to resolve @a objectName
     */
    KexiDBReportData(const QString &objectName, const QString& pluginId, KDbConnection *conn);
    virtual ~KexiDBReportData();

    virtual QStringList fieldNames() const;
    virtual void setSorting(const QList<SortedField>& sorting);

    //! Adds a condition <field> <relation> <value> to the data source.
    //! @note Only single-character relation operators such as "=" or ">" are supported now.
    //! @todo Use KDb parser to support all relation operators such as ">=".
    virtual void addCondition(const QString &field, const QVariant &value,
                              const QString &relation = QLatin1String("="));

    virtual QString sourceName() const;
    virtual int fieldNumber(const QString &field) const;
    virtual QVariant value(int) const;
    virtual QVariant value(const QString &field) const;

    virtual bool open();
    virtual bool close();
    virtual bool moveNext();
    virtual bool movePrevious();
    virtual bool moveFirst();
    virtual bool moveLast();

    virtual qint64 at() const;
    virtual qint64 recordCount() const;

    //Utility Functions
    virtual QStringList scriptList() const;
    virtual QString scriptCode(const QString& script) const;
    virtual QStringList dataSourceNames() const;
    virtual KReportDataSource* create(const QString& source) const Q_REQUIRED_RESULT;

private:
    class Private;
    Private * const d;

    bool getSchema(const QString& pluginId = QString());
};

#endif

