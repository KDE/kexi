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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef DR_PROP_TEST_H
#define DR_PROP_TEST_H

int drPropTest()
{
	QValueList<QCString> names = driver->propertyNames();
	kdDebug() << QString("%1 properties found:").arg(names.count()) << endl;
	for (QValueList<QCString>::ConstIterator it = names.constBegin(); it!=names.constEnd(); ++it) {
		kdDebug() << " - " << (*it) << ":" 
			<< " caption=\"" << driver->propertyCaption(*it) << "\""
			<< " type=" << driver->propertyValue(*it).typeName() 
			<< " value=\""<<driver->propertyValue(*it).toString()<<"\"" << endl;
	}
//		QVariant propertyValue( const QCString& propName ) const;

//		QVariant propertyCaption( const QCString& propName ) const;

	return 0;
}

#endif

