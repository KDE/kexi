/* This file is part of the KDE project
   Copyright (C) 2004 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2003-2017 Jarosław Staniek <staniek@kde.org>

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

#include "kexirecordnavigator.h"

#include <core/KexiRecordNavigatorHandler.h>
#include <kexiutils/SmallToolButton.h>
#include <kexiutils/utils.h>

#include <KGuiItem>
#include <KLocalizedString>

#include <QAbstractScrollArea>
#include <QApplication>
#include <QEvent>
#include <QFocusEvent>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPixmap>
#include <QScrollBar>
#include <QStyleOptionFrame>
#include <QWheelEvent>

//! @internal
/*! @warning not reentrant! */
struct KexiRecordNavigatorStatic {
    KexiRecordNavigatorStatic()
     : pen(":/kexi-tableview-pen")
     , plus(":/kexi-tableview-plus")
     , pointer(":/kexi-tableview-pointer")
    {
    }
    static QPixmap replaceColors(const QPixmap &pixmap, const QPalette &palette)
    {
        QColor fc(palette.color(QPalette::Foreground));
        QPixmap p(pixmap);
        KexiUtils::replaceColors(&p, fc);
        return p;
    }
    QPixmap pen, plus, pointer;
};

Q_GLOBAL_STATIC(KexiRecordNavigatorStatic, KexiRecordNavigator_static)

// ----
namespace {
//! A line edit that does paint the top border
class KexiRecordNavigatorRecordNumberEditor : public QLineEdit
{
    Q_OBJECT

public:
    KexiRecordNavigatorRecordNumberEditor(QWidget *parent)
        : QLineEdit(parent)
    {
        // Set transparent base, actual base will be custom-painted in paintEvent()
        QPalette navRecordNumberPalette(palette());
        navRecordNumberPalette.setBrush(QPalette::Base, QBrush(Qt::transparent));
        setPalette(navRecordNumberPalette);
    }

    //! Custom-paint the background: skip the top border
    void paintEvent(QPaintEvent *event) override
    {
        QPainter p(this);
        QStyleOptionFrame panelOption;
        initStyleOption(&panelOption);
        panelOption.palette.setBrush(QPalette::Base, qApp->palette().brush(QPalette::Base));
        panelOption.rect.setY(panelOption.rect.y() + 2 /*(skip the top border)*/);
        style()->drawPrimitive(QStyle::PE_PanelLineEdit, &panelOption, &p, this);
        QLineEdit::paintEvent(event);
    }
};
}

// ----

//! @internal
class Q_DECL_HIDDEN KexiRecordNavigator::Private
{
public:
    Private(KexiRecordNavigator *navigator)
            : q(navigator)
            , handler(0)
            , view(0)
            , editingIndicatorLabel(0)
            , editingIndicatorEnabled(false)
            , editingIndicatorVisible(false)
            , isInsertingEnabled(true)
    {
    }

    //! Update height of buttons and line edits: they are not in the layout
    void updateSizeOfButtonsAndLineEdits()
    {
        // update height of buttons and line edits: they are not in the layout
        const int h = q->height();
        for (QWidget *w : widgetsToResize) {
            w->setFixedHeight(h + 2);
            w->parentWidget()->setFixedHeight(h);
            w->move(0, (w->parentWidget()->height() - w->height()) / 2);
        }
        navRecordNumberParent->setFixedWidth(navRecordNumber->width());
        navRecordCountParent->setFixedWidth(navRecordCount->width());
    }

    KexiRecordNavigator * const q;
    KexiRecordNavigatorHandler *handler;
    QHBoxLayout *lyr;
    QLabel *textLabel;
    QToolButton *navBtnFirst;
    QToolButton *navBtnPrev;
    QToolButton *navBtnNext;
    QToolButton *navBtnLast;
    QToolButton *navBtnNew;
    QWidget *navRecordNumberParent;
    QLineEdit *navRecordNumber;
    QIntValidator *navRecordNumberValidator;
    QWidget *navRecordCountParent;
    QLineEdit *navRecordCount; //!< readonly counter
    int nav1DigitWidth;
    QAbstractScrollArea *view;

    QLabel *editingIndicatorLabel;
    QList<QWidget*> widgetsToResize;
    bool editingIndicatorEnabled;
    bool editingIndicatorVisible;
    bool isInsertingEnabled;
};

//--------------------------------------------------

