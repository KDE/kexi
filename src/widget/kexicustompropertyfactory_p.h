/* This file is part of the KDE project
   Copyright (C) 2005 Jaroslaw Staniek <js@iidea.pl>

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

#ifndef KEXICUSTOMPROPFACTORY_P_H
#define KEXICUSTOMPROPFACTORY_P_H

#include <koproperty/editors/pixmapedit.h>
#include <koproperty/editors/stringedit.h>
#include <kexiblobbuffer.h>

//! Kexi-specific image editor for property editor's item
class KexiImagePropertyEdit : public KoProperty::PixmapEdit
{
	Q_OBJECT

	public:
		KexiImagePropertyEdit(KoProperty::Property *property, 
			QWidget *parent=0, const char *name=0);
		virtual ~KexiImagePropertyEdit();

		virtual QVariant value() const;
		virtual void setValue(const QVariant &value, bool emitChange=true);
		virtual void drawViewer(QPainter *p, const QColorGroup &cg, const QRect &r, 
			const QVariant &value);

	public slots:
		virtual void selectPixmap();

	protected:
		KexiBLOBBuffer::Id_t m_id;
};

/*! Identifier editor based on ordinary string editor but always keeps a valid identifier 
 or empty value. It's line edit has IdentifierValidator::IdentifierValidator set, so user 
 is unable to enter invalid characters. Any chages to a null value or empty string, 
 have no effect. 

 @todo move this to koproperty library (when KexiUtils is moves to kofficecore)
 */
class KexiIdentifierPropertyEdit : public KoProperty::StringEdit
{
	Q_OBJECT

	public:
		KexiIdentifierPropertyEdit(KoProperty::Property *property, 
			QWidget *parent=0, const char *name=0);
		virtual ~KexiIdentifierPropertyEdit();

		/*! Reimplemented: sets \a value but it is converted to identifier
		 using KexiUtils::string2Identifier(). 
		 If \a value is null or empty string, this method has no effect. */
		virtual void setValue(const QVariant &value, bool emitChange=true);
};

#endif
