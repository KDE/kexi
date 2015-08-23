/* This file is part of the KDE project
   Copyright (C) 2005-2009 Jarosław Staniek <staniek@kde.org>

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

#include "kexidatasourcepage.h"
#include <KexiIcon.h>
#include <widget/properties/KexiPropertyEditorView.h>
#include <widget/KexiObjectInfoLabel.h>
#include <widget/KexiDataSourceComboBox.h>
#include <widget/fields/KexiFieldListView.h>
#include <widget/fields/KexiFieldComboBox.h>
#include <kexiutils/SmallToolButton.h>
#include <kexiproject.h>
#include <formeditor/commands.h>

#include <KDbConnection>

#include <KProperty>

#include <kfadewidgeteffect.h>
#include <KLocalizedString>

#include <QLabel>
#include <QHBoxLayout>

KexiDataSourcePage::KexiDataSourcePage(QWidget *parent)
        : KexiPropertyPaneViewBase(parent)
        , m_noDataSourceAvailableSingleText(
            xi18n("No data source could be assigned for this widget.") )
        , m_noDataSourceAvailableMultiText(
            xi18n("No data source could be assigned for multiple widgets.") )
        , m_insideClearFormDataSourceSelection(false)
#ifdef KEXI_NO_AUTOFIELD_WIDGET
        , m_tableOrQuerySchema(0)
#endif
{
    infoLabel()->setContentsMargins(0, 0, 0, spacing());

    m_noDataSourceAvailableLabel = new QLabel(m_noDataSourceAvailableSingleText, this);
    m_noDataSourceAvailableLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    m_noDataSourceAvailableLabel->setContentsMargins(0, 0, 0, spacing());
    m_noDataSourceAvailableLabel->setAlignment(Qt::AlignBottom | Qt::AlignLeft);
    m_noDataSourceAvailableLabel->setWordWrap(true);
    mainLayout()->addWidget(m_noDataSourceAvailableLabel);

    //-Widget's Data Source
    QHBoxLayout *hlyr = new QHBoxLayout();
    mainLayout()->addLayout(hlyr);
#if 0
//! @todo unhide this when expression work
// m_widgetDSLabel = new QLabel(futureI18nc("Table Field, Query Field or Expression", "Source field or expression"), this);
#else
    m_widgetDSLabel = new QLabel(
        xi18nc("Table Field or Query Field", "Widget's data source"), this);
#endif
    m_widgetDSLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    m_widgetDSLabel->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
    hlyr->addWidget(m_widgetDSLabel);

#if 0
    m_clearWidgetDSButton = new KexiSmallToolButton(
        koIcon("edit-clear-locationbar-rtl"), QString(), this);
    m_clearWidgetDSButton->setObjectName("clearWidgetDSButton");
    m_clearWidgetDSButton->setMinimumHeight(m_widgetDSLabel->minimumHeight());
    m_clearWidgetDSButton->setToolTip(futureI18n("Clear widget's data source"));
    hlyr->addWidget(m_clearWidgetDSButton);
    connect(m_clearWidgetDSButton, SIGNAL(clicked()),
            this, SLOT(clearWidgetDataSourceSelection()));
#endif

    m_widgetDataSourceCombo = new KexiFieldComboBox(this);
    m_widgetDataSourceCombo->setObjectName("sourceFieldCombo");
    m_widgetDataSourceCombo->setContentsMargins(0, 0, 0, 0);
    m_widgetDSLabel->setBuddy(m_widgetDataSourceCombo);
    connect(m_widgetDataSourceCombo->lineEdit(), SIGNAL(textChanged(QString)),
        this, SLOT(slotWidgetDataSourceTextChanged(QString)));
    mainLayout()->addWidget(m_widgetDataSourceCombo);

    m_widgetDataSourceComboSpacer = addWidgetSpacer();

    //- Form's Data Source
    hlyr = new QHBoxLayout();
    hlyr->setContentsMargins(0, 0, 0, 0);
    mainLayout()->addLayout(hlyr);
    m_dataSourceLabel = new QLabel(xi18n("Form's data source"), this);
    m_dataSourceLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    m_dataSourceLabel->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
    hlyr->addWidget(m_dataSourceLabel);

    m_gotoButton = new KexiSmallToolButton(
        koIcon("go-jump"), QString(), this);
    m_gotoButton->setObjectName("gotoButton");
    m_gotoButton->setToolTip(xi18n("Go to selected form's data source"));
    m_gotoButton->setWhatsThis(xi18n("Goes to selected form's data source"));
    hlyr->addWidget(m_gotoButton);
    connect(m_gotoButton, SIGNAL(clicked()), this, SLOT(slotGotoSelected()));

#if 0
    m_clearDSButton = new KexiSmallToolButton(
        koIcon("edit-clear-locationbar-rtl"), QString(), this);
    m_clearDSButton->setObjectName("clearDSButton");
    m_clearDSButton->setMinimumHeight(m_dataSourceLabel->minimumHeight());
    m_clearDSButton->setToolTip(futureI18n("Clear form's data source"));
    hlyr->addWidget(m_clearDSButton);
    connect(m_clearDSButton, SIGNAL(clicked()), this, SLOT(clearFormDataSourceSelection()));
#endif

    m_formDataSourceCombo = new KexiDataSourceComboBox(this);
    m_formDataSourceCombo->setObjectName("dataSourceCombo");
    m_formDataSourceCombo->setContentsMargins(0, 0, 0, 0);
    m_dataSourceLabel->setBuddy(m_formDataSourceCombo);
    mainLayout()->addWidget(m_formDataSourceCombo);

    m_formDataSourceComboSpacer = addWidgetSpacer();

#ifdef KEXI_NO_AUTOFIELD_WIDGET
    m_availableFieldsLabel = 0;
    m_addField = 0;
    mainLayout()->addStretch();
#else
    //2. Inserting fields

    //helper info
//! @todo allow to hide such helpers by adding global option
    hlyr = new QHBoxLayout();
    hlyr->setContentsMargins(0, 0, 0, 0);
    mainLayout()->addLayout(hlyr);
    m_mousePointerLabel = new QLabel(this);
    hlyr->addWidget(m_mousePointerLabel);
    m_mousePointerLabel->setPixmap(koIcon("mouse_pointer"));
    m_mousePointerLabel->setFixedWidth(m_mousePointerLabel->pixmap()
                                       ? m_mousePointerLabel->pixmap()->width() : 0);
    m_availableFieldsDescriptionLabel = new QLabel(
        futureI18n("Select fields from the list below and drag them onto"
             " a form or click the <interface>Insert</interface> button"), this);
    m_availableFieldsDescriptionLabel->setAlignment(Qt::AlignLeft);
    m_availableFieldsDescriptionLabel->setWordWrap(true);
    hlyr->addWidget(m_availableFieldsDescriptionLabel);

    //Available Fields
    hlyr = new QHBoxLayout();
    hlyr->setContentsMargins(0, 0, 0, 0);
    mainLayout()->addLayout(hlyr);
    m_availableFieldsLabel = new QLabel(futureI18n("Available fields"), this);
    m_availableFieldsLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    hlyr->addWidget(m_availableFieldsLabel);

    m_addField = new KexiSmallToolButton(
        koIcon("add_field"), futureI18nc("Insert selected field into form", "Insert"), this);
    m_addField->setObjectName("addFieldButton");
    m_addField->setFocusPolicy(Qt::StrongFocus);
    m_addField->setToolTip(futureI18n("Insert selected fields into form"));
    m_addField->setWhatsThis(futureI18n("Inserts selected fields into form"));
    hlyr->addWidget(m_addField);
    connect(m_addField, SIGNAL(clicked()), this, SLOT(slotInsertSelectedFields()));

    m_fieldListView = new KexiFieldListView(this,
        KexiFieldListView::ShowDataTypes | KexiFieldListView::AllowMultiSelection);
    m_fieldListView->setObjectName("fieldListView");
    m_fieldListView->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
    m_availableFieldsLabel->setBuddy(m_fieldListView);
    mainLayout()->addWidget(m_fieldListView, 1);
    connect(m_fieldListView, SIGNAL(selectionChanged()),
            this, SLOT(slotFieldListViewSelectionChanged()));
    connect(m_fieldListView,
            SIGNAL(fieldDoubleClicked(QString,QString,QString)),
            this, SLOT(slotFieldDoubleClicked(QString,QString,QString)));
#endif

    mainLayout()->addStretch(1);

    connect(m_formDataSourceCombo, SIGNAL(textChanged(QString)),
            this, SLOT(slotFormDataSourceTextChanged(QString)));
    connect(m_formDataSourceCombo, SIGNAL(dataSourceChanged()),
            this, SLOT(slotFormDataSourceChanged()));
    connect(m_widgetDataSourceCombo, SIGNAL(selected()),
            this, SLOT(slotFieldSelected()));

    clearFormDataSourceSelection();
    slotFieldListViewSelectionChanged();
}

KexiDataSourcePage::~KexiDataSourcePage()
{
#ifdef KEXI_NO_AUTOFIELD_WIDGET
    delete m_tableOrQuerySchema;
#endif
}

void KexiDataSourcePage::setProject(KexiProject *prj)
{
    m_widgetDataSourceCombo->setProject(prj);
    m_formDataSourceCombo->setProject(prj);
}

void KexiDataSourcePage::clearFormDataSourceSelection(bool alsoClearComboBox)
{
    if (m_insideClearFormDataSourceSelection)
        return;
    m_insideClearFormDataSourceSelection = true;
    if (alsoClearComboBox && !m_formDataSourceCombo->selectedName().isEmpty())
        m_formDataSourceCombo->setDataSource(QString(), QString());
    m_gotoButton->setEnabled(false);
    m_widgetDataSourceCombo->setFieldOrExpression(QString());
#ifndef KEXI_NO_AUTOFIELD_WIDGET
    m_addField->setEnabled(false);
    m_fieldListView->clear();
#endif
    m_insideClearFormDataSourceSelection = false;
}

void KexiDataSourcePage::slotWidgetDataSourceTextChanged(const QString &text)
{
    if (text.isEmpty()) {
        clearWidgetDataSourceSelection();
    }
}

void KexiDataSourcePage::clearWidgetDataSourceSelection()
{
    m_widgetDataSourceCombo->setFieldOrExpression(QString());
    slotFieldSelected();
}

void KexiDataSourcePage::slotGotoSelected()
{
    const QString pluginId(m_formDataSourceCombo->selectedPluginId());
    if (pluginId == "org.kexi-project.table" || pluginId == "org.kexi-project.query") {
        if (m_formDataSourceCombo->isSelectionValid())
            emit jumpToObjectRequested(pluginId, m_formDataSourceCombo->selectedName());
    }
}

void KexiDataSourcePage::slotInsertSelectedFields()
{
#ifndef KEXI_NO_AUTOFIELD_WIDGET
    QStringList selectedFieldNames(m_fieldListView->selectedFieldNames());
    if (selectedFieldNames.isEmpty())
        return;

    emit insertAutoFields(m_fieldListView->schema()->table()
                            ? "org.kexi-project.table" : "org.kexi-project.query",
                          m_fieldListView->schema()->name(), selectedFieldNames);
#endif
}

void KexiDataSourcePage::slotFieldDoubleClicked(const QString& sourcePluginId, const QString& sourceName,
        const QString& fieldName)
{
#ifdef KEXI_NO_AUTOFIELD_WIDGET
    Q_UNUSED(sourcePluginId);
    Q_UNUSED(sourceName);
    Q_UNUSED(fieldName);
#else
    QStringList selectedFields;
    selectedFields.append(fieldName);
    emit insertAutoFields(sourcePluginId, sourceName, selectedFields);
#endif
}

void KexiDataSourcePage::slotFormDataSourceTextChanged(const QString &text)
{
    const bool enable = m_formDataSourceCombo->isSelectionValid();
    if (text.isEmpty()) {
        clearFormDataSourceSelection();
    } else if (!enable) {
        clearFormDataSourceSelection(m_formDataSourceCombo->selectedName().isEmpty()/*alsoClearComboBox*/);
    }
    updateSourceFieldWidgetsAvailability();
}

