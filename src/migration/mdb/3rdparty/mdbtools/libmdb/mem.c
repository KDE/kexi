/* MDB Tools - A library for reading MS Access database files
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */
#ifdef JAVA
#include "javadefines.h"
#else
#include "mdbtools.h"
#include <locale.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif
#endif  /* JAVA */
/**
 * mdb_init:
 *
 * Initializes the LibMDB library.  This function should be called exactly once
 * by calling program and prior to any other function.
 *
 **/
/* METHOD */ void mdb_init()
{
#if !MDB_NO_BACKENDS
	mdb_init_backends();
#endif
}

/**
 * mdb_exit:
 *
 * Cleans up the LibMDB library.  This function should be called exactly once
 * by the calling program prior to exiting (or prior to final use of LibMDB 
 * functions).
 *
 **/
/* METHOD */ void mdb_exit()
{
#if !MDB_NO_BACKENDS
	mdb_remove_backends();
#endif
}
