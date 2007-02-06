/* This file is part of the KDE project
   Copyright (C) 2006 Jaroslaw Staniek <js@iidea.pl>

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

#include "kexilookupcolumnpage.h"

#include <qlabel.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qheader.h>

#include <kiconloader.h>
#include <klocale.h>
#include <ktoolbarbutton.h>
#include <kdebug.h>
#include <kpopupmenu.h>

#include <widget/kexipropertyeditorview.h>
#include <widget/kexidatasourcecombobox.h>
#include <widget/kexifieldlistview.h>
#include <widget/kexifieldcombobox.h>
#include <widget/kexismalltoolbutton.h>
#include <kexidb/connection.h>
#include <kexiproject.h>

#include <koproperty/property.h>
#include <koproperty/utils.h>

QString mimeTypeToType(const QString& mimeType)
{
	if (mimeType=="kexi/table")
		return "table";
	else if (mimeType=="kexi/query")
		return "query";
//! @todo more types
	return mimeType;
}

QString typeToMimeType(const QString& type)
{
	if (type=="table")
		return "kexi/table";
	else if (type=="query")
		return "kexi/query";
//! @todo more types
	return type;
}

//----------------------------------------------

//! @internal
class KexiLookupColumnPage::Private
{
	public:
		Private()
		 : currentFieldUid(-1)
		 , insideClearRowSourceSelection(false)
		 , propertySetEnabled(true)
		{
		}
		~Private()
		{
		}

		bool hasPropertySet() const {
			return propertySet;
		}

		void setPropertySet(KoProperty::Set* aPropertySet) {
			propertySet = aPropertySet;
		}

		QVariant propertyValue(const QCString& propertyName) const {
			return propertySet ? propertySet->property(propertyName).value() : QVariant();
		}

		void changeProperty(const QCString &property, const QVariant &value)
		{
			if (!propertySetEnabled)
				return;
			propertySet->changeProperty(property, value);
		}

		void updateInfoLabelForPropertySet(const QString& textToDisplayForNullSet) {
			KexiPropertyEditorView::updateInfoLabelForPropertySet(
				objectInfoLabel, propertySet, textToDisplayForNullSet);
		}

		KexiDataSourceComboBox *rowSourceCombo;
		KexiFieldComboBox *boundColumnCombo, *visibleColumnCombo;
		KexiObjectInfoLabel *objectInfoLabel;
		QLabel *rowSourceLabel, *boundColumnLabel, *visibleColumnLabel;
		QToolButton *clearRowSourceButton, *gotoRowSourceButton, *clearBoundColumnButton,
			*clearVisibleColumnButton;
		//! Used only in assignPropertySet() to check whether we already have the set assigned
		int currentFieldUid;

		bool insideClearRowSourceSelection : 1;
		//! True is changeProperty() works. Used to block updating properties when within assignPropertySet().
		bool propertySetEnabled : 1;

	private:
		//! A property set that is displayed on the page. 
		//! The set is also updated after any change in this page's data.
		QGuardedPtr<KoProperty::Set> propertySet;
};

//----------------------------------------------

KexiLookupColumnPage::KexiLookupColumnPage(QWidget *parent)
 : QWidget(parent)
 , d(new Private())
{
	setName("KexiLookupColumnPage");

	QVBoxLayout *vlyr = new QVBoxLayout(this);
	d->objectInfoLabel = new KexiObjectInfoLabel(this, "KexiObjectInfoLabel");
	vlyr->addWidget(d->objectInfoLabel);

//todo	d->noDataSourceAvailableSingleText = i18n("No data source could be assigned for this widget.");
//todo	d->noDataSourceAvailableMultiText = i18n("No data source could be assigned for multiple widgets.");

	//-Row Source
	QWidget *contents = new QWidget(this);
	vlyr->addWidget(contents);
	QVBoxLayout *contentsVlyr = new QVBoxLayout(contents);

	QHBoxLayout *hlyr = new QHBoxLayout(contentsVlyr);
	d->rowSourceLabel = new QLabel(i18n("Row source:"), contents);
	d->rowSourceLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	d->rowSourceLabel->setMargin(2);
	d->rowSourceLabel->setMinimumHeight(IconSize(KIcon::Small)+4);
	d->rowSourceLabel->setAlignment(Qt::AlignLeft|Qt::AlignBottom);
	hlyr->addWidget(d->rowSourceLabel);

	d->gotoRowSourceButton = new KexiSmallToolButton(contents, QString::null, "goto", "gotoRowSourceButton");
	d->gotoRowSourceButton->setMinimumHeight(d->rowSourceLabel->minimumHeight());
	QToolTip::add(d->gotoRowSourceButton, i18n("Go to selected row source"));
	hlyr->addWidget(d->gotoRowSourceButton);
	connect(d->gotoRowSourceButton, SIGNAL(clicked()), this, SLOT(slotGotoSelectedRowSource()));

	d->clearRowSourceButton = new KexiSmallToolButton(contents, QString::null,
		"clear_left", "clearRowSourceButton");
	d->clearRowSourceButton->setMinimumHeight(d->rowSourceLabel->minimumHeight());
	QToolTip::add(d->clearRowSourceButton, i18n("Clear row source"));
	hlyr->addWidget(d->clearRowSourceButton);
	connect(d->clearRowSourceButton, SIGNAL(clicked()), this, SLOT(clearRowSourceSelection()));

	d->rowSourceCombo = new KexiDataSourceComboBox(contents, "rowSourceCombo");
	d->rowSourceLabel->setBuddy(d->rowSourceCombo);
	contentsVlyr->addWidget(d->rowSourceCombo);

	contentsVlyr->addSpacing(8);

	//- Bound Column
	hlyr = new QHBoxLayout(contentsVlyr);
	d->boundColumnLabel = new QLabel(i18n("Bound column:"), contents);
	d->boundColumnLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	d->boundColumnLabel->setMargin(2);
	d->boundColumnLabel->setMinimumHeight(IconSize(KIcon::Small)+4);
	d->boundColumnLabel->setAlignment(Qt::AlignLeft|Qt::AlignBottom);
	hlyr->addWidget(d->boundColumnLabel);

	d->clearBoundColumnButton = new KexiSmallToolButton(contents, QString::null,
		"clear_left", "clearBoundColumnButton");
	d->clearBoundColumnButton->setMinimumHeight(d->boundColumnLabel->minimumHeight());
	QToolTip::add(d->clearBoundColumnButton, i18n("Clear bound column"));
	hlyr->addWidget(d->clearBoundColumnButton);
	connect(d->clearBoundColumnButton, SIGNAL(clicked()), this, SLOT(clearBoundColumnSelection()));

	d->boundColumnCombo = new KexiFieldComboBox(contents, "boundColumnCombo");
	d->boundColumnLabel->setBuddy(d->boundColumnCombo);
	contentsVlyr->addWidget(d->boundColumnCombo);

	contentsVlyr->addSpacing(8);

	//- Visible Column
	hlyr = new QHBoxLayout(contentsVlyr);
	d->visibleColumnLabel = new QLabel(i18n("Visible column:"), contents);
	d->visibleColumnLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	d->visibleColumnLabel->setMargin(2);
	d->visibleColumnLabel->setMinimumHeight(IconSize(KIcon::Small)+4);
	d->visibleColumnLabel->setAlignment(Qt::AlignLeft|Qt::AlignBottom);
	hlyr->addWidget(d->visibleColumnLabel);

	d->clearVisibleColumnButton = new KexiSmallToolButton(contents, QString::null,
		"clear_left", "clearVisibleColumnButton");
	d->clearVisibleColumnButton->setMinimumHeight(d->visibleColumnLabel->minimumHeight());
	QToolTip::add(d->clearVisibleColumnButton, i18n("Clear visible column"));
	hlyr->addWidget(d->clearVisibleColumnButton);
	connect(d->clearVisibleColumnButton, SIGNAL(clicked()), this, SLOT(clearVisibleColumnSelection()));

	d->visibleColumnCombo = new KexiFieldComboBox(contents, "visibleColumnCombo");
	d->visibleColumnLabel->setBuddy(d->visibleColumnCombo);
	contentsVlyr->addWidget(d->visibleColumnCombo);

	vlyr->addStretch(1);

	connect(d->rowSourceCombo, SIGNAL(textChanged(const QString &)), 
		this, SLOT(slotRowSourceTextChanged(const QString &)));
	connect(d->rowSourceCombo, SIGNAL(dataSourceChanged()), this, SLOT(slotRowSourceChanged()));
	connect(d->boundColumnCombo, SIGNAL(selected()), this, SLOT(slotBoundColumnSelected()));
	connect(d->visibleColumnCombo, SIGNAL(selected()), this, SLOT(slotVisibleColumnSelected()));

	clearBoundColumnSelection();
	clearVisibleColumnSelection();
}

KexiLookupColumnPage::~KexiLookupColumnPage()
{
	delete d;
}

void KexiLookupColumnPage::setProject(KexiProject *prj)
{
	d->rowSourceCombo->setProject(prj,
		true/*showTables*/, true/*showQueries*/
	);
	d->boundColumnCombo->setProject(prj);
	d->visibleColumnCombo->setProject(prj);
}

