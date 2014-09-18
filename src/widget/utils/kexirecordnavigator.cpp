/* This file is part of the KDE project
   Copyright (C) 2004 Lucijan Busch <lucijan@kde.org>
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

#include <QLabel>
#include <QIntValidator>
#include <QAbstractScrollArea>
#include <Q3ScrollView> //tmp
#include <QScrollBar>
#include <QPixmap>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QEvent>
#include <QHBoxLayout>
#include <QPainter>
#include <QStyleOptionFrameV2>

#include <klocale.h>
#include <klineedit.h>
#include <kguiitem.h>
#include <kdebug.h>

#include "kexirecordnavigator.h"
#include "kexirecordmarker.h"
#include <kexiutils/SmallToolButton.h>
#include <kexiutils/utils.h>
#include <core/KexiRecordNavigatorHandler.h>

//! @internal
class KexiRecordNavigator::Private
{
public:
    Private()
            : handler(0)
            , view(0)
            , editingIndicatorLabel(0)
            , editingIndicatorEnabled(false)
            , editingIndicatorVisible(false)
            , isInsertingEnabled(true)
    {
    }
    KexiRecordNavigatorHandler *handler;
    QHBoxLayout *lyr;
    QLabel *textLabel;
    QToolButton *navBtnFirst;
    QToolButton *navBtnPrev;
    QToolButton *navBtnNext;
    QToolButton *navBtnLast;
    QToolButton *navBtnNew;
    KLineEdit *navRecordNumber;
    QIntValidator *navRecordNumberValidator;
    KLineEdit *navRecordCount; //!< readonly counter
    uint nav1DigitWidth;
    QAbstractScrollArea *view;

    QLabel *editingIndicatorLabel;
    bool editingIndicatorEnabled;
    bool editingIndicatorVisible;
    bool isInsertingEnabled;
};

//--------------------------------------------------

KexiRecordNavigator::KexiRecordNavigator(QWidget *parent, QAbstractScrollArea* parentView)
        : QWidget(parent)
        , d(new Private)
{
    if (!parentView) { //tmp
        setAutoFillBackground(true);
    }
    setFocusPolicy(Qt::NoFocus);
    setParentView(parentView);
    d->lyr = new QHBoxLayout(this);
    d->lyr->setContentsMargins(0, /*winStyle ? 1 :*/ 0, 0, 0);
    d->lyr->setSpacing(2);

    d->textLabel = new QLabel(this);
    d->lyr->addWidget(d->textLabel);
    setLabelText(i18n("Record:"));

    setFont( KexiUtils::smallFont() );
    QFontMetrics fm(font());
    d->nav1DigitWidth = fm.width("8");

    d->navBtnFirst = createAction(KexiRecordNavigator::Actions::moveToFirstRecord());
    d->navBtnPrev = createAction(KexiRecordNavigator::Actions::moveToPreviousRecord());
    d->navBtnPrev->setAutoRepeat(true);

    d->lyr->addSpacing(2);

    d->navRecordNumber = new KLineEdit(this);
    d->lyr->addWidget(d->navRecordNumber, 0, Qt::AlignVCenter);
    KexiUtils::WidgetMargins margins;
    margins.copyToWidget(d->navRecordNumber);
    d->navRecordNumber->setFrame(false);
    if (parentView) {
        kDebug() << parentView->horizontalScrollBar()->height();
        if (parentView) {
            d->navRecordNumber->setFixedHeight( qMax(0 /*parentView->bottomMargin()*/, fm.height() + 2) );
        }
    }
    else { // tmp
        d->navRecordNumber->setFixedHeight( qMax(qobject_cast<Q3ScrollView*>(parent)->bottomMargin(), fm.height() + 2) );
    }
    d->navRecordNumber->setAlignment(Qt::AlignRight | (/*winStyle ? Qt::AlignBottom :*/ Qt::AlignVCenter));
    d->navRecordNumber->setFocusPolicy(Qt::ClickFocus);
    d->navRecordNumberValidator = new QIntValidator(1, INT_MAX, this);
    d->navRecordNumber->setValidator(d->navRecordNumberValidator);
    d->navRecordNumber->installEventFilter(this);
    d->navRecordNumber->setToolTip(i18n("Current record number"));

    QLabel *lbl_of = new QLabel(i18nc("\"of\" in record number information: N of M", "of"), this);
    lbl_of->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    lbl_of->setFixedWidth(fm.width(lbl_of->text()) + d->nav1DigitWidth);
    lbl_of->setAlignment(Qt::AlignCenter);
    d->lyr->addWidget(lbl_of, 0, Qt::AlignVCenter);

    d->navRecordCount = new KLineEdit(this);
    d->lyr->addWidget(d->navRecordCount, 0, Qt::AlignVCenter);
    d->navRecordCount->setFrame(false);
    d->navRecordCount->setReadOnly(true);
    QPalette navRecordCountPalette(d->navRecordCount->palette());
    navRecordCountPalette.setBrush( QPalette::Base, QBrush(Qt::transparent) );
    d->navRecordCount->setPalette(navRecordCountPalette);
    if (parentView) {
        d->navRecordCount->setFixedHeight( qMax(0/*parentView->bottomMargin()*/, fm.height() + 2) );
    }
    else { // tmp
        d->navRecordCount->setFixedHeight( qMax(qobject_cast<Q3ScrollView*>(parent)->bottomMargin(), fm.height() + 2) );
    }
    d->navRecordCount->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    d->navRecordCount->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    d->navRecordCount->setFocusPolicy(Qt::NoFocus);
    d->navRecordCount->setToolTip(i18n("Number of records"));

    d->navBtnNext = createAction(KexiRecordNavigator::Actions::moveToNextRecord());
    d->navBtnNext->setAutoRepeat(true);
    d->navBtnLast = createAction(KexiRecordNavigator::Actions::moveToLastRecord());

    d->lyr->addSpacing(2);

    d->navBtnNew = createAction(KexiRecordNavigator::Actions::moveToNewRecord());
    d->navBtnNew->setEnabled(isInsertingEnabled());

    d->lyr->addSpacing(6);
    d->lyr->addStretch(10);

    connect(d->navBtnPrev, SIGNAL(clicked()), this, SLOT(slotPrevButtonClicked()));
    connect(d->navBtnNext, SIGNAL(clicked()), this, SLOT(slotNextButtonClicked()));
    connect(d->navBtnLast, SIGNAL(clicked()), this, SLOT(slotLastButtonClicked()));
    connect(d->navBtnFirst, SIGNAL(clicked()), this, SLOT(slotFirstButtonClicked()));
    connect(d->navBtnNew, SIGNAL(clicked()), this, SLOT(slotNewButtonClicked()));

    setRecordCount(0);
    setCurrentRecordNumber(0);
    setLeftMargin(0);
}