KexiRecordNavigator::KexiRecordNavigator(QAbstractScrollArea &parentView, QWidget *parent)
        : QWidget(parent)
        , d(new Private(this))
{
    d->view = &parentView;
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    d->view->addScrollBarWidget(this, Qt::AlignLeft);
    d->view->horizontalScrollBar()->installEventFilter(this);

    d->lyr = new QHBoxLayout(this);
    d->lyr->setContentsMargins(0, /*winStyle ? 1 :*/ 0, 0, 0);
    d->lyr->setSpacing(2);

    d->textLabel = new QLabel(this);
    d->lyr->addWidget(d->textLabel);
    setLabelText(xi18n("Record:"));

    setFont(KexiUtils::smallestReadableFont());
    QFontMetrics fm(font());
    d->nav1DigitWidth = fm.width("8");

    d->navBtnFirst = createAction(KexiRecordNavigator::Actions::moveToFirstRecord());
    d->navBtnPrev = createAction(KexiRecordNavigator::Actions::moveToPreviousRecord());
    d->navBtnPrev->setAutoRepeat(true);

    d->lyr->addSpacing(2);

    d->navRecordNumberParent = new QWidget;
    d->navRecordNumberParent->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    d->lyr->addWidget(d->navRecordNumberParent, 0, Qt::AlignVCenter);

    const QString style(this->style()->objectName());
    if (style == "breeze" || style == "oxygen") {
        d->navRecordNumber = new QLineEdit(d->navRecordNumberParent);
    } else {
        d->navRecordNumber = new KexiRecordNavigatorRecordNumberEditor(d->navRecordNumberParent);
    }
    d->widgetsToResize.append(d->navRecordNumber);
    d->navRecordNumber->setContentsMargins(QMargins());
    d->navRecordNumber->setFrame(false);
    d->navRecordNumber->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    d->navRecordNumber->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    d->navRecordNumber->setFocusPolicy(Qt::ClickFocus);
    d->navRecordNumberValidator = new QIntValidator(1, INT_MAX, this);
    d->navRecordNumber->setValidator(d->navRecordNumberValidator);
    d->navRecordNumber->installEventFilter(this);
    d->navRecordNumber->setToolTip(xi18n("Current record number"));

    QLabel *lbl_of = new QLabel(xi18nc("\"of\" in record number information: N of M", "of"), this);
    if (style == "oxygen") {
        lbl_of->setContentsMargins(0, 1, 0, 0);
    }
    lbl_of->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    lbl_of->setFixedWidth(fm.width(lbl_of->text()) + d->nav1DigitWidth);
    lbl_of->setAlignment(Qt::AlignCenter);
    d->lyr->addWidget(lbl_of, 0, Qt::AlignVCenter);

    d->navRecordCountParent = new QWidget;
    d->navRecordCountParent->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    d->lyr->addWidget(d->navRecordCountParent, 0, Qt::AlignVCenter);

    d->navRecordCount = new QLineEdit(d->navRecordCountParent);
    d->widgetsToResize.append(d->navRecordCount);
    d->navRecordCount->setContentsMargins(QMargins());
    d->navRecordCount->setFrame(false);
    d->navRecordCount->setReadOnly(true);
    QPalette navRecordCountPalette(d->navRecordCount->palette());
    navRecordCountPalette.setBrush( QPalette::Base, QBrush(Qt::transparent) );
    d->navRecordCount->setPalette(navRecordCountPalette);
    d->navRecordCount->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    d->navRecordCount->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    d->navRecordCount->setFocusPolicy(Qt::NoFocus);
    d->navRecordCount->setToolTip(xi18n("Number of records"));

    d->navBtnNext = createAction(KexiRecordNavigator::Actions::moveToNextRecord());
    d->navBtnNext->setAutoRepeat(true);
    d->navBtnLast = createAction(KexiRecordNavigator::Actions::moveToLastRecord());

    d->lyr->addSpacing(2);

    d->navBtnNew = createAction(KexiRecordNavigator::Actions::moveToNewRecord());
    d->navBtnNew->setEnabled(isInsertingEnabled());

    d->lyr->addSpacing(2);

    connect(d->navBtnPrev, SIGNAL(clicked()), this, SLOT(slotPrevButtonClicked()));
    connect(d->navBtnNext, SIGNAL(clicked()), this, SLOT(slotNextButtonClicked()));
    connect(d->navBtnLast, SIGNAL(clicked()), this, SLOT(slotLastButtonClicked()));
    connect(d->navBtnFirst, SIGNAL(clicked()), this, SLOT(slotFirstButtonClicked()));
    connect(d->navBtnNew, SIGNAL(clicked()), this, SLOT(slotNewButtonClicked()));

    setRecordCount(0);
    setCurrentRecordNumber(0);
}

