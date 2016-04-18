/* This file is part of the KDE project
   Copyright (C) 2016 Jarosław Staniek <staniek@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include "KexiWidgetWidthAnimator.h"
#include <QLayout>
#include <QPainter>
#include <QPaintEvent>
#include <QSplitter>
#include <QPropertyAnimation>
#include <QDebug>

const int PROJECT_NAVIGATOR_INDEX = 0;
const int MAIN_AREA_INDEX = 1;
const int PROPERTY_EDITOR_INDEX = 2;

class KexiWidgetWidthAnimator::Private
{
public:
    explicit Private(KexiWidgetWidthAnimator *qq)
        : q(qq)
        , widthAnimation(0)
        , originalWidth(0)
        , frozen(false)
    {
    }

    void setSubWidgetsVisible(bool set)
    {
        QWidget *targetWidget = qobject_cast<QWidget*>(q->parent());
        QLayout *lyr = targetWidget->layout();
        if (!lyr) {
            return;
        }
        for (int i = 0; i < lyr->count(); ++i) {
            QWidget *subWidget = lyr->itemAt(i)->widget();
            if (subWidget) {
                subWidget->setVisible(set);
            }
        }
    }

    KexiWidgetWidthAnimator * const q;
    QPropertyAnimation *widthAnimation;
    int originalWidth;
    bool frozen;
    QPixmap snapshot;
};

KexiWidgetWidthAnimator::KexiWidgetWidthAnimator(QWidget *targetWidget)
 : QObject(targetWidget), d(new Private(this))
{
    targetWidget->installEventFilter(this);
}

KexiWidgetWidthAnimator::~KexiWidgetWidthAnimator()
{
    delete d;
}

void KexiWidgetWidthAnimator::setVisible(bool set)
{
    const bool stoppedBeforeFinishing = d->widthAnimation && d->widthAnimation->state() == QAbstractAnimation::Running;
    QWidget *targetWidget = qobject_cast<QWidget*>(parent());
    if (set == targetWidget->isVisible()) {
        return;
    }
    if (!stoppedBeforeFinishing && !set) {
        d->originalWidth = width();
    }
    if (!d->widthAnimation) {
        d->widthAnimation = new QPropertyAnimation(this, "width");
        d->widthAnimation->setDuration(500);
        d->widthAnimation->setEasingCurve(QEasingCurve::InOutQuart);
        connect(d->widthAnimation, &QPropertyAnimation::finished,
                this, &KexiWidgetWidthAnimator::slotWidthAnimationFinished);
    }
    if (stoppedBeforeFinishing) {
        d->widthAnimation->pause();
        const QVariant end = d->widthAnimation->endValue();
        d->widthAnimation->setEndValue(d->widthAnimation->startValue());
        d->widthAnimation->setStartValue(end);
        d->widthAnimation->setCurrentTime(
                d->widthAnimation->duration() - d->widthAnimation->currentTime());
        d->widthAnimation->resume();
    } else {
        d->widthAnimation->setStartValue(set ? 0 : d->originalWidth);
        d->widthAnimation->setEndValue(set ? d->originalWidth : 0);
        if (set) {
            targetWidget->setVisible(true);
        }
        d->setSubWidgetsVisible(true);
        d->frozen = false;
        if (!set) {
            d->snapshot = targetWidget->grab();
        }
        d->frozen = true;
        d->setSubWidgetsVisible(false);
        d->widthAnimation->start();
    }
    //setUpdatesEnabled(false);
}

int KexiWidgetWidthAnimator::width() const
{
    return qobject_cast<QWidget*>(parent())->width();
}

void KexiWidgetWidthAnimator::setWidth(int width)
{
    QWidget *targetWidget = qobject_cast<QWidget*>(parent());
    QSplitter* splitter = qobject_cast<QSplitter*>(targetWidget->parentWidget());
    const int index = splitter->indexOf(targetWidget);
    if (index < 0) {
        return;
    }
    QList<int> sizes(splitter->sizes());
    //qDebug() << "setWidth-" << sizes;
    int oldWidth = sizes[index];
    if (sizes.count() >= (MAIN_AREA_INDEX+1)) { // make total size unchanged
        sizes[MAIN_AREA_INDEX] += (oldWidth - width);
    }
    sizes[index] = width;
    targetWidget->resize(width, targetWidget->height());
    splitter->setSizes(sizes);
    //qDebug() << "setWidth" << index << width << sizes << splitter->sizes();
}

void KexiWidgetWidthAnimator::slotWidthAnimationFinished()
{
    if (width() == 0) {
        QWidget *targetWidget = qobject_cast<QWidget*>(parent());
        targetWidget->setVisible(false);
    }
    d->frozen = false;
    d->setSubWidgetsVisible(true);
    //setUpdatesEnabled(true);
}

bool KexiWidgetWidthAnimator::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == parent()) {
        if (event->type() == QEvent::Paint && d->frozen) {
            QPaintEvent *paintEvent = static_cast<QPaintEvent*>(event);
            QWidget *targetWidget = qobject_cast<QWidget*>(parent());
            QPainter p(targetWidget);
            p.drawPixmap(paintEvent->rect(), d->snapshot,
                         QRect(d->originalWidth - targetWidget->width(), 0,
                               targetWidget->width(), targetWidget->height()));
        }
    }
    return QObject::eventFilter(obj, event);
}
