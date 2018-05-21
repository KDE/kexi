/* This file is part of the KDE project
   Copyright (C) 2005,2006 Jarosław Staniek <staniek@kde.org>

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

#include "kexicsvexport.h"
#include "kexicsvwidgets.h"
#include <core/KexiMainWindowIface.h>
#include <core/kexiproject.h>
#include <core/kexipartinfo.h>
#include <core/kexipartmanager.h>
#include <core/kexiguimsghandler.h>
#include <kexiutils/utils.h>
#include <widget/kexicharencodingcombobox.h>

#include <KDb>
#include <KDbConnection>
#include <KDbCursor>
#include <KDbQuerySchema>
#include <KDbTableOrQuerySchema>
#include <KDbTableSchema>
#include <KDbUtils>

#include <KLocalizedString>

#include <QApplication>
#include <QTextStream>
#include <QCheckBox>
#include <QClipboard>
#include <QDebug>
#include <QSaveFile>

using namespace KexiCSVExport;

Options::Options()
        : mode(File), itemId(0), addColumnNames(true), useTempQuery(false)
{
}

bool Options::assign(QMap<QString, QString> *args)
{
    bool result = true;
    mode = (args->value("destinationType") == "file")
           ? KexiCSVExport::File : KexiCSVExport::Clipboard;

    if (args->contains("delimiter"))
        delimiter = args->value("delimiter");
    else
        delimiter = (mode == File) ? KEXICSV_DEFAULT_FILE_DELIMITER : KEXICSV_DEFAULT_CLIPBOARD_DELIMITER;

    if (args->contains("textQuote"))
        textQuote = args->value("textQuote");
    else
        textQuote = (mode == File) ? KEXICSV_DEFAULT_FILE_TEXT_QUOTE : KEXICSV_DEFAULT_CLIPBOARD_TEXT_QUOTE;

    bool ok;
    itemId = args->value("itemId").toInt(&ok);
    if (!ok || itemId == 0) {
        result = false; //neverSaved items are supported
    }
    if (args->contains("forceDelimiter"))
        forceDelimiter = args->value("forceDelimiter");
    if (args->contains("addColumnNames"))
        addColumnNames = (args->value("addColumnNames") == "1");
    useTempQuery = (args->value("useTempQuery") == "1");
    return result;
}

//------------------------------------

bool KexiCSVExport::exportData(KDbConnection* conn, KDbTableOrQuerySchema *tableOrQuery,
                               const Options& options, int recordCount, QTextStream *predefinedTextStream)
{
    if (!conn)
        return false;

    KDbQuerySchema* query = tableOrQuery->query();
    QList<QVariant> queryParams;
    if (!query) {
        query = tableOrQuery->table()->query();
    }
    else {
        queryParams = KexiMainWindowIface::global()->currentParametersForQuery(query->id());
    }

    if (recordCount == -1)
        recordCount = conn->recordCount(tableOrQuery, queryParams);
    if (recordCount == -1)
        return false;

//! @todo move this to non-GUI location so it can be also used via command line
//! @todo add a "finish" page with a progressbar.
//! @todo look at recordCount whether the data is really large;
//!       if so: avoid copying to clipboard (or ask user) because of system memory

//! @todo OPTIMIZATION: use fieldsExpanded(true /*UNIQUE*/)
//! @todo OPTIMIZATION? (avoid multiple data retrieving) look for already fetched data within KexiProject..

    const KDbQueryColumnInfo::Vector fields(
        query->fieldsExpanded(conn, KDbQuerySchema::FieldsExpandedMode::WithInternalFields));
    QString buffer;

    QScopedPointer<QSaveFile> kSaveFile;
    QTextStream *stream = 0;
    QScopedPointer<QTextStream> kSaveFileTextStream;

    const bool copyToClipboard = options.mode == Clipboard;
    if (copyToClipboard) {
//! @todo (during exporting): enlarge bufSize by factor of 2 when it became too small
        int bufSize = qMin((recordCount < 0 ? 10 : recordCount) * fields.count() * 20, 128000);
        buffer.reserve(bufSize);
        if (buffer.capacity() < bufSize) {
            qWarning() << "Cannot allocate memory for" << bufSize
                       << "characters";
            return false;
        }
    } else {
        if (predefinedTextStream) {
            stream = predefinedTextStream;
        } else {
            if (options.fileName.isEmpty()) {//sanity
                qWarning() << "Fname is empty";
                return false;
            }
            kSaveFile.reset(new QSaveFile(options.fileName));

            //qDebug() << "QSaveFile Filename:" << kSaveFile->fileName();

            if (kSaveFile->open(QIODevice::WriteOnly)) {
                kSaveFileTextStream.reset(new QTextStream(kSaveFile.data()));
                stream = kSaveFileTextStream.data();
                //qDebug() << "have a stream";
            }
            if (QFileDevice::NoError != kSaveFile->error() || !stream) {//sanity
                qWarning() << "Status != 0 or stream == 0";
//! @todo show error
                return false;
            }
        }
    }

//! @todo escape strings

#define _ERR \
    if (kSaveFile) { kSaveFile->cancelWriting(); } \
    return false

#define APPEND(what) \
    if (copyToClipboard) buffer.append(what); else (*stream) << (what)

