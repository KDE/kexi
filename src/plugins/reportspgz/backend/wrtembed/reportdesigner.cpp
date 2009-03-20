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

#include "reportdesigner.h"
#include "reportsection.h"
#include "reportentities.h"
#include "reportscene.h"
#include "reportsceneview.h"
#include "reportentities.h"
#include "reportentitylabel.h"
#include "reportentityfield.h"
#include "reportentitytext.h"
#include "reportentityline.h"
#include "reportentitybarcode.h"
#include "reportentityimage.h"
#include "reportentitychart.h"
#include "reportentityshape.h"
#include "reportentitycheck.h"
#include "reportsectiondetailgroup.h"

// dialogs
#include "sectioneditor.h"
#include "reportsectiondetail.h"

// qt
//#include <qpixmap.h>
#include <qlayout.h>
#include <qdom.h>
#include <qtextstream.h>
#include <qiodevice.h>
#include <qfile.h>
#include <QFileDialog>

#include <QCloseEvent>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <qregexp.h>
#include <qmessagebox.h>
#include <qsqlerror.h>
#include <qlineedit.h>
#include <qspinbox.h>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QSplitter>

#include <kexidb/connection.h>
#include <kexidb/utils.h>
#include <kexidb/schemadata.h>
#include <kexiutils/tristate.h>
#include <kexipart.h>

#include <koproperty/EditorView.h>
#include <KoRuler.h>
#include <KoZoomHandler.h>
#include <KoGlobal.h>
#include <KoPageFormat.h>
#include <kaction.h>
#include <kdebug.h>
#include <kexisectionheader.h>
#include <kexieditor.h>
#include <kross/core/manager.h>

//
// define and implement the ReportWriterSectionData class
// a simple class to hold/hide data in the ReportHandler class
//
class ReportWriterSectionData
{
public:
    ReportWriterSectionData() {
        selected_items_rw = 0;
        mouseAction = ReportWriterSectionData::MA_None;
        insertItem = KRObjectData::EntityNone;
    }
    virtual ~ReportWriterSectionData() {
        selected_items_rw = 0;
    }

    enum MouseAction {
        MA_None = 0,
        MA_Insert = 1,
        MA_Grab = 2,
        MA_MoveStartPoint,
        MA_MoveEndPoint,
        MA_ResizeNW = 8,
        MA_ResizeN,
        MA_ResizeNE,
        MA_ResizeE,
        MA_ResizeSE,
        MA_ResizeS,
        MA_ResizeSW,
        MA_ResizeW
    };

    int selected_x_offset;
    int selected_y_offset;

    ReportWindow * selected_items_rw;

    MouseAction mouseAction;
    KRObjectData::EntityTypes insertItem;

    // copy data
    int copy_x_pos;        // the base x position of the copy (typically the first items original pos)
    int copy_y_pos;        // the base y position of the copy (typically the first items original pos)
    QList<ReportEntity*> copy_list;
    QList<ReportEntity*> cut_list;
};

//! @internal
class ReportDesigner::Private
{
public:
    QGridLayout *grid;
    QSplitter *splitter;
    QGraphicsScene *activeScene;
    KoRuler *hruler;
    KoZoomHandler *zoom;
    QVBoxLayout *vboxlayout;
    KexiSectionHeader *head;
    QPushButton *pageButton;
};

ReportDesigner::ReportDesigner(QWidget * parent, KexiDB::Connection *cn)
        : QWidget(parent), d(new Private())
{
    m_conn = cn;
    init();
}
void ReportDesigner::init()
{
    m_modified = false;
    detail = 0;
    d->hruler = 0;

    m_sectionData = new ReportWriterSectionData();
    createProperties();

    //setSizePolicy ( QSizePolicy::Expanding, QSizePolicy::Expanding );

    reportHead = reportFoot = 0;
    pageHeadFirst = pageHeadOdd = pageHeadEven = pageHeadLast = pageHeadAny = 0;
    pageFootFirst = pageFootOdd = pageFootEven = pageFootLast = pageFootAny = 0;


    d->grid = new QGridLayout(this);
    d->grid->setSpacing(0);
    d->grid->setMargin(0);
    d->grid->setColumnStretch(1, 1);
    d->grid->setRowStretch(1, 1);
    d->grid->setSizeConstraint(QLayout::SetFixedSize);

    d->vboxlayout = new QVBoxLayout();
    d->vboxlayout->setSpacing(0);
    d->vboxlayout->setMargin(0);
    d->vboxlayout->setSizeConstraint(QLayout::SetFixedSize);

    //Create nice rulers
    d->zoom = new KoZoomHandler();
    d->hruler = new KoRuler(this, Qt::Horizontal, d->zoom);

    d->pageButton = new QPushButton(this);

    //Messy, but i cant find another way
    delete d->hruler->tabChooser();
    d->hruler->setUnit(KoUnit(KoUnit::Centimeter));

    d->grid->addWidget(d->pageButton, 0, 0);
    d->grid->addWidget(d->hruler, 0, 1);
    d->grid->addLayout(d->vboxlayout, 1, 0, 1, 2);

    d->pageButton->setMaximumSize(QSize(22, 22));
    d->pageButton->setMinimumSize(QSize(22, 22));

    detail = new ReportSectionDetail(this);
    d->vboxlayout->insertWidget(0, detail);

    setLayout(d->grid);

    connect(d->pageButton, SIGNAL(pressed()), this, SLOT(slotPageButton_Pressed()));
    emit pagePropertyChanged(*m_set);

    connect(m_set, SIGNAL(propertyChanged(KoProperty::Set &, KoProperty::Property &)), this, SLOT(slotPropertyChanged(KoProperty::Set &, KoProperty::Property &)));

    changeSet(m_set);
}

