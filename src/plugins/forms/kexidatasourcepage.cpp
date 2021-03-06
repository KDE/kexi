/* This file is part of the KDE project
   Copyright (C) 2005-2017 Jarosław Staniek <staniek@kde.org>

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
#include <KexiStyle.h>
#include <config-kexi.h>
#include <widget/properties/KexiObjectInfoWidget.h>
#include <widget/KexiDataSourceComboBox.h>
#include <widget/fields/KexiFieldListView.h>
#include <widget/fields/KexiFieldComboBox.h>
#include <kexiutils/SmallToolButton.h>
#include <kexiutils/KexiFadeWidgetEffect.h>
#include <kexiutils/utils.h>
#include <kexiproject.h>
#include <formeditor/commands.h>

#include <KDbConnection>

#include <KProperty>

#include <KLocalizedString>

#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>

KexiDataSourcePage::KexiDataSourcePage(QWidget *parent)
        : QWidget(parent)
        , m_noDataSourceAvailableSingleText(
            xi18n("[Can't assign to this widget]") )
        , m_noDataSourceAvailableMultiText(
            xi18n("[Can't assign to multiple widgets]") )
        , m_insideClearFormDataSourceSelection(false)
        , m_slotWidgetDataSourceTextChangedEnabled(true)
#ifndef KEXI_AUTOFIELD_FORM_WIDGET_SUPPORT
        , m_tableOrQuerySchema(0)
#endif
{
    const KexiStyle::PropertyPane &s = KexiStyle::propertyPane();
    m_mainLyr = s.createVLayout(this);

    s.createTitleLabel(xi18nc("@label Form data source - title", "Data source"), m_mainLyr);

    m_formLyr = s.createFormLayout(m_mainLyr);

    //- Form's Data Source
//! @todo Port "Go to selected form's data source"
#if 0
    m_gotoButton = new KexiSmallToolButton(
        koIcon("go-jump"), QString(), this);
    m_gotoButton->setObjectName("gotoButton");
    m_gotoButton->setToolTip(xi18n("Go to selected form's data source"));
    m_gotoButton->setWhatsThis(xi18n("Goes to selected form's data source"));
    hlyr->addWidget(m_gotoButton);
    connect(m_gotoButton, SIGNAL(clicked()), this, SLOT(slotGotoSelected()));
#endif

    m_formDataSourceCombo = new KexiDataSourceComboBox;
    m_formDataSourceCombo->setObjectName("dataSourceCombo");
    s.addLabelAndWidget(xi18nc("@label Forms's data source (table or query)", "Form"),
                        m_formDataSourceCombo, m_formLyr);

    //-Widget's Data Source
    m_widgetDataSourceContainer = new QWidget;
    QVBoxLayout *widgetDataSourceContainerLyr = new QVBoxLayout(m_widgetDataSourceContainer);
    widgetDataSourceContainerLyr->setContentsMargins(0, 0, 0, 0);

    m_widgetDataSourceCombo = new KexiFieldComboBox;
    widgetDataSourceContainerLyr->addWidget(m_widgetDataSourceCombo);
    s.alterComboBoxStyle(m_widgetDataSourceCombo);
    m_widgetDataSourceCombo->setObjectName("sourceFieldCombo");
    connect(m_widgetDataSourceCombo, &KexiFieldComboBox::editTextChanged,
        this, &KexiDataSourcePage::slotWidgetDataSourceTextChanged);

    m_noDataSourceAvailableLabel = s.createWarningLabel(m_noDataSourceAvailableSingleText);
    m_noDataSourceAvailableLabel->hide();
    widgetDataSourceContainerLyr->addWidget(m_noDataSourceAvailableLabel);

    s.addLabelAndWidget(xi18nc("@label Widget's data source (table field or query field)", "Widget"),
                        m_widgetDataSourceContainer, m_formLyr);

#ifndef KEXI_AUTOFIELD_FORM_WIDGET_SUPPORT
    m_mainLyr->addStretch();
#else
    //2. Inserting fields

    //helper info
//! @todo allow to hide such helpers by adding global option
    hlyr = new QHBoxLayout();
    hlyr->setContentsMargins(0, 0, 0, 0);
    m_mainLyr->addLayout(hlyr);
    m_mousePointerLabel = new QLabel(this);
    hlyr->addWidget(m_mousePointerLabel);
    m_mousePointerLabel->setPixmap(koIcon("tool-pointer"));
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
    m_mainLyr->addLayout(hlyr);
    m_availableFieldsLabel = new QLabel(futureI18n("Available fields"), this);
    m_availableFieldsLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    hlyr->addWidget(m_availableFieldsLabel);

    m_addField = new KexiSmallToolButton(
        KexiIcon("add-field"), futureI18nc("Insert selected field into form", "Insert"), this);
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
    m_mainLyr->addWidget(m_fieldListView, 1);
    connect(m_fieldListView, SIGNAL(selectionChanged()),
            this, SLOT(slotFieldListViewSelectionChanged()));
    connect(m_fieldListView,
            SIGNAL(fieldDoubleClicked(QString,QString,QString)),
            this, SLOT(slotFieldDoubleClicked(QString,QString,QString)));
#endif

    m_mainLyr->addStretch(1);

    connect(m_formDataSourceCombo, SIGNAL(editTextChanged(QString)),
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
#ifndef KEXI_AUTOFIELD_FORM_WIDGET_SUPPORT
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
    //! @todo m_gotoButton->setEnabled(false);
    m_widgetDataSourceCombo->setFieldOrExpression(QString());
#ifdef KEXI_AUTOFIELD_FORM_WIDGET_SUPPORT
    m_addField->setEnabled(false);
    m_fieldListView->clear();
#endif
    m_insideClearFormDataSourceSelection = false;
}

void KexiDataSourcePage::slotWidgetDataSourceTextChanged(const QString &text)
{
    if (!m_slotWidgetDataSourceTextChangedEnabled) {
        return;
    }
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
    bool ok;
    (void)KexiProject::pluginIdToTableOrQueryType(pluginId, &ok);
    if (ok) {
        if (m_formDataSourceCombo->isSelectionValid())
            emit jumpToObjectRequested(pluginId, m_formDataSourceCombo->selectedName());
    }
}

void KexiDataSourcePage::slotInsertSelectedFields()
{
#ifdef KEXI_AUTOFIELD_FORM_WIDGET_SUPPORT
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
#ifdef KEXI_AUTOFIELD_FORM_WIDGET_SUPPORT
    QStringList selectedFields;
    selectedFields.append(fieldName);
    emit insertAutoFields(sourcePluginId, sourceName, selectedFields);
#else
    Q_UNUSED(sourcePluginId);
    Q_UNUSED(sourceName);
    Q_UNUSED(fieldName);
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
    bool isIdAcceptable;
    const KDbTableOrQuerySchema::Type type = KexiProject::pluginIdToTableOrQueryType(
                pluginId, &isIdAcceptable);
    if (isIdAcceptable && m_formDataSourceCombo->isSelectionValid()) {
        KDbTableOrQuerySchema *tableOrQuery = new KDbTableOrQuerySchema(
            m_formDataSourceCombo->project()->dbConnection(), name.toLatin1(), type);
        if (tableOrQuery->table() || tableOrQuery->query()) {
#ifdef KEXI_AUTOFIELD_FORM_WIDGET_SUPPORT
            m_fieldListView->setSchema(tableOrQuery);
#else
            m_tableOrQuerySchema = tableOrQuery;
#endif
            dataSourceFound = true;
            m_slotWidgetDataSourceTextChangedEnabled = false; // block clearing widget's data source property
            m_widgetDataSourceCombo->setTableOrQuery(name, type);
            m_slotWidgetDataSourceTextChangedEnabled = true;
        } else {
            delete tableOrQuery;
        }
    }
    if (!dataSourceFound) {
        m_widgetDataSourceCombo->setTableOrQuery(QString(), KDbTableOrQuerySchema::Type::Table);
    }
    //! @todo m_gotoButton->setEnabled(dataSourceFound);
    if (dataSourceFound) {
        slotFieldListViewSelectionChanged();
    } else {
#ifdef KEXI_AUTOFIELD_FORM_WIDGET_SUPPORT
        m_addField->setEnabled(false);
#endif
    }
    updateSourceFieldWidgetsAvailability();
    emit formDataSourceChanged(pluginId, name);
}

void KexiDataSourcePage::slotFieldSelected()
{
    KDbField::Type dataType = KDbField::InvalidType;
#ifdef KEXI_AUTOFIELD_FORM_WIDGET_SUPPORT
    //! @todo this should also work for expressions
        KDbField *field = m_fieldListView->schema()->field(
                                   m_widgetDataSourceCombo->fieldOrExpression());
#else
    KDbField *field = m_tableOrQuerySchema->field(
                               m_widgetDataSourceCombo->fieldOrExpression());  //temp
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

void KexiDataSourcePage::assignPropertySet(KPropertySet* propertySet, AssignFlags flags)
{
    const KexiStyle::PropertyPane &s = KexiStyle::propertyPane();
    QString objectName;
    if (propertySet)
        objectName = propertySet->propertyValue("objectName").toString();
    if (flags != ForceAssign && !objectName.isEmpty() && objectName == m_currentObjectName) {
        return; //the same object
    }
    m_currentObjectName = objectName;
    QString objectClassName;
    if (propertySet) {
        objectClassName = propertySet->propertyValue("this:className").toString();
    }

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
            m_widgetDataSourceCombo->show();
            s.setFormLabelAndWidgetVisible(m_widgetDataSourceContainer, m_formLyr, true);
            updateSourceFieldWidgetsAvailability();
        }
    }

    if (isForm) {
        // no source field can be set
        s.setFormLabelAndWidgetVisible(m_widgetDataSourceContainer, m_formLyr, false);
    }
    else if (!hasDataSourceProperty) {
        if (multipleSelection) {
            m_noDataSourceAvailableLabel->setText(m_noDataSourceAvailableMultiText);
        }
        else {
            m_noDataSourceAvailableLabel->setText(m_noDataSourceAvailableSingleText);
        }
        s.setFormLabelAndWidgetVisible(m_widgetDataSourceContainer, m_formLyr, true);
        m_widgetDataSourceCombo->hide();
        m_noDataSourceAvailableLabel->show();
        m_widgetDataSourceCombo->setEditText(QString());
    }

    m_mainLyr->update();
    qApp->processEvents();
}

void KexiDataSourcePage::slotFieldListViewSelectionChanged()
{
#ifdef KEXI_AUTOFIELD_FORM_WIDGET_SUPPORT
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
#ifdef KEXI_AUTOFIELD_FORM_WIDGET_SUPPORT
    m_fieldListView->setEnabled(hasDataSource);
    m_availableFieldsLabel->setEnabled(hasDataSource);
    m_mousePointerLabel->setEnabled(hasDataSource);
    m_availableFieldsDescriptionLabel->setEnabled(hasDataSource);
#endif
}

QString KexiDataSourcePage::selectedPluginId() const
{
    return m_formDataSourceCombo->selectedPluginId();
}

QString KexiDataSourcePage::selectedName() const
{
    return m_formDataSourceCombo->selectedName();
}
