/* This file is part of the KDE project
   Copyright (C) 2002   Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2003   Joseph Wenninger<jowenn@kde.org>
   Copyright (C) 2003-2007 Jarosław Staniek <staniek@kde.org>

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

#include "KexiRelationsView.h"
#include <KexiIcon.h>
#include <kexiutils/utils.h>
#include <KexiStyle.h>
#include <kexiproject.h>
#include <KexiMainWindowIface.h>
#include "KexiRelationsScrollArea.h"
#include "KexiRelationsConnection.h"

#include <KDbConnection>
#include <KDbTableOrQuerySchema>

#include <KComboBox>
#include <KLocalizedString>

#include <QTimer>
#include <QHBoxLayout>
#include <QLabel>
#include <QGridLayout>
#include <QMenu>
#include <QPushButton>
#include <QDebug>

//! @internal
class Q_DECL_HIDDEN KexiRelationsView::Private
{
public:
    Private()
    {
    }

    KComboBox *tableCombo;
    QPushButton *btnAdd;
    KexiRelationsScrollArea *scrollArea;
    KDbConnection *conn;

    QMenu *tableQueryPopup; //!< over table/query
    QMenu *connectionPopup; //!< over connection
    QMenu *areaPopup; //!< over outer area
    QAction *openSelectedTableAction, *designSelectedTableAction,
    *appendSelectedFieldAction, *appendSelectedFieldsAction, *hideTableAction;
};

//---------------

KexiRelationsView::KexiRelationsView(QWidget *parent)
        : KexiView(parent)
        , d(new Private)
{
    QWidget *mainWidget = new QWidget(this);
    QGridLayout *g = new QGridLayout(mainWidget);
    g->setContentsMargins(0, 0, 0, 0);
    g->setSpacing(KexiUtils::spacingHint());

    QWidget *horWidget = new QWidget(mainWidget);
    QHBoxLayout *hlyr = new QHBoxLayout(horWidget);
    hlyr->setContentsMargins(0, 0, 0, 0);
    g->addWidget(horWidget, 0, 0);

    d->tableCombo = new KComboBox(horWidget);
    d->tableCombo->setObjectName("tables_combo");
    d->tableCombo->setMinimumWidth(QFontMetrics(font()).width("w")*20);
    d->tableCombo->setInsertPolicy(QComboBox::NoInsert);
    d->tableCombo->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred));
    QLabel *lbl = new QLabel(xi18n("Table:"), horWidget);
    lbl->setBuddy(d->tableCombo);
    lbl->setIndent(3);
    hlyr->addWidget(lbl);
    hlyr->addWidget(d->tableCombo);

    d->btnAdd = new QPushButton(xi18nc("Insert table/query into relations view", "&Insert"), horWidget);
    hlyr->addWidget(d->btnAdd);
    hlyr->addStretch(1);
    connect(d->btnAdd, SIGNAL(clicked()), this, SLOT(slotAddTable()));

    d->scrollArea = new KexiRelationsScrollArea(mainWidget);
    d->scrollArea->setObjectName("scroll_area");
    KexiStyle::setupFrame(d->scrollArea);
    setViewWidget(mainWidget, false/* no focus proxy */);
    setFocusProxy(d->scrollArea);
    g->addWidget(d->scrollArea, 1, 0);

    //actions
    d->tableQueryPopup = new QMenu(this);
    d->tableQueryPopup->setObjectName("tableQueryPopup");
    connect(d->tableQueryPopup, SIGNAL(aboutToShow()), this, SLOT(aboutToShowPopupMenu()));

    d->hideTableAction = plugSharedAction("edit_delete", xi18n("&Hide Table"), d->tableQueryPopup);
    if (d->hideTableAction)
        d->hideTableAction->setIcon(QIcon());

    d->connectionPopup = new QMenu(this);
    d->connectionPopup->setObjectName("connectionPopup");
    connect(d->connectionPopup, SIGNAL(aboutToShow()), this, SLOT(aboutToShowPopupMenu()));