ReportDesigner::ReportDesigner(QWidget *parent, KexiDB::Connection *cn, const QString & d) : QWidget(parent), d(new Private())
{
    kDebug() << "***********************************************************";
    kDebug() << d;
    kDebug() << "***********************************************************";

    m_conn = cn;
    init();
    QDomDocument doc;
    doc.setContent(d);
    QDomElement root = doc.documentElement();
    if (root.tagName() != "report") {
        // arg we got an xml file but not one i know of
        kDebug() << "root element was not <report>";;
    }

    deleteDetail();

    QDomNodeList nlist = root.childNodes();
    QDomNode it;

    for (int i = 0; i < nlist.count(); i++) {
        it = nlist.item(i);
        // at this level all the children we get should be Elements
        if (it.isElement()) {
            QString n = it.nodeName();
            if (n == "title") {
                setReportTitle(it.firstChild().nodeValue());
            } else if (n == "datasource") {
                setReportDataSource(it.firstChild().nodeValue());
            } else if (n == "script") {
                m_interpreter->setValue(it.toElement().attribute("interpreter"));
                m_script->setValue(it.firstChild().nodeValue());
            } else if (n == "grid") {
                setGridOptions(it.toElement().attribute("visible").toInt() == 0 ? false : true, it.toElement().attribute("divisions").toInt());
            }

            //TODO Load page options
            else if (n == "size") {
                if (it.firstChild().isText()) {
                    propertySet()->property("PageSize").setValue(it.firstChild().nodeValue());
                } else {
                    //bad code! bad code!
                    // this code doesn't check the elements and assumes they are what they should be.
                    QDomNode n1 = it.firstChild();
                    QDomNode n2 = n1.nextSibling();
                    if (n1.nodeName() == "width") {
                        propertySet()->property("CustomWidth").setValue(n1.firstChild().nodeValue().toDouble());

                        propertySet()->property("CustomHeight").setValue(n2.firstChild().nodeValue().toDouble());
                    } else {
                        propertySet()->property("CustomHeight").setValue(n1.firstChild().nodeValue().toDouble());

                        propertySet()->property("CustomWidth").setValue(n2.firstChild().nodeValue().toDouble());
                    }
                    propertySet()->property("PageSize").setValue("Custom");
                }
            } else if (n == "labeltype") {
                //TODO Labels
                //rd->pageOptions->setLabelType ( it.firstChild().nodeValue() );
            } else if (n == "portrait") {
                propertySet()->property("Orientation").setValue("Portrait");
            } else if (n == "landscape") {
                propertySet()->property("Orientation").setValue("Landscape");
            } else if (n == "topmargin") {
                propertySet()->property("TopMargin").setValue(it.firstChild().nodeValue().toDouble());
            } else if (n == "bottommargin") {
                propertySet()->property("BottomMargin").setValue(it.firstChild().nodeValue().toDouble());
            } else if (n == "leftmargin") {
                propertySet()->property("LeftMargin").setValue(it.firstChild().nodeValue().toDouble());
            } else if (n == "rightmargin") {
                propertySet()->property("RightMargin").setValue(it.firstChild().nodeValue().toDouble());
            } else if (n == "rpthead") {
                if (getSection(KRSectionData::ReportHead) == 0) {
                    insertSection(KRSectionData::ReportHead);
                    getSection(KRSectionData::ReportHead)->initFromXML(it);
                } else {
                    kDebug() << "While loading xml tried to add more than one rpthead";
                }
            } else if (n == "rptfoot") {
                if (getSection(KRSectionData::ReportFoot) == 0) {
                    insertSection(KRSectionData::ReportFoot);
                    getSection(KRSectionData::ReportFoot)->initFromXML(it);
                } else {
                    kDebug() << "While loading xml tried to add more than one rpthead";
                }
            } else if (n == "pghead") {
                // we need to determine which page this is for
                // firstpage | odd | even | lastpage
                // or any if none was specified
                ReportSection * rs = 0;
                if (!it.namedItem("firstpage").isNull()) {
                    if (getSection(KRSectionData::PageHeadFirst) == 0) {
                        insertSection(KRSectionData::PageHeadFirst);
                        rs = getSection(KRSectionData::PageHeadFirst);
                    } else {
                        kDebug() << "tried to load more than one page head first";
                    }
                } else if (!it.namedItem("odd").isNull()) {
                    if (getSection(KRSectionData::PageHeadOdd) == 0) {
                        insertSection(KRSectionData::PageHeadOdd);
                        rs = getSection(KRSectionData::PageHeadOdd);
                    } else {
                        kDebug() << "tried to load more than one page head odd";
                    }
                } else if (!it.namedItem("even").isNull()) {
                    if (getSection(KRSectionData::PageHeadEven) == 0) {
                        insertSection(KRSectionData::PageHeadEven);
                        rs = getSection(KRSectionData::PageHeadEven);
                    } else {
                        kDebug() << "tried to load more than one page head even";
                    }
                } else if (!it.namedItem("lastpage").isNull()) {
                    if (getSection(KRSectionData::PageHeadLast) == 0) {
                        insertSection(KRSectionData::PageHeadLast);
                        rs = getSection(KRSectionData::PageHeadLast);
                    } else {
                        kDebug() << "tried to load more than one page head last";
                    }
                } else {
                    // we have an any pghead
                    if (getSection(KRSectionData::PageHeadAny) == 0) {
                        insertSection(KRSectionData::PageHeadAny);
                        rs = getSection(KRSectionData::PageHeadAny);
                    } else {
                        kDebug() << "tried to load more than one page head any";
                    }
                }
                if (rs) rs->initFromXML(it);
            } else if (n == "pgfoot") {
                // we need to determine which page this is for
                ReportSection * rs = 0;
                if (!it.namedItem("firstpage").isNull()) {
                    if (getSection(KRSectionData::PageFootFirst) == 0) {
                        insertSection(KRSectionData::PageFootFirst);
                        rs = getSection(KRSectionData::PageFootFirst);
                    } else {
                        kDebug() << "tried to load more than one page foot first";
                    }
                } else if (!it.namedItem("odd").isNull()) {
                    if (getSection(KRSectionData::PageFootOdd) == 0) {
                        insertSection(KRSectionData::PageFootOdd);
                        rs = getSection(KRSectionData::PageFootOdd);
                    } else {
                        kDebug() << "tried to load more than one page foot odd";
                    }
                } else if (!it.namedItem("even").isNull()) {
                    if (getSection(KRSectionData::PageFootEven) == 0) {
                        insertSection(KRSectionData::PageFootEven);
                        rs = getSection(KRSectionData::PageFootEven);
                    } else {
                        kDebug() << "tried to load more than one page foot even";
                    }
                } else if (!it.namedItem("lastpage").isNull()) {
                    if (getSection(KRSectionData::PageFootLast) == 0) {
                        insertSection(KRSectionData::PageFootLast);
                        rs = getSection(KRSectionData::PageFootLast);
                    } else {
                        kDebug() << "tried to load more than one page foot last";
                    }
                } else {
                    // we have the any page foot
                    if (getSection(KRSectionData::PageFootAny) == 0) {
                        insertSection(KRSectionData::PageFootAny);
                        rs = getSection(KRSectionData::PageFootAny);
                    } else {
                        kDebug() << "tried to load more than one page foot any";
                    }
                }
                if (rs) rs->initFromXML(it);
            } else if (n == "section") {
                ReportSectionDetail * rsd = new ReportSectionDetail(this);
                rsd->initFromXML(it);
                setDetail(rsd);
            } else {
                kDebug() << "Encountered an unknown Element: "  << n;
            }
        } else {
            kDebug() << "Encountered a child node of root that is not an Element";
        }
    }
    this->slotPageButton_Pressed();
    
    setModified(false);
}

