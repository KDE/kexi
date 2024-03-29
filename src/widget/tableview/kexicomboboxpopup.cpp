/* This file is part of the KDE project
   Copyright (C) 2004-2017 Jarosław Staniek <staniek@kde.org>

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

#include "kexicomboboxpopup.h"

#include "KexiDataTableScrollArea.h"
#include "KexiTableScrollArea_p.h"
#include "kexitableedit.h"
#include <kexi_global.h>

#include <KDbConnection>
#include <KDbCursor>
#include <KDbExpression>
#include <KDbLookupFieldSchema>
#include <KDbQuerySchema>
#include <KDbTableViewColumn>

#include <QDebug>
#include <QEvent>
#include <QKeyEvent>
#include <QScrollBar>
#include <QApplication>
#include <QDesktopWidget>

/*! @internal
 Helper for KexiComboBoxPopup. */
class KexiComboBoxPopup_KexiTableView : public KexiDataTableScrollArea
{
    Q_OBJECT
public:
    explicit KexiComboBoxPopup_KexiTableView(QWidget* parent = nullptr)
            : KexiDataTableScrollArea(parent) {
        init();
    }
    void init() {
        setObjectName("KexiComboBoxPopup_tv");
        setReadOnly(true);
        setLineWidth(0);
        d->moveCursorOnMouseRelease = true;
        KexiTableScrollArea::Appearance a(appearance());
        a.navigatorEnabled = false;
//! @todo add option for backgroundAltering??
        a.backgroundAltering = false;
        a.fullRecordSelection = true;
        a.recordHighlightingEnabled = true;
        a.recordMouseOverHighlightingEnabled = true;
        a.persistentSelections = false;
        a.recordMouseOverHighlightingColor = palette().highlight().color();
        a.recordMouseOverHighlightingTextColor = palette().highlightedText().color();
        a.recordHighlightingTextColor = a.recordMouseOverHighlightingTextColor;
        a.horizontalGridEnabled = false;
        a.verticalGridEnabled = false;
        setAppearance(a);
        setInsertingEnabled(false);
        setSortingEnabled(false);
        setVerticalHeaderVisible(false);
        setHorizontalHeaderVisible(false);
        setContextMenuEnabled(false);
        setScrollbarToolTipsEnabled(false);
        installEventFilter(this);
        setBottomMarginInternal(0);
    }
    virtual void setData(KDbTableViewData *data, bool owner = true) {
        KexiTableScrollArea::setData(data, owner);
    }
    bool setData(KDbCursor *cursor) {
        return KexiDataTableScrollArea::setData(cursor);
    }
};

//========================================

//! @internal
class KexiComboBoxPopupPrivate
{
public:
    KexiComboBoxPopupPrivate()
            : int_f(0)
            , privateQuery(0) {
        maxRecordCount = KexiComboBoxPopup::defaultMaxRecordCount;
    }
    ~KexiComboBoxPopupPrivate() {
        delete int_f;
        delete privateQuery;
    }

    KexiComboBoxPopup_KexiTableView *tv;
    KDbField *int_f; //!< @todo remove this -temporary
    KDbQuerySchema* privateQuery;
    int maxRecordCount;
    //! Columns that should be kept visible; the others should be hidden.
    //! Used when query is used as the record source type (KDbLookupFieldSchemaRecordSource::Query).
    //! We're doing this in this case because it's hard to alter the query to remove columns.
    QList<int> visibleColumnsToShow;
};

//========================================

const int KexiComboBoxPopup::defaultMaxRecordCount = 8;

KexiComboBoxPopup::KexiComboBoxPopup(KDbConnection *conn, KDbTableViewColumn *column,
                                     QWidget *parent)
    : QFrame(parent, Qt::Popup), d(new KexiComboBoxPopupPrivate)
{
    init();
    //setup tv data
    setData(conn, column, 0);
}

KexiComboBoxPopup::KexiComboBoxPopup(KDbConnection *conn, KDbField *field, QWidget *parent)
    : QFrame(parent, Qt::Popup), d(new KexiComboBoxPopupPrivate)
{
    init();
    //setup tv data
    setData(conn, nullptr, field);
}

KexiComboBoxPopup::~KexiComboBoxPopup()
{
    delete d;
}