//! @todo areaPopup
    d->areaPopup = new QMenu(this);
    d->areaPopup->setObjectName("areaPopup");

    d->appendSelectedFieldAction = new QAction(KexiIcon("add-field"), xi18n("&Append Field"), this);
    d->appendSelectedFieldAction->setObjectName("relationsview_appendField");
    connect(d->appendSelectedFieldAction, SIGNAL(triggered()),
            this, SLOT(appendSelectedFields()));

    d->appendSelectedFieldsAction = new QAction(KexiIcon("add-field"), xi18n("&Append Fields"), this);
    d->appendSelectedFieldsAction->setObjectName("relationsview_appendFields");
    connect(d->appendSelectedFieldsAction, SIGNAL(triggered()),
            this, SLOT(appendSelectedFields()));

    d->openSelectedTableAction = new QAction(koIcon("document-open"), xi18n("&Open Table"), this);
    d->openSelectedTableAction->setObjectName("relationsview_openTable");
    connect(d->openSelectedTableAction, SIGNAL(triggered()),
            this, SLOT(openSelectedTable()));

    d->designSelectedTableAction = new QAction(KexiIcon("mode-selector-design"), xi18n("&Design Table"), this);
    connect(d->designSelectedTableAction, SIGNAL(triggered()),
            this, SLOT(designSelectedTable()));
    d->designSelectedTableAction->setObjectName("relationsview_designTable");

    plugSharedAction("edit_delete", this, SLOT(removeSelectedObject()));

    connect(d->scrollArea, SIGNAL(tableViewGotFocus()),
            this, SLOT(tableViewGotFocus()));
    connect(d->scrollArea, SIGNAL(connectionViewGotFocus()),
            this, SLOT(connectionViewGotFocus()));
    connect(d->scrollArea, SIGNAL(emptyAreaGotFocus()),
            this, SLOT(emptyAreaGotFocus()));
    connect(d->scrollArea, SIGNAL(tableContextMenuRequest(QPoint)),
            this, SLOT(tableContextMenuRequest(QPoint)));
    connect(d->scrollArea, SIGNAL(connectionContextMenuRequest(QPoint)),
            this, SLOT(connectionContextMenuRequest(QPoint)));
    connect(d->scrollArea, SIGNAL(tableHidden(KDbTableSchema*)),
            this, SLOT(slotTableHidden(KDbTableSchema*)));
    connect(d->scrollArea, SIGNAL(tablePositionChanged(KexiRelationsTableContainer*)),
            this, SIGNAL(tablePositionChanged(KexiRelationsTableContainer*)));
    connect(d->scrollArea, SIGNAL(aboutConnectionRemove(KexiRelationsConnection*)),
            this, SIGNAL(aboutConnectionRemove(KexiRelationsConnection*)));

    //! @todo
#if 0
    if (!embedd) {
        /*todo  setContextHelp(xi18n("Relations"), xi18n("To create a relationship simply drag the source field onto the target field. "
              "An arrowhead is used to show which table is the parent (master) and which table is the child (slave) in the relationship."));*/
    }
#endif

#ifdef TESTING_KexiRelationWidget
    for (int i = 0;i < (int)d->db->tableNames().count();i++)
        QTimer::singleShot(100, this, SLOT(slotAddTable()));
#endif

    invalidateActions();
}

KexiRelationsView::~KexiRelationsView()
{
    delete d;
}

TablesHash* KexiRelationsView::tables() const
{
    return d->scrollArea->tables();
}

KexiRelationsTableContainer* KexiRelationsView::table(const QString& name) const
{
    return d->scrollArea->tables()->value(name);
}

const QSet<KexiRelationsConnection*>* KexiRelationsView::relationsConnections() const
{
    return d->scrollArea->relationsConnections();
}

void
KexiRelationsView::slotAddTable()
{
    if (d->tableCombo->currentIndex() == -1)
        return;
    const QString tname = d->tableCombo->itemText(d->tableCombo->currentIndex());
    KDbTableSchema *t = d->conn->tableSchema(tname);
    addTable(t);
}

