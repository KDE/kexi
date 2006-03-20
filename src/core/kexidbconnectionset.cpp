/* This file is part of the KDE project
   Copyright (C) 2003,2005 Jaroslaw Staniek <js@iidea.pl>

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

#include "kexidbconnectionset.h"
#include "kexidbshortcutfile.h"

#include <kdebug.h>
#include <kstandarddirs.h>

#include <qfile.h>

//! @internal
class KexiDBConnectionSetPrivate
{
public:
	KexiDBConnectionSetPrivate()
	 : dataForFilenames(101)
	{
		list.setAutoDelete(true);
		maxid=-1;
	}
	KexiDB::ConnectionData::List list;
	QMap<KexiDB::ConnectionData*, QString> filenamesForData;
	QDict<KexiDB::ConnectionData> dataForFilenames;
	int maxid;
};

KexiDBConnectionSet::KexiDBConnectionSet()
: QObject()
, d(new KexiDBConnectionSetPrivate())
{
}

KexiDBConnectionSet::~KexiDBConnectionSet()
{
	delete d;
}

bool KexiDBConnectionSet::addConnectionData(KexiDB::ConnectionData *data, const QString& _filename)
{
	if (!data)
		return false;
	if (data->id<0)
		data->id = d->maxid+1;
	//TODO: 	check for id-duplicates
	
	d->maxid = QMAX(d->maxid,data->id);
//	d->list.append(data);

	QString filename( _filename );
	bool generateUniqueFilename = filename.isEmpty() 
		|| !filename.isEmpty() && data==d->dataForFilenames[filename];

	if (generateUniqueFilename) {
		QString dir = KGlobal::dirs()->saveLocation("data", "kexi/connections/", false /*!create*/);
		if (dir.isEmpty())
			return false;
		QString baseFilename( dir + (data->hostName.isEmpty() ? "localhost" : data->hostName) );
		int i = 0;
		while (KStandardDirs::exists(baseFilename+(i>0 ? QString::number(i) : QString::null)+".kexic"))
			i++;
		if (!KStandardDirs::exists(dir)) {
			//make 'connections' dir and protect it
			if (!KStandardDirs::makeDir(dir, 0700))
				return false;
		}
		filename = baseFilename+(i>0 ? QString::number(i) : QString::null)+".kexic";
	}
	addConnectionDataInternal(data, filename);
	bool result = saveConnectionData(data, data);
	if (!result)
		removeConnectionDataInternal(data);
	return result;
}

void KexiDBConnectionSet::addConnectionDataInternal(KexiDB::ConnectionData *data, const QString& filename)
{
	d->filenamesForData.insert(data, filename);
	d->dataForFilenames.insert(filename, data);
	d->list.append(data);
}

bool KexiDBConnectionSet::saveConnectionData(KexiDB::ConnectionData *oldData, 
	KexiDB::ConnectionData *newData)
{
	if (!oldData || !newData)
		return false;
	QMap<KexiDB::ConnectionData*, QString>::ConstIterator it = d->filenamesForData.find( oldData );
	if (it == d->filenamesForData.constEnd() || it.data().isEmpty())
		return false;
	const QString filename( it.data() );
	KexiDBConnShortcutFile shortcutFile(filename);
	if (!shortcutFile.saveConnectionData(*newData, newData->savePassword)) // true/*savePassword*/))
		return false;
	if (oldData!=newData)
		*oldData = *newData;
	return true;
}

void KexiDBConnectionSet::removeConnectionDataInternal(KexiDB::ConnectionData *data)
{
	QMap<KexiDB::ConnectionData*, QString>::ConstIterator it = d->filenamesForData.find( data );
	const QString filename( it.data() );
	d->filenamesForData.remove(data);
	d->dataForFilenames.remove(filename);
	d->list.removeRef(data);
}

bool KexiDBConnectionSet::removeConnectionData(KexiDB::ConnectionData *data)
{
	if (!data)
		return false;
	QMap<KexiDB::ConnectionData*, QString>::ConstIterator it = d->filenamesForData.find( data );
	if (it == d->filenamesForData.constEnd() || it.data().isEmpty())
		return false;
	QFile file( it.data() );
	if (!file.remove())
		return false;
	removeConnectionDataInternal(data);
	return true;
}

const KexiDB::ConnectionData::List& KexiDBConnectionSet::list() const
{
	return d->list;
}

void KexiDBConnectionSet::clear()
{
	d->list.clear();
	d->filenamesForData.clear();
	d->dataForFilenames.clear();
}

void KexiDBConnectionSet::load()
{
	clear();
//	QStringList dirs( KGlobal::dirs()->findDirs("data", "kexi/connections") );
//	kexidbg << dirs << endl;
	QStringList files( KGlobal::dirs()->findAllResources("data", "kexi/connections/*.kexic") );
//	//also try for capital file extension
//	files += KGlobal::dirs()->findAllResources("data", "kexi/connections/*.KEXIC");
//	kexidbg << files << endl;

	foreach(QStringList::ConstIterator, it, files) {
		KexiDB::ConnectionData *data = new KexiDB::ConnectionData();
		KexiDBConnShortcutFile shortcutFile( *it );
		if (!shortcutFile.loadConnectionData(*data)) {
			delete data;
			continue;
		}
		addConnectionDataInternal(data, *it);
	}
}

QString KexiDBConnectionSet::fileNameForConnectionData(KexiDB::ConnectionData *data) const
{
	if (!data)
		return QString::null;
	QMap<KexiDB::ConnectionData*, QString>::ConstIterator it = d->filenamesForData.find( data );
	return (it == d->filenamesForData.constEnd()) ? QString::null : it.data();
}

KexiDB::ConnectionData* KexiDBConnectionSet::connectionDataForFileName(const QString& fileName) const
{
	return d->dataForFilenames[fileName];
}
