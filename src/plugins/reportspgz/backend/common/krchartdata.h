/*
 * Kexi Report Plugin
 * Copyright (C) 2007-2008 by Adam Pigg (adam@piggz.co.uk)
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * Please contact info@openmfg.com with any questions on this license.
 */
#ifndef KRCHARTDATA_H
#define KRCHARTDATA_H

#include "krobjectdata.h"
#include <QRect>
#include <qdom.h>
#include "krsize.h"
#include "krpos.h"
#include <parsexmlutils.h>
#include <KDChartWidget>

namespace KexiDB
{
class Connection;
class Cursor;
class Field;
}

/**
 @author Adam Pigg <adam@piggz.co.uk>
*/

namespace Scripting
{
class Chart;
}

class KRChartData : public KRObjectData
{
public:
    KRChartData() {
        m_connection = 0; createProperties();
    }
    KRChartData(QDomNode & element);
    ~KRChartData();
    virtual KRChartData * toChart();
    virtual int type() const;
    KDChart::Widget *widget() {
        return m_chartWidget;
    }

    /**
    @brief Perform the query for the chart and set the charts data
    */
    void populateData();
    void setConnection(KexiDB::Connection*);

    /**
    @brief Set the value of a field from the master (report) data set
    This data will be used when retrieving the data for the chart
    as the values in a 'where' clause.
    */
    void setLinkData(QString, QVariant);

    /**
    @brief Return the list of master fields
    The caller will use this to set the current value for each field
    at that stage in the report
    */
    QStringList masterFields();

protected:
    KRSize m_size;
    KoProperty::Property * m_dataSource;
    KoProperty::Property * m_font;
    KoProperty::Property * m_chartType;
    KoProperty::Property * m_chartSubType;
    KoProperty::Property * m_threeD;
    KoProperty::Property * m_colorScheme;
    KoProperty::Property * m_aa;
    KoProperty::Property * m_xTitle;
    KoProperty::Property * m_yTitle;

    KoProperty::Property *m_backgroundColor;
    KoProperty::Property *m_displayLegend;

    KoProperty::Property *m_linkMaster;
    KoProperty::Property *m_linkChild;

    KDChart::Widget *m_chartWidget;

    void set3D(bool);
    void setAA(bool);
    void setColorScheme(const QString &);
    void setAxis(const QString&, const QString&);
    void setBackgroundColor(const QColor&);
    void setLegend(bool);

    QStringList fieldNames(const QString &);
    QStringList fieldNamesHackUntilImprovedParser(const QString &);

private:
    virtual void createProperties();
    static int RTTI;
    KexiDB::Connection* m_connection;

    friend class ORPreRenderPrivate;
    friend class Scripting::Chart;

    KexiDB::Cursor *dataSet();

    QMap<QString,QVariant> m_links; //Map of field->value for child/master links

};

#endif