void KexiComboBoxPopup::init()
{
    setObjectName("KexiComboBoxPopup");
    setAttribute(Qt::WA_WindowPropagation);
    setAttribute(Qt::WA_X11NetWmWindowTypeCombo);

    QPalette pal(palette());
    pal.setBrush(backgroundRole(), pal.brush(QPalette::Base));
    setPalette(pal);
    setLineWidth(1);
    setFrameStyle(Box | Plain);

    d->tv = new KexiComboBoxPopup_KexiTableView(this);
    d->tv->setFrameShape(QFrame::NoFrame);
    d->tv->setLineWidth(0);
    installEventFilter(this);

    connect(d->tv, SIGNAL(itemReturnPressed(KDbRecordData*,int,int)),
            this, SLOT(slotTVItemAccepted(KDbRecordData*,int,int)));

    connect(d->tv, SIGNAL(itemMouseReleased(KDbRecordData*,int,int)),
            this, SLOT(slotTVItemAccepted(KDbRecordData*,int,int)));

    connect(d->tv, SIGNAL(itemDblClicked(KDbRecordData*,int,int)),
            this, SLOT(slotTVItemAccepted(KDbRecordData*,int,int)));
}

void KexiComboBoxPopup::setData(KDbConnection *conn, KDbTableViewColumn *column, KDbField *aField)
{
    d->visibleColumnsToShow.clear();
    const KDbField *field = aField;
    if (!field && column) {
        field = column->field();
    }
    if (!field) {
        qWarning() << "No field or column specified";
        return;
    }

    // case 1: simple related data
    if (column && column->relatedData()) {
        d->tv->setColumnsResizeEnabled(true); //only needed when using single column
        setDataInternal(column->relatedData(), false /*!owner*/);
        return;
    }

    // case 2: lookup field
    const KDbLookupFieldSchema *lookupFieldSchema = nullptr;
    if (field->table())
        lookupFieldSchema = field->table()->lookupFieldSchema(*field);
    delete d->privateQuery;
    d->privateQuery = 0;
    const QList<int> visibleColumns(lookupFieldSchema ? lookupFieldSchema->visibleColumns() : QList<int>());
    if (!visibleColumns.isEmpty() && lookupFieldSchema && lookupFieldSchema->boundColumn() >= 0) {
        if (!conn) {
            qWarning() << "No connection specified";
            return;
        }
        const bool multipleLookupColumnJoined = visibleColumns.count() > 1;
//! @todo support more RowSourceType's, not only table and query
        KDbCursor *cursor = 0;
        switch (lookupFieldSchema->recordSource().type()) {
        case KDbLookupFieldSchemaRecordSource::Type::Table: {
            KDbTableSchema *lookupTable = conn->tableSchema(lookupFieldSchema->recordSource().name());
            if (!lookupTable)
//! @todo errmsg
                return;
            if (multipleLookupColumnJoined) {
                /*qDebug() << "--- Orig query: ";
                qDebug() << *lookupTable->query();
                qDebug() << field->table()->connection()->selectStatement(*lookupTable->query());*/
                d->privateQuery = new KDbQuerySchema(*lookupTable->query(), conn);
            } else {
                // Create a simple SELECT query that contains only needed columns,
                // that is visible and bound ones. The bound columns are placed on the end.
                // Don't do this if one or more visible or bound columns cannot be found.
                const KDbQueryColumnInfo::Vector fieldsExpanded(
                    lookupTable->query()->fieldsExpanded(conn));
                d->privateQuery = new KDbQuerySchema;
                bool columnsFound = true;
                QList<int> visibleAndBoundColumns = visibleColumns;
                visibleAndBoundColumns.append(lookupFieldSchema->boundColumn());
                //qDebug() << visibleAndBoundColumns;
                foreach (int index, visibleAndBoundColumns) {
                    KDbQueryColumnInfo *columnInfo = fieldsExpanded.value(index);
                    if (!columnInfo || !columnInfo->field() || !d->privateQuery->addField(columnInfo->field())) {
                        columnsFound = false;
                        break;
                    }
                }
                if (columnsFound) {
                    // proper data source: bound + visible columns
                    cursor = conn->prepareQuery(d->privateQuery);
                    /*qDebug() << "--- Composed query:";
                    qDebug() << *d->privateQuery;
                    qDebug() << field->table()->connection()->selectStatement(*d->privateQuery);*/
                } else {
                    // for sanity
                    delete d->privateQuery;
                    d->privateQuery = 0;
                    cursor = conn->prepareQuery(lookupTable);
                }
            }
            break;
        }
        case KDbLookupFieldSchemaRecordSource::Type::Query: {
            KDbQuerySchema *lookupQuery
                = conn->querySchema(lookupFieldSchema->recordSource().name());
            if (!lookupQuery)
//! @todo errmsg
                return;
            if (multipleLookupColumnJoined) {
                /*qDebug() << "--- Orig query: ";
                qDebug() << *lookupQuery;
                qDebug() << field->table()->connection()->selectStatement(*lookupQuery);*/
                d->privateQuery = new KDbQuerySchema(*lookupQuery, conn);
            } else {
                d->visibleColumnsToShow = visibleColumns;
                std::sort(d->visibleColumnsToShow.begin(), d->visibleColumnsToShow.end()); // because we will depend on a sorted list
                cursor = conn->prepareQuery(lookupQuery);
            }
            break;
        }
        default:;
        }
        if (multipleLookupColumnJoined && d->privateQuery) {
            // append a column computed using multiple columns
            const KDbQueryColumnInfo::Vector fieldsExpanded(d->privateQuery->fieldsExpanded(conn));
            int fieldsExpandedSize(fieldsExpanded.size());
            KDbExpression expr;
            QList<int>::ConstIterator it(visibleColumns.constBegin());
            for (it += visibleColumns.count() - 1; it != visibleColumns.constEnd(); --it) {
                KDbQueryColumnInfo *ci = ((*it) < fieldsExpandedSize) ? fieldsExpanded.at(*it) : 0;
                if (!ci) {
                    qWarning() << *it << ">= fieldsExpandedSize";
                    continue;
                }
                KDbVariableExpression fieldExpr(ci->field()->table()->name() + "." + ci->field()->name());
                //! @todo KEXI3 check this we're calling KDbQuerySchema::validate() instead of this: fieldExpr.field = ci->field;
                //! @todo KEXI3 check this we're calling KDbQuerySchema::validate() instead of this: fieldExpr.tablePositionForField = d->privateQuery->tableBoundToColumn(*it);
                if (expr.isValid()) {
//! @todo " " separator hardcoded...
//! @todo use SQL sub-parser here...
                    KDbConstExpression constExpr(KDbToken::CHARACTER_STRING_LITERAL, " ");
                    expr = KDbBinaryExpression(constExpr, KDbToken::CONCATENATION, expr);
                    expr = KDbBinaryExpression(fieldExpr, KDbToken::CONCATENATION, expr);
                } else {
                    expr = fieldExpr;
                }
            }
            //qDebug() << expr;

            KDbField *f = new KDbField();
            f->setExpression(expr);
            if (!d->privateQuery->addField(f)) {
                qWarning() << "d->privateQuery->addField(f)";
                delete f;
                return;
            }
            QString errorMessage, errorDescription;
            //! @todo KEXI3 check d->privateQuery->validate()
            if (!d->privateQuery->validate(&errorMessage, &errorDescription)) {
                qWarning() << "error in query:" << d->privateQuery << "\n"
                           << "errorMessage:" << errorMessage
                           << "\nerrorDescription:" << errorDescription;
                return;
            }
#if 0 //does not work yet
// <remove later>
//! @todo temp: improved display by hiding all columns except the computed one
            const int numColumntoHide = d->privateQuery->fieldsExpanded().count() - 1;
            for (int i = 0; i < numColumntoHide; i++)
                d->privateQuery->setColumnVisible(i, false);
// </remove later>
#endif
//! @todo ...
            //qDebug() << "--- Private query:" << KDbConnectionAndQuerySchema(conn, *d->privateQuery);
            cursor = conn->prepareQuery(d->privateQuery);
        }
        if (!cursor)
//! @todo errmsg
            return;

        if (d->tv->data())
            d->tv->data()->disconnect(this);
        d->tv->setData(cursor);

        connect(d->tv, SIGNAL(dataRefreshed()), this, SLOT(slotDataReloadRequested()));
        updateSize();
        return;
    }

    qWarning() << "no column relatedData \n - moving to setData(KDbField &)";

    // case 3: enum hints
    d->tv->setColumnsResizeEnabled(true);   //only needed when using single column

//! @todo THIS IS PRIMITIVE: we'd need to employ KDbReference here!
    d->int_f = new KDbField(field->name(), KDbField::Text);
    KDbTableViewData *data = new KDbTableViewData();
    data->addColumn(new KDbTableViewColumn(d->int_f));
    const QVector<QString> hints(field->enumHints());
    for (int i = 0; i < hints.size(); i++) {
        KDbRecordData *newData = data->createItem();
        (*newData)[0] = QVariant(hints[i]);
        //qDebug() << "added: '" << hints[i] << "'";
        data->append(newData);
    }
    setDataInternal(data, true);
    updateSize();
}

