/* This file is part of the KDE project
   Copyright (C) 2006-2014 Jarosław Staniek <staniek@kde.org>

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

#include "kexidbcombobox.h"
#include "kexidblineedit.h"
#include "kexiformscrollview.h"
#include <widget/tableview/kexicomboboxpopup.h>
#include <widget/tableview/kexicelleditorfactory.h>
#include <kexiutils/utils.h>

#include <KDbQuerySchema>

#include <KComboBox>

#include <QDebug>
#include <QApplication>
#include <QPainter>
#include <QStyle>
#include <qdrawutil.h>
#include <QCursor>
#include <QList>

//! @internal
class KexiDBComboBox::Private
{
public:
    Private()
            : popup(0)
            , visibleColumnInfo(0)
            , isEditable(false)
            , buttonPressed(false)
            , mouseOver(false)
            , dataEnteredByHand(true) {
    }
    ~Private() {
    }

    KexiComboBoxPopup *popup;
    KComboBox *paintedCombo; //!< fake combo used only to pass it as 'this' for QStyle (because styles use <static_cast>)
    QSize sizeHint; //!< A cache for KexiDBComboBox::sizeHint(),
    //!< rebuilt by KexiDBComboBox::fontChange() and KexiDBComboBox::styleChange()
    KDbQueryColumnInfo* visibleColumnInfo;
    //! used for collecting subwidgets and their childrens (if isEditable is false)
    QList<QWidget*> subWidgetsWithDisabledEvents;
    bool isEditable; //!< true is the combo box is editable
    bool buttonPressed;
    bool mouseOver;
    bool dataEnteredByHand;
};

//-------------------------------------

KexiDBComboBox::KexiDBComboBox(QWidget *parent)
        : KexiDBAutoField(parent, NoLabel)
        , KexiComboBoxBase()
        , d(new Private())
{
//! @todo fix creating popup for forms instead; remove KexiComboBoxBase::m_setReinstantiatePopupOnShow
    m_reinstantiatePopupOnShow = true;
    m_focusPopupBeforeShow = true;

    setMouseTracking(true);
    setFocusPolicy(Qt::WheelFocus);
    installEventFilter(this);
    d->paintedCombo = new KComboBox(this);
    d->paintedCombo->hide();
    d->paintedCombo->move(0, 0);
}

KexiDBComboBox::~KexiDBComboBox()
{
    delete d;
}

KDbTableViewColumn* KexiDBComboBox::column() const
{
    return 0;
}

KDbField* KexiDBComboBox::field() const
{
    return KexiDBAutoField::field();
}

QVariant KexiDBComboBox::origValue() const
{
    return KexiDataItemInterface::originalValue();
}

QVariant KexiDBComboBox::value()
{
    return KexiComboBoxBase::value();
}

KexiComboBoxPopup *KexiDBComboBox::popup() const
{
    return d->popup;
}

void KexiDBComboBox::setPopup(KexiComboBoxPopup *popup)
{
    d->popup = popup;
    if (popup) {
        connect(popup, SIGNAL(hidden()), this, SLOT(slotPopupHidden()));
    }
}

void KexiDBComboBox::slotPopupHidden()
{
    moveCursorToEnd();
    selectAll();
}

void KexiDBComboBox::setEditable(bool set)
{
    if (d->isEditable == set)
        return;
    d->isEditable = set;
    d->paintedCombo->setEditable(set);
    if (set)
        createEditor();
    else {
        delete subwidget();
        setSubwidget(0);
    }
    update();
}

bool KexiDBComboBox::isEditable() const
{
    return d->isEditable;
}

void KexiDBComboBox::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setPen(palette().color(QPalette::Text));
    QPalette pal(palette());
    pal.setColor(QPalette::Base, paletteBackgroundColor()); //update base color using (reimplemented) bg color

    if (width() < 5 || height() < 5) {
        qDrawShadePanel(&p, rect(), pal, false /* !sunken */,
                        2 /*line width*/, &pal.brush(QPalette::Button)/*fill*/);
        return;
    }

    //! @todo
    QStyleOptionComboBox option;
    option.palette = pal;
    option.initFrom(d->paintedCombo);

    if (isEnabled())
        option.state |= QStyle::State_Enabled;
    if (hasFocus())
        option.state |= QStyle::State_HasFocus;
    if (d->mouseOver)
        option.state |= QStyle::State_MouseOver;

    style()->drawComplexControl(QStyle::CC_ComboBox, &option, &p, d->paintedCombo);

    //! @todo support reverse layout
