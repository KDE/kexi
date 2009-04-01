/* This file is part of the KDE project
   Copyright (C) 2004 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2004-2007 Jarosław Staniek <staniek@kde.org>

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

#include "kexiquerydesignerguieditor.h"

#include <qlayout.h>
#include <qpainter.h>
#include <qdom.h>
#include <qregexp.h>
#include <QSplitter>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <Q3ValueList>
#include <QVBoxLayout>
#include <q3tl.h>

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <kexidb/field.h>
#include <kexidb/queryschema.h>
#include <kexidb/connection.h>
#include <kexidb/parser/parser.h>
#include <kexidb/parser/sqlparser.h>
#include <kexidb/utils.h>
#include <kexidb/roweditbuffer.h>
#include <kexiutils/identifier.h>
#include <kexiproject.h>
#include <KexiMainWindowIface.h>
#include <kexiinternalpart.h>
#include <kexitableview.h>
#include <kexitableviewdata.h>
#include <kexidragobjects.h>
#include <kexidatatable.h>
#include <kexi.h>
#include <kexisectionheader.h>
#include <widget/tableview/kexidataawarepropertyset.h>
#include <widget/relations/KexiRelationsView.h>
#include <widget/relations/KexiRelationsTableContainer.h>
#include <koproperty/Property.h>
#include <koproperty/Set.h>
#include "kexiquerypart.h"
#include <KexiWindow.h>
#include <KexiWindowData.h>
#include <kexi_global.h>

//! @todo remove KEXI_NO_QUERY_TOTALS later
#define KEXI_NO_QUERY_TOTALS

//! indices for table columns
#define COLUMN_ID_COLUMN 0
#define COLUMN_ID_TABLE 1
#define COLUMN_ID_VISIBLE 2
#ifdef KEXI_NO_QUERY_TOTALS
# define COLUMN_ID_SORTING 3
# define COLUMN_ID_CRITERIA 4
#else
# define COLUMN_ID_TOTALS 3
# define COLUMN_ID_SORTING 4
# define COLUMN_ID_CRITERIA 5
#endif

/*! @internal */
class KexiQueryDesignerGuiEditor::Private
{
public:
    Private()
            : fieldColumnIdentifiers(101, false/*case insens.*/) {
        droppedNewRecord = 0;
        slotTableAdded_enabled = true;
    }

    bool changeSingleCellValue(KexiDB::RecordData &record, int columnNumber,
                               const QVariant& value, KexiDB::ResultInfo* result) {
        data->clearRowEditBuffer();
        if (!data->updateRowEditBuffer(&record, columnNumber, value)
                || !data->saveRowChanges(record, true)) {
            if (result)
                *result = data->result();
            return false;
        }
        return true;
    }

    KexiTableViewData *data;
    KexiDataTable *dataTable;
    QPointer<KexiDB::Connection> conn;

    KexiRelationsView *relations;
    KexiSectionHeader *head;
    QSplitter *spl;

    /*! Used to remember in slotDroppedAtRow() what data was dropped,
     so we can create appropriate prop. set in slotRowInserted()
     This information is cached and entirely refreshed on updateColumnsData(). */
    KexiTableViewData *fieldColumnData, *tablesColumnData;

    /*! Collects identifiers selected in 1st (field) column,
     so we're able to distinguish between table identifiers selected from
     the dropdown list, and strings (e.g. expressions) entered by hand.
     This information is cached and entirely refreshed on updateColumnsData().
     The dict is filled with (char*)1 values (doesn't matter what it is);
    */
    Q3Dict<char> fieldColumnIdentifiers;

    KexiDataAwarePropertySet* sets;
    KexiDB::RecordData *droppedNewRecord;

    QString droppedNewTable, droppedNewField;

bool slotTableAdded_enabled : 1;
};

static bool isAsterisk(const QString& tableName, const QString& fieldName)
{
    return tableName == "*" || fieldName.endsWith("*");
}

//! @internal \return true if sorting is allowed for \a fieldName and \a tableName
static bool sortingAllowed(const QString& fieldName, const QString& tableName)
{
    return !(fieldName == "*" || (fieldName.isEmpty() && tableName == "*"));
}

//=========================================================

KexiQueryDesignerGuiEditor::KexiQueryDesignerGuiEditor(
    QWidget *parent)
        : KexiView(parent)
        , d(new Private())
{
    d->conn = KexiMainWindowIface::global()->project()->dbConnection();

    d->spl = new QSplitter(Qt::Vertical, this);
    d->spl->setChildrenCollapsible(false);
    d->relations = new KexiRelationsView(d->spl);
    d->spl->addWidget(d->relations);
    d->relations->setObjectName("relations");
    connect(d->relations, SIGNAL(tableAdded(KexiDB::TableSchema&)),
            this, SLOT(slotTableAdded(KexiDB::TableSchema&)));
    connect(d->relations, SIGNAL(tableHidden(KexiDB::TableSchema&)),
            this, SLOT(slotTableHidden(KexiDB::TableSchema&)));
    connect(d->relations, SIGNAL(appendFields(KexiDB::TableOrQuerySchema&, const QStringList&)),
            this, SLOT(slotAppendFields(KexiDB::TableOrQuerySchema&, const QStringList&)));

    d->head = new KexiSectionHeader(i18n("Query Columns"), Qt::Vertical, d->spl);
    d->spl->addWidget(d->head);
    d->dataTable = new KexiDataTable(d->head, false);
    d->head->setWidget(d->dataTable);
    d->dataTable->setObjectName("guieditor_dataTable");
    d->dataTable->dataAwareObject()->setSpreadSheetMode();

    d->data = new KexiTableViewData(); //just empty data
    d->sets = new KexiDataAwarePropertySet(this, d->dataTable->dataAwareObject());
    initTableColumns();
    initTableRows();

    Q3ValueList<int> c;
    c << COLUMN_ID_COLUMN << COLUMN_ID_TABLE << COLUMN_ID_CRITERIA;
    if (d->dataTable->tableView()/*sanity*/) {
        d->dataTable->tableView()->adjustColumnWidthToContents(COLUMN_ID_VISIBLE);
        d->dataTable->tableView()->adjustColumnWidthToContents(COLUMN_ID_SORTING);
        d->dataTable->tableView()->maximizeColumnsWidth(c);
        d->dataTable->tableView()->setDropsAtRowEnabled(true);
        connect(d->dataTable->tableView(), SIGNAL(dragOverRow(KexiDB::RecordData*, int, QDragMoveEvent*)),
                this, SLOT(slotDragOverTableRow(KexiDB::RecordData*, int, QDragMoveEvent*)));
        connect(d->dataTable->tableView(), SIGNAL(droppedAtRow(KexiDB::RecordData*, int, QDropEvent*, KexiDB::RecordData*&)),
                this, SLOT(slotDroppedAtRow(KexiDB::RecordData*, int, QDropEvent*, KexiDB::RecordData*&)));
        connect(d->dataTable->tableView(), SIGNAL(newItemAppendedForAfterDeletingInSpreadSheetMode()),
                this, SLOT(slotNewItemAppendedForAfterDeletingInSpreadSheetMode()));
    }
    connect(d->data, SIGNAL(aboutToChangeCell(KexiDB::RecordData*, int, QVariant&, KexiDB::ResultInfo*)),
            this, SLOT(slotBeforeCellChanged(KexiDB::RecordData*, int, QVariant&, KexiDB::ResultInfo*)));
    connect(d->data, SIGNAL(rowInserted(KexiDB::RecordData*, uint, bool)),
            this, SLOT(slotRowInserted(KexiDB::RecordData*, uint, bool)));
    connect(d->relations, SIGNAL(tablePositionChanged(KexiRelationsTableContainer*)),
            this, SLOT(slotTablePositionChanged(KexiRelationsTableContainer*)));
    connect(d->relations, SIGNAL(aboutConnectionRemove(KexiRelationsConnection*)),
            this, SLOT(slotAboutConnectionRemove(KexiRelationsConnection*)));

// QVBoxLayout *l = new QVBoxLayout(this);
// l->addWidget(d->spl);

    addChildView(d->relations);
    addChildView(d->dataTable);
    //setViewWidget(d->dataTable, true);
    setViewWidget(d->spl, false/* no focus proxy*/);
    setFocusProxy(d->dataTable);
    d->relations->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    d->head->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    updateGeometry();
    d->spl->setSizes(Q3ValueList<int>() << 800 << 400);
}

KexiQueryDesignerGuiEditor::~KexiQueryDesignerGuiEditor()
{
    delete d;
}

