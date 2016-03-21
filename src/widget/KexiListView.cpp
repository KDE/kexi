/* This file is part of the KDE project
   Copyright (C) 2016 Jarosław Staniek <staniek@kde.org>

   Forked from kwidgetsaddons/src/kpageview_p.cpp:
   Copyright (C) 2006 Tobias Koenig (tokoe@kde.org)
   Copyright (C) 2007 Rafael Fernández López (ereslibre@kde.org)

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

#include "KexiListView.h"
#include "KexiListView_p.h"

#include <QApplication>
#include <QPainter>
#include <QTextLayout>

KexiListView::KexiListView(QWidget *parent)
    : QListView(parent)
{
    setViewMode(QListView::ListMode);
    setMovement(QListView::Static);
    setVerticalScrollMode(QListView::ScrollPerPixel);
    setItemDelegate(new KexiListViewDelegate(this));
}

KexiListView::~KexiListView()
{
}

void KexiListView::setModel(QAbstractItemModel *model)
{
    /*
      KPageListViewProxy *proxy = new KPageListViewProxy( this );
      proxy->setSourceModel( model );
      proxy->rebuildMap();

      connect( model, SIGNAL(layoutChanged()), proxy, SLOT(rebuildMap()) );
    */
    connect(model, SIGNAL(layoutChanged()), this, SLOT(updateWidth()));

//  QListView::setModel( proxy );
    QListView::setModel(model);

    // Set our own selection model, which won't allow our current selection to be cleared
    setSelectionModel(new KexiListViewSelectionModel(model, this));

    updateWidth();
}

void KexiListView::updateWidth()
{
    if (!model()) {
        return;
    }

    int rows = model()->rowCount();

    int width = 0;
    for (int i = 0; i < rows; ++i) {
        width = qMax(width, sizeHintForIndex(model()->index(i, 0)).width());
    }

    setFixedWidth(width + 25);
}

// ----

KexiListViewDelegate::KexiListViewDelegate(QObject *parent)
    : QAbstractItemDelegate(parent)
{
}

static int layoutText(QTextLayout *layout, int maxWidth)
{
    qreal height = 0;
    int textWidth = 0;
    layout->beginLayout();
    while (true) {
        QTextLine line = layout->createLine();
        if (!line.isValid()) {
            break;
        }
        line.setLineWidth(maxWidth);
        line.setPosition(QPointF(0, height));
        height += line.height();
        textWidth = qMax(textWidth, qRound(line.naturalTextWidth() + 0.5));
    }
    layout->endLayout();
    return textWidth;
}

void KexiListViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    if (!index.isValid()) {
        return;
    }

    QStyleOptionViewItemV4 opt(option);
    opt.showDecorationSelected = true;
    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();

    int iconSize = style->pixelMetric(QStyle::PM_IconViewIconSize);
    const QString text = index.model()->data(index, Qt::DisplayRole).toString();
    const QIcon icon = index.model()->data(index, Qt::DecorationRole).value<QIcon>();
    const QPixmap pixmap = icon.pixmap(iconSize, iconSize);

    QFontMetrics fm = painter->fontMetrics();
    int wp = pixmap.width() / pixmap.devicePixelRatio();
    int hp = pixmap.height() / pixmap.devicePixelRatio();

    QTextLayout iconTextLayout(text, option.font);
    QTextOption textOption(Qt::AlignHCenter);
    iconTextLayout.setTextOption(textOption);
    int maxWidth = qMax(3 * wp, 8 * fm.height());
    layoutText(&iconTextLayout, maxWidth);

    QPen pen = painter->pen();
    QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
                              ? QPalette::Normal : QPalette::Disabled;
    if (cg == QPalette::Normal && !(option.state & QStyle::State_Active)) {
        cg = QPalette::Inactive;
    }

    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);
    if (option.state & QStyle::State_Selected) {
        painter->setPen(option.palette.color(cg, QPalette::HighlightedText));
    } else {
        painter->setPen(option.palette.color(cg, QPalette::Text));
    }

    painter->drawPixmap(option.rect.x() + (option.rect.width() / 2)
                        - (wp / 2), option.rect.y() + 5, pixmap);
    if (!text.isEmpty()) {
        iconTextLayout.draw(painter, QPoint(option.rect.x() + (option.rect.width() / 2)
                                            - (maxWidth / 2), option.rect.y() + hp + 7));
    }

    painter->setPen(pen);

    drawFocus(painter, option, option.rect);
}

QSize KexiListViewDelegate::sizeHint(const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QSize(0, 0);
    }

    QStyleOptionViewItemV4 opt(option);
    opt.showDecorationSelected = true;
    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();

    int iconSize = style->pixelMetric(QStyle::PM_IconViewIconSize);
    const QString text = index.model()->data(index, Qt::DisplayRole).toString();
    const QIcon icon = index.model()->data(index, Qt::DecorationRole).value<QIcon>();
    const QPixmap pixmap = icon.pixmap(iconSize, iconSize);

    QFontMetrics fm = option.fontMetrics;
    int gap = 0; //fm.height();
    int wp = pixmap.width() / pixmap.devicePixelRatio();
    int hp = pixmap.height() / pixmap.devicePixelRatio();

    if (hp == 0) {
        /**
         * No pixmap loaded yet, we'll use the default icon size in this case.
         */
        hp = iconSize;
        wp = iconSize;
    }

    QTextLayout iconTextLayout(text, option.font);
    int wt = layoutText(&iconTextLayout, qMax(3 * wp, 8 * fm.height()));
    int ht = iconTextLayout.boundingRect().height();

    int width, height;
    if (text.isEmpty()) {
        height = hp;
    } else {
        height = hp + ht + 10;
    }

    width = qMax(wt, wp) + gap;

    return QSize(width, height);
}

void KexiListViewDelegate::drawFocus(QPainter *painter, const QStyleOptionViewItem &option,
                                     const QRect &rect) const
{
    if (option.state & QStyle::State_HasFocus) {
        QStyleOptionFocusRect o;
        o.QStyleOption::operator=(option);
        o.rect = rect;
        o.state |= QStyle::State_KeyboardFocusChange;
        QPalette::ColorGroup cg = (option.state & QStyle::State_Enabled)
                                  ? QPalette::Normal : QPalette::Disabled;
        o.backgroundColor = option.palette.color(cg, (option.state & QStyle::State_Selected)
                            ? QPalette::Highlight : QPalette::Background);
        QApplication::style()->drawPrimitive(QStyle::PE_FrameFocusRect, &o, painter);
    }
}

// ----

KexiListViewSelectionModel::KexiListViewSelectionModel(QAbstractItemModel *model, QObject *parent)
    : QItemSelectionModel(model, parent)
{
}

void KexiListViewSelectionModel::clear()
{
    // Don't allow the current selection to be cleared
}

void KexiListViewSelectionModel::select(const QModelIndex &index,
                                        QItemSelectionModel::SelectionFlags command)
{
    // Don't allow the current selection to be cleared
    if (!index.isValid() && (command & QItemSelectionModel::Clear)) {
        return;
    }
    QItemSelectionModel::select(index, command);
}

void KexiListViewSelectionModel::select(const QItemSelection &selection,
                                        QItemSelectionModel::SelectionFlags command)
{
    // Don't allow the current selection to be cleared
    if (!selection.count() && (command & QItemSelectionModel::Clear)) {
        return;
    }
    QItemSelectionModel::select(selection, command);
}