KexiRecordNavigator::~KexiRecordNavigator()
{
    delete d;
}

QToolButton* KexiRecordNavigator::createAction(const KGuiItem& item)
{
    QToolButton *toolButton;
    d->lyr->addWidget(toolButton = new KexiSmallToolButton(item.icon(), QString(), this), 0, Qt::AlignVCenter);
    toolButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolButton->setMinimumWidth(toolButton->sizeHint().width() + 2*3);
    toolButton->setFocusPolicy(Qt::NoFocus);
    toolButton->setToolTip(item.toolTip());
    toolButton->setWhatsThis(item.whatsThis());
    return toolButton;
}

void KexiRecordNavigator::setInsertingEnabled(bool set)
{
    if (d->isInsertingEnabled == set)
        return;
    d->isInsertingEnabled = set;
    if (isEnabled())
        d->navBtnNew->setEnabled(d->isInsertingEnabled);
}

void KexiRecordNavigator::setEnabled(bool set)
{
    QWidget::setEnabled(set);
    if (set && !d->isInsertingEnabled)
        d->navBtnNew->setEnabled(false);
}

bool KexiRecordNavigator::eventFilter(QObject *o, QEvent *e)
{
    if (o == d->navRecordNumber) {
        bool recordEntered = false;
        bool ret;
        if (e->type() == QEvent::KeyPress) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(e);
            switch (ke->key()) {
            case Qt::Key_Escape: {
                ke->accept();
                d->navRecordNumber->undo();
                if (d->view)
                    d->view->setFocus();
                return true;
            }
            case Qt::Key_Enter:
            case Qt::Key_Return:
            case Qt::Key_Tab:
            case Qt::Key_Backtab: {
                recordEntered = true;
                ke->accept(); //to avoid pressing Enter later
                ret = true;
            }
            default:;
            }
        } else if (e->type() == QEvent::FocusOut) {
            if (static_cast<QFocusEvent*>(e)->reason() != Qt::TabFocusReason
                    && static_cast<QFocusEvent*>(e)->reason() != Qt::BacktabFocusReason
                    && static_cast<QFocusEvent*>(e)->reason() != Qt::OtherFocusReason)
            {
                recordEntered = true;
            }
            ret = false;
        }

        if (recordEntered) {
            bool ok = true;
            uint r = d->navRecordNumber->text().toUInt(&ok);
            if (!ok || r < 1)
                r = (recordCount() > 0) ? 1 : 0;
            if (d->view && (hasFocus() || e->type() == QEvent::KeyPress))
                d->view->setFocus();
            setCurrentRecordNumber(r);
            emit recordNumberEntered(r);
            if (d->handler)
                d->handler->moveToRecordRequested(r - 1);
            return ret;
        }
    }
    return false;
}

