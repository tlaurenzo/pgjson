#include "jsonutil.h"
#include "jsonbinaryconst.h"

#define JSONPARSE_EXTRA_DECL \
	dynbuffer_t *dest; \
	char error_message[256];

#define DEST (parsestate->dest)

/* number of bytes of length to reserve for objects and arrays */
#define RESERVE_LENGTH 1

/**
 * Write type and length bytes
 */
static void write_type_length(dynbuffer_t *dest, uint8_t type, uint32_t length)
{
	int i;
	uint32_t lenchunk;
	uint8_t nextbyte;

	/* reserve 5 bytes so we can do unchecked writes */
	dynbuffer_ensure_delta(dest, 5);

	/* byte 1: Type spec */
	lenchunk=(length&0x0f);
	length>>=4;
	nextbyte=(type<<JSONBINARY_TYPE_SHIFT) | lenchunk;
	if (!length) {
		/* 4 bit length */
		dynbuffer_append_byte_nocheck(dest, nextbyte);
		return;
	} else {
		dynbuffer_append_byte_nocheck(dest, nextbyte | JSONBINARY_TYPE_LENGTHCONT);
	}

	/* byte 2-5 */
	/* the compiler should unroll this loop */
	for (i=0; i<4; i++) {
		lenchunk=length & 0x7f;
		length>>=7;
		if (!length) {
			/* 11 bit length */
			dynbuffer_append_byte_nocheck(dest, lenchunk);
			return;
		} else {
			dynbuffer_append_byte_nocheck(dest, lenchunk | 0x80);
		}
	}
}

/**
 * finalizes an object or array given the startpos and RESERVE_LENGTH.
 * goes back and writes the type+length bytes, moving the buffer as necessary
 * to ensure length alignment
 */
static void finalize_object_array(dynbuffer_t *dest, uint8_t type, uint32_t startpos)
{
	uint32_t actlen=dest->pos - startpos - RESERVE_LENGTH - 1;
	uint32_t lenchunk;
	uint32_t lenshift;
	uint8_t lenbytes[4];
	uint32_t lenindex;

	/* ensure we have max needed room to grow */
	dynbuffer_ensure_delta(dest, 5);

	/* fast path for empty object/array */
	if (!actlen) {
		dest->pos=startpos;
		dynbuffer_append_byte_nocheck(dest, (type<<JSONBINARY_TYPE_SHIFT));
		return;
	}

	/* write the type code+4bit length */
	lenchunk=actlen&0x0f;
	lenshift=actlen>>4;

	if (!lenshift) {
		dest->contents[startpos]=(type<<JSONBINARY_TYPE_SHIFT) | lenchunk;
		if (RESERVE_LENGTH>0) {
			/* move left by RESERVE_LENGTH */
			memmove(dest->contents+startpos+1, dest->contents+startpos+1+RESERVE_LENGTH, actlen);
			dest->pos-=RESERVE_LENGTH;
		}
		return;
	} else {
		/* more than the typespec byte - write typespec with continue bit set */
		dest->contents[startpos]=(type<<JSONBINARY_TYPE_SHIFT) | lenchunk | JSONBINARY_TYPE_LENGTHCONT;
	}

	/* write bytes 2-5 */
	for (lenindex=0; lenindex<4; lenindex++) {
		lenchunk=lenshift&0x7f;
		lenshift>>=7;

		if (!lenshift) {
			/* end of length */
			lenbytes[lenindex]=lenchunk;
			lenindex++;
			break;
		} else {
			/* continues */
			lenbytes[lenindex]=lenchunk | 0x80;
		}
	}

	/* now determine how much to move the buffer */
	if (lenindex==RESERVE_LENGTH) {
		/* reserved byte count was correct guess */
		/* add the lenbytes and return - all pointers are correct */
		memcpy(dest->contents+startpos+1, lenbytes, RESERVE_LENGTH);
		return;
	} else {
		/* move the object contents forward or back */
		memmove(dest->contents+startpos+1+lenindex, dest->contents+startpos+1+RESERVE_LENGTH, actlen);

		/* adjust pos */
		if (lenindex>RESERVE_LENGTH) dest->pos+=(lenindex-RESERVE_LENGTH);
		else dest->pos-=(RESERVE_LENGTH-lenindex);

		/* add the lenbytes */
		memcpy(dest->contents+startpos+1, lenbytes, lenindex);
		return;
	}
}