ReportDesigner::~ReportDesigner()
{
}

void ReportDesigner::closeEvent(QCloseEvent * e)
{
    if (m_modified != false) {
        switch (QMessageBox::information(this, i18n("Report Writer"), i18n("The document '%1' contains unsaved changes.\nDo you want to save the changes before closing?" , windowTitle()), i18n("Save"), i18n("Discard"), i18n("Cancel"), 0, 2)) {
        case 0:
            // save the doc...
            // if we get a not save result we'll bail so the
            // user doesn't loose any work.
//    if ( !saveToDb() ) return;
        case 1:
            // all we have to do is just accept the close event
            break;
        case 2:
            return;
        default:
            qDebug("Encountered a problem in the close event handler....");
            // should we just go ahead and close??? or should we not close???
        }
    }
    e->accept();
}

void ReportDesigner::slotSectionEditor()
{
    SectionEditor * se = new SectionEditor(this);
    se->init(this);
    se->exec();
    delete se;
}

ReportSection * ReportDesigner::getSection(KRSectionData::Section s) const
{
    ReportSection *sec;
    switch (s) {
    case KRSectionData::PageHeadAny:
        sec = pageHeadAny;
        break;
    case KRSectionData::PageHeadEven:
        sec = pageHeadEven;
        break;
    case KRSectionData::PageHeadOdd:
        sec = pageHeadOdd;
        break;
    case KRSectionData::PageHeadFirst:
        sec = pageHeadFirst;
        break;
    case KRSectionData::PageHeadLast:
        sec = pageHeadLast;
        break;
    case KRSectionData::PageFootAny:
        sec = pageFootAny;
        break;
    case KRSectionData::PageFootEven:
        sec = pageFootEven;
        break;
    case KRSectionData::PageFootOdd:
        sec = pageFootOdd;
        break;
    case KRSectionData::PageFootFirst:
        sec = pageFootFirst;
        break;
    case KRSectionData::PageFootLast:
        sec = pageFootLast;
        break;
    case KRSectionData::ReportHead:
        sec = reportHead;
        break;
    case KRSectionData::ReportFoot:
        sec = reportFoot;
        break;
    default:
        sec = NULL;
    }
    return sec;
}
void ReportDesigner::removeSection(KRSectionData::Section s)
{
    ReportSection* sec = getSection(s);
    if (sec != NULL) {
        delete sec;

        switch (s) {
        case KRSectionData::PageHeadAny:
            pageHeadAny = NULL;
            break;
        case KRSectionData::PageHeadEven:
            sec = pageHeadEven = NULL;
            break;
        case KRSectionData::PageHeadOdd:
            pageHeadOdd = NULL;
            break;
        case KRSectionData::PageHeadFirst:
            pageHeadFirst = NULL;
            break;
        case KRSectionData::PageHeadLast:
            pageHeadLast = NULL;
            break;
        case KRSectionData::PageFootAny:
            pageFootAny = NULL;
            break;
        case KRSectionData::PageFootEven:
            pageFootEven = NULL;
            break;
        case KRSectionData::PageFootOdd:
            pageFootOdd = NULL;
            break;
        case KRSectionData::PageFootFirst:
            pageFootFirst = NULL;
            break;
        case KRSectionData::PageFootLast:
            pageFootLast = NULL;
            break;
        case KRSectionData::ReportHead:
            reportHead = NULL;
            break;
        case KRSectionData::ReportFoot:
            reportFoot = NULL;
            break;
        default:
            sec = NULL;
        }

        setModified(true);
        adjustSize();
    }
}
void ReportDesigner::insertSection(KRSectionData::Section s)
{
    ReportSection* sec = getSection(s);
    if (sec == NULL) {
        int idx = 0;
        for (int i = 1; i <= s; ++i) {
            if (getSection((KRSectionData::Section)(i)) != NULL)
                idx++;
        }
        if (s > KRSectionData::ReportHead)
            idx++;
        kDebug() << idx;
        ReportSection *rs = new ReportSection(this);
        d->vboxlayout->insertWidget(idx, rs);

        switch (s) {
        case KRSectionData::PageHeadAny:
            rs->setTitle(i18n("Page Header (Any)"));
            pageHeadAny = rs;
            break;
        case KRSectionData::PageHeadEven:
            rs->setTitle(i18n("Page Header (Even)"));
            pageHeadEven = rs;
            break;
        case KRSectionData::PageHeadOdd:
            rs->setTitle(i18n("Page Header (Odd)"));
            pageHeadOdd = rs;
            break;
        case KRSectionData::PageHeadFirst:
            rs->setTitle(i18n("Page Header (First)"));
            pageHeadFirst = rs;
            break;
        case KRSectionData::PageHeadLast:
            rs->setTitle(i18n("Page Header (Last)"));
            pageHeadLast = rs;
            break;
        case KRSectionData::PageFootAny:
            rs->setTitle(i18n("Page Footer (Any)"));
            pageFootAny = rs;
            break;
        case KRSectionData::PageFootEven:
            rs->setTitle(i18n("Page Footer (Even)"));
            pageFootEven = rs;
            break;
        case KRSectionData::PageFootOdd:
            rs->setTitle(i18n("Page Footer (Odd)"));
            pageFootOdd = rs;
            break;
        case KRSectionData::PageFootFirst:
            rs->setTitle(i18n("Page Footer (First)"));
            pageFootFirst = rs;
            break;
        case KRSectionData::PageFootLast:
            rs->setTitle(i18n("Page Footer (Last)"));
            pageFootLast = rs;
            break;
        case KRSectionData::ReportHead:
            rs->setTitle(i18n("Report Header"));
            reportHead = rs;
            break;
        case KRSectionData::ReportFoot:
            rs->setTitle(i18n("Report Footer"));
            reportFoot = rs;
            break;
        }

        rs->show();
        setModified(true);
        adjustSize();
        emit pagePropertyChanged(*m_set);
    }
}