void KexiComboBoxPopup::setDataInternal(KDbTableViewData *data, bool owner)
{
    if (d->tv->data())
        d->tv->data()->disconnect(this);
    d->tv->setData(data, owner);
    connect(d->tv, SIGNAL(dataRefreshed()), this, SLOT(slotDataReloadRequested()));

    updateSize();
}

void KexiComboBoxPopup::updateSize(int minWidth)
{
    const int records = qMin(d->maxRecordCount, d->tv->recordCount());

    KexiTableEdit *te = dynamic_cast<KexiTableEdit*>(parentWidget());
    int width = qMax(d->tv->tableSize().width(),
                           (te ? te->totalSize().width() : (parentWidget() ? parentWidget()->width() : 0/*sanity*/)));
    //qDebug() << "size=" << size();
    const QRect screen = QApplication::desktop()->availableGeometry(this);
    resize(qMin(screen.width(), qMax(minWidth, width)), d->tv->recordHeight() * records + 3);

    //qDebug() << "size after=" << size() << d->tv->verticalScrollBar()->isVisible() << d->tv->horizontalScrollBar()->isVisible();
    if (d->visibleColumnsToShow.isEmpty()) {
        // record source type is not Query
        d->tv->setColumnResizeEnabled(0, true);
        d->tv->setColumnResizeEnabled(d->tv->columnCount() - 1, false);
        d->tv->setColumnWidth(1, 0); //!< @todo A temp. hack to hide the bound column
        if (d->tv->verticalScrollBar()->isVisible()) {
            d->tv->setColumnWidth(0, d->tv->width() - 1 - d->tv->verticalScrollBar()->width());
        } else {
            d->tv->setColumnWidth(0, d->tv->width() - 1);
        }
        d->tv->triggerUpdate();
        if (d->tv->recordNumberAt(0) == 0 && records == d->tv->recordCount()) {
            d->tv->setColumnWidth(0, d->tv->width() - 1);
        }
    }
    else {
        // record source type is Query
        // Set width to 0 and disable resizing of columns that shouldn't be visible
        const KDbQueryColumnInfo::Vector fieldsExpanded(
            d->tv->cursor()->query()->fieldsExpanded(d->tv->cursor()->connection()));
        QList<int>::ConstIterator visibleColumnsToShowIt = d->visibleColumnsToShow.constBegin();
        for (int i = 0; i < fieldsExpanded.count(); ++i) {
            bool show = visibleColumnsToShowIt != d->visibleColumnsToShow.constEnd() && i == *visibleColumnsToShowIt;
            d->tv->setColumnResizeEnabled(i, show);
            if (show) {
                if (d->visibleColumnsToShow.count() == 1) {
                    d->tv->setColumnWidth(i, d->tv->width() - 1);
                }
                ++visibleColumnsToShowIt;
            }
            else {
                d->tv->setColumnWidth(i, 0);
            }
            //qDebug() << i << show;
        }
    }
}