void
KexiQueryDesignerGuiEditor::initTableColumns()
{
    KexiTableViewColumn *col1 = new KexiTableViewColumn("column", KexiDB::Field::Enum, i18n("Column"),
            i18n("Describes field name or expression for the designed query."));
    col1->setRelatedDataEditable(true);

    d->fieldColumnData = new KexiTableViewData(KexiDB::Field::Text, KexiDB::Field::Text);
    col1->setRelatedData(d->fieldColumnData);
    d->data->addColumn(col1);

    KexiTableViewColumn *col2 = new KexiTableViewColumn("table", KexiDB::Field::Enum, i18n("Table"),
            i18n("Describes table for a given field. Can be empty."));
    d->tablesColumnData = new KexiTableViewData(KexiDB::Field::Text, KexiDB::Field::Text);
    col2->setRelatedData(d->tablesColumnData);
    d->data->addColumn(col2);

    KexiTableViewColumn *col3 = new KexiTableViewColumn("visible", KexiDB::Field::Boolean, i18n("Visible"),
            i18n("Describes visibility for a given field or expression."));
    col3->field()->setDefaultValue(QVariant(false));
    col3->field()->setNotNull(true);
    d->data->addColumn(col3);

#ifndef KEXI_NO_QUERY_TOTALS
    KexiTableViewColumn *col4 = new KexiTableViewColumn("totals", KexiDB::Field::Enum, i18n("Totals"),
            i18n("Describes a way of computing totals for a given field or expression."));
    QVector<QString> totalsTypes;
    totalsTypes.append(i18n("Group by"));
    totalsTypes.append(i18n("Sum"));
    totalsTypes.append(i18n("Average"));
    totalsTypes.append(i18n("Min"));
    totalsTypes.append(i18n("Max"));
    //todo: more like this
    col4->field()->setEnumHints(totalsTypes);
    d->data->addColumn(col4);
#endif

    KexiTableViewColumn *col5 = new KexiTableViewColumn("sort", KexiDB::Field::Enum, i18n("Sorting"),
            i18n("Describes a way of sorting for a given field."));
    QVector<QString> sortTypes;
    sortTypes.append("");
    sortTypes.append(i18n("Ascending"));
    sortTypes.append(i18n("Descending"));
    col5->field()->setEnumHints(sortTypes);
    d->data->addColumn(col5);

    KexiTableViewColumn *col6 = new KexiTableViewColumn("criteria", KexiDB::Field::Text, i18n("Criteria"),
            i18n("Describes the criteria for a given field or expression."));
    d->data->addColumn(col6);

// KexiTableViewColumn *col7 = new KexiTableViewColumn(i18n("Or"), KexiDB::Field::Text);
// d->data->addColumn(col7);
}

void KexiQueryDesignerGuiEditor::initTableRows()
{
    d->data->deleteAllRows();
    //const int columns = d->data->columnsCount();
    for (int i = 0; i < (int)d->sets->size(); i++) {
        KexiDB::RecordData* record;
        d->data->append(record = d->data->createItem());
        (*record)[COLUMN_ID_VISIBLE] = QVariant(false);
    }
    d->dataTable->dataAwareObject()->setData(d->data);

    updateColumnsData();
}

void KexiQueryDesignerGuiEditor::updateColumnsData()
{
    d->dataTable->dataAwareObject()->acceptRowEdit();

    QStringList sortedTableNames;
    foreach(KexiRelationsTableContainer* cont, *d->relations->tables()) {
        sortedTableNames += cont->schema()->name();
    }
    qHeapSort(sortedTableNames);

    //several tables can be hidden now, so remove rows for these tables
    QList<int> rowsToDelete;
    for (int r = 0; r < (int)d->sets->size(); r++) {
        KoProperty::Set *set = d->sets->at(r);
        if (set) {
            QString tableName = (*set)["table"].value().toString();
            QString fieldName = (*set)["field"].value().toString();
            const bool allTablesAsterisk = tableName == "*" && d->relations->tables()->isEmpty();
            const bool fieldNotFound = tableName != "*"
                                       && !(*set)["isExpression"].value().toBool()
                                       && sortedTableNames.end() == qFind(sortedTableNames.begin(), sortedTableNames.end(), tableName);

            if (allTablesAsterisk || fieldNotFound) {
                //table not found: mark this line for later removal
                rowsToDelete += r;
            }
        }
    }
    d->data->deleteRows(rowsToDelete);

    //update 'table' and 'field' columns
    d->tablesColumnData->deleteAllRows();
    d->fieldColumnData->deleteAllRows();
    d->fieldColumnIdentifiers.clear();

    KexiDB::RecordData *record = d->fieldColumnData->createItem();
    (*record)[COLUMN_ID_COLUMN] = "*";
    (*record)[COLUMN_ID_TABLE] = "*";
    d->fieldColumnData->append(record);
    d->fieldColumnIdentifiers.insert((*record)[COLUMN_ID_COLUMN].toString(), (char*)1); //cache

// tempData()->clearQuery();
    tempData()->unregisterForTablesSchemaChanges();
    foreach(const QString& tableName, sortedTableNames) {
        //table
        /*! @todo what about query? */
        KexiDB::TableSchema *table = d->relations->tables()->value(tableName)->schema()->table();
        d->conn->registerForTableSchemaChanges(*tempData(), *table); //this table will be used
        record = d->tablesColumnData->createItem();
        (*record)[COLUMN_ID_COLUMN] = table->name();
        (*record)[COLUMN_ID_TABLE] = (*record)[COLUMN_ID_COLUMN];
        d->tablesColumnData->append(record);
        //fields
        record = d->fieldColumnData->createItem();
        (*record)[COLUMN_ID_COLUMN] = table->name() + ".*";
        (*record)[COLUMN_ID_TABLE] = (*record)[COLUMN_ID_COLUMN];
        d->fieldColumnData->append(record);
        d->fieldColumnIdentifiers.insert((*record)[COLUMN_ID_COLUMN].toString(), (char*)1); //cache
//  for (KexiDB::Field::ListIterator t_it = table->fieldsIterator();t_it.current();++t_it) {
        foreach(KexiDB::Field *field, *table->fields()) {
            record = d->fieldColumnData->createItem();
            (*record)[COLUMN_ID_COLUMN] = table->name() + "." + field->name();
            (*record)[COLUMN_ID_TABLE] = QString("  ") + field->name();
            d->fieldColumnData->append(record);
            d->fieldColumnIdentifiers.insert((*record)[COLUMN_ID_COLUMN].toString(), (char*)1); //cache
        }
    }
//TODO
}

KexiRelationsView *KexiQueryDesignerGuiEditor::relationsView() const
{
    return d->relations;
}

KexiQueryPart::TempData *
KexiQueryDesignerGuiEditor::tempData() const
{
    return static_cast<KexiQueryPart::TempData*>(window()->data());
}

static QString msgCannotSwitch_EmptyDesign()
{
    return i18n("Cannot switch to data view, because query design is empty.\n"
                "First, please create your design.");
}