void KexiRecordNavigator::setCurrentRecordNumber(uint r)
{
    uint recCnt = recordCount();
    if (r > (recCnt + (d->isInsertingEnabled ? 1 : 0)))
        r = recCnt + (d->isInsertingEnabled ? 1 : 0);
    QString n;
    if (r > 0)
        n = QString::number(r);
    else
        n = " ";

    d->navRecordNumber->setText(n);
    updateButtons(recCnt);
}

void KexiRecordNavigator::updateButtons(uint recCnt)
{
    const uint r = currentRecordNumber();
    if (isEnabled()) {
        d->navBtnPrev->setEnabled(r > 1);
        d->navBtnFirst->setEnabled(r > 1);
        d->navBtnNext->setEnabled(r > 0
                                  && r < (recCnt + (d->isInsertingEnabled ? (1 + (d->editingIndicatorVisible ? 1 : 0)/*if we're editing, next btn is avail.*/) : 0)));
        d->navBtnLast->setEnabled(r != (recCnt + (d->editingIndicatorVisible ? 1 : 0)) && (d->editingIndicatorVisible || recCnt > 0));
    }
}

void KexiRecordNavigator::setRecordCount(uint count)
{
    const QString & n = QString::number(count);
    if (d->isInsertingEnabled && currentRecordNumber() == 0) {
        setCurrentRecordNumber(1);
    }
    if (d->navRecordCount->text().length() != n.length()) {//resize
        d->navRecordCount->setFixedWidth(d->nav1DigitWidth * (n.length() + 1));

        if (d->view && d->view->horizontalScrollBar()->isVisible()) {
            //+width of the delta
            resize(width() + (n.length() - d->navRecordCount->text().length())*d->nav1DigitWidth, height());
        }
    }
    //update record number widget's width
    const int w = d->nav1DigitWidth * qMax(qMax(n.length(), 2) + 1, d->navRecordNumber->text().length() + 1) + 2;
    if (d->navRecordNumber->width() != w) //resize
        d->navRecordNumber->setFixedWidth(w);

    d->navRecordCount->setText(n);
    if (!d->view) { // tmp
        qobject_cast<Q3ScrollView*>(parentWidget())->updateScrollBars();
    }
    updateButtons(recordCount());
}

uint KexiRecordNavigator::currentRecordNumber() const
{
    bool ok = true;
    int r = d->navRecordNumber->text().toInt(&ok);
    if (!ok || r < 1)
        r = 0;
    return r;
}

uint KexiRecordNavigator::recordCount() const
{
    bool ok = true;
    int r = d->navRecordCount->text().toInt(&ok);
    if (!ok || r < 1)
        r = 0;
    return r;
}

void KexiRecordNavigator::setParentView(QAbstractScrollArea *view)
{
    d->view = view;
}

void KexiRecordNavigator::setLeftMargin(int leftMargin)
{
    QWidget::updateGeometry();
// kDebug() <<"view "<<d->view;
    if (d->view) {
        int navWidth;
        if (d->view->horizontalScrollBar()->isVisible()) {
            navWidth = sizeHint().width();
        } else {
            navWidth = leftMargin + d->view->viewport()->width(); // TODO
        }

        /*  kDebug() << "setGeometry("<<QRect(
              d->view->frameWidth(),
              d->view->height() - d->view->horizontalScrollBar()->sizeHint().height()-d->view->frameWidth(),
              navWidth,
              d->view->horizontalScrollBar()->sizeHint().height())<<")";*/
        setGeometry(
            d->view->frameWidth(),
            d->view->height() - d->view->horizontalScrollBar()->sizeHint().height() - d->view->frameWidth(),
            navWidth,
            d->view->horizontalScrollBar()->sizeHint().height()
        );
    }
    else { // !d->view - tmp
        qobject_cast<Q3ScrollView*>(parentWidget())->updateScrollBars();
        int navWidth;
        if (qobject_cast<Q3ScrollView*>(parentWidget())->horizontalScrollBar()->isVisible()) {
            navWidth = sizeHint().width();
        }
        else {
            navWidth = leftMargin + qobject_cast<Q3ScrollView*>(parentWidget())->clipper()->width();
        }
        setGeometry(
            qobject_cast<Q3ScrollView*>(parentWidget())->frameWidth(),
            qobject_cast<Q3ScrollView*>(parentWidget())->height()
                - qobject_cast<Q3ScrollView*>(parentWidget())->horizontalScrollBar()->sizeHint().height()
                - qobject_cast<Q3ScrollView*>(parentWidget())->frameWidth(),
            navWidth,
            qobject_cast<Q3ScrollView*>(parentWidget())->horizontalScrollBar()->sizeHint().height()
        );
    }
}

