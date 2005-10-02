/* This file is part of the KDE project
   Copyright (C) 2002, 2003 Joseph Wenninger <jowenn@kde.org>
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

#include "kexidragobjects.h"

#include <qcstring.h>
#include <qdatastream.h>
#include <kdebug.h>

/// implementation of KexiFieldDrag

KexiFieldDrag::KexiFieldDrag(const QString& sourceMimeType, const QString& sourceName, 
	const QString& field, QWidget *parent, const char *name)
 : QStoredDrag("kexi/field", parent, name)
{
	QByteArray data;
	QDataStream stream1(data,IO_WriteOnly);
	stream1 << sourceMimeType << sourceName << field;
	setEncodedData(data);
}

KexiFieldDrag::KexiFieldDrag(const QString& sourceMimeType, const QString& sourceName, 
	const QStringList& fields, QWidget *parent, const char *name)
 : QStoredDrag((fields.count() > 1) ? "kexi/fields" : "kexi/field", parent, name)
{
	QByteArray data;
	QDataStream stream1(data,IO_WriteOnly);
	if (fields.count() > 1)
		stream1 << sourceMimeType << sourceName << fields;
	else {
		QString field;
		if (fields.count() == 1)
			field = fields.first();
		else
			kexidbg << "KexiFieldDrag::KexiFieldDrag(): fields list is empty!" << endl;
		stream1 << sourceMimeType << sourceName << field;
	}
	setEncodedData(data);
}

KexiFieldDrag::~KexiFieldDrag()
{
}

bool
KexiFieldDrag::canDecodeSingle(QMimeSource *e)
{
	return e->provides("kexi/field");
}

bool
KexiFieldDrag::canDecodeMultiple(QMimeSource *e)
{
	return e->provides("kexi/field") || e->provides("kexi/fields");
}

bool
KexiFieldDrag::decodeSingle( QDropEvent* e, QString& sourceMimeType, 
	QString& sourceName, QString& field )
{
	QByteArray payload( e->data("kexi/field") );
	if (payload.isEmpty())
		return false;
	e->accept();
	QDataStream stream1(payload, IO_ReadOnly);
	stream1 >> sourceMimeType;
	stream1 >> sourceName;
	stream1 >> field;
//	kdDebug() << "KexiFieldDrag::decode() decoded: " << sourceMimeType<<"/"<<sourceName<<"/"<<field << endl;
	return true;
}

bool
KexiFieldDrag::decodeMultiple( QDropEvent* e, QString& sourceMimeType, 
	QString& sourceName, QStringList& fields )
{
	QByteArray payload( e->data("kexi/fields") );
	if (payload.isEmpty()) {//try single
		QString field;
		bool res = KexiFieldDrag::decodeSingle( e, sourceMimeType, sourceName, field );
		if (!res)
			return false;
		fields.append(field);
		return true;
	}
	e->accept();
	QDataStream stream1(payload, IO_ReadOnly);
	stream1 >> sourceMimeType;
	stream1 >> sourceName;
	stream1 >> fields;
//	kdDebug() << "KexiFieldDrag::decode() decoded: " << sourceMimeType<<"/"<<sourceName<<"/"<<fields << endl;
	return true;
}

/// implementation of KexiDataProviderDrag

KexiDataProviderDrag::KexiDataProviderDrag(const QString& sourceMimeType, const QString& sourceName,
	QWidget *parent, const char *name)
 : QStoredDrag("kexi/dataprovider", parent, name)
{
	QByteArray data;
	QDataStream stream1(data,IO_WriteOnly);
	stream1 << sourceMimeType << sourceName;
	setEncodedData(data);
}


bool
KexiDataProviderDrag::canDecode(QDragMoveEvent *e)
{
	return e->provides("kexi/dataprovider");
}

bool
KexiDataProviderDrag::decode( QDropEvent* e, QString& sourceMimeType, QString& sourceName)
{
	QCString tmp;
	QByteArray payload = e->data("kexidataprovider");
	if(payload.size())
	{
		e->accept();
		QDataStream stream1(payload, IO_ReadOnly);
		stream1 >> sourceMimeType;
		stream1 >> sourceName;
//		kdDebug() << "KexiDataProviderDrag::decode() decoded: " << sourceMimeType <<"/"<<sourceName<< endl;
		return true;
	}
	return false;
}