bool
KexiQueryDesignerGuiEditor::buildSchema(QString *errMsg)
{
    //build query schema
    KexiQueryPart::TempData * temp = tempData();
    if (temp->query()) {
        temp->clearQuery();
    } else {
        temp->setQuery(new KexiDB::QuerySchema());
    }

    //add tables
    foreach(KexiRelationsTableContainer* cont, *d->relations->tables()) {
        /*! @todo what about query? */
        temp->query()->addTable(cont->schema()->table());
    }

    //add fields, also build:
    // -WHERE expression
    // -ORDER BY list
    KexiDB::BaseExpr *whereExpr = 0;
    const uint count = qMin(d->data->count(), d->sets->size());
    bool fieldsFound = false;
    KexiTableViewData::Iterator it(d->data->constBegin());
    for (uint i = 0; i < count && it != d->data->constEnd(); ++it, i++) {
        if (!(**it)[COLUMN_ID_TABLE].isNull()
                && (**it)[COLUMN_ID_COLUMN].isNull()) {
            //show message about missing field name, and set focus to that cell
            kexipluginsdbg << "no field provided!";
            d->dataTable->dataAwareObject()->setCursorPosition(i, 0);
            if (errMsg)
                *errMsg = i18n("Select column for table \"%1\"",
                               (**it)[COLUMN_ID_TABLE].toString());
            return false;
        }

        KoProperty::Set *set = d->sets->at(i);
        if (set) {
            QString tableName = (*set)["table"].value().toString().trimmed();
            QString fieldName = (*set)["field"].value().toString();
            QString fieldAndTableName = fieldName;
            KexiDB::Field *currentField = 0; // will be set if this column is a single field
            KexiDB::QueryColumnInfo* currentColumn = 0;
            if (!tableName.isEmpty())
                fieldAndTableName.prepend(tableName + ".");
            const bool fieldVisible = (*set)["visible"].value().toBool();
            QString criteriaStr = (*set)["criteria"].value().toString();
            QByteArray alias((*set)["alias"].value().toByteArray());
            if (!criteriaStr.isEmpty()) {
                int token;
                KexiDB::BaseExpr *criteriaExpr = parseExpressionString(criteriaStr, token,
                                                 true/*allowRelationalOperator*/);
                if (!criteriaExpr) {//for sanity
                    if (errMsg)
                        *errMsg = i18n("Invalid criteria \"%1\"", criteriaStr);
                    delete whereExpr;
                    return false;
                }
                //build relational expression for column variable
                KexiDB::VariableExpr *varExpr = new KexiDB::VariableExpr(fieldAndTableName);
                criteriaExpr = new KexiDB::BinaryExpr(KexiDBExpr_Relational, varExpr, token, criteriaExpr);
                //critera ok: add it to WHERE section
                if (whereExpr)
                    whereExpr = new KexiDB::BinaryExpr(KexiDBExpr_Logical, whereExpr, AND, criteriaExpr);
                else //first expr.
                    whereExpr = criteriaExpr;
            }
            if (tableName.isEmpty()) {
                if ((*set)["isExpression"].value().toBool() == true) {
                    //add expression column
                    int dummyToken;
                    KexiDB::BaseExpr *columnExpr = parseExpressionString(fieldName, dummyToken,
                                                   false/*!allowRelationalOperator*/);
                    if (!columnExpr) {
                        if (errMsg)
                            *errMsg = i18n("Invalid expression \"%1\"", fieldName);
                        return false;
                    }
                    temp->query()->addExpression(columnExpr, fieldVisible);
                    if (fieldVisible)
                        fieldsFound = true;
                    if (!alias.isEmpty())
                        temp->query()->setColumnAlias(temp->query()->fieldCount() - 1, alias);
                }
                //TODO
            } else if (tableName == "*") {
                //all tables asterisk
                temp->query()->addAsterisk(new KexiDB::QueryAsterisk(temp->query(), 0), fieldVisible);
                if (fieldVisible)
                    fieldsFound = true;
                continue;
            } else {
                KexiDB::TableSchema *t = d->conn->tableSchema(tableName);
                if (fieldName == "*") {
                    //single-table asterisk: <tablename> + ".*" + number
                    temp->query()->addAsterisk(new KexiDB::QueryAsterisk(temp->query(), t), fieldVisible);
                    if (fieldVisible)
                        fieldsFound = true;
                } else {
                    if (!t) {
                        kexipluginswarn << "query designer: NO TABLE '"
                        << (*set)["table"].value().toString() << "'";
                        continue;
                    }
                    currentField = t->field(fieldName);
                    if (!currentField) {
                        kexipluginswarn << "query designer: NO FIELD '" << fieldName << "'";
                        continue;
                    }
                    if (!fieldVisible && criteriaStr.isEmpty() && set->contains("isExpression")
                            && (*set)["sorting"].value().toString() != "nosorting") {
                        kexipluginsdbg << "invisible field with sorting: do not add it to the fields list";
                        continue;
                    }
                    temp->query()->addField(currentField, fieldVisible);
                    currentColumn = temp->query()->expandedOrInternalField(
                                        temp->query()->fieldsExpanded().count() - 1);
                    if (fieldVisible)
                        fieldsFound = true;
                    if (!alias.isEmpty())
                        temp->query()->setColumnAlias(temp->query()->fieldCount() - 1, alias);
                }
            }
        } else {//!set
            kexipluginsdbg << (**it)[COLUMN_ID_TABLE].toString();
        }
    }
    if (!fieldsFound) {
        if (errMsg)
            *errMsg = msgCannotSwitch_EmptyDesign();
        return false;
    }
    if (whereExpr)
        kexipluginsdbg << "KexiQueryDesignerGuiEditor::buildSchema(): setting CRITERIA: "
        << whereExpr->debugString();

    //set always, because if whereExpr==NULL,
    //this will clear prev. expr
    temp->query()->setWhereExpression(whereExpr);

    //add relations (looking for connections)
    foreach(KexiRelationsConnection* conn, *d->relations->connections()) {
        KexiRelationsTableContainer *masterTable = conn->masterTable();
        KexiRelationsTableContainer *detailsTable = conn->detailsTable();

        /*! @todo what about query? */
        temp->query()->addRelationship(
            masterTable->schema()->table()->field(conn->masterField()),
            detailsTable->schema()->table()->field(conn->detailsField()));
    }

    // Add sorting information (ORDER BY) - we can do that only now
    //  after all QueryColumnInfo items are instantiated
    KexiDB::OrderByColumnList orderByColumns;
    it = d->data->constBegin();
    int fieldNumber = -1; //field number (empty rows are omitted)
    for (uint i = 0/*row number*/; i < count && it != d->data->constEnd(); ++it, i++) {
        KoProperty::Set *set = d->sets->at(i);
        if (!set)
            continue;
        fieldNumber++;
        KexiDB::Field *currentField = 0;
        KexiDB::QueryColumnInfo *currentColumn = 0;
        QString sortingString((*set)["sorting"].value().toString());
        if (sortingString != "ascending" && sortingString != "descending")
            continue;
        if (!(*set)["visible"].value().toBool()) {
            // this row defines invisible field but contains sorting information,
            // what means KexiDB::Field should be used as a reference for this sorting
            // Note1: alias is not supported here.

            // Try to find a field (not mentioned after SELECT):
            currentField = temp->query()->findTableField((*set)["field"].value().toString());
            if (!currentField) {
                kexipluginswarn << "KexiQueryDesignerGuiEditor::buildSchema(): NO FIELD '"
                << (*set)["field"].value().toString()
                << " available for sorting";
                continue;
            }
            orderByColumns.appendField(*currentField, sortingString == "ascending");
            continue;
        }
        currentField = temp->query()->field((uint)fieldNumber);
        if (!currentField || currentField->isExpression() || currentField->isQueryAsterisk())
//! @todo support expressions here
            continue;
//! @todo ok, but not for expressions
        QString aliasString((*set)["alias"].value().toString());
        currentColumn = temp->query()->columnInfo(
                            (*set)["table"].value().toString() + "."
                            + (aliasString.isEmpty() ? currentField->name() : aliasString));
        if (currentField && currentColumn) {
            if (currentColumn->visible)
                orderByColumns.appendColumn(*currentColumn, sortingString == "ascending");
            else if (currentColumn->field)
                orderByColumns.appendField(*currentColumn->field, sortingString == "ascending");
        }
    }
    temp->query()->setOrderByColumnList(orderByColumns);

    temp->query()->debug();
    temp->registerTableSchemaChanges(temp->query());
    //TODO?
    return true;
}

tristate
KexiQueryDesignerGuiEditor::beforeSwitchTo(Kexi::ViewMode mode, bool &dontStore)
{
    kexipluginsdbg << "KexiQueryDesignerGuiEditor::beforeSwitch()" << mode;

    if (!d->dataTable->dataAwareObject()->acceptRowEdit())
        return cancelled;

    if (mode == Kexi::DesignViewMode) {
        return true;
    } else if (mode == Kexi::DataViewMode) {
//  if (!d->dataTable->dataAwareObject()->acceptRowEdit())
        //  return cancelled;

        if (!isDirty() && window()->neverSaved()) {
            KMessageBox::information(this, msgCannotSwitch_EmptyDesign());
            return cancelled;
        }
        if (isDirty() || !tempData()->query()) {
            //remember current design in a temporary structure
            dontStore = true;
            QString errMsg;
            //build schema; problems are not allowed
            if (!buildSchema(&errMsg)) {
                KMessageBox::sorry(this, errMsg);
                return cancelled;
            }
        }
        //TODO
        return true;
    } else if (mode == Kexi::TextViewMode) {
        dontStore = true;
        //build schema; ignore problems
        buildSchema();
        /*  if (tempData()->query && tempData()->query->fieldCount()==0) {
              //no fields selected: let's add "*" (all-tables asterisk),
              // otherwise SQL statement will be invalid
              tempData()->query->addAsterisk( new KexiDB::QueryAsterisk( tempData()->query ) );
            }*/
        //todo
        return true;
    }

    return false;
}

tristate
KexiQueryDesignerGuiEditor::afterSwitchFrom(Kexi::ViewMode mode)
{
    const bool was_dirty = isDirty();
    if (mode == Kexi::NoViewMode || (mode == Kexi::DataViewMode && !tempData()->query())) {
        //this is not a SWITCH but a fresh opening in this view mode
        if (!window()->neverSaved()) {
            if (!loadLayout()) {
                //err msg
                window()->setStatus(d->conn,
                                    i18n("Query definition loading failed."),
                                    i18n("Query design may be corrupted so it could not be opened even in text view.\n"
                                         "You can delete the query and create it again."));
                return false;
            }
            // Invalid queries case:
            // KexiWindow::switchToViewMode() first opens DesignViewMode,
            // and then KexiQueryPart::loadSchemaData() doesn't allocate QuerySchema object
            // do we're carefully looking at window()->schemaData()
            KexiDB::QuerySchema * q = dynamic_cast<KexiDB::QuerySchema *>(window()->schemaData());
            if (q) {
                KexiDB::ResultInfo result;
                showFieldsForQuery(q, result);
                if (!result.success) {
                    window()->setStatus(&result, i18n("Query definition loading failed."));
                    tempData()->proposeOpeningInTextViewModeBecauseOfProblems = true;
                    return false;
                }
            }
//! @todo load global query properties
        }
    } else if (mode == Kexi::TextViewMode || mode == Kexi::DataViewMode) {
        // Switch from text or data view. In the second case, the design could be changed as well
        // because there could be changes made in the text view before switching to the data view.
        if (tempData()->queryChangedInPreviousView) {
            //previous view changed query data
            //-clear and regenerate GUI items
            initTableRows();
            //todo
            if (tempData()->query()) {
                //there is a query schema to show
                showTablesForQuery(tempData()->query());
                //-show fields
                KexiDB::ResultInfo result;
                showFieldsAndRelationsForQuery(tempData()->query(), result);
                if (!result.success) {
                    window()->setStatus(&result, i18n("Query definition loading failed."));
                    return false;
                }
            } else {
                d->relations->clear();
            }
        }
//! @todo load global query properties
    }

    if (mode == Kexi::DataViewMode) {
        //this is just a SWITCH from data view
        //set cursor if needed:
        if (d->dataTable->dataAwareObject()->currentRow() < 0
                || d->dataTable->dataAwareObject()->currentColumn() < 0) {
            d->dataTable->dataAwareObject()->ensureCellVisible(0, 0);
            d->dataTable->dataAwareObject()->setCursorPosition(0, 0);
        }
    }
    tempData()->queryChangedInPreviousView = false;
    setFocus(); //to allow shared actions proper update
    if (!was_dirty)
        setDirty(false);
    return true;
}


