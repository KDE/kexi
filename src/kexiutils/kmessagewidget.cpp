/* This file is part of the KDE libraries
 *
 * Copyright (c) 2011 Aurélien Gâteau <agateau@kde.org>
 * Copyright (C) 2011-2013 Jarosław Staniek <staniek@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */
#include "kmessagewidget.h"
#include "kmessagewidget_p.h"

#include "KexiCloseButton.h"
#include "KexiIcon.h"
#include <kexiutils/utils.h>

#include <KColorScheme>
#include <KStandardAction>
#include <KStandardGuiItem>

#include <QDebug>
#include <QEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QShowEvent>
#include <QTimeLine>
#include <QToolButton>
#include <QPointer>
#include <QPainterPath>
#include <QTimer>
#include <QAction>

static const int LAYOUT_SPACING = 6;

ClickableLabel::ClickableLabel(QWidget *parent)
 : QLabel(parent)
{
}

ClickableLabel::~ClickableLabel()
{
}

void ClickableLabel::mousePressEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton) {
        emit clicked();
    }
    QLabel::mousePressEvent(ev);
}

//---------------------------------------------------------------------
// KMessageWidgetFrame
//---------------------------------------------------------------------

KMessageWidgetFrame::KMessageWidgetFrame(QWidget* parent)
 : QFrame(parent), radius(7),
   m_calloutPointerDirection(KMessageWidget::NoPointer),
   m_sizeForRecentTransformation(-1, -1),
   m_calloutPointerGlobalPosition(-QWIDGETSIZE_MAX, -QWIDGETSIZE_MAX)
{
    const qreal rad = radius;
    m_polyline << QPointF(0, 0)
               << QPointF(0, rad * 2.0) //<< QPointF(rad, rad * 2.0 - 0.5)
               << QPointF(rad * 2.0, 0);
    m_polygon << QPointF(m_polyline[0].x(), m_polyline[0].y() - 1)
              << QPointF(m_polyline[1].x(), m_polyline[1].y() - 1)
              << QPointF(m_polyline[2].x(), m_polyline[2].y() - 1);
}

void KMessageWidgetFrame::paintEvent(QPaintEvent* event)
{
    QFrame::paintEvent(event);
    paintCalloutPointer();
}

KMessageWidget::CalloutPointerDirection KMessageWidgetFrame::calloutPointerDirection() const
{
    return m_calloutPointerDirection;
}

void KMessageWidgetFrame::setCalloutPointerDirection(
    KMessageWidget::CalloutPointerDirection direction)
{
    m_calloutPointerDirection = direction;
    m_sizeForRecentTransformation = QSize(-1, -1);
}

void KMessageWidgetFrame::updateCalloutPointerTransformation() const
{
    if (m_sizeForRecentTransformation == parentWidget()->size())
        return;

    m_calloutPointerTransformation.reset();

    const QSizeF s(parentWidget()->size());
    m_sizeForRecentTransformation = parentWidget()->size();
    // qDebug() << size() << parentWidget()->size();
    const qreal rad = radius;
    // Original: [v    ]
    //           [     ]
    switch (m_calloutPointerDirection) {
    case KMessageWidget::Up:
        //  ^
        // [    ]
        m_calloutPointerTransformation
            .rotate(180.0)
            .translate(- rad * 5.0 + 0.5, - rad * 2 - 0.5)
            .scale(-1.0, 1.0);
        break;
    case KMessageWidget::Down:
        // [    ]
        //  v
        // No rotation needed, this is original position of polyline below
        m_calloutPointerTransformation
            .translate(rad * 3.0 + 0.5, s.height() - rad * 2);
        break;
    case KMessageWidget::Left:
        // <[     ]
        //  [     ]
        m_calloutPointerTransformation
            .rotate(90.0)
            .translate(rad * 1.5, - rad * 2 - 3.5);
        break;
    case KMessageWidget::Right:
        // [     ]>
        // [     ]
        m_calloutPointerTransformation
            .rotate(-90.0)
            .translate(- rad * 1.5, s.width() - rad * 2 - 3.5)
            .scale(-1.0, 1.0);
        break;
    default:
        break;
    }
}