// use native line ending for copying, RFC 4180 one for saving to file
#define APPEND_EOLN \
    if (copyToClipboard) { APPEND('\n'); } else { APPEND("\r\n"); }

    // 0. Cache information
    const int fieldsCount = query->fieldsExpanded(conn).count(); //real fields count without internals
    const QChar delimiter(options.delimiter.at(0));
    const bool hasTextQuote = !options.textQuote.isEmpty();
    const QString textQuote(hasTextQuote ? options.textQuote.at(0) : QString());
    const QByteArray escapedTextQuote((textQuote + textQuote).toLatin1());   //ok?
    //cache for faster checks
    QScopedArrayPointer<bool> isText(new bool[fieldsCount]);
    QScopedArrayPointer<bool> isDateTime(new bool[fieldsCount]);
    QScopedArrayPointer<bool> isTime(new bool[fieldsCount]);
    QScopedArrayPointer<bool> isBLOB(new bool[fieldsCount]);
    QScopedArrayPointer<int> visibleFieldIndex(new int[fieldsCount]);
// bool isInteger[fieldsCount]; //cache for faster checks
// bool isFloatingPoint[fieldsCount]; //cache for faster checks
    for (int i = 0; i < fieldsCount; i++) {
        KDbQueryColumnInfo* ci;
        const int indexForVisibleLookupValue = fields[i]->indexForVisibleLookupValue();
        if (-1 != indexForVisibleLookupValue) {
            ci = query->expandedOrInternalField(conn, indexForVisibleLookupValue);
            visibleFieldIndex[i] = indexForVisibleLookupValue;
        } else {
            ci = fields[i];
            visibleFieldIndex[i] = i;
        }

        const KDbField::Type t = ci->field()->type(); // cache: evaluating type of expressions can be expensive
        isText[i] = KDbField::isTextType(t);
        isDateTime[i] = t == KDbField::DateTime;
        isTime[i] = t == KDbField::Time;
        isBLOB[i] = t == KDbField::BLOB;
//  isInteger[i] = KDbField::isIntegerType(t)
//   || t == KDbField::Boolean;
//  isFloatingPoint[i] = KDbField::isFPNumericType(t);
    }

    // 1. Output column names
    if (options.addColumnNames) {
        for (int i = 0; i < fieldsCount; i++) {
            //qDebug() << "Adding column names";
            if (i > 0) {
                APPEND(delimiter);
            }

            if (hasTextQuote) {
                APPEND(textQuote + fields[i]->captionOrAliasOrName().replace(textQuote, escapedTextQuote) + textQuote);
            } else {
                APPEND(fields[i]->captionOrAliasOrName());
            }
        }
        APPEND_EOLN
    }

    KexiGUIMessageHandler handler;
    KDbCursor *cursor = conn->executeQuery(query, queryParams);
    if (!cursor) {
        handler.showErrorMessage(conn->result());
        _ERR;
    }
    for (cursor->moveFirst(); !cursor->eof() && !cursor->result().isError(); cursor->moveNext()) {
        //qDebug() << "Adding records";
        const int realFieldCount = qMin(cursor->fieldCount(), fieldsCount);
        for (int i = 0; i < realFieldCount; i++) {
            const int real_i = visibleFieldIndex[i];
            if (i > 0) {
                APPEND(delimiter);
            }

            if (cursor->value(real_i).isNull()) {
                continue;
            }

            if (isText[real_i]) {
                if (hasTextQuote)
                    APPEND(textQuote + QString(cursor->value(real_i).toString()).replace(textQuote, escapedTextQuote) + textQuote);
                else
                    APPEND(cursor->value(real_i).toString());
            } else if (isDateTime[real_i]) { //avoid "T" in ISO DateTime
                APPEND(cursor->value(real_i).toDateTime().date().toString(Qt::ISODate) + " "
                       + cursor->value(real_i).toDateTime().time().toString(Qt::ISODate));
            } else if (isTime[real_i]) { //time is temporarily stored as null date + time...
                APPEND(cursor->value(real_i).toTime().toString(Qt::ISODate));
            } else if (isBLOB[real_i]) { //BLOB is escaped in a special way
                if (hasTextQuote)
//! @todo add options to suppport other types from KDbBLOBEscapingType enum...
                    APPEND(textQuote + KDb::escapeBLOB(cursor->value(real_i).toByteArray(),
                                                       KDb::BLOBEscapingType::Hex) + textQuote);
                else
                    APPEND(KDb::escapeBLOB(cursor->value(real_i).toByteArray(), KDb::BLOBEscapingType::Hex));
            } else {//other types
                APPEND(cursor->value(real_i).toString());
            }
        }
        APPEND_EOLN
    }

    if (copyToClipboard)
        buffer.squeeze();

    if (!conn->deleteCursor(cursor)) {
        handler.showErrorMessage(conn->result());
        _ERR;
    }

    if (copyToClipboard)
        QApplication::clipboard()->setText(buffer, QClipboard::Clipboard);

    //qDebug() << "Done";

    if (kSaveFile) {
        stream->flush();
        if (!kSaveFile->commit()) {
            qWarning() << "Error committing the file" << kSaveFile->fileName();
        }
    }
    return true;
}
