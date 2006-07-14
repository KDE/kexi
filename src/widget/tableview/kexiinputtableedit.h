/* This file is part of the KDE project
   Copyright (C) 2002 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2003-2004 Jaroslaw Staniek <js@iidea.pl>

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

#ifndef KEXIINPUTTABLEEDIT_H
#define KEXIINPUTTABLEEDIT_H

#include <klineedit.h>
#include <qvariant.h>

#include "kexitableedit.h"
#include "kexicelleditorfactory.h"

/*! @short General purpose cell editor using line edit widget.
*/
class KexiInputTableEdit : public KexiTableEdit
{
	Q_OBJECT

	public:
		KexiInputTableEdit(KexiTableViewColumn &column, QScrollView *parent=0);

		virtual ~KexiInputTableEdit();

		virtual bool valueChanged();

		//! \return true if editor's value is null (not empty)
		virtual bool valueIsNull();

		//! \return true if editor's value is empty (not null). 
		//! Only few field types can accept "EMPTY" property 
		//! (check this with KexiDB::Field::hasEmptyProperty()), 
		virtual bool valueIsEmpty();

		virtual QVariant value();

		virtual bool cursorAtStart();
		virtual bool cursorAtEnd();

//		virtual bool eventFilter(QObject* watched, QEvent* e);
//js		void end(bool mark);
//js		void backspace();
		virtual void clear();

		/*! \return total size of this editor, including any buttons, etc. (if present). */
		virtual QSize totalSize();

		/*! Handles action having standard name \a actionName. 
		 Action could be: "edit_cut", "edit_paste", etc. */
		virtual void handleAction(const QString& actionName);

		/*! Handles copy action for value. The \a value is copied to clipboard in format appropriate 
		 for the editor's impementation, e.g. for image cell it can be a pixmap. 
		 Reimplemented after KexiTableEdit. */
		virtual void handleCopyAction(const QVariant& value);

	protected slots:
		void setRestrictedCompletion();
		void completed(const QString &);

	protected:
		//! initializes this editor with \a add value
		virtual void setValueInternal(const QVariant& add, bool removeOld);

		void showHintButton();
		void init();
		virtual void paintEvent( QPaintEvent *e );

		/*! \return text for \a value. \a add is a text that should be added to the value if possible.
		 If \a setValidator is true, an appropriate validator will be setup for the internal line edit 
		 widget when needed. */
		QString valueToText(const QVariant& value, const QString& add, bool setValidator = false);

		bool m_calculatedCell;
		QString m_decsym; //! decimal symbol
		QString m_origText; //! orig. Line Edit's text after conversion - for easy comparing
		KLineEdit *m_lineedit;

	signals:
		void hintClicked();
};

KEXI_DECLARE_CELLEDITOR_FACTORY_ITEM(KexiInputEditorFactoryItem)

#endif