KexiRecordNavigator::~KexiRecordNavigator()
{
    delete d;
}

QToolButton* KexiRecordNavigator::createAction(const KGuiItem& item)
{
    // add the button outside of the layout: needed because oxygen, and especially qtcurve draw strange margins
    QWidget *par = new QWidget(this);
    d->lyr->addWidget(par, 0, Qt::AlignVCenter);
    QToolButton *toolButton = new KexiSmallToolButton(item.icon(), par);
    d->widgetsToResize.append(toolButton);
    toolButton->setMinimumWidth(toolButton->sizeHint().width() + 2*3);
    par->setMinimumWidth(toolButton->minimumWidth());
    toolButton->setFocusPolicy(Qt::NoFocus);
    toolButton->setToolTip(item.toolTip());
    toolButton->setWhatsThis(item.whatsThis());
    toolButton->installEventFilter(this);
    return toolButton;
}

void KexiRecordNavigator::setInsertingEnabled(bool set)
{
    if (d->isInsertingEnabled == set)
        return;
    d->isInsertingEnabled = set;
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
    const QEvent::Type t = e->type();
    const QScrollBar *bar;
    if (t == QEvent::Wheel) {
        wheelEvent(static_cast<QWheelEvent*>(e));
        return true;
    } else if (o == d->navRecordNumber) {
        bool recordEntered = false;
        bool ret;
        if (t == QEvent::KeyPress) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(e);
            switch (ke->key()) {
            case Qt::Key_Escape: {
                ke->accept();
                d->navRecordNumber->undo();
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
        } else if (t == QEvent::FocusOut) {
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
            if (hasFocus() || t == QEvent::KeyPress) {
                d->view->setFocus();
            }
            setCurrentRecordNumber(r);
            emit recordNumberEntered(r);
            if (d->handler)
                d->handler->moveToRecordRequested(r - 1);
            return ret;
        }
    } else if ((bar = d->view->horizontalScrollBar()) == o) {
        // Visually hide and deactivate the scrollbar if it's unusable (there are no content to scroll)
        if (bar->value() == 0 && bar->minimum() == 0 && bar->maximum() == 0) {
            QWidget *w = qobject_cast<QWidget*>(o);
            if (t == QEvent::Paint && w->style()->objectName() == QLatin1String("gtk+")) {
                // workaround for GTK+ style: fill the background
                QPainter p(w);
                p.fillRect(w->rect(), w->palette().color(QPalette::Window));
            }
            return true;
        }
    }
    return false;
}

void KexiRecordNavigator::wheelEvent(QWheelEvent* wheelEvent)
{
    const int delta = wheelEvent->delta();

    // trigger the respective button slots
    if (delta > 0) {
        if (d->navBtnPrev->isEnabled()) {
            slotPrevButtonClicked();
        }
    } else if (delta < 0) {
        if (d->navBtnNext->isEnabled()) {
            slotNextButtonClicked();
        }
    }

    // scroll wheel events also cancel the editing,
    // so move focus out of the navRecordNumber
    if (d->navRecordNumber->hasFocus()) {
        if (d->view) {
            d->view->setFocus();
        }
    }
}

void KexiRecordNavigator::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    d->updateSizeOfButtonsAndLineEdits();
}

void KexiRecordNavigator::setCurrentRecordNumber(int r)
{
    int recCnt = recordCount();
    if (r > (recCnt + (d->isInsertingEnabled ? 1 : 0)))
        r = recCnt + (d->isInsertingEnabled ? 1 : 0);
    QString n;
    if (r > 0)
        n = QString::number(r);
    else
        n = " ";

    d->navRecordNumber->setText(n);
    updateButtons(recCnt);
    d->updateSizeOfButtonsAndLineEdits();
}

void KexiRecordNavigator::updateButtons(int recCnt)
{
    const int r = currentRecordNumber();
    if (isEnabled()) {
        d->navBtnPrev->setEnabled(r > 1);
        d->navBtnFirst->setEnabled(r > 1);
        d->navBtnNext->setEnabled(r > 0
                                  && r < (recCnt + (d->isInsertingEnabled ? (1 + (d->editingIndicatorVisible ? 1 : 0)/*if we're editing, next btn is avail.*/) : 0)));
        d->navBtnLast->setEnabled(r != (recCnt + (d->editingIndicatorVisible ? 1 : 0)) && (d->editingIndicatorVisible || recCnt > 0));
    }
}

