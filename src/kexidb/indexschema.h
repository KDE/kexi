/* This file is part of the KDE project
   Copyright (C) 2003 Jaroslaw Staniek <js@iidea.pl>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef KEXIDB_INDEX_H
#define KEXIDB_INDEX_H

#include <qvaluelist.h>
#include <qstring.h>

#include <kexidb/fieldlist.h>
#include <kexidb/schemadata.h>
#include <kexidb/reference.h>

namespace KexiDB {

class Connection;
class TableSchema;
class QuerySchema;

/*! KexiDB::IndexSchema provides information about database index
	that can be created on database table. 
	
	IndexSchema object stores information about table fields that
	defines this index and additional properties like: if index is unique,
	primary key (requires unique). Single-field index can be also auto generated.

	Example:
	<pre>
	          ---------
	 ---r1--<|         |
	         | Table A |----r3---<
	 ---r2--<|         |
	          ---------
	</pre>
	Table A has two re
*/

class KEXI_DB_EXPORT IndexSchema : public FieldList, public SchemaData
{
	public:
		typedef QPtrList<IndexSchema> List;
		typedef QPtrListIterator<IndexSchema> ListIterator;

		/*! Constructs empty index schema object 
		 that is assigned to \a table, and will be owned by this table.
		 Any fields added with addField() won't be owned by index,
		 but by its table. Do not forget to add this fields to table,
		 because adding these to IndexSchema is not enough. 
		 */
		IndexSchema(TableSchema *tableSchema);

		/*! Copy constructor. */
		IndexSchema(const IndexSchema& idx);

		/*! Destroys the index. Field objects are not deleted.
		 All Reference objects listed in referencesFrom() list 
		 are destroyed (these are also detached from 
		 reference-side indices before destruction). 
		 Reference objects listed in referencesTo() are not touched. */
		~IndexSchema();

//		void setName(const QString& name);

		/*! Adds field at the and of field list. 
		 Field will not be owned by index. Field must belong to a table
		 the index is bulit on, otherwise field couldn't be added. */
		virtual FieldList& addField(Field *field);

		/*! \return table that index is defined for. */
		TableSchema* table() const;

		/*! \return list of references from the table (of this index), i.e. any such reference in which
		 the table is at 'master' side. See Reference class documentation for more information.
		 All objects listed here will be automatically destroyed on this IndexSchema object destruction. */
		Reference::List* masterReferences() { return &m_master_refs; }

		/*! \return list of references to the table (of this index), i.e. any such reference in which
		 the table is at 'details' side. See Reference class documentation for more information. */
		Reference::List* detailsReferences() { return &m_details_refs; }

		/*! Attaches reference definition \a ref to this IndexSchema object. 
		 If \a ref reference has this IndexSchema defined at the master-side,
		 \a ref is added to the list of master refererences (available with masterReferences())
		 If \a ref reference has this IndexSchema defined at the details-side,
		 \a ref is added to the list of details refererences (available with detailsReferences()).
		 For the former case, attached \a ref object is now owned by this IndexSchema object. 

		 Note: call detachReference() for IndexSchema object that \a ref 
		 was previously attached to, if any. */
		void attachReference(Reference *ref);

		/*! Detaches reference definition \a ref for this IndexSchema object
		 from the list of master refererences (available with masterReferences()),
		 or details references, depending on which side of the reference
		 is this IndexSchem object assigned.

		 Note: If \a ref was detached from referencesFrom() list, this \a ref object now has no parent, 
		 so attach it to somewhere or destruct it. 
		*/
		void detachReference(Reference *ref);

		/*! \return true if index is auto-generated.
			Auto-generated index is one-field index
			that was automatically generated 
			for CREATE TABLE statement when the field has 
			UNIQUE or PRIMARY KEY constraint enabled.
			
			Any newly created IndexSchema object 
			has this flag set to false.
			
			This flag is handled internally by TableSchema.
			It can be usable for GUI application if we do not 
			want display implicity/auto generated indices
			on the indices list or we if want to show these 
			indices to the user in a special way.
		*/
		bool isAutoGenerated() const;
		
		/*! \return true if this index is primary key of its table. 
			This can be one or multifield. */
		bool isPrimaryKey() const;
		
		/*! Sets PRIMARY KEY flag. \sa isPrimary().
		 Note: Setting PRIMARY KEY on (true), 
		 UNIQUE flag will be also implicity set. */
		void setPrimaryKey(bool set);

		/*! \return true if this is unique index. 
		 This can be one or multifield. */
		bool isUnique() const;
		
		/*! Sets UNIQUE flag. \sa isUnique(). 
		 Note: Setting UNIQUE off (false), PRIMARY KEY flag will 
		 be also implicity set off, because this UNIQUE 
		 is the requirement for PRIMARY KEYS. */
		void setUnique(bool set);

		/*! Shows debug information about index. */
		virtual void debug() const;
	protected:

		/*! Sets auto-generated flag. This method should be called only
		 from TableSchema code	
		\sa isAutoGenerated(). */
		void setAutoGenerated(bool set);

	//js	QStringList m_primaryKeys;
//		Connection *m_conn;
		TableSchema *m_tableSchema; //! table on that index is built
		Reference::List m_master_refs; //! list of references from the table (of this index),
		                             //! this index is foreign key for these references
		                             //! and therefore - owner of these
		Reference::List m_details_refs; //! list of references to table (of this index)
		bool m_primary : 1;
		bool m_unique : 1;
		bool m_isAutoGenerated : 1;

	friend class Connection;
	friend class TableSchema;
	friend class QuerySchema;
};

} //namespace KexiDB

#endif
