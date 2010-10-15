/**
 * bsonconst.h
 * Constants for bson structures
 */
#ifndef __BSONCONST_H__
#define __BSONCONST_H__

typedef enum {
	BSONTYPE_DOUBLE=0x1,
	BSONTYPE_STRING=0x2,
	BSONTYPE_DOCUMENT=0x3,
	BSONTYPE_ARRAY=0x4,
	BSONTYPE_BINARY=0x5,
	BSONTYPE_UNDEFINED=0x6,
	BSONTYPE_OBJID=0x7,
	BSONTYPE_BOOL=0x8,
	BSONTYPE_DATETIME=0x9,
	BSONTYPE_NULL=0xa,
	BSONTYPE_REGEX=0xb,
	BSONTYPE_DBPOINTER=0xc,
	BSONTYPE_JSCODE=0xd,
	BSONTYPE_SYMBOL=0xe,
	BSONTYPE_JSCODE_WSCOPE=0xf,
	BSONTYPE_INT32=0x10,
	BSONTYPE_TIMESTAMP=0x11,
	BSONTYPE_INT64=0x12,

	/* use this to range check */
	BSONTYPE_MAX_KNOWN=0x12
} bsontype_t;

typedef enum {
	BSONBINTYPE_GENERIC=0x0,
	BSONBINTYPE_FUNCTION=0x1,
	BSONBINTYPE_OLDBIN=0x2,
	BSONBINTYPE_UUID=0x3,
	BSONBINTYPE_MD5=0x5,
	BSONBINTYPE_USERDEFINED=0x80
} bsonbintype_t;

typedef union {
	double   doublevalue;
	uint64_t intvalue;
} bsondouble_t;

/** TODO: Implement BSON byte swap functions for big-endian machines **/
#define BSON_SWAP_HTOB32(host_int32) host_int32
#define BSON_SWAP_HTOB64(host_int64) host_int64
#define BSON_SWAP_BTOH32(bson_int32) bson_int32
#define BSON_SWAP_BTOH64(bson_int64) bson_int64


#endif
