/* This file is part of the KDE project
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2004-2009 Jarosław Staniek <staniek@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include "kexiformview.h"

#include <QFileInfo>
#include <QFocusEvent>
#include <QDragMoveEvent>
#include <QEvent>
#include <QHBoxLayout>
#include <QDropEvent>
#include <QResizeEvent>
#include <QApplication>

#include <formeditor/form.h>
#include <formeditor/formIO.h>
//2.0 #include <formeditor/formmanager.h>
#include <formeditor/objecttree.h>
#include <formeditor/container.h>
//2.0 #include <formeditor/widgetpropertyset.h>
#include <formeditor/commands.h>
#include <formeditor/widgetwithsubpropertiesinterface.h>
#include <formeditor/objecttree.h>

#include <kexi.h>
#include <kexi_global.h>
#include <kexidragobjects.h>
#include <kexidb/field.h>
#include <kexidb/fieldlist.h>
#include <kexidb/connection.h>
#include <kexidb/cursor.h>
#include <kexidb/utils.h>
#include <kexidb/preparedstatement.h>
#include <tableview/kexitableviewdata.h>
#include <widget/kexipropertyeditorview.h>
#include <widget/kexiqueryparameters.h>
#include <kexiutils/utils.h>
#include <KexiMainWindowIface.h>

#include <koproperty/Set.h>
#include <koproperty/Property.h>

#include "widgets/kexidbform.h"
#include "kexiformscrollview.h"
#include "kexidatasourcepage.h"
#include "kexiformmanager.h"
#include "widgets/kexidbautofield.h"

#define NO_DSWIZARD

//! @todo #define KEXI_SHOW_SPLITTER_WIDGET

KexiFormView::KexiFormView(QWidget *parent, bool /*dbAware*/)
        : KexiDataAwareView(parent)
//2.0        , m_propertySet(0)
        , m_resizeMode(KexiFormView::ResizeDefault)
        , m_query(0)
        , m_queryIsOwned(false)
        , m_cursor(0)
// , m_firstFocusWidget(0)
{
    m_delayedFormContentsResizeOnShow = 0;
//! @todo remove?
    setSortedProperties(true);

    QHBoxLayout *l = new QHBoxLayout(this);
//Qt3 l->setAutoAdd(true);

    m_scrollView = new KexiFormScrollView(this, viewMode() == Kexi::DataViewMode);
    l->addWidget(m_scrollView);

    initForm();

    if (viewMode() == Kexi::DesignViewMode) {
//        connect(KFormDesigner::FormManager::self(), SIGNAL(propertySetSwitched(KoProperty::Set*, bool, const QByteArray&)),
//                this, SLOT(slotPropertySetSwitched(KoProperty::Set*, bool, const QByteArray&)));
        connect(form(), SIGNAL(propertySetSwitched()), this, SLOT(slotPropertySetSwitched()));
//        connect(KFormDesigner::FormManager::self(), SIGNAL(dirty(KFormDesigner::Form *, bool)),
//                this, SLOT(slotDirty(KFormDesigner::Form *, bool)));
        connect(form(), SIGNAL(modified()), this, SLOT(slotModified()));

        connect(m_dbform, SIGNAL(handleDragMoveEvent(QDragMoveEvent*)),
                this, SLOT(slotHandleDragMoveEvent(QDragMoveEvent*)));
        connect(m_dbform, SIGNAL(handleDropEvent(QDropEvent*)),
                this, SLOT(slotHandleDropEvent(QDropEvent*)));

        // action stuff
//2.0        plugSharedAction("formpart_taborder", KFormDesigner::FormManager::self(), SLOT(editTabOrder()));
        plugSharedAction("formpart_taborder", form(), SLOT(editTabOrder()));
//2.0        plugSharedAction("formpart_adjust_size", KFormDesigner::FormManager::self(), SLOT(adjustWidgetSize()));
        plugSharedAction("formpart_adjust_size", form(), SLOT(adjustWidgetSize()));
//TODO  plugSharedAction("formpart_pixmap_collection", formPart()->manager(), SLOT(editFormPixmapCollection()));
//TODO  plugSharedAction("formpart_connections", formPart()->manager(), SLOT(editConnections()));

//2.0        plugSharedAction("edit_copy", KFormDesigner::FormManager::self(), SLOT(copyWidget()));
        plugSharedAction("edit_copy", form(), SLOT(copyWidget()));
//2.0        plugSharedAction("edit_cut", KFormDesigner::FormManager::self(), SLOT(cutWidget()));
        plugSharedAction("edit_cut", form(), SLOT(cutWidget()));
//2.0        plugSharedAction("edit_paste", KFormDesigner::FormManager::self(), SLOT(pasteWidget()));
        plugSharedAction("edit_paste", form(), SLOT(pasteWidget()));
//2.0        plugSharedAction("edit_delete", KFormDesigner::FormManager::self(), SLOT(deleteWidget()));
        plugSharedAction("edit_delete", form(), SLOT(deleteWidget()));
//2.0        plugSharedAction("edit_select_all", KFormDesigner::FormManager::self(), SLOT(selectAll()));
        plugSharedAction("edit_select_all", form(), SLOT(selectAll()));
//2.0        plugSharedAction("formpart_clear_contents", KFormDesigner::FormManager::self(), SLOT(clearWidgetContent()));
        plugSharedAction("formpart_clear_contents", form(), SLOT(clearWidgetContent()));
//2.0        plugSharedAction("edit_undo", KFormDesigner::FormManager::self(), SLOT(undo()));
        plugSharedAction("edit_undo", form(), SLOT(undo()));
//2.0        plugSharedAction("edit_redo", KFormDesigner::FormManager::self(), SLOT(redo()));
        plugSharedAction("edit_redo", form(), SLOT(redo()));

//! @todo        plugSharedAction("formpart_layout_menu", KFormDesigner::FormManager::self(), 0);
//2.0        plugSharedAction("formpart_layout_hbox", KFormDesigner::FormManager::self(), SLOT(layoutHBox()));
//2.0        plugSharedAction("formpart_layout_vbox", KFormDesigner::FormManager::self(), SLOT(layoutVBox()));
//2.0        plugSharedAction("formpart_layout_grid", KFormDesigner::FormManager::self(), SLOT(layoutGrid()));
        plugSharedAction("formpart_layout_hbox", form(), SLOT(layoutHBox()));
        plugSharedAction("formpart_layout_vbox", form(), SLOT(layoutVBox()));
        plugSharedAction("formpart_layout_grid", form(), SLOT(layoutGrid()));
#ifdef KEXI_SHOW_SPLITTER_WIDGET
//2.0        plugSharedAction("formpart_layout_hsplitter", KFormDesigner::FormManager::self(), SLOT(layoutHSplitter()));
//2.0        plugSharedAction("formpart_layout_vsplitter", KFormDesigner::FormManager::self(), SLOT(layoutVSplitter()));
        plugSharedAction("formpart_layout_hsplitter", form(), SLOT(layoutHSplitter()));
        plugSharedAction("formpart_layout_vsplitter", form(), SLOT(layoutVSplitter()));
#endif
//2.0        plugSharedAction("formpart_break_layout", KFormDesigner::FormManager::self(), SLOT(breakLayout()));
        plugSharedAction("formpart_break_layout", form(), SLOT(breakLayout()));

//2.0        plugSharedAction("formpart_format_raise", KFormDesigner::FormManager::self(), SLOT(bringWidgetToFront()));
//2.0        plugSharedAction("formpart_format_lower", KFormDesigner::FormManager::self(), SLOT(sendWidgetToBack()));
        plugSharedAction("formpart_format_raise", form(), SLOT(bringWidgetToFront()));
        plugSharedAction("formpart_format_lower", form(), SLOT(sendWidgetToBack()));

//2.0        plugSharedAction("other_widgets_menu", KFormDesigner::FormManager::self(), 0);
        plugSharedAction("other_widgets_menu", form(), 0);
        setAvailable("other_widgets_menu", true);

//2.0        plugSharedAction("formpart_align_menu", KFormDesigner::FormManager::self(), 0);
        plugSharedAction("formpart_align_menu", form(), 0);
//2.0        plugSharedAction("formpart_align_to_left", KFormDesigner::FormManager::self(), SLOT(alignWidgetsToLeft()));
        plugSharedAction("formpart_align_to_left", form(), SLOT(alignWidgetsToLeft()));
//2.0        plugSharedAction("formpart_align_to_right", KFormDesigner::FormManager::self(), SLOT(alignWidgetsToRight()));
        plugSharedAction("formpart_align_to_right", form(), SLOT(alignWidgetsToRight()));
//2.0        plugSharedAction("formpart_align_to_top", KFormDesigner::FormManager::self(), SLOT(alignWidgetsToTop()));
        plugSharedAction("formpart_align_to_top", form(), SLOT(alignWidgetsToTop()));
//2.0        plugSharedAction("formpart_align_to_bottom", KFormDesigner::FormManager::self(), SLOT(alignWidgetsToBottom()));
        plugSharedAction("formpart_align_to_bottom", form(), SLOT(alignWidgetsToBottom()));
//2.0        plugSharedAction("formpart_align_to_grid", KFormDesigner::FormManager::self(), SLOT(alignWidgetsToGrid()));
        plugSharedAction("formpart_align_to_grid", form(), SLOT(alignWidgetsToGrid()));

//2.0        plugSharedAction("formpart_adjust_size_menu", KFormDesigner::FormManager::self(), 0);
        plugSharedAction("formpart_adjust_size_menu", form(), 0);
//2.0        plugSharedAction("formpart_adjust_to_fit", KFormDesigner::FormManager::self(), SLOT(adjustWidgetSize()));
        plugSharedAction("formpart_adjust_to_fit", form(), SLOT(adjustWidgetSize()));
//2.0        plugSharedAction("formpart_adjust_size_grid", KFormDesigner::FormManager::self(), SLOT(adjustSizeToGrid()));
        plugSharedAction("formpart_adjust_size_grid", form(), SLOT(adjustSizeToGrid()));
//2.0        plugSharedAction("formpart_adjust_height_small", KFormDesigner::FormManager::self(),  SLOT(adjustHeightToSmall()));
        plugSharedAction("formpart_adjust_height_small", form(),  SLOT(adjustHeightToSmall()));
//2.0        plugSharedAction("formpart_adjust_height_big", KFormDesigner::FormManager::self(), SLOT(adjustHeightToBig()));
        plugSharedAction("formpart_adjust_height_big", form(), SLOT(adjustHeightToBig()));
//2.0        plugSharedAction("formpart_adjust_width_small", KFormDesigner::FormManager::self(), SLOT(adjustWidthToSmall()));
        plugSharedAction("formpart_adjust_width_small", form(), SLOT(adjustWidthToSmall()));
//2.0        plugSharedAction("formpart_adjust_width_big", KFormDesigner::FormManager::self(), SLOT(adjustWidthToBig()));
        plugSharedAction("formpart_adjust_width_big", form(), SLOT(adjustWidthToBig()));

//2.0        plugSharedAction("format_font", KFormDesigner::FormManager::self(), SLOT(changeFont()));
        plugSharedAction("format_font", form(), SLOT(changeFont()));

        // - setup local actions
        QList<QAction*> viewActions;
        QAction* a;
        a = form()->action("edit_undo");
        a->setProperty("iconOnly", true);
        viewActions << a;
        a = form()->action("edit_redo");
        a->setProperty("iconOnly", true);
        viewActions << a;
        setViewActions(viewActions);
    }

//2.0: moved up     initForm();

    KexiDataAwareView::init(m_scrollView, m_scrollView, m_scrollView,
                            /* skip data-awarness if design mode */ viewMode() == Kexi::DesignViewMode);

    connect(this, SIGNAL(focus(bool)), this, SLOT(slotFocus(bool)));
    /// @todo skip this if ther're no borders
// m_dbform->resize( m_dbform->size()+QSize(m_scrollView->verticalScrollBar()->width(), m_scrollView->horizontalScrollBar()->height()) );
}

