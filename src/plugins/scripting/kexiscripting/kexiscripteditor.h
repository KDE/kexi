/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2004-2005 Jaroslaw Staniek <js@iidea.pl>
   Copyright (C) 2005 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2005 Sebastian Sauer <mail@dipe.org>

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

#ifndef KEXISCRIPTEDITOR_H
#define KEXISCRIPTEDITOR_H

#include <kexieditor.h>

namespace Kross { namespace Api {
  class ScriptAction;
}}

/**
 * The KexiEditor class embeds text editor
 * for editing scripting code.
 */
class KexiScriptEditor : public KexiEditor
{
        Q_OBJECT

    public:

        /**
         * Constructor.
         */
        KexiScriptEditor(KexiMainWindow *mainWin, QWidget *parent, const char *name = 0);

        /**
         * Destructor.
         */
        virtual ~KexiScriptEditor();

        /**
         * \returns true if this editor is already initialized (\a initialize was
         * called) else false is returned.
         */
        bool isInitialized() const;

        /**
         * Initializes the editor. Call this if you like to start
         * with a clear editor instance. Thinks like the language
         * highlighter will be reset, undo/redo are cleared and
         * setDirty(false) is set.
         */
        void initialize(Kross::Api::ScriptAction* scriptaction);

    public slots:
        void slotTextChanged();
        void setLineNo(long);

    private:
        /// \internal d-pointer class.
        class Private;
        /// \internal d-pointer instance.
        Private* const d;
};

#endif
