/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2004 Jaroslaw Staniek <js@iidea.pl>

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

#ifndef LIBACTIONWIDGET_H
#define LIBACTIONWIDGET_H


#include <kactionclasses.h>

namespace KFormDesigner
{

class WidgetInfo;

/**
 * KToggleAction subclass which remembers the matching class name.
 */
class KFORMEDITOR_EXPORT LibActionWidget : public KToggleAction
{
	Q_OBJECT
	public:
		/** LibActionWidget object is initialized to be mutually
			exclusive with all other LibActionWidget objects */
		LibActionWidget(WidgetInfo *, KActionCollection *collection);
		virtual ~LibActionWidget();

	signals:
		/**
		 * emits a signal containing the class name
		 */
		void prepareInsert(const QCString &className);

	protected slots:
		/** reimplemented from KToggleAction */
		virtual void slotActivated();

	private:
		QCString m_className;
};

}

#endif