KexiFormView::~KexiFormView()
{
    if (m_cursor) {
        KexiDB::Connection *conn = KexiMainWindowIface::global()->project()->dbConnection();
        conn->deleteCursor(m_cursor);
        m_cursor = 0;
    }
    deleteQuery();

    // Important: form window is closed.
    // Set property set to 0 because there is *only one* instance of a property set class
    // in Kexi, so the main window wouldn't know the set in fact has been changed.
//2.0    m_propertySet = 0;
    propertySetSwitched();
}

void
KexiFormView::deleteQuery()
{
    if (m_cursor) {
        KexiDB::Connection *conn = KexiMainWindowIface::global()->project()->dbConnection();
        conn->deleteCursor(m_cursor);
        m_cursor = 0;
    }

    if (m_queryIsOwned) {
        delete m_query;
    } else {
//! @todo remove this shared query from listened queries list
    }
    m_query = 0;
}

void
KexiFormView::setForm(KFormDesigner::Form *f)
{
    if (viewMode() == Kexi::DataViewMode)
        tempData()->previewForm = f;
    else
        tempData()->form = f;
    m_form = f;
}

void KexiFormView::initForm()
{
// <here moved from ctor>
    m_dbform = new KexiDBForm(m_scrollView->viewport(), m_scrollView);
    m_dbform->setObjectName(
        i18nc("Widget name. This string will be used to name widgets of this class. "
              "It must _not_ contain white spaces and non latin1 characters.", "form"));
    QPalette pal(m_dbform->palette());
    pal.setBrush(QPalette::Window, palette().brush(QPalette::Window));
    m_dbform->setPalette(pal); // avoid inheriting QPalette::Window role
                               // from m_scrollView->viewport()
// m_dbform->resize( m_scrollView->viewport()->size() - QSize(20, 20) );
// m_dbform->resize(QSize(400, 300));
    m_scrollView->setWidget(m_dbform);
    m_scrollView->setResizingEnabled(viewMode() != Kexi::DataViewMode);

// initForm();

    if (viewMode() == Kexi::DataViewMode) {
        m_scrollView->recordNavigator()->setRecordHandler(m_scrollView);
        //m_scrollView->viewport()->setPaletteBackgroundColor(m_dbform->palette().active().background());
        QPalette pal(m_scrollView->viewport()->palette());
        pal.setBrush(m_scrollView->viewport()->backgroundRole(), 
            m_dbform->palette().brush(QPalette::Background));
        m_scrollView->viewport()->setPalette(pal);
//moved to formmanager  connect(formPart()->manager(), SIGNAL(noFormSelected()), SLOT(slotNoFormSelected()));
    }
// </here>

    setForm(
        new KFormDesigner::Form(
            KexiFormManager::self()->library(), 
            viewMode() == Kexi::DataViewMode ? KFormDesigner::Form::DataMode : KFormDesigner::Form::DesignMode,
            *KexiMainWindowIface::global()->actionCollection(), 
            *KexiFormManager::self()->widgetActionGroup())
    );
// if (viewMode()==Kexi::DataViewMode)
    //form()->setDesignMode(false);
    form()->createToplevel(m_dbform, m_dbform);

    if (viewMode() == Kexi::DesignViewMode) {
        //we want to be informed about executed commands
//2.0: moved to Form        connect(form()->commandHistory(), SIGNAL(commandExecuted(K3Command*)),
//                              form(), SLOT(slotCommandExecuted(K3Command*)));
//        connect(form(), SIGNAL(assignAction(KFormDesigner::Form*)),
//                KexiFormManager::self(), SLOT(slotHistoryCommandExecuted(Command*)));
    }

    const bool newForm = window()->id() < 0;

    KexiDB::FieldList *fields = 0;
    if (newForm) {
        // Show the form wizard if this is a new Form
#ifndef NO_DSWIZARD
        KexiDataSourceWizard *w = new KexiDataSourceWizard(
            KexiMainWindowIface::global()->thisWidget());
        if (!w->exec())
            fields = 0;
        else
            fields = w->fields();
        delete w;
#endif
    }

    if (fields) {
        QDomDocument dom;
        formPart()->generateForm(fields, dom);
        KFormDesigner::FormIO::loadFormFromDom(form(), m_dbform, dom);
        //! @todo handle errors
    } else
        loadForm();

    if (form()->autoTabStops())
        form()->autoAssignTabStops();

    //collect tab order information
    m_dbform->updateTabStopsOrder(form());

// if (m_dbform->orderedFocusWidgets()->first())
    // m_scrollView->setFocusProxy( m_dbform->orderedFocusWidgets()->first() );

#ifdef __GNUC__
#warning "TODO port initForm()"
#else
#pragma WARNING( port initForm() )
#endif
//! @todo port initForm()
//  KFormDesigner::FormManager::self()->importForm(form(), viewMode() == Kexi::DataViewMode);
    if (viewMode() == Kexi::DesignViewMode) {
//2.0        initForm(form());
        //from FormManager::initForm(Form *form)
//2.0    m_forms.append(form);
//2.0    if (m_treeview)
//2.0       m_treeview->setForm(form);
//2.0    m_active = form;
//2.0    connect(form, SIGNAL(selectionChanged(QWidget*, bool, bool)),
//2.0            m_propSet, SLOT(setSelectedWidgetWithoutReload(QWidget*, bool, bool)));
/*2.0    if (m_treeview) {
        connect(form, SIGNAL(selectionChanged(QWidget*, bool, bool)),
                m_treeview, SLOT(setSelectedWidget(QWidget*, bool)));
        connect(form, SIGNAL(childAdded(ObjectTreeItem*)), m_treeview, SLOT(addItem(ObjectTreeItem*)));
        connect(form, SIGNAL(childRemoved(ObjectTreeItem*)), m_treeview, SLOT(removeItem(ObjectTreeItem*)));
    }
    connect(m_propSet, SIGNAL(widgetNameChanged(const QByteArray&, const QByteArray&)),
            form, SLOT(changeName(const QByteArray&, const QByteArray&)));
*/
//2.0    form->setSelectedWidget(form->widget());
    form()->selectWidget(form()->widget()); //added in 2.0
//2.0    windowChanged(form->widget());
        //end of: from FormManager::initForm(Form *form)
    }
    else {
//2.0 rm        m_preview.append(form);
//2.0        form()->setDesignMode(false);
        form()->setMode(KFormDesigner::Form::DataMode);
    }
    m_scrollView->setForm(form());

// m_dbform->updateTabStopsOrder(form());
// QSize s = m_dbform->size();
// QApplication::sendPostedEvents();
// m_scrollView->resize( s );
// m_dbform->resize(s);
    m_scrollView->refreshContentsSize();
// m_scrollView->refreshContentsSizeLater(true,true);

    if (newForm && !fields) {
        /* Our form's area will be resized more than once.
        Let's resize form widget itself later. */
        m_delayedFormContentsResizeOnShow = 3;
    }

    slotPropertySetSwitched(); // this prepares the data source page
    updateDataSourcePage();

    if (!newForm && viewMode() == Kexi::DesignViewMode) {
        form()->clearUndoStack();
    }
}