KexiDB::SchemaData*
KexiQueryDesignerGuiEditor::storeNewData(const KexiDB::SchemaData& sdata, bool &cancel)
{
    if (!d->dataTable->dataAwareObject()->acceptRowEdit()) {
        cancel = true;
        return 0;
    }
    QString errMsg;
    KexiQueryPart::TempData * temp = tempData();
    if (!temp->query() || !(viewMode() == Kexi::DesignViewMode && !temp->queryChangedInPreviousView)) {
        //only rebuild schema if it has not been rebuilt previously
        if (!buildSchema(&errMsg)) {
            KMessageBox::sorry(this, errMsg);
            cancel = true;
            return 0;
        }
    }
    (KexiDB::SchemaData&)*temp->query() = sdata; //copy main attributes

    bool ok = d->conn->storeObjectSchemaData(
                  *temp->query(), true /*newObject*/);
    window()->setId(temp->query()->id());

    if (ok)
        ok = storeLayout();

// temp->query = 0; //will be returned, so: don't keep it
    if (!ok) {
        temp->setQuery(0);
//  delete query;
        return 0;
    }
    return temp->takeQuery(); //will be returned, so: don't keep it in temp
}

tristate KexiQueryDesignerGuiEditor::storeData(bool dontAsk)
{
    if (!d->dataTable->dataAwareObject()->acceptRowEdit())
        return cancelled;

    const bool was_dirty = isDirty();
    tristate res = KexiView::storeData(dontAsk); //this clears dirty flag
    if (true == res)
        res = buildSchema();
    if (true == res)
        res = storeLayout();
    if (true != res) {
        if (was_dirty)
            setDirty(true);
    }
    return res;
}

void KexiQueryDesignerGuiEditor::showTablesForQuery(KexiDB::QuerySchema *query)
{
//replaced by code below that preserves geometries d->relations->clear();

    // instead of hiding all tables and showing some tables,
    // show only these new and hide these unncecessary; the same for connections)
    d->slotTableAdded_enabled = false; //speedup
    d->relations->removeAllConnections(); //connections will be recreated
    d->relations->hideAllTablesExcept(query->tables());
    foreach(KexiDB::TableSchema* table, *query->tables()) {
        d->relations->addTable(table);
    }

    d->slotTableAdded_enabled = true;
    updateColumnsData();
}

void KexiQueryDesignerGuiEditor::addConnection(
    KexiDB::Field *masterField, KexiDB::Field *detailsField)
{
    SourceConnection conn;
    conn.masterTable = masterField->table()->name(); //<<<TODO
    conn.masterField = masterField->name();
    conn.detailsTable = detailsField->table()->name();
    conn.detailsField = detailsField->name();
    d->relations->addConnection(conn);
}

void KexiQueryDesignerGuiEditor::showFieldsForQuery(KexiDB::QuerySchema *query, KexiDB::ResultInfo& result)
{
    showFieldsOrRelationsForQueryInternal(query, true, false, result);
}

void KexiQueryDesignerGuiEditor::showRelationsForQuery(KexiDB::QuerySchema *query, KexiDB::ResultInfo& result)
{
    showFieldsOrRelationsForQueryInternal(query, false, true, result);
}

void KexiQueryDesignerGuiEditor::showFieldsAndRelationsForQuery(KexiDB::QuerySchema *query,
        KexiDB::ResultInfo& result)
{
    showFieldsOrRelationsForQueryInternal(query, true, true, result);
}

