/* This file is part of the KDE project
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2004-2016 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIFORMSCROLLVIEW_H
#define KEXIFORMSCROLLVIEW_H

#include "kexiformutils_export.h"

#include <QScrollArea>
#include <QMargins>

#include <core/KexiRecordNavigatorHandler.h>
#include <widget/dataviewcommon/kexidataprovider.h>
#include <formeditor/kexiformeventhandler.h>
#include <widget/utils/kexirecordnavigator.h>
#include <widget/utils/kexisharedactionclient.h>
#include <widget/dataviewcommon/kexidataawareobjectiface.h>

class KexiRecordNavigator;
class KexiDBForm;
namespace KFormDesigner {
class Form;
}

//! @short A widget for displaying a form view in a scrolled area.
/** Users can resize the form's main widget, according to grid settings.
 * The content is resized so the widget can be further resized.
 * This class also implements:
 * - record navigation handling (KexiRecordNavigatorHandler)
 *  - shared actions handling (KexiSharedActionClient)
 *  - data-aware behaviour (KexiDataAwareObjectInterface)
 *  - data provider bound to data-aware widgets (KexiFormDataProvider)
 *
 * @see KexiTableView
*/
class KEXIFORMUTILS_EXPORT KexiFormScrollView :
            public QScrollArea,
            public KexiRecordNavigatorHandler,
            public KexiSharedActionClient,
            public KexiDataAwareObjectInterface,
            public KexiFormDataProvider,
            public KexiFormEventHandler
{
    Q_OBJECT
    KEXI_DATAAWAREOBJECTINTERFACE

public:
    KexiFormScrollView(QWidget *parent, bool preview);
    virtual ~KexiFormScrollView();

    void setForm(KFormDesigner::Form *form);

    KFormDesigner::Form* form() const;

    //! Needed to avoid conflict with QWidget::data().
    inline KDbTableViewData* data() const { return KexiDataAwareObjectInterface::data(); }

    /*! Reimplemented from KexiDataAwareObjectInterface
     for checking 'readOnly' flag from a widget
     ('readOnly' flag from data member is still checked though). */
    virtual bool columnEditable(int col) override;

    /*! \return number of visible columns in this view.
     There can be a number of duplicated columns defined,
     so columnCount() can return greater or smaller number than dataColumns(). */
    virtual int columnCount() const override;

    //! @return the number of records in the data set (if data set is present).
    virtual int recordCount() const override;

    //! \return number of the currently selected record number or -1.
    virtual int currentRecord() const override;

    /*! \return column information for column number \a col.
     Reimplemented for KexiDataAwareObjectInterface:
     column data corresponding to widget number is used here
     (see fieldNumberForColumn()). */
    virtual KDbTableViewColumn* column(int col) override;

    /*! \return field number within data model connected to a data-aware
     widget at column \a col. */
    virtual int fieldNumberForColumn(int col) override;

    /*! @internal Used by KexiFormView in view switching. */
    void beforeSwitchView();

    /*! \return last record visible on the screen (counting from 0).
     The returned value is guaranteed to be smaller or equal to currentRecord() or -1
     if there are no records.
     Implemented for KexiDataAwareObjectInterface. */
    //! @todo unimplemented for now, this will be used for continuous forms
    virtual int lastVisibleRecord() const override;

    /*! \return vertical scrollbar. Implemented for KexiDataAwareObjectInterface. */
    virtual QScrollBar* verticalScrollBar() const override;

    KexiDBForm* dbFormWidget() const;

    //! @return true if snapping to grid is enabled. The defalt value is false.
    bool isSnapToGridEnabled() const;

    bool isResizingEnabled() const;
    void setResizingEnabled(bool enabled);
    void setRecordNavigatorVisible(bool visible);

    bool isOuterAreaVisible() const;
    void setOuterAreaIndicatorVisible(bool visible);

    void refreshContentsSizeLater();

    KexiRecordNavigator* recordNavigator() const;

    bool isPreviewing() const;

    QMargins viewportMargins() const;

    void setViewportMargins(const QMargins &margins);

    //! @return widget displaying contents of the main area.
    QWidget* mainAreaWidget() const;

    //! Sets widget for displaying contents of the main area.
    void setMainAreaWidget(QWidget* widget);

    //! temporary
    int leftMargin() const { return 0; }

    //! temporary
    int bottomMargin() const { return 0; }

    //! temporary
    void updateScrollBars() {}

    /*! @return geometry of the viewport, i.e. the scrollable area, minus any scrollbars, etc.
     Implementation for KexiDataAwareObjectInterface. */
    virtual QRect viewportGeometry() const override;

public Q_SLOTS:
    //! Implementation for KexiDataAwareObjectInterface
    //! \return arbitraty value of 10.
    virtual int recordsPerPage() const override;

    //! Implementation for KexiDataAwareObjectInterface
    virtual void ensureCellVisible(int record, int col) override;

    //! Implementation for KexiDataAwareObjectInterface
    virtual void ensureColumnVisible(int col) override;

    virtual void moveToRecordRequested(int r) override;
    virtual void moveToLastRecordRequested() override;
    virtual void moveToPreviousRecordRequested() override;
    virtual void moveToNextRecordRequested() override;
    virtual void moveToFirstRecordRequested() override;
    virtual void addNewRecordRequested() override {
        KexiDataAwareObjectInterface::addNewRecordRequested();
    }

    /*! Cancels changes made to the currently active editor.
     Reverts the editor's value to old one.
     \return true on success or false on failure (e.g. when editor does not exist) */
    virtual bool cancelEditor() override;

public Q_SLOTS:
    /*! Clear command history right after final resize. */
    void refreshContentsSize();

    /*! Handles verticalScrollBar()'s valueChanged(int) signal.
     Called when vscrollbar's value has been changed. */
    //! @todo unused for now, will be used for continuous forms
    virtual void verticalScrollBarValueChanged(int v) override {
        KexiDataAwareObjectInterface::verticalScrollBarValueChanged(v);
    }

Q_SIGNALS:
    void itemChanged(KDbRecordData* data, int record, int column) override;
    void itemChanged(KDbRecordData* data, int record, int column, const QVariant &oldValue) override;
    void itemDeleteRequest(KDbRecordData* data, int record, int column) override;
    void currentItemDeleteRequest() override;
    void newItemAppendedForAfterDeletingInSpreadSheetMode() override; //!< does nothing
    void dataRefreshed() override;
    void dataSet(KDbTableViewData *data) override;
    void itemSelected(KDbRecordData* data) override;
    void cellSelected(int record, int column) override;
    void sortedColumnChanged(int column) override;
    void recordEditingStarted(int record) override;
    void recordEditingTerminated(int record) override;
    void updateSaveCancelActions() override;
    void reloadActions() override;

    //! Emitted when the main widget area is being interactively resized.
    bool resized();

protected Q_SLOTS:
    //! Handles KDbTableViewData::recordRepaintRequested() signal
    virtual void slotRecordRepaintRequested(KDbRecordData* data) override;

    //! Handles KDbTableViewData::aboutToDeleteRecord() signal. Prepares info for slotRecordDeleted().
    virtual void slotAboutToDeleteRecord(KDbRecordData* data, KDbResultInfo* result, bool repaint) override {
        KexiDataAwareObjectInterface::slotAboutToDeleteRecord(data, result, repaint);
    }

    //! Handles KDbTableViewData::recordDeleted() signal to repaint when needed.
    virtual void slotRecordDeleted() override {
        KexiDataAwareObjectInterface::slotRecordDeleted();
    }

    //! Handles KDbTableViewData::recordInserted() signal to repaint when needed.
    virtual void slotRecordInserted(KDbRecordData* data, bool repaint) override;

    //! Like above, not db-aware version
    virtual void slotRecordInserted(KDbRecordData* data, int record, bool repaint) override;

    virtual void slotRecordsDeleted(const QList<int>&) override;

    virtual void slotDataDestroying() override {
        KexiDataAwareObjectInterface::slotDataDestroying();
    }

    /*! Reloads data for this widget.
     Handles KDbTableViewData::reloadRequested() signal. */
    virtual void reloadData() override {
        KexiDataAwareObjectInterface::reloadData();
    }

    //! Copy current selection to a clipboard (e.g. cell)
    virtual void copySelection() override;

    //! Cut current selection to a clipboard (e.g. cell)
    virtual void cutSelection() override;

    //! Paste current clipboard contents (e.g. to a cell)
    virtual void paste() override;

protected:
    //! Implementation for KexiDataAwareObjectInterface
    virtual void clearColumnsInternal(bool repaint) override;

    //! Implementation for KexiDataAwareObjectInterface
    virtual KDbOrderByColumn::SortOrder currentLocalSortOrder() const override;

    //! Implementation for KexiDataAwareObjectInterface
    virtual int currentLocalSortColumn() const override;

    //! Implementation for KexiDataAwareObjectInterface. Visually does nothing
    //! but remembers index of the currently sorted column and order.
    virtual void setLocalSortOrder(int column, KDbOrderByColumn::SortOrder order) override;

    //! Implementation for KexiDataAwareObjectInterface.
    //! Just calls KexiDataAwareObjectInterface's implementation.
    void sortColumnInternal(int col, int order = 0) override;

    //! Implementation for KexiDataAwareObjectInterface.
    //! Nothing to do here. Record navigator is already updated.
    virtual void updateGUIAfterSorting(int previousRecord) override;

    //! Implementation for KexiDataAwareObjectInterface
    virtual void createEditor(int record, int column, const QString& addText = QString(),
                              CreateEditorFlags flags = DefaultCreateEditorFlags) override;

    //! Implementation for KexiDataAwareObjectInterface
    virtual KexiDataItemInterface *editor(int col, bool ignoreMissingEditor = false) override;

    //! Implementation for KexiDataAwareObjectInterface
    virtual void editorShowFocus(int record, int column) override;

    /*! Implementation for KexiDataAwareObjectInterface
     Redraws specified cell. */
    virtual void updateCell(int record, int column) override;

    /*! Redraws the current cell. Implemented after KexiDataAwareObjectInterface. */
    virtual void updateCurrentCell() override;

    /*! Implementation for KexiDataAwareObjectInterface
     Redraws all cells of specified record. */
    virtual void updateRecord(int record) override;

    /*! Implementation for KexiDataAwareObjectInterface
     Updates contents of the widget. Just call update() here on your widget. */
    virtual void updateWidgetContents() override;

    /*! Implementation for KexiDataAwareObjectInterface
     Implementation for KexiDataAwareObjectInterface
     Updates widget's contents size e.g. using QScrollView::resizeContents(). */
    virtual void updateWidgetContentsSize() override;

    //! Reimplemented from KexiFormDataProvider. Reaction for change of \a item.
    virtual void valueChanged(KexiDataItemInterface* item) override;

    /*! Reimplemented from KexiFormDataProvider.
     \return information whether we're currently at new record or not.
     This can be used e.g. by data-aware widgets to determine if "(auto)"
     label should be displayed. */
    virtual bool cursorAtNewRecord() const override;

    /*! Implementation for KexiFormDataProvider. */
    virtual void lengthExceeded(KexiDataItemInterface *item, bool lengthExceeded) override;

    /*! Implementation for KexiFormDataProvider. */
    virtual void updateLengthExceededMessage(KexiDataItemInterface *item) override;

    //! Implementation for KexiDataAwareObjectInterface
    //! Called by KexiDataAwareObjectInterface::setCursorPosition()
    //! if cursor's position is really changed.
    virtual void selectCellInternal(int previousRecord, int previousColumn) override;

    /*! Reimplementation: used to refresh "editing indicator" visibility. */
    virtual void initDataContents() override;

    /*! @internal
     Updates record appearance after canceling record edit.
     Reimplemented from KexiDataAwareObjectInterface: just undoes changes for every data item.
     Used by cancelRecordEditing(). */
    virtual void updateAfterCancelRecordEditing() override;

    /*! @internal
     Updates record appearance after accepting record edit.
     Reimplemented from KexiDataAwareObjectInterface: just clears 'edit' indicator.
     Used by cancelRecordEditing(). */
    virtual void updateAfterAcceptRecordEditing() override;

    /*! @internal
     Used to invoke copy/paste/cut etc. actions at the focused widget's level. */
    void handleDataWidgetAction(const QString& actionName);

    /*! @internal */
    bool shouldDisplayDefaultValueForItem(KexiFormDataItemInterface* itemIface) const;

    virtual void setHBarGeometry(QScrollBar & hbar, int x, int y, int w, int h);

    const QTimer *delayedResizeTimer() const;

    //! Update section of vertical header
    virtual void updateVerticalHeaderSection(int section) override;

private:
    class Private;
    Private * const d;
};

#endif