void KexiFormView::updateAutoFieldsDataSource()
{
//! @todo call this when form's data source is changed
    //update autofields:
    //-inherit captions
    //-inherit data types
    //(this data has not been stored in the form)
    QString dataSourceString(m_dbform->dataSource());
    QString dataSourcePartClassString(m_dbform->dataSourcePartClass());
    KexiDB::Connection *conn = KexiMainWindowIface::global()->project()->dbConnection();
    KexiDB::TableOrQuerySchema tableOrQuery(
        conn, dataSourceString.toLatin1(), dataSourcePartClassString == "org.kexi-project.table");
    if (!tableOrQuery.table() && !tableOrQuery.query())
        return;
    foreach (KFormDesigner::ObjectTreeItem *item, *form()->objectTree()->hash()) {
        KexiDBAutoField *afWidget = dynamic_cast<KexiDBAutoField*>(item->widget());
        if (afWidget) {
            KexiDB::QueryColumnInfo *colInfo = tableOrQuery.columnInfo(afWidget->dataSource());
            if (colInfo) {
                afWidget->setColumnInfo(colInfo);
                //setFieldTypeInternal((int)colInfo->field->type());
                //afWidget->setFieldCaptionInternal(colInfo->captionOrAliasOrName());
            }
        }
    }
}

void KexiFormView::updateValuesForSubproperties()
{
//! @todo call this when form's data source is changed
    //update autofields:
    //-inherit captions
    //-inherit data types
    //(this data has not been stored in the form)
    QString dataSourceString(m_dbform->dataSource());
    QString dataSourcePartClassString(m_dbform->dataSourcePartClass());
    KexiDB::Connection *conn = KexiMainWindowIface::global()->project()->dbConnection();
    KexiDB::TableOrQuerySchema tableOrQuery(
        conn, dataSourceString.toLatin1(), dataSourcePartClassString == "org.kexi-project.table");
    if (!tableOrQuery.table() && !tableOrQuery.query())
        return;

    foreach (KFormDesigner::ObjectTreeItem *item, *form()->objectTree()->hash()) {
        // (delayed) set values for subproperties
//! @todo this could be at the KFD level, but KFD is going to be merged anyway with kexiforms, right?
        KFormDesigner::WidgetWithSubpropertiesInterface* subpropIface
            = dynamic_cast<KFormDesigner::WidgetWithSubpropertiesInterface*>(item->widget());
        if (subpropIface && subpropIface->subwidget() && item->subproperties()) {
            QWidget *subwidget = subpropIface->subwidget();
            QHash<QString, QVariant>* subprops = item->subproperties();
            for (QHash<QString, QVariant>::const_iterator subpropIt = subprops->constBegin(); 
                subpropIt != subprops->constEnd(); ++subpropIt)
            {
                kDebug() << "delayed setting of the subproperty: widget="
                    << item->widget()->objectName() << " prop=" << subpropIt.key() << " val="
                    << subpropIt.value();

                QMetaProperty meta = KexiUtils::findPropertyWithSuperclasses(
                                         subwidget, subpropIt.key().toLatin1().constData());
                if (meta.isValid()) {
                    // Special case: the property value of type enum (set) but is saved as a string list,
                    // not as int, so we need to translate it to int. It's been created as such
                    // by FormIO::readPropertyValue(). Example: "alignment" property.
                    if (meta.isEnumType() && subpropIt.value().type() == QVariant::StringList) {
                        const QByteArray keysCombined(subpropIt.value().toStringList().join("|").toLatin1());
                        subwidget->setProperty(subpropIt.key().toLatin1(),
                                               meta.enumerator().keysToValue(keysCombined.constData()));
                    } else {
                        subwidget->setProperty(subpropIt.key().toLatin1(), subpropIt.value());
                    }
                }
            }//for
        }
    }
}

