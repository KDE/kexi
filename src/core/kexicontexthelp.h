/* This file is part of the KDE project
   Copyright (C) 2003 Joseph Wenninger <jowenn@kde.org>

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

#ifndef KEXICONTEXTHELP_H
#define KEXICONTEXTHELP_H


#include "keximainwindow.h"
#include <KoContextHelp.h>

class KEXICORE_EXPORT KexiContextHelp : public KoContextHelpWidget
{
    Q_OBJECT

public:
    KexiContextHelp(KexiMainWindow *view, QWidget *parent = 0);
    ~KexiContextHelp();
private slots:
    void linkClickedInternal(const QString &link);

signals:
    void unhandledLink(const QString& link);
};

#endif
