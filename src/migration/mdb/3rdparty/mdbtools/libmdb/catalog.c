/* MDB Tools - A library for reading MS Access database file
 * Copyright (C) 2000 Brian Bruns
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "mdbtools.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

const char *
mdb_get_objtype_string(int obj_type)
{
static const char *type_name[] = {"Form",
			"Table",
			"Macro",
			"System Table",
			"Report",
			"Query",
			"Linked Table",
			"Module",
			"Relationship",
			"Unknown 0x09",
			"User Info",
			"Database"
		};

	if (obj_type > 11) {
		return NULL;
	} else {
		return type_name[obj_type];
	}
}

void mdb_free_catalog(MdbHandle *mdb)
{
	unsigned int i, j;
	MdbCatalogEntry *entry;

	if ((!mdb) || (!mdb->catalog)) return;
	for (i=0; i<mdb->catalog->len; i++) {
		entry = (MdbCatalogEntry *)g_ptr_array_index(mdb->catalog, i);
		if (entry) {
			if (entry->props) {
				for (j=0; j<entry->props->len; j++)
					mdb_free_props(g_array_index(entry->props, MdbProperties*, j));
				g_array_free(entry->props, TRUE);
			}
			g_free(entry);
		}
	}
	g_ptr_array_free(mdb->catalog, TRUE);
	mdb->catalog = NULL;
}

GPtrArray *mdb_read_catalog (MdbHandle *mdb, int objtype)
{
	MdbCatalogEntry *entry, msysobj;
	MdbTableDef *table;
	char obj_id[256];
	char obj_name[MDB_MAX_OBJ_NAME];
	char obj_type[256];
	char obj_flags[256];
	char obj_props[MDB_BIND_SIZE];
	int type;
	unsigned int i;
	MdbColumn *col_props;
	int kkd_size_ole;

	if (!mdb) return NULL;
	if (mdb->catalog) mdb_free_catalog(mdb);
	mdb->catalog = g_ptr_array_new();
	mdb->num_catalog = 0;

	/* dummy up a catalog entry so we may read the table def */
	memset(&msysobj, 0, sizeof(MdbCatalogEntry));
	msysobj.mdb = mdb;
	msysobj.object_type = MDB_TABLE;
	msysobj.table_pg = 2;
	strcpy(msysobj.object_name, "MSysObjects");

	/* mdb_table_dump(&msysobj); */

	table = mdb_read_table(&msysobj);
	if (!table) return NULL;

	mdb_read_columns(table);

	mdb_bind_column_by_name(table, "Id", obj_id, NULL);
	mdb_bind_column_by_name(table, "Name", obj_name, NULL);
	mdb_bind_column_by_name(table, "Type", obj_type, NULL);
	mdb_bind_column_by_name(table, "Flags", obj_flags, NULL);
	i = mdb_bind_column_by_name(table, "LvProp", obj_props, &kkd_size_ole);
	col_props = g_ptr_array_index(table->columns, i-1);

	mdb_rewind_table(table);

	while (mdb_fetch_row(table)) {
		type = atoi(obj_type);
		if (objtype==MDB_ANY || type == objtype) {
			//fprintf(stderr, "obj_id: %10ld objtype: %-3d (0x%04x) obj_name: %s\n",
			//	(atol(obj_id) & 0x00FFFFFF), type, type, obj_name);
			entry = (MdbCatalogEntry *) g_malloc0(sizeof(MdbCatalogEntry));
			entry->mdb = mdb;
			strcpy(entry->object_name, obj_name);
			entry->object_type = (type & 0x7F);
			entry->table_pg = atol(obj_id) & 0x00FFFFFF;
			entry->flags = atol(obj_flags);
			mdb->num_catalog++;
			g_ptr_array_add(mdb->catalog, entry);
			if (kkd_size_ole) {
				size_t kkd_len;
				void *kkd = mdb_ole_read_full(mdb, col_props, &kkd_len);
				//mdb_buffer_dump(kkd, 0, kkd_len);
				entry->props = mdb_kkd_to_props(mdb, kkd, kkd_len);
				free(kkd);
			}
		}
	}
	//mdb_dump_catalog(mdb, MDB_TABLE);

	mdb_free_tabledef(table);

	return mdb->catalog;
}


MdbCatalogEntry *
mdb_get_catalogentry_by_name(MdbHandle *mdb, const gchar* name)
{
	unsigned int i;
	MdbCatalogEntry *entry;

	for (i=0; i<mdb->num_catalog; i++) {
		entry = g_ptr_array_index(mdb->catalog, i);
		if (!g_ascii_strcasecmp(entry->object_name, name))
			return entry;
	}
	return NULL;
}

void
mdb_dump_catalog(MdbHandle *mdb, int obj_type)
{
	unsigned int i;
	MdbCatalogEntry *entry;

	mdb_read_catalog(mdb, obj_type);
	for (i=0;i<mdb->num_catalog;i++) {
                entry = g_ptr_array_index(mdb->catalog,i);
		if (obj_type==MDB_ANY || entry->object_type==obj_type) {
			printf("Type: %-12s Name: %-48s Page: %06lx\n",
			mdb_get_objtype_string(entry->object_type),
			entry->object_name,
			entry->table_pg);
		}
	}
	return;
}

