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
#include "reportsectiondetail.h"
#include "reportsectiondetailgroup.h"
#include "reportsection.h"
#include <qdom.h>
#include <kdebug.h>

//
// ReportSectionDetail
//
ReportSectionDetail::ReportSectionDetail(ReportDesigner * rptdes, const char * name)
        : QWidget(rptdes)
{
    Q_UNUSED(name);

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_pageBreak = BreakNone;
    m_vboxlayout = new QVBoxLayout(this);
    m_vboxlayout->setSpacing(0);
    m_vboxlayout->setMargin(0);
    m_reportDesigner = rptdes;
    m_detail = new ReportSection(rptdes /*, this*/);
    m_vboxlayout->addWidget(m_detail);

    this->setLayout(m_vboxlayout);
}
ReportSectionDetail::~ReportSectionDetail()
{
    // Qt should be handling everything for us
    m_reportDesigner = 0;
}

int ReportSectionDetail::pageBreak() const
{
    return m_pageBreak;
}
void ReportSectionDetail::setPageBreak(int pb)
{
    m_pageBreak = pb;
}

ReportSection * ReportSectionDetail::detailSection() const
{
    return m_detail;
}

void ReportSectionDetail::buildXML(QDomDocument & doc, QDomElement & section)
{
    if (pageBreak() != ReportSectionDetail::BreakNone) {
        QDomElement spagebreak = doc.createElement("pagebreak");
        if (pageBreak() == ReportSectionDetail::BreakAtEnd)
            spagebreak.setAttribute("when", "at end");
        section.appendChild(spagebreak);
    }

    for (uint i = 0; i < (uint)groupList.count(); i++) {
        ReportSectionDetailGroup * rsdg = groupList.at(i);
        QDomElement grp = doc.createElement("report:group");

        grp.setAttribute("report:group-column", rsdg->column());
        if (rsdg->pageBreak() == ReportSectionDetailGroup::BreakAfterGroupFooter) {
            grp.setAttribute("report:group-page-break", "after-footer");
        } else if (rsdg->pageBreak() == ReportSectionDetailGroup::BreakBeforeGroupHeader) {
            grp.setAttribute("report:group-page-break", "before-header");
        }

        //group head
        if (rsdg->groupHeaderVisible()) {
            QDomElement gheader = doc.createElement("report:section");
            gheader.setAttribute("report:section-type", "group-header");
            rsdg->groupHeader()->buildXML(doc, gheader);
            grp.appendChild(gheader);
        }
        // group foot
        if (rsdg->groupFooterVisible()) {
            QDomElement gfooter = doc.createElement("report:section");
            gfooter.setAttribute("report:section-type", "group-footer");
            rsdg->groupHeader()->buildXML(doc, gfooter);
            grp.appendChild(gfooter);
        }

        section.appendChild(grp);
    }

    // detail section
    QDomElement gdetail = doc.createElement("report:section");
    gdetail.setAttribute("report:section-type", "detail");
    m_detail->buildXML(doc, gdetail);
    section.appendChild(gdetail);
}

