/* This file is part of the KDE project
   Copyright (C) 2002 Peter Simonsson <psn@linux.se>
   Copyright (C) 2003-2014 Jarosław Staniek <staniek@kde.org>

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

#include "kexitableedit.h"
#include <widget/dataviewcommon/kexidataawareobjectiface.h>
#include <widget/tableview/KexiTableScrollArea.h>
#include <db/field.h>
#include <db/utils.h>

#include <QPalette>
#include <QPainter>
#include <QKeyEvent>
#include <QEvent>

#include <kglobal.h>
#include <kdebug.h>

#ifdef KEXI_MOBILE
KexiTableEdit::KexiTableEdit(KexiDB::TableViewColumn &column, QWidget* parent)
        : QWidget(parent)
        , m_column(&column)
        , m_usesSelectedTextColor(true)
        , m_view(0)
#else
        KexiTableEdit::KexiTableEdit(KexiDB::TableViewColumn &column, QWidget* parent)
        : QWidget(parent)
        , m_column(&column)
        , m_usesSelectedTextColor(true)
        , m_view(0)
#endif
{
    QPalette pal(palette());
    pal.setBrush(backgroundRole(), pal.brush(QPalette::Base));
    setPalette(pal);

    //margins
    if (displayedField()->isFPNumericType()) {
#ifdef Q_OS_WIN
        m_leftMargin = 0;
#else
        m_leftMargin = 0;
#endif
        m_rightMargin = 6;
    } else if (displayedField()->isIntegerType()) {
#ifdef Q_OS_WIN
        m_leftMargin = 1;
#else
        m_leftMargin = 0;
#endif
        m_rightMargin = 6;
    } else {//default
#ifdef Q_OS_WIN
        m_leftMargin = 5;
#else
        m_leftMargin = 5;
#endif
        m_rightMargin = 0;
    }

    m_rightMarginWhenFocused = m_rightMargin;
}

KexiTableEdit::~KexiTableEdit()
{
}

KexiDB::Field *KexiTableEdit::field() const
{
    return m_column->field();
}

KexiDB::QueryColumnInfo *KexiTableEdit::columnInfo() const
{
    return m_column->columnInfo();
}

void KexiTableEdit::setColumnInfo(KexiDB::QueryColumnInfo *)
{
}

KexiDB::TableViewColumn *KexiTableEdit::column() const
{
    return m_column;
}

QWidget* KexiTableEdit::widget()
{
    return m_view;
}

void KexiTableEdit::hideWidget()
{
    hide();
}

void KexiTableEdit::showWidget()
{
    show();
}

bool KexiTableEdit::usesSelectedTextColor() const
{
    return m_usesSelectedTextColor;
}

int KexiTableEdit::leftMargin() const
{
    return m_leftMargin;
}

bool KexiTableEdit::handleKeyPress(QKeyEvent* ke, bool editorActive)
{
    Q_UNUSED(ke);
    Q_UNUSED(editorActive);
    return false;
}

bool KexiTableEdit::handleDoubleClick()
{
    return false;
}

QSize KexiTableEdit::totalSize() const
{
    return QWidget::size();
}

void KexiTableEdit::createInternalEditor(KexiDB::QuerySchema& schema)
{
    Q_UNUSED(schema);
}

KexiDB::Field *KexiTableEdit::displayedField() const
{
    if (m_column->visibleLookupColumnInfo())
        return m_column->visibleLookupColumnInfo()->field; //mainly for lookup field in KexiComboBoxTableEdit:

    return m_column->field(); //typical case
}

void KexiTableEdit::setViewWidget(QWidget *v)
{
    m_view = v;
    m_view->move(0, 0);
    setFocusProxy(m_view);
}

void KexiTableEdit::moveChild(QWidget * child, int x, int y)
{
#ifndef KEXI_MOBILE
    child->move(x, y);
#endif
}

void KexiTableEdit::resize(int w, int h)
{
    QWidget::resize(w, h);
    if (m_view) {
        if (!layout()) { //if there is layout (eg. KexiInputTableEdit), resize is automatic
            m_view->move(0, 0);
            m_view->resize(w, h);
        }
    }
}

#if 0
bool
KexiTableEdit::eventFilter(QObject* watched, QEvent* e)
{
    if (watched == this) {
        if (e->type() == QEvent::KeyPress) {
            QKeyEvent* ev = static_cast<QKeyEvent*>(e);

            if (ev->key() == Qt::Key_Escape) {
                return false;
            }
        } else {
            return false;
        }
    }
    return false;
}
#endif

void KexiTableEdit::paintFocusBorders(QPainter *p, QVariant &, int x, int y, int w, int h)
{
    p->drawRect(x, y, w, h);
}

void KexiTableEdit::setupContents(QPainter *p, bool focused, const QVariant& val,
                                  QString &txt, int &align, int &/*x*/, int &y_offset, int &w, int &h)
{
    Q_UNUSED(p);
    Q_UNUSED(focused);
    Q_UNUSED(h);
    KexiDB::Field *realField = displayedField();

#ifdef Q_OS_WIN
    y_offset = -1;
#else
    y_offset = 0;
#endif

    if (realField->isFPNumericType()) {
//! @todo ADD OPTION to displaying NULL VALUES as e.g. "(null)"
        if (!val.isNull()) {
            txt = KexiDB::formatNumberForVisibleDecimalPlaces(
                      val.toDouble(), realField->visibleDecimalPlaces());
        }
        align |= Qt::AlignRight;
    } else if (realField->isIntegerType()) {
        qint64 num = val.toLongLong();
        align |= Qt::AlignRight;
        if (!val.isNull())
            txt = QString::number(num);
    } else {//default:
        if (!val.isNull()) {
            txt = val.toString();
        }
        align |= Qt::AlignLeft;
    }
    w -= rightMargin(focused);
}

