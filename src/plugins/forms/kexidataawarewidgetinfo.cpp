/* This file is part of the KDE project
   Copyright (C) 2005 Jarosław Staniek <staniek@kde.org>

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

#include "kexidataawarewidgetinfo.h"

class KexiDataAwareWidgetInfo::Private
{
public:
    Private() : inlineEditingEnabledWhenDataSourceSet(true) {}
    bool inlineEditingEnabledWhenDataSourceSet;
};

KexiDataAwareWidgetInfo::KexiDataAwareWidgetInfo(KFormDesigner::WidgetFactory *f)
        : KFormDesigner::WidgetInfo(f)
        , d( new Private )
{
    init();
}

KexiDataAwareWidgetInfo::~KexiDataAwareWidgetInfo()
{
    delete d;
}

void KexiDataAwareWidgetInfo::init()
{
    setAutoSyncForProperty("dataSource", false);
    setAutoSyncForProperty("dataSourcePartClass", false);
}

void KexiDataAwareWidgetInfo::setInlineEditingEnabledWhenDataSourceSet(bool set)
{
    d->inlineEditingEnabledWhenDataSourceSet = set;
}

bool KexiDataAwareWidgetInfo::inlineEditingEnabledWhenDataSourceSet() const
{
    return d->inlineEditingEnabledWhenDataSourceSet;
}
