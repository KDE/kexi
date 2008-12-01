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
#include "kexireportdesignview.h"
#include <core/KexiMainWindowIface.h>
#include <kdebug.h>
#include <QScrollArea>
#include <core/KexiWindow.h>
#include "reportentityselector.h"
#include <kpushbutton.h>
KexiReportDesignView::KexiReportDesignView(QWidget *parent, ReportEntitySelector* r)
        : KexiView(parent)
{
    scr = new QScrollArea(this);
    layout()->addWidget(scr);
    res = r;
    _rd = 0;

    editCutAction = new KAction(KIcon("edit-cut"), i18n("Cut"), this);
    editCutAction->setObjectName("editcut");
    editCopyAction = new KAction(KIcon("edit-copy"), i18n("Copy"), this);
    editCopyAction->setObjectName("editcopy");
    editPasteAction = new KAction(KIcon("edit-paste"), i18n("Paste"), this);
    editPasteAction->setObjectName("editpaste");
    editDeleteAction = new KAction(KIcon("edit-delete"), i18n("Delete"), this);
    editDeleteAction->setObjectName("editdelete");

    sectionEdit = new KAction(i18n("Section Editor"), this);
    sectionEdit->setObjectName("sectionedit");

    itemRaiseAction = new KAction(KIcon("arrow-up"), i18n("Raise"), this);
    itemRaiseAction->setObjectName("itemraise");
    itemLowerAction = new KAction(KIcon("arrow-down"), i18n("Lower"), this);
    itemLowerAction->setObjectName("itemlower");
    //parameterEdit = new KAction ( i18n ( "Parameter Editor" ), this );
    //parameterEdit->setObjectName("parameteredit");
    QList<QAction*> al;
    KAction *sep = new KAction("", this);
    sep->setSeparator(true);

    al << editCutAction << editCopyAction << editPasteAction << editDeleteAction << sep << sectionEdit << sep << itemLowerAction << itemRaiseAction;
    setViewActions(al);

}


KexiReportDesignView::~KexiReportDesignView()
{
}

KoProperty::Set *KexiReportDesignView::propertySet()
{
    return _rd->itemPropertySet();
}

void KexiReportDesignView::slotDesignerPropertySetChanged()
{
    propertySetReloaded(true);
    propertySetSwitched();
}
KexiDB::SchemaData* KexiReportDesignView::storeNewData(const KexiDB::SchemaData& sdata, bool &cancel)
{
    KexiDB::SchemaData *s = KexiView::storeNewData(sdata, cancel);
    kexipluginsdbg << "new id:" << s->id();

    if (!s || cancel) {
        delete s;
        return 0;
    }
    if (!storeData()) {
        //failure: remove object's schema data to avoid garbage
        KexiDB::Connection *conn = KexiMainWindowIface::global()->project()->dbConnection();
        conn->removeObject(s->id());
        delete s;
        return 0;
    }
    return s;
    
    
    
    
    #if 0
    
    
    KexiDB::SchemaData *rpt = new KexiDB::SchemaData();
    *rpt = sdata;
    //sdata.setName ( name );
    //sdata.setDescription ( _rd->reportTitle() );
    KexiDB::Connection *conn = KexiMainWindowIface::global()->project()->dbConnection();

    if (conn->storeObjectSchemaData(*rpt, true /*newObject*/)) {
        window()->setId(rpt->id());

        if (rpt->id() > 0 && storeDataBlock(_rd->document().toString(), "pgzreport_layout")) {
            kDebug() << "Saved OK " << rpt->id();
        } else {
            kDebug() << "NOT Saved OK";
            return 0;
        }
    } else {
        kDebug() << "Unable to store schema data";
        return 0;
    }
    return rpt;
    #endif
}

tristate KexiReportDesignView::storeData(bool dontAsk)
{
    QString src  = _rd->document().toString();
    KexiDB::Connection *conn = KexiMainWindowIface::global()->project()->dbConnection();

    if (storeDataBlock(src, "pgzreport_layout")) {
        kDebug() << "Saved OK";
        setDirty(false);
        return true;
    } else {
        kDebug() << "NOT Saved OK";
    }


    return false;
}

tristate KexiReportDesignView::beforeSwitchTo(Kexi::ViewMode mode, bool &dontStore)
{
    kDebug() << mode;
    dontStore = true;
    if (_rd && mode == Kexi::DataViewMode) {
        tempData()->document = _rd->document().toString();
        tempData()->reportSchemaChangedInPreviousView = true;
    }
    return true;
}

tristate KexiReportDesignView::afterSwitchFrom(Kexi::ViewMode mode)
{
    kDebug() << tempData()->document;
    if (tempData()->document.isEmpty()) {
        _rd = new ReportDesigner(this, KexiMainWindowIface::global()->project()->dbConnection());
    } else {
        if (_rd) {
            scr->takeWidget();
            delete _rd;
            _rd = 0;

        }
        _rd = new ReportDesigner(this, KexiMainWindowIface::global()->project()->dbConnection(), tempData()->document);
    }

    scr->setWidget(_rd);

    //plugSharedAction ( "edit_copy", _rd, SLOT ( slotEditCopy() ) );
    //plugSharedAction ( "edit_cut", _rd, SLOT ( slotEditCut() ) );
    //plugSharedAction ( "edit_paste", _rd, SLOT ( slotEditPaste() ) );
    //plugSharedAction ( "edit_delete", _rd, SLOT ( slotEditDelete() ) );

    connect(_rd, SIGNAL(propertySetChanged()), this, SLOT(slotDesignerPropertySetChanged()));
    connect(_rd, SIGNAL(dirty()), this, SLOT(setDirty()));

    //Edit Actions
    connect(editCutAction, SIGNAL(activated()), _rd, SLOT(slotEditCut()));
    connect(editCopyAction, SIGNAL(activated()), _rd, SLOT(slotEditCopy()));
    connect(editPasteAction, SIGNAL(activated()), _rd, SLOT(slotEditPaste()));
    connect(editDeleteAction, SIGNAL(activated()), _rd, SLOT(slotEditDelete()));

    connect(sectionEdit, SIGNAL(activated()), _rd, SLOT(slotSectionEditor()));

    //Control Actions
    connect(res->itemLabel, SIGNAL(clicked()), this, SLOT(slotLabel()));
    connect(res->itemField, SIGNAL(clicked()), this, SLOT(slotField()));
    connect(res->itemText, SIGNAL(clicked()), this, SLOT(slotText()));
    connect(res->itemLine, SIGNAL(clicked()), this, SLOT(slotLine()));
    connect(res->itemBarcode, SIGNAL(clicked()), this, SLOT(slotBarcode()));
    connect(res->itemChart, SIGNAL(clicked()),this, SLOT(slotChart()));
    connect(res->itemImage, SIGNAL(clicked()), this, SLOT(slotImage()));
    connect(res->itemShape, SIGNAL(clicked()), this, SLOT(slotShape()));
connect(res->itemCheck, SIGNAL(clicked()), this, SLOT(slotCheck()));

    //Raise/Lower
    connect(itemRaiseAction, SIGNAL(activated()), _rd, SLOT(slotRaiseSelected()));
    connect(itemLowerAction, SIGNAL(activated()), _rd, SLOT(slotLowerSelected()));
    return true;
}

KexiReportPart::TempData* KexiReportDesignView::tempData() const
{
    return static_cast<KexiReportPart::TempData*>(window()->data());
}
