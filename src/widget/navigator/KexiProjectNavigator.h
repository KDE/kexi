/* This file is part of the KDE project
   Copyright (C) 2002, 2003 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2003-2014 Jarosław Staniek <staniek@kde.org>
   Copyright (C) 2010 Adam Pigg <adam@piggz.co.uk>

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

#ifndef KEXIPROJECTNAVIGATOR_H
#define KEXIPROJECTNAVIGATOR_H

#include "kexiextwidgets_export.h"
#include <kexi.h>

#include <QPointer>
#include <QMenu>

class KIcon;
class KAction;
class KActionCollection;
class QModelIndex;
class KexiProjectModel;

namespace KexiPart
{
    class Info;
    class Item;
}
class KexiProject;

/*! @internal */
class KexiMenuBase : public QMenu
{
public:
    KexiMenuBase(QWidget *parent, KActionCollection *collection);
    ~KexiMenuBase();

    QAction* addAction(const QString& actionName);

protected:
    QPointer<KActionCollection> m_actionCollection;
};

/*! @internal */
class KexiItemMenu : public KexiMenuBase
{
public:
    KexiItemMenu(QWidget *parent, KActionCollection *collection);
    ~KexiItemMenu();

    //! Rebuilds the menu entirely using information obtained from \a partInfo
    //! and \a partItem.
    void update(KexiPart::Info* partInfo, KexiPart::Item* partItem);
};

/*! @internal */
class KexiGroupMenu : public KexiMenuBase
{
public:
    KexiGroupMenu(QWidget *parent, KActionCollection *collection);
    ~KexiGroupMenu();

    //! Rebuilds the menu entirely using information obtained from \a partInfo.
    void update(KexiPart::Info* partInfo);
};


//! @short Project navigator widget
//! It is also used to display list of objects for given database project in other places of Kexi.
class KEXIEXTWIDGETS_EXPORT KexiProjectNavigator : public QWidget
{
    Q_OBJECT

public:
    enum Feature {
        NoFeatures   = 0,
        Writable     = 1,                          //!< Support actions that modify
                                                   //!< the project (e.g. delete, rename).
        ContextMenus = 2,                          //!< Supports context menus
        Borders      = 4,                          //!< Show borders like in a list view
        DefaultFeatures = Writable | ContextMenus  //!< The default
    };
    //! Specifies features of a navigator
    Q_DECLARE_FLAGS(Features, Feature)

    explicit KexiProjectNavigator(QWidget* parent, Features features = DefaultFeatures);
    virtual ~KexiProjectNavigator();

    /*! Sets project \a prj for this browser. If \a partManagerErrorMessages is not NULL
     it will be set to error message if there's a problem with loading any KexiPart.
     If \a itemsPartClass is empty (the default), items of all part classes are displayed,
     items for only one part class are displayed. In the latter case, no group (parent)
     items are displayed.
     Previous items are removed. */
    void setProject(KexiProject* prj, const QString& itemsPartClass = QString(),
                    QString* partManagerErrorMessages = 0, bool addAsSearchableModel = true);

    /*! \return items' part class previously set by setProject. Returns empty string
     if setProject() was not executed yet or itemsPartClass argument of setProject() was
     empty (i.e. all part classes are displayed). */
    QString itemsPartClass() const;

    KexiPart::Item* selectedPartItem() const;

    bool actionEnabled(const QString& actionName) const;

    KexiProjectModel* model() const;

public Q_SLOTS:
    virtual void setFocus();
    void updateItemName(KexiPart::Item& item, bool dirty);
    void selectItem(KexiPart::Item& item);
    void clearSelection();
    void clear();

    //! Sets by main window to disable actions that may try to modify the project.
    //! Does not disable actions like opening objects.
    void setReadOnly(bool set);

    bool isReadOnly() const;

Q_SIGNALS:
    void openItem(KexiPart::Item*, Kexi::ViewMode viewMode);

    /*! this signal is emitted when user double clicked (or single -depending on settings)
     or pressed return ky on the part item.
     This signal differs from openItem() signal in that if the object is already opened
     in view mode other than \a viewMode, the mode is not changed. */
    void openOrActivateItem(KexiPart::Item*, Kexi::ViewMode viewMode);

    void newItem(KexiPart::Info*);

    void removeItem(KexiPart::Item*);

    void selectionChanged(KexiPart::Item* item);

    void executeItem(KexiPart::Item*);

    void exportItemToClipboardAsDataTable(KexiPart::Item*);

    void exportItemToFileAsDataTable(KexiPart::Item*);

    void printItem(KexiPart::Item*);

    void pageSetupForItem(KexiPart::Item*);

protected Q_SLOTS:
    void slotExecuteItem(const QModelIndex &item);
    void slotSelectionChanged(const QModelIndex& i);
    void slotSettingsChanged(int);
    
    void slotNewObject();
    void slotOpenObject();
    void slotDesignObject();
    void slotEditTextObject();
    void slotRemove();
    void slotCut();
    void slotCopy();
    void slotPaste();
    void slotRename();
    void slotExecuteObject();
    void slotExportToClipboardAsDataTable();
    void slotExportToFileAsDataTable();
    void slotPrintObject();
    void slotPageSetupForObject();

    void slotUpdateEmptyStateLabel();

protected:
    void itemRenameDone();

    KAction* addAction(const QString& name, const KIcon& icon, const QString& text,
                       const QString& toolTip, const QString& whatsThis, const char* slot);

    virtual void contextMenuEvent ( QContextMenuEvent *event );


private:
    class Private;
    Private * const d;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(KexiProjectNavigator::Features)

#endif
