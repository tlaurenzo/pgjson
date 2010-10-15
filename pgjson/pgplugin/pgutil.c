/*** backporting a couple of things from postgres 9 ***/
/*-------------------------------------------------------------------------
 *
 * encode.c
 *	  Various data encoding/decoding things.
 *
 * Copyright (c) 2001-2010, PostgreSQL Global Development Group
 *
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/backend/utils/adt/encode.c,v 1.26 2010/01/02 16:57:53 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include "pgutil.h"

static const char hextbl[] = "0123456789abcdef";

static const int8 hexlookup[128] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

unsigned
pgjson_hex_encode(const char *src, unsigned len, char *dst)
{
	const char *end = src + len;

	while (src < end)
	{
		*dst++ = hextbl[(*src >> 4) & 0xF];
		*dst++ = hextbl[*src & 0xF];
		src++;
	}
	return len * 2;
}

static inline char
get_hex(char c)
{
	int			res = -1;

	if (c > 0 && c < 127)
		res = hexlookup[(unsigned char) c];

	if (res < 0)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("invalid hexadecimal digit: \"%c\"", c)));

	return (char) res;
}

unsigned
pgjson_hex_decode(const char *src, unsigned len, char *dst)
{
	const char *s,
			   *srcend;
	char		v1,
				v2,
			   *p;

	srcend = src + len;
	s = src;
	p = dst;
	while (s < srcend)
	{
		if (*s == ' ' || *s == '\n' || *s == '\t' || *s == '\r')
		{
			s++;
			continue;
		}
		v1 = get_hex(*s++) << 4;
		if (s >= srcend)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				  errmsg("invalid hexadecimal data: odd number of digits")));

		v2 = get_hex(*s++);
		*p++ = v1 | v2;
	}

	return p - dst;
}