//! Used in KexiFormView::loadForm()
static void setUnsavedBLOBIdsForDataViewMode(
    QWidget* widget, const QHash<QByteArray, KexiBLOBBuffer::Id_t>& unsavedLocalBLOBsByName)
{
    if (-1 != KexiUtils::indexOfPropertyWithSuperclasses(widget, "pixmapId")) {
        const KexiBLOBBuffer::Id_t blobID
        = unsavedLocalBLOBsByName.value(widget->objectName().toLatin1());
        if (blobID > 0)
            widget->setProperty(
                "pixmapId",
                (uint /* KexiBLOBBuffer::Id_t is unsafe and unsupported by QVariant - will be fixed in Qt4*/)blobID);
    }
    const QList<QWidget*> list(widget->findChildren<QWidget*>());
    if (list.isEmpty())
        return;
    foreach(QWidget *w, list) {
        setUnsavedBLOBIdsForDataViewMode(w, unsavedLocalBLOBsByName);
    }
}

void
KexiFormView::loadForm()
{
//@todo also load m_resizeMode !

    kDebug() << "Loading the form with id : " << window()->id();
    // If we are previewing the Form, use the tempData instead of the form stored in the db
    if (viewMode() == Kexi::DataViewMode && !tempData()->tempForm.isNull()) {
        KFormDesigner::FormIO::loadFormFromString(form(), m_dbform, tempData()->tempForm);
        setUnsavedBLOBIdsForDataViewMode(m_dbform, tempData()->unsavedLocalBLOBsByName);
        updateAutoFieldsDataSource();
        updateValuesForSubproperties();
        return;
    }

    // normal load
    QString data;
    loadDataBlock(data);
    KFormDesigner::FormIO::loadFormFromString(form(), m_dbform, data);

    //"autoTabStops" property is loaded -set it within the form tree as well
    form()->setAutoTabStops(m_dbform->autoTabStops());

    updateAutoFieldsDataSource();
    updateValuesForSubproperties();
}

void
KexiFormView::slotPropertySetSwitched()
{
#if 0 //2.0
// if (set && window()!=KexiMainWindowIface::global()->currentWindow())
    if (form() != KFormDesigner::FormManager::self()->activeForm())
        return; //this is not the current form view
//2.0    m_propertySet = set;
#endif
    QWidgetList* widgets = form()->selectedWidgets();
//    delete m_propertySet;
//2.0    m_propertySet = 0;
    //todo
//      KFormDesigner::WidgetPropertySet
/* 2.0
    if (forceReload) {
        propertySetReloaded(true//preservePrevSelection
        , propertyToSelect);
    }
    else {*/
//2.0: we're sharing the same Set object so use propertySetReloaded() instead
//        propertySetSwitched();
    propertySetReloaded();
/*    }*/
    formPart()->dataSourcePage()->assignPropertySet(&form()->propertySet());
}

tristate
KexiFormView::beforeSwitchTo(Kexi::ViewMode mode, bool &dontStore)
{
    if (mode != viewMode()) {
        if (viewMode() == Kexi::DataViewMode) {
            if (!m_scrollView->acceptRowEdit())
                return cancelled;

            m_scrollView->beforeSwitchView();
        } else {
            //remember our pos
            tempData()->scrollViewContentsPos
                = QPoint(m_scrollView->contentsX(), m_scrollView->contentsY());
        }
    }

    // we don't store on db, but in our TempData
    dontStore = true;
    if (isDirty() && (mode == Kexi::DataViewMode) && form()->objectTree()) {
        KexiFormPart::TempData* temp = tempData();
        if (!KFormDesigner::FormIO::saveFormToString(form(), temp->tempForm))
            return false;

        //collect blobs from design mode by name for use in data view mode
        temp->unsavedLocalBLOBsByName.clear();
        for (QHash<QWidget*, KexiBLOBBuffer::Id_t>::const_iterator it
                = temp->unsavedLocalBLOBs.constBegin();
                it != temp->unsavedLocalBLOBs.constEnd(); ++it) {
            if (!it.key())
                continue;
            temp->unsavedLocalBLOBsByName.insert(it.key()->objectName().toLatin1(), it.value());
        }
    }

    return true;
}

tristate KexiFormView::afterSwitchFrom(Kexi::ViewMode mode)
{
    if (mode == 0 || mode == Kexi::DesignViewMode) {
        if (window()->neverSaved()) {
            m_dbform->resize(QSize(400, 300));
            m_scrollView->refreshContentsSizeLater(true, true);
            //m_delayedFormContentsResizeOnShow = false;
        }
    }

    if (mode != 0 && mode != Kexi::DesignViewMode) {
        //preserve contents pos after switching to other view
        m_scrollView->setContentsPos(tempData()->scrollViewContentsPos.x(),
                                     tempData()->scrollViewContentsPos.y());
    }
// if (mode == Kexi::DesignViewMode) {
    //m_scrollView->move(0,0);
    //m_scrollView->setContentsPos(0,0);
    //m_scrollView->moveChild(m_dbform, 0, 0);
// }

    if ((mode == Kexi::DesignViewMode) && viewMode() == Kexi::DataViewMode) {
        // The form may have been modified, so we must recreate the preview
        delete m_dbform; // also deletes form()
        initForm();
/* moved to initForm()
        m_dbform = new KexiDBForm(m_scrollView->viewport(), m_scrollView);
        m_dbform->setObjectName(
            i18nc("Widget name. This string will be used to name widgets of this class. "
                  "It must _not_ contain white spaces and non latin1 characters.", "form"));
        QPalette pal(m_dbform->palette());
        pal.setBrush(QPalette::Window, palette().brush(QPalette::Window));
        m_dbform->setPalette(pal); // avoid inheriting QPalette::Window role
                                   // from m_scrollView->viewport()

        initForm();
        */
//moved to formmanager  slotNoFormSelected();

        //reset position
        m_scrollView->setContentsPos(0, 0);
        m_dbform->move(0, 0);

    }

    //update tab stops if needed
    if (viewMode() == Kexi::DataViewMode) {
//  //propagate current "autoTabStops" property value to the form tree
//  form()->setAutoTabStops( m_dbform->autoTabStops() );

//  if(form()->autoTabStops())
//   form()->autoAssignTabStops();
    } else {
        //set "autoTabStops" property
        m_dbform->setAutoTabStops(form()->autoTabStops());
    }

    if (viewMode() == Kexi::DataViewMode) {
//TMP!!
        initDataSource();

        //handle events for this form
        m_scrollView->setMainWidgetForEventHandling(m_dbform);

        //set focus on 1st focusable widget which has valid dataSource property set
        QList<QWidget*> *orderedFocusWidgets = m_dbform->orderedFocusWidgets();
        if (!orderedFocusWidgets->isEmpty()) {
//   QWidget *www = focusWidget();
            //if (Kexi::hasParent(this, qApp->focusWidget())) {
            KexiUtils::unsetFocusWithReason(QApplication::focusWidget(), Qt::TabFocusReason);
            //}

            QWidget *widget;
            foreach(widget, *orderedFocusWidgets) {
                KexiFormDataItemInterface *iface = dynamic_cast<KexiFormDataItemInterface*>(widget);
                if (iface)
                    kDebug() << iface->dataSource();
                if (iface && iface->columnInfo() && !iface->isReadOnly()
                        /*! @todo add option for skipping autoincremented fields */
                        /* also skip autoincremented fields:*/
                        && !iface->columnInfo()->field->isAutoIncrement()) { //!iface->dataSource().isEmpty()
                    break;
                }
            }
            if (!widget) //eventually, focus first available widget if nothing other is available
                widget = orderedFocusWidgets->first();

            widget->setFocus();
            KexiUtils::setFocusWithReason(widget, Qt::TabFocusReason);
            m_setFocusInternalOnce = widget;
        }

        if (m_query)
            m_scrollView->selectFirstRow();
    }

    //dirty only if it's a new object
    if (mode == 0)
        setDirty(window()->partItem()->neverSaved());

    if (mode == Kexi::DataViewMode && viewMode() == Kexi::DesignViewMode) {
//  slotPropertySetSwitched
//  emit KFormDesigner::FormManager::self()->propertySetSwitched( KFormDesigner::FormManager::self()->propertySet()->set(), true );
    }

    return true;
}