void ReportSectionDetail::initFromXML(QDomNode & section)
{
    QDomNodeList nl = section.childNodes();
    QDomNode node;
    QString n;

    for (int i = 0; i < nl.count(); i++) {
        node = nl.item(i);
        n = node.nodeName();

        kDebug() << n;

        if (n == "pagebreak") {
            QDomElement eThis = node.toElement();
            if (eThis.attribute("when") == "at end")
                setPageBreak(BreakAtEnd);
        } else if (n == "report:group") {
            ReportSectionDetailGroup * rsdg = new ReportSectionDetailGroup("unnamed", this, this);
            QDomNodeList gnl = node.childNodes();
            QDomNode gnode;
            bool show_head = false;
            bool show_foot = false;
            for (int gi = 0; gi < gnl.count(); gi++) {
                gnode = gnl.item(gi);


                if (gnode.nodeName() == "column") {
                    rsdg->setColumn(gnode.firstChild().nodeValue());
                } else if (gnode.nodeName() == "pagebreak") {
                    QDomElement elemThis = gnode.toElement();
                    QString n = elemThis.attribute("when");
                    if ("after foot" == n)
                        rsdg->setPageBreak(ReportSectionDetailGroup::BreakAfterGroupFooter);
                } else if (gnode.nodeName() == "groupheader") {
                    rsdg->groupHeader()->initFromXML(gnode);
                    rsdg->setGroupHeaderVisible(true);
                    show_head = true;
                } else if (gnode.nodeName() == "groupfooter") {
                    rsdg->groupFooter()->initFromXML(gnode);
                    rsdg->setGroupFooterVisible(true);
                    show_foot = true;
                } else {
                    kDebug() << "encountered unknown element while parsing group element: " << gnode.nodeName();
                }
            }
            insertSection(groupSectionCount(), rsdg);
            rsdg->setGroupHeaderVisible(show_head);
            rsdg->setGroupFooterVisible(show_foot);
        } else if (n == "report:section" && node.toElement().attribute("report:section-type") == "detail") {
            kDebug() << "Creating detail section";
            m_detail->initFromXML(node);
        } else {
            // unknown element
            kDebug() << "while parsing section encountered and unknown element: " <<  n;
        }
    }

}

ReportDesigner * ReportSectionDetail::reportDesigner() const
{
    return m_reportDesigner;
}

int ReportSectionDetail::groupSectionCount() const
{
    return groupList.count();
}

ReportSectionDetailGroup * ReportSectionDetail::groupSection(int i) const
{
    return groupList.at(i);
}

void ReportSectionDetail::insertSection(int idx, ReportSectionDetailGroup * rsd)
{
    groupList.insert(idx, rsd);

    rsd->groupHeader()->setParent(this);
    rsd->groupFooter()->setParent(this);

    idx = 0;
    int gi = 0;
    for (gi = 0; gi < (int) groupList.count(); gi++) {
        rsd = groupList.at(gi);
        m_vboxlayout->removeWidget(rsd->groupHeader());
        m_vboxlayout->insertWidget(idx, rsd->groupHeader());
        idx++;
    }
    m_vboxlayout->removeWidget(m_detail);
    m_vboxlayout->insertWidget(idx, m_detail);
    idx++;
    for (gi = ((int) groupList.count() - 1); gi >= 0; gi--) {
        rsd = groupList.at(gi);
        m_vboxlayout->removeWidget(rsd->groupFooter());
        m_vboxlayout->insertWidget(idx, rsd->groupFooter());
        idx++;
    }

    if (m_reportDesigner) m_reportDesigner->setModified(true);
    adjustSize();
}

int ReportSectionDetail::indexOfSection(const QString & name) const
{
    // find the item by its name
    ReportSectionDetailGroup * rsd = 0;
    for (uint i = 0; i < (uint)groupList.count(); i++) {
        rsd = groupList.at(i);
        if (name == rsd->column()) return i;
    }
    return -1;
}

void ReportSectionDetail::removeSection(int idx, bool del)
{
    ReportSectionDetailGroup * rsd = groupList.at(idx);

    m_vboxlayout->removeWidget(rsd->groupHeader());
    m_vboxlayout->removeWidget(rsd->groupFooter());

    groupList.removeAt(idx);

    if (m_reportDesigner) m_reportDesigner->setModified(TRUE);
    if (del) delete rsd;
    adjustSize();
}

QSize ReportSectionDetail::sizeHint() const
{
    QSize s;
    ReportSectionDetailGroup * rsdg = 0;
    for (int gi = 0; gi < (int) groupList.count(); gi++) {
        rsdg = groupList.at(gi);
        if (rsdg->groupHeaderVisible()) s += rsdg->groupHeader()->size();
        if (rsdg->groupFooterVisible()) s += rsdg->groupFooter()->size();
    }
    return s += m_detail->size();
}
