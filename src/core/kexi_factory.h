/* This file is part of the KDE project
   Copyright (C) 2002, 2003 Joseph Wenninger <jowenn@kde.org>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef KEXI_FACTORY_H
#define KEXI_FACTORY_H

#include <koFactory.h>

class KInstance;
class KAboutData;

class KexiFactory : public KoFactory
{
    Q_OBJECT
public:
    KexiFactory( QObject* parent = 0, const char* name = 0 );
    ~KexiFactory();

    virtual KParts::Part *createPartObject( QWidget *parentWidget = 0, const char *widgetName = 0,
		QObject *parent = 0, const char *name = 0, const char *classname = "KoDocument",
		const QStringList &args = QStringList() );

    static KInstance* global();

    // _Creates_ a KAboutData but doesn't keep ownership
    static KAboutData* aboutData();

private:
    static KInstance* s_global;
    static KAboutData* s_aboutData;
};

#endif