void KMessageWidgetFrame::setCalloutPointerPosition(const QPoint& globalPos)
{
    m_calloutPointerGlobalPosition = globalPos;
    updateCalloutPointerPosition();
}

QPoint KMessageWidgetFrame::calloutPointerPosition() const
{
    return m_calloutPointerGlobalPosition;
}

void KMessageWidgetFrame::updateCalloutPointerPosition() const
{
    if (m_calloutPointerGlobalPosition == QPoint(-QWIDGETSIZE_MAX, -QWIDGETSIZE_MAX))
        return;
    QWidget *messageWidgetParent = parentWidget()->parentWidget();
    if (messageWidgetParent) {
/*        qDebug() << "m_calloutPointerGlobalPosition:" << m_calloutPointerGlobalPosition
         << "pos():" << pos()
         << "pointerPosition():" << pointerPosition()
         << "(m_calloutPointerGlobalPosition - pos() - pointerPosition()):"
         << (m_calloutPointerGlobalPosition - pos() - pointerPosition())
         << "messageWidgetParent->mapFromGlobal():"
         << messageWidgetParent->mapFromGlobal(
              m_calloutPointerGlobalPosition - pos() - pointerPosition());*/
        parentWidget()->move(
            messageWidgetParent->mapFromGlobal(
                m_calloutPointerGlobalPosition - pos() - pointerPosition())
        );
    }
}

void KMessageWidgetFrame::paintCalloutPointer()
{
    updateCalloutPointerTransformation();

    if (m_calloutPointerTransformation.isIdentity())
        return;
    QPainter painter(this);
    painter.setTransform(m_calloutPointerTransformation);

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(bgBrush.color(), 1.0));
    painter.setBrush(bgBrush);
    painter.drawPolygon(m_polygon);
    painter.setPen(QPen(borderBrush, 1.0));
    painter.drawPolyline(m_polyline);
}

QPoint KMessageWidgetFrame::pointerPosition() const
{
    updateCalloutPointerTransformation();
    //qDebug() << "MAPPED:" << t.map(polyline[1]) << mapToGlobal(t.map(polyline[1]).toPoint());
    return m_calloutPointerTransformation.map(m_polyline[1]).toPoint();
}

//---------------------------------------------------------------------
// KMessageWidgetPrivate
//---------------------------------------------------------------------
class KMessageWidgetPrivate
{
public:
    KMessageWidgetPrivate();
    void init(KMessageWidget*);

    KMessageWidget* q;
    KMessageWidgetFrame* content;
    ClickableLabel* iconLabel;
    ClickableLabel* textLabel;
    KexiCloseButton* closeButton;
    QTimeLine* timeLine;

    KMessageWidget::MessageType messageType;
    bool wordWrap;
    QList<QToolButton*> buttons;
    QPixmap contentSnapShot;
    QAction* defaultAction;
    QPointer<QToolButton> defaultButton;
    QSet<QAction*> leftAlignedButtons;
    KColorScheme::ColorSet colorSet;
    KColorScheme::BackgroundRole bgRole;
    KColorScheme::ForegroundRole fgRole;
    bool autoDelete;
    QWidget* contentsWidget;
    bool clickClosesMessage;
    bool resizeToContentsOnTimeLineFinished;

    void createLayout();
    void updateSnapShot();
    void updateLayout();
    void updateStyleSheet();
};

KMessageWidgetPrivate::KMessageWidgetPrivate()
 : contentsWidget(0)
{
}