void KexiLookupColumnPage::assignPropertySet(KoProperty::Set* propertySet)
{
	if (!d->hasPropertySet() && !propertySet)
		return;
	if (propertySet && d->currentFieldUid == (*propertySet)["uid"].value().toInt())
		return; //already assigned

	d->propertySetEnabled = false;
	d->setPropertySet( propertySet );
	d->updateInfoLabelForPropertySet( i18n("No field selected") );
	
	const bool hasRowSource = d->hasPropertySet() && !d->propertyValue("rowSourceType").isNull()
		&& !d->propertyValue("rowSource").isNull();

	QString rowSource, rowSourceType;
	if (hasRowSource) {
		rowSourceType = typeToMimeType( d->propertyValue("rowSourceType").toString() );
		rowSource = d->propertyValue("rowSource").toString();
	}
	d->rowSourceCombo->setDataSource( rowSourceType, rowSource );
	d->rowSourceLabel->setEnabled( d->hasPropertySet() );
	d->rowSourceCombo->setEnabled( d->hasPropertySet() );
	if (!d->hasPropertySet())
		d->clearRowSourceButton->setEnabled( false );

	int boundColumn = -1, visibleColumn = -1;
	if (d->rowSourceCombo->isSelectionValid()) {
		boundColumn = d->propertyValue("boundColumn").toInt();
		visibleColumn = d->propertyValue("visibleColumn").toInt();
	}
	d->boundColumnCombo->setFieldOrExpression(boundColumn);
	d->visibleColumnCombo->setFieldOrExpression(visibleColumn);
	updateBoundColumnWidgetsAvailability();
	d->propertySetEnabled = true;
}

