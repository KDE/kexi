/* This file is part of the KDE project
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2004-2005 Jaroslaw Staniek <js@iidea.pl>

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

#ifndef KexiDBSubForm_H
#define KexiDBSubForm_H

#include <qscrollview.h>
#include <formeditor/form.h>

//! @short A form embedded as a widget inside other form
class KEXIFORMUTILS_EXPORT KexiDBSubForm : public QScrollView
{
	Q_OBJECT
	Q_PROPERTY(QString formName READ formName WRITE setFormName DESIGNABLE true)

	public:
		KexiDBSubForm(KFormDesigner::Form *parentForm, QWidget *parent, const char *name);
		~KexiDBSubForm() {}

		//! \return the name of the subform to display inside this widget
		QString formName() const { return m_formName; }

		//! Sets the name of the subform to display inside this widget
		void setFormName(const QString &name);

		//void  paintEvent(QPaintEvent *ev);

	private:
		KFormDesigner::Form *m_parentForm;
		KFormDesigner::Form *m_form;
		QWidget *m_widget;
		QString m_formName;
};

#endif
