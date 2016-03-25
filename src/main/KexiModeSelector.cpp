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
#include "KexiModeSelector_p.h"
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

    KexiModeData *mode = static_cast<KexiModeData*>(index.internalPointer());
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
    KexiModeData *mode = static_cast<KexiModeData*>(index.internalPointer());
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

class KexiModeSelector::Private
{
public:
    Private() {}
    KexiModeSelectorModel model;
};

KexiModeSelector::KexiModeSelector(QWidget *parent)
 : KexiListView(DontUseDelegate, parent), d(new Private)
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
    connect(this, &KexiModeSelector::currentChanged,
            this, &KexiModeSelector::currentModeChanged);

    KexiStyledIconParameters param;
    param.color = palette().color(QPalette::Text);
    param.selectedColor = palette().color(QPalette::HighlightedText);
    param.disabledColor = palette().color(QPalette::Disabled, QPalette::Text);
    {
        KexiModeData *welcomeMode = new KexiModeData;
        welcomeMode->name = xi18nc("Welcome mode", "Welcome");
        param.context = KIconLoader::Action;
        param.name = "mode-selector-welcome";
        welcomeMode->icon = KexiStyle::icon(param);
        d->model.modes << welcomeMode;
    }
    {
        KexiModeData *projectMode = new KexiModeData;
        projectMode->enabled = false;
        projectMode->name = xi18nc("Project mode", "Project");
        param.context = KIconLoader::Action;
        param.name = "mode-selector-project";
        projectMode->icon = KexiStyle::icon(param);
        d->model.modes << projectMode;
    }
    {
        KexiModeData *dataMode = new KexiModeData;
        dataMode->name = xi18nc("Data mode", "Data");
        param.context = KIconLoader::Action;
        param.name = "mode-selector-data";
        dataMode->icon = KexiStyle::icon(param);
        d->model.modes << dataMode;
    }
    {
        KexiModeData *designMode = new KexiModeData;
        designMode->name = xi18nc("Design mode", "Design");
        param.context = KIconLoader::Action;
        param.name = "mode-selector-design";
        designMode->icon = KexiStyle::icon(param);
        d->model.modes << designMode;
    }
    {
        KexiModeData *helpMode = new KexiModeData;
        helpMode->name = xi18nc("Help mode", "Help");
        param.context = KIconLoader::Action;
        param.name = "mode-selector-help";
        helpMode->icon = KexiStyle::icon(param);
        d->model.modes << helpMode;
    }
    setModel(&d->model);
    setCurrentMode(Kexi::WelcomeGlobalMode);
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

Kexi::GlobalViewMode KexiModeSelector::currentMode() const
{
    int index = currentIndex().row();
    if (index < 0 || index > Kexi::LastGlobalMode) {
        index = 0;
    }
    return static_cast<Kexi::GlobalViewMode>(index);
}

void KexiModeSelector::setCurrentMode(Kexi::GlobalViewMode mode)
{
    setCurrentIndex(model()->index(int(mode), 0));
}
