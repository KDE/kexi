/* This file is part of the KDE project
    Copyright (C) 2003-2004 Jaroslaw Staniek <js@iidea.pl>
    Copyright (C) 2004 Martin Ellis <m.a.ellis@ncl.ac.uk>

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

#include <kdebug.h>
#include <qdict.h>
#include <qvaluevector.h>
#include "driver_p.h"

using namespace KexiDB;

namespace KexiDB {
	QAsciiDict<bool>* DriverPrivate::kexiSQLDict = 0;
	
	/*! QAsciiDict keys need to be a pointer to *something*.  Used
	    for SQL keyword dictionaries
	*/
  static bool _dummy;
}


DriverPrivate::DriverPrivate()
 : isFileDriver(false)
 , isDBOpenedAfterCreate(false)
 , features(Driver::NoFeatures)
{
	kexiSQLDict = 0;
	driverSQLDict = 0;

	properties["client_library_version"] = "";
	propertyCaptions["client_library_version"] =
	  i18n("Client library version");

	properties["default_server_encoding"] = "";
	propertyCaptions["default_server_encoding"] =
	  i18n("Default character encoding on server");
}

void DriverPrivate::initInternalProperties()
{
	properties["is_file_database"] = QVariant(isFileDriver, 1);
	propertyCaptions["is_file_database"] = i18n("File-based database driver");
	if (isFileDriver) {
		properties["file_database_mimetype"] = fileDBDriverMimeType;
		propertyCaptions["file_database_mimetype"] =
		  i18n("File-based database's MIME type");
	}
	
	QString str;
	if (features & Driver::SingleTransactions)
		str = i18n("Single transactions");
	else if (features & Driver::MultipleTransactions)
		str = i18n("Multiple transactions");
	else if (features & Driver::NestedTransactions)
		str = i18n("Nested transactions");
	else if (features & Driver::IgnoreTransactions)
		str = i18n("Ignored");
	else
		str = i18n("None");
	properties["transaction_support"] = str;
	propertyCaptions["transaction_support"] = i18n("Transaction support");

	properties["kexidb_driver_version"] =
	  QString("%1.%2").arg(versionMajor()).arg(versionMinor());
	propertyCaptions["kexidb_driver_version"] =
	  i18n("KexiDB driver version");
}

DriverPrivate::~DriverPrivate() {
	delete driverSQLDict;
}


void DriverPrivate::initKexiKeywords() {
	// QAsciiDict constructor args:
	//   size (preferable prime)
	//   case sensitive flag (false)
	//   copy strings (false)
	if(!kexiSQLDict) {
		kexiSQLDict = new QAsciiDict<bool>(79, false, false);
		initKeywords(kexiSQLKeywords, *kexiSQLDict);
	}
}

void DriverPrivate::initDriverKeywords(const char* keywords[], int hashSize) {
	driverSQLDict = new QAsciiDict<bool>(hashSize, false, false);
	initKeywords(keywords, *driverSQLDict);
}

void DriverPrivate::initKeywords(const char* keywords[], 
    QAsciiDict<bool>& dict) {
	for(int i = 0; keywords[i] != 0; i++) {
		dict.insert(keywords[i], &_dummy);
	}
}