void KexiQueryDesignerGuiEditor::showFieldsOrRelationsForQueryInternal(
    KexiDB::QuerySchema *query, bool showFields, bool showRelations, KexiDB::ResultInfo& result)
{
    result.clear();
    const bool was_dirty = isDirty();

    //1. Show explicitly declared relations:
    if (showRelations) {
        foreach(KexiDB::Relationship *rel, *query->relationships()) {
//! @todo: now only sigle-field relationships are implemented!
            KexiDB::Field *masterField = rel->masterIndex()->fields()->first();
            KexiDB::Field *detailsField = rel->detailsIndex()->fields()->first();
            addConnection(masterField, detailsField);
        }
    }

    //2. Collect information about criterias
    // --this must be top level chain of AND's
    // --this will also show joins as: [table1.]field1 = [table2.]field2
    KexiUtils::CaseInsensitiveHash<QString, KexiDB::BaseExpr*> criterias;
    KexiDB::BaseExpr* e = query->whereExpression();
    KexiDB::BaseExpr* eItem = 0;
    while (e) {
        //eat parentheses because the expression can be (....) AND (... AND ... )
        while (e && e->toUnary() && e->token() == '(')
            e = e->toUnary()->arg();

        if (e->toBinary() && e->token() == AND) {
            eItem = e->toBinary()->left();
            e = e->toBinary()->right();
        } else {
            eItem = e;
            e = 0;
        }

        //eat parentheses
        while (eItem && eItem->toUnary() && eItem->token() == '(')
            eItem = eItem->toUnary()->arg();

        if (!eItem)
            continue;

        kexidbg << eItem->toString();
        KexiDB::BinaryExpr* binary = eItem->toBinary();
        if (binary && eItem->exprClass() == KexiDBExpr_Relational) {
            KexiDB::Field *leftField = 0, *rightField = 0;
            if (eItem->token() == '='
                    && binary->left()->toVariable()
                    && binary->right()->toVariable()
                    && (leftField = query->findTableField(binary->left()->toString()))
                    && (rightField = query->findTableField(binary->right()->toString()))) {
//! @todo move this check to parser on QuerySchema creation
//!       or to QuerySchema creation (WHERE expression should be then simplified
//!       by removing joins

                //this is relationship defined as following JOIN: [table1.]field1 = [table2.]field2
                if (showRelations) {
//! @todo testing primary key here is too simplified; maybe look ar isForeignKey() or indices..
//! @todo what about multifield joins?
                    if (leftField->isPrimaryKey())
                        addConnection(leftField /*master*/, rightField /*details*/);
                    else
                        addConnection(rightField /*master*/, leftField /*details*/);
//! @todo addConnection() should have "bool oneToOne" arg, for 1-to-1 relations
                }
            } else if (binary->left()->toVariable()) {
                //this is: variable , op , argument
                //store variable -> argument:
                criterias.insertMulti(binary->left()->toVariable()->name, binary->right());
            } else if (binary->right()->toVariable()) {
                //this is: argument , op , variable
                //store variable -> argument:
                criterias.insertMulti(binary->right()->toVariable()->name, binary->left());
            }
        }
    } //while

    if (!showFields)
        return;

    //3. show fields (including * and table.*)
    uint row_num = 0;
    QSet<KexiDB::BaseExpr*> usedCriterias; // <-- used criterias will be saved here
    //     so in step 4. we will be able to add
    //     remaining invisible columns with criterias
    query->debug();
    foreach(KexiDB::Field* field, *query->fields()) {
        field->debug();
    }
    foreach(KexiDB::Field* field, *query->fields()) {
        //append a new row
        QString tableName, fieldName, columnAlias, criteriaString;
        KexiDB::BinaryExpr *criteriaExpr = 0;
        KexiDB::BaseExpr *criteriaArgument = 0;
        if (field->isQueryAsterisk()) {
            if (field->table()) {//single-table asterisk
                tableName = field->table()->name();
                fieldName = "*";
            } else {//all-tables asterisk
                tableName = "*";
                fieldName = "";
            }
        } else {
            columnAlias = query->columnAlias(row_num);
            if (field->isExpression()) {
//    if (columnAlias.isEmpty()) {
//     columnAlias = i18n("expression", "expr%1").arg(row_num); //TODO
//    }
//    if (columnAlias.isEmpty())
//TODO: ok? perhaps do not allow to omit aliases?
                fieldName = field->expression()->toString();
//    else
//     fieldName = columnAlias + ": " + field->expression()->toString();
            } else {
                tableName = field->table()->name();
                fieldName = field->name();
                criteriaArgument = criterias.value(fieldName);
                if (!criteriaArgument) {//try table.field
                    criteriaArgument = criterias.value(tableName + "." + fieldName);
                }
                if (criteriaArgument) {//criteria expression is just a parent of argument
                    criteriaExpr = criteriaArgument->parent()->toBinary();
                    usedCriterias.insert(criteriaArgument); //save info. about used criteria
                }
            }
        }
        //create new row data
        KexiDB::RecordData *newRecord = createNewRow(tableName, fieldName, true /* visible*/);
        if (criteriaExpr) {
//! @todo fix for !INFIX operators
            if (criteriaExpr->token() == '=')
                criteriaString = criteriaArgument->toString();
            else
                criteriaString = criteriaExpr->tokenToString() + " " + criteriaArgument->toString();
            (*newRecord)[COLUMN_ID_CRITERIA] = criteriaString;
        }
        d->dataTable->dataAwareObject()->insertItem(newRecord, row_num);
        //OK, row inserted: create a new set for it
        KoProperty::Set &set = *createPropertySet(row_num, tableName, fieldName, true/*new one*/);
        if (!columnAlias.isEmpty())
            set["alias"].setValue(columnAlias, false);
        if (!criteriaString.isEmpty())
            set["criteria"].setValue(criteriaString, false);
        if (field->isExpression()) {
            if (!d->changeSingleCellValue(*newRecord, COLUMN_ID_COLUMN,
                                          QVariant(columnAlias + ": " + field->expression()->toString()), &result))
                return; //problems with setting column expression
        }
        row_num++;
    }

    //4. show ORDER BY information
    d->data->clearRowEditBuffer();
    const KexiDB::OrderByColumnList orderByColumns(query->orderByColumnList());
    QHash<KexiDB::QueryColumnInfo*, int> columnsOrder(
        query->columnsOrder(KexiDB::QuerySchema::UnexpandedListWithoutAsterisks));
    for (KexiDB::OrderByColumn::ListConstIterator orderByColumnIt(orderByColumns.constBegin());
            orderByColumnIt != orderByColumns.constEnd(); ++orderByColumnIt) {
        KexiDB::OrderByColumn* orderByColumn = *orderByColumnIt;
        KexiDB::QueryColumnInfo *column = orderByColumn->column();
        KexiDB::RecordData *record = 0;
        KoProperty::Set *rowPropertySet = 0;
        if (column) {
            //sorting for visible column
            if (column->visible) {
                if (columnsOrder.contains(column)) {
                    const int columnPosition = columnsOrder.value(column);
                    record = d->data->at(columnPosition);
                    rowPropertySet = d->sets->at(columnPosition);
                    kexipluginsdbg << "KexiQueryDesignerGuiEditor::showFieldsOrRelationsForQueryInternal():\n\t"
                    "Setting \"" << orderByColumn->debugString() << "\" sorting for row #"
                    << columnPosition;
                }
            }
        } else if (orderByColumn->field()) {
            //this will be presented as invisible field: create new row
            KexiDB::Field* field = orderByColumn->field();
            QString tableName(field->table() ? field->table()->name() : QString());
            record = createNewRow(tableName, field->name(), false /* !visible*/);
            d->dataTable->dataAwareObject()->insertItem(record, row_num);
            rowPropertySet = createPropertySet(row_num, tableName, field->name(), true /*newOne*/);
            propertySetSwitched();
            kexipluginsdbg << "KexiQueryDesignerGuiEditor::showFieldsOrRelationsForQueryInternal():\n\t"
            "Setting \"" << orderByColumn->debugString() << "\" sorting for invisible field "
            << field->name() << ", table " << tableName << " -row #" << row_num;
            row_num++;
        }
        //alter sorting for either existing or new row
        if (record && rowPropertySet) {
            d->data->updateRowEditBuffer(record, COLUMN_ID_SORTING,
                                         orderByColumn->ascending() ? 1 : 2); // this will automatically update "sorting" property
            // in slotBeforeCellChanged()
            d->data->saveRowChanges(*record, true);
            (*rowPropertySet)["sorting"].clearModifiedFlag(); // this property should look "fresh"
            if (!(*record)[COLUMN_ID_VISIBLE].toBool()) //update
                (*rowPropertySet)["visible"].setValue(QVariant(false), false/*rememberOldValue*/);
        }
    }

    //5. Show fields for unused criterias (with "Visible" column set to false)
    foreach(
        KexiDB::BaseExpr *criteriaArgument, // <-- contains field or table.field
        criterias)
    {
        if (usedCriterias.contains(criteriaArgument))
            continue;
        //unused: append a new row
        KexiDB::BinaryExpr *criteriaExpr = criteriaArgument->parent()->toBinary();
        if (!criteriaExpr) {
            kexipluginswarn << "KexiQueryDesignerGuiEditor::showFieldsOrRelationsForQueryInternal(): "
            "criteriaExpr is not a binary expr";
            continue;
        }
        KexiDB::VariableExpr *columnNameArgument = criteriaExpr->left()->toVariable(); //left or right
        if (!columnNameArgument) {
            columnNameArgument = criteriaExpr->right()->toVariable();
            if (!columnNameArgument) {
                kexipluginswarn << "KexiQueryDesignerGuiEditor::showFieldsOrRelationsForQueryInternal(): "
                "columnNameArgument is not a variable (table or table.field) expr";
                continue;
            }
        }
        KexiDB::Field* field = 0;
        if (!columnNameArgument->name.contains('.') && query->tables()->count() == 1) {
            //extreme case: only field name provided for one-table query:
            field = query->tables()->first()->field(columnNameArgument->name);
        } else {
            field = query->findTableField(columnNameArgument->name);
        }

        if (!field) {
            kexipluginswarn << "KexiQueryDesignerGuiEditor::showFieldsOrRelationsForQueryInternal(): "
            "no columnInfo found in the query for name \"" << columnNameArgument->name;
            continue;
        }
        QString tableName, fieldName, columnAlias, criteriaString;
//! @todo what about ALIAS?
        tableName = field->table()->name();
        fieldName = field->name();
        //create new row data
        KexiDB::RecordData *newRecord = createNewRow(tableName, fieldName, false /* !visible*/);
        if (criteriaExpr) {
//! @todo fix for !INFIX operators
            if (criteriaExpr->token() == '=')
                criteriaString = criteriaArgument->toString();
            else
                criteriaString = criteriaExpr->tokenToString() + " " + criteriaArgument->toString();
            (*newRecord)[COLUMN_ID_CRITERIA] = criteriaString;
        }
        d->dataTable->dataAwareObject()->insertItem(newRecord, row_num);
        //OK, row inserted: create a new set for it
        KoProperty::Set &set = *createPropertySet(row_num++, tableName, fieldName, true/*new one*/);
//! @todo  if (!columnAlias.isEmpty())
//! @todo   set["alias"].setValue(columnAlias, false);
////  if (!criteriaString.isEmpty())
        set["criteria"].setValue(criteriaString, false);
        set["visible"].setValue(QVariant(false), false);
    }

    //current property set has most probably changed
    propertySetSwitched();

    if (!was_dirty)
        setDirty(false);
    //move to 1st column, 1st row
    d->dataTable->dataAwareObject()->ensureCellVisible(0, 0);
// tempData()->registerTableSchemaChanges(query);
}

bool KexiQueryDesignerGuiEditor::loadLayout()
{
    QString xml;
// if (!loadDataBlock( xml, "query_layout" )) {
    loadDataBlock(xml, "query_layout");
    //TODO errmsg
//  return false;
// }
    if (xml.isEmpty()) {
        //in a case when query layout was not saved, build layout by hand
        // -- dynamic cast because of a need for handling invalid queries
        //    (as in KexiQueryDesignerGuiEditor::afterSwitchFrom()):
        KexiDB::QuerySchema * q = dynamic_cast<KexiDB::QuerySchema *>(window()->schemaData());
        if (q) {
            showTablesForQuery(q);
            KexiDB::ResultInfo result;
            showRelationsForQuery(q, result);
            if (!result.success) {
                window()->setStatus(&result, i18n("Query definition loading failed."));
                return false;
            }
        }
        return true;
    }

    QDomDocument doc;
    doc.setContent(xml);
    QDomElement doc_el = doc.documentElement(), el;
    if (doc_el.tagName() != "query_layout") {
        //TODO errmsg
        return false;
    }

    const bool was_dirty = isDirty();

    //add tables and relations to the relation view
    for (el = doc_el.firstChild().toElement(); !el.isNull(); el = el.nextSibling().toElement()) {
        if (el.tagName() == "table") {
            KexiDB::TableSchema *t = d->conn->tableSchema(el.attribute("name"));
            int x = el.attribute("x", "-1").toInt();
            int y = el.attribute("y", "-1").toInt();
            int width = el.attribute("width", "-1").toInt();
            int height = el.attribute("height", "-1").toInt();
            QRect rect;
            if (x != -1 || y != -1 || width != -1 || height != -1)
                rect = QRect(x, y, width, height);
            d->relations->addTable(t, rect);
        } else if (el.tagName() == "conn") {
            SourceConnection src_conn;
            src_conn.masterTable = el.attribute("mtable");
            src_conn.masterField = el.attribute("mfield");
            src_conn.detailsTable = el.attribute("dtable");
            src_conn.detailsField = el.attribute("dfield");
            d->relations->addConnection(src_conn);
        }
    }

    if (!was_dirty)
        setDirty(false);
    return true;
}

