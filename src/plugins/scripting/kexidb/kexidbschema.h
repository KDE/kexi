/***************************************************************************
 * kexidbschema.h
 * This file is part of the KDE project
 * copyright (C)2004-2005 by Sebastian Sauer (mail@dipe.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 ***************************************************************************/

#ifndef KROSS_KEXIDB_KEXIDBSCHEMA_H
#define KROSS_KEXIDB_KEXIDBSCHEMA_H

#include <qstring.h>

#include <api/object.h>
#include <api/class.h>

#include <kexidb/drivermanager.h>
#include <kexidb/schemadata.h>
#include <kexidb/tableschema.h>
#include <kexidb/queryschema.h>

namespace Kross { namespace KexiDB {

    // Forward-declarations.
    class KexiDBFieldList;
    class KexiDBQuerySchema;

    /**
     * The KexiDBSchema object provides common functionality for schemas
     * like KexiDBTableSchema or KexiDBQuerySchema.
     *
     * Example (in Python) ;
     * @code
     * # Get the tableschema from a KexiDBConnection object.
     * tableschema = connection.tableSchema("dept")
     * # Print some information.
     * print "table=%s description=%s" % (tableschema.name(), tableschema.description())
     * # Get the "SELECT * FROM dept;" queryschema for the table.
     * queryschema = tableschema.query()
     * # Walk through the fields/columns the queryschema has and print the fieldnames.
     * for field in queryschema.fieldlist().fields():
     *     print "fieldname=%s" % field.name()
     * # Execute the query. The returned KexiDBCursor object could be used then to iterate through the result.
     * cursor = connection.executeQuerySchema(queryschema)
     * @endcode
     */
    template<class T>
    class KexiDBSchema : public Kross::Api::Class<T>
    {
        public:
            KexiDBSchema(const QString& name, ::KexiDB::SchemaData* schema, ::KexiDB::FieldList* fieldlist);
            virtual ~KexiDBSchema();

        private:

            /** Returns the name of the schema. */
            const QString name() const;
            /** Set the name of the schema. */
            void setName(const QString& name);

            /** Returns the caption of the schema. */
            const QString caption() const;
            /** Set the caption of the schema. */
            void setCaption(const QString& caption);

            /** Returns a description of the schema. */
            const QString description() const;
            /** Set a description of the schema. */
            void setDescription(const QString& description);

            /** Returns the KexiDBFieldList object this schema has. */
            KexiDBFieldList* fieldlist() const;

        protected:
            ::KexiDB::SchemaData* m_schema;
            ::KexiDB::FieldList* m_fieldlist;
    };

    /**
     * The KexiDBTableSchema object implements a KexiDBSchema for tables.
     */
    class KexiDBTableSchema : public KexiDBSchema<KexiDBTableSchema>
    {
        public:
            KexiDBTableSchema(::KexiDB::TableSchema* tableschema);
            virtual ~KexiDBTableSchema();
            virtual const QString getClassName() const;
            ::KexiDB::TableSchema* tableschema();

        private:

            /** Return the KexiDBQuerySchema object that represents a
            "SELECT * FROM this_KexiDBTableSchema_object" SQL-statement. */
            KexiDBQuerySchema* query();

    };

    /**
     * The KexiDBTableSchema object implements a KexiDBSchema for queries.
     */
    class KexiDBQuerySchema : public KexiDBSchema<KexiDBQuerySchema>
    {
        public:
            KexiDBQuerySchema(::KexiDB::QuerySchema* queryschema);
            virtual ~KexiDBQuerySchema();
            virtual const QString getClassName() const;
            ::KexiDB::QuerySchema* queryschema();

        private:

            /** Returns the SQL-statement of this query schema. */
            const QString statement() const;
            /** Set the SQL-statement of this query schema. */
            void setStatement(const QString& statement);
            /** Set the where-expression. */
            bool setWhereExpression(const QString& whereexpression);

    };

}}

#endif