void KexiDataSourcePage::slotFormDataSourceChanged()
{
    if (!m_formDataSourceCombo->project())
        return;
    const QString pluginId(m_formDataSourceCombo->selectedPluginId());
    bool dataSourceFound = false;
    QString name(m_formDataSourceCombo->selectedName());
    const bool isIdAcceptable = pluginId == QLatin1String("org.kexi-project.table")
        || pluginId == QLatin1String("org.kexi-project.query");
    if (isIdAcceptable && m_formDataSourceCombo->isSelectionValid())
    {
        KDbTableOrQuerySchema *tableOrQuery = new KDbTableOrQuerySchema(
            m_formDataSourceCombo->project()->dbConnection(), name.toLatin1(),
            pluginId == "org.kexi-project.table");
        if (tableOrQuery->table() || tableOrQuery->query()) {
#ifdef KEXI_NO_AUTOFIELD_WIDGET
            m_tableOrQuerySchema = tableOrQuery;
#else
            m_fieldListView->setSchema(tableOrQuery);
#endif
            dataSourceFound = true;
            m_widgetDataSourceCombo->setTableOrQuery(name, pluginId == "org.kexi-project.table");
        } else {
            delete tableOrQuery;
        }
    }
    if (!dataSourceFound) {
        m_widgetDataSourceCombo->setTableOrQuery(QString(), true);
    }
    m_gotoButton->setEnabled(dataSourceFound);
    if (dataSourceFound) {
        slotFieldListViewSelectionChanged();
    } else {
#ifndef KEXI_NO_AUTOFIELD_WIDGET
        m_addField->setEnabled(false);
#endif
    }
    updateSourceFieldWidgetsAvailability();
    emit formDataSourceChanged(pluginId, name);
}