bool KexiQueryDesignerGuiEditor::storeLayout()
{
    KexiQueryPart::TempData * temp = tempData();

    // Save SQL without driver-escaped keywords.
    if (window()->schemaData()) //set this instance as obsolete (only if it's stored)
        d->conn->setQuerySchemaObsolete(window()->schemaData()->name());

    KexiDB::Connection::SelectStatementOptions options;
    options.identifierEscaping = KexiDB::Driver::EscapeKexi | KexiDB::Driver::EscapeAsNecessary;
    options.addVisibleLookupColumns = false;
    QString sqlText = d->conn->selectStatement(*temp->query(), options);
    if (!storeDataBlock(sqlText, "sql")) {
        return false;
    }

    //serialize detailed XML query definition
    QString xml = "<query_layout>", tmp;
    foreach(KexiRelationsTableContainer* cont, *d->relations->tables()) {
        /*! @todo what about query? */
        tmp = QString("<table name=\"") + QString(cont->schema()->name()) + "\" x=\""
              + QString::number(cont->x())
              + "\" y=\"" + QString::number(cont->y())
              + "\" width=\"" + QString::number(cont->width())
              + "\" height=\"" + QString::number(cont->height())
              + "\"/>";
        xml += tmp;
    }


    foreach(KexiRelationsConnection *conn, *d->relations->connections()) {
        tmp = QString("<conn mtable=\"") + QString(conn->masterTable()->schema()->name())
              + "\" mfield=\"" + conn->masterField() + "\" dtable=\""
              + QString(conn->detailsTable()->schema()->name())
              + "\" dfield=\"" + conn->detailsField() + "\"/>";
        xml += tmp;
    }
    xml += "</query_layout>";
    if (!storeDataBlock(xml, "query_layout")) {
        return false;
    }
    return true;
}

QSize KexiQueryDesignerGuiEditor::sizeHint() const
{
    QSize s1 = d->relations->sizeHint();
    QSize s2 = d->head->sizeHint();
    return QSize(qMax(s1.width(), s2.width()), s1.height() + s2.height());
}

KexiDB::RecordData*
KexiQueryDesignerGuiEditor::createNewRow(const QString& tableName, const QString& fieldName,
        bool visible) const
{
    KexiDB::RecordData *newRecord = d->data->createItem();
    QString key;
    if (tableName == "*")
        key = "*";
    else {
        if (!tableName.isEmpty())
            key = (tableName + ".");
        key += fieldName;
    }
    (*newRecord)[COLUMN_ID_COLUMN] = key;
    (*newRecord)[COLUMN_ID_TABLE] = tableName;
    (*newRecord)[COLUMN_ID_VISIBLE] = QVariant(visible);
#ifndef KEXI_NO_QUERY_TOTALS
    (*newRecord)[COLUMN_ID_TOTALS] = QVariant(0);
#endif
    return newRecord;
}

void KexiQueryDesignerGuiEditor::slotDragOverTableRow(
    KexiDB::RecordData * /*record*/, int /*row*/, QDragMoveEvent* e)
{
    if (e->provides("kexi/field")) {
        e->acceptAction(true);
    }
}

void
KexiQueryDesignerGuiEditor::slotDroppedAtRow(KexiDB::RecordData * /*record*/, int /*row*/,
        QDropEvent *ev, KexiDB::RecordData*& newRecord)
{
    QString sourcePartClass;
    QString srcTable;
    QString srcField;

    if (!KexiFieldDrag::decodeSingle(ev, sourcePartClass, srcTable, srcField))
        return;
    //insert new row at specific place
    newRecord = createNewRow(srcTable, srcField, true /* visible*/);
    d->droppedNewRecord = newRecord;
    d->droppedNewTable = srcTable;
    d->droppedNewField = srcField;
    //TODO
}

void KexiQueryDesignerGuiEditor::slotNewItemAppendedForAfterDeletingInSpreadSheetMode()
{
    KexiDB::RecordData *record = d->data->last();
    if (record)
        (*record)[COLUMN_ID_VISIBLE] = QVariant(false); //the same init as in initTableRows()
}

void KexiQueryDesignerGuiEditor::slotRowInserted(KexiDB::RecordData* record, uint row, bool /*repaint*/)
{
    if (d->droppedNewRecord && d->droppedNewRecord == record) {
        createPropertySet(row, d->droppedNewTable, d->droppedNewField, true);
        propertySetSwitched();
        d->droppedNewRecord = 0;
    }
}

void KexiQueryDesignerGuiEditor::slotTableAdded(KexiDB::TableSchema & /*t*/)
{
    if (!d->slotTableAdded_enabled)
        return;
    updateColumnsData();
    setDirty();
    d->dataTable->setFocus();
}

void KexiQueryDesignerGuiEditor::slotTableHidden(KexiDB::TableSchema & /*t*/)
{
    updateColumnsData();
    setDirty();
}

QByteArray KexiQueryDesignerGuiEditor::generateUniqueAlias() const
{
//! @todo add option for using non-i18n'd "expr" prefix?
    const QByteArray expStr(
        i18nc("short for 'expression' word (only latin letters, please)", "expr").toLatin1());
//! @todo optimization: cache it?
    QSet<QByteArray> aliases;
    const int setsSize = d->sets->size();
    for (int r = 0; r < setsSize; r++) {
//! @todo use iterator here
        KoProperty::Set *set = d->sets->at(r);
        if (set) {
            const QByteArray a((*set)["alias"].value().toByteArray().toLower());
            if (!a.isEmpty())
                aliases.insert(a);
        }
    }
    int aliasNr = 1;
    for (;;aliasNr++) {
        if (!aliases.contains(expStr + QByteArray::number(aliasNr)))
            break;
    }
    return expStr + QByteArray::number(aliasNr);
}

//! @todo this is primitive, temporary: reuse SQL parser
KexiDB::BaseExpr*
KexiQueryDesignerGuiEditor::parseExpressionString(const QString& fullString, int& token,
        bool allowRelationalOperator)
{
    QString str = fullString.trimmed();
    int len = 0;
    //KexiDB::BaseExpr *expr = 0;
    //1. get token
    token = 0;
    //2-char-long tokens
    if (str.startsWith(">="))
        token = GREATER_OR_EQUAL;
    else if (str.startsWith("<="))
        token = LESS_OR_EQUAL;
    else if (str.startsWith("<>"))
        token = NOT_EQUAL;
    else if (str.startsWith("!="))
        token = NOT_EQUAL2;
    else if (str.startsWith("=="))
        token = '=';

    if (token != 0)
        len = 2;
    else if (str.startsWith("=") //1-char-long tokens
             || str.startsWith("<")
             || str.startsWith(">")) {
        token = str[0].toLatin1();
        len = 1;
    } else {
        if (allowRelationalOperator)
            token = '=';
    }

    if (!allowRelationalOperator && token != 0)
        return 0;

    //1. get expression after token
    if (len > 0)
        str = str.mid(len).trimmed();
    if (str.isEmpty())
        return 0;

    KexiDB::BaseExpr *valueExpr = 0;
    QRegExp re;
    if (str.length() >= 2 &&
            (
                (str.startsWith("\"") && str.endsWith("\""))
                || (str.startsWith("'") && str.endsWith("'")))
       ) {
        valueExpr = new KexiDB::ConstExpr(CHARACTER_STRING_LITERAL, str.mid(1, str.length() - 2));
    } else if (str.startsWith("[") && str.endsWith("]")) {
        valueExpr = new KexiDB::QueryParameterExpr(str.mid(1, str.length() - 2));
    } else if ((re = QRegExp("(\\d{1,4})-(\\d{1,2})-(\\d{1,2})")).exactMatch(str)) {
        valueExpr = new KexiDB::ConstExpr(DATE_CONST, QDate::fromString(
                                              re.cap(1).rightJustified(4, '0') + "-" + re.cap(2).rightJustified(2, '0')
                                              + "-" + re.cap(3).rightJustified(2, '0'), Qt::ISODate));
    } else if ((re = QRegExp("(\\d{1,2}):(\\d{1,2})")).exactMatch(str)
               || (re = QRegExp("(\\d{1,2}):(\\d{1,2}):(\\d{1,2})")).exactMatch(str)) {
        QString res = re.cap(1).rightJustified(2, '0') + ":" + re.cap(2).rightJustified(2, '0')
                      + ":" + re.cap(3).rightJustified(2, '0');
//  kexipluginsdbg << res;
        valueExpr = new KexiDB::ConstExpr(TIME_CONST, QTime::fromString(res, Qt::ISODate));
    } else if ((re = QRegExp("(\\d{1,4})-(\\d{1,2})-(\\d{1,2})\\s+(\\d{1,2}):(\\d{1,2})")).exactMatch(str)
               || (re = QRegExp("(\\d{1,4})-(\\d{1,2})-(\\d{1,2})\\s+(\\d{1,2}):(\\d{1,2}):(\\d{1,2})")).exactMatch(str)) {
        QString res = re.cap(1).rightJustified(4, '0') + "-" + re.cap(2).rightJustified(2, '0')
                      + "-" + re.cap(3).rightJustified(2, '0')
                      + "T" + re.cap(4).rightJustified(2, '0') + ":" + re.cap(5).rightJustified(2, '0')
                      + ":" + re.cap(6).rightJustified(2, '0');
//  kexipluginsdbg << res;
        valueExpr = new KexiDB::ConstExpr(DATETIME_CONST,
                                          QDateTime::fromString(res, Qt::ISODate));
    } else if (str[0] >= '0' && str[0] <= '9' || str[0] == '-' || str[0] == '+') {
        //number
        QString decimalSym = KGlobal::locale()->decimalSymbol();
        bool ok;
        int pos = str.indexOf('.');
        if (pos == -1) {//second chance: local decimal symbol
            pos = str.indexOf(decimalSym);
        }
        if (pos >= 0) {//real const number
            const int left = str.left(pos).toInt(&ok);
            if (!ok)
                return 0;
            const int right = str.mid(pos + 1).toInt(&ok);
            if (!ok)
                return 0;
            valueExpr = new KexiDB::ConstExpr(REAL_CONST, QPoint(left, right)); //decoded to QPoint
        } else {
            //integer const
            const qint64 val = str.toLongLong(&ok);
            if (!ok)
                return 0;
            valueExpr = new KexiDB::ConstExpr(INTEGER_CONST, val);
        }
    } else if (str.toLower() == "null") {
        valueExpr = new KexiDB::ConstExpr(SQL_NULL, QVariant());
    } else {//identfier
        if (!KexiUtils::isIdentifier(str))
            return 0;
        valueExpr = new KexiDB::VariableExpr(str);
        //find first matching field for name 'str':
        foreach(KexiRelationsTableContainer *cont, *d->relations->tables()) {
            /*! @todo what about query? */
            if (cont->schema()->table() && cont->schema()->table()->field(str)) {
                valueExpr->toVariable()->field = cont->schema()->table()->field(str);
                break;
            }
        }
    }
    return valueExpr;
}

