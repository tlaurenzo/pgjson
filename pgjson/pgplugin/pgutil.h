#ifndef __PGUTIL_H__
#define __PGUTIL_H__

unsigned
pgjson_hex_encode(const char *src, unsigned len, char *dst);

unsigned
pgjson_hex_decode(const char *src, unsigned len, char *dst);

#endif