void KexiRecordNavigator::setHBarGeometry(QScrollBar & hbar, int x, int y, int w, int h)
{
    hbar.setGeometry(x + width(), y, w - width(), h);
}

void KexiRecordNavigator::setLabelText(const QString& text)
{
    d->textLabel->setText(text.isEmpty() ? QString() : (QString::fromLatin1(" ") + text + " "));
}

void KexiRecordNavigator::setButtonToolTipText(KexiRecordNavigator::Button btn, const QString& txt)
{
    switch (btn) {
    case KexiRecordNavigator::ButtonFirst:
        d->navBtnFirst->setToolTip(txt);
        break;
    case KexiRecordNavigator::ButtonPrevious:
        d->navBtnPrev->setToolTip(txt);
        break;
    case KexiRecordNavigator::ButtonNext:
        d->navBtnNext->setToolTip(txt);
        break;
    case KexiRecordNavigator::ButtonLast:
        d->navBtnLast->setToolTip(txt);
        break;
    case KexiRecordNavigator::ButtonNew:
        d->navBtnNew->setToolTip(txt);
        break;
    }
}

void KexiRecordNavigator::setButtonWhatsThisText(KexiRecordNavigator::Button btn, const QString& txt)
{
    switch (btn) {
    case KexiRecordNavigator::ButtonFirst:
        d->navBtnFirst->setWhatsThis(txt);
        break;
    case KexiRecordNavigator::ButtonPrevious:
        d->navBtnPrev->setWhatsThis(txt);
        break;
    case KexiRecordNavigator::ButtonNext:
        d->navBtnNext->setWhatsThis(txt);
        break;
    case KexiRecordNavigator::ButtonLast:
        d->navBtnLast->setWhatsThis(txt);
        break;
    case KexiRecordNavigator::ButtonNew:
        d->navBtnNew->setWhatsThis(txt);
        break;
    }
}

void KexiRecordNavigator::setNumberFieldToolTips(const QString& numberTooltip, const QString& countTooltip)
{
    d->navRecordNumber->setToolTip(numberTooltip);
    d->navRecordCount->setToolTip(countTooltip);
}

void KexiRecordNavigator::setInsertingButtonVisible(bool set)
{
    d->navBtnNew->setVisible(set);
}

void KexiRecordNavigator::slotPrevButtonClicked()
{
    emit prevButtonClicked();
    if (d->handler)
        d->handler->moveToPreviousRecordRequested();
}

void KexiRecordNavigator::slotNextButtonClicked()
{
    emit nextButtonClicked();
    if (d->handler)
        d->handler->moveToNextRecordRequested();
}

void KexiRecordNavigator::slotLastButtonClicked()
{
    emit lastButtonClicked();
    if (d->handler)
        d->handler->moveToLastRecordRequested();
}

void KexiRecordNavigator::slotFirstButtonClicked()
{
    emit firstButtonClicked();
    if (d->handler)
        d->handler->moveToFirstRecordRequested();
}

void KexiRecordNavigator::slotNewButtonClicked()
{
    emit newButtonClicked();
    if (d->handler)
        d->handler->addNewRecordRequested();
}

void KexiRecordNavigator::setRecordHandler(KexiRecordNavigatorHandler *handler)
{
    d->handler = handler;
}

bool KexiRecordNavigator::isInsertingEnabled() const
{
    return d->isInsertingEnabled;
}

bool KexiRecordNavigator::editingIndicatorVisible() const
{
    return d->editingIndicatorVisible;
}

bool KexiRecordNavigator::editingIndicatorEnabled() const
{
    return d->editingIndicatorEnabled;
}

