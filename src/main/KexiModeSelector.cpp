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

#include "KexiModeSelector.h"
#include <kexiutils/KexiStyle.h>

#include <KLocalizedString>

#include <QDebug>
#include <QApplication>
#include <QPainter>

KexiModeSelectorModel::KexiModeSelectorModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

KexiModeSelectorModel::~KexiModeSelectorModel()
{
    qDeleteAll(modes);
}

int KexiModeSelectorModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return modes.count();
}

QVariant KexiModeSelectorModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= modes.size())
        return QVariant();

    KexiMode *mode = static_cast<KexiMode*>(index.internalPointer());
    switch (role) {
    case Qt::DisplayRole:
        return mode->name;
    case Qt::DecorationRole:
        return mode->icon;
    default:;
    }
    return QVariant();
}

Qt::ItemFlags KexiModeSelectorModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractListModel::flags(index);
    KexiMode *mode = static_cast<KexiMode*>(index.internalPointer());
    if (!mode->enabled) {
        flags &= ~(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    }
    return flags;
}

QModelIndex KexiModeSelectorModel::index(int row, int column, const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    if (row < 0 || row >= modes.count()) {
        return QModelIndex();
    }
    return createIndex(row, column, modes.at(row));
}

// ----

KexiModeSelectorDelegate::KexiModeSelectorDelegate(QObject *parent)
 : KexiListViewDelegate(parent)
{
}

void KexiModeSelectorDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const
{
    KexiListViewDelegate::paint(painter, option, index);
    KexiStyle::overpaintModeSelectorItem(painter, option, index);
}

// ----

KexiModeSelector::KexiModeSelector(QWidget *parent)
 : KexiListView(DontUseDelegate, parent)
{
    KexiStyle::setupModeSelector(this);
    setSpacing(0);
    setContentsMargins(0, 0, 0, 0);
    setFocusPolicy(Qt::NoFocus);
    setEditTriggers(NoEditTriggers);
    setDropIndicatorShown(false);
    setSelectionBehavior(SelectRows);
    setSelectionRectVisible(false);
    setUniformItemSizes(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    setItemDelegate(new KexiModeSelectorDelegate(this));

    KexiStyledIconParameters param;
    param.color = palette().color(QPalette::Text);
    param.selectedColor = palette().color(QPalette::HighlightedText);
    param.disabledColor = palette().color(QPalette::Disabled, QPalette::Text);
    {
        KexiMode *welcomeMode = new KexiMode;
        welcomeMode->name = xi18nc("Welcome mode", "Welcome");
        param.context = KIconLoader::Action;
        param.name = "mode-selector-welcome";
        welcomeMode->icon = KexiStyle::icon(param);
        m_model.modes << welcomeMode;
    }
    {
        KexiMode *projectMode = new KexiMode;
        projectMode->enabled = false;
        projectMode->name = xi18nc("Project mode", "Project");
        param.context = KIconLoader::Action;
        param.name = "mode-selector-project";
        projectMode->icon = KexiStyle::icon(param);
        m_model.modes << projectMode;
    }
    {
        KexiMode *dataMode = new KexiMode;
        dataMode->name = xi18nc("Data mode", "Data");
        param.context = KIconLoader::Action;
        param.name = "mode-selector-data";
        dataMode->icon = KexiStyle::icon(param);
        m_model.modes << dataMode;
    }
    {
        KexiMode *designMode = new KexiMode;
        designMode->name = xi18nc("Design mode", "Design");
        param.context = KIconLoader::Action;
        param.name = "mode-selector-design";
        designMode->icon = KexiStyle::icon(param);
        m_model.modes << designMode;
    }
    {
        KexiMode *helpMode = new KexiMode;
        helpMode->name = xi18nc("Help mode", "Help");
        param.context = KIconLoader::Action;
        param.name = "mode-selector-help";
        helpMode->icon = KexiStyle::icon(param);
        m_model.modes << helpMode;
    }
    setModel(&m_model);
    selectionModel()->select(model()->index(0, 0), QItemSelectionModel::Select);
}

KexiModeSelector::~KexiModeSelector()
{
}

void KexiModeSelector::paintEvent(QPaintEvent *event)
{
    KexiListView::paintEvent(event);

    QRect selectedRect;
    if (!selectedIndexes().isEmpty()) {
        selectedRect = visualRect(selectedIndexes().first());
    }
    QPainter painter(viewport());
    KexiStyle::overpaintModeSelector(this, &painter, selectedRect);
}