void KMessageWidgetPrivate::init(KMessageWidget *q_ptr)
{
    q = q_ptr;

    q->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    timeLine = new QTimeLine(500, q);
    QObject::connect(timeLine, SIGNAL(valueChanged(qreal)), q, SLOT(slotTimeLineChanged(qreal)));
    QObject::connect(timeLine, SIGNAL(finished()), q, SLOT(slotTimeLineFinished()));

    content = new KMessageWidgetFrame(q);
    content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    wordWrap = false;
    resizeToContentsOnTimeLineFinished = false;

    if (contentsWidget) {
        iconLabel = 0;
        textLabel = 0;
    }
    else {
        iconLabel = new ClickableLabel(content);
        iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
        QObject::connect(iconLabel, SIGNAL(clicked()), q, SLOT(tryClickCloseMessage()));

        textLabel = new ClickableLabel(content);
        textLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        textLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        textLabel->setContentsMargins(0, 0, 0, 0);
        QObject::connect(textLabel, SIGNAL(clicked()), q, SLOT(tryClickCloseMessage()));
#if 0
    content->setAutoFillBackground(true);
    content->setBackgroundRole(QPalette::Dark);
    textLabel->setAutoFillBackground(true);
    textLabel->setBackgroundRole(QPalette::Mid);
#endif
    }
    closeButton = new KexiCloseButton(content);
    QObject::connect(closeButton, SIGNAL(clicked()), q, SLOT(animatedHide()));

    defaultAction = 0;
    autoDelete = false;
    clickClosesMessage = false;
    q->setMessageType(KMessageWidget::Information);
}

