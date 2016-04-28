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

#include <kexiutils/utils.h>

#include <QAbstractScrollArea>
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

    void setSubWidgetsScrollbarsVisible(bool set)
    {
        QWidget *targetWidget = qobject_cast<QWidget*>(q->parent());
        QLayout *lyr = targetWidget->layout();
        if (!lyr) {
            return;
        }
        for (int i = 0; i < lyr->count(); ++i) {
            QAbstractScrollArea* area = qobject_cast<QAbstractScrollArea*>(lyr->itemAt(i)->widget());
            if (area) {
                //! @todo remember and restore these settings instead of hardcoding!
                area->setHorizontalScrollBarPolicy(set ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
                area->setVerticalScrollBarPolicy(set ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
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
        const int duration = (KexiUtils::graphicEffectsLevel() & KexiUtils::SimpleAnimationEffects) ? 300 : 0;
        d->widthAnimation->setDuration(duration);
        d->widthAnimation->setEasingCurve(QEasingCurve::InOutQuad);
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
        //qDebug() << "targetWidget->isVisible():" << set << targetWidget->isVisible() << d->originalWidth << width();
        const int w = d->originalWidth;
        if (set) {
            targetWidget->setVisible(true);
            d->originalWidth = w; // restore because setVisible() causes resize event and changes d->originalWidth
        }
        if (d->widthAnimation->duration() > 0) {
            d->setSubWidgetsVisible(true);
            d->frozen = false;
            const QSize currentSize = targetWidget->size();
            //qDebug() << "currentSize:" << currentSize << d->originalWidth;
            targetWidget->resize(d->originalWidth, targetWidget->height());
            d->setSubWidgetsScrollbarsVisible(false);
            targetWidget->updateGeometry();
            d->snapshot = targetWidget->grab();
            targetWidget->resize(currentSize);
            d->originalWidth = w; // restore because targetWidget->resize() causes resize event and changes d->originalWidth
            d->frozen = true;
            d->setSubWidgetsScrollbarsVisible(true);
            d->setSubWidgetsVisible(false);
        }
        d->widthAnimation->start();
    }
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
    QWidget *targetWidget = qobject_cast<QWidget*>(parent());
    if (width() == 0) {
        targetWidget->setVisible(false);
    }
    d->frozen = false;
    d->setSubWidgetsVisible(true);
    emit animationFinished(targetWidget->isVisible());
}

bool KexiWidgetWidthAnimator::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == parent()) {
        switch (event->type()) {
        case QEvent::Paint:
            if (d->frozen) {
                //qDebug() << "d->snapshot.size():" << d->snapshot.size();
                QPaintEvent *paintEvent = static_cast<QPaintEvent*>(event);
                QWidget *targetWidget = qobject_cast<QWidget*>(parent());
                QPainter p(targetWidget);
                p.drawPixmap(paintEvent->rect(), d->snapshot,
                             QRect(d->originalWidth - targetWidget->width(), 0,
                                   targetWidget->width(), targetWidget->height()));
            }
            break;
        case QEvent::Resize:
            if (!d->widthAnimation || d->widthAnimation->state() != QAbstractAnimation::Running) {
                QResizeEvent *resizeEvent = static_cast<QResizeEvent*>(event);
                d->originalWidth = resizeEvent->size().width();
                //qDebug() << "new size:" << resizeEvent->size();
            }
            break;
        default:;
        }
    }
    return QObject::eventFilter(obj, event);
}