void KexiTableEdit::paintSelectionBackground(QPainter *p, bool /*focused*/,
        const QString& txt, int align, int x, int y_offset, int w, int h, const QColor& fillColor,
        const QFontMetrics &fm, bool readOnly, bool fullRecordSelection)
{
    if (!readOnly && !fullRecordSelection && !txt.isEmpty()) {
        QRect bound = fm.boundingRect(x, y_offset, w - (x + x), h, align, txt);
        bound.setY(0);
        bound.setWidth(qMin(bound.width() + 2, w - (x + x) + 1));
        if (align & Qt::AlignLeft) {
            bound.setX(bound.x() - 1);
        } else if (align & Qt::AlignRight) {
            bound.moveLeft(w - bound.width());   //move to left, if too wide
        }
//! @todo align center
        bound.setHeight(h - 1);
        p->fillRect(bound, fillColor);
    } else if (fullRecordSelection) {
        p->fillRect(0, 0, w, h, fillColor);
    }
}

int KexiTableEdit::widthForValue(const QVariant &val, const QFontMetrics &fm)
{
    return fm.width(val.toString());
}

void KexiTableEdit::repaintRelatedCell()
{
#ifndef KEXI_MOBILE
    if (dynamic_cast<KexiDataAwareObjectInterface*>(parentWidget())) {
        dynamic_cast<KexiDataAwareObjectInterface*>(parentWidget())->updateCurrentCell();
    }
#endif
}

bool KexiTableEdit::showToolTipIfNeeded(const QVariant& value, const QRect& rect, const QFontMetrics& fm,
                                        bool focused)
{
    Q_UNUSED(value);
    Q_UNUSED(rect);
    Q_UNUSED(fm);
    Q_UNUSED(focused);
    return false;
}

int KexiTableEdit::rightMargin(bool focused) const
{
    return focused ? m_rightMarginWhenFocused : m_rightMargin;
}