#if 0 //TODO
//bool reverse = QApplication::reverseLayout();
    style()->drawComplexControl(QStyle::CC_ComboBox, &option, &p, d->paintedCombo  /*this*/
                                flags, (uint)QStyle::SC_All,
                                (d->buttonPressed ? QStyle::SC_ComboBoxArrow : QStyle::SC_None)
                               );

    if (d->isEditable) {
        //if editable, editor paints itself, nothing to do
    } else { //not editable: we need to paint the current item
        QRect editorGeometry(this->editorGeometry());
        if (hasFocus()) {
            if (0 == qstrcmp(style()->name(), "windows")) //a hack
                p.fillRect(editorGeometry, palette().brush(QPalette::Highlight));
            QRect r(QStyle::visualRect(style()->subRect(QStyle::SR_ComboBoxFocusRect, d->paintedCombo), this));
            r = QRect(r.left() - 1, r.top() - 1, r.width() + 2, r.height() + 2); //enlare by 1 pixel each side to avoid covering by the subwidget
            style()->drawPrimitive(QStyle::PE_FocusRect, &p,
                                   r, cg, flags | QStyle::Style_FocusAtBorder, QStyleOption(cg.highlight()));
        }
        //todo
    }
#endif
}

QRect KexiDBComboBox::editorGeometry() const
{
//! @todo 20080316, sebsauer; crashes here with;
#if 0
    QRect r(QStyle::visualRect(
                qApp->layoutDirection(),
                d->paintedCombo->geometry(),
                style()->subControlRect(QStyle::CC_ComboBox, 0, QStyle::SC_ComboBoxEditField, d->paintedCombo)));
#else
    QRect r = d->paintedCombo->geometry();
    r.setSize(size());
#endif
    return r;
}

void KexiDBComboBox::createEditor()
{
    KexiDBAutoField::createEditor();
    if (subwidget()) {
        subwidget()->setGeometry(editorGeometry());
        if (!d->isEditable) {
            QStyleOptionComboBox option;
            option.initFrom(subwidget());
            const QRect comboRect = subwidget()->style()->subControlRect(
                QStyle::CC_ComboBox, &option, QStyle::SC_ComboBoxEditField, subwidget());
            //qDebug() << "comboRect:" << comboRect;
            subwidget()->setContentsMargins(comboRect.left(), comboRect.top(),
                width() - comboRect.right(), height() - comboRect.bottom());
            int l, t, r, b;
            subwidget()->getContentsMargins(&l, &t, &r, &b);
            //qDebug() << "altered margins:" << l << t << r << b;

            subwidget()->setFocusPolicy(Qt::NoFocus);
            setFocusProxy(0); // Subwidget is not focusable but the form requires focusable
                              // widget in order to manage data updates so let it be this KexiDBComboBox.
            subwidget()->setCursor(QCursor(Qt::ArrowCursor)); // widgets like listedit have IbeamCursor, we don't want that
            QPalette subwidgetPalette(subwidget()->palette());
            subwidgetPalette.setColor(QPalette::Base, Qt::transparent);
            subwidget()->setPalette(subwidgetPalette);
            d->subWidgetsWithDisabledEvents.clear();
            d->subWidgetsWithDisabledEvents << subwidget();
            if (!designMode()) {
                subwidget()->installEventFilter(this);
            }
            QList<QWidget*> widgets(subwidget()->findChildren<QWidget*>());
            foreach(QWidget *widget, widgets) {
                d->subWidgetsWithDisabledEvents << widget;
                widget->installEventFilter(this);
            }
        }
    }
    updateGeometry();
}