void
KexiRelationsView::addTable(KDbTableSchema *t, const QRect &rect)
{
    if (!t)
        return;
    if (!d->scrollArea->tableContainer(t)) {
        KexiRelationsTableContainer *c = d->scrollArea->addTableContainer(t, rect);
        //qDebug() << "adding table" << t->name();
        if (!c)
            return;
        connect(c, SIGNAL(fieldsDoubleClicked(KDbTableOrQuerySchema&,QStringList)),
                this, SIGNAL(appendFields(KDbTableOrQuerySchema&,QStringList)));
    }

    const QString tname = t->name().toLower();
    const int count = d->tableCombo->count();
    int i = 0;
    for (; i < count; i++) {
        if (d->tableCombo->itemText(i).toLower() == tname)
            break;
    }
    if (i < count) {
        int oi = d->tableCombo->currentIndex();
        //qDebug() << "removing a table from the combo box";
        d->tableCombo->removeItem(i);
        if (d->tableCombo->count() > 0) {
            if (oi >= d->tableCombo->count()) {
                oi = d->tableCombo->count() - 1;
            }
            d->tableCombo->setCurrentIndex(oi);
        } else {
            d->tableCombo->setEnabled(false);
            d->btnAdd->setEnabled(false);
        }
    }
    emit tableAdded(t);
}

void
KexiRelationsView::addConnection(const SourceConnection& conn)
{
    d->scrollArea->addConnection(conn);
}

void
KexiRelationsView::addTable(const QString& t)
{
    for (int i = 0; i < d->tableCombo->count(); i++) {
        if (d->tableCombo->itemText(i) == t) {
            d->tableCombo->setCurrentIndex(i);
            slotAddTable();
        }
    }
}

void KexiRelationsView::tableViewGotFocus()
{
    invalidateActions();
}

void KexiRelationsView::connectionViewGotFocus()
{
    invalidateActions();
}

void KexiRelationsView::emptyAreaGotFocus()
{
    invalidateActions();
}

void KexiRelationsView::tableContextMenuRequest(const QPoint& pos)
{
    invalidateActions();
    executePopup(pos);
}

void KexiRelationsView::connectionContextMenuRequest(const QPoint& pos)
{
    invalidateActions();
    executePopup(pos);
}

void KexiRelationsView::emptyAreaContextMenuRequest(const QPoint& /*pos*/)
{
    invalidateActions();
    //! @todo
}

void KexiRelationsView::invalidateActions()
{
    setAvailable("edit_delete", d->scrollArea->selectedConnection() || d->scrollArea->focusedTableContainer());
}

void KexiRelationsView::executePopup(QPoint pos)
{
    if (pos == QPoint(-1, -1)) {
        pos = mapToGlobal(
                  d->scrollArea->focusedTableContainer() ? d->scrollArea->focusedTableContainer()->pos() + d->scrollArea->focusedTableContainer()->rect().center() : rect().center());
    }
    if (d->scrollArea->focusedTableContainer())
        d->tableQueryPopup->exec(pos);
    else if (d->scrollArea->selectedConnection())
        d->connectionPopup->exec(pos);
}

void KexiRelationsView::removeSelectedObject()
{
    d->scrollArea->removeSelectedObject();
}

void KexiRelationsView::appendSelectedFields()
{
    KexiRelationsTableContainer* currentTableContainer = d->scrollArea->focusedTableContainer();
    if (!currentTableContainer)
        return;
    emit appendFields(*currentTableContainer->schema(), currentTableContainer->selectedFieldNames());
}

void KexiRelationsView::openSelectedTable()
{
    /*! @todo what about query? */
    if (!d->scrollArea->focusedTableContainer() || !d->scrollArea->focusedTableContainer()->schema()->table())
        return;
    bool openingCancelled;
    KexiMainWindowIface::global()->openObject(
        "kexi/table", d->scrollArea->focusedTableContainer()->schema()->name(),
        Kexi::DataViewMode, &openingCancelled);
}

void KexiRelationsView::designSelectedTable()
{
    /*! @todo what about query? */
    if (!d->scrollArea->focusedTableContainer() || !d->scrollArea->focusedTableContainer()->schema()->table())
        return;
    bool openingCancelled;
    KexiMainWindowIface::global()->openObject(
        "kexi/table", d->scrollArea->focusedTableContainer()->schema()->name(),
        Kexi::DesignViewMode, &openingCancelled);
}

QSize KexiRelationsView::sizeHint() const
{
    return d->scrollArea->sizeHint();
}

void KexiRelationsView::slotTableHidden(KDbTableSchema* table)
{
    const QString &t = table->name().toLower();
    int i;
    for (i = 0; i < d->tableCombo->count() && t > d->tableCombo->itemText(i).toLower(); i++) {
    }
    d->tableCombo->insertItem(i, table->name());
    if (!d->tableCombo->isEnabled()) {
        d->tableCombo->setCurrentIndex(0);
        d->tableCombo->setEnabled(true);
        d->btnAdd->setEnabled(true);
    }

    emit tableHidden(table);
}