void KMessageWidgetPrivate::createLayout()
{
    delete content->layout();

    content->resize(q->size());

    qDeleteAll(buttons);
    buttons.clear();

    QList<QToolButton*> buttonsTabOrder;
    Q_FOREACH(QAction* action, q->actions()) {
        QToolButton* button = new QToolButton(content);
        button->setDefaultAction(action);
        button->setFocusPolicy(Qt::StrongFocus);
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        buttons.append(button);
        if (defaultAction == action) {
            buttonsTabOrder.prepend(button); // default button is focused first
            q->setFocusProxy(button);
            defaultButton = button;
        }
        else
            buttonsTabOrder.append(button);
    }
    QToolButton *previousButton = 0;
    Q_FOREACH(QToolButton* button, buttonsTabOrder) {
        if (previousButton)
            QWidget::setTabOrder(previousButton, button);
        previousButton = button;
    }

    // Only set autoRaise on if there are no buttons, otherwise the close
    // button looks weird
    //closeButton->setAutoRaise(buttons.isEmpty());

    QHBoxLayout* buttonLayout = 0;
    int leftContentSpacerItemWidth = LAYOUT_SPACING;
    int rightContentSpacerItemWidth = LAYOUT_SPACING;
    int bottomContentSpacerItemHeight = LAYOUT_SPACING;
    switch (content->calloutPointerDirection()) {
    case KMessageWidget::Up:
        break;
    case KMessageWidget::Down:
        bottomContentSpacerItemHeight = content->radius * 2 + LAYOUT_SPACING;
        break;
    case KMessageWidget::Left:
        leftContentSpacerItemWidth = content->radius * 2 + LAYOUT_SPACING;
        break;
    case KMessageWidget::Right:
        rightContentSpacerItemWidth = content->radius * 2 + LAYOUT_SPACING;
        break;
    default:;
    }
    if (wordWrap) {
        QGridLayout* layout = new QGridLayout(content);
        layout->setSpacing(LAYOUT_SPACING);
        if (contentsWidget) {
            layout->addItem(new QSpacerItem(leftContentSpacerItemWidth, LAYOUT_SPACING), 0, 0);
            layout->addWidget(contentsWidget, 1, 0, 1, 2);
            layout->addItem(new QSpacerItem(rightContentSpacerItemWidth, LAYOUT_SPACING), 3, 0);

/*            if (contentsWidget->maximumWidth() < QWIDGETSIZE_MAX
                && contentsWidget->maximumHeight() < QWIDGETSIZE_MAX
                && contentsWidget->maximumSize() == contentsWidget->minimumSize())
            {
                qDebug() << "contentsWidget->maximumSize():" << contentsWidget->maximumSize();
                qDebug() << "content->size():" << content->size();
                contentsWidget->setFixedSize(
                    contentsWidget->maximumSize() - QSize(120, 0));
                //q->setFixedSize(
                //    contentsWidget->maximumSize() + QSize(100, 0));

                qFatal() << contentsWidget->maximumSize();
            }*/
        }
        else {
            layout->addItem(new QSpacerItem(leftContentSpacerItemWidth, LAYOUT_SPACING), 0, 0);
            layout->addWidget(iconLabel, 1, 1, Qt::AlignCenter | Qt::AlignTop);
            //iconLabel->setContentsMargins(0, LAYOUT_SPACING, 0, 0);
            iconLabel->setAlignment(Qt::AlignCenter | Qt::AlignTop);
            layout->addWidget(textLabel, 1, 2);
            textLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
            layout->addItem(new QSpacerItem(rightContentSpacerItemWidth, LAYOUT_SPACING), 0, 3);
        }

        buttonLayout = new QHBoxLayout;
        buttonLayout->setSpacing(LAYOUT_SPACING);
        bool stretchAdded = false;
        Q_FOREACH(QToolButton* button, buttons) {
            if (!stretchAdded && !leftAlignedButtons.contains(button->defaultAction())) {
                buttonLayout->addStretch();
                stretchAdded = true;
            }
            // For some reason, calling show() is necessary here, but not in
            // wordwrap mode
            button->show();
            buttonLayout->addWidget(button);
        }
        if (contentsWidget) {
            buttonLayout->addStretch();
            buttonLayout->addWidget(closeButton);
            buttonLayout->setContentsMargins(0, 10, 0, 0);
            layout->addLayout(buttonLayout, 0, 0);
        }
        else {
            buttonLayout->addWidget(closeButton);
            //?? buttonLayout->setContentsMargins(0, 10, 0, 0);
            layout->addItem(buttonLayout, 2, 1, 1, 2);
            layout->addItem(new QSpacerItem(rightContentSpacerItemWidth, bottomContentSpacerItemHeight), 3, 3);
        }
    } else {
        QHBoxLayout* layout = new QHBoxLayout(content);
        layout->setSpacing(LAYOUT_SPACING);
        if (contentsWidget) {
            layout->addWidget(contentsWidget);
        }
        else {
            layout->addWidget(iconLabel);
            layout->addWidget(textLabel);
        }
        Q_FOREACH(QToolButton* button, buttons) {
            layout->addWidget(button);
        }

        layout->addWidget(closeButton);
    };

    // add margins based on outer margins
    int left, top, right, bottom;
    q->getContentsMargins(&left, &top, &right, &bottom);
    //qDebug() << "q->getContentsMargins:" << left << top << right << bottom;
    switch (content->calloutPointerDirection()) {
    case KMessageWidget::Up:
        left += 1;
        top += 4;
        bottom += 4;
        right += 2;
        if (!buttons.isEmpty()) {
            top += 4;
            right += 3;
        }
        break;
    case KMessageWidget::Down:
        left += 1;
        top += 4;
        bottom += 4;
        right += 2;
        if (!buttons.isEmpty()) {
            right += 3;
        }
        break;
    case KMessageWidget::Left: {
        left += 0;
        top += 3;
        bottom += 3;
        right += 1;
        if (buttonLayout) {
            int leftSp = content->radius * 2 + LAYOUT_SPACING;
            buttonLayout->insertSpacing(0, leftSp);
            buttonLayout->addSpacing(LAYOUT_SPACING);
        }
        break;
    }
    case KMessageWidget::Right:
        left += 0;
        top += 3;
        bottom += 3;
        right += 1;
        break;
    default:;
    }
    content->layout()->setContentsMargins(
        left, top, right, bottom);

    if (q->isVisible()) {
        if (content->sizeHint().height() >= 0) {
            //q->setFixedHeight(content->sizeHint().height());
            q->setFixedHeight(QWIDGETSIZE_MAX);
        }
    }
    content->updateGeometry();
    q->updateGeometry();
}

void KMessageWidgetPrivate::updateLayout()
{
    if (content->layout()) {
        createLayout();
    }
}