void KexiFormView::initDataSource()
{
    deleteQuery();
    QString dataSourceString(m_dbform->dataSource());
    QString dataSourcePartClassString(m_dbform->dataSourcePartClass());
//! @todo also handle anonymous (not stored) queries provided as statements here
    bool ok = !dataSourceString.isEmpty();

    /*   if (m_previousDataSourceString.toLower()==dataSourceString.toLower() && !m_cursor) {
          //data source changed: delete previous cursor
          m_conn->deleteCursor(m_cursor);
          m_cursor = 0;
        }*/

    KexiDB::TableSchema *tableSchema = 0;
    KexiDB::Connection *conn = 0;
    QStringList sources;
    bool forceReadOnlyDataSource = false;

    if (ok) {
//  m_previousDataSourceString = dataSourceString;

        //collect all data-aware widgets and create query schema
        m_scrollView->setMainDataSourceWidget(m_dbform);
        sources = m_scrollView->usedDataSources();
        conn = KexiMainWindowIface::global()->project()->dbConnection();
        if (dataSourcePartClassString.isEmpty() /*table type is the default*/
                || dataSourcePartClassString == "org.kexi-project.table") {
            tableSchema = conn->tableSchema(dataSourceString);
            if (tableSchema) {
                /* We will build a _minimum_ query schema from selected table fields. */
                m_query = new KexiDB::QuerySchema();
                m_queryIsOwned = true;

                if (dataSourcePartClassString.isEmpty())
                    m_dbform->setDataSourcePartClass("org.kexi-project.table"); //update for compatibility
            }
        }

        if (!tableSchema) {
            if (dataSourcePartClassString.isEmpty() /*also try to find a query (for compatibility with Kexi<=0.9)*/
                    || dataSourcePartClassString == "org.kexi-project.query") {
                //try to find predefined query schema.
                //Note: In general, we could not skip unused fields within this query because
                //      it can have GROUP BY clause.
                //! @todo check if the query could have skipped unused fields (no GROUP BY, no joins, etc.)
                m_query = conn->querySchema(dataSourceString);
                m_queryIsOwned = false;
                ok = m_query != 0;
                if (ok && dataSourcePartClassString.isEmpty())
                    m_dbform->setDataSourcePartClass("org.kexi-project.query"); //update for compatibility
                // query results are read-only
//! @todo There can be read-write queries, e.g. simple "SELECT * FROM...". Add a checking function to KexiDB.
                forceReadOnlyDataSource = true;
            } else //no other classes are supported
                ok = false;
        }
    }

    QSet<QString> invalidSources;
    if (ok) {
        KexiDB::IndexSchema *pkey = tableSchema ? tableSchema->primaryKey() : 0;
        if (pkey) {
            //always add all fields from table's primary key
            // (don't worry about duplicates, unique list will be computed later)
            sources += pkey->names();
            kDebug() << "pkey added to data sources: " << pkey->names();
        }
        kDebug() << "sources=" << sources;

        uint index = 0;
        for (QStringList::ConstIterator it = sources.constBegin();
                it != sources.constEnd(); ++it, index++) {
            /*! @todo add expression support */
            QString fieldName((*it).toLower());
            //remove "tablename." if it was prepended
            if (tableSchema && fieldName.startsWith(tableSchema->name().toLower() + "."))
                fieldName = fieldName.mid(tableSchema->name().length() + 1);
            //remove "queryname." if it was prepended
            if (!tableSchema && fieldName.startsWith(m_query->name().toLower() + "."))
                fieldName = fieldName.mid(m_query->name().length() + 1);
            KexiDB::Field *f = tableSchema ? tableSchema->field(fieldName) : m_query->field(fieldName);
            if (!f) {
                /*! @todo show error */
                //remove this widget from the set of data widgets in the provider
                /*! @todo fieldName is ok, but what about expressions? */
                invalidSources.insert(fieldName);
                kDebug() << "invalidSources+=" << index << " (" << (*it) << ")";
                continue;
            }
            if (tableSchema) {
                if (!m_query->hasField(f)) {
                    //we're building a new query: add this field
                    m_query->addField(f);
                }
            }
        }
        if (invalidSources.count() == sources.count()) {
            //all data sources are invalid! don't execute the query
            deleteQuery();
        } else {
            KexiDB::debug(m_query->parameters());
            // like in KexiQueryView::executeQuery()
            QList<QVariant> params;
            {
                KexiUtils::WaitCursorRemover remover;
                params = KexiQueryParameters::getParameters(this, *conn->driver(), *m_query, ok);
            }
            if (ok) //input cancelled
                m_cursor = conn->executeQuery(*m_query, params);
        }
        m_scrollView->invalidateDataSources(invalidSources, m_query);
        ok = m_cursor != 0;
    }

    if (!invalidSources.isEmpty())
        m_dbform->updateTabStopsOrder();

    if (ok) {
//! @todo PRIMITIVE!! data setting:
//! @todo KexiTableViewData is not great name for data class here... rename/move?
        KexiTableViewData* data = new KexiTableViewData(m_cursor);
        if (forceReadOnlyDataSource)
            data->setReadOnly(true);
        data->preloadAllRows();

///*! @todo few backends return result count for free! - no need to reopen() */
//   int resultCount = -1;
//   if (ok) {
//    resultCount = m_conn->resultCount(m_conn->selectStatement(*m_query));
//    ok = m_cursor->reopen();
//   }
//   if (ok)
//    ok = ! (!m_cursor->moveFirst() && m_cursor->error());

        m_scrollView->setData(data, true /*owner*/);
    } else
        m_scrollView->setData(0, false);
}

