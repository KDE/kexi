/*
* Kexi Report Plugin
* Copyright (C) 2007-2009 by Adam Pigg <adam@piggz.co.uk>
* Copyright (C) 2011-2017 Jarosław Staniek <staniek@kde.org>
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
* License along with this library.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "kexireportdesignview.h"
#include <core/KexiMainWindowIface.h>
#include <core/KexiWindow.h>
#include "kexisourceselector.h"
#include <KexiIcon.h>
#include <kexiutils/utils.h>

#include <KDbConnection>

#include <QShortcut>
#include <QDebug>
#include <QScrollArea>
#include <QLayout>

#include <KStandardGuiItem>

KexiReportDesignView::KexiReportDesignView(QWidget *parent, KexiSourceSelector *s)
        : KexiView(parent)
{
    m_scrollArea = new QScrollArea(this);
    setViewWidget(m_scrollArea);
    m_sourceSelector = s;

    m_reportDesigner = 0;

    m_editCutAction = KStandardAction::cut(this);
    m_editCutAction->setProperty("iconOnly", true);
    m_editCopyAction = KStandardAction::copy(this);
    m_editCopyAction->setProperty("iconOnly", true);
    m_editPasteAction = KStandardAction::paste(this);
    m_editPasteAction->setProperty("iconOnly", true);
    const KGuiItem del = KStandardGuiItem::del();
    m_editDeleteAction = new QAction(del.icon(), del.text(), this);
    m_editDeleteAction->setObjectName("editdelete");
    m_editDeleteAction->setToolTip(del.toolTip());
    m_editDeleteAction->setWhatsThis(del.whatsThis());
    m_editDeleteAction->setProperty("iconOnly", true);

    m_editSectionAction = new QAction(xi18n("Edit Sections"), this);
    m_editSectionAction->setObjectName("sectionedit");

    m_itemRaiseAction = new QAction(koIcon("object-order-front"), xi18n("Raise"), this);
    m_itemRaiseAction->setObjectName("itemraise");
    m_itemLowerAction = new QAction(koIcon("object-order-back"), xi18n("Lower"), this);
    m_itemLowerAction->setObjectName("itemlower");
    QList<QAction*> al;
    QAction *sep = new QAction(QString(), this);
    sep->setSeparator(true);

    al << m_editCutAction << m_editCopyAction << m_editPasteAction << m_editDeleteAction << sep << m_editSectionAction << sep << m_itemLowerAction << m_itemRaiseAction;
    setViewActions(al);

}

KexiReportDesignView::~KexiReportDesignView()
{
}

KPropertySet *KexiReportDesignView::propertySet()
{
    return m_reportDesigner->selectedItemPropertySet();
}

//! Finds or creates property
static KProperty* findOrCreateProperty(KPropertySet *set, const char *name, const QVariant &value)
{
    KProperty *prop;
    if (set->contains(name)) {
        prop = &set->property(name);
        prop->setValue(value);
    } else {
        prop = new KProperty(name, value);
        prop->setVisible(false);
        set->addProperty(prop);
    }
    return prop;
}

void KexiReportDesignView::slotDesignerPropertySetChanged()
{
    KPropertySet *set = propertySet();
    if (set) {
        KProperty *prop = findOrCreateProperty(set, "this:visibleObjectNameProperty", "name");
        Q_UNUSED(prop)
    }
    propertySetReloaded(true);
    propertySetSwitched();
}

KDbObject* KexiReportDesignView::storeNewData(const KDbObject& object,
                                                       KexiView::StoreNewDataOptions options,
                                                       bool *cancel)
{
    KDbObject *s = KexiView::storeNewData(object, options, cancel);
    if (!s || *cancel) {
        delete s;
        return 0;
    }
    //qDebug() << "new id:" << s->id();

    if (!storeData()) {
        //failure: remove object's object data to avoid garbage
        KDbConnection *conn = KexiMainWindowIface::global()->project()->dbConnection();
        conn->removeObject(s->id());
        delete s;
        return 0;
    }
    return s;

}

tristate KexiReportDesignView::storeData(bool dontAsk)
{
    Q_UNUSED(dontAsk);

    QDomDocument doc("kexireport");
    QDomElement root = doc.createElement("kexireport");
    QDomElement conndata = connectionData();

    if (conndata.isNull()) {
        //qDebug() << "Null conn data!";
    }

    root.appendChild(m_reportDesigner->document());
    root.appendChild(conndata);
    doc.appendChild(root);

    QString src  = doc.toString();
    //qDebug() << src;

    if (storeDataBlock(src, "layout")) {
        //qDebug() << "Saved OK";
        setDirty(false);
        return true;
    }

    //qDebug() << "NOT Saved OK";
    return false;
}

tristate KexiReportDesignView::beforeSwitchTo(Kexi::ViewMode mode, bool *dontStore)
{
    //qDebug() << mode;
    *dontStore = true;
    if (m_reportDesigner && mode == Kexi::DataViewMode) {
        //qDebug() << "Saving temp data";
        tempData()->reportDefinition = m_reportDesigner->document();
        //qDebug() << m_reportDesigner->document().toDocument().toString();
        tempData()->reportSchemaChangedInPreviousView = true;
    }
    return true;
}

tristate KexiReportDesignView::afterSwitchFrom(Kexi::ViewMode mode)
{
    Q_UNUSED(mode);

    if (tempData()->reportDefinition.isNull()) {
        m_reportDesigner = new KReportDesigner(this);
    } else {
        if (m_reportDesigner) {
            m_scrollArea->takeWidget();
            delete m_reportDesigner;
            m_reportDesigner = 0;
        }

        m_reportDesigner = new KReportDesigner(this, tempData()->reportDefinition);
        setConnectionData(tempData()->connectionDefinition);
        m_reportDesigner->setScriptSource(qobject_cast<KexiReportPart*>(part()));
    }
    connect(m_reportDesigner, SIGNAL(itemInserted(QString)), this, SIGNAL(itemInserted(QString)));

    m_scrollArea->setWidget(m_reportDesigner);

    connect(m_reportDesigner, SIGNAL(propertySetChanged()), this, SLOT(slotDesignerPropertySetChanged()));
    connect(m_reportDesigner, SIGNAL(dirty()), this, SLOT(setDirty()));

     //Added default keyboard shortcuts for the actions
     QShortcut *cutShortcut = new QShortcut(QKeySequence(QKeySequence::Cut), m_reportDesigner);
     QShortcut *copyShortcut = new QShortcut(QKeySequence(QKeySequence::Copy), m_reportDesigner);
     QShortcut *pasteShortcut = new QShortcut(QKeySequence(QKeySequence::Paste), m_reportDesigner);
     QShortcut *deleteShortcut = new QShortcut(QKeySequence(QKeySequence::Delete), m_reportDesigner);

     connect(cutShortcut, SIGNAL(activated()), m_reportDesigner, SLOT(slotEditCut()));
     connect(copyShortcut, SIGNAL(activated()), m_reportDesigner, SLOT(slotEditCopy()));
     connect(pasteShortcut, SIGNAL(activated()), m_reportDesigner, SLOT(slotEditPaste()));
     connect(deleteShortcut, SIGNAL(activated()), m_reportDesigner, SLOT(slotEditDelete()));

    //Edit Actions
    connect(m_editCutAction, SIGNAL(triggered()), m_reportDesigner, SLOT(slotEditCut()));
    connect(m_editCopyAction, SIGNAL(triggered()), m_reportDesigner, SLOT(slotEditCopy()));
    connect(m_editPasteAction, SIGNAL(triggered()), m_reportDesigner, SLOT(slotEditPaste()));
    connect(m_editDeleteAction, SIGNAL(triggered()), m_reportDesigner, SLOT(slotEditDelete()));

    connect(m_editSectionAction, SIGNAL(triggered()), m_reportDesigner, SLOT(slotSectionEditor()));

    //Raise/Lower
    connect(m_itemRaiseAction, SIGNAL(triggered()), m_reportDesigner, SLOT(slotRaiseSelected()));
    connect(m_itemLowerAction, SIGNAL(triggered()), m_reportDesigner, SLOT(slotLowerSelected()));
    return true;
}

KexiReportPartTempData* KexiReportDesignView::tempData() const
{
    return static_cast<KexiReportPartTempData*>(window()->data());
}

void KexiReportDesignView::slotDataSourceChanged()
{
    if (m_sourceSelector->isSelectionValid()) {
        m_reportDesigner->setDataSource(new KexiDBReportDataSource(
            m_sourceSelector->selectedName(), m_sourceSelector->selectedPluginId(), tempData()));
        tempData()->connectionDefinition = connectionData();
    } else {
        m_reportDesigner->setDataSource(nullptr);
        tempData()->connectionDefinition = QDomElement();
    }
    setDirty(true);
}

void KexiReportDesignView::triggerAction(const QString &action)
{
    m_reportDesigner->slotItem(action);
}

QDomElement KexiReportDesignView::connectionData() const
{
    QDomDocument dd;
    QDomElement conndata = dd.createElement("connection");
    conndata.setAttribute("type", "internal"); // for backward compatibility, currently always
                                               // internal, we used to have "external" in old KEXI
    conndata.setAttribute("source", m_sourceSelector->selectedName());
    conndata.setAttribute("class", m_sourceSelector->selectedPluginId());
    return conndata;
}

void KexiReportDesignView::setConnectionData(const QDomElement &c)
{
    //qDebug() << c;
    if (c.attribute("type") == "internal") {
        QString sourceClass(c.attribute("class"));
        if (sourceClass != "org.kexi-project.table" && sourceClass != "org.kexi-project.query") {
            sourceClass.clear(); // KexiDataSourceComboBox will try to find table, then query
        }
        m_sourceSelector->setDataSource(sourceClass, c.attribute("source"));
        slotDataSourceChanged();
    }
}