void KMessageWidgetPrivate::updateSnapShot()
{
    contentSnapShot = QPixmap(content->size());
    contentSnapShot.fill(Qt::transparent);
    content->render(&contentSnapShot, QPoint(), QRegion(), QWidget::DrawChildren);
}

void KMessageWidgetPrivate::updateStyleSheet()
{
    KColorScheme scheme(QPalette::Active, colorSet);
    content->bgBrush = scheme.background(bgRole);
    content->borderBrush = scheme.foreground(fgRole);
    QBrush fg = scheme.foreground();

    int left, top, right, bottom;
    content->getContentsMargins(&left, &top, &right, &bottom);
    //qDebug() << "content->getContentsMargins:" << left << top << right << bottom;
    if (!buttons.isEmpty()) {
        //q->setContentsMargins(0, 0, 0, 0);
        content->setContentsMargins(LAYOUT_SPACING, 0, 0, 0);
    }
    q->getContentsMargins(&left, &top, &right, &bottom);
    //qDebug() << "q->getContentsMargins:" << left << top << right << bottom;
#if 1
    int add = content->radius * 2;
    switch (content->calloutPointerDirection()) {
    case KMessageWidget::Up:
        top += add;
        break;
    case KMessageWidget::Down:
        bottom += add;
        break;
    case KMessageWidget::Left:
        left += add;
        break;
    case KMessageWidget::Right:
        right += add;
        break;
    default:;
    }
    content->setStyleSheet(
        QString(".KMessageWidgetFrame {"
            "background-color: %1;"
            "border-radius: %2px;"
            "margin: %3px %4px %5px %6px;"
            "border: 1px solid %7;"
            "}"
            ".QLabel { color: %8; }"
            )
        .arg(content->bgBrush.color().name())
        .arg(content->radius)
        .arg(top)
        .arg(right)
        .arg(bottom)
        .arg(left)
        .arg(content->borderBrush.color().name())
        .arg(fg.color().name())
    );
    closeButton->setStyle(QApplication::style()); // clear stylesheets style from this button
#endif
}

//---------------------------------------------------------------------
// KMessageWidget
//---------------------------------------------------------------------
KMessageWidget::KMessageWidget(QWidget* parent)
    : QFrame(parent)
    , d(new KMessageWidgetPrivate)
{
    d->init(this);
}

KMessageWidget::KMessageWidget(const QString& text, QWidget* parent)
    : QFrame(parent)
    , d(new KMessageWidgetPrivate)
{
    d->init(this);
    setText(text);
}

KMessageWidget::KMessageWidget(QWidget* contentsWidget, QWidget* parent)
    : QFrame(parent)
    , d(new KMessageWidgetPrivate)
{
    d->contentsWidget = contentsWidget;
    d->init(this);
}

KMessageWidget::~KMessageWidget()
{
    delete d;
}

QString KMessageWidget::text() const
{
    return d->textLabel ? d->textLabel->text() : QString();
}

void KMessageWidget::setText(const QString& text)
{
    if (d->textLabel) {
        d->textLabel->setText(text);
        updateGeometry();
    }
}

KMessageWidget::MessageType KMessageWidget::messageType() const
{
    return d->messageType;
}

void KMessageWidget::setMessageType(KMessageWidget::MessageType type)
{
    d->messageType = type;
    QIcon icon;
    d->colorSet = KColorScheme::View;
    switch (type) {
    case Positive:
        icon = koIcon("dialog-ok");
        d->bgRole = KColorScheme::PositiveBackground;
        d->fgRole = KColorScheme::PositiveText;
        break;
    case Information:
        icon = koIcon("dialog-information");
        d->bgRole = KColorScheme::NeutralBackground;
        d->fgRole = KColorScheme::NeutralText;
        break;
    case Warning:
        icon = koIcon("dialog-warning");
        d->bgRole = KColorScheme::NeutralBackground;
        d->fgRole = KColorScheme::NeutralText;
        break;
    case Error:
        icon = koIcon("dialog-error");
        d->bgRole = KColorScheme::NegativeBackground;
        d->fgRole = KColorScheme::NegativeText;
        break;
    }
    if (d->iconLabel) {
        const int size = KIconLoader::global()->currentSize(KIconLoader::MainToolbar);
        d->iconLabel->setPixmap(icon.pixmap(size));
    }

    d->updateStyleSheet();
    d->updateLayout();
}