void KexiLookupColumnPage::clearBoundColumnSelection()
{
	d->boundColumnCombo->setCurrentText("");
	d->boundColumnCombo->setFieldOrExpression(QString::null);
	slotBoundColumnSelected();
	d->clearBoundColumnButton->setEnabled(false);
}

void KexiLookupColumnPage::slotBoundColumnSelected()
{
//	KexiDB::Field::Type dataType = KexiDB::Field::InvalidType;
//! @todo this should also work for expressions
/*disabled	KexiDB::Field *field = d->fieldListView->schema()->field( d->boundColumnCombo->fieldOrExpression() );
	if (field)
		dataType = field->type();
*/
	d->clearBoundColumnButton->setEnabled( !d->boundColumnCombo->fieldOrExpression().isEmpty() );
	if (!d->boundColumnCombo->fieldOrExpression().isEmpty()) {
		kdDebug() << endl;
	}

	// update property set
	if (d->hasPropertySet()) {
		d->changeProperty("boundColumn", d->boundColumnCombo->indexOfField());
	}
/*
	emit boundColumnChanged(
		d->boundColumnCombo->fieldOrExpression(),
		d->boundColumnCombo->fieldOrExpressionCaption(),
		dataType
	);*/
}

void KexiLookupColumnPage::clearVisibleColumnSelection()
{
	d->visibleColumnCombo->setCurrentText("");
	d->visibleColumnCombo->setFieldOrExpression(QString::null);
	slotVisibleColumnSelected();
	d->clearVisibleColumnButton->setEnabled(false);
}

void KexiLookupColumnPage::slotVisibleColumnSelected()
{
//	KexiDB::Field::Type dataType = KexiDB::Field::InvalidType;
//! @todo this should also work for expressions
	d->clearVisibleColumnButton->setEnabled( !d->visibleColumnCombo->fieldOrExpression().isEmpty() );

	// update property set
	if (d->hasPropertySet()) {
//! @todo support expression in special "visibleExpression"
		d->changeProperty("visibleColumn", d->visibleColumnCombo->indexOfField());
	}
}