/**
 * append a string to dest as modified utf8
 */
static void write_object_label(dynbuffer_t *dest, uint8_t *s, size_t len)
{
	size_t index;
	uint8_t c;

	/* reserve len*2 bytes - the neurotic case of all nulls */
	dynbuffer_ensure_delta(dest, len*2+1);

	for (index=0; index<len; index++) {
		c=s[index];
		if (c) dynbuffer_append_byte_nocheck(dest, c);
		else {
			dynbuffer_append_byte_nocheck(dest, 0xc0);
			dynbuffer_append_byte_nocheck(dest, 0x80);
		}
	}
	dynbuffer_append_byte_nocheck(dest, 0);
}

/* actions */
#define JSONPARSE_ACTION_OBJECT_START() \
	uint32_t startpos=DEST->pos; \
	DEST->pos+=RESERVE_LENGTH+1;
#define JSONPARSE_ACTION_OBJECT_LABEL(fieldindex, s, len) \
	write_object_label(DEST, s, len);
#define JSONPARSE_ACTION_OBJECT_END() \
	finalize_object_array(DEST, JSONBINARY_TYPE_OBJECT, startpos);


#define JSONPARSE_ACTION_ARRAY_START() \
	uint32_t startpos=DEST->pos; \
	DEST->pos+=RESERVE_LENGTH+1;
#define JSONPARSE_ACTION_ARRAY_END() \
	finalize_object_array(DEST, JSONBINARY_TYPE_ARRAY, startpos);


#define JSONPARSE_ACTION_VALUE_NULL() {\
	dynbuffer_ensure_delta(DEST, 2); \
	dynbuffer_append_byte_nocheck(DEST, JSONBINARY_SS_PREFIX); \
	dynbuffer_append_byte_nocheck(DEST, JSONBINARY_SS_DATA_NULL); \
	}

#define JSONPARSE_ACTION_VALUE_UNDEFINED() {\
	dynbuffer_ensure_delta(DEST, 2); \
	dynbuffer_append_byte_nocheck(DEST, JSONBINARY_SS_PREFIX); \
	dynbuffer_append_byte_nocheck(DEST, JSONBINARY_SS_DATA_UNDEFINED); \
	}

#define JSONPARSE_ACTION_VALUE_BOOL(bl) {\
	dynbuffer_ensure_delta(DEST, 2); \
	dynbuffer_append_byte_nocheck(DEST, JSONBINARY_SS_PREFIX); \
	dynbuffer_append_byte_nocheck(DEST, (bl ? JSONBINARY_SS_DATA_TRUE : JSONBINARY_SS_DATA_FALSE)); \
	}

#define JSONPARSE_ACTION_VALUE_NUMERIC(s, len) {\
	write_type_length(DEST, JSONBINARY_TYPE_NUMBER, len); \
	dynbuffer_append(DEST, s, len); \
	}

#define JSONPARSE_ACTION_VALUE_STRING(s, len) {\
	write_type_length(DEST, JSONBINARY_TYPE_STRING, len); \
	dynbuffer_append(DEST, s, len); \
	}

#define JSONPARSE_ACTION_ERROR(msg, got) \
	snprintf(parsestate->error_message, sizeof(parsestate->error_message), \
			"Error: %s (got %s)", msg, jsonlex_token_str(got));


#include "jsonlex.inc.c"
#include "jsonparse.inc.c"

bool json_transcode_json_to_binary(uint8_t *source, size_t sourcelen, dynbuffer_t *dest)
{
	bool result;
	jsonparseinfo_t parseinfo;

	/* init the lexer */
	jsonlex_init_io(&parseinfo.lexstate, source, sourcelen);
	parseinfo.dest=dest;
	parseinfo.error_message[0]=0;

	result=jsonparse(&parseinfo);

	if (!result) {
		dest->pos=0;
		dynbuffer_append(dest, parseinfo.error_message, strlen(parseinfo.error_message));
		dynbuffer_append_byte(dest, 0);
	}

	/* destroy */
	jsonlex_destroy(&parseinfo.lexstate);

	return result;
}

