/* This file is part of the KDE project
   Copyright (C) 2006 Sebastian Sauer <mail@dipe.org>

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
   Boston, MA 02110-1301, USA.
*/

#ifndef KEXIMACRODESIGNVIEW_H
#define KEXIMACRODESIGNVIEW_H

#include "keximacroview.h"

// Forward declarations.
namespace KoMacro {
	class Action;
	class Macro;
	class MacroItem;
}
namespace KoProperty {
	class Property;
}
namespace KexiDB {
	class ResultInfo;
}
class KexiTableItem;

/**
 * The KexiScriptDesignView implements \a KexiMacroView to provide
 * a GUI-Editor to edit a Macro.
 */
class KexiMacroDesignView : public KexiMacroView
{
		Q_OBJECT
	public:

		/**
		* Constructor.
		*
		* \param mainwin The \a KexiMainWindow instance this \a KexiViewBase
		*        belongs to.
		* \param parent The parent widget this widget should be displayed in.
		* \param macro The \a KoMacro::Macro instance this view is for.
		*/
		KexiMacroDesignView(KexiMainWindow *mainwin, QWidget *parent, ::KoMacro::Macro* const macro);

		/**
		* Destructor.
		*/
		virtual ~KexiMacroDesignView();

		/**
		* Load the data from XML source and fill the internally
		* used \a KoMacro::Macro instance.
		*/
		virtual bool loadData();

		/**
		* \return the \a KoProperty::Set properties this view provides.
		*/
		virtual KoProperty::Set* propertySet();

	private slots:

		/**
		* Called before a cell changed in the internaly used
		* \a KexiTableView .
		*/
		void beforeCellChanged(KexiTableItem*, int, QVariant&, KexiDB::ResultInfo*);

		/**
		* Called if the passed \p item got updated.
		*/
		void rowUpdated(KexiTableItem* item);

		/**
		* Called if a row got deleted.
		*/
		void rowDeleted();

		/**
		* Called if a row got inserted.
		*/
		void rowInserted(KexiTableItem* item, uint row, bool repaint);

		/**
		* Called if a property in the \a KoProperty got changed.
		*/
		void propertyChanged(KoProperty::Set&, KoProperty::Property&);

		/**
		* Reloads the propertyset delayed.
		*/
		void reloadPropertyLater();

	private:
		/// \internal d-pointer class.
		class Private;
		/// \internal d-pointer instance.
		Private* const d;

		/**
		* Update the table's data.
		*/
		void updateData();

		/**
		* Update the \a KoProperty::Set set with the passed \a KoMacro::MacroItem
		* \p item and the variablename \p variablename .
		*/
		bool updateSet(KoProperty::Set* set, KSharedPtr<KoMacro::MacroItem> item, const QString& variablename);

		/**
		* Update the properties of the \a KoProperty::Set \p set at
		* row-number \p row with the \a KoMacro::MacroItem \p macroitem .
		*/
		void updateProperties(int row, KoProperty::Set* set, KSharedPtr<KoMacro::MacroItem> macroitem);
};

#endif
