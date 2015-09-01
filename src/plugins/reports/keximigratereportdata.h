/*
* Kexi Report Plugin
* Copyright (C) 2007-2009 by Adam Pigg (adam@piggz.co.uk)
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __KEXIMIGRATEREPORTDATA_H__
#define __KEXIMIGRATEREPORTDATA_H__

#include <QString>
#include <QStringList>

#include <KDbUtils>

#include <KoReportData>

#include <migration/migratemanager.h>
#include <migration/keximigrate.h>

class KexiMigrateReportData : public KoReportData
{
public:
    explicit KexiMigrateReportData(const QString &);

    virtual ~KexiMigrateReportData();

    virtual int fieldNumber(const QString &field) const;
    virtual QStringList fieldNames() const;

    virtual QVariant value(unsigned int) const;
    virtual QVariant value(const QString &field) const;

    virtual bool open() {
        return true;
    }
    virtual bool close() {
        return true;
    }
    virtual bool moveNext();
    virtual bool movePrevious();
    virtual bool moveFirst();
    virtual bool moveLast();

    virtual qint64 at() const;

    virtual qint64 recordCount() const;

private:
    class Private;
    Private * const d;
};

#endif