KexiTableScrollArea* KexiComboBoxPopup::tableView()
{
    return d->tv;
}

void KexiComboBoxPopup::resize(int w, int h)
{
    //d->tv->horizontalScrollBar()->hide();
    //d->tv->verticalScrollBar()->hide();
    d->tv->move(0, 0);
    d->tv->resize(w + 1, h - 1);
    QFrame::resize(d->tv->size() + QSize(1, 1));
    update();
    updateGeometry();
}

void KexiComboBoxPopup::setMaxRecordCount(int r)
{
    d->maxRecordCount = r;
}

int KexiComboBoxPopup::maxRecordCount() const
{
    return d->maxRecordCount;
}

void KexiComboBoxPopup::slotTVItemAccepted(KDbRecordData *data, int record, int)
{
    hide();
    emit recordAccepted(data, record);
}

bool KexiComboBoxPopup::eventFilter(QObject *o, QEvent *e)
{
#if 0
    if (e->type() == QEvent::Resize) {
        qDebug() << "QResizeEvent"
                 << dynamic_cast<QResizeEvent*>(e)->size()
                 << "old=" << dynamic_cast<QResizeEvent*>(e)->oldSize()
                 << o << qobject_cast<QWidget*>(o)->geometry()
                 << "visible=" << qobject_cast<QWidget*>(o)->isVisible();
    }
#endif
    if (o == this && (e->type() == QEvent::Hide || e->type() == QEvent::FocusOut)) {
        //qDebug() << "HIDE!!!";
        emit hidden();
    } else if (e->type() == QEvent::MouseButtonPress) {
        //qDebug() << "QEvent::MousePress";
    } else if (o == d->tv) {
        //qDebug() << "QEvent::KeyPress TV";
        if (e->type() == QEvent::KeyPress) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(e);
            const int k = ke->key();
            if ((ke->modifiers() == Qt::NoModifier && (k == Qt::Key_Escape || k == Qt::Key_F4))
                    || (ke->modifiers() == Qt::AltModifier && k == Qt::Key_Up)) {
                hide();
                emit cancelled();
                emit hidden();
                return true;
            }
        }
    }
    return QFrame::eventFilter(o, e);
}

void KexiComboBoxPopup::slotDataReloadRequested()
{
    updateSize();
}

#include "kexicomboboxpopup.moc"
