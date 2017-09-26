/*
* Kexi Report Plugin
* Copyright (C) 2007-2009 by Adam Pigg <adam@piggz.co.uk>
* Copyright (C) 2017 Jarosław Staniek <staniek@kde.org>
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
#include <kexiutils/utils.h>

#include <KLocalizedString>

#include <QDebug>
#include <QLabel>
#include <QDomElement>
#include <QVBoxLayout>

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
    KexiDataSourceComboBox *dataSource;
};

KexiSourceSelector::KexiSourceSelector(KexiProject* project, QWidget* parent)
        : QWidget(parent)
        , d(new Private)
{
    d->conn = project->dbConnection();

    d->layout = new QVBoxLayout(this);
    d->dataSource = new KexiDataSourceComboBox(this);
    d->dataSource->setProject(project);
    connect(d->dataSource, &KexiDataSourceComboBox::dataSourceChanged, this,
            &KexiSourceSelector::dataSourceChanged);

    QLabel *label = new QLabel(xi18n("Report's data source:"));
    label->setBuddy(d->dataSource);
    d->layout->addWidget(label);
    d->layout->addWidget(d->dataSource);
    d->layout->addStretch();
    setLayout(d->layout);
}

KexiSourceSelector::~KexiSourceSelector()
{
    delete d;
}

void KexiSourceSelector::setConnectionData(const QDomElement &c)
{
    qDebug() << c;
    if (c.attribute("type") == "internal") {
        QString sourceClass(c.attribute("class"));
        if (sourceClass != "org.kexi-project.table" && sourceClass != "org.kexi-project.query") {
            sourceClass.clear(); // KexiDataSourceComboBox will try to find table, then query
        }
        d->dataSource->setDataSource(sourceClass, c.attribute("source"));
        emit dataSourceChanged();
    }
}

QDomElement KexiSourceSelector::connectionData()
{
    QDomDocument dd;
    QDomElement conndata = dd.createElement("connection");
    conndata.setAttribute("type", "internal"); // for backward compatibility, currently always
                                               // internal, we used to have "external" in old Kexi
    conndata.setAttribute("source", d->dataSource->selectedName());
    conndata.setAttribute("class", d->dataSource->selectedPluginId());
    return conndata;
}

KReportDataSource* KexiSourceSelector::createDataSource() const
{
    if (d->dataSource->isSelectionValid()) {
        return new KexiDBReportDataSource(d->dataSource->selectedName(),
                                          d->dataSource->selectedPluginId(), d->conn);
    }
    return nullptr;
}

