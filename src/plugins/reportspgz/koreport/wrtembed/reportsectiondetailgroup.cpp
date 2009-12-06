/*
 * OpenRPT report writer and rendering engine
 * Copyright (C) 2001-2007 by OpenMFG, LLC
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

#include "reportsectiondetailgroup.h"
#include <qobject.h>
#include "reportdesigner.h"
#include "reportsection.h"
#include "reportsectiondetail.h"
#include <kdebug.h>

//
// ReportSectionDetailGroup
//
ReportSectionDetailGroup::ReportSectionDetailGroup(const QString & column, ReportSectionDetail * rsd, QWidget * parent, const char * name)
        : QObject(parent)
{
    Q_UNUSED(name);
    
    m_pageBreak = BreakNone;
    ReportDesigner * rd = 0;
    m_reportSectionDetail = rsd;
    if (m_reportSectionDetail) {
        rd = rsd->reportDesigner();
    } else {
        kDebug() << "Error RSD is null";
    }
    m_groupHeader = new ReportSection(rd /*, _rsd*/);
    m_groupFooter = new ReportSection(rd /*, _rsd*/);
    setGroupHeaderVisible(false);
    setGroupFooterVisible(false);

    setColumn(column);
}

ReportSectionDetailGroup::~ReportSectionDetailGroup()
{
    // I delete these here so that there are no widgets
    //left floating around
    delete m_groupHeader;
    delete m_groupFooter;
}

void ReportSectionDetailGroup::setGroupHeaderVisible(bool yes)
{
    if (groupHeaderVisible() != yes) {
        if (m_reportSectionDetail && m_reportSectionDetail->reportDesigner()) m_reportSectionDetail->reportDesigner()->setModified(true);
    }
    if (yes) m_groupHeader->show();
    else m_groupHeader->hide();
    m_reportSectionDetail->adjustSize();
}

void ReportSectionDetailGroup::setGroupFooterVisible(bool yes)
{
    if (groupFooterVisible() != yes) {
        if (m_reportSectionDetail && m_reportSectionDetail->reportDesigner()) m_reportSectionDetail->reportDesigner()->setModified(true);
    }
    if (yes) m_groupFooter->show();
    else m_groupFooter->hide();
    m_reportSectionDetail->adjustSize();
}

void ReportSectionDetailGroup::setPageBreak(int pb)
{
    m_pageBreak = pb;
}

bool ReportSectionDetailGroup::groupHeaderVisible() const
{
    return m_groupHeader->isVisible();
}
bool ReportSectionDetailGroup::groupFooterVisible() const
{
    return m_groupFooter->isVisible();
}
int ReportSectionDetailGroup::pageBreak() const
{
    return m_pageBreak;
}

QString ReportSectionDetailGroup::column()
{
    return m_column;
}
void ReportSectionDetailGroup::setColumn(const QString & s)
{
    if (m_column != s) {
        m_column = s;
        if (m_reportSectionDetail && m_reportSectionDetail->reportDesigner()) m_reportSectionDetail->reportDesigner()->setModified(true);
    }

    m_groupHeader->setTitle(m_column + " Group Header");
    m_groupFooter->setTitle(m_column + " Group Footer");
}

ReportSection * ReportSectionDetailGroup::groupHeader() const
{
    return m_groupHeader;
}
ReportSection * ReportSectionDetailGroup::groupFooter() const
{
    return m_groupFooter;
}




