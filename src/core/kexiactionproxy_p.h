/* This file is part of the KDE project
   Copyright (C) 2004 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIACTIONPROXY_P_H
#define KEXIACTIONPROXY_P_H

#include <qobject.h>

class KexiActionProxy;

//! Helper class for KexiActionProxy::plugSharedActionToExternalGUI() method.
class KAction_setEnabled_Helper : public QObject
{
    Q_OBJECT
public:
    KAction_setEnabled_Helper(KexiActionProxy* proxy);

public slots:
    void slotSetEnabled(bool enabled);

protected:
    KexiActionProxy *m_proxy;
};

#endif

