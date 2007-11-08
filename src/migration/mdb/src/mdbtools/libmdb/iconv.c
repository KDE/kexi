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

#include "mdbtools.h"
#include "errno.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#ifdef _WIN32
#include <windows.h>
#endif

/*
 * This function is used in reading text data from an MDB table.
 */
int
mdb_unicode2ascii(MdbHandle *mdb, char *src, size_t slen, char *dest, size_t dlen)
{
	char *tmp = NULL;
	size_t tlen = 0;
	size_t len_in, len_out;
	char *in_ptr, *out_ptr;

	if ((!src) || (!dest) || (!dlen))
		return 0;

	/* Uncompress 'Unicode Compressed' string into tmp */
	if (IS_JET4(mdb) && (slen>=2)
	 && ((src[0]&0xff)==0xff) && ((src[1]&0xff)==0xfe)) {
		unsigned int compress=1;
		src += 2;
		slen -= 2;
		tmp = (char *)g_malloc(slen*2);
		while (slen) {
			if (*src == 0) {
				compress = (compress) ? 0 : 1;
				src++;
				slen--;
			} else if (compress) {
				tmp[tlen++] = *src++;
				tmp[tlen++] = 0;
				slen--;
			} else if (slen >= 2){
				tmp[tlen++] = *src++;
				tmp[tlen++] = *src++;
				slen-=2;
			}
		}
	}

	in_ptr = (tmp) ? tmp : src;
	out_ptr = dest;
	len_in = (tmp) ? tlen : slen;
	len_out = dlen;

#if HAVE_ICONV
	
	while (1) {
		iconv(mdb->iconv_in, &in_ptr, &len_in, &out_ptr, &len_out);
		if ((!len_in) || (errno == E2BIG)) break;
		/* Don't bail if impossible conversion is encountered */
		in_ptr += (IS_JET4(mdb)) ? 2 : 1;
		len_in -= (IS_JET4(mdb)) ? 2 : 1;
		*out_ptr++ = '?';
		len_out--;
	}
	
	dlen -= len_out;
#else
	if (IS_JET3(mdb)) {
		strncpy(out_ptr, in_ptr, len_in);
		dlen = len_in;
	} else {
		/* rough UCS-2LE to ISO-8859-1 conversion */
		unsigned int i;
		for (i=0; i<len_in; i+=2)
			dest[i/2] = (in_ptr[i+1] == 0) ? in_ptr[i] : '?';
		dlen = len_in/2;
	}
#endif

	if (tmp) g_free(tmp);
	dest[dlen]='\0';
	
	return dlen;
}

/*
 * This function is used in writing text data to an MDB table.
 * If slen is 0, strlen will be used to calculate src's length.
 */
int
mdb_ascii2unicode(MdbHandle *mdb, char *src, size_t slen, char *dest, size_t dlen)
{
        size_t len_in, len_out;
        char *in_ptr, *out_ptr;

	if ((!src) || (!dest) || (!dlen))
		return 0;

        in_ptr = src;
        out_ptr = dest;
        len_in = (slen) ? slen : strlen(in_ptr);
        len_out = dlen;

#ifdef HAVE_ICONV
	iconv(mdb->iconv_out, &in_ptr, &len_in, &out_ptr, &len_out);
	
	dlen -= len_out;
#else
	if (IS_JET3(mdb)) {
		dlen = MIN(len_in, len_out);
		strncpy(out_ptr, in_ptr, dlen);
	} else {
		unsigned int i;
		slen = MIN(len_in, len_out/2);
		dlen = slen*2;
		for (i=0; i<slen; i++) {
			out_ptr[i*2] = in_ptr[i];
			out_ptr[i*2+1] = 0;
		}
	}
#endif

	/* Unicode Compression */
	if(IS_JET4(mdb) && (dlen>4)) {
		unsigned char *tmp = g_malloc(dlen);
		unsigned int tptr = 0, dptr = 0;
		int comp = 1;

		tmp[tptr++] = 0xff;
		tmp[tptr++] = 0xfe;
		while((dptr < dlen) && (tptr < dlen)) {
			if (((dest[dptr+1]==0) && (comp==0))
			 || ((dest[dptr+1]!=0) && (comp==1))) {
				/* switch encoding mode */
				tmp[tptr++] = 0;
				comp = (comp) ? 0 : 1;
			} else if (dest[dptr]==0) {
				/* this string cannot be compressed */
				tptr = dlen;
			} else if (comp==1) {
				/* encode compressed character */
				tmp[tptr++] = dest[dptr];
				dptr += 2;
			} else if (tptr+1 < dlen) {
				/* encode uncompressed character */
				tmp[tptr++] = dest[dptr];
				tmp[tptr++] = dest[dptr+1];
				dptr += 2;
			} else {
				/* could not encode uncompressed character
				 * into single byte */
				tptr = dlen;
			}
		}
		if (tptr < dlen) {
			memcpy(dest, tmp, tptr);
			dlen = tptr;
		}
		g_free(tmp);
	}

	return dlen;
}

void mdb_iconv_init(MdbHandle *mdb)
{
	const char *iconv_code;

	/* check environment variable */
	if (!(iconv_code=getenv("MDBICONV"))) {
		iconv_code="UTF-8";
	}

#ifdef HAVE_ICONV
        if (IS_JET4(mdb)) {
                mdb->iconv_out = iconv_open("UCS-2LE", iconv_code);
                mdb->iconv_in = iconv_open(iconv_code, "UCS-2LE");
        } else {
		/* According to Microsoft Knowledge Base pages 289525 and */
		/* 202427, code page info is not contained in the database */

		/* check environment variable */
		const char *jet3_iconv_code_from_env = getenv("MDB_JET3_CHARSET");
		if (jet3_iconv_code_from_env)
			mdb_set_encoding(mdb, jet3_iconv_code_from_env);
		else if (!mdb->jet3_iconv_code) {
#ifdef _WIN32
			// get the default from OS
			char default_encoding[] = "CP       ";
			if (GetLocaleInfoA( MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), SORT_DEFAULT), 
			 LOCALE_IDEFAULTANSICODEPAGE, default_encoding+2, sizeof(default_encoding)-2-1 ))
				mdb->jet3_iconv_code = g_strdup(default_encoding);
			else
#endif
				mdb->jet3_iconv_code = g_strdup("CP1252");
		}

		mdb->iconv_out = iconv_open(mdb->jet3_iconv_code, iconv_code);
		mdb->iconv_in = iconv_open(iconv_code, mdb->jet3_iconv_code);
	}
#endif
}
void mdb_iconv_close(MdbHandle *mdb)
{
#ifdef HAVE_ICONV
        if (mdb->iconv_out != (iconv_t)-1) iconv_close(mdb->iconv_out);
        if (mdb->iconv_in != (iconv_t)-1) iconv_close(mdb->iconv_in);
#endif
}
