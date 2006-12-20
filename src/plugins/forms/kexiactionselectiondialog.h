/* This file is part of the KDE project
   Copyright (C) 2005-2006 Jaroslaw Staniek <js@iidea.pl>

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

#ifndef KEXIACTIONSELECTIONDIALOG_H
#define KEXIACTIONSELECTIONDIALOG_H

#include <kdialogbase.h>
#include "kexiformeventhandler.h"

class KexiMainWindow;
class KListView;
namespace KexiPart {
	class Item;
}

//! @short A dialog for selecting an action to be executed for a form's command button
/*! Available actions are:
 - application's global actions like "edit->copy" (KAction-based)
 - opening/printing/executing of selected object (table/query/form/script/macrto, etc.)
*/
class KEXIFORMUTILS_EXPORT KexiActionSelectionDialog : public KDialogBase
{
	Q_OBJECT
	public:
		KexiActionSelectionDialog(KexiMainWindow* mainWin, QWidget *parent, 
			const KexiFormEventAction::ActionData& action, const QCString& actionWidgetName);
		~KexiActionSelectionDialog();

		/*! \return selected action data or empty action if dialog has been rejected 
		 or "No action" has been selected. */
		KexiFormEventAction::ActionData currentAction() const;

		//! \return the \a KexiMainWindow instance.
		KexiMainWindow* mainWin() const;

		virtual bool eventFilter(QObject *o, QEvent *e);

	protected slots:
		void slotActionCategorySelected(QListViewItem* item);
		void slotKActionItemExecuted(QListViewItem*);
		void slotKActionItemSelected(QListViewItem*);
		void slotActionToExecuteItemExecuted(QListViewItem* item);
		void slotActionToExecuteItemSelected(QListViewItem*);
		void slotCurrentFormActionItemExecuted(QListViewItem*);
		void slotCurrentFormActionItemSelected(QListViewItem*);
		void slotItemForOpeningOrExecutingSelected(KexiPart::Item* item);

	protected:
		void updateOKButtonStatus();

		class KexiActionSelectionDialogPrivate;
		KexiActionSelectionDialogPrivate* d;
};

#endif
