/* This file is part of the KDE project
   Copyright (C) 2003-2005 Jaroslaw Staniek <js@iidea.pl>

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

#ifndef KEXIUTILS_UTILS_H
#define KEXIUTILS_UTILS_H

#include "kexiutils_export.h"

#include <qguardedptr.h>
#include <qobjectlist.h>
#include <kmimetype.h>
class QColor;

// General Utils

namespace KexiUtils
{
	//! \return true if \a o has parent \a par.
	inline bool hasParent(QObject* par, QObject* o)
	{
		if (!o || !par)
			return false;
		while (o && o!=par)
			o = o->parent();
		return o==par;
	}

	//! \return parent object of \a o that inherits \a className or NULL if no such parent
	template<class type>
	inline type* findParent(QObject* o, const char* className)
	{
		if (!o || !className || className[0]=='\0')
			return 0;
		while ( ((o=o->parent())) && !o->inherits(className) )
			;
		return static_cast<type*>(o);
	}

	//! \return first found child of \a o, that inherit \a className.
	//! Returned pointer type is casted.
	template<class type>
	type* findFirstChild(QObject *o, const char* className)
	{
		if (!o || !className || className[0]=='\0')
			return 0;
		QObjectList *l = o->queryList( className );
		QObjectListIt it( *l );
		return static_cast<type*>(it.current());
	}

	//! QDateTime - a hack needed because QVariant(QTime) has broken isNull()
	inline QDateTime stringToHackedQTime(const QString& s)
	{
		if (s.isEmpty())
			return QDateTime();
	//		kdDebug() << QDateTime( QDate(0,1,2), QTime::fromString( s, Qt::ISODate ) ).toString(Qt::ISODate) << endl;;
		return QDateTime( QDate(0,1,2), QTime::fromString( s, Qt::ISODate ) );
	}

	/*! Sets "wait" cursor with 1 second delay (or 0 seconds if noDelay is true).
	 Does nothing if GUI is not GUI-aware. (see KApplication::guiEnabled()) */
	KEXIUTILS_EXPORT void setWaitCursor(bool noDelay = false);

	/*! Remove "wait" cursor previously set with \a setWaitCursor(), 
	 even if it's not yet visible.
	 Does nothing if GUI is not GUI-aware. (see KApplication::guiEnabled()) */
	KEXIUTILS_EXPORT void removeWaitCursor();

	/*! Helper class. Allocate it in yor code block as follows:
	 <code>
	 KexiUtils::WaitCursor wait;
	 </code>
	 .. and wait cursor will be visible (with a delay) until you're in this block. without 
	 a need to call removeWaitCursor() before exiting the block.
	 Does nothing if GUI is not GUI-aware. (see KApplication::guiEnabled()) */
	class KEXIUTILS_EXPORT WaitCursor
	{
		public:
			WaitCursor(bool noDelay = false);
			~WaitCursor();
	};

	/*! \return filter string in QFileDialog format for a mime type pointed by \a mime
	 If \a kdeFormat is true, QFileDialog-compatible filter string is generated, 
	 eg. "Image files (*.png *.xpm *.jpg)", otherwise KFileDialog -compatible 
	 filter string is generated, eg. "*.png *.xpm *.jpg|Image files (*.png *.xpm *.jpg)".
	 "\\n" is appended if \a kdeFormat is true, otherwise ";;" is appended. */
	KEXIUTILS_EXPORT QString fileDialogFilterString(const KMimeType::Ptr& mime, bool kdeFormat = true);

	/*! @overload QString fileDialogFilterString(const KMimeType::Ptr& mime, bool kdeFormat = true) */
	KEXIUTILS_EXPORT QString fileDialogFilterString(const QString& mimeString, bool kdeFormat = true);

	/*! Like QString fileDialogFilterString(const KMimeType::Ptr& mime, bool kdeFormat = true)
	 but returns a list of filter strings. */
	KEXIUTILS_EXPORT QString fileDialogFilterStrings(const QStringList& mimeStrings, bool kdeFormat);

	/*! \return a color being a result of blending \a c1 with \a c2 with \a factor1 
	 and \a factor1 factors: (c1*factor1+c2*factor2)/(factor1+factor2). */
	KEXIUTILS_EXPORT QColor blendedColors(const QColor& c1, const QColor& c2, int factor1 = 1, int factor2 = 1);

	/*! \return a contrast color for a color \a c: 
	 If \a c is light color, darker color created using c.dark(200) is returned; 
	 otherwise lighter color created using c.light(200) is returned. */
	KEXIUTILS_EXPORT QColor contrastColor(const QColor& c);

	/*! \return a lighter color for a color \a c and a factor \a factor.
	 For colors like Qt::red or Qt::green where hue and saturation are near to 255,
	 hue is decreased so the result will be more bleached. 
	 For black color the result is dark gray rather than black. */
	KEXIUTILS_EXPORT QColor bleachedColor(const QColor& c, int factor);

	/*! Serializes \a map to \a array.
	 KexiUtils::deserializeMap() can be used to deserialize this array back to map. */
	KEXIUTILS_EXPORT void serializeMap(const QMap<QString,QString>& map, QByteArray& array);
	KEXIUTILS_EXPORT void serializeMap(const QMap<QString,QString>& map, QString& string);

	/*! \return a map deserialized from a byte array \a array.
	 \a array need to contain data previously serialized using KexiUtils::serializeMap(). */
	KEXIUTILS_EXPORT QMap<QString,QString> deserializeMap(const QByteArray& array);

	/*! \return a map deserialized from \a string.
	 \a string need to contain data previously serialized using KexiUtils::serializeMap(). */
	KEXIUTILS_EXPORT QMap<QString,QString> deserializeMap(const QString& string);

	/*! \return a valid filename converted from \a string by:
	 - replacing \\, /, :, *, ?, ", <, >, |, \n \t characters with a space
	 - simplifing whitespace by removing redundant space characters using QString::simplifyWhiteSpace()
	 Do not pass full paths here, but only filename strings. */
	KEXIUTILS_EXPORT QString stringToFileName(const QString& string);

	/*! Performs a simple \a string  encrypttion using rot47-like algorithm. 
	 Each character's unicode value is increased by 47 + i (where i is index of the character). 
	 The resulting string still contains redable characters.
	 Do not use this for data that can be accessed by attackers! */
	KEXIUTILS_EXPORT void simpleCrypt(QString& string);

	/*! Performs a simple \a string decrypttion using rot47-like algorithm, 
	 using opposite operations to KexiUtils::simpleCrypt(). */
	KEXIUTILS_EXPORT void simpleDecrypt(QString& string);
}


//! sometimes we leave a space in the form of empty QFrame and want to insert here
//! a widget that must be instantiated by hand.
//! This macro inserts a widget \a what into a frame \a where.
#define GLUE_WIDGET(what, where) \
	{ QVBoxLayout *lyr = new QVBoxLayout(where); \
	  lyr->addWidget(what); }


#endif //KEXIUTILS_UTILS_H