KMessageWidget::CalloutPointerDirection KMessageWidget::calloutPointerDirection() const
{
    return d->content->calloutPointerDirection();
}

void KMessageWidget::setCalloutPointerDirection(KMessageWidget::CalloutPointerDirection direction)
{
    d->content->setCalloutPointerDirection(direction);
    d->updateStyleSheet();
    d->updateLayout();
    d->content->updateCalloutPointerPosition();
}

QSize KMessageWidget::sizeHint() const
{
    ensurePolished();
    //qDebug() << "d->content->sizeHint():" << d->content->sizeHint();
    //qDebug() << "QFrame::sizeHint():" << QFrame::sizeHint();
    return QFrame::sizeHint();
/*    QSize s1(QFrame::sizeHint());
    QSize s2(d->content->sizeHint());
    return QSize(qMax(s1.width(), s2.width()), qMax(s1.height(), s2.height()));*/
}

QSize KMessageWidget::minimumSizeHint() const
{
    ensurePolished();
    //qDebug() << "d->content->minimumSizeHint():" << d->content->minimumSizeHint();
    //qDebug() << "QFrame::minimumSizeHint():" << QFrame::minimumSizeHint();
    return QFrame::minimumSizeHint();
/*    QSize s1(QFrame::minimumSizeHint());
    QSize s2(d->content->minimumSizeHint());
    return QSize(qMax(s1.width(), s2.width()), qMax(s1.height(), s2.height()));*/
}

bool KMessageWidget::event(QEvent* event)
{
    if (event->type() == QEvent::Polish && !d->content->layout()) {
        d->createLayout();
    }
    else if (event->type() == QEvent::Hide) {
        //qDebug() << "QEvent::Hide" << event->spontaneous();
        if (!event->spontaneous()) {
            if (d->autoDelete) {
                deleteLater();
            }
        }
    }
    else if (event->type() == QEvent::MouseButtonPress) {
        if (static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
            tryClickCloseMessage();
        }
    }
    return QFrame::event(event);
}

void KMessageWidget::resizeEvent(QResizeEvent* event)
{
    QFrame::resizeEvent(event);
    if (d->timeLine->state() == QTimeLine::NotRunning) {
        d->content->resize(size());
        d->updateStyleSheet(); // needed because margins could be changed
    }
}

void KMessageWidget::paintEvent(QPaintEvent* event)
{
    QFrame::paintEvent(event);
    if (d->timeLine->state() == QTimeLine::Running) {
        QPainter painter(this);
        painter.setOpacity(d->timeLine->currentValue() * d->timeLine->currentValue());
        painter.drawPixmap(0, 0, d->contentSnapShot);
    }
}

void KMessageWidget::showEvent(QShowEvent* event)
{
    QFrame::showEvent(event);
    if (!event->spontaneous()) {
#if 0
        int wantedHeight = d->content->sizeHint().height();
        d->content->setGeometry(0, 0, width(), wantedHeight);
        if (d->buttons.isEmpty()) {
            setFixedHeight(wantedHeight);
        }
#endif
    }
}

bool KMessageWidget::wordWrap() const
{
    return d->wordWrap;
}

void KMessageWidget::setWordWrap(bool wordWrap)
{
    d->wordWrap = wordWrap;
    if (d->textLabel) {
        d->textLabel->setWordWrap(wordWrap);
        d->updateLayout();
    }
}

bool KMessageWidget::isCloseButtonVisible() const
{
    return d->closeButton->isVisible();
}

void KMessageWidget::setCloseButtonVisible(bool show)
{
    d->closeButton->setVisible(show);
}

bool KMessageWidget::clickClosesMessage() const
{
    return d->clickClosesMessage;
}

void KMessageWidget::setClickClosesMessage(bool set)
{
    d->clickClosesMessage = set;
}

