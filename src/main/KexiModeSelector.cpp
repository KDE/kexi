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
#include <kexiutils/KexiIcon.h>

#include <KLocalizedString>

#include <QApplication>

KexiModeSelectorModel::KexiModeSelectorModel(QObject *parent)
    : QAbstractListModel(parent)
{
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

    if (role == Qt::DisplayRole) {
        return modes.at(index.row()).name;
    } else if (role == Qt::DecorationRole) {
        return modes.at(index.row()).icon;
    }
    return QVariant();
}

// ----

KexiModeSelector::KexiModeSelector(QWidget *parent)
 : KexiListView(parent)
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

    KexiMode welcomeMode;
    welcomeMode.name = xi18n("Welcome");
    welcomeMode.icon = qApp->windowIcon();
    model.modes << welcomeMode;
    KexiMode projectMode;
    projectMode.name = xi18nc("Project mode", "Project");
    projectMode.icon = koIcon("project-development");
    model.modes << projectMode;
    setModel(&model);
}

KexiModeSelector::~KexiModeSelector()
{
}