void KexiDBComboBox::setLabelPosition(LabelPosition position)
{
    if (subwidget()) {
        if (-1 != subwidget()->metaObject()->indexOfProperty("frameShape")) {
            subwidget()->setProperty("frameShape", QVariant((int)QFrame::NoFrame));
        }
        subwidget()->setGeometry(editorGeometry());
    }
    // update size policy
    QSizePolicy sizePolicy(this->sizePolicy());
    if (position == Left)
        sizePolicy.setHorizontalPolicy(QSizePolicy::Minimum);
    else
        sizePolicy.setVerticalPolicy(QSizePolicy::Minimum);
    setSizePolicy(sizePolicy);
}

QRect KexiDBComboBox::buttonGeometry() const
{
    QRect arrowRect(
        style()->subControlRect(
            QStyle::CC_ComboBox, 0, QStyle::SC_ComboBoxArrow, d->paintedCombo));
    //! @todo ok?
    arrowRect = QStyle::visualRect(
                    qApp->layoutDirection(), d->paintedCombo->geometry(), arrowRect);
    arrowRect.setHeight(qMax(height() - (2 * arrowRect.y()), arrowRect.height()));      // a fix for Motif style
    return arrowRect;
}

bool KexiDBComboBox::handleMousePressEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton || designMode())
        return true;

    if (!isEditable() || buttonGeometry().contains(e->pos())) {
        d->buttonPressed = false;
        showPopup();
        return true;
    }
    return false;
}

bool KexiDBComboBox::handleKeyPressEvent(QKeyEvent *ke)
{
    const int k = ke->key();
    const bool dropDown = (ke->modifiers() == Qt::NoModifier && ((k == Qt::Key_F2 && !d->isEditable) || k == Qt::Key_F4))
                          || (ke->modifiers() == Qt::AltModifier && k == Qt::Key_Down);
    const bool escPressed = ke->modifiers() == Qt::NoModifier && k == Qt::Key_Escape;
    const bool popupVisible =  popup() && popup()->isVisible();
    if ((dropDown || escPressed) && popupVisible) {
        popup()->hide();
        return true;
    } else if (dropDown && !popupVisible) {
        d->buttonPressed = false;
        showPopup();
        return true;
    } else if (popupVisible) {
        const bool enterPressed = k == Qt::Key_Enter || k == Qt::Key_Return;
        if (enterPressed/* && m_internalEditorValueChanged*/) {
            acceptPopupSelection();
            return true;
        }
        return handleKeyPressForPopup(ke);
    }

    return false;
}

bool KexiDBComboBox::keyPressed(QKeyEvent *ke)
{
    if (KexiDBAutoField::keyPressed(ke))
        return true;

    const int k = ke->key();
    const bool popupVisible =  popup() && popup()->isVisible();
    const bool escPressed = ke->modifiers() == Qt::NoModifier && k == Qt::Key_Escape;
    if (escPressed && popupVisible) {
        popup()->hide();
        return true;
    }
    if (ke->modifiers() == Qt::NoModifier
            && (k == Qt::Key_PageDown || k == Qt::Key_PageUp)
            && popupVisible) {
        return true;
    }
    return false;
}

void KexiDBComboBox::mousePressEvent(QMouseEvent *e)
{
    if (handleMousePressEvent(e))
        return;

    KexiDBAutoField::mousePressEvent(e);
}

void KexiDBComboBox::mouseDoubleClickEvent(QMouseEvent *e)
{
    mousePressEvent(e);
}

