/* This file is part of the KDE project
   Copyright (C) 2005 Jaroslaw Staniek <js@iidea.pl>

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

#ifndef KEXIDATAAWAREWIDGETINFO_H
#define KEXIDATAAWAREWIDGETINFO_H

#include <widgetfactory.h>

//! A widget info for data-aware widgets
/*! Used within factories just like KFormDesigner::WidgetInfo,
 but also predefines specific behaviour, 
 e.g. sets autoSync flag to false for "dataSource" property.
*/
class KEXIFORMUTILS_EXPORT KexiDataAwareWidgetInfo : public KFormDesigner::WidgetInfo
{
	public:
		KexiDataAwareWidgetInfo(KFormDesigner::WidgetFactory *f = 0);
		virtual ~KexiDataAwareWidgetInfo();
};

#endif
