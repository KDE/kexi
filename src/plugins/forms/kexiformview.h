/* This file is part of the KDE project
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2004-2011 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIFORMVIEW_H
#define KEXIFORMVIEW_H

#include <QTimer>
#include <QDragMoveEvent>
#include <QResizeEvent>
#include <QDropEvent>

#include <widget/dataviewcommon/kexidataawareview.h>

#include <core/KexiWindow.h>
#include <core/KexiWindowData.h>
#include <core/kexiblobbuffer.h>
#include <formeditor/form.h>

#include "kexiformpart.h"

#define NO_DSWIZARD

class KexiFormPart;
class KexiFormPartTempData;
class KexiDBForm;
class KexiFormScrollView;
namespace KexiDB
{
class Cursor;
}
namespace KFormDesigner
{
class Container;
}

//! The KexiFormView lass provides a data-driven (record-based) form view .
/*! The KexiFormView can display data provided "by hand"
 or from KexiDB-compatible database source.

 This class provides a single view used inside KexiWindow.
 It takes care of saving/loading form, of enabling actions when needed.
 One KexiFormView object is instantiated for data view mode
 and a second KexiFormView object is instantiated for design view mode.

 @see KexiDataTableView
*/
class KEXIFORMUTILS_EXPORT KexiFormView : public KexiDataAwareView
{
    Q_OBJECT

public:
    enum ResizeMode {
        ResizeAuto = 0,
        ResizeDefault = ResizeAuto,
        ResizeFixed = 1,
        NoResize = 2 /*! @todo */
    };

    explicit KexiFormView(QWidget *parent, bool dbAware = true);
    virtual ~KexiFormView();

    virtual QSize preferredSizeHint(const QSize& otherSize);

    int resizeMode() const;

    KFormDesigner::Form* form() const;

    /*! Assigns \a id local (static) BLOB's identifier for \a widget widget.
     Previously assigned BLOB will be usassigned.
     If \a id is 0, BLOB is unassigned and no new is assigned.

     This method is called when a widget supporting BLOB data
     (currently, images from KexiDBImageBox, within KexiDBFactory) has BLOB assigned by identifier \a id.
     BLOB identifiers are defined by KexiBLOBBuffer (KexiBLOBBuffer::self() instance).

     The data collected by this method is used on form's design saving (in design mode).
     Local BLOBs are retrieved KexiBLOBBuffer::self() and stored in "kexi__blobs" 'system' table.
     Note that db-aware BLOBs (non local) are not handled this way.
    */
    void setUnsavedLocalBLOB(QWidget *widget, KexiBLOBBuffer::Id_t id);

public slots:
    /*! Inserts autofields onto the form at \a pos position.
     \a sourcePartClass can be "org.kexi-project.table" or "org.kexi-project.query",
     \a sourceName is a name of a table or query, \a fields is a list of fields to insert (one or more)
     Fields are inserted using standard KFormDesigner::InsertWidgetCommand framework,
     so undo/redo is available for this operation.

     If multiple fields are provided, they will be aligned vertically.
     If \a pos is QPoint(-1,-1) (the default), position is computed automatically
     based on a position last inserted field using this method.
     If this method has not been called yet, position of QPoint(40, 40) will be set.

     Called by:
     - slotHandleDropEvent() when field(s) are dropped from the data source pane onto the form
     - KexiFormManager is a used clicked "Insert fields" button on the data source pane. */
    void insertAutoFields(const QString& sourcePartClass, const QString& sourceName,
                          const QStringList& fields, KFormDesigner::Container* targetContainerWidget,
                          const QPoint& pos = QPoint(-1, -1));

protected slots:
    void slotPropertySetSwitched();
    void setFormModified();
    void slotFocus(bool in);
    void slotHandleDragMoveEvent(QDragMoveEvent* e);

    //! Handles field(s) dropping from the data source pane onto the form
    //! @see insertAutoFields()
    void slotHandleDropEvent(QDropEvent* e);

    void slotWidgetSelectionChanged(QWidget *w, KFormDesigner::Form::WidgetSelectionFlags flags);
    void slotWidgetNameChanged(const QByteArray& oldname, const QByteArray& newname);

protected:
    virtual tristate beforeSwitchTo(Kexi::ViewMode mode, bool &dontStore);
    virtual tristate afterSwitchFrom(Kexi::ViewMode mode);
    virtual KoProperty::Set* propertySet();
    virtual KexiDB::SchemaData* storeNewData(const KexiDB::SchemaData& sdata,
                                             KexiView::StoreNewDataOptions options,
                                             bool &cancel);
    virtual tristate storeData(bool dontAsk = false);
    KexiFormPartTempData* tempData() const;
    KexiFormPart* formPart() const;
    void setForm(KFormDesigner::Form *f);
    void initForm();
    void loadForm();

    //! Used in loadForm()
    void updateAutoFieldsDataSource();

    //! Used in loadForm()
    void updateValuesForSubproperties();

    virtual void resizeEvent(QResizeEvent *);

    //! Reimplemented for context key event of top-level form widget.
    //! Redirects to Container::eventFilter().
    virtual void contextMenuEvent(QContextMenuEvent *e);

    void initDataSource();
    virtual void setFocusInternal();

    /*! Called after loading the form contents (before showing it). */
    void updateTabStopsOrder();

    /*! @internal */
    void deleteQuery();

    /*! @internal */
    void updateDataSourcePage();

    /*! Reimplemented after KexiView.
     Updates actions (e.g. availability). */
    virtual void updateActions(bool activated);

    //! Updates internal actions specific to forms.
    //! @todo merge with other "update" routines?
    void updateActionsInternal();

private:
    class Private;
    Private * const d;
};

#endif