bool KexiDBComboBox::eventFilter(QObject *o, QEvent *e)
{
#if 0
    if (e->type() != QEvent::Paint
            && e->type() != QEvent::Leave
            && e->type() != QEvent::MouseMove
            && e->type() != QEvent::HoverMove
            && e->type() != QEvent::HoverEnter
            && e->type() != QEvent::HoverLeave)
    {
        qDebug() << e << o << subwidget();
        qDebug() << "FOCUS WIDGET:" << focusWidget();
    }
#endif
    if (o == this || o == popup() || o == subwidget()) {
        if (e->type() == QEvent::KeyPress) {
            // handle F2/F4
            if (handleKeyPressEvent(static_cast<QKeyEvent*>(e)))
                return true;
        }
    }
    if (o == this) {
        if (e->type() == QEvent::Resize) {
            d->paintedCombo->resize(size());
            if (subwidget())
                subwidget()->setGeometry(editorGeometry());
        } else if (e->type() == QEvent::Enter) {
            if (!d->isEditable
                    || /*over button if editable combo*/buttonGeometry().contains(static_cast<QMouseEvent*>(e)->pos())) {
                d->mouseOver = true;
                update();
            }
        } else if (e->type() == QEvent::MouseMove) {
            if (d->isEditable) {
                const bool overButton = buttonGeometry().contains(static_cast<QMouseEvent*>(e)->pos());
                if (overButton != d->mouseOver) {
                    d->mouseOver = overButton;
                    update();
                }
            }
        } else if (e->type() == QEvent::Leave) {
            d->mouseOver = false;
            update();
        } else if (e->type() == QEvent::FocusOut || e->type() == QEvent::Hide) {
            if (!d->isEditable) {
                moveCursorToEnd();
            }
            if (popup()) {
                popup()->hide();
            }
            if (popup() && popup()->isVisible()) {
                undoChanges();
            }
            return true;
        }
    }
    if (!d->isEditable && d->subWidgetsWithDisabledEvents.contains(dynamic_cast<QWidget*>(o))) {
        //qDebug() << "**********************####" << e->type() << o;
        if (e->type() == QEvent::MouseButtonPress) {
            // clicking the subwidget should mean the same as clicking the combo box (i.e. show the popup)
            if (handleMousePressEvent(static_cast<QMouseEvent*>(e)))
                return true;
        } else if (e->type() == QEvent::KeyPress) {
            if (handleKeyPressEvent(static_cast<QKeyEvent*>(e)))
                return true;
        }
        if (e->type() != QEvent::Paint)
            return true;
    }
    return KexiDBAutoField::eventFilter(o, e);
}

bool KexiDBComboBox::subwidgetStretchRequired(KexiDBAutoField* autoField) const
{
    Q_UNUSED(autoField);
    return true;
}

QWidget* KexiDBComboBox::internalEditor() const
{
    return /*WidgetWithSubpropertiesInterface*/subwidget();
}

void KexiDBComboBox::setPaletteBackgroundColor(const QColor & color)
{
    KexiDBAutoField::setPaletteBackgroundColor(color);
    update();
}

bool KexiDBComboBox::valueChanged()
{
    //qDebug() << KexiDataItemInterface::originalValue().toString() << " ? " << value().toString();
    return KexiDataItemInterface::originalValue() != value();
}

void
KexiDBComboBox::setColumnInfo(KDbQueryColumnInfo* cinfo)
{
    KexiFormDataItemInterface::setColumnInfo(cinfo);
}

void KexiDBComboBox::setVisibleColumnInfo(KDbQueryColumnInfo* cinfo)
{
    d->visibleColumnInfo = cinfo;
    // we're assuming we already have columnInfo()
    setColumnInfoInternal(columnInfo(), d->visibleColumnInfo);
}

KDbQueryColumnInfo* KexiDBComboBox::visibleColumnInfo() const
{
    return d->visibleColumnInfo;
}

QColor KexiDBComboBox::paletteBackgroundColor() const
{
    return KexiDBAutoField::paletteBackgroundColor();
}

void KexiDBComboBox::moveCursorToEndInInternalEditor()
{
    if (m_moveCursorToEndInInternalEditor_enabled)
        moveCursorToEnd();
}

