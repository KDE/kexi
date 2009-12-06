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

#include "sectioneditor.h"
#include "reportdesigner.h"
#include "reportsection.h"
#include "reportsectiondetail.h"
#include "detailgroupsectiondialog.h"
#include "reportsectiondetailgroup.h"

#include <QVariant>
#include <QComboBox>
#include <QLineEdit>
#include <QMessageBox>


/*
 *  Constructs a SectionEditor as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
SectionEditor::SectionEditor(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
        : QDialog(parent, fl)
{
    Q_UNUSED(name);
    Q_UNUSED(modal);
    
    setupUi(this);


    // signals and slots connections
    connect(buttonOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(cbReportHeader, SIGNAL(toggled(bool)), this, SLOT(cbReportHeader_toggled(bool)));
    connect(cbReportFooter, SIGNAL(toggled(bool)), this, SLOT(cbReportFooter_toggled(bool)));
    connect(cbHeadFirst, SIGNAL(toggled(bool)), this, SLOT(cbHeadFirst_toggled(bool)));
    connect(cbHeadLast, SIGNAL(toggled(bool)), this, SLOT(cbHeadLast_toggled(bool)));
    connect(cbHeadEven, SIGNAL(toggled(bool)), this, SLOT(cbHeadEven_toggled(bool)));
    connect(cbHeadOdd, SIGNAL(toggled(bool)), this, SLOT(cbHeadOdd_toggled(bool)));
    connect(cbFootFirst, SIGNAL(toggled(bool)), this, SLOT(cbFootFirst_toggled(bool)));
    connect(cbFootLast, SIGNAL(toggled(bool)), this, SLOT(cbFootLast_toggled(bool)));
    connect(cbFootEven, SIGNAL(toggled(bool)), this, SLOT(cbFootEven_toggled(bool)));
    connect(cbFootOdd, SIGNAL(toggled(bool)), this, SLOT(cbFootOdd_toggled(bool)));
    connect(cbHeadAny, SIGNAL(toggled(bool)), this, SLOT(cbHeadAny_toggled(bool)));
    connect(cbFootAny, SIGNAL(toggled(bool)), this, SLOT(cbFootAny_toggled(bool)));

    connect(btnAdd, SIGNAL(clicked()), this, SLOT(btnAdd_clicked()));
    connect(btnEdit, SIGNAL(clicked()), this, SLOT(btnEdit_clicked()));
    connect(btnRemove, SIGNAL(clicked()), this, SLOT(btnRemove_clicked()));
    connect(btnMoveUp, SIGNAL(clicked()), this, SLOT(btnMoveUp_clicked()));
    connect(brnMoveDown, SIGNAL(clicked()), this, SLOT(brnMoveDown_clicked()));
}

/*
 *  Destroys the object and frees any allocated resources
 */
SectionEditor::~SectionEditor()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void SectionEditor::languageChange()
{
    retranslateUi(this);
}

void SectionEditor::cbReportHeader_toggled(bool yes)
{
    if (m_reportDesigner != NULL) {
        if (yes) {
            m_reportDesigner->insertSection(KRSectionData::ReportHeader);
        } else {
            m_reportDesigner->removeSection(KRSectionData::ReportHeader);
        }
    }

}

void SectionEditor::cbReportFooter_toggled(bool yes)
{
    if (m_reportDesigner != NULL) {
        if (yes) {
            m_reportDesigner->insertSection(KRSectionData::ReportFooter);
        } else {
            m_reportDesigner->removeSection(KRSectionData::ReportFooter);
        }
    }

}

void SectionEditor::cbHeadFirst_toggled(bool yes)
{
    if (m_reportDesigner != NULL) {
        if (yes) {
            m_reportDesigner->insertSection(KRSectionData::PageHeaderFirst);
        } else {
            m_reportDesigner->removeSection(KRSectionData::PageHeaderFirst);
        }
    }

}

void SectionEditor::cbHeadLast_toggled(bool yes)
{
    if (m_reportDesigner != NULL) {
        if (yes) {
            m_reportDesigner->insertSection(KRSectionData::PageHeaderLast);
        } else {
            m_reportDesigner->removeSection(KRSectionData::PageHeaderLast);
        }
    }

}

void SectionEditor::cbHeadEven_toggled(bool yes)
{
    if (m_reportDesigner != NULL) {
        if (yes) {
            m_reportDesigner->insertSection(KRSectionData::PageHeaderEven);
        } else {
            m_reportDesigner->removeSection(KRSectionData::PageHeaderEven);
        }
    }

}

