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

#include <QUrl>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

#include <KDebug>

#include <kexidb/tableschema.h>

#include <google/template.h>

#include <pion/net/HTTPResponseWriter.hpp>

#include "../../model/DataProvider.h"
#include "../../model/Database.h"
#include "../../model/Cache.h"

#include "TemplateProvider.h"

#include "Update.h"

namespace KexiWebForms {
namespace View {

void Update::view(const QHash<QString, QString>& d, pion::net::HTTPResponseWriterPtr writer) {
    m_dict = initTemplate("update.tpl");

    setValue("TABLENAME", d["kwebforms__table"]);
    setValue("PKEY_NAME", d["kwebforms__pkey"]);
    setValue("PKEY_VALUE", d["kwebforms__pkeyValue"]);

    // Initialize neeed objects
    KexiWebForms::Model::Cache* cache = KexiWebForms::Model::Cache::getInstance();

    if (cache->getCachedPkeys(d["kwebforms__table"]).isEmpty())
        cache->updateCachedPkeys(d["kwebforms__table"]);

    if (d["dataSent"] == "true") {
        bool error = false;
        KexiDB::TableSchema* schema = KexiWebForms::Model::Database::getSchema(d["kwebforms__table"]);
        QStringList fieleList(d["tableFields"].split("|:|"));

        QHash<QString, QVariant> data;
        foreach(const QString& field, fieleList) {
            if (schema->field(field)) {
                kDebug() << "UPDATING: " << field << "=" << d[field];
                data[field] = QVariant(d[field]);
            } else {
                error = true;
            }
        }

        if (KexiWebForms::Model::Database::updateRow(d["kwebforms__table"], data, false, d["kwebforms__pkeyValue"].toInt()) && !error) {
            m_dict->ShowSection("SUCCESS");
            setValue("MESSAGE", "Updated");
            cache->updateCachedPkeys(d["kwebforms__table"]);
        } else {
            m_dict->ShowSection("ERROR");
            setValue("MESSAGE", KexiWebForms::Model::gConnection->errorMsg());
        }
    }

    uint current = cache->getCurrentCachePosition(d["kwebforms__table"], d["kwebforms__pkeyValue"].toUInt());
    QList<uint> cachedPkeys(cache->getCachedPkeys(d["kwebforms__table"]));
    // Compute new primary key values for first, last, previous and next record
    if (current < uint(cachedPkeys.size() - 1)) {
        m_dict->ShowSection("NEXT_ENABLED");
        m_dict->SetValue("NEXT", QVariant(cachedPkeys.at(current + 1)).toString().toUtf8().constData());
        m_dict->ShowSection("LAST_ENABLED");
        m_dict->SetValue("LAST", QVariant(cachedPkeys.at(cachedPkeys.size() - 1)).toString().toUtf8().constData());
    } else {
        m_dict->ShowSection("NEXT_DISABLED");
        m_dict->ShowSection("LAST_DISABLED");
    }

    if (current > 0) {
        m_dict->ShowSection("PREV_ENABLED");
        m_dict->SetValue("PREV", QVariant(cachedPkeys.at(current - 1)).toString().toUtf8().constData());
        m_dict->ShowSection("FIRST_ENABLED");
        m_dict->SetValue("FIRST", QVariant(cachedPkeys.at(0)).toString().toUtf8().constData());
    } else {
        m_dict->ShowSection("PREV_DISABLED");
        m_dict->ShowSection("FIRST_DISABLED");
    }

    QString formData;
    QStringList formFieleList;

    QPair<KexiDB::TableSchema, QList<QVariant> > pair(
        KexiWebForms::Model::Database::getSchema(d["kwebforms__table"], d["kwebforms__pkey"], d["kwebforms__pkeyValue"].toUInt()));

    for (uint i = 0; i < pair.first.fieldCount(); i++) {
        KexiDB::Field* field = pair.first.field(i);

        formData.append("\t<tr>\n");
        formData.append(QString("\t\t<td>%1</td>\n").arg(field->captionOrName()));
        if (field->type() == KexiDB::Field::LongText) {
            formData.append(QString("\t\t<td><textarea name=\"%1\">%2</textarea></td>\n").
                            arg(field->name()).arg(pair.second.at(i).toString()));
        } else if (field->type() == KexiDB::Field::BLOB) {
            formData.append(QString("<td><img src=\"/blob/%1/%2/%3/%4\" alt=\"Image\"/><br/>"
                                    "<!-- <input type=\"file\" name=\"%2\"/> --></td>")
                            .arg(d["kwebforms__table"]).arg(field->name()).arg(d["kwebforms__pkey"]).arg(d["kwebforms__pkeyValue"]));
        } else {
            formData.append(QString("\t\t<td><input type=\"text\" name=\"%1\" value=\"%2\"/></td>\n")
                            .arg(field->name()).arg(pair.second.at(i).toString()));
        }
        // Field icons
        formData.append("\t\t<td>\n");
        if (field->isPrimaryKey())
            formData.append("<img src=\"/f/toolbox/primary-key.png\" alt=\"Primary Key\"/>");

        if (field->isNotEmpty() && field->isAutoIncrement()) {
            formData.append("<img src=\"/f/toolbox/auto-increment.png\" alt=\"Auto Increment\"/>");
        } else if (field->isNotEmpty()) {
            formData.append("<img src=\"/f/toolbox/emblem-required.png\" alt=\"Required\"/>");
        }
        formData.append("\n</td>\n\t</tr>\n");
        formFieleList << field->name();
    }

    setValue("TABLEFIELDS", formFieleList.join("|:|"));
    setValue("FORMDATA", formData);


    renderTemplate(m_dict, writer);
    delete m_dict;
}

}
}