void KexiRecordNavigator::setRecordCount(int count)
{
    const QString & n = QString::number(count);
    if (d->isInsertingEnabled && currentRecordNumber() == 0) {
        setCurrentRecordNumber(1);
    }
    if (d->navRecordCount->text().length() != n.length()) {//resize
        d->navRecordCount->setFixedWidth(d->nav1DigitWidth * (n.length() + 1));

        if (d->view->horizontalScrollBar()->isVisible()) {
            //+width of the delta
            //resize(width() + (n.length() - d->navRecordCount->text().length())*d->nav1DigitWidth, height());
        }
    }
    //update record number widget's width
    const int w = d->nav1DigitWidth * qMax(qMax(n.length(), 2) + 1, d->navRecordNumber->text().length() + 1) + 2;
    if (d->navRecordNumber->width() != w) //resize
        d->navRecordNumber->setFixedWidth(w);

    d->navRecordCount->setText(n);
    updateButtons(recordCount());
    d->updateSizeOfButtonsAndLineEdits();
}

int KexiRecordNavigator::currentRecordNumber() const
{
    bool ok = true;
    int r = d->navRecordNumber->text().toInt(&ok);
    if (!ok || r < 1)
        r = 0;
    return r;
}

int KexiRecordNavigator::recordCount() const
{
    bool ok = true;
    int r = d->navRecordCount->text().toInt(&ok);
    if (!ok || r < 1)
        r = 0;
    return r;
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
    d->navBtnNew->parentWidget()->setVisible(set);
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
            d->editingIndicatorLabel->setFixedWidth(KexiRecordNavigator::penPixmap(palette()).width() + 2*2);
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
        d->editingIndicatorLabel->setPixmap(KexiRecordNavigator::penPixmap(palette()));
        d->editingIndicatorLabel->setToolTip(xi18n("Editing indicator"));
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
    QStyleOptionFrame option;
    option.initFrom(this);
    option.features = QStyleOptionFrame::Flat;
    option.rect = QRect(option.rect.left() - 5, option.rect.top(),
                        option.rect.width() + 10, option.rect.height() + 5); // to avoid rounding
    style()->drawPrimitive(QStyle::PE_Frame, &option, &p, this);
}

//------------------------------------------------

//! @internal
class KexiRecordNavigatorActionsInternal
{
public:
    KexiRecordNavigatorActionsInternal()
            : moveToFirstRecord(xi18n("First record"), "go-first-view", xi18n("Go to first record"))
            , moveToPreviousRecord(xi18n("Previous record"), "go-previous-view", xi18n("Go to previous record"))
            , moveToNextRecord(xi18n("Next record"), "go-next-view", xi18n("Go to next record"))
            , moveToLastRecord(xi18n("Last record"), "go-last-view", xi18n("Go to last record"))
            , moveToNewRecord(xi18n("New record"), "list-add", xi18n("Go to new record")) {
        moveToFirstRecord.setWhatsThis(xi18n("Moves cursor to first record."));
        moveToPreviousRecord.setWhatsThis(xi18n("Moves cursor to previous record."));
        moveToNextRecord.setWhatsThis(xi18n("Moves cursor to next record."));
        moveToLastRecord.setWhatsThis(xi18n("Moves cursor to last record."));
        moveToNewRecord.setWhatsThis(xi18n("Moves cursor to new record and allows inserting."));
    }
    KGuiItem moveToFirstRecord;
    KGuiItem moveToPreviousRecord;
    KGuiItem moveToNextRecord;
    KGuiItem moveToLastRecord;
    KGuiItem moveToNewRecord;
};

Q_GLOBAL_STATIC(KexiRecordNavigatorActionsInternal, KexiRecordNavigatorActions_internal)

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

//static
QPixmap KexiRecordNavigator::penPixmap(const QPalette &palette)
{
    return KexiRecordNavigatorStatic::replaceColors(
                KexiRecordNavigator_static->pen, palette);
}

//static
QPixmap KexiRecordNavigator::plusPixmap(const QPalette &palette)
{
    return KexiRecordNavigatorStatic::replaceColors(
                KexiRecordNavigator_static->plus, palette);
}

//static
QPixmap KexiRecordNavigator::pointerPixmap(const QPalette &palette)
{
    return KexiRecordNavigatorStatic::replaceColors(
                KexiRecordNavigator_static->pointer, palette);
}

#include "kexirecordnavigator.moc"