void KexiLookupColumnPage::slotRowSourceChanged()
{
	if (!d->rowSourceCombo->project())
		return;
	QString mime = d->rowSourceCombo->selectedMimeType();
	bool rowSourceFound = false;
	QString name = d->rowSourceCombo->selectedName();
	if ((mime=="kexi/table" || mime=="kexi/query") && d->rowSourceCombo->isSelectionValid()) {
		KexiDB::TableOrQuerySchema *tableOrQuery = new KexiDB::TableOrQuerySchema(
			d->rowSourceCombo->project()->dbConnection(), name.latin1(), mime=="kexi/table");
		if (tableOrQuery->table() || tableOrQuery->query()) {
//disabled			d->fieldListView->setSchema( tableOrQuery );
/*tmp*/			delete tableOrQuery;
			rowSourceFound = true;
			d->boundColumnCombo->setTableOrQuery(name, mime=="kexi/table");
			d->visibleColumnCombo->setTableOrQuery(name, mime=="kexi/table");
		}
		else {
			delete tableOrQuery;
		}
	}
	if (!rowSourceFound) {
		d->boundColumnCombo->setTableOrQuery("", true);
		d->visibleColumnCombo->setTableOrQuery("", true);
	}
	clearBoundColumnSelection();
	clearVisibleColumnSelection();
	d->clearRowSourceButton->setEnabled(rowSourceFound);
	d->gotoRowSourceButton->setEnabled(rowSourceFound);
/* disabled
	if (dataSourceFound) {
		slotFieldListViewSelectionChanged();
	} else {
		d->addField->setEnabled(false);
	}*/
	updateBoundColumnWidgetsAvailability();

	//update property set
	if (d->hasPropertySet()) {
		d->changeProperty("rowSourceType", mimeTypeToType(mime));
		d->changeProperty("rowSource", name);
	}

//disabled	emit formDataSourceChanged(mime, name);
//! @todo update d->propertySet ^^
}

void KexiLookupColumnPage::slotRowSourceTextChanged(const QString & string)
{
	Q_UNUSED(string);
	const bool enable = d->rowSourceCombo->isSelectionValid();
	if (enable) {
		updateBoundColumnWidgetsAvailability();
	}
	else {
		clearRowSourceSelection( d->rowSourceCombo->selectedName().isEmpty()/*alsoClearComboBox*/ );
	}
}

void KexiLookupColumnPage::clearRowSourceSelection(bool alsoClearComboBox)
{
	if (d->insideClearRowSourceSelection)
		return;
	d->insideClearRowSourceSelection = true;
	if (alsoClearComboBox && !d->rowSourceCombo->selectedName().isEmpty())
		d->rowSourceCombo->setDataSource("", "");
	d->clearRowSourceButton->setEnabled(false);
	d->gotoRowSourceButton->setEnabled(false);
	d->insideClearRowSourceSelection = false;
}

void KexiLookupColumnPage::slotGotoSelectedRowSource()
{
	QString mime = d->rowSourceCombo->selectedMimeType();
	if (mime=="kexi/table" || mime=="kexi/query") {
		if (d->rowSourceCombo->isSelectionValid())
			emit jumpToObjectRequested(mime.latin1(), d->rowSourceCombo->selectedName().latin1());
	}
}

void KexiLookupColumnPage::updateBoundColumnWidgetsAvailability()
{
	const bool hasRowSource = d->rowSourceCombo->isSelectionValid();
	d->boundColumnCombo->setEnabled( hasRowSource );
	d->boundColumnLabel->setEnabled( hasRowSource );
	d->clearBoundColumnButton->setEnabled( hasRowSource && !d->boundColumnCombo->fieldOrExpression().isEmpty() );
	d->visibleColumnCombo->setEnabled( hasRowSource );
	d->visibleColumnLabel->setEnabled( hasRowSource );
	d->clearVisibleColumnButton->setEnabled( hasRowSource && !d->visibleColumnCombo->fieldOrExpression().isEmpty() );
}

#include "kexilookupcolumnpage.moc"
