/* This file is part of the KDE project
   Copyright (C) 2005-2016 Jarosław Staniek <staniek@kde.org>

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

#include "KexiDataSourceComboBox.h"
#include <KexiIcon.h>
#include <kexi.h>
#include <kexiproject.h>
#include <kexipart.h>
#include <kexipartmanager.h>
#include <kexipartinfo.h>
#include <kexipartitem.h>

#include <KDbConnection>

#include <QDebug>
#include <QLineEdit>

#ifdef KEXI_SHOW_UNIMPLEMENTED
#define ADD_DEFINEQUERY_ROW
#endif

//! @internal
class Q_DECL_HIDDEN KexiDataSourceComboBox::Private
{
public:
    Private()
            : tableIcon(KexiIcon("table"))
            , queryIcon(KexiIcon("query"))
            , tablesCount(0)
            , prevIndex(-1)
            , showTables(true)
            , showQueries(true) {
    }
    int firstTableIndex() const {
        int index = 1; //skip empty row
#ifdef ADD_DEFINEQUERY_ROW
        index++; /*skip 'define query' row*/
#endif
        return index;
    }
    int firstQueryIndex() const {
        return firstTableIndex() + tablesCount;
    }

    QPointer<KexiProject> prj;
    QIcon tableIcon, queryIcon;
    int tablesCount;
    int prevIndex; //!< Used in slotActivated()
    bool showTables;
    bool showQueries;
};

//------------------------

KexiDataSourceComboBox::KexiDataSourceComboBox(QWidget *parent)
        : KComboBox(true/*rw*/, parent)
        , d(new Private())
{
    setInsertPolicy(NoInsert);
    setCompletionMode(KCompletion::CompletionPopupAuto);
    setMaxVisibleItems(16);
    connect(this, SIGNAL(activated(int)), this, SLOT(slotActivated(int)));
    connect(this, SIGNAL(returnPressed(QString)),
            this, SLOT(slotReturnPressed(QString)));
    connect(this, SIGNAL(editTextChanged(QString)), this, SLOT(slotTextChanged(QString)));
}

KexiDataSourceComboBox::~KexiDataSourceComboBox()
{
    delete d;
}

KexiProject* KexiDataSourceComboBox::project() const
{
    return d->prj;
}

void KexiDataSourceComboBox::setProject(KexiProject *prj, bool showTables, bool showQueries)
{
    if (static_cast<KexiProject *>(d->prj) == prj)
        return;

    if (d->prj) {
        disconnect(d->prj, 0, this, 0);
    }
    d->prj = prj;
    d->showTables = showTables;
    d->showQueries = showQueries;
    clear();
    d->tablesCount = 0;
    if (!d->prj)
        return;

    //needed for updating contents of the combo box
    connect(d->prj, SIGNAL(newItemStored(KexiPart::Item*)),
            this, SLOT(slotNewItemStored(KexiPart::Item*)));
    connect(d->prj, SIGNAL(itemRemoved(KexiPart::Item)),
            this, SLOT(slotItemRemoved(KexiPart::Item)));
    connect(d->prj, SIGNAL(itemRenamed(KexiPart::Item,QString)),
            this, SLOT(slotItemRenamed(KexiPart::Item,QString)));

    KDbConnection *conn = d->prj->dbConnection();
    if (!conn)
        return;

    addItem(""); //special item: empty but not null
#ifdef ADD_DEFINEQUERY_ROW
    //special item: define query
    addItem(xi18n("Define Query..."));
#endif

    KCompletion *comp = completionObject();

    if (d->showTables) {
        //tables
        KexiPart::Info* partInfo = Kexi::partManager().infoForPluginId("org.kexi-project.table");
        if (!partInfo)
            return;
        KexiPart::ItemList list;
        prj->getSortedItems(&list, partInfo);
        list.sort();
        d->tablesCount = 0;
        foreach(KexiPart::Item *item, list) {
            addItem(d->tableIcon, item->name()); //or caption()?
            comp->addItem(item->name());
            d->tablesCount++;
        }
    }

    if (d->showQueries) {
        //queries
        KexiPart::Info* partInfo = Kexi::partManager().infoForPluginId("org.kexi-project.query");
        if (!partInfo)
            return;
        KexiPart::ItemList list;
        prj->getSortedItems(&list, partInfo);
        list.sort();
        foreach(KexiPart::Item *item, list) {
            addItem(d->queryIcon, item->name()); //or caption()?
            comp->addItem(item->name());
        }
    }
    setCurrentIndex(0);
}

