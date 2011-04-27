/* This file is part of the KDE libraries
 *
 * Copyright (c) 2011 Aurélien Gâteau <agateau@kde.org>
 * Copyright (C) 2011 Jarosław Staniek <staniek@kde.org>
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

#include <kaction.h>
#include <kcolorscheme.h>
#include <kdebug.h>
#include <kicon.h>
#include <kiconloader.h>
#include <kstandardaction.h>
#include <kglobalsettings.h>

#include <QEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QTimeLine>
#include <QToolButton>
#include <QPointer>
#include <QPainterPath>

//---------------------------------------------------------------------
// KMessageWidgetFrame
//---------------------------------------------------------------------
class KMessageWidgetFrame : public QFrame
{
public:
    KMessageWidgetFrame(QWidget* parent = 0)
     : QFrame(parent), radius(5), arr(6.0)
     {
     }
     
    virtual void paintEvent(QPaintEvent* event) {
        QFrame::paintEvent(event);
        QPainter painter(this);
        const QSizeF s(size());
        const qreal rad = radius;
        QPolygonF polyline;
        polyline << QPointF(rad * 3.0 + 0.5, s.height() - rad * 2)
                 << QPointF(rad * 3.0 + 0.5 + rad, s.height() - 0.5)
                 << QPointF(rad * 3.0 + 0.5 + rad * 2.0, s.height() - rad * 2);
        QPolygonF polygon;
        polygon << QPointF(polyline[0].x(), polyline[0].y() - 0.5)
                << polyline[1]
                << QPointF(polyline[2].x(), polyline[2].y() - 0.5);
        
        painter.setPen(QPen(bg.color(), 1.0));
        painter.setBrush(bg);
        painter.drawPolygon(polygon);
        painter.setPen(QPen(fg, 1.0));
        painter.setRenderHint(QPainter::Antialiasing);
        painter.drawPolyline(polyline);
    }

    const int radius;
    const qreal arr;
    QBrush bg;
    QBrush fg;
};

//---------------------------------------------------------------------
// KMessageWidgetPrivate
//---------------------------------------------------------------------
class KMessageWidgetPrivate
{
public:
    void init(KMessageWidget*);

    KMessageWidget* q;
    KMessageWidgetFrame* content;
    QLabel* iconLabel;
    QLabel* textLabel;
    QToolButton* closeButton;
    QTimeLine* timeLine;

    QString text;
    KMessageWidget::MessageType messageType;
    KMessageWidget::Shape shape;
    QList<QToolButton*> buttons;
    QPixmap contentSnapShot;
    QAction* defaultAction;
    QPointer<QToolButton> defaultButton;
    KColorScheme::ColorSet colorSet;
    KColorScheme::BackgroundRole bgRole;
    KColorScheme::ForegroundRole fgRole;
    bool autoDelete;

    void createLayout();
    void updateSnapShot();
    void updateStyleSheet();
};

void KMessageWidgetPrivate::init(KMessageWidget *q_ptr)
{
    q = q_ptr;

    q->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    timeLine = new QTimeLine(500, q);
    QObject::connect(timeLine, SIGNAL(valueChanged(qreal)), q, SLOT(slotTimeLineChanged(qreal)));
    QObject::connect(timeLine, SIGNAL(finished()), q, SLOT(slotTimeLineFinished()));

    content = new KMessageWidgetFrame(q);
    content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    shape = KMessageWidget::LineShape;

    iconLabel = new QLabel(content);
    iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    textLabel = new QLabel(content);
    textLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    textLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

    KAction* closeAction = KStandardAction::close(q, SLOT(animatedHide()), q);

    closeButton = new QToolButton(content);
    closeButton->setAutoRaise(true);
    closeButton->setDefaultAction(closeAction);
    
    defaultAction = 0;
    autoDelete = false;
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

    if (shape == KMessageWidget::LineShape) {
        QHBoxLayout* layout = new QHBoxLayout(content);
        layout->addWidget(iconLabel);
        layout->addWidget(textLabel);

        Q_FOREACH(QToolButton* button, buttons) {
            layout->addWidget(button);
        }

        layout->addWidget(closeButton);
    } else {
        QGridLayout* layout = new QGridLayout(content);
        layout->setSpacing(0);
        layout->addWidget(iconLabel, 0, 0);
        layout->addWidget(textLabel, 0, 1);

        QHBoxLayout* buttonLayout = new QHBoxLayout;
        buttonLayout->addStretch();
        Q_FOREACH(QToolButton* button, buttons) {
            // For some reason, calling show() is necessary here, but not in
            // LineShape.
            button->show();
            buttonLayout->addWidget(button);
        }
        buttonLayout->addWidget(closeButton);
        layout->addItem(buttonLayout, 1, 0, 1, 2);
    }

    q->updateGeometry();
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
    content->bg = scheme.background(bgRole);
    content->fg = scheme.foreground(fgRole);
    int left, top, right, bottom;
    q->getContentsMargins(&left, &top, &right, &bottom);
    content->setStyleSheet(
        QString(".QFrame {"
            "background-color: %1;"
            "border-radius: %2px;"
            "margin: %3px %4px %5px %6px;"
            "border: 1px solid %7;"
            "}"
            ".QLabel { color: %7; }"
            )
        .arg(content->bg.color().name())
        .arg(content->radius)
        .arg(top).arg(right).arg(content->radius * 2 + bottom).arg(left)
        .arg(content->fg.color().name())
    );
}

//---------------------------------------------------------------------
// KMessageWidget
//---------------------------------------------------------------------
KMessageWidget::KMessageWidget(QWidget* parent)
: QFrame(parent)
, d(new KMessageWidgetPrivate)
{
    d->init(this);
    setMessageType(InformationMessageType);
}

KMessageWidget::~KMessageWidget()
{
    delete d;
}

QString KMessageWidget::text() const
{
    return d->text;
}

void KMessageWidget::setText(const QString& text)
{
    d->text = text;
    d->textLabel->setText(text);
}

KMessageWidget::MessageType KMessageWidget::messageType() const
{
    return d->messageType;
}

void KMessageWidget::setMessageType(KMessageWidget::MessageType type)
{
    d->messageType = type;
    KIcon icon;
    d->colorSet = KColorScheme::Window;
    switch (type) {
    case PositiveMessageType:
        icon = KIcon("dialog-ok");
        d->bgRole = KColorScheme::PositiveBackground;
        d->fgRole = KColorScheme::PositiveText;
        break;
    case InformationMessageType:
        icon = KIcon("dialog-information");
        d->bgRole = KColorScheme::NormalBackground;
        d->fgRole = KColorScheme::NormalText;
        d->colorSet = KColorScheme::View;
        break;
    case WarningMessageType:
        icon = KIcon("dialog-warning");
        d->bgRole = KColorScheme::NeutralBackground;
        d->fgRole = KColorScheme::NeutralText;
        break;
    case ErrorMessageType:
        icon = KIcon("dialog-error");
        d->bgRole = KColorScheme::NegativeBackground;
        d->fgRole = KColorScheme::NegativeText;
        break;
    }
    const int size = KIconLoader::global()->currentSize(KIconLoader::MainToolbar);
    d->iconLabel->setPixmap(icon.pixmap(size));

    d->updateStyleSheet();
}

bool KMessageWidget::event(QEvent* event)
{
    if (event->type() == QEvent::Polish && !layout()) {
        d->createLayout();
    }
    else if (event->type() == QEvent::Hide) {
        if (d->autoDelete) {
            deleteLater();
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
    d->content->adjustSize();
    int wantedHeight = d->content->height();
    d->content->resize(width(), wantedHeight);
    setFixedHeight(wantedHeight);
}

KMessageWidget::Shape KMessageWidget::shape() const
{
    return d->shape;
}

void KMessageWidget::setShape(KMessageWidget::Shape shape)
{
    d->shape = shape;
    d->createLayout();
}

bool KMessageWidget::showCloseButton() const
{
    return d->closeButton->isVisible();
}

void KMessageWidget::setShowCloseButton(bool show)
{
    d->closeButton->setVisible(show);
}

void KMessageWidget::addAction(QAction* action)
{
    QFrame::addAction(action);
    d->createLayout();
}

void KMessageWidget::setDefaultAction(QAction* action)
{
    d->defaultAction = action;
    d->createLayout();
}

void KMessageWidget::removeAction(QAction* action)
{
    QFrame::removeAction(action);
    d->createLayout();
}

void KMessageWidget::setAutoDelete(bool set)
{
    d->autoDelete = set;
}

void KMessageWidget::animatedShow()
{
    if (!(KGlobalSettings::graphicEffectsLevel() & KGlobalSettings::SimpleAnimationEffects)) {
        show();
        return;
    }

    QFrame::show();
    setFixedHeight(0);
    d->content->adjustSize();
    int wantedHeight = d->content->height();
    d->content->resize(width(), wantedHeight);
    d->content->move(0, -wantedHeight);

    d->updateSnapShot();

    d->timeLine->setDirection(QTimeLine::Forward);
    if (d->timeLine->state() == QTimeLine::NotRunning) {
        d->timeLine->start();
    }
}

void KMessageWidget::slotTimeLineChanged(qreal value)
{
    setFixedHeight(qMin(value * 2, 1.) * d->content->height());
    repaint();
}

void KMessageWidget::slotTimeLineFinished()
{
    if (d->timeLine->direction() == QTimeLine::Forward) {
        // Show
        d->content->move(0, 0);
        if (d->defaultButton) {
            d->defaultButton->setFocus();
        }
    } else {
        // Hide
        hide();
    }
}

void KMessageWidget::animatedHide()
{
    if (!(KGlobalSettings::graphicEffectsLevel() & KGlobalSettings::SimpleAnimationEffects)) {
        hide();
        return;
    }

    d->content->move(0, -d->content->height());
    d->updateSnapShot();

    d->timeLine->setDirection(QTimeLine::Backward);
    if (d->timeLine->state() == QTimeLine::NotRunning) {
        d->timeLine->start();
    }
}

#include "kmessagewidget.moc"
