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

#include "kexiblobbuffer.h"

#include <assert.h>

#include <qfile.h>
#include <qfileinfo.h>
#include <qbuffer.h>

#include <kdebug.h>
#include <kstaticdeleter.h>
#include <kimageio.h>

#include <kexidb/connection.h>

static KStaticDeleter<KexiBLOBBuffer> m_bufferDeleter;
static KexiBLOBBuffer* m_buffer = 0;

//-----------------

class KexiBLOBBuffer::Private
{
	public:
		Private()
		 : maxId(0)
		 , inMemoryItems(1009)
		 , storedItems(1009)
		 , itemsByURL(1009)
		{
		}
		Id_t maxId; //!< Used to compute maximal recently used identifier for unstored BLOB
//! @todo will be changed to QHash<quint64, Item>
		QIntDict<Item> inMemoryItems; //!< for unstored BLOBs
		QIntDict<Item> storedItems; //!< for stored items
		QDict<Item> itemsByURL;
		QGuardedPtr<KexiDB::Connection> conn;
};

//-----------------

KexiBLOBBuffer::Handle::Handle(Item* item)
 : m_item(item)
{
	if (m_item)
		m_item->refs++;
}

KexiBLOBBuffer::Handle::Handle(const Handle& handle)
{
	*this = handle;
}

KexiBLOBBuffer::Handle::Handle()
 : m_item(0)
{
}

KexiBLOBBuffer::Handle::~Handle()
{
	if (m_item) {
		m_item->refs--;
		if (m_item->refs<=0)
			KexiBLOBBuffer::self()->removeItem(m_item->id, m_item->stored);
	}
}

KexiBLOBBuffer::Handle& KexiBLOBBuffer::Handle::operator=(const Handle& handle)
{
	m_item = handle.m_item;
	if (m_item)
		m_item->refs++;
	return *this;
}

void KexiBLOBBuffer::Handle::setStoredWidthID(KexiBLOBBuffer::Id_t id)
{
	if (!m_item)
		return;
	if (m_item->stored) {
		kdWarning() << "KexiBLOBBuffer::Handle::setStoredWidthID(): object for id=" << id 
			<< " is aleady stored" << endl;
		return;
	}

	KexiBLOBBuffer::self()->takeItem(m_item);
	m_item->id = id; //new id
	m_item->stored = true;
//! @todo What about other handles for this item? 
//! @todo They were assuming it's unstored item, but it's stored now....
	KexiBLOBBuffer::self()->insertItem(m_item);
}

//-----------------

KexiBLOBBuffer::Item::Item(const QByteArray& data, KexiBLOBBuffer::Id_t ident, bool _stored,
	const QString& _name, const QString& _caption, const QString& _mimeType, 
	Id_t _folderId, const QPixmap& pixmap)
 : name(_name), caption(_caption), mimeType(_mimeType), refs(0), 
 id(ident), folderId(_folderId), stored(_stored),
 m_pixmapLoaded(new bool(false)/*workaround for pixmap() const*/)
{
	if (pixmap.isNull())
		m_pixmap = new QPixmap();
	else
		m_pixmap = new QPixmap(pixmap);

	if (data.isEmpty())
		m_data = new QByteArray();
	else
		m_data = new QByteArray(data);
}

KexiBLOBBuffer::Item::~Item()
{
	kexipluginsdbg << "KexiBLOBBuffer::Item::~Item()" << endl;
	delete m_pixmap;
	m_pixmap = 0;
	delete m_data;
	m_data = 0;
	delete m_pixmapLoaded;
}

QPixmap KexiBLOBBuffer::Item::pixmap() const
{
	if (!*m_pixmapLoaded && m_pixmap->isNull() && !m_data->isEmpty()) {
		QString type( KImageIO::typeForMime(mimeType) );
		if (!KImageIO::canRead( type ) || !m_pixmap->loadFromData(*m_data, type.latin1())) {
			//! @todo inform about error?
		}
		*m_pixmapLoaded = true;
	}
	return *m_pixmap;
}

QByteArray KexiBLOBBuffer::Item::data() const
{
	if (!m_data->isEmpty())
		return *m_data;

	if (m_data->isEmpty() && m_pixmap->isNull())
		return QByteArray();
	
	if (m_data->isEmpty() && !m_pixmap->isNull()) {
		//convert pixmap to byte array
		//(do it only on demand)
		QBuffer buffer( *m_data );
		buffer.open( IO_WriteOnly );
		m_pixmap->save( &buffer, mimeType.isEmpty() ? (const char*)"PNG"/*! @todo default? */ : mimeType.latin1() );
	}
	return *m_data;
}

//-----------------

KexiBLOBBuffer::KexiBLOBBuffer()
 : QObject()
 , d(new Private())
{
	Q_ASSERT(!m_buffer);
	d->inMemoryItems.setAutoDelete(true);
	d->storedItems.setAutoDelete(true);
}

KexiBLOBBuffer::~KexiBLOBBuffer()
{
	delete d;
}