QDomDocument ReportDesigner::document()
{
    QDomDocument doc = QDomDocument("pgzKexiReportDef");
    QDomElement root = doc.createElement("report");
    doc.appendChild(root);

    //title
    QDomElement title = doc.createElement("title");
    title.appendChild(doc.createTextNode(reportTitle()));
    root.appendChild(title);

    QDomElement rds = doc.createElement("datasource");
    rds.appendChild(doc.createTextNode(reportDataSource()));
    root.appendChild(rds);

    QDomElement scr = doc.createElement("script");
    scr.setAttribute("interpreter", m_interpreter->value().toString());
    scr.appendChild(doc.createTextNode(m_script->value().toString()));
    root.appendChild(scr);

    QDomElement grd = doc.createElement("grid");
    grd.setAttribute("visible", m_showGrid->value().toBool());
    grd.setAttribute("divisions", m_gridDivisions->value().toInt());
    grd.setAttribute("snap", m_gridSnap->value().toBool());
    root.appendChild(grd);

    // pageOptions
    // -- size
    QDomElement size = doc.createElement("size");

    if (m_pageSize->value().toString() == "Custom") {
        QDomElement page_width = doc.createElement("width");
        page_width.appendChild(doc.createTextNode(QString::number(pageUnit().fromUserValue(m_customWidth->value().toInt()))));
        size.appendChild(page_width);
        QDomElement page_height = doc.createElement("height");
        page_height.appendChild(doc.createTextNode(QString::number(pageUnit().fromUserValue(m_customWidth->value().toInt()))));
        size.appendChild(page_height);
    } else if (m_pageSize->value().toString() == "Labels") {
        size.appendChild(doc.createTextNode("Labels"));
        QDomElement labeltype = doc.createElement("labeltype");
        labeltype.appendChild(doc.createTextNode(m_labelType->value().toString()));
        root.appendChild(labeltype);
    } else {
        size.appendChild(doc.createTextNode(m_pageSize->value().toString()));
    }
    root.appendChild(size);
    // -- orientation
    root.appendChild(doc.createElement(m_orientation->value().toString().toLower()));
    // -- margins
    QDomElement margin;
    margin = doc.createElement("topmargin");
    margin.appendChild(doc.createTextNode(QString::number(m_topMargin->value().toDouble())));
    root.appendChild(margin);
    margin = doc.createElement("bottommargin");
    margin.appendChild(doc.createTextNode(QString::number(m_bottomMargin->value().toDouble())));
    root.appendChild(margin);
    margin = doc.createElement("rightmargin");
    margin.appendChild(doc.createTextNode(QString::number(m_rightMargin->value().toDouble())));
    root.appendChild(margin);
    margin = doc.createElement("leftmargin");
    margin.appendChild(doc.createTextNode(QString::number(m_leftMargin->value().toDouble())));
    root.appendChild(margin);

    QDomElement section;

    // report head
    if (reportHead) {
        section = doc.createElement("rpthead");
        reportHead->buildXML(doc, section);
        root.appendChild(section);
    }

    // page head first
    if (pageHeadFirst) {
        section = doc.createElement("pghead");
        section.appendChild(doc.createElement("firstpage"));
        pageHeadFirst->buildXML(doc, section);
        root.appendChild(section);
    }
    // page head odd
    if (pageHeadOdd) {
        section = doc.createElement("pghead");
        section.appendChild(doc.createElement("odd"));
        pageHeadOdd->buildXML(doc, section);
        root.appendChild(section);
    }
    // page head even
    if (pageHeadEven) {
        section = doc.createElement("pghead");
        section.appendChild(doc.createElement("even"));
        pageHeadEven->buildXML(doc, section);
        root.appendChild(section);
    }
    // page head last
    if (pageHeadLast) {
        section = doc.createElement("pghead");
        section.appendChild(doc.createElement("lastpage"));
        pageHeadLast->buildXML(doc, section);
        root.appendChild(section);
    }
    // page head any
    if (pageHeadAny) {
        section = doc.createElement("pghead");
        pageHeadAny->buildXML(doc, section);
        root.appendChild(section);
    }

    section = doc.createElement("section");
    detail->buildXML(doc, section);
    root.appendChild(section);

    // page foot first
    if (pageFootFirst) {
        section = doc.createElement("pgfoot");
        section.appendChild(doc.createElement("firstpage"));
        pageFootFirst->buildXML(doc, section);
        root.appendChild(section);
    }
    // page foot odd
    if (pageFootOdd) {
        section = doc.createElement("pgfoot");
        section.appendChild(doc.createElement("odd"));
        pageFootOdd->buildXML(doc, section);
        root.appendChild(section);
    }
    // page foot even
    if (pageFootEven) {
        section = doc.createElement("pgfoot");
        section.appendChild(doc.createElement("even"));
        pageFootEven->buildXML(doc, section);
        root.appendChild(section);
    }
    // page foot last
    if (pageFootLast) {
        section = doc.createElement("pgfoot");
        section.appendChild(doc.createElement("lastpage"));
        pageFootLast->buildXML(doc, section);
        root.appendChild(section);
    }
    // page foot any
    if (pageFootAny) {
        section = doc.createElement("pgfoot");
        pageFootAny->buildXML(doc, section);
        root.appendChild(section);
    }

    // report foot
    if (reportFoot) {
        section = doc.createElement("rptfoot");
        reportFoot->buildXML(doc, section);
        root.appendChild(section);
    }

    return doc;
}

void ReportDesigner::setReportTitle(const QString & str)
{
    if (reportTitle() != str) {
        m_title->setValue(str);
        setModified(true);
    }
}
QString ReportDesigner::reportTitle()
{
    return m_title->value().toString();
}

QString ReportDesigner::reportDataSource()
{
    return m_dataSource->value().toString();
}
void ReportDesigner::setReportDataSource(const QString &q)
{
    if (m_dataSource->value().toString() != q) {
        m_dataSource->setValue(q);
        setModified(true);
    }
}

bool ReportDesigner::isModified()
{
    return m_modified;
}
void ReportDesigner::setModified(bool mod)
{
    m_modified = mod;
 
    if (m_modified) {
        emit(dirty());

    }
}

