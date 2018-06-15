/* This file is part of the KDE project
   Copyright (C) 2002 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2003-2016 Jarosław Staniek <staniek@kde.org>

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

#include "kexiinputtableedit.h"
#include <kexi_global.h>
#include <kexiutils/utils.h>

#include <KDbField>
#include <KDbFieldValidator>
#include <KDbLongLongValidator>

#include <KColorScheme>

#include <QPainter>
#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLocale>
#include <QDebug>

//! @internal
class MyLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit MyLineEdit(QWidget *parent) : QLineEdit(parent) {}
protected:
    virtual void drawFrame(QPainter * p) {
        p->setPen(QPen(palette().text(), 1.0));
        QRect r = rect();
        p->drawLine(r.topLeft(), r.topRight());
        p->drawLine(r.topRight(), r.bottomRight());
        p->drawLine(r.topRight(), r.bottomLeft());
        if (pos().x() == 0) //draw left side only when it is @ the edge
            p->drawLine(r.bottomLeft(), r.topLeft());
    }
};

//======================================================

KexiInputTableEdit::KexiInputTableEdit(KDbTableViewColumn *column, QWidget *parent)
        : KexiTableEdit(column, parent)
{
    init();
}

KexiInputTableEdit::~KexiInputTableEdit()
{
}

void KexiInputTableEdit::init()
{
// qDebug() << "m_origValue.typeName()==" << m_origValue.typeName();
// qDebug() << "type==" << field()->typeName();
// qDebug() << "displayed type==" << displayedField()->typeName();

    m_textFormatter.setField( field() );
    KexiTextFormatter::OverrideDecimalPlaces overrideDecimalPlaces;
    overrideDecimalPlaces.enabled = true;
    overrideDecimalPlaces.value = -1; // all possible digits
    m_textFormatter.setOverrideDecimalPlaces(overrideDecimalPlaces);
    m_textFormatter.setGroupSeparatorsEnabled(false); // needed, otherwise text box contains separators (confusing)

    //create layer for internal editor
    QHBoxLayout *lyr =  new QHBoxLayout(this);
    lyr->setContentsMargins(0, 0, 0, 0);

    //create internal editor
    m_lineedit = new MyLineEdit(this);
    m_lineedit->setObjectName("KexiInputTableEdit-MyLineEdit");
    connect(m_lineedit, SIGNAL(textEdited(QString)),
            this, SLOT(slotTextEdited(QString)));
    updateLineEditStyleSheet();
    lyr->addWidget(m_lineedit);
    const bool align_right = displayedField()->isNumericType();
    if (align_right) {
        m_lineedit->setAlignment(Qt::AlignRight);
    }

    setViewWidget(m_lineedit);
    m_calculatedCell = false;

//! @todo
#if 0
    connect(m_cview->completionBox(), SIGNAL(activated(QString)),
            this, SLOT(completed(QString)));
    connect(m_cview->completionBox(), SIGNAL(highlighted(QString)),
            this, SLOT(completed(QString)));
    m_cview->completionBox()->setTabHandling(true);
#endif
}

void KexiInputTableEdit::setValueInternal(const QVariant& add, bool removeOld)
{
    bool lengthExceeded;
    QString text(m_textFormatter.toString(removeOld ? QVariant() : KexiDataItemInterface::originalValue(), add.toString(),
                                          &lengthExceeded));
    if (text.isEmpty()) {
        if (KexiDataItemInterface::originalValue().toString().isEmpty()) {
            //we have to set NULL initial value:
            m_lineedit->setText(QString());
        }
    } else {
        m_lineedit->setText(text);
    }

//! @todo by default we're moving to the end of editor, ADD OPTION allowing "select all chars"
#if 0
    m_cview->selectAll();
#else
    m_lineedit->end(false);
#endif
    if (!m_lineedit->validator()) {
        QValidator *validator = new KDbFieldValidator(*field(), m_lineedit);
        validator->setObjectName("KexiInputTableEdit-validator");
        m_lineedit->setValidator(validator);
    }
    emitLengthExceededIfNeeded(lengthExceeded);
}

void KexiInputTableEdit::paintEvent(QPaintEvent * /*e*/)
{
    QPainter p(this);
    p.setPen(QPen(palette().text(), 1.0));
    p.drawRect(rect());
}

void
KexiInputTableEdit::setRestrictedCompletion()
{
//! @todo
#if 0
    if (m_cview->text().isEmpty())
        return;

    qDebug() << "something to do";
    m_cview->useGlobalKeyBindings();

    QStringList newC;
    QStringList::ConstIterator it, end(m_comp.constEnd());
    for (it = m_comp.constBegin(); it != end; ++it) {
        if ((*it).startsWith(m_cview->text()))
            newC.append(*it);
    }
    m_cview->setCompletedItems(newC);
#endif
}

