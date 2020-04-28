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
* License along with this library.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "kexisourceselector.h"
#include "KexiDataSourceComboBox.h"
#include <kexiproject.h>

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

QString KexiSourceSelector::selectedPluginId() const
{
    return d->dataSource->selectedPluginId();
}

QString KexiSourceSelector::selectedName() const
{
    return d->dataSource->selectedName();
}

bool KexiSourceSelector::isSelectionValid() const
{
    return d->dataSource->isSelectionValid();
}

void KexiSourceSelector::setDataSource(const QString& pluginId, const QString& name)
{
    d->dataSource->setDataSource(pluginId, name);
}