void KexiFormView::slotModified()
{
    KexiView::setDirty(form()->isModified());
}

KexiDB::SchemaData*
KexiFormView::storeNewData(const KexiDB::SchemaData& sdata, bool &cancel)
{
    KexiDB::SchemaData *s = KexiView::storeNewData(sdata, cancel);
    kDebug() << "new id:" << s->id();

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
}

tristate
KexiFormView::storeData(bool dontAsk)
{
    Q_UNUSED(dontAsk);
    kDebug() << window()->partItem()->name() << " [" << window()->id() << "]";

    //-- first, store local BLOBs, so identifiers can be updated
//! @todo remove unused data stored previously
    KexiDB::Connection *conn = KexiMainWindowIface::global()->project()->dbConnection();
    KexiDB::TableSchema *blobsTable = conn->tableSchema("kexi__blobs");
    if (!blobsTable) { //compatibility check for older Kexi project versions
//! @todo show message about missing kexi__blobs?
        return false;
    }
    // Not all engines accept passing NULL to PKEY o_id, so we're omitting it.
    QStringList blobsFieldNamesWithoutID(blobsTable->names());
    blobsFieldNamesWithoutID.pop_front();
    KexiDB::FieldList *blobsFieldsWithoutID = blobsTable->subList(blobsFieldNamesWithoutID);

    KexiDB::PreparedStatement::Ptr st = conn->prepareStatement(
                                            KexiDB::PreparedStatement::InsertStatement, *blobsFieldsWithoutID);
    if (!st) {
        delete blobsFieldsWithoutID;
        //! @todo show message
        return false;
    }
    KexiBLOBBuffer *blobBuf = KexiBLOBBuffer::self();
    KexiFormView *designFormView = dynamic_cast<KexiFormView*>(
        window()->viewForMode(Kexi::DesignViewMode));
    if (designFormView) {
        for (QHash<QWidget*, KexiBLOBBuffer::Id_t>::const_iterator it
                = tempData()->unsavedLocalBLOBs.constBegin();
                it != tempData()->unsavedLocalBLOBs.constEnd(); ++it) {
            if (!it.key()) {
                kWarning() << "it.key()==0 !";
                continue;
            }
            kDebug() << "name=" << it.key()->objectName() << " dataID=" << it.value();
            KexiBLOBBuffer::Handle h(blobBuf->objectForId(it.value(), /*!stored*/false));
            if (!h)
                continue; //no BLOB assigned

            QString originalFileName(h.originalFileName());
            QFileInfo fi(originalFileName);
            QString caption(fi.baseName().replace('_', " ").simplified());

            if (st) {
                *st /* << NO, (pgsql doesn't support this):QVariant()*/ /*id*/
                    << h.data() << originalFileName << caption
                    << h.mimeType() << (uint)/*! @todo unsafe */h.folderId();
                if (!st->execute()) {
                    delete blobsFieldsWithoutID;
                    kDebug() << "execute error";
                    return false;
                }
            }
            delete blobsFieldsWithoutID;
            blobsFieldsWithoutID = 0;
            const quint64 storedBLOBID = conn->lastInsertedAutoIncValue("o_id", "kexi__blobs");
            if ((quint64) - 1 == storedBLOBID) {
                //! @todo show message?
                return false;
            }
            kDebug() << "storedDataID=" << storedBLOBID;
            h.setStoredWidthID((KexiBLOBBuffer::Id_t /*unsafe - will be fixed in Qt4*/)storedBLOBID);
            //set widget's internal property so it can be saved...
            const QVariant oldStoredPixmapId(it.key()->property("storedPixmapId"));
            it.key()->setProperty("storedPixmapId",
                                  QVariant((uint /* KexiBLOBBuffer::Id_t is unsafe and unsupported by QVariant - will be fixed in Qt4*/)storedBLOBID));
            KFormDesigner::ObjectTreeItem *widgetItem
            = designFormView->form()->objectTree()->lookup(it.key()->objectName());
            if (widgetItem)
                widgetItem->addModifiedProperty("storedPixmapId", oldStoredPixmapId);
            else
                kWarning() << "no '" << widgetItem->name() << "' widget found within a form";
        }
    }

    //-- now, save form's XML
    QString data;
    if (!KFormDesigner::FormIO::saveFormToString(tempData()->form, data))
        return false;
    if (!storeDataBlock(data))
        return false;

    //all blobs are now saved
    tempData()->unsavedLocalBLOBs.clear();

    tempData()->tempForm.clear();
    return true;
}

#if 0
/// Action stuff /////////////////
void
KexiFormView::slotWidgetSelected(KFormDesigner::Form *f, bool multiple)
{
    if (f != form())
        return;

    enableFormActions();
    // Enable edit actions
    setAvailable("edit_copy", true);
    setAvailable("edit_cut", true);
    setAvailable("edit_clear", true);

    // 'Align Widgets' menu
    setAvailable("formpart_align_menu", multiple);
    setAvailable("formpart_align_to_left", multiple);
    setAvailable("formpart_align_to_right", multiple);
    setAvailable("formpart_align_to_top", multiple);
    setAvailable("formpart_align_to_bottom", multiple);

    setAvailable("formpart_adjust_size_menu", true);
    setAvailable("formpart_adjust_width_small", multiple);
    setAvailable("formpart_adjust_width_big", multiple);
    setAvailable("formpart_adjust_height_small", multiple);
    setAvailable("formpart_adjust_height_big", multiple);

    setAvailable("formpart_format_raise", true);
    setAvailable("formpart_format_lower", true);

    // If the widgets selected is a container, we enable layout actions
    if (!multiple) {
        KFormDesigner::ObjectTreeItem *item = f->objectTree()->lookup(f->selectedWidgets()->first()->name());
        if (item && item->container())
            multiple = true;
    }
    // Layout actions
    setAvailable("formpart_layout_hbox", multiple);
    setAvailable("formpart_layout_vbox", multiple);
    setAvailable("formpart_layout_grid", multiple);

    KFormDesigner::Container *container = f->activeContainer();
    setAvailable("formpart_break_layout", container ?
                 (container->layoutType() != KFormDesigner::Container::NoLayout) : false);
}

void
KexiFormView::slotFormWidgetSelected(KFormDesigner::Form *f)
{
    if (f != form())
        return;

    disableWidgetActions();
    enableFormActions();

    // Layout actions
    setAvailable("formpart_layout_hbox", true);
    setAvailable("formpart_layout_vbox", true);
    setAvailable("formpart_layout_grid", true);
    setAvailable("formpart_break_layout", (f->toplevelContainer()->layoutType() != KFormDesigner::Container::NoLayout));
}

void
KexiFormView::slotNoFormSelected() // == form in preview mode
{
    disableWidgetActions();

    // Disable paste action
    setAvailable("edit_paste", false);
    setAvailable("edit_undo", false);
    setAvailable("edit_redo", false);

    // Disable 'Tools' actions
    setAvailable("formpart_pixmap_collection", false);
    setAvailable("formpart_connections", false);
    setAvailable("formpart_taborder", false);
    setAvailable("formpart_change_style", false);
}

