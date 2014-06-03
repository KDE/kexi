/* This file is part of the KDE project
   Copyright (C) 2004-2012 Jarosław Staniek <staniek@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef KEXIALTERTABLEDIALOG_P_H
#define KEXIALTERTABLEDIALOG_P_H

#include "kexitabledesignerview.h"
#include <QList>
#include <kundo2command.h>
#include <QByteArray>

class KexiDataAwarePropertySet;

namespace KexiTableDesignerCommands
{
class Command;
}

//! @internal indices for table columns
#define COLUMN_ID_ICON 0
#define COLUMN_ID_CAPTION 1
#define COLUMN_ID_TYPE 2
#define COLUMN_ID_DESC 3

//! @internal
class KexiTableDesignerViewPrivate
{
public:
    explicit KexiTableDesignerViewPrivate(KexiTableDesignerView* aDesignerView);
    ~KexiTableDesignerViewPrivate();

    int generateUniqueId();

    /*! @internal
     Sets property \a propertyName in property set \a set to \a newValue.
     If \a commandGroup is not 0, a new ChangeFieldPropertyCommand object is added there as well.
     While setting the new value, addHistoryCommand_in_slotPropertyChanged_enabled is set to false,
     so addHistoryCommand() wont be executed in slotPropertyChanged() as an answer to setting
     the property.

     If \a forceAddCommand is false (the default) and \a newValue does not differ from curent property value
     (set[propertyName].value()), ChangeFieldPropertyCommand command is not added to the \a commandGroup.
     Otherwise, command is always added.

     \a rememberOldValue argument is passed to Property::setValue()

     If \a slist and \a nlist if not NULL and not empty, these are passed to Property::setListData().
     If \a slist and \a nlist if not NULL but empty, Property::setListData(0) is called.

     addHistoryCommand_in_slotPropertyChanged_enabled is then set back to the original state.
     */
    void setPropertyValueIfNeeded(const KoProperty::Set& set, const QByteArray& propertyName,
                                  const QVariant& newValue, KexiTableDesignerCommands::Command* commandGroup,
                                  bool forceAddCommand = false, bool rememberOldValue = true,
                                  QStringList* const slist = 0, QStringList* const nlist = 0);

    /*! Like above but allows to specify \a oldValue. */
    void setPropertyValueIfNeeded(
        const KoProperty::Set& set, const QByteArray& propertyName,
        const QVariant& newValue, const QVariant& oldValue, KexiTableDesignerCommands::Command* commandGroup,
        bool forceAddCommand = false, bool rememberOldValue = true,
        QStringList* const slist = 0, QStringList* const nlist = 0);

    /*! @internal
     Used in updatePropertiesVisibility().
     Does nothing if visibility should not be changed, i.e. when prop->isVisible()==visible,
     otherwise sets changed to true and sets visibility of property \a prop to \a visible.
    */
    void setVisibilityIfNeeded(const KoProperty::Set& set, KoProperty::Property* prop,
                               bool visible, bool &changed, KexiTableDesignerCommands::Command *commandGroup);

    bool updatePropertiesVisibility(KexiDB::Field::Type fieldType, KoProperty::Set &set,
                                    KexiTableDesignerCommands::Command *commandGroup = 0);

    /*! \return message used to ask user for accepting saving the design.
     \a emptyTable is set to true if the table designed contains no rows.
     If \a skipWarning is true, no warning about data loss is appended (useful when
     only non-physical altering actions will be performed). */
    QString messageForSavingChanges(bool &emptyTable, bool skipWarning = false);

    /*! Updates icon in the first column, depending on property set \a set.
     For example, when "rowSource" and "rowSourceType" propertiesa are not empty,
     "combo" icon appears. */
    void updateIconForRecord(KexiDB::RecordData &record, KoProperty::Set& set);

    KexiTableDesignerView* designerView;

    KexiTableView *view; //!< helper

    KexiDB::TableViewData *data;

    KexiDataAwarePropertySet *sets;

    int row; //!< used to know if a new row is selected in slotCellSelected()

    KToggleAction *action_toggle_pkey;

    QAction *contextMenuTitle;

    int uniqueIdCounter;

    //! internal
    int maxTypeNameTextWidth;
    //! Set to true in beforeSwitchTo() to avoid asking again in storeData()
    bool dontAskOnStoreData;

    bool slotTogglePrimaryKeyCalled;

    bool primaryKeyExists;
    //! Used in slotPropertyChanged() to avoid infinite recursion
    bool slotPropertyChanged_primaryKey_enabled;
    //! Used in slotPropertyChanged() to avoid infinite recursion
    bool slotPropertyChanged_subType_enabled;
    //! used in slotPropertyChanged() to disable addHistoryCommand()
    bool addHistoryCommand_in_slotPropertyChanged_enabled;
    //! used in slotRowUpdated() to disable addHistoryCommand()
    bool addHistoryCommand_in_slotRowUpdated_enabled;
    //! used in slotAboutToDeleteRow() to disable addHistoryCommand()
    bool addHistoryCommand_in_slotAboutToDeleteRow_enabled;
    //! used in slotRowInserted() to disable addHistoryCommand()
    bool addHistoryCommand_in_slotRowInserted_enabled;

    //! used to disable slotBeforeCellChanged()
    bool slotBeforeCellChanged_enabled;

//! @todo temp; remove this:
    //! Temporary flag, used for testing the Alter Table machinery. Affects storeData()
    //! Used in slotExecuteRealAlterTable() to switch on real alter table for a while.
    bool tempStoreDataUsingRealAlterTable;

    /*! Set to a recent result of calling \ref tristate KexiTableDesignerView::storeData(bool dontAsk).
     Then, it is used in \ref void KexiTableDesignerView::executeRealAlterTable()
     to know what return value should be. */
    tristate recentResultOfStoreData;

    KActionCollection* historyActionCollection;
    KUndo2Stack* history;

    //! A set used in KexiTableDesignerView::buildField() to quickly identify
    //! properties internal to the designer
    QSet<QByteArray> internalPropertyNames;
};

#endif