void KexiQueryDesignerGuiEditor::slotBeforeCellChanged(KexiDB::RecordData *record, int colnum,
        QVariant& newValue, KexiDB::ResultInfo* result)
{
    if (colnum == COLUMN_ID_COLUMN) {
        if (newValue.isNull()) {
            d->data->updateRowEditBuffer(record, COLUMN_ID_TABLE, QVariant(), false/*!allowSignals*/);
            d->data->updateRowEditBuffer(record, COLUMN_ID_VISIBLE, QVariant(false));//invisible
            d->data->updateRowEditBuffer(record, COLUMN_ID_SORTING, QVariant());
#ifndef KEXI_NO_QUERY_TOTALS
            d->data->updateRowEditBuffer(record, COLUMN_ID_TOTALS, QVariant());//remove totals
#endif
            d->data->updateRowEditBuffer(record, COLUMN_ID_CRITERIA, QVariant());//remove crit.
            d->sets->eraseCurrentPropertySet();
        } else {
            //auto fill 'table' column
            QString fieldId(newValue.toString().trimmed());   //tmp, can look like "table.field"
            QString fieldName; //"field" part of "table.field" or expression string
            QString tableName; //empty for expressions
            QByteArray alias;
            QString columnValueForExpr; //for setting pretty printed "alias: expr" in 1st column
            const bool isExpression = !d->fieldColumnIdentifiers[fieldId];
            if (isExpression) {
                //this value is entered by hand and doesn't match
                //any value in the combo box -- we're assuming this is an expression
                //-table remains null
                //-find "alias" in something like "alias : expr"
                const int id = fieldId.indexOf(':');
                if (id > 0) {
                    alias = fieldId.left(id).trimmed().toLatin1();
                    if (!KexiUtils::isIdentifier(alias)) {
                        result->success = false;
                        result->allowToDiscardChanges = true;
                        result->column = colnum;
                        result->msg = i18n(
                                          "Entered column alias \"%1\" is not a valid identifier.", QString(alias));
                        result->desc = i18n("Identifiers should start with a letter or '_' character");
                        return;
                    }
                }
                fieldName = fieldId.mid(id + 1).trimmed();
                //check expr.
                KexiDB::BaseExpr *e;
                int dummyToken;
                if ((e = parseExpressionString(fieldName, dummyToken, false/*allowRelationalOperator*/))) {
                    fieldName = e->toString(); //print it prettier
                    //this is just checking: destroy expr. object
                    delete e;
                } else {
                    result->success = false;
                    result->allowToDiscardChanges = true;
                    result->column = colnum;
                    result->msg = i18n("Invalid expression \"%1\"", fieldName);
                    return;
                }
            } else {//not expr.
                //this value is properly selected from combo box list
                if (fieldId == "*") {
                    tableName = "*";
                } else {
                    if (!KexiDB::splitToTableAndFieldParts(
                                fieldId, tableName, fieldName, KexiDB::SetFieldNameIfNoTableName)) {
                        kexipluginswarn << "KexiQueryDesignerGuiEditor::slotBeforeCellChanged(): no 'field' or 'table.field'";
                        return;
                    }
                }
            }
            bool saveOldValue = true;
            KoProperty::Set *set = d->sets->findPropertySetForItem(*record); //*propertyBuffer();
            if (!set) {
                saveOldValue = false; // no old val.
                const int row = d->data->indexOf(record);
                if (row < 0) {
                    result->success = false;
                    return;
                }
                set = createPropertySet(row, tableName, fieldName, true);
                propertySetSwitched();
            }
            d->data->updateRowEditBuffer(record, COLUMN_ID_TABLE, QVariant(tableName), false/*!allowSignals*/);
            d->data->updateRowEditBuffer(record, COLUMN_ID_VISIBLE, QVariant(true));
#ifndef KEXI_NO_QUERY_TOTALS
            d->data->updateRowEditBuffer(record, COLUMN_ID_TOTALS, QVariant(0));
#endif
            if (!sortingAllowed(fieldName, tableName)) {
                // sorting is not available for "*" or "table.*" rows
//! @todo what about expressions?
                d->data->updateRowEditBuffer(record, COLUMN_ID_SORTING, QVariant());
            }
            //update properties
            (*set)["field"].setValue(fieldName, saveOldValue);
            if (isExpression) {
                //-no alias but it's needed:
                if (alias.isEmpty()) //-try oto get old alias
                    alias = (*set)["alias"].value().toByteArray();
                if (alias.isEmpty()) //-generate smallest unique alias
                    alias = generateUniqueAlias();
            }
            (*set)["isExpression"].setValue(QVariant(isExpression), saveOldValue);
            if (!alias.isEmpty()) {
                (*set)["alias"].setValue(alias, saveOldValue);
                //pretty printed "alias: expr"
                newValue = QString(alias) + ": " + fieldName;
            }
            (*set)["caption"].setValue(QString(), saveOldValue);
            (*set)["table"].setValue(tableName, saveOldValue);
            updatePropertiesVisibility(*set);
        }
    } else if (colnum == COLUMN_ID_TABLE) {
        if (newValue.isNull()) {
            if (!(*record)[COLUMN_ID_COLUMN].toString().isEmpty())
                d->data->updateRowEditBuffer(record, COLUMN_ID_COLUMN, QVariant(), false/*!allowSignals*/);
            d->data->updateRowEditBuffer(record, COLUMN_ID_VISIBLE, QVariant(false));//invisible
#ifndef KEXI_NO_QUERY_TOTALS
            d->data->updateRowEditBuffer(record, COLUMN_ID_TOTALS, QVariant());//remove totals
#endif
            d->data->updateRowEditBuffer(record, COLUMN_ID_CRITERIA, QVariant());//remove crit.
            d->sets->eraseCurrentPropertySet();
        }
        //update property
        KoProperty::Set *set = d->sets->findPropertySetForItem(*record);
        if (set) {
            if ((*set)["isExpression"].value().toBool() == false) {
                (*set)["table"] = newValue;
                (*set)["caption"] = QString();
            } else {
                //do not set table for expr. columns
                newValue = QVariant();
            }
//   KoProperty::Set &set = *propertyBuffer();
            updatePropertiesVisibility(*set);
        }
    } else if (colnum == COLUMN_ID_VISIBLE) {
        bool saveOldValue = true;
        if (!propertySet()) {
            saveOldValue = false;
            createPropertySet(d->dataTable->dataAwareObject()->currentRow(),
                              (*record)[COLUMN_ID_TABLE].toString(), (*record)[COLUMN_ID_COLUMN].toString(), true);
#ifndef KEXI_NO_QUERY_TOTALS
            d->data->updateRowEditBuffer(record, COLUMN_ID_TOTALS, QVariant(0));//totals
#endif
            propertySetSwitched();
        }
        KoProperty::Set &set = *propertySet();
        set["visible"].setValue(newValue, saveOldValue);
    }
#ifndef KEXI_NO_QUERY_TOTALS
    else if (colnum == COLUMN_ID_TOTALS) {
        //TODO:
        //unused yet
        setDirty(true);
    }
#endif
    else if (colnum == COLUMN_ID_SORTING) {
        KoProperty::Set *set = d->sets->findPropertySetForItem(*record);
        QString table(set->property("table").value().toString());
        QString field(set->property("field").value().toString());
        if (newValue.toInt() == 0 || sortingAllowed(field, table)) {
            KoProperty::Property &property = set->property("sorting");
            QString key(property.listData()->keysAsStringList()[ newValue.toInt()]);
            kexipluginsdbg << "new key=" << key;
            property.setValue(key, true);
        } else { //show msg: sorting is not available
            result->success = false;
            result->allowToDiscardChanges = true;
            result->column = colnum;
            result->msg = i18n("Could not set sorting for multiple columns (%1)",
                               table == "*" ? table : (table + ".*"));
        }
    } else if (colnum == COLUMN_ID_CRITERIA) {
//! @todo this is primitive, temporary: reuse SQL parser
        QString operatorStr, argStr;
        KexiDB::BaseExpr* e = 0;
        const QString str = newValue.toString().trimmed();
        int token;
        QString field, table;
        KoProperty::Set *set = d->sets->findPropertySetForItem(*record);
        if (set) {
            field = (*set)["field"].value().toString();
            table = (*set)["table"].value().toString();
        }
        if (!str.isEmpty() && (!set || table == "*" || field.contains("*"))) {
            //asterisk found! criteria not allowed
            result->success = false;
            result->allowToDiscardChanges = true;
            result->column = colnum;
            if (propertySet())
                result->msg = i18n("Could not set criteria for \"%1\"",
                                   table == "*" ? table : field);
            else
                result->msg = i18n("Could not set criteria for empty row");
            //moved to result->allowToDiscardChanges handler //d->dataTable->dataAwareObject()->cancelEditor(); //prevents further editing of this cell
        } else if (str.isEmpty() || (e = parseExpressionString(str, token, true/*allowRelationalOperator*/))) {
            if (e) {
                QString tokenStr;
                if (token != '=') {
                    KexiDB::BinaryExpr be(KexiDBExpr_Relational, 0, token, 0);
                    tokenStr = be.tokenToString() + " ";
                }
                (*set)["criteria"] = tokenStr + e->toString(); //print it prettier
                //this is just checking: destroy expr. object
                delete e;
            } else if (str.isEmpty()) {
                (*set)["criteria"] = QVariant(); //clear it
            }
            setDirty(true);
        } else {
            result->success = false;
            result->allowToDiscardChanges = true;
            result->column = colnum;
            result->msg = i18n("Invalid criteria \"%1\"", newValue.toString());
        }
    }
}