QStringList ReportDesigner::queryList()
{
    //Get the list of queries in the database
    kDebug();
    QStringList qs;
    if (m_conn && m_conn->isConnected()) {
        QList<int> tids = m_conn->tableIds();
        qs << "";
        for (int i = 0; i < tids.size(); ++i) {
            KexiDB::TableSchema* tsc = m_conn->tableSchema(tids[i]);
            if (tsc)
                qs << tsc->name();
            else
                kDebug() << "Error retrieving table schema: " << tids[i];
        }

        QList<int> qids = m_conn->queryIds();
        qs << "";
        for (int i = 0; i < qids.size(); ++i) {
            KexiDB::QuerySchema* qsc = m_conn->querySchema(qids[i]);
            if (qsc)
                qs << qsc->name();
            else
                kDebug() << "Error retrieving query schema: " << qids[i];
        }
    }
    kDebug() << "done";
    return qs;
}

QStringList ReportDesigner::fieldList()
{
    QStringList qs;
    qs << "";

    if (isConnected()) {
        //Get the list of fields in the selected query
        KexiDB::TableOrQuerySchema *flds = new KexiDB::TableOrQuerySchema(m_conn, m_dataSource->value().toString().toLocal8Bit());

        KexiDB::QueryColumnInfo::Vector cs = flds->columns();

        for (int i = 0 ; i < cs.count(); ++i) {
            qs << cs[i]->field->name();
        }
    } else {
        kDebug() << "Cannot return field list";
    }
    return qs;
}

QStringList ReportDesigner::scriptList()
{
    QList<int> scriptids = m_conn->objectIds(KexiPart::ScriptObjectType);
    QStringList scripts;
    QStringList scriptnames = m_conn->objectNames(KexiPart::ScriptObjectType);
    QString script;
    
    int id, i;
    id = i = 0;
    
    kDebug() << scriptids << scriptnames;
    kDebug() << m_interpreter->value().toString();
    
    //A blank entry
    scripts << "";
    
    if (isConnected()) {
        foreach (id, scriptids) {
            kDebug() << "ID:" << id;
            tristate res;
            res = m_conn->loadDataBlock(id, script, QString());
            if (res == true){
                QDomDocument domdoc;
                bool parsed = domdoc.setContent(script, false);

                QDomElement scriptelem = domdoc.namedItem("script").toElement();
                if (parsed && !scriptelem.isNull()) {
                    if (m_interpreter->value().toString() == scriptelem.attribute("language") && scriptelem.attribute("scripttype") == "object") {
                        scripts << scriptnames[i];
                    }
                }
                else {
                    kDebug() << "Unable to parse script";
                }
            }
            else{
                kDebug() << "Unable to loadDataBlock";
            }
            ++i;
        }
    }
    kDebug() << scripts;
    return scripts;
}

void ReportDesigner::createProperties()
{
    QStringList keys, strings;
    m_set = new KoProperty::Set(0, "Report");

    connect(m_set, SIGNAL(propertyChanged(KoProperty::Set &, KoProperty::Property &)), this, SLOT(propertyChanged(KoProperty::Set &, KoProperty::Property &)));

    m_title = new KoProperty::Property("Title", "Report", "Title", "Report Title");

    keys = queryList();
    m_dataSource = new KoProperty::Property("DataSource", keys, keys, "", "Data Source");

    keys.clear();
    keys = pageFormats();
    m_pageSize = new KoProperty::Property("PageSize", keys, keys, "A4", "Page Size");

    keys.clear();strings.clear();
    keys << "Portrait" << "Landscape";
    strings << i18n("Portrait") << i18n("Landscape");
    m_orientation = new KoProperty::Property("Orientation", keys, strings, "Portrait", "Page Orientation");

    keys.clear();strings.clear();
    keys = KoUnit::listOfUnitName();
    strings = KoUnit::listOfUnitName();

    KoUnit u(KoUnit::Centimeter);
    m_unit = new KoProperty::Property("PageUnit", keys, strings, KoUnit::unitDescription(u), "Page Unit");

    m_showGrid = new KoProperty::Property("ShowGrid", true, "Show Grid", "Show Grid");
    m_gridSnap = new KoProperty::Property("GridSnap", true, "Grid Snap", "Grid Snap");
    m_gridDivisions = new KoProperty::Property("GridDivisions", 4, "Grid Divisions", "Grid Divisions");

    m_leftMargin = new KoProperty::Property("LeftMargin", KoUnit::unit("cm").fromUserValue(1.0), "Left Margin", "Left Margin", KoProperty::Double);
    m_rightMargin = new KoProperty::Property("RightMargin", KoUnit::unit("cm").fromUserValue(1.0), "Right Margin", "Right Margin", KoProperty::Double);
    m_topMargin = new KoProperty::Property("TopMargin", KoUnit::unit("cm").fromUserValue(1.0), "Top Margin", "Top Margin", KoProperty::Double);
    m_bottomMargin = new KoProperty::Property("BottomMargin", KoUnit::unit("cm").fromUserValue(1.0), "Bottom Margin", "Bottom Margin", KoProperty::Double);
    m_leftMargin->setOption("unit", "cm");
    m_rightMargin->setOption("unit", "cm");
    m_topMargin->setOption("unit", "cm");
    m_bottomMargin->setOption("unit", "cm");

    keys = Kross::Manager::self().interpreters();
    m_interpreter = new KoProperty::Property("Interpreter", keys, keys, keys[0], "Script Interpreter");

    keys = scriptList();
    m_script = new KoProperty::Property("Script", keys, keys, "", "Object Script");
    
    m_set->addProperty(m_title);
    m_set->addProperty(m_dataSource);
    m_set->addProperty(m_pageSize);
    m_set->addProperty(m_orientation);
    m_set->addProperty(m_unit);
    m_set->addProperty(m_gridSnap);
    m_set->addProperty(m_showGrid);
    m_set->addProperty(m_gridDivisions);
    m_set->addProperty(m_leftMargin);
    m_set->addProperty(m_rightMargin);
    m_set->addProperty(m_topMargin);
    m_set->addProperty(m_bottomMargin);
    m_set->addProperty(m_interpreter);
    m_set->addProperty(m_script);
    
    KoProperty::Property* _customHeight;
    KoProperty::Property* _customWidth;

}