void KexiRelationsView::aboutToShowPopupMenu()
{
    KexiRelationsTableContainer* currentTableContainer = d->scrollArea->focusedTableContainer();
    if (currentTableContainer /*&& currentTableContainer->schema()->table()*/) {
        /*! @todo what about query? */
        d->tableQueryPopup->clear();
        d->tableQueryPopup->addSection(KexiIcon("table"),
                                     QString(d->scrollArea->focusedTableContainer()->schema()->name()) + " : " + xi18n("Table"));
        QStringList selectedFieldNames(currentTableContainer->selectedFieldNames());
        if (currentTableContainer && !selectedFieldNames.isEmpty()) {
            if (selectedFieldNames.count() > 1 || selectedFieldNames.first() == "*") //multiple
                d->tableQueryPopup->addAction(d->appendSelectedFieldsAction);
            else
                d->tableQueryPopup->addAction(d->appendSelectedFieldAction);
            d->tableQueryPopup->addSeparator();
        }
        d->tableQueryPopup->addAction(d->openSelectedTableAction);
        d->tableQueryPopup->addAction(d->designSelectedTableAction);
        d->tableQueryPopup->addSeparator();
        d->tableQueryPopup->addAction(d->hideTableAction);
    } else if (d->scrollArea->selectedConnection()) {
        unplugSharedAction("edit_delete", d->connectionPopup);
        d->connectionPopup->clear();
        d->connectionPopup->addSection(QIcon(),
                                     d->scrollArea->selectedConnection()->toString() + " : " + xi18n("Relationship"));
        plugSharedAction("edit_delete", d->connectionPopup);
    }
}

bool KexiRelationsView::clear()
{
    d->scrollArea->clear();
    return setConnection(d->conn);
}

/*! Removes all coonections from the view. */
void KexiRelationsView::removeAllConnections()
{
    d->scrollArea->removeAllConnections();
}

bool KexiRelationsView::setConnection(KDbConnection *conn)
{
    d->tableCombo->clear();
    d->conn = conn;
    if (conn) {
        bool ok;
        QStringList result = d->conn->tableNames(false/*no system tables*/, &ok);
        if (!ok) {
            return false;
        }
        result.sort();
        d->tableCombo->addItems(result);
    }
    d->scrollArea->setConnection(conn);
    return true;
}

void
KexiRelationsView::objectCreated(const QString &mime, const QString& name)
{
    if (mime == "kexi/table" || mime == "kexi/query") {
//! @todo query?
        const int count = d->tableCombo->count();
        QString strName(name);
        int i = 0;
        for (; i < count && d->tableCombo->itemText(i) <= strName; i++) {
        }
        d->tableCombo->insertItem(i, name);
    }
}

void
KexiRelationsView::objectDeleted(const QString &mime, const QString& name)
{
    if (mime == "kexi/table" || mime == "kexi/query") {
        for (int i = 0; i < d->tableCombo->count(); i++) {
//! @todo query?
            if (d->tableCombo->itemText(i) == name) {
                d->tableCombo->removeItem(i);
                if (d->tableCombo->currentIndex() == i) {
                    if (i == (d->tableCombo->count() - 1))
                        d->tableCombo->setCurrentIndex(i - 1);
                    else
                        d->tableCombo->setCurrentIndex(i);
                }
                break;
            }
        }
    }
}

void
KexiRelationsView::objectRenamed(const QString &mime, const QString& name,
                                 const QString& newName)
{
    if (mime == "kexi/table" || mime == "kexi/query") {
        const int count = d->tableCombo->count();
        for (int i = 0; i < count; i++) {
//! @todo query?
            if (d->tableCombo->itemText(i) == name) {
                d->tableCombo->removeItem(i);
                int j = 0;
                for (; j < count && d->tableCombo->itemText(j) <= newName; j++) {
                }
                d->tableCombo->insertItem(j, newName);
                break;
            }
        }
    }
}

void
KexiRelationsView::hideAllTablesExcept(QList<KDbTableSchema*>* tables)
{
    d->scrollArea->hideAllTablesExcept(tables);
}