void
KexiInputTableEdit::completed(const QString &s)
{
// qDebug() << s;
    m_lineedit->setText(s);
}

bool KexiInputTableEdit::valueChanged()
{
    return KexiTableEdit::valueChanged();
}

bool KexiInputTableEdit::valueIsNull()
{
    return m_lineedit->text().isNull();
}

bool KexiInputTableEdit::valueIsEmpty()
{
    return !m_lineedit->text().isNull() && m_lineedit->text().isEmpty();
}

bool KexiInputTableEdit::valueIsValid()
{
    return m_textFormatter.valueIsValid(m_lineedit->text())
           && m_lineedit->hasAcceptableInput();
}

QVariant KexiInputTableEdit::value()
{
    return m_textFormatter.fromString(m_lineedit->text());
}

void
KexiInputTableEdit::clear()
{
    m_lineedit->clear();
}

bool KexiInputTableEdit::cursorAtStart()
{
    return m_lineedit->cursorPosition() == 0;
}

bool KexiInputTableEdit::cursorAtEnd()
{
    return KexiUtils::cursorAtEnd(m_lineedit);
}

QSize KexiInputTableEdit::totalSize() const
{
    if (!m_lineedit)
        return size();
    return m_lineedit->size();
}

void KexiInputTableEdit::handleCopyAction(const QVariant& value, const QVariant& visibleValue)
{
    Q_UNUSED(visibleValue);
//! @todo handle rich text?
    bool lengthExceeded;
    qApp->clipboard()->setText(m_textFormatter.toString(value, QString(), &lengthExceeded));
}

void KexiInputTableEdit::handleAction(const QString& actionName)
{
    const bool alreadyVisible = m_lineedit->isVisible();

    if (actionName == "edit_paste") {
        if (!alreadyVisible) { //paste as the entire text if the cell was not in edit mode
            emit editRequested();
            m_lineedit->clear();
        }
        m_lineedit->paste();
    } else if (actionName == "edit_cut") {
//! @todo handle rich text?
        if (!alreadyVisible) { //cut the entire text if the cell was not in edit mode
            emit editRequested();
            m_lineedit->selectAll();
        }
        m_lineedit->cut();
    }
}

bool KexiInputTableEdit::showToolTipIfNeeded(const QVariant& value, const QRect& rect,
        const QFontMetrics& fm, bool focused)
{
    bool lengthExceeded;
    QString text(value.type() == QVariant::String
        ? value.toString() : m_textFormatter.toString(value, QString(), &lengthExceeded));

    QRect internalRect(rect);
    internalRect.setLeft(rect.x() + leftMargin());
    internalRect.setWidth(internalRect.width() - rightMargin(focused) - 2*3);
    //qDebug() << rect << internalRect << fm.width(text);
    return fm.width(text) > internalRect.width();
}

void KexiInputTableEdit::moveCursorToEnd()
{
    m_lineedit->end(false/*!mark*/);
}

void KexiInputTableEdit::moveCursorToStart()
{
    m_lineedit->home(false/*!mark*/);
}

void KexiInputTableEdit::selectAll()
{
    m_lineedit->selectAll();
}

void KexiInputTableEdit::slotTextEdited(const QString& text)
{
    signalValueChanged();
    bool lengthExceeded = m_textFormatter.lengthExceeded(text);
    emitLengthExceededIfNeeded(lengthExceeded);
}

bool KexiInputTableEdit::fixup()
{
    const QString t(m_lineedit->text());
    bool lengthExceeded = m_textFormatter.lengthExceeded(t);
    if (lengthExceeded) {
        m_lineedit->setText(t.left(field()->maxLength()));
    }
    return true;
}

void KexiInputTableEdit::updateLineEditStyleSheet()
{
    KColorScheme cs(QPalette::Active);
    QColor focus = cs.decoration(KColorScheme::FocusColor).color();
    const bool align_right = displayedField()->isNumericType();
    m_lineedit->setStyleSheet(QString("QLineEdit { \
      border: 1px solid %1; \
      border-radius: 0px; \
      padding: 0px %2px 0px %3px; }")
      .arg(focus.name())
      .arg(m_rightMarginWhenFocused) // right
      .arg(align_right ? 0 : 2) // left
    );
    //qDebug() << m_rightMarginWhenFocused << m_lineedit->styleSheet();
}

KEXI_CELLEDITOR_FACTORY_ITEM_IMPL(KexiInputEditorFactoryItem, KexiInputTableEdit)

#include "kexiinputtableedit.moc"