/**
@brief Handle property changes
*/
void ReportDesigner::slotPropertyChanged(KoProperty::Set &s, KoProperty::Property &p)
{
    setModified(true);
    emit pagePropertyChanged(s);

    if (p.name() == "PageUnit") {
        d->hruler->setUnit(pageUnit());
        QString newstr = m_set->property("PageUnit").value().toString().mid(m_set->property("PageUnit").value().toString().indexOf("(") + 1, 2);
        
        m_set->property("LeftMargin").setOption("unit", newstr);
        m_set->property("RightMargin").setOption("unit", newstr);
        m_set->property("TopMargin").setOption("unit", newstr);
        m_set->property("BottomMargin").setOption("unit", newstr);
    }
}

/**
@brief When the 'page' button in the top left is pressed, change the property set to the reports properties.
*/
void ReportDesigner::slotPageButton_Pressed()
{
    QStringList sl = scriptList();
    
    m_script->setListData(sl, sl);
    changeSet(m_set);
}

/**
@brief Return a list of supported page formats
@return A QStringList of page formats
*/
QStringList ReportDesigner::pageFormats()
{
    QStringList lst;
    lst << "A4" << "Letter" << "Legal" << "A3" << "A5";
    return lst;
}

QSize ReportDesigner::sizeHint() const
{
    int w = 0;
    int h = 0;

    if (pageFootAny)
        h += pageFootAny->sizeHint().height();
    if (pageFootEven)
        h += pageFootEven->sizeHint().height();
    if (pageFootFirst)
        h += pageFootFirst->sizeHint().height();
    if (pageFootLast)
        h += pageFootLast->sizeHint().height();
    if (pageFootOdd)
        h += pageFootOdd->sizeHint().height();
    if (pageHeadAny)
        h += pageHeadAny->sizeHint().height();
    if (pageHeadEven)
        h += pageHeadEven->sizeHint().height();
    if (pageHeadFirst)
        h += pageHeadFirst->sizeHint().height();
    if (pageHeadLast)
        h += pageHeadLast->sizeHint().height();
    if (pageHeadOdd)
        h += pageHeadOdd->sizeHint().height();
    if (reportHead)
        h += reportHead->sizeHint().height();
    if (reportFoot) {
        h += reportFoot->sizeHint().height();

    }
    if (detail) {
        h += detail->sizeHint().height();
        w += detail->sizeHint().width();
    }

    h += d->hruler->height();

    return QSize(w, h);
}

/**
@brief Calculate the width of the page in pixels given the paper size, orientation, dpi and margin
@return integer value of width in pixels
*/
int ReportDesigner::pageWidthPx() const
{
    int cw = 0;
    int ch = 0;
    int width = 0;

    KoPageFormat::Format pf = KoPageFormat::formatFromString(m_set->property("PageSize").value().toString());

    cw = POINT_TO_INCH(MM_TO_POINT(KoPageFormat::width(pf, KoPageFormat::Portrait))) * KoGlobal::dpiX();

    ch = POINT_TO_INCH(MM_TO_POINT(KoPageFormat::height(pf, KoPageFormat::Portrait))) * KoGlobal::dpiY();

    width = (m_set->property("Orientation").value().toString() == "Portrait" ? cw : ch);

    width = width - POINT_TO_INCH(m_set->property("LeftMargin").value().toDouble()) * KoGlobal::dpiX();
    width = width - POINT_TO_INCH(m_set->property("RightMargin").value().toDouble()) * KoGlobal::dpiX();

    return width;
}

void ReportDesigner::resizeEvent(QResizeEvent * event)
{
    Q_UNUSED(event);
    //hruler->setRulerLength ( vboxlayout->geometry().width() );
    d->hruler->setRulerLength(pageWidthPx());
}

void ReportDesigner::setDetail(ReportSectionDetail *rsd)
{
    if (detail == NULL) {
        int idx = 0;
        if (pageHeadFirst) idx++;
        if (pageHeadOdd) idx++;
        if (pageHeadEven) idx++;
        if (pageHeadLast) idx++;
        if (pageHeadAny) idx++;
        if (reportHead) idx++;
        detail = rsd;
        d->vboxlayout->insertWidget(idx, detail);
    }
}
void ReportDesigner::deleteDetail()
{
    if (detail != 0) {
        delete detail;
        detail = 0;
    }
}

KoUnit ReportDesigner::pageUnit() const
{
    QString u;
    bool found;

    u = m_set->property("PageUnit").value().toString();
    u = u.mid(u.indexOf("(") + 1, 2);

    KoUnit unit = KoUnit::unit(u, &found);
    if (!found) {
        unit = KoUnit::unit("cm");
    }

    return unit;
}

void ReportDesigner::setGridOptions(bool vis, int div)
{
    m_showGrid->setValue(QVariant(vis));
    m_gridDivisions->setValue(div);
}

//
// methods for the sectionMouse*Event()
//
void ReportDesigner::sectionContextMenuEvent(ReportScene * s, QGraphicsSceneContextMenuEvent * e)
{
    QMenu pop;
    QAction *popCut = 0;
    QAction *popCopy = 0;
    QAction *popPaste = 0;
    QAction* popDelete = 0;

    bool itemsSelected = selectionCount() > 0;
    if (itemsSelected) {
        popCut = pop.addAction(i18n("Cut"));
        popCopy = pop.addAction(i18n("Copy"));
    }
    if (m_sectionData->copy_list.count() > 0)
        popPaste = pop.addAction(i18n("Paste"));

    if (itemsSelected) {
        pop.addSeparator();
        popDelete = pop.addAction(i18n("Delete"));
    }

    QAction * ret = pop.exec(e->screenPos());
    if (ret == popCut)
        slotEditCut();
    else if (ret == popCopy)
        slotEditCopy();
    else if (ret == popPaste)
        slotEditPaste(s, e->scenePos());
    else if (ret == popDelete)
        slotEditDelete();
}