void
KexiFormView::enableFormActions()
{
    // Enable 'Tools' actions
    setAvailable("formpart_pixmap_collection", true);
    setAvailable("formpart_connections", true);
    setAvailable("formpart_taborder", true);

#ifdef __GNUC__
#warning "Port this.."
#else
#pragma WARNING( Port this.. )
#endif
//! @todo    setAvailable("edit_paste", KFormDesigner::FormManager::self()->isPasteEnabled());
}

void
KexiFormView::disableWidgetActions()
{
    // Disable edit actions
    setAvailable("edit_copy", false);
    setAvailable("edit_cut", false);
    setAvailable("edit_clear", false);

    // Disable format functions
    setAvailable("formpart_align_menu", false);
    setAvailable("formpart_align_to_left", false);
    setAvailable("formpart_align_to_right", false);
    setAvailable("formpart_align_to_top", false);
    setAvailable("formpart_align_to_bottom", false);

    setAvailable("formpart_adjust_size_menu", false);
    setAvailable("formpart_adjust_width_small", false);
    setAvailable("formpart_adjust_width_big", false);
    setAvailable("formpart_adjust_height_small", false);
    setAvailable("formpart_adjust_height_big", false);

    setAvailable("formpart_format_raise", false);
    setAvailable("formpart_format_lower", false);

    setAvailable("formpart_layout_hbox", false);
    setAvailable("formpart_layout_vbox", false);
    setAvailable("formpart_layout_grid", false);
    setAvailable("formpart_break_layout", false);
}

void
KexiFormView::setUndoEnabled(bool enabled)
{
    setAvailable("edit_undo", enabled);
}

void
KexiFormView::setRedoEnabled(bool enabled)
{
    setAvailable("edit_redo", enabled);
}
#endif //0

QSize
KexiFormView::preferredSizeHint(const QSize& otherSize)
{
    if (window()->neverSaved()) {
        //ignore otherSize if possible
//  return KexiView::preferredSizeHint( (window() && window()->mdiParent()) ? QSize(10000,10000) : otherSize);
    }

    return (m_dbform->size()
            + QSize(m_scrollView->verticalScrollBar()->isVisible() ? m_scrollView->verticalScrollBar()->width()*3 / 2 : 10,
                    m_scrollView->horizontalScrollBar()->isVisible() ? m_scrollView->horizontalScrollBar()->height()*3 / 2 : 10))
           .expandedTo(KexiView::preferredSizeHint(otherSize));
}

void
KexiFormView::resizeEvent(QResizeEvent *e)
{
    if (viewMode() == Kexi::DataViewMode) {
        m_scrollView->refreshContentsSizeLater(
            e->size().width() != e->oldSize().width(),
            e->size().height() != e->oldSize().height()
        );
    }
    KexiView::resizeEvent(e);
    m_scrollView->updateNavPanelGeometry();
    if (m_delayedFormContentsResizeOnShow > 0) { // && isVisible()) {
        m_delayedFormContentsResizeOnShow--;
        m_dbform->resize(e->size() - QSize(30, 30));
    }
}

void
KexiFormView::setFocusInternal()
{
    if (viewMode() == Kexi::DataViewMode) {
        if (m_dbform->focusWidget()) {
            //better-looking focus
            if (m_setFocusInternalOnce) {
                KexiUtils::setFocusWithReason(m_setFocusInternalOnce, Qt::OtherFocusReason);
                m_setFocusInternalOnce = 0;
            } else {
                //ok? SET_FOCUS_USING_REASON(m_dbform->focusWidget(), QFocusEvent::Other);//Tab);
            }
            return;
        }
    }
    QWidget::setFocus();
}

void
KexiFormView::show()
{
    KexiDataAwareView::show();

//moved from KexiFormScrollView::show():

    //now get resize mode settings for entire form
    // if (resizeMode() == KexiFormView::ResizeAuto)
    if (viewMode() == Kexi::DataViewMode) {
        if (resizeMode() == KexiFormView::ResizeAuto)
            m_scrollView->setResizePolicy(Q3ScrollView::AutoOneFit);
    }
}

void
KexiFormView::slotFocus(bool in)
{
#ifdef __GNUC__
#warning "Port this.."
#else
#pragma WARNING( Port this.. )
#endif
/* todo
    if (in && form() && KFormDesigner::FormManager::self() && KFormDesigner::FormManager::self()->activeForm() != form()) {
        KFormDesigner::FormManager::self()->windowChanged(m_dbform);
        updateDataSourcePage();
    }*/
}

void
KexiFormView::updateDataSourcePage()
{
    if (viewMode() == Kexi::DesignViewMode) {
        QString dataSourcePartClass, dataSource;
        KoProperty::Set &set = form()->propertySet();
        dataSourcePartClass = set.propertyValue("dataSourcePartClass").toString();
        dataSource = set.propertyValue("dataSource").toString();
        formPart()->dataSourcePage()->setFormDataSource(dataSourcePartClass, dataSource);
    }
}

void
KexiFormView::slotHandleDragMoveEvent(QDragMoveEvent* e)
{
    if (KexiFieldDrag::canDecodeMultiple(e)) {
        e->accept(true);
        //dirty: drawRect(QRect( e->pos(), QSize(50, 20)), 2);
    }
}

void
KexiFormView::slotHandleDropEvent(QDropEvent* e)
{
    const QWidget *targetContainerWidget = dynamic_cast<const QWidget*>(sender());
    KFormDesigner::ObjectTreeItem *targetContainerWidgetItem = targetContainerWidget
            ? form()->objectTree()->lookup(targetContainerWidget->objectName()) : 0;
    if (targetContainerWidgetItem && targetContainerWidgetItem->container()
            && KexiFieldDrag::canDecodeMultiple(e)) {
        QString sourcePartClass, sourceName;
        QStringList fields;
        if (!KexiFieldDrag::decodeMultiple(e, sourcePartClass, sourceName, fields))
            return;
        insertAutoFields(sourcePartClass, sourceName, fields,
                         targetContainerWidgetItem->container(), e->pos());
    }
}

