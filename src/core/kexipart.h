/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2003 Jaroslaw Staniek <js@iidea.pl>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef KEXIPART_H
#define KEXIPART_H

#include <qobject.h>
#include <qmap.h>

class KexiMainWindow;
class KexiDialogBase;
class KActionCollection;

namespace KexiPart
{
	class Info;
	class Item;
	class GUIClient;

/**
 * The main class for kexi frontend parts like tables, queries, relations
 */
class KEXICORE_EXPORT Part : public QObject
{
	Q_OBJECT

	public:
		
		Part(QObject *parent, const char *name, const QStringList &);
		virtual ~Part();

		KexiDialogBase* execute(KexiMainWindow *win, const KexiPart::Item &item);

		/*! i18n'd iunstance name usable for displaying in gui.
		 @todo move this to Info class when the name could be moved as localised property 
		 to service's .desktop file. */
		QString instanceName() const { return m_names["instance"]; }
		
		inline Info *info() const { return m_info; }

		inline GUIClient *guiClient() const { return m_guiClient; }

		inline GUIClient *instanceGuiClient() const { return m_instanceGuiClient; }

	protected:
		virtual KexiDialogBase* createInstance(KexiMainWindow *win, const KexiPart::Item &item) = 0;

		//! Creates GUICLient for this part, attached to \a win
		//! This method is called from KexiMainWindow
		void createGUIClient(KexiMainWindow *win);

		virtual void initPartActions( KActionCollection *col ) {};
		virtual void initInstanceActions( KActionCollection *col ) {};

		inline void setInfo(Info *info) { m_info = info; }

		//! Set of i18n'd action names for, initialised on KexiPart::Part subclass ctor
		//! The names are useful because the same action can have other name for each part
		//! E.g. "New table" vs "New query" can have different forms for some languages...
		QMap<QString,QString> m_names;

	private:
		Info *m_info;
		GUIClient *m_guiClient;
		GUIClient *m_instanceGuiClient;

	friend class Manager;
	friend class KexiMainWindow;
	friend class GUIClient;
};

}

#endif

