/* This file is part of the KDE project
Copyright (C) 2002   Lucijan Busch <lucijan@gmx.at>

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
the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.
*/

#ifndef KEXICREATEPROJECTPAGE_H
#define KEXICREATEPROJECTPAGE_H

#include <qwidget.h>
#include <qmap.h>
#include <qvariant.h>

class KexiCreateProject;
class KexiProject;

typedef QMap<QString, QVariant> DataMap;

class KEXIPRJWIZARD_EXPORT KexiCreateProjectPage : public QWidget
{
	Q_OBJECT

	public:
		KexiCreateProjectPage(KexiCreateProject *parent, QPixmap *wpic, const char *name=0);
		~KexiCreateProjectPage();

		QVariant data(const QString &property) const;
		void setProperty(QString property, QVariant data);

		bool	m_loaded;
		
	protected:
		DataMap	m_data;
		KexiProject *project();
	private:
		KexiProject *m_project;
	signals:
		void valueChanged(KexiCreateProjectPage *, QString &);
		void acceptPage();
};

#endif