void
KexiFormView::insertAutoFields(const QString& sourcePartClass, const QString& sourceName,
                               const QStringList& fields, KFormDesigner::Container* targetContainer, const QPoint& _pos)
{
    if (fields.isEmpty())
        return;

    KexiDB::Connection *conn = KexiMainWindowIface::global()->project()->dbConnection();
    KexiDB::TableOrQuerySchema tableOrQuery(conn, sourceName.toLatin1(),
                                            sourcePartClass == "org.kexi-project.table");
    if (!tableOrQuery.table() && !tableOrQuery.query()) {
        kWarning() << "no such table/query \"" << sourceName << "\"";
        return;
    }

    QPoint pos(_pos);
    //if pos is not specified, compute a new position:
    if (pos == QPoint(-1, -1)) {
        if (m_widgetGeometryForRecentInsertAutoFields.isValid()) {
            pos = m_widgetGeometryForRecentInsertAutoFields.bottomLeft()
                  + QPoint(0, form()->gridSize());
        } else {
            pos = QPoint(40, 40); //start here
        }
    }

    // there will be many actions performed, do not update property pane until all that's finished
#ifdef __GNUC__
#warning "TODO port blockPropertyEditorUpdating?"
#else
#pragma WARNING( port blockPropertyEditorUpdating? )
#endif
//! @todo 2.0?    KFormDesigner::FormManager::self()->blockPropertyEditorUpdating(this);

//! todo unnamed query colums are not supported

//  KFormDesigner::WidgetList* prevSelection = form()->selectedWidgets();
    QWidgetList widgetsToSelect;
    KFormDesigner::PropertyCommandGroup *group = new KFormDesigner::PropertyCommandGroup(
//        *form(),
        fields.count() == 1
        ? i18n("Insert AutoField widget") : i18n("Insert %1 AutoField widgets", fields.count())
    );

    foreach(const QString& field, fields) {
        KexiDB::QueryColumnInfo* column = tableOrQuery.columnInfo(field);
        if (!column) {
            kWarning() << "no such field \"" << field 
                << "\" in table/query \"" << sourceName << "\"";
            continue;
        }
//! todo add autolabel using field's caption or name
        //KFormDesigner::Container *targetContainer;
        /*  QWidget* targetContainerWidget = QApplication::widgetAt(pos, true);
            while (targetContainerWidget
              && !dynamic_cast<KFormDesigner::Container*>(targetContainerWidget))
            {
              targetContainerWidget = targetContainerWidget->parentWidget();
            }
            if (dynamic_cast<KFormDesigner::Container*>(targetContainerWidget))
              targetContainer = dynamic_cast<KFormDesigner::Container*>(targetContainerWidget);
            else
              targetContainer = form()->toplevelContainer();*/
        KFormDesigner::InsertWidgetCommand *insertCmd = new KFormDesigner::InsertWidgetCommand(
            *targetContainer,
//! @todo this is hardcoded!
            "KexiDBAutoField",
//! @todo this name can be invalid for expressions: if so, fall back to a default class' prefix!
            pos, column->aliasOrName(),
            group
        );
        insertCmd->redo();
//2.0        group->addCommand(insertCmd, false/*don't exec twice*/);

        KFormDesigner::ObjectTreeItem *newWidgetItem
            = form()->objectTree()->hash()->value(insertCmd->widgetName());
        KexiDBAutoField* newWidget = newWidgetItem 
            ? dynamic_cast<KexiDBAutoField*>(newWidgetItem->widget()) : 0;
        widgetsToSelect.append(newWidget);
//#if 0
        KFormDesigner::PropertyCommandGroup *subGroup
            = new KFormDesigner::PropertyCommandGroup(
//2.0                *form(),
                QString(),
                group);
        QHash<QByteArray, QVariant> propValues;
        propValues.insert("dataSource", column->aliasOrName());
        propValues.insert("fieldTypeInternal", (int)column->field->type());
        propValues.insert("fieldCaptionInternal", column->captionOrAliasOrName());
        form()->createPropertyCommandsInDesignMode(
            newWidget, propValues, subGroup, false/*!addToActiveForm*/);
//2.0            true /*!execFlagForSubCommands*/);
        subGroup->redo();
//2.0        group->addCommand(subGroup, false/*will not be executed on CommandGroup::execute()*/);

//#endif
        //set data source and caption
        //-we don't need to use PropertyCommand here beacause we don't need UNDO
        // for these single commands
//  newWidget->setDataSource(column->aliasOrName());
//  newWidget->setFieldTypeInternal((int)column->field->type());
//  newWidget->setFieldCaptionInternal(column->captionOrAliasOrName());
        //resize again because autofield's type changed what can lead to changed sizeHint()
//  newWidget->resize(newWidget->sizeHint());
        QWidgetList list;
        list.append(newWidget);
        KFormDesigner::AdjustSizeCommand *adjustCommand
            = new KFormDesigner::AdjustSizeCommand(
                *form(), KFormDesigner::AdjustSizeCommand::SizeToFit,
                list,
                group);
        adjustCommand->redo();
//2.0        group->addCommand(adjustCommand,
//2.0                          false/*will not be executed on CommandGroup::execute()*/
//2.0                         );

        if (newWidget) {//move position down for next widget
            pos.setY(pos.y() + newWidget->height() + form()->gridSize());
        }
    }
    if (widgetsToSelect.last()) {
        //resize form if needed
        QRect oldFormRect(m_dbform->geometry());
        QRect newFormRect(oldFormRect);
        newFormRect.setWidth(qMax(m_dbform->width(), widgetsToSelect.last()->geometry().right() + 1));
        newFormRect.setHeight(qMax(m_dbform->height(), widgetsToSelect.last()->geometry().bottom() + 1));
        if (newFormRect != oldFormRect) {
            //1. resize by hand
            m_dbform->setGeometry(newFormRect);
            //2. store information about resize
            KFormDesigner::PropertyCommand *resizeFormCommand = new KFormDesigner::PropertyCommand(
                *form(), m_dbform->objectName().toLatin1(), oldFormRect, newFormRect, "geometry",
                group);
//2.0            group->addCommand(resizeFormCommand, true/*will be executed on CommandGroup::execute()*/);
        }

        //remember geometry of the last inserted widget
        m_widgetGeometryForRecentInsertAutoFields = widgetsToSelect.last()->geometry();
    }

    //eventually, add entire command group to active form
    form()->addCommand(group); //2.0 , true/*exec*/);

// group->debug();

    //enable proper REDO usage
//2.0    group->resetAllowExecuteFlags();

    m_scrollView->repaint();
    m_scrollView->viewport()->repaint();
    m_scrollView->repaintContents();
    m_scrollView->updateContents();
    m_scrollView->clipper()->repaint();
    m_scrollView->refreshContentsSize();

    //select all inserted widgets, if multiple
    if (widgetsToSelect.count() > 1) {
        form()->selectWidget(0);
        foreach (QWidget *w, widgetsToSelect) {
            form()->selectWidget(w, 
                KFormDesigner::Form::AddToPreviousSelection | KFormDesigner::Form::DontRaise);
        }
    }

    // eventually, update property pane
#ifdef __GNUC__
#warning "TODO port unblockPropertyEditorUpdating?"
#else
#pragma WARNING( port unblockPropertyEditorUpdating? )
#endif
//2.0?    KFormDesigner::FormManager::self()->unblockPropertyEditorUpdating(this, KFormDesigner::FormManager::self()->propertySet());
}

void
KexiFormView::setUnsavedLocalBLOB(QWidget *widget, KexiBLOBBuffer::Id_t id)
{
//! @todo if there already was data assigned, remember it should be dereferenced
    if (id == 0)
        tempData()->unsavedLocalBLOBs.remove(widget);
    else
        tempData()->unsavedLocalBLOBs.insert(widget, id);
}

/*
todo
void KexiFormView::updateActions(bool activated)
{
  if (viewMode()==Kexi::DesignViewMode) {
    if (form()->selectedWidget()) {
      if (form()->widget() == form()->selectedWidget())
        KFormDesigner::FormManager::self()->emitFormWidgetSelected( form() );
      else
        KFormDesigner::FormManager::self()->emitWidgetSelected( form(), false );
    }
    else if (form()->selectedWidgets()) {
      KFormDesigner::FormManager::self()->emitWidgetSelected( form(), true );
    }
  }
  KexiDataAwareView::updateActions(activated);
}*/

/*
void KexiFormView::parentDialogDetached()
{
  m_dbform->updateTabStopsOrder(form());
}

void KexiFormView::parentDialogAttached(KMdiChildFrm *)
{
  m_dbform->updateTabStopsOrder(form());
}*/

#include "kexiformview.moc"

