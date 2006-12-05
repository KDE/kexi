/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2004-2005 Jaroslaw Staniek <js@iidea.pl>
   Copyright (C) 2005 Cedric Pasteur <cedric.pasteur@free.fr>

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

#ifndef KEXIEDITOR_H
#define KEXIEDITOR_H

#include <qwidget.h>
#include "kexiviewbase.h"

class KTextEdit;
class KexiEditorPrivate;

namespace KTextEditor
{
	class Document;
	class View;
}

//! An text editor view that uses both KTextEditor and KTextEdit
/*! It is used for SQL and script editor. */
class KEXIEXTWIDGETS_EXPORT KexiEditor : public KexiViewBase
{
	Q_OBJECT

	public:

		/**
		* Constructor.
		* 
		* \param mainWin The \a KexiMainWindow instance this KexiEditor
		*        belongs too.
		* \param parent The parent \a QWidget this KexiEditor is child
		*        of.  You don't need to free the KexiEditor cause Qt 
		*        will handle that for us.
		* \param name The name this KexiEditor has. Used only for debugging.
		*/
		KexiEditor(KexiMainWindow *mainWin, QWidget *parent, const char *name = 0);

		/**
		* Destructor.
		*/
		virtual ~KexiEditor();

		/**
		* \return true if internally the KTextEditor::EditorChooser got
		* used else, if a simple KTextEdit is used, false is returned.
		*/
		static bool isAdvancedEditor();

		/**
		* \return the text displayed in the editor-widget.
		*/
		QString text();

		/**
		* Set the highlight-mode to \p highlightmodename . If
		* \a isAdvancedEditor returns false (KTextEdit is used
		* rather then KTextEditor), then the method just does
		* nothing. The \p highlightmodename could be any kind
		* of string like e.g. "python", "kjs" or "sql" 
		* KTextEditor supports.
		*/
		void setHighlightMode(const QString& highlightmodename);

		/**
		* Find row and column for this \p character and jump to the
		* position.
		*/
		void jump(int character);
		
		/**
		* Set the cursor position to \p line and \p col .
		*/
		void setCursorPosition(int line, int col);
		
		/**
		* Clear all remembered undo/redo-actions. Only
		* avaiable if \a isAdvancedEditor returns true.
		*/
		void clearUndoRedo();

	public slots:
		/*! Sets editor's text to \a text. 'Dirty' flag remains unchanged. 
		 Undo/redo buffer is cleared.*/
		void setText(const QString &text);
		/*! Display the configuration-dialog. Only avaiable if isAdvancedEditor() returns true. */
		void slotConfigureEditor();

	protected:
		/*! Update the actions. This call is redirected to \a KexiViewBase::updateActions */
		virtual void updateActions(bool activated);

	signals:
		/*! Emitted if the text displayed in the editor changed. */
		void textChanged();

	private:
		/*! Private d-pointer class. */
		KexiEditorPrivate *d;
};

#endif