void SectionEditor::cbHeadOdd_toggled(bool yes)
{
    if (m_reportDesigner != NULL) {
        if (yes) {
            m_reportDesigner->insertSection(KRSectionData::PageHeaderOdd);
        } else {
            m_reportDesigner->removeSection(KRSectionData::PageHeaderOdd);
        }
    }

}

void SectionEditor::cbFootFirst_toggled(bool yes)
{
    if (m_reportDesigner != NULL) {
        if (yes) {
            m_reportDesigner->insertSection(KRSectionData::PageFooterFirst);
        } else {
            m_reportDesigner->removeSection(KRSectionData::PageFooterFirst);
        }
    }

}

void SectionEditor::cbFootLast_toggled(bool yes)
{
    if (m_reportDesigner != NULL) {
        if (yes) {
            m_reportDesigner->insertSection(KRSectionData::PageFooterLast);
        } else {
            m_reportDesigner->removeSection(KRSectionData::PageFooterLast);
        }
    }

}

void SectionEditor::cbFootEven_toggled(bool yes)
{
    if (m_reportDesigner != NULL) {
        if (yes) {
            m_reportDesigner->insertSection(KRSectionData::PageFooterEven);
        } else {
            m_reportDesigner->removeSection(KRSectionData::PageFooterEven);
        }
    }

}

void SectionEditor::cbFootOdd_toggled(bool yes)
{
    if (m_reportDesigner != NULL) {
        if (yes) {
            m_reportDesigner->insertSection(KRSectionData::PageFooterOdd);
        } else {
            m_reportDesigner->removeSection(KRSectionData::PageFooterOdd);
        }
    }

}


void SectionEditor::init(ReportDesigner * rw)
{
    this->m_reportDesigner = NULL;
    // set all the properties

    cbReportHeader->setChecked(rw->section(KRSectionData::ReportHeader) != NULL);
    cbReportFooter->setChecked(rw->section(KRSectionData::ReportFooter) != NULL);

    cbHeadFirst->setChecked(rw->section(KRSectionData::PageHeaderFirst) != NULL);
    cbHeadOdd->setChecked(rw->section(KRSectionData::PageHeaderOdd) != NULL);
    cbHeadEven->setChecked(rw->section(KRSectionData::PageHeaderEven) != NULL);
    cbHeadLast->setChecked(rw->section(KRSectionData::PageHeaderLast) != NULL);
    cbHeadAny->setChecked(rw->section(KRSectionData::PageHeaderAny) != NULL);

    cbFootFirst->setChecked(rw->section(KRSectionData::PageFooterFirst) != NULL);
    cbFootOdd->setChecked(rw->section(KRSectionData::PageFooterOdd) != NULL);
    cbFootEven->setChecked(rw->section(KRSectionData::PageFooterEven) != NULL);
    cbFootLast->setChecked(rw->section(KRSectionData::PageFooterLast) != NULL);
    cbFootAny->setChecked(rw->section(KRSectionData::PageFooterAny) != NULL);

    // now set the rw value
    this->m_reportDesigner = rw;
    m_reportSectionDetail = rw->detailSection();

    if (m_reportSectionDetail) {
        for (int i = 0; i < m_reportSectionDetail->groupSectionCount(); i++) {
            lbGroups->insertItem(m_reportSectionDetail->groupSection(i)->column());
        }
    }
}

void SectionEditor::cbHeadAny_toggled(bool yes)
{
    if (m_reportDesigner != NULL) {
        if (yes) {
            m_reportDesigner->insertSection(KRSectionData::PageHeaderAny);
        } else {
            m_reportDesigner->removeSection(KRSectionData::PageHeaderAny);
        }
    }
}

void SectionEditor::cbFootAny_toggled(bool yes)
{
    if (m_reportDesigner != NULL) {
        if (yes) {
            m_reportDesigner->insertSection(KRSectionData::PageFooterAny);
        } else {
            m_reportDesigner->removeSection(KRSectionData::PageFooterAny);
        }
    }
}

