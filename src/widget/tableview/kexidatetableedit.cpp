/* This file is part of the KDE project
   Copyright (C) 2002   Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2003   Daniel Molkentin <molkentin@kde.org>
   Copyright (C) 2003-2004,2006 Jarosław Staniek <staniek@kde.org>

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

#include "kexidatetableedit.h"

#include <QApplication>
#include <QPainter>
#include <QVariant>
#include <QRect>
#include <QPalette>
#include <QColor>
#include <QFontMetrics>
#include <QDateTime>
#include <QCursor>
#include <QPoint>
#include <QLayout>
#include <QToolButton>
#include <QClipboard>

#include <kdebug.h>
#include <klocale.h>
#include <klineedit.h>

#include <kexiutils/utils.h>


KexiDateTableEdit::KexiDateTableEdit(KexiDB::TableViewColumn &column, QWidget *parent)
        : KexiInputTableEdit(column, parent)
{
    setObjectName("KexiDateTableEdit");

//! @todo add QValidator so date like "2006-59-67" cannot be even entered

    kDebug() << m_formatter.inputMask();
    m_lineedit->setInputMask(m_formatter.inputMask());
}

KexiDateTableEdit::~KexiDateTableEdit()
{
}

void KexiDateTableEdit::setValueInInternalEditor(const QVariant &value)
{
    if (value.isValid() && value.toDate().isValid())
        m_lineedit->setText(m_formatter.toString(value.toDate()));
    else
        m_lineedit->setText(QString());
}

void KexiDateTableEdit::setValueInternal(const QVariant& add_, bool removeOld)
{
    if (removeOld) {
        //new date entering... just fill the line edit
//! @todo cut string if too long..
        QString add(add_.toString());
        m_lineedit->setText(add);
        m_lineedit->setCursorPosition(add.length());
        return;
    }
    setValueInInternalEditor(KexiDataItemInterface::originalValue());
    m_lineedit->setCursorPosition(0); //ok?
}

void KexiDateTableEdit::setupContents(QPainter *p, bool focused, const QVariant& val,
                                      QString &txt, int &align, int &x, int &y_offset, int &w, int &h)
{
    Q_UNUSED(p);
    Q_UNUSED(focused);
    Q_UNUSED(x);
    Q_UNUSED(w);
    Q_UNUSED(h);
#ifdef Q_WS_WIN
    y_offset = -1;
#else
    y_offset = 0;
#endif
    if (val.toDate().isValid())
        txt = m_formatter.toString(val.toDate());
    align |= Qt::AlignLeft;
}

bool KexiDateTableEdit::valueIsNull()
{
    if (m_formatter.isEmpty(m_lineedit->text())) //empty date is null
        return true;
    return dateValue().isNull();
}

bool KexiDateTableEdit::valueIsEmpty()
{
    return valueIsNull();//js OK? TODO (nonsense?)
}

QDate KexiDateTableEdit::dateValue() const
{
    return m_formatter.fromString(m_lineedit->text());
}

QVariant KexiDateTableEdit::value()
{
    return m_formatter.stringToVariant(m_lineedit->text());
}

bool KexiDateTableEdit::valueIsValid()
{
    if (m_formatter.isEmpty(m_lineedit->text())) //empty date is valid
        return true;
    return m_formatter.fromString(m_lineedit->text()).isValid();
}

bool KexiDateTableEdit::valueChanged()
{
    //kDebug() << m_origValue.toString() << " ? " << m_lineedit->text();
    return KexiDataItemInterface::originalValue() != m_lineedit->text();
}

void KexiDateTableEdit::handleCopyAction(const QVariant& value, const QVariant& visibleValue)
{
    Q_UNUSED(visibleValue);
    if (!value.isNull() && value.toDate().isValid())
        qApp->clipboard()->setText(m_formatter.toString(value.toDate()));
    else
        qApp->clipboard()->setText(QString());
}

void KexiDateTableEdit::handleAction(const QString& actionName)
{
    const bool alreadyVisible = m_lineedit->isVisible();

    if (actionName == "edit_paste") {
        const QVariant newValue(m_formatter.fromString(qApp->clipboard()->text()));
        if (!alreadyVisible) { //paste as the entire text if the cell was not in edit mode
            emit editRequested();
            m_lineedit->clear();
        }
        setValueInInternalEditor(newValue);
    } else
        KexiInputTableEdit::handleAction(actionName);
}

KEXI_CELLEDITOR_FACTORY_ITEM_IMPL(KexiDateEditorFactoryItem, KexiDateTableEdit)

#include "kexidatetableedit.moc"
