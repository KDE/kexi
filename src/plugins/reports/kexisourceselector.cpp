/*
* Kexi Report Plugin
* Copyright (C) 2007-2009 by Adam Pigg (adam@piggz.co.uk)
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "kexisourceselector.h"
#include "KexiDataSourceComboBox.h"
#include <kexiproject.h>

#include <KLocalizedString>

#include <QDebug>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QDomElement>
#include <QVBoxLayout>

//#define NO_EXTERNAL_SOURCES

#ifdef NO_EXTERNAL_SOURCES
//! @todo KEXI3 enable external data sources for 2.3
#endif

class Q_DECL_HIDDEN KexiSourceSelector::Private
{
public:
    Private()
    {
    }

    ~Private()
    {
    }

    KDbConnection *conn;

    QVBoxLayout *layout;
    QComboBox *sourceType;
    KexiDataSourceComboBox *internalSource;
    QLineEdit *externalSource;
    QPushButton *setData;
};

KexiSourceSelector::KexiSourceSelector(KexiProject* project, QWidget* parent)
        : QWidget(parent)
        , d(new Private)
{
    d->conn = project->dbConnection();

    d->layout = new QVBoxLayout(this);
    d->sourceType = new QComboBox(this);
    d->internalSource = new KexiDataSourceComboBox(this);
    d->internalSource->setProject(project);
    d->externalSource = new QLineEdit(this);
    d->setData = new QPushButton(xi18n("Set Data"));

    connect(d->setData, &QPushButton::clicked, this, &KexiSourceSelector::sourceDataChanged);

    d->sourceType->addItem(xi18n("Internal"), QVariant("internal"));
    d->sourceType->addItem(xi18n("External"), QVariant("external"));

#ifndef NO_EXTERNAL_SOURCES

//!@TODO enable when adding external data

    d->layout->addWidget(new QLabel(xi18n("Source type:"), this));
    d->layout->addWidget(d->sourceType);
    d->layout->addSpacing(10);
#else
    d->sourceType->setVisible(false);
    d->externalSource->setVisible(false);
#endif

    d->layout->addWidget(new QLabel(xi18n("Report's data source:"), this));
    d->layout->addWidget(d->internalSource);
    d->layout->addSpacing(10);

#ifndef NO_EXTERNAL_SOURCES
    d->layout->addWidget(new QLabel(xi18n("External source:"), this));
    d->layout->addWidget(d->externalSource);
#endif
    d->layout->addSpacing(20);
    d->layout->addWidget(d->setData);
    d->layout->addStretch();
    setLayout(d->layout);
}

KexiSourceSelector::~KexiSourceSelector()
{
    delete d;
}

void KexiSourceSelector::setConnectionData(const QDomElement &c)
{
    if (c.attribute("type") == "internal") {
        d->sourceType->setCurrentIndex(d->sourceType->findData("internal"));
        d->internalSource->setCurrentIndex(d->internalSource->findText(c.attribute("source")));
    }

    if (c.attribute("type") == "external") {
        d->sourceType->setCurrentIndex(d->sourceType->findText("external"));
        d->externalSource->setText(c.attribute("source"));
    }

    emit sourceDataChanged();
}

QDomElement KexiSourceSelector::connectionData()
{
    QDomDocument dd;
    QDomElement conndata = dd.createElement("connection");

#ifndef NO_EXTERNAL_SOURCES
//!@TODO Make a better gui for selecting external data source

    conndata.setAttribute("type", d->sourceType->itemData(d->sourceType->currentIndex()).toString());

    if (d->sourceType->itemData(d->sourceType->currentIndex()).toString() == "internal") {
        conndata.setAttribute("source", d->internalSource->currentText());
    } else {
        conndata.setAttribute("source", d->externalSource->text());
    }
#else
    conndata.setAttribute("type", "internal");
    conndata.setAttribute("source", d->internalSource->currentText());
#endif
    return conndata;
}

KReportDataSource* KexiSourceSelector::createSourceData() const
{
//!@TODO Fix when enable external data
#ifndef NO_EXTERNAL_SOURCES
    if (d->sourceType->itemData(d->sourceType->currentIndex()).toString() == "internal" && d->internalSource->isSelectionValid()) {
        return new KexiDBReportDataSource(d->internalSource->selectedName(), d->internalSource->selectedPluginId(), d->conn);
    }

#ifndef KEXI_MOBILE
//! @todo KEXI3
#if 0
    if (d->sourceType->itemData(d->sourceType->currentIndex()).toString() == "external") {
        return new KexiMigrateReportData(d->externalSource->text());
        return d->kexiMigrateData;
    }
#endif
#endif

#else
    if (d->internalSource->isSelectionValid()) {
        return new KexiDBReportData(d->internalSource->selectedName(), d->conn);
    }
#endif
    return 0;
}

