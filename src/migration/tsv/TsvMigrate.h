/* This file is part of the KDE project
Copyright (C) 2004-2009 Adam Pigg <adam@piggz.co.uk>
Copyright (C) 2016 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXITSVMIGRATE_H
#define KEXITSVMIGRATE_H

#include <migration/keximigrate.h>

#include <QFile>

class QTextCodec;

namespace KexiMigration
{

struct FileInfo;

//! "Tab Separated Values" document import plugin
class TsvMigrate : public KexiMigrate
{
    Q_OBJECT

public:
    explicit TsvMigrate(QObject *parent, const QVariantList &args = QVariantList());

    virtual ~TsvMigrate();

  protected:
    //! Connect to source
    KDbConnection* drv_createConnection() override;

    bool drv_connect() override;

    //! Disconnect from source
    virtual bool drv_disconnect() override;

    //! Get table names in source
    virtual bool drv_tableNames(QStringList *tablenames) override;

    bool drv_copyTable(const QString& srcTable, KDbConnection *destConn,
                       KDbTableSchema* dstTable,
                       const RecordFilter *recordFilter = 0) override;

    //! Read schema for a given table
    virtual bool drv_readTableSchema(const QString& originalName, KDbTableSchema *tableSchema) override;

    //! Starts reading data from the source dataset's table
    QSharedPointer<KDbSqlResult> drv_readFromTable(const QString & tableName) override;

  private:
    bool openFile(FileInfo *info);
};

}

#endif