void KexiDBComboBox::selectAllInInternalEditor()
{
    if (m_selectAllInInternalEditor_enabled)
        selectAll();
}

void KexiDBComboBox::setValueInternal(const QVariant& add, bool removeOld)
{
    // use KexiDBAutoField instead of KexiComboBoxBase::setValueInternal
    // expects existing popup(), but we want to have delayed creation
    if (popup())
        popup()->hide();
    KexiComboBoxBase::setValueInternal(add, removeOld);
}

void KexiDBComboBox::setVisibleValueInternal(const QVariant& value)
{
    KexiFormDataItemInterface *iface = dynamic_cast<KexiFormDataItemInterface*>((QWidget*)subwidget());
    if (iface)
        iface->setValue(value, QVariant(), false /*!removeOld*/);
}

QVariant KexiDBComboBox::visibleValue()
{
    return KexiComboBoxBase::visibleValue();
}

void KexiDBComboBox::setValueInInternalEditor(const QVariant& value)
{
    if (!m_setValueInInternalEditor_enabled)
        return;
    KexiFormDataItemInterface *iface = dynamic_cast<KexiFormDataItemInterface*>((QWidget*)subwidget());
    if (iface)
        iface->setValue(value, QVariant(), false/*!removeOld*/);
}

QVariant KexiDBComboBox::valueFromInternalEditor()
{
    return KexiDBAutoField::value();
}

QPoint KexiDBComboBox::mapFromParentToGlobal(const QPoint& pos) const
{
    if (!parentWidget())
        return QPoint(-1, -1);
    return parentWidget()->mapToGlobal(pos);
}

int KexiDBComboBox::popupWidthHint() const
{
    return width();
}

void KexiDBComboBox::fontChange(const QFont & oldFont)
{
    d->sizeHint = QSize(); //force rebuild the cache
    KexiDBAutoField::fontChange(oldFont);
}

void KexiDBComboBox::styleChange(QStyle& oldStyle)
{
    KexiDBAutoField::styleChange(oldStyle);
    d->sizeHint = QSize(); //force rebuild the cache
    if (subwidget())
        subwidget()->setGeometry(editorGeometry());
}

QSize KexiDBComboBox::sizeHint() const
{
    if (isVisible() && d->sizeHint.isValid())
        return d->sizeHint;

    const int maxWidth = 7 * fontMetrics().width(QChar('x')) + 18;
    const int maxHeight = qMax(fontMetrics().lineSpacing(), 14) + 2;
    QStyleOptionComboBox option;
    option.initFrom(d->paintedCombo);
    d->sizeHint = (style()->sizeFromContents(
                       QStyle::CT_ComboBox, &option, QSize(maxWidth, maxHeight), d->paintedCombo).expandedTo(QApplication::globalStrut()));

    return d->sizeHint;
}

void KexiDBComboBox::editRequested()
{
}

void KexiDBComboBox::acceptRequested()
{
    signalValueChanged();
}

void KexiDBComboBox::slotRowAccepted(KDbRecordData *record, int row)
{
    d->dataEnteredByHand = false;
    KexiComboBoxBase::slotRowAccepted(record, row);
    d->dataEnteredByHand = true;
}

void KexiDBComboBox::slotItemSelected(KDbRecordData *record)
{
    KexiComboBoxBase::slotItemSelected(record);
}

void KexiDBComboBox::slotInternalEditorValueChanged(const QVariant& v)
{
    KexiComboBoxBase::slotInternalEditorValueChanged(v);
}

void KexiDBComboBox::beforeSignalValueChanged()
{
    if (d->dataEnteredByHand) {
        KexiFormDataItemInterface *iface = dynamic_cast<KexiFormDataItemInterface*>((QWidget*)subwidget());
        if (iface) {
            slotInternalEditorValueChanged(iface->value());
        }
    }
}

void KexiDBComboBox::undoChanges()
{
    KexiDBAutoField::undoChanges();
    KexiComboBoxBase::undoChanges();
}