void KexiRecordNavigator::setEditingIndicatorEnabled(bool set)
{
    d->editingIndicatorEnabled = set;
    if (d->editingIndicatorEnabled) {
        if (!d->editingIndicatorLabel) {
            d->editingIndicatorLabel = new QLabel(this);
            d->editingIndicatorLabel->setAlignment(Qt::AlignCenter);
            QPixmap pix( *KexiRecordMarker::penPixmap() );
            d->editingIndicatorLabel->setFixedWidth(pix.width() + 2*2);
            d->lyr->insertWidget(0, d->editingIndicatorLabel);
        }
        d->editingIndicatorLabel->show();
    } else {
        if (d->editingIndicatorLabel) {
            d->editingIndicatorLabel->hide();
        }
    }
}

void KexiRecordNavigator::showEditingIndicator(bool show)
{
    d->editingIndicatorVisible = show;
    updateButtons(recordCount()); //this will refresh 'next btn'
    if (!d->editingIndicatorEnabled)
        return;
    if (d->editingIndicatorVisible) {
        QPixmap pix( *KexiRecordMarker::penPixmap() );
        d->editingIndicatorLabel->setPixmap(pix);
        d->editingIndicatorLabel->setToolTip(i18n("Editing indicator"));
    } else {
        d->editingIndicatorLabel->setPixmap(QPixmap());
        d->editingIndicatorLabel->setToolTip(QString());
    }
}

void KexiRecordNavigator::paintEvent(QPaintEvent* pe)
{
    QWidget::paintEvent(pe);
    QPainter p(this);
    // add frame on top
    QStyleOptionFrameV2 option;
    option.initFrom(this);
    option.features = QStyleOptionFrameV2::Flat;
    option.rect = QRect(option.rect.left() - 5, option.rect.top(),
                        option.rect.width() + 10, option.rect.height() + 5); // to avoid rounding
    style()->drawPrimitive(QStyle::PE_Frame, &option, &p, this);
}

void KexiRecordNavigator::insertAsideOfHorizontalScrollBar(QAbstractScrollArea *area)
{
    if (parentWidget() != area->horizontalScrollBar()->parentWidget()) {
        setParent(area->horizontalScrollBar()->parentWidget());
        qobject_cast<QBoxLayout*>(area->horizontalScrollBar()->parentWidget()->layout())
                ->insertWidget(0, this);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    }
}

//------------------------------------------------

//! @internal
class KexiRecordNavigatorActionsInternal
{
public:
    KexiRecordNavigatorActionsInternal()
            : moveToFirstRecord(i18n("First record"), "go-first-view", i18n("Go to first record"))
            , moveToPreviousRecord(i18n("Previous record"), "go-previous-view", i18n("Go to previous record"))
            , moveToNextRecord(i18n("Next record"), "go-next-view", i18n("Go to next record"))
            , moveToLastRecord(i18n("Last record"), "go-last-view", i18n("Go to last record"))
            , moveToNewRecord(i18n("New record"), "list-add", i18n("Go to new record")) {
        moveToFirstRecord.setWhatsThis(i18n("Moves cursor to first record."));
        moveToPreviousRecord.setWhatsThis(i18n("Moves cursor to previous record."));
        moveToNextRecord.setWhatsThis(i18n("Moves cursor to next record."));
        moveToLastRecord.setWhatsThis(i18n("Moves cursor to last record."));
        moveToNewRecord.setWhatsThis(i18n("Moves cursor to new record and allows inserting."));
    }
    KGuiItem moveToFirstRecord;
    KGuiItem moveToPreviousRecord;
    KGuiItem moveToNextRecord;
    KGuiItem moveToLastRecord;
    KGuiItem moveToNewRecord;
};

K_GLOBAL_STATIC(KexiRecordNavigatorActionsInternal, KexiRecordNavigatorActions_internal)

const KGuiItem& KexiRecordNavigator::Actions::moveToFirstRecord()
{
    return KexiRecordNavigatorActions_internal->moveToFirstRecord;
}

const KGuiItem& KexiRecordNavigator::Actions::moveToPreviousRecord()
{
    return KexiRecordNavigatorActions_internal->moveToPreviousRecord;
}

const KGuiItem& KexiRecordNavigator::Actions::moveToNextRecord()
{
    return KexiRecordNavigatorActions_internal->moveToNextRecord;
}

const KGuiItem& KexiRecordNavigator::Actions::moveToLastRecord()
{
    return KexiRecordNavigatorActions_internal->moveToLastRecord;
}

const KGuiItem& KexiRecordNavigator::Actions::moveToNewRecord()
{
    return KexiRecordNavigatorActions_internal->moveToNewRecord;
}

#include "kexirecordnavigator.moc"
