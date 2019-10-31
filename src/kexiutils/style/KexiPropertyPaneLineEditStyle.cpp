/* This file is part of the KDE project
   Copyright (C) 2016 Jarosław Staniek <staniek@kde.org>

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

#include "KexiPropertyPaneLineEditStyle.h"
#include <KexiIcon.h>
#include "KexiStyle.h"

#include <KColorScheme>
#include <KColorUtils>

#include <QDebug>
#include <QEvent>
#include <QProxyStyle>
#include <QStyleOption>
#include <QComboBox>
#include <QLineEdit>
#include <QAbstractItemView>

static int MenuButton_IndicatorWidth = 20;

//! A style modification for KexiPropertyPaneLineEdit to allow minimal size
class KexiPropertyPaneLineEditProxyStyle : public QProxyStyle
{
public:
    explicit KexiPropertyPaneLineEditProxyStyle(QStyle *s = nullptr)
        : QProxyStyle(s)
        , m_viewFocusBrush(KColorScheme::View, KColorScheme::FocusColor)
    {
    }

    int pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const Q_DECL_OVERRIDE {
        const QLineEdit* lineEdit = qobject_cast<const QLineEdit*>(widget);
        if (lineEdit) {
            switch( metric ) {
            // frame width
            case PM_DefaultFrameWidth:
                return 0;
            case PM_ComboBoxFrameWidth:
                return 0;
            default:
                break;
            }
        }
        return QProxyStyle::pixelMetric(metric, option, widget);
    }

    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc, const QWidget *widget) const Q_DECL_OVERRIDE {
        if (cc == CC_ComboBox) {
            if (sc == SC_ComboBoxEditField) {
                //qDebug() << opt->rect;
                QRect r(opt->rect);
                r.adjust(1, 0, -MenuButton_IndicatorWidth, 0);
                return visualRect(opt->direction, opt->rect, r);
            }
        }
        return QProxyStyle::subControlRect(cc, opt, sc, widget);
    }

    QSize sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size, const QWidget *widget) const Q_DECL_OVERRIDE {
        if (type == CT_ComboBox) {
            return size;
        }
        else if (type == CT_LineEdit) {
            return size - QSize(MenuButton_IndicatorWidth, 0);
        }
        return QProxyStyle::sizeFromContents(type, option, size, widget);
    }

    void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter,
                       const QWidget* widget ) const Q_DECL_OVERRIDE
    {
        switch (element) {
        case PE_FrameLineEdit: {
            //qDebug() << "**" << option->rect;
            return;
        }
        case PE_PanelLineEdit: {
            const bool enabled(option->state & State_Enabled);
            const bool hasFocus(enabled && (option->state & State_HasFocus));
            const QComboBox* combo = qobject_cast<const QComboBox*>(widget->parentWidget());
            const bool popupVisible = combo && combo->view()->isVisible();
            if (hasFocus || popupVisible) {
                //normal: QColor outline(KColorUtils::mix(option->palette.color(QPalette::Window),
                //                       option->palette.color( QPalette::WindowText ), 0.25));
                const QColor outline = m_viewFocusBrush.brush(option->palette).color();
                const QColor background(option->palette.color(QPalette::Base));
                QRectF frameRect(option->rect);
                if (outline.isValid()) {
                    painter->setPen(outline);
                    frameRect.adjust(0.5, 0.5, -0.5, -0.5);
                } else {
                    painter->setPen(Qt::NoPen);
                }
                if (combo) {
                    frameRect.adjust(-MenuButton_IndicatorWidth - 1, 0, MenuButton_IndicatorWidth - 1, 0);
                }
                painter->setBrush(background.isValid() ? background : Qt::NoBrush);
                const qreal radius = 1.5;
                painter->setRenderHint(QPainter::Antialiasing);
                //painter->setClipRect(option->rect.adjusted(-MenuButton_IndicatorWidth, 0, MenuButton_IndicatorWidth, 0));
                painter->drawRoundedRect(frameRect, radius, radius);
                return;
            }
        }
        default:
            break;
        }
        QProxyStyle::drawPrimitive(element, option, painter, widget);
    }

    void drawControl(ControlElement element, const QStyleOption *opt, QPainter *p, const QWidget *widget) const override {
        if (element == CE_ComboBoxLabel) {
            const QStyleOptionComboBox *cb = qstyleoption_cast<const QStyleOptionComboBox *>(opt);
            if (cb) {
                QStyleOptionComboBox cbNew(*cb);
                cbNew.palette.setBrush(QPalette::Base, QBrush());
                QProxyStyle::drawControl(element, &cbNew, p, widget);
                return;
            }
            //return;
        }
        QProxyStyle::drawControl(element, opt, p, widget);
    }

    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *option, QPainter *painter,
                            const QWidget *widget = nullptr) const Q_DECL_OVERRIDE
    {
        QProxyStyle::drawComplexControl(cc, option, painter, widget);
        const bool enabled(option->state & State_Enabled);
        const bool hasFocus(enabled && (option->state & State_HasFocus));
        const QComboBox* combo = qobject_cast<const QComboBox*>(widget);
        const bool popupVisible = combo && combo->view()->isVisible();
        if (cc == CC_ComboBox && (hasFocus || popupVisible)) {
            const qreal radius = 1.5;
            QRectF frameRect(option->rect);
            const QColor outline = m_viewFocusBrush.brush(option->palette).color();
            if (outline.isValid()) {
                painter->setPen(outline);
                frameRect.adjust(0.5, 0.5, -0.5, -0.5);
            } else {
                painter->setPen(Qt::NoPen);
            }
            painter->setBrush(Qt::NoBrush);
            //qDebug() << frameRect << "frameRect";
            painter->drawRoundedRect(frameRect, radius, radius);
        }
    }

    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption *option = nullptr,
                       const QWidget *widget = nullptr) const Q_DECL_OVERRIDE
    {
        if (standardIcon == SP_LineEditClearButton) {
            return koDarkIcon("edit-clear-small");
        }
        return QProxyStyle::standardIcon(standardIcon, option, widget);
    }

private:
    KStatefulBrush m_viewFocusBrush;
};

class Filter : public QObject
{
public:
    Filter(QObject *parent) : QObject(parent)
    {
        parent->installEventFilter(this);
    }
    bool eventFilter(QObject *watched, QEvent *event) Q_DECL_OVERRIDE {
        if (event->type() == QEvent::StyleChange) {
            return true;
        }
        return QObject::eventFilter(watched, event);
    }
};

class KexiPropertyPaneLineEditProxyStyleGlobal
{
public:
    void alterStyle(QWidget *w) {
        if (!style) {
            KexiPropertyPaneLineEditProxyStyle *s = new KexiPropertyPaneLineEditProxyStyle(w->style());
            style.reset(s);
        }
        (void)new Filter(w);
        w->setStyle(style.data());
    }
    QScopedPointer<KexiPropertyPaneLineEditProxyStyle> style;
};

Q_GLOBAL_STATIC(KexiPropertyPaneLineEditProxyStyleGlobal, s_style)

void alterPropertyPaneLineEditProxyStyle(QWidget *w)
{
    if (w) {
        s_style->alterStyle(w);
    }
}
