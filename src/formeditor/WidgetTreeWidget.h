/* This file is part of the KDE project
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2008-2010 Jarosław Staniek <staniek@kde.org>

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

#ifndef KFD_WIDGETTREEWIDGET_H
#define KFD_WIDGETTREEWIDGET_H

#include <QTreeWidget>

#include "form.h"

class QContextMenuEvent;

namespace KFormDesigner
{

class ObjectTreeItem;

//! @short An item in WidgetTreeWidget associated with an ObjectTreeItem.
class KFORMEDITOR_EXPORT WidgetTreeWidgetItem : public QTreeWidgetItem
{
public:
    /*! Flags for loadTree() */
    enum LoadTreeFlag {
        NoLoadTreeFlags = 0,
        LoadTreeForAddedTabPage = 1
    };
    Q_DECLARE_FLAGS(LoadTreeFlags, LoadTreeFlag)

    //! Creates tree item. If @a forcedTabPageIndex >= 0, it is used as index for tab page.
    WidgetTreeWidgetItem(WidgetTreeWidgetItem *parent, ObjectTreeItem *data,
        LoadTreeFlags loadTreeFlags = NoLoadTreeFlags, int forcedTabPageIndex = -1,
        const QString& forcedTabPageName = QString());

    //! For TabStopDialog
    explicit WidgetTreeWidgetItem(QTreeWidget *tree, ObjectTreeItem *data = 0,
        LoadTreeFlags loadTreeFlags = NoLoadTreeFlags, int forcedTabPageIndex = -1,
        const QString& forcedTabPageName = QString());
    virtual ~WidgetTreeWidgetItem();

    //! \return the item name, ie the ObjectTreeItem name
    QString name() const;

    //! \return the ObjectTreeItem information associated to this item.
    ObjectTreeItem* data() const;

    //! Added to unhide.
    virtual QVariant data(int column, int role) const { return QTreeWidgetItem::data(column, role); }

    //! Reimplemented to alter sorting for certain widget types, e.g. tab pages.
    virtual bool operator<( const QTreeWidgetItem & other ) const;

    //! Used to alter sorting for certain widget types, e.g. tab pages.
    QString customSortingKey() const;

protected:
    //! Initializes text, icon, selectable flag, custom serting key
    void init(int forcedTabPageIndex, const QString& forcedTabPageName);
    void initTextAndIcon(int forcedTabPageIndex, const QString& forcedTabPageName);


private:
    class Private;
    Private* const d;
};

/*! @short A graphical view of Form's ObjectTree.
 This is a tree representin hierarchy of form widgets.
 The actually selected widgets are written bold
 and selected. Clicking on items selects the corresponding widgets on the form.
 */
class KFORMEDITOR_EXPORT WidgetTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    //! Options for the widget's behaviour or look
    enum Option {
        NoOptions = 0,
        DisableSelection = 1,  //!< disables item selection
        DisableContextMenu = 2 //!< disables context menu
    };
    Q_DECLARE_FLAGS(Options, Option)

    explicit WidgetTreeWidget(QWidget *parent = 0, Options options = NoOptions);

    virtual ~WidgetTreeWidget();

    //! @return selected tree item or 0 if there is no selection or more than one item is selected.
    WidgetTreeWidgetItem* selectedItem() const;

    //! \return the pixmap name for a given class, to be shown next to the widget name.
    QString iconNameForClass(const QByteArray &classname) const;

    //! @see ObjectTreeItem* WidgetLibrary::selectableItem(ObjectTreeItem*)
    ObjectTreeItem* selectableItem(ObjectTreeItem* item);

public slots:
    /*! Sets \a form as the current Form in the list. The list will automatically
     be filled with an item for each widget in the Form, and selection will be synced.
     Nothing happens if \a form is already the current Form.
     */
    void setForm(KFormDesigner::Form *form);

    /*! Sets the widget \a w as selected item, so it will be written bold.
     It replaces previous selection if \a flags & Form::ReplacePreviousSelection is true. */
    void selectWidget(QWidget *w,
                      KFormDesigner::Form::WidgetSelectionFlags flags = KFormDesigner::Form::ReplacePreviousSelection);

    /*! Adds the ObjectTreeItem \a item in the list, with the appropriate parent. */
    void addItem(KFormDesigner::ObjectTreeItem *item);

    /*! Removess the ObjectTreeItem \a item from the list. */
    void removeItem(KFormDesigner::ObjectTreeItem *item);

    /*! Renames the list item from \a oldname to \a newname. */
    void renameItem(const QByteArray &oldname, const QByteArray &newname);

protected slots:
    /*! The selected list item has changed. */
    void slotSelectionChanged();

    /*! Called before Form object is destroyed. */
    void slotBeforeFormDestroyed();

protected:
    //! Internal function to fill the list.
    void loadTree(ObjectTreeItem *item, WidgetTreeWidgetItem *parent,
        WidgetTreeWidgetItem::LoadTreeFlags flags = WidgetTreeWidgetItem::NoLoadTreeFlags);

    //! @return the item whose name is @a name.
    WidgetTreeWidgetItem* findItem(const QString &name);

    //! @return the item whose text in column 0 is @a text.
    WidgetTreeWidgetItem* findItemByFirstColumn(const QString& text);

    virtual void contextMenuEvent(QContextMenuEvent* e);

    void handleContextMenuEvent(QContextMenuEvent* e);

    void selectWidgetForItem(QTreeWidgetItem *item);

    //! Try to alter selection of the item is nonselectable item clicked and parent item is available.
    QTreeWidgetItem* tryToAlterSelection(QTreeWidgetItem* current);

    //! If @a item is (grand)child of tab widget, activate proper tab page.
    //! Do it recursively because there may be nested tab widgets.
    void activateTabPageIfNeeded(QTreeWidgetItem* item);

private:

    class Private;
    Private* const d;

    friend class TabStopDialog;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(WidgetTreeWidget::Options)
Q_DECLARE_OPERATORS_FOR_FLAGS(WidgetTreeWidgetItem::LoadTreeFlags)

}

#endif
