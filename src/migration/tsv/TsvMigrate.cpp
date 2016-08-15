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
#include <KDbSqlResult>

#include <QDebug>
#include <QDir>
#include <QTextCodec>

const int MAX_SAMPLE_TEXT_SIZE = 1024 * 10; // max 10KiB of text to detect encoding

using namespace KexiMigration;

/* This is the implementation for the TSV file import routines. */
KEXI_PLUGIN_FACTORY(TsvMigrate, "keximigrate_tsv.json")

namespace KexiMigration {
struct FileInfo
{
    QFile file;
    QTextCodec *codec;
    QVector<QString> fieldNames;
};
}

TsvMigrate::TsvMigrate(QObject *parent, const QVariantList& args)
        : KexiMigrate(parent, args)
{
}


TsvMigrate::~TsvMigrate()
{
}

KDbConnection* TsvMigrate::drv_createConnection()
{
    // nothing to do, just success
    m_result = KDbResult();
    return nullptr;
}

bool TsvMigrate::drv_connect()
{
    return QDir().exists(data()->source->databaseName());
}

bool TsvMigrate::drv_disconnect()
{
    return true;
}

bool TsvMigrate::drv_tableNames(QStringList *tablenames)
{
  // return base part of filename only so table name will look better
  tablenames->append(QFileInfo(data()->source->databaseName()).baseName());
  return true;
}

//! @return next line read from the file split by tabs, decoded to unicode and with last \n removed
static QVector<QByteArray> readLine(FileInfo *info, bool *eof)
{
    QByteArray line = info->file.readLine();
    int count = line.length();
    if (line.endsWith('\n')) {
        --count;
    }
    if (line.isEmpty()) {
        *eof = true;
        return QVector<QByteArray>();
    }
    *eof = false;
    int i = 0;
    int start = 0;
    int fields = 0;
    QVector<QByteArray> result(info->fieldNames.isEmpty() ? 10 : info->fieldNames.count());
    for (; i < count; ++i) {
        if (line[i] == '\t') {
            if (fields >= result.size()) {
                result.resize(result.size() * 2);
            }
            result[fields] = line.mid(start, i - start);
            ++fields;
            start = i + 1;
        }
    }
    result[fields] = line.mid(start, i - start); // last value
    result.resize(fields + 1);
    return result;
}

bool TsvMigrate::drv_copyTable(const QString& srcTable, KDbConnection *destConn,
                               KDbTableSchema* dstTable,
                               const RecordFilter *recordFilter)
{
    Q_UNUSED(srcTable)
    FileInfo info;
    if (!openFile(&info)) {
        return false;
    }
    Q_FOREVER {
        bool eof;
        QVector<QByteArray> line = readLine(&info, &eof);
        if (eof) {
            break;
        }
        QList<QVariant> vals;
        for(int i = 0; i < line.count(); ++i) {
            vals.append(line.at(i));
        }
        if (recordFilter && !(*recordFilter)(vals)) {
            continue;
        }
        if (!destConn->insertRecord(dstTable, vals)) {
            return false;
        }
    }
    return true;
}

bool TsvMigrate::drv_readTableSchema(const QString& originalName, KDbTableSchema *tableSchema)
{
    Q_UNUSED(originalName)
    FileInfo info;
    if (!openFile(&info)) {
        return false;
    }
    for (const QString &name : info.fieldNames) {
        KDbField *f = new KDbField(name, KDbField::Text);
        if (!tableSchema->addField(f)) {
            delete f;
            tableSchema->clear();
            return false;
        }
    }
    return true;
}

class TsvRecord : public KDbSqlRecord
{
public:
    inline explicit TsvRecord(const QVector<QByteArray> &values, const FileInfo &m_info)
        : m_values(values), m_info(&m_info)
    {
    }

    inline QString stringValue(int index) Q_DECL_OVERRIDE {
        return m_info->codec->toUnicode(m_values.value(index));
    }

    inline QByteArray toByteArray(int index) Q_DECL_OVERRIDE {
        return m_values.value(index);
    }

    inline KDbSqlString cstringValue(int index) Q_DECL_OVERRIDE {
        return KDbSqlString(m_values[index].constData(), m_values[index].length());
    }

private:
    const QVector<QByteArray> m_values;
    const FileInfo *m_info;
};

class TsvResult : public KDbSqlResult
{
public:
    inline explicit TsvResult(FileInfo *info) : m_info(info), m_eof(false) {
        Q_ASSERT(info);
    }

    inline int fieldsCount() Q_DECL_OVERRIDE {
        return m_info->fieldNames.count();
    }

    //! Not needed for ImportTableWizard
    inline KDbSqlField *field(int index) Q_DECL_OVERRIDE {
        Q_UNUSED(index);
        return nullptr;
    }

    //! Not needed for ImportTableWizard
    inline KDbField* createField(const QString &tableName, int index) Q_DECL_OVERRIDE {
        Q_UNUSED(tableName);
        Q_UNUSED(index);
        return nullptr;
    }

    inline KDbSqlRecord* fetchRecord() Q_DECL_OVERRIDE {
        QVector<QByteArray> record = readLine(m_info, &m_eof);
        if (m_eof) {
            return nullptr;
        }
        return new TsvRecord(record, *m_info);
    }

    inline KDbResult lastResult() Q_DECL_OVERRIDE {
        return KDbResult();
    }

    inline ~TsvResult() {
        delete m_info;
    }

private:
    FileInfo *m_info;
    bool m_eof;
};

KDbSqlResult* TsvMigrate::drv_readFromTable(const QString &tableName)
{
    Q_UNUSED(tableName)
    QScopedPointer<FileInfo> info(new FileInfo);
    if (!openFile(info.data())) {
        return nullptr;
    }
    return new TsvResult(info.take());
}

bool TsvMigrate::openFile(FileInfo *info)
{
    info->file.setFileName(data()->source->databaseName());
    if (!info->file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    {
        const QByteArray sample(info->file.read(MAX_SAMPLE_TEXT_SIZE));
        info->codec = QTextCodec::codecForUtfText(sample);
    }

    if (!info->file.seek(0)) {
        info->codec = 0;
        info->file.close();
        return false;
    }
    bool eof;
    QVector<QByteArray> record = readLine(info, &eof);
    const QString s();
    info->fieldNames.resize(record.count());
    for (int i = 0; i < record.count(); ++i) {
        info->fieldNames[i] = info->codec->toUnicode(record[i]);
    }
    return !eof;
}

#include "TsvMigrate.moc"
