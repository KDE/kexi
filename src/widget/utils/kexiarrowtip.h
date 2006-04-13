/* This file is part of the KDE project
   Copyright (C) 2006 Jaroslaw Staniek <js@iidea.pl>

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

   Original Author:  Till Busch <till@bux.at>
   Original Project: buX (www.bux.at)
*/

#ifndef KEXIARROWTIP_H
#define KEXIARROWTIP_H

#include <qwidget.h>

//! \brief A tooltip-like widget with additional arrow
/*! The widget also suppors fade in and fade out effect, 
 if the underlying display system supports this.
*/
class KEXIGUIUTILS_EXPORT KexiArrowTip : public QWidget
{
	Q_OBJECT
	public:
		KexiArrowTip(const QString& text, QWidget* parent);
		virtual ~KexiArrowTip();

		inline QString text() const { return m_text; }
		virtual bool close() { return close(false); }
		virtual bool close( bool alsoDelete );

	public slots:
		virtual void show();
		virtual void hide();

	protected slots:
		void increaseOpacity();
		void decreaseOpacity();

	protected:
		virtual void paintEvent( QPaintEvent *pev );
//		virtual void closeEvent( QCloseEvent * e );
		void draw(QPainter& p);

		QString m_text;
		int m_arrowHeight;
		double m_opacity;
};

#endif