void KexiQueryDesignerGuiEditor::slotTablePositionChanged(KexiRelationsTableContainer*)
{
    setDirty(true);
}

void KexiQueryDesignerGuiEditor::slotAboutConnectionRemove(KexiRelationsConnection*)
{
    setDirty(true);
}

void KexiQueryDesignerGuiEditor::slotAppendFields(
    KexiDB::TableOrQuerySchema& tableOrQuery, const QStringList& fieldNames)
{
//! @todo how about query columns and multiple fields?
    KexiDB::TableSchema *table = tableOrQuery.table();
    if (!table || fieldNames.isEmpty())
        return;
    QString fieldName(fieldNames.first());
    if (fieldName != "*" && !table->field(fieldName))
        return;
    int row_num;
    //find last filled row in the GUI table
    for (row_num = d->sets->size() - 1; row_num >= 0 && !d->sets->at(row_num); row_num--)
        ;
    row_num++; //after
    //add row
    KexiDB::RecordData *newRecord = createNewRow(table->name(), fieldName, true /* visible*/);
    d->dataTable->dataAwareObject()->insertItem(newRecord, row_num);
    d->dataTable->dataAwareObject()->setCursorPosition(row_num, 0);
    //create buffer
    createPropertySet(row_num, table->name(), fieldName, true/*new one*/);
    propertySetSwitched();
    d->dataTable->setFocus();
}

KoProperty::Set *KexiQueryDesignerGuiEditor::propertySet()
{
    return d->sets->currentPropertySet();
}

void KexiQueryDesignerGuiEditor::updatePropertiesVisibility(KoProperty::Set& set)
{
    const bool asterisk = isAsterisk(
                              set["table"].value().toString(), set["field"].value().toString()
                          );
#ifndef KEXI_NO_UNFINISHED
    set["caption"].setVisible(!asterisk);
#endif
    set["alias"].setVisible(!asterisk);
    /*always invisible #ifndef KEXI_NO_UNFINISHED
      set["sorting"].setVisible( !asterisk );
    #endif*/
    propertySetReloaded(true);
}

KoProperty::Set*
KexiQueryDesignerGuiEditor::createPropertySet(int row,
        const QString& tableName, const QString& fieldName, bool newOne)
{
    //const bool asterisk = isAsterisk(tableName, fieldName);
    QString typeName = "KexiQueryDesignerGuiEditor::Column";
    KoProperty::Set *set = new KoProperty::Set(d->sets, typeName);
    KoProperty::Property *prop;

    //meta-info for property editor
    set->addProperty(prop = new KoProperty::Property("this:classString", i18n("Query column")));
    prop->setVisible(false);
//! \todo add table_field icon (add buff->addProperty(prop = new KexiProperty("this:iconName", "table_field") );
// prop->setVisible(false);

    set->addProperty(prop = new KoProperty::Property("table", QVariant(tableName)));
    prop->setVisible(false);//always hidden

    set->addProperty(prop = new KoProperty::Property("field", QVariant(fieldName)));
    prop->setVisible(false);//always hidden

    set->addProperty(prop = new KoProperty::Property("caption", QVariant(QString()), i18n("Caption")));
#ifdef KEXI_NO_UNFINISHED
    prop->setVisible(false);
#endif

    set->addProperty(prop = new KoProperty::Property("alias", QVariant(QString()), i18n("Alias")));

    set->addProperty(prop = new KoProperty::Property("visible", QVariant(true)));
    prop->setVisible(false);

    /*TODO:
      set->addProperty(prop = new KexiProperty("totals", QVariant(QString())) );
      prop->setVisible(false);*/

    //sorting
    QStringList slist, nlist;
    slist << "nosorting" << "ascending" << "descending";
    nlist << i18n("None") << i18n("Ascending") << i18n("Descending");
    set->addProperty(prop = new KoProperty::Property("sorting",
            slist, nlist, slist[0], i18n("Sorting")));
    prop->setVisible(false);

    set->addProperty(prop = new KoProperty::Property("criteria", QVariant(QString())));
    prop->setVisible(false);

    set->addProperty(prop = new KoProperty::Property("isExpression", QVariant(false)));
    prop->setVisible(false);

    connect(set, SIGNAL(propertyChanged(KoProperty::Set&, KoProperty::Property&)),
            this, SLOT(slotPropertyChanged(KoProperty::Set&, KoProperty::Property&)));

    d->sets->set(row, set, newOne);

    updatePropertiesVisibility(*set);
    return set;
}

void KexiQueryDesignerGuiEditor::setFocus()
{
    d->dataTable->setFocus();
}

void KexiQueryDesignerGuiEditor::slotPropertyChanged(KoProperty::Set& set, KoProperty::Property& property)
{
    const QByteArray pname(property.name());
    /*
     * TODO (js) use KexiProperty::setValidator(QString) when implemented as described in TODO #60
     */
    if (pname == "alias" || pname == "name") {
        const QVariant& v = property.value();
        if (!v.toString().trimmed().isEmpty() && !KexiUtils::isIdentifier(v.toString())) {
            KMessageBox::sorry(this,
                               KexiUtils::identifierExpectedMessage(property.caption(), v.toString()));
            property.resetValue();
        }
        if (pname == "alias") {
            if (set["isExpression"].value().toBool() == true) {
                //update value in column #1
                d->dataTable->dataAwareObject()->acceptEditor();
//    d->dataTable->dataAwareObject()->setCursorPosition(d->dataTable->dataAwareObject()->currentRow(),0);
                //d->dataTable->dataAwareObject()->startEditCurrentCell();
                d->data->updateRowEditBuffer(d->dataTable->dataAwareObject()->selectedItem(),
                                             0, QVariant(set["alias"].value().toString() + ": " + set["field"].value().toString()));
                d->data->saveRowChanges(*d->dataTable->dataAwareObject()->selectedItem(), true);
//    d->dataTable->dataAwareObject()->acceptRowEdit();
            }
        }
    }
}

void KexiQueryDesignerGuiEditor::slotNewItemStored(KexiPart::Item& item)
{
    d->relations->objectCreated(item.partClass(), item.name());
}

void KexiQueryDesignerGuiEditor::slotItemRemoved(const KexiPart::Item& item)
{
    d->relations->objectDeleted(item.partClass(), item.name());
}

void KexiQueryDesignerGuiEditor::slotItemRenamed(const KexiPart::Item& item, const QString& oldName)
{
    d->relations->objectRenamed(item.partClass(), oldName, item.name());
}

#include "kexiquerydesignerguieditor.moc"
