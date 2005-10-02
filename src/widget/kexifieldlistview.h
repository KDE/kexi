/* This file is part of the KDE project
   Copyright (C) 2003-2005 Jaroslaw Staniek <js@iidea.pl>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef KEXIFIELDLISTVIEW_H
#define KEXIFIELDLISTVIEW_H

#include <qframe.h>
#include <qpixmap.h>
#include <klistview.h>

namespace KexiDB {
	class TableOrQuerySchema;
}

/*! This widget provides a list of fields from a table or query.
*/
class KEXIEXTWIDGETS_EXPORT KexiFieldListView : public KListView
{
	Q_OBJECT

	public:
		//! Flags used to alter list's behaviour and appearance
		enum Options { 
			ShowDataTypes = 1, //!< if set, 'data type' column is added
			ShowAsterisk = 2, //!< if set, asterisk ('*') item is prepended to the list
			AllowMultiSelection = 4 //!< if set, multiple selection is allowed
		};

		KexiFieldListView(QWidget *parent, const char *name = 0, 
			int options = ShowDataTypes | AllowMultiSelection );
		virtual ~KexiFieldListView();

		/*! Sets table or query schema \a schema. 
		 The schema object will be owned by the KexiFieldListView object. */
		void setSchema(KexiDB::TableOrQuerySchema* schema);

		KexiDB::TableOrQuerySchema* schema() const { return m_schema; }

//		void setReadOnly(bool);
//		virtual QSize sizeHint();

	protected:
		virtual QDragObject *dragObject();

		KexiDB::TableOrQuerySchema* m_schema;
		QPixmap m_keyIcon; //!< a small "primary key" icon for 0-th column
		QPixmap m_noIcon; //!< blank icon of the same size as m_keyIcon
		int m_options;
};

#endif