KexiBLOBBuffer::Handle KexiBLOBBuffer::insertPixmap(const KURL& url)
{
	if (url.isEmpty() )
		return KexiBLOBBuffer::Handle();
	if (!url.isValid()) {
		kexipluginswarn << "::insertPixmap: INVALID URL '" << url << "'" << endl;
		return KexiBLOBBuffer::Handle();
	}
//! @todo what about searching by filename only and then compare data?
	Item * item = d->itemsByURL.find(url.prettyURL());
	if (item)
		return KexiBLOBBuffer::Handle(item);

	QString fileName = url.isLocalFile() ? url.path() : url.prettyURL();
//! @todo download the file if remote, then set fileName properly
	QFile f(fileName);
	if (!f.open(IO_ReadOnly)) {
		//! @todo err msg
		return KexiBLOBBuffer::Handle();
	}
	QString mimeType( KImageIO::mimeType( fileName ) );

	QByteArray data( f.readAll() );
	if (f.status()!=IO_Ok) {
		//! @todo err msg
		return KexiBLOBBuffer::Handle();
	}
	QFileInfo fi(url.fileName());
	QString caption(fi.baseName().replace('_', " ").simplifyWhiteSpace());

	item = new Item(data, ++d->maxId, /*!stored*/false, url.fileName(), caption, mimeType);
	insertItem(item);

	//cache
	item->prettyURL = url.prettyURL();
	d->itemsByURL.replace(url.prettyURL(), item);
	return KexiBLOBBuffer::Handle(item);
}

KexiBLOBBuffer::Handle KexiBLOBBuffer::insertObject(const QByteArray& data, 
	const QString& name, const QString& caption, const QString& mimeType, KexiBLOBBuffer::Id_t identifier)
{
	KexiBLOBBuffer::Id_t newIdentifier;
	if (identifier>0)
		newIdentifier = identifier;
	else
		newIdentifier = ++d->maxId;

	Item *item = new Item(data, newIdentifier, identifier>0, name, caption, mimeType);
	insertItem( item );
	return KexiBLOBBuffer::Handle(item);
}

KexiBLOBBuffer::Handle KexiBLOBBuffer::insertPixmap(const QPixmap& pixmap)
{
	if (pixmap.isNull())
		return KexiBLOBBuffer::Handle();

	Item * item = new Item(
		QByteArray(), //(pixmap will be converted to byte array on demand)
		++d->maxId, 
		false, //not stored
		QString::null, 
		QString::null, 
		"image/png", //!< @todo OK? What about jpegs?
		0, //folder id
		pixmap);

	insertItem(item);
	return KexiBLOBBuffer::Handle(item);
}

KexiBLOBBuffer::Handle KexiBLOBBuffer::objectForId(Id_t id, bool stored)
{
	if (id<=0)
		return KexiBLOBBuffer::Handle();
	if (stored) {
		Item *item = d->storedItems.find(id);
		if (item || !d->conn)
			return KexiBLOBBuffer::Handle(item);
		//retrieve stored BLOB:

//#if 0
		assert(d->conn);
		KexiDB::TableSchema *blobsTable = d->conn->tableSchema("kexi__blobs"); 
		if (!blobsTable) {
		//! @todo err msg
			return KexiBLOBBuffer::Handle();
		}
/*		QStringList where;
		where << "o_id";
		KexiDB::PreparedStatement::Ptr st = d->conn->prepareStatement(
			KexiDB::PreparedStatement::SelectStatement, *blobsTable, where);*/
//! @todo use PreparedStatement
		KexiDB::QuerySchema schema;
		schema.addField( blobsTable->field("o_data") );
		schema.addField( blobsTable->field("o_name") );
		schema.addField( blobsTable->field("o_caption") );
		schema.addField( blobsTable->field("o_mime") );
		schema.addField( blobsTable->field("o_folder_id") );
		schema.addToWhereExpression(blobsTable->field("o_id"), QVariant((Q_LLONG)id));

		KexiDB::RowData rowData;
		tristate res = d->conn->querySingleRecord(
			schema,
//			QString::fromLatin1("SELECT o_data, o_name, o_caption, o_mime FROM kexi__blobs where o_id=")
//			+QString::number(id), 
			rowData); 
		if (res!=true || rowData.size()<4) {
		//! @todo err msg
			kdWarning() << "KexiBLOBBuffer::objectForId("<<id<<","<<stored
			<<"): res!=true || rowData.size()<4; res=="<<res.toString()<<" rowData.size()=="<<rowData.size()<< endl;
			return KexiBLOBBuffer::Handle();
		}

		item = new Item(
			rowData[0].toByteArray(),
			id, 
			true, //stored
			rowData[1].toString(),
			rowData[2].toString(),
			rowData[3].toString(),
			(Id_t)rowData[4].toInt() //!< @todo folder id: fix Id_t for Qt4
		);

		insertItem(item);
		return KexiBLOBBuffer::Handle(item);
//#endif
	}
	else
		return KexiBLOBBuffer::Handle(d->inMemoryItems.find(id));
}

KexiBLOBBuffer::Handle KexiBLOBBuffer::objectForId(Id_t id)
{
	KexiBLOBBuffer::Handle h(objectForId(id, false/*!stored*/));
	if (h)
		return h;
	return objectForId(id, true/*stored*/);
}

void KexiBLOBBuffer::removeItem(Id_t id, bool stored)
{
	Item *item;
	if (stored)
		item = d->storedItems.take(id);
	else
		item = d->inMemoryItems.take(id);

	if (item && !item->prettyURL.isEmpty()) {
		d->itemsByURL.remove(item->prettyURL);
	}
	delete item;
}

void KexiBLOBBuffer::takeItem(Item *item)
{
	assert(item);
	if (item->stored)
		d->storedItems.take(item->id);
	else
		d->inMemoryItems.take(item->id);
}

void KexiBLOBBuffer::insertItem(Item *item)
{
	assert(item);
	if (item->stored)
		d->storedItems.insert(item->id, item);
	else
		d->inMemoryItems.insert(item->id, item);
}

void KexiBLOBBuffer::setConnection(KexiDB::Connection *conn)
{
	KexiBLOBBuffer::self()->d->conn = conn;
}

KexiBLOBBuffer* KexiBLOBBuffer::self()
{
	if(!m_buffer) {
		m_bufferDeleter.setObject( m_buffer, new KexiBLOBBuffer() );
	}
	return m_buffer;
}

#include "kexiblobbuffer.moc"
