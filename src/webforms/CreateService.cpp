/*
 * This file is part of the KDE project
 *
 * (C) Copyright 2008 by Lorenzo Villani <lvillani@binaryhelix.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <QString>

#include <KDebug>

#include <pion/net/HTTPResponseWriter.hpp>

#include <google/template.h>

#include <kexidb/cursor.h>
#include <kexidb/connection.h>
#include <kexidb/queryschema.h>
#include <kexidb/roweditbuffer.h>
#include <kexidb/field.h>

#include "DataProvider.h"
#include "TemplateProvider.h"

#include "CreateService.h"

using namespace pion::net;

namespace KexiWebForms {

    void CreateService::operator()(pion::net::HTTPRequestPtr& request, pion::net::TCPConnectionPtr& tcp_conn) {
        HTTPResponseWriterPtr writer(HTTPResponseWriter::create(tcp_conn, *request,
                    boost::bind(&TCPConnection::finish, tcp_conn)));

        /* Retrieve the requested table name */
        QString requestedTable(QString(request->getOriginalResource().c_str()).split('/').at(2));
        setValue("TABLENAME", requestedTable);


        KexiDB::TableSchema* tableSchema = gConnection->tableSchema(requestedTable);


        /* Build the form */
        if (request->getQuery("dataSent") == "true") {
            KexiDB::QuerySchema schema(*tableSchema);
            KexiDB::Cursor* cursor = gConnection->prepareQuery(schema);

            QStringList fieldsList(QString(request->getQuery("tableFields").c_str()).split("|:|"));
            kDebug() << "Fields: " << fieldsList;

            QStringListIterator iterator(fieldsList);

            KexiDB::RecordData recordData(tableSchema->fieldCount());
            KexiDB::RowEditBuffer editBuffer(true);

            while (iterator.hasNext()) {
                QString currentFieldName(iterator.next());
                QString currentFieldValue(QUrl::fromPercentEncoding(request->getQuery(currentFieldName.toUtf8().constData()).c_str()));

                /*! @note This removes pluses */
                currentFieldValue.replace("+", " ");
                QVariant currentValue(currentFieldValue);

                kDebug() << "Inserting " << currentFieldName << "=" << currentValue.toString() << endl;
                editBuffer.insert(*schema.columnInfo(currentFieldName), currentValue);
            }


            if (cursor->insertRow(recordData, editBuffer)) {
                /** @note Restore this */
                //cachedPkeys[requestedTable].clear();
                m_dict->ShowSection("SUCCESS");
                setValue("MESSAGE", "Row added successfully");
            } else {
                m_dict->ShowSection("ERROR");
                setValue("MESSAGE", gConnection->errorMsg());
            }

            kDebug() << "Deleting cursor..." << endl;
            gConnection->deleteCursor(cursor);
        } else {
            m_dict->ShowSection("FORM");

            QString formData;
            QStringList fieldsList;

            for (uint i = 0; i < tableSchema->fieldCount(); i++) {
                KexiDB::Field* currentField = tableSchema->field(i);
                const KexiDB::Field::Type type = currentField->type();
                QString fieldName(currentField->name());

                formData.append("<tr>");
                    
                formData.append("<td>").append(currentField->captionOrName()).append("</td>");
                /** @todo Show "upload image" if type is KexiDB::Field::BLOB */
                if (type == KexiDB::Field::LongText) {
                    formData.append("<td>").append("<textarea name=\"");
                    formData.append(fieldName).append("\"></textarea></td>");
                } else {
                    formData.append("<td>").append("<input type=\"text\" name=\"");
                    formData.append(fieldName).append("\" value=\"").append(currentField->defaultValue().toString());
                    formData.append("\"/></td>");
                }

                // Field properties images
                formData.append("<td>");
                currentField->isAutoIncrement() ? formData.append("<img src=\"/toolbox/auto-increment.png\" alt=\"Auto increment\"/>&nbsp;") : 0;
                currentField->isPrimaryKey() ? formData.append("<img src=\"/toolbox/primary-key.png\" alt=\"Primary Key\"/>&nbsp;") : 0;
                ( currentField->isNotEmpty() || currentField->isNotNull() ) ? formData.append("<img src=\"/toolbox/emblem-required.png\" alt=\"Required\"/>&nbsp;") : 0;
                formData.append("</td>");
                    
                formData.append("</tr>");
                fieldsList << fieldName;
                
                /*QString fieldName(tableSchema->field(i)->name());

                formData.append("<tr>");
                formData.append("<td>").append(tableSchema->field(i)->captionOrName()).append("</td>");
                formData.append("<td><input type=\"text\" name=\"");
                formData.append(fieldName).append("\" value=\"\"/></td>");
                formData.append("</tr>");
                fieldsList << fieldName;*/
            }

            setValue("TABLEFIELDS", fieldsList.join("|:|"));
            setValue("FORMDATA", formData);
        }


        renderTemplate(m_dict, writer);
    }

}