void KMessageWidget::addAction(QAction* action)
{
    QFrame::addAction(action);
    d->updateLayout();
}

void KMessageWidget::setDefaultAction(QAction* action)
{
    d->defaultAction = action;
    d->createLayout();
}

void KMessageWidget::setButtonLeftAlignedForAction(QAction *action)
{
    d->leftAlignedButtons.insert(action);
}

void KMessageWidget::removeAction(QAction* action)
{
    QFrame::removeAction(action);
    d->updateLayout();
}

void KMessageWidget::setAutoDelete(bool set)
{
    d->autoDelete = set;
}

void KMessageWidget::animatedShow()
{
    if (!(KexiUtils::graphicEffectsLevel() & KexiUtils::SimpleAnimationEffects)) {
        show();
        return;
    }

    if (isVisible()) {
        return;
    }

    d->content->updateCalloutPointerPosition();
    QFrame::show();
    if (d->contentsWidget) {
        int wantedHeight = height();
        d->content->setGeometry(0, 0, width(), wantedHeight);
        setFixedHeight(wantedHeight);
    }
    else {
        setFixedHeight(0);
        int wantedHeight = d->content->sizeHint().height();
        d->content->setGeometry(0, -wantedHeight, width(), wantedHeight);
    }

    d->updateSnapShot();

    d->timeLine->setDirection(QTimeLine::Forward);
    if (d->timeLine->state() == QTimeLine::NotRunning) {
        d->timeLine->start();
    }
}

void KMessageWidget::animatedHide()
{
    if (!(KexiUtils::graphicEffectsLevel() & KexiUtils::SimpleAnimationEffects)) {
        hide();
        return;
    }

    if (!isVisible()) {
        return;
    }

    d->content->move(0, -d->content->height());
    d->updateSnapShot();

    d->timeLine->setDirection(QTimeLine::Backward);
    if (d->timeLine->state() == QTimeLine::NotRunning) {
        d->timeLine->start();
    }
}

void KMessageWidget::setCalloutPointerPosition(const QPoint& globalPos)
{
    d->content->setCalloutPointerPosition(globalPos);
}

QPoint KMessageWidget::calloutPointerPosition() const
{
    return d->content->calloutPointerPosition();
}

QBrush KMessageWidget::backgroundBrush() const
{
    return d->content->bgBrush;
}

QBrush KMessageWidget::borderBrush() const
{
    return d->content->borderBrush;
}

void KMessageWidget::resizeToContents()
{
//    qDebug() << LAYOUT_SPACING + d->iconLabel->width() + LAYOUT_SPACING + d->textLabel->width() + LAYOUT_SPACING;
//    qDebug() << "sizeHint():" << sizeHint();
//    qDebug() << "d->content->sizeHint():" << d->content->sizeHint();
    d->resizeToContentsOnTimeLineFinished = true; // try to resize later too if animation in progress
    (void)sizeHint(); // to update d->content->sizeHint()
    setFixedSize(d->content->sizeHint());
}

void KMessageWidget::slotTimeLineChanged(qreal value)
{
    if (!d->contentsWidget) {
        setFixedHeight(qMin(value * 2, qreal(1.0)) * d->content->height());
    }
    update();
}

void KMessageWidget::slotTimeLineFinished()
{
    if (d->timeLine->direction() == QTimeLine::Forward) {
        // Show
        d->content->move(0, 0);
        d->content->updateCalloutPointerPosition();
        if (d->resizeToContentsOnTimeLineFinished) {
            d->resizeToContentsOnTimeLineFinished = false;
            d->content->resize(size());
            d->updateStyleSheet(); // needed because margins could be changed
        }
        //q->setFixedHeight(QWIDGETSIZE_MAX);
        if (d->defaultButton) {
            d->defaultButton->setFocus();
        }
        emit animatedShowFinished();
    } else {
        // Hide
        hide();
        emit animatedHideFinished();
    }
}

void KMessageWidget::tryClickCloseMessage()
{
    if (d->clickClosesMessage) {
        QTimer::singleShot(100, this, SLOT(animatedHide()));
    }
}