void KexiDataSourceComboBox::setDataSource(const QString& pluginId, const QString& name)
{
    if (name.isEmpty()) {
        if (currentIndex() != 0) {
            clearEditText();
            setCurrentIndex(0);
            d->prevIndex = -1;
            emit dataSourceChanged();
        }
        return;
    }

    QString _pluginId(pluginId);
    if (_pluginId.isEmpty())
        _pluginId = "org.kexi-project.table";
    int i = findItem(_pluginId, name);
    if (i == -1) {
        if (pluginId.isEmpty())
            i = findItem("org.kexi-project.query", name);
        if (i == -1) {
            if (currentIndex() != 0) {
                setCurrentIndex(0);
            }
            return;
        }
    }
    if (currentIndex() != i) {
        setCurrentIndex(i);
        slotActivated(i);
    }
}

void KexiDataSourceComboBox::slotNewItemStored(KexiPart::Item* item)
{
    QString name(item->name());
    //insert a new item, maintaining sort order and splitting to tables and queries
    if (item->pluginId() == "org.kexi-project.table") {
        int i = 1; /*skip empty row*/
#ifdef ADD_DEFINEQUERY_ROW
        i++; /*skip 'define query' row*/
#endif
        for (; i < d->firstQueryIndex() && name >= itemText(i); i++) {
        }
        insertItem(i, d->tableIcon, name);
        completionObject()->addItem(name);
        d->tablesCount++;
    } else if (item->pluginId() == "org.kexi-project.query") {
        int i;
        for (i = d->firstQueryIndex(); i < count() && name >= itemText(i); i++) {
        }
        insertItem(i, d->queryIcon, name);
        completionObject()->addItem(name);
    }
}

int KexiDataSourceComboBox::findItem(const QString& pluginId, const QString& name)
{
    int i, end;
    if (pluginId == "org.kexi-project.table") {
        i = 0;
#ifdef ADD_DEFINEQUERY_ROW
        i++; //skip 'define query'
#endif
        end = d->firstQueryIndex();
    } else if (pluginId == "org.kexi-project.query") {
        i = d->firstQueryIndex();
        end = count();
    } else
        return -1;

    QString nameString(name);

    for (; i < end; i++)
        if (itemText(i) == nameString)
            return i;

    return -1;
}

void KexiDataSourceComboBox::slotItemRemoved(const KexiPart::Item& item)
{
    const int i = findItem(item.pluginId(), item.name());
    if (i == -1)
        return;
    removeItem(i);
    completionObject()->removeItem(item.name());
    if (item.pluginId() == "org.kexi-project.table")
        d->tablesCount--;
#if 0 //disabled because even invalid data source can be set
    if (currentItem() == i) {
        if (i == (count() - 1))
            setCurrentItem(i - 1);
        else
            setCurrentItem(i);
    }
#endif
}

void KexiDataSourceComboBox::slotItemRenamed(const KexiPart::Item& item, const QString& oldName)
{
    const int i = findItem(item.pluginId(), QString(oldName));
    if (i == -1) {
        return;
    }
    setItemText(i, item.name());
    completionObject()->removeItem(oldName);
    completionObject()->addItem(item.name());
    setEditText(oldName); //still keep old name
}

void KexiDataSourceComboBox::slotActivated(int index)
{
    if (index >= 0 && index < count() && d->prevIndex != currentIndex()) {
        d->prevIndex = currentIndex();
        emit dataSourceChanged();
    }
}

void KexiDataSourceComboBox::slotTextChanged(const QString &text)
{
    Q_UNUSED(text)
    //! @todo This place may be useful when we alow to enter values not being on the list
}

QString KexiDataSourceComboBox::selectedPluginId() const
{
    if (selectedName().isEmpty()) {
        return QString();
    }
    const int index = currentIndex();
    if (index >= d->firstTableIndex() && index < (int)d->firstQueryIndex()) {
        return "org.kexi-project.table";
    }
    else if (index >= (int)d->firstQueryIndex() && index < count()) {
        return "org.kexi-project.query";
    }
    return QString();
}

QString KexiDataSourceComboBox::selectedName() const
{
    if (isSelectionValid()) {
        return itemText(currentIndex());
    }
    return currentText();
}

bool KexiDataSourceComboBox::isSelectionValid() const
{
    const int index = currentIndex();
    return index >= d->firstTableIndex() && index < count() && itemText(index) == currentText();
}

void KexiDataSourceComboBox::slotReturnPressed(const QString & text)
{
    //if selected text is valid: no completion is required.
    if (isSelectionValid()) {
        return;
    }

    //text is available: select item for this text:
    bool changed = false;
    if (text.isEmpty() && 0 != currentIndex()) {
        setCurrentIndex(0);
        changed = true;
    } else {
        const int index = findText(text, Qt::MatchExactly);
        if (index >= 0 && index != currentIndex()) {
            setCurrentIndex(index);
            changed = true;
        }
    }
    if (changed) {
        emit dataSourceChanged();
    }
}

void KexiDataSourceComboBox::focusOutEvent(QFocusEvent *e)
{
    KComboBox::focusOutEvent(e);
    slotReturnPressed(currentText());
}