void ReportDesigner::sectionMouseReleaseEvent(ReportSceneView * v, QMouseEvent * e)
{
    e->accept();
    QGraphicsItem * item = 0;
    if (e->button() == Qt::LeftButton) {
        switch (m_sectionData->mouseAction) {
        case ReportWriterSectionData::MA_Insert:
            switch (m_sectionData->insertItem) {
            case KRObjectData::EntityLabel :
                item = new ReportEntityLabel(v->designer(), v->scene());
                break;
            case KRObjectData::EntityField :
                item = new ReportEntityField(v->designer(), v->scene());
                break;
            case KRObjectData::EntityText :
                item = new ReportEntityText(v->designer(), v->scene());
                break;
            case KRObjectData::EntityBarcode :
                item = new ReportEntityBarcode(v->designer(), v->scene());
                break;
            case KRObjectData::EntityImage :
                item = new ReportEntityImage(v->designer(), v->scene());
                break;
            case KRObjectData::EntityLine :
                kDebug() << "Adding Line";
                item = new ReportEntityLine(v->designer(), v->scene());
                //dynamic_cast<QGraphicsLineItem*>(item)->setLine ( e->x()-10, e->y(), e->x()+10, e->y() );
                dynamic_cast<QGraphicsLineItem*>(item)->setLine(e->x(), e->y(), e->x() + 20, e->y());
                break;
            case KRObjectData::EntityChart :
                item = new ReportEntityChart(v->designer(), v->scene());
                break;
            case KRObjectData::EntityShape :
                item = new ReportEntityShape(v->designer(), v->scene());
                break;
            case KRObjectData::EntityCheck :
                item = new ReportEntityCheck(v->designer(), v->scene());
                break;
            default:
                kDebug() << "attempted to insert an unknown item";;
            }
            if (item) {
                if (m_sectionData->insertItem != KRObjectData::EntityLine)
                    item->setPos(e->x(), e->y());

                item->setVisible(true);
                if (v && v->designer())
                    v->designer()->setModified(true);
            }

            m_sectionData->mouseAction = ReportWriterSectionData::MA_None;
            m_sectionData->insertItem = KRObjectData::EntityNone;
            break;
        default:
            // what to do? Nothing
            // either we don't know what is going on
            // or everything has been done elsewhere
            break;
        }
    }
}

unsigned int ReportDesigner::selectionCount()
{
    return activeScene()->selectedItems().count();
}

void ReportDesigner::slotItem(KRObjectData::EntityTypes typ)
{
    m_sectionData->mouseAction = ReportWriterSectionData::MA_Insert;
    m_sectionData->insertItem = typ;
}

void ReportDesigner::changeSet(KoProperty::Set *s)
{
    m_itmset = s;
    emit(propertySetChanged());
}



//
// Edit Actions
//
void ReportDesigner::slotEditDelete()
{
    QGraphicsItem * item = 0;
    while (selectionCount() > 0) {
        item = activeScene()->selectedItems()[0];
        if (item) {
            setModified(true);
            QGraphicsScene * scene = item->scene();
            delete item;
            scene->update();
            m_sectionData->mouseAction = ReportWriterSectionData::MA_None;
        }
    }
    activeScene()->selectedItems().clear();
    m_sectionData->selected_items_rw = 0;

    //TODO temporary
    //clears cut and copy lists to make sure we do not crash
    //if weve deleted something in the list
    //should really check if an item is in the list first
    //and remove it.
    m_sectionData->cut_list.clear();
    m_sectionData->copy_list.clear();
}

void ReportDesigner::slotEditCut()
{
    if (selectionCount() > 0) {
        //First delete any items that are curerntly in the list
        //so as not to leak memory
        for (int i = 0; i < m_sectionData->cut_list.count(); i++) {
            delete m_sectionData->cut_list[i];
        }
        m_sectionData->cut_list.clear();

        QGraphicsItem * item = activeScene()->selectedItems().first();
        if (item) {
            if (item->type() == KRObjectData::EntityLine) {
                m_sectionData->copy_x_pos = ((ReportEntityLine*) item)->line().p1().x();
                m_sectionData->copy_y_pos = ((ReportEntityLine*) item)->line().p1().y();
            } else {
                m_sectionData->copy_x_pos = (int) item->x();
                m_sectionData->copy_y_pos = (int) item->y();
            }

            m_sectionData->copy_list.clear();

            for (int i = 0; i < activeScene()->selectedItems().count(); i++) {
                QGraphicsItem *itm = activeScene()->selectedItems()[i];
                m_sectionData->cut_list.append(dynamic_cast<ReportEntity*>(itm));
                m_sectionData->copy_list.append(dynamic_cast<ReportEntity*>(itm));
            }
            int c = activeScene()->selectedItems().count();
            for (int i = 0; i < c; i++) {
                QGraphicsItem *itm = activeScene()->selectedItems()[0];
                activeScene()->removeItem(itm);
                activeScene()->update();
            }
            m_sectionData->selected_x_offset = 10;
            m_sectionData->selected_y_offset = 10;
        }
    }
}

void ReportDesigner::slotEditCopy()
{
    if (selectionCount() < 1)
        return;

    QGraphicsItem * item = activeScene()->selectedItems().first();
    if (item) {
        m_sectionData->copy_list.clear();
        if (item->type() == KRObjectData::EntityLine) {
            m_sectionData->copy_x_pos = ((ReportEntityLine*) item)->line().p1().x();
            m_sectionData->copy_y_pos = ((ReportEntityLine*) item)->line().p1().y();
        } else {
            m_sectionData->copy_x_pos = (int) item->x();
            m_sectionData->copy_y_pos = (int) item->y();
        }

        for (int i = 0; i < activeScene()->selectedItems().count(); i++) {
            m_sectionData->copy_list.append(dynamic_cast<ReportEntity*>(activeScene()->selectedItems()[i]));
        }
        m_sectionData->selected_x_offset = 10;
        m_sectionData->selected_y_offset = 10;
    }
}

void ReportDesigner::slotEditPaste()
{
    // call the editPaste function passing it a reportsection and point
    //  that make sense as defaults (same canvas / slightly offset pos of orig copy)
    QPoint p;
    p.setX(m_sectionData->copy_x_pos + m_sectionData->selected_x_offset);
    p.setY(m_sectionData->copy_y_pos + m_sectionData->selected_x_offset);
    slotEditPaste(activeScene(), p);
}

