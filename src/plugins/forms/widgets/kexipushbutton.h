/* This file is part of the KDE project
   Copyright (C) 2005 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2004-2006 Jaroslaw Staniek <js@iidea.pl>

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

#ifndef KexiPushButton_H
#define KexiPushButton_H

#include <kpushbutton.h>
#include "../kexiformeventhandler.h"

//! @short Push Button widget for Kexi forms
class KEXIFORMUTILS_EXPORT KexiPushButton : public KPushButton
{
	Q_OBJECT
	Q_PROPERTY(QString onClickAction READ onClickAction WRITE setOnClickAction DESIGNABLE true)
	Q_PROPERTY(QString onClickActionOption READ onClickActionOption WRITE setOnClickActionOption DESIGNABLE true)

	public:
		KexiPushButton( const QString & text, QWidget * parent, const char * name = 0 );
		~KexiPushButton();

	public slots:
		//! action string for "on click" event
		//! @see KexiFormPart::slotAssignAction()
		//! @see KexiFormEventAction::ActionData
		QString onClickAction() const { return m_onClickActionData.string; }
		void setOnClickAction(const QString& actionString) { m_onClickActionData.string = actionString; }

		//! action option allowing to select whether the object should be opened in data view mode or printed, etc.
		//! @see KexiFormPart::slotAssignAction()
		//! @see KexiFormEventAction::ActionData
		QString onClickActionOption() const { return m_onClickActionData.option; }
		void setOnClickActionOption(const QString& option) { m_onClickActionData.option = option; }

	protected:
		KexiFormEventAction::ActionData m_onClickActionData;
};

#endif
