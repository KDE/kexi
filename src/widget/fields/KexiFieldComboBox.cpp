/* This file is part of the KDE project
   Copyright (C) 2005-2006 Jarosław Staniek <staniek@kde.org>

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

#include "KexiFieldComboBox.h"
#include <kexiutils/utils.h>
#include <kexiproject.h>
#include <kexi_global.h>
#include "KexiFieldListModel.h"

#include <KDbTableSchema>
#include <KDbQuerySchema>
#include <KDbTableOrQuerySchema>
#include <KDbUtils>

#include <QPushButton>
#include <QPoint>
#include <QLineEdit>

//! @internal
class Q_DECL_HIDDEN KexiFieldComboBox::Private
{
public:
    Private()
    {
    }
    ~Private() {
    }
    QPointer<KexiProject> prj;
    QPointer<KexiFieldListModel> model;

    QString tableOrQueryName;
    QString fieldOrExpression;

    KDbTableOrQuerySchema::Type type = KDbTableOrQuerySchema::Type::Table;
    bool insideSetFieldOrExpression = false;
};

//------------------------

KexiFieldComboBox::KexiFieldComboBox(QWidget *parent)
        : KComboBox(true/*rw*/, parent)
        , d(new Private())
{
    setInsertPolicy(NoInsert);
    setCompletionMode(KCompletion::CompletionPopupAuto);

    setMaxVisibleItems(16);
    connect(this, SIGNAL(activated(int)),
            this, SLOT(slotActivated(int)));
    connect(this, SIGNAL(returnPressed(QString)),
            this, SLOT(slotReturnPressed(QString)));
}

KexiFieldComboBox::~KexiFieldComboBox()
{
    delete d;
}

void KexiFieldComboBox::setProject(KexiProject *prj)
{
    if (static_cast<KexiProject*>(d->prj) == prj)
        return;
    d->prj = prj;
    setTableOrQuery(QString(), KDbTableOrQuerySchema::Type::Table);
}

KexiProject* KexiFieldComboBox::project() const
{
    return d->prj;
}

void KexiFieldComboBox::setTableOrQuery(const QString& name, KDbTableOrQuerySchema::Type type)
{
    d->tableOrQueryName = name;
    d->type = type;
    clear();

    if (d->tableOrQueryName.isEmpty() || !d->prj)
        return;

    KDbTableOrQuerySchema tableOrQuery(d->prj->dbConnection(), d->tableOrQueryName.toLatin1(),
                                       type);
    if (!tableOrQuery.table() && !tableOrQuery.query())
        return;

    delete d->model;
    d->model = new KexiFieldListModel(this, ShowEmptyItem);

    d->model->setSchema(d->prj->dbConnection(), &tableOrQuery);
    setModel(d->model);

    //update selection
    setFieldOrExpression(d->fieldOrExpression);
}

QString KexiFieldComboBox::tableOrQueryName() const
{
    return d->tableOrQueryName;
}

bool KexiFieldComboBox::isTableAssigned() const
{
    return d->type == KDbTableOrQuerySchema::Type::Table;
}

void KexiFieldComboBox::setFieldOrExpression(const QString& string)
{
    if (d->insideSetFieldOrExpression) {
        return;
    }
    KexiUtils::BoolBlocker guard(&d->insideSetFieldOrExpression, true);
    const QString name(string);
    const int pos = name.indexOf('.');
    if (pos == -1) {
        d->fieldOrExpression = name;
    } else {
        QString objectName = name.left(pos);
        if (d->tableOrQueryName != objectName) {
            d->fieldOrExpression = name;
            setEditText(name);
//! @todo show error
            qWarning() << "invalid table/query name in" << name;
            return;
        }
        d->fieldOrExpression = name.mid(pos + 1);
    }

    //! @todo show 'the item doesn't match' info?
    setEditText(d->fieldOrExpression);
}

void KexiFieldComboBox::setFieldOrExpression(int index)
{
    if (index >= 0) {
        index++; //skip 1st empty item
    }
    if (index >= count()) {
        qWarning() << "index" << index << "out of range 0.." << (count() - 1);
        index = -1;
    }
    if (index <= 0) {
        setCurrentIndex(0);
        d->fieldOrExpression.clear();
    } else {
        setCurrentIndex(index);
        d->fieldOrExpression = itemData(currentIndex(), Qt::DisplayRole).toString();
        lineEdit()->setText(d->fieldOrExpression);
    }
}

QString KexiFieldComboBox::fieldOrExpression() const
{
    return d->fieldOrExpression;
}

int KexiFieldComboBox::indexOfField() const
{
    if (d->tableOrQueryName.isEmpty()) {
        return -1;
    }
    KDbTableOrQuerySchema tableOrQuery(d->prj->dbConnection(), d->tableOrQueryName.toLatin1(),
                                       d->type);
    if (!tableOrQuery.table() && !tableOrQuery.query())
        return -1;

    return currentIndex() > 0 ? (currentIndex() - 1) : -1;
}

QString KexiFieldComboBox::fieldOrExpressionCaption() const
{
    return itemData(currentIndex()).toString();
}

void KexiFieldComboBox::slotActivated(int i)
{
    d->fieldOrExpression = itemData(i, Qt::DisplayRole).toString();
    setFieldOrExpression(d->fieldOrExpression);
    emit selected();
}

void KexiFieldComboBox::slotReturnPressed(const QString & text)
{
    //text is available: select item for this text:
    int index;
    if (text.isEmpty()) {
        index = 0;
    } else {
        index = findText(text, Qt::MatchExactly);
        if (index < 1)
            return;
    }
    setCurrentIndex(index);
    slotActivated(index);
}

void KexiFieldComboBox::focusOutEvent(QFocusEvent *e)
{
    KComboBox::focusOutEvent(e);
    // accept changes if the focus is moved
    if (!KDbUtils::hasParent(this, focusWidget())) {
        //(a check needed because drop-down listbox also causes a focusout)
        slotReturnPressed(currentText());
    }
}