void SectionEditor::btnEdit_clicked()
{
    if (m_reportSectionDetail) {
        int idx = lbGroups->currentItem();
        if (idx < 0) return;
        ReportSectionDetailGroup * rsdg = m_reportSectionDetail->groupSection(idx);
        DetailGroupSectionDialog * dgsd = new DetailGroupSectionDialog(this);

        dgsd->cbColumn->clear();
        dgsd->cbColumn->insertItems(0, m_reportDesigner->fieldList());
        dgsd->cbColumn->setEditText(rsdg->column());

        dgsd->breakAfterFooter->setChecked(rsdg->pageBreak() == ReportSectionDetailGroup::BreakAfterGroupFooter);
        dgsd->cbHead->setChecked(rsdg->groupHeaderVisible());
        dgsd->cbFoot->setChecked(rsdg->groupFooterVisible());

        bool exitLoop = false;
        while (!exitLoop) {
            if (dgsd->exec() == QDialog::Accepted) {
                QString column = dgsd->cbColumn->currentText();
                bool showgh = dgsd->cbHead->isChecked();
                bool showgf = dgsd->cbFoot->isChecked();
                bool breakafterfoot = dgsd->breakAfterFooter->isChecked();

                if (column != rsdg->column() && m_reportSectionDetail->indexOfSection(column) != -1) {
                    QMessageBox::warning(this, i18n("Error Encountered"),
                                         i18n("Unable to add a new group because its name would not be unique"));
                } else {

                    lbGroups->changeItem(column, idx);

                    rsdg->setColumn(column);
                    rsdg->setGroupHeaderVisible(showgh);
                    rsdg->setGroupFooterVisible(showgf);
                    if (breakafterfoot)
                        rsdg->setPageBreak(ReportSectionDetailGroup::BreakAfterGroupFooter);
                    else
                        rsdg->setPageBreak(ReportSectionDetailGroup::BreakNone);

                    exitLoop = true;
                }
            } else {
                exitLoop = true;
            }
        }

        if (dgsd) delete dgsd;
    }
}

void SectionEditor::btnAdd_clicked()
{
    if (m_reportSectionDetail) {
        // lets add a new section
        QString name;
        int i = 0;
        while (i < 100 && m_reportSectionDetail->indexOfSection(name) != -1) {
            i++;
            name = QString().sprintf("unnamed%d", i);
        }
        if (m_reportSectionDetail->indexOfSection(name) != -1) {
            QMessageBox::warning(
                this, i18n("Error Encountered"),
                i18n("Unable to add a new group because its name would not be unique"));
            return;
        }
        ReportSectionDetailGroup * rsdg = new ReportSectionDetailGroup(name, m_reportSectionDetail, m_reportSectionDetail);
        m_reportSectionDetail->insertSection(m_reportSectionDetail->groupSectionCount(), rsdg);
        lbGroups->insertItem(name);
        lbGroups->setCurrentItem(lbGroups->count() - 1);
        btnEdit_clicked();
    }
}


void SectionEditor::btnRemove_clicked()
{
    if (m_reportSectionDetail) {
        int idx = lbGroups->currentItem();
        if (idx != -1) {
            lbGroups->removeItem(idx);
            m_reportSectionDetail->removeSection(idx, true);
        }
    }
}


void SectionEditor::btnMoveUp_clicked()
{
    if (m_reportSectionDetail) {
        int idx = lbGroups->currentItem();
        if (idx <= 0) return;
        QString s = lbGroups->currentText();
        lbGroups->removeItem(idx);
        lbGroups->insertItem(s, idx - 1);
        ReportSectionDetailGroup * rsdg = m_reportSectionDetail->groupSection(idx);
        bool showgh = rsdg->groupHeaderVisible();
        bool showgf = rsdg->groupFooterVisible();
        m_reportSectionDetail->removeSection(idx);
        m_reportSectionDetail->insertSection(idx - 1, rsdg);
        rsdg->setGroupHeaderVisible(showgh);
        rsdg->setGroupFooterVisible(showgf);
    }
}


void SectionEditor::brnMoveDown_clicked()
{
    if (m_reportSectionDetail) {
        int idx = lbGroups->currentItem();
        if (idx == (int)(lbGroups->count() - 1)) return;
        QString s = lbGroups->currentText();
        lbGroups->removeItem(idx);
        lbGroups->insertItem(s, idx + 1);
        ReportSectionDetailGroup * rsdg = m_reportSectionDetail->groupSection(idx);
        bool showgh = rsdg->groupHeaderVisible();
        bool showgf = rsdg->groupFooterVisible();
        m_reportSectionDetail->removeSection(idx);
        m_reportSectionDetail->insertSection(idx + 1, rsdg);
        rsdg->setGroupHeaderVisible(showgh);
        rsdg->setGroupFooterVisible(showgf);
    }
}