void KexiDataSourcePage::slotFieldSelected()
{
    KDbField::Type dataType = KDbField::InvalidType;
#ifdef KEXI_NO_AUTOFIELD_WIDGET
    KDbField *field = m_tableOrQuerySchema->field(
                               m_widgetDataSourceCombo->fieldOrExpression());  //temp
#else
//! @todo this should also work for expressions
    KDbField *field = m_fieldListView->schema()->field(
                               m_widgetDataSourceCombo->fieldOrExpression());
#endif
    if (field)
        dataType = field->type();

    emit dataSourceFieldOrExpressionChanged(
        m_widgetDataSourceCombo->fieldOrExpression(),
        m_widgetDataSourceCombo->fieldOrExpressionCaption(),
        dataType
    );
}

void KexiDataSourcePage::setFormDataSource(const QString& pluginId, const QString& name)
{
    m_formDataSourceCombo->setDataSource(pluginId, name);
}

#define KexiDataSourcePage_FADE 1

void KexiDataSourcePage::assignPropertySet(KPropertySet* propertySet)
{
    QString objectName;
    if (propertySet)
        objectName = propertySet->propertyValue("objectName").toString();
    if (!objectName.isEmpty() && objectName == m_currentObjectName)
        return; //the same object
    m_currentObjectName = objectName;

//! @todo
#if KexiDataSourcePage_FADE
    KFadeWidgetEffect *animation = 0;
    if (isVisible())
        animation = new KFadeWidgetEffect(this);
#endif
    QString objectClassName;
    if (propertySet) {
        objectClassName = propertySet->propertyValue("this:className").toString();
    }
    updateInfoLabelForPropertySet(propertySet);

    const bool isForm = objectClassName == "KexiDBForm";
    const bool multipleSelection = objectClassName == "special:multiple";
    const bool hasDataSourceProperty = propertySet
                                       && propertySet->contains("dataSource") && !multipleSelection;

    if (!isForm) {
        //this is a widget
        QString dataSource;
        if (hasDataSourceProperty) {
            if (propertySet) {
                dataSource = (*propertySet)["dataSource"].value().toString();
            }
            m_noDataSourceAvailableLabel->hide();
            m_widgetDataSourceCombo->setFieldOrExpression(dataSource);
            m_widgetDataSourceCombo->setEnabled(true);
            m_widgetDSLabel->show();
            m_widgetDataSourceCombo->show();
            m_widgetDataSourceComboSpacer->show();
            updateSourceFieldWidgetsAvailability();
        }
    }

    if (isForm) {
        m_noDataSourceAvailableLabel->hide();
    }
    else if (!hasDataSourceProperty) {
        if (multipleSelection) {
            m_noDataSourceAvailableLabel->setText(m_noDataSourceAvailableMultiText);
        }
        else {
            m_noDataSourceAvailableLabel->setText(m_noDataSourceAvailableSingleText);
        }
        m_noDataSourceAvailableLabel->show();
        m_widgetDataSourceCombo->setEditText(QString());
    }

    if (isForm || !hasDataSourceProperty) {
        //no source field can be set
        m_widgetDSLabel->hide();
        m_widgetDataSourceCombo->hide();
        m_widgetDataSourceComboSpacer->hide();
    }
//! @todo
#if KexiDataSourcePage_FADE
    if (animation)
        animation->start(100);
#endif
}

void KexiDataSourcePage::slotFieldListViewSelectionChanged()
{
#ifndef KEXI_NO_AUTOFIELD_WIDGET
    //update "add field" button's state
    for (Q3ListViewItemIterator it(m_fieldListView); it.current(); ++it) {
        if (it.current()->isSelected()) {
            m_addField->setEnabled(true);
            return;
        }
    }
    m_addField->setEnabled(false);
#endif
}

void KexiDataSourcePage::updateSourceFieldWidgetsAvailability()
{
    const bool hasDataSource = m_formDataSourceCombo->isSelectionValid();
    m_widgetDataSourceCombo->setEnabled(hasDataSource);
    m_widgetDSLabel->setEnabled(hasDataSource);
#ifndef KEXI_NO_AUTOFIELD_WIDGET
    m_fieldListView->setEnabled(hasDataSource);
    m_availableFieldsLabel->setEnabled(hasDataSource);
    m_mousePointerLabel->setEnabled(hasDataSource);
    m_availableFieldsDescriptionLabel->setEnabled(hasDataSource);
#endif
}