void ReportDesigner::slotEditPaste(QGraphicsScene * canvas, const QPointF & pos)
{
    Q_UNUSED(pos);
    // paste a new item of the copy we have in the specified location
    if (m_sectionData->copy_list.count() > 0) {
        QGraphicsItem * pasted_ent = 0;
        canvas->clearSelection();
        m_sectionData->mouseAction = ReportWriterSectionData::MA_None;

        for (int i = 0; i < m_sectionData->copy_list.count(); i++) {
            pasted_ent = 0;
            int type = dynamic_cast<KRObjectData*>(m_sectionData->copy_list[i])->type();
            kDebug() << type;
            QPointF o(m_sectionData->selected_x_offset, m_sectionData->selected_y_offset);
            if (type == KRObjectData::EntityLabel) {
                ReportEntityLabel * ent = dynamic_cast<ReportEntityLabel*>(m_sectionData->copy_list[i])->clone();
                ent->setEntityName(suggestEntityName("Label"));
                ent->setPos(ent->pos() + o);
                pasted_ent = ent;
            } else if (type == KRObjectData::EntityField) {
                ReportEntityField * ent = dynamic_cast<ReportEntityField*>(m_sectionData->copy_list[i])->clone();
                ent->setPos(ent->pos() + o);
                ent->setEntityName(suggestEntityName("Field"));
                pasted_ent = ent;
            } else if (type == KRObjectData::EntityText) {
                ReportEntityText * ent = dynamic_cast<ReportEntityText*>(m_sectionData->copy_list[i])->clone();
                ent->setPos(ent->pos() + o);
                ent->setEntityName(suggestEntityName("Text"));
                pasted_ent = ent;
            } else if (type == KRObjectData::EntityLine) {
                ReportEntityLine * ent = dynamic_cast<ReportEntityLine*>(m_sectionData->copy_list[i])->clone();
                ent->setEntityName(suggestEntityName("Line"));
                pasted_ent = ent;
            } else if (type == KRObjectData::EntityBarcode) {
                ReportEntityBarcode * ent = dynamic_cast<ReportEntityBarcode*>(m_sectionData->copy_list[i])->clone();
                ent->setPos(ent->pos() + o);
                ent->setEntityName(suggestEntityName("Barcode"));
                pasted_ent = ent;
            } else if (type == KRObjectData::EntityImage) {
                ReportEntityImage * ent = dynamic_cast<ReportEntityImage*>(m_sectionData->copy_list[i])->clone();
                ent->setPos(ent->pos() + o);
                ent->setEntityName(suggestEntityName("Image"));
                pasted_ent = ent;
            }
            //TODO add graph
            //else if(cp.copy_item == ReportWriterSectionData::GraphItem) {
            //    ReportEntityGraph * ent = new ReportEntityGraph(cp.copy_graph, rw, canvas);
            //    ent->setX(pos.x() + cp.copy_offset_x);
            //    ent->setY(pos.y() + cp.copy_offset_y);
            //    ent->setSize(cp.copy_rect.width(), cp.copy_rect.height());
            //    ent->show();
            //    pasted_ent = ent;
            //}
            else {
                kDebug() << "Tried to paste an item I don't understand.";
            }

            if (pasted_ent) {
                canvas->addItem(pasted_ent);
                pasted_ent->show();
                m_sectionData->mouseAction = ReportWriterSectionData::MA_Grab;
                setModified(true);
            }
        }
        m_sectionData->selected_x_offset += 10;
        m_sectionData->selected_y_offset += 10;
    }
}
void ReportDesigner::slotRaiseSelected()
{
    dynamic_cast<ReportScene*>(activeScene())->raiseSelected();
}

void ReportDesigner::slotLowerSelected()
{
    dynamic_cast<ReportScene*>(activeScene())->lowerSelected();
}

QGraphicsScene* ReportDesigner::activeScene()
{
    return d->activeScene;
}

void ReportDesigner::setActiveScene(QGraphicsScene* a)
{
    if (d->activeScene && d->activeScene != a)
        d->activeScene->clearSelection();
    d->activeScene = a;
}

KoZoomHandler* ReportDesigner::zoomHandler()
{
    return d->zoom;
}

QString ReportDesigner::suggestEntityName(const QString &n) const
{
    ReportSection *sec;
    int itemCount = 0;
    //Count items in the main sections
    for (int i = 1; i <= 12; ++i) {
        sec = getSection((KRSectionData::Section) i);
        if (sec) {
            const QGraphicsItemList l = sec->items();
            itemCount += l.count();
        }
    }

    //Count items in the group headers/footers
    for (int i = 0; i < detail->groupSectionCount(); ++i) {
        sec = detail->getSection(i)->getGroupHead();
        if (sec) {
            const QGraphicsItemList l = sec->items();
            itemCount += l.count();
        }
        sec = detail->getSection(i)->getGroupFoot();
        if (sec) {
            const QGraphicsItemList l = sec->items();
            itemCount += l.count();
        }
    }

    if (detail) {
        sec = detail->getDetail();
        if (sec) {
            const QGraphicsItemList l = sec->items();
            itemCount += l.count();
        }
    }

    while (!isEntityNameUnique(n + QString::number(itemCount))) {
        itemCount++;
    }
    return n + QString::number(itemCount);
}

bool ReportDesigner::isEntityNameUnique(const QString &n, KRObjectData* ignore) const
{
    ReportSection *sec;
    bool unique = true;

    //Check items in the main sections
    for (int i = 1; i <= 12; ++i) {
        sec = getSection((KRSectionData::Section) i);
        if (sec) {
            const QGraphicsItemList l = sec->items();
            for (QGraphicsItemList::const_iterator it = l.begin(); it != l.end(); it++) {
                KRObjectData* itm = dynamic_cast<KRObjectData*>(*it);
                if (itm->entityName() == n  && itm != ignore) {
                    unique = false;
                    break;
                }
            }
            if (!unique) break;
        }
    }

    //Count items in the group headers/footers
    if (unique) {
        for (int i = 0; i < detail->groupSectionCount(); ++i) {
            sec = detail->getSection(i)->getGroupHead();
            if (sec) {
                const QGraphicsItemList l = sec->items();
                for (QGraphicsItemList::const_iterator it = l.begin(); it != l.end(); it++) {
                    KRObjectData* itm = dynamic_cast<KRObjectData*>(*it);
                    if (itm->entityName() == n  && itm != ignore) {
                        unique = false;
                        break;
                    }
                }

            }
            sec = detail->getSection(i)->getGroupFoot();
            if (unique && sec) {
                const QGraphicsItemList l = sec->items();
                for (QGraphicsItemList::const_iterator it = l.begin(); it != l.end(); it++) {
                    KRObjectData* itm = dynamic_cast<KRObjectData*>(*it);
                    if (itm->entityName() == n  && itm != ignore) {
                        unique = false;
                        break;
                    }
                }
            }
        }
    }
    if (unique && detail) {
        sec = detail->getDetail();
        if (sec) {
            const QGraphicsItemList l = sec->items();
            for (QGraphicsItemList::const_iterator it = l.begin(); it != l.end(); it++) {
                KRObjectData* itm = dynamic_cast<KRObjectData*>(*it);
                if (itm->entityName() == n  && itm != ignore) {
                    unique = false;
                    break;
                }
            }
        }
    }

    return unique;
}
