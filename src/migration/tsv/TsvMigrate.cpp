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

#include "TsvMigrate.h"
#include <kexi.h>

#include <QDebug>
#include <QDir>
#include <QTextCodec>

const int MAX_SAMPLE_TEXT_SIZE = 1024 * 10; // max 10KiB of text to detect encoding

using namespace KexiMigration;

/* This is the implementation for the TSV file import routines. */
KEXI_PLUGIN_FACTORY(TsvMigrate, "keximigrate_tsv.json")

TsvMigrate::TsvMigrate(QObject *parent, const QVariantList& args)
        : KexiMigrate(parent, args)
{
  m_codec = 0;
  m_DataFile = 0;
  m_Row = -1;
  m_FileRow = -1;
}


TsvMigrate::~TsvMigrate()
{
}

bool TsvMigrate::drv_connect()
{
  return QDir().exists(data()->source->databaseName());
}

bool TsvMigrate::drv_disconnect()
{
  if (m_DataFile) {
    delete m_DataFile;
    m_DataFile = 0;
  }

  return true;
}

bool TsvMigrate::drv_tableNames(QStringList& tablenames)
{
  // return base part of filename only so table name will look better
  tablenames << QFileInfo(data()->source->databaseName()).baseName();
  return true;
}

bool TsvMigrate::drv_copyTable(const QString& srcTable, KDbConnection *destConn,
                               KDbTableSchema* dstTable)
{
    Q_UNUSED(srcTable)
    if (!drv_readFromTable(QString())) {
        return false;
    }
    Q_FOREVER {
        bool eof;
        QStringList line = readLine(&eof);
        if (eof) {
            break;
        }
        QList<QVariant> vals;
        for(int i = 0; i < line.count() && i < m_FieldNames.count(); ++i) {
            vals.append(line.at(i));
        }
        for(int i = line.count(); i < m_FieldNames.count(); ++i) { // possibly missing values
            vals.append(QVariant());
        }
        if (!destConn->insertRecord(dstTable, vals)) {
            return false;
        }
    }
    return true;
}

bool TsvMigrate::drv_readTableSchema(const QString& originalName, KDbTableSchema& tableSchema)
{
    if (!drv_readFromTable(originalName)) {
        return false;
    }
    bool ok = true;
    for (int i = 0; i < m_FieldNames.count(); ++i) {
        KDbField *f = new KDbField(m_FieldNames[i], KDbField::Text);
        if (!tableSchema.addField(f)) {
            delete f;
            tableSchema.clear();
            ok = false;
            break;
        }
    }
    return ok;
}

QStringList TsvMigrate::readLine(bool *eof)
{
    QByteArray line = m_DataFile->readLine();
    if (line.endsWith('\n')) {
        line.chop(1);
        *eof = false;
    } else {
        if (line.isEmpty()) {
            *eof = true;
            return QStringList();
        }
    }
    return m_codec->toUnicode(line).split('\t');
}

bool TsvMigrate::drv_readFromTable(const QString & tableName)
{
  Q_UNUSED(tableName)
  delete m_DataFile;
  m_DataFile = 0;
  m_codec = 0;

  m_DataFile = new QFile(data()->source->databaseName());

  m_Row = -1;
  m_FileRow = -1;

  if (!m_DataFile->open(QIODevice::ReadOnly | QIODevice::Text)) {
    delete m_DataFile;
    m_DataFile = 0;
    return false;
  }
  {
    const QByteArray sample(m_DataFile->read(MAX_SAMPLE_TEXT_SIZE));
    m_codec = QTextCodec::codecForUtfText(sample);
  }

  if (!m_DataFile->seek(0)) {
      m_codec = 0;
      delete m_DataFile;
      m_DataFile = 0;
      return false;
  }
  bool eof;
  m_FieldNames = readLine(&eof);
  return !eof;
}

bool TsvMigrate::drv_moveNext()
{
    //qDebug();
  if (m_Row < m_FileRow)
  {
   m_Row++;
  }
  else
  {
    if (m_DataFile->atEnd())
      return false;

    bool eof;
    m_FieldValues.append(readLine(&eof));
    if (eof) {
        return false;
    }
    m_Row++;
    m_FileRow++;
  }
  return true;
}

bool TsvMigrate::drv_movePrevious()
{
    //qDebug();
  if (m_Row > 0)
  {
    m_Row--;
    return true;
  }
  return false;
}

QVariant TsvMigrate::drv_value(int i)
{
    if (m_Row >= 0)   {
        return QVariant(m_FieldValues[m_Row][i]);
    }
    return QVariant();
}

bool TsvMigrate::drv_moveFirst()
{
    //qDebug();
    m_Row = -1;
    return drv_moveNext();
}

bool TsvMigrate::drv_moveLast()
{
    //qDebug();
    while(drv_moveNext()) {}
    return true;
}

#include "TsvMigrate.moc"
