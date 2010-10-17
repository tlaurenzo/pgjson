#include "util/setup.h"
#include "json/bsonliteral.h"
#include "json/bsonconst.h"

const uint8_t BSONLITERAL_UNDEFINED[4] = { 0xfa, 0xff, 0xff, 0xff };
const uint8_t BSONLITERAL_NULL[4] = { 0xf6, 0xff, 0xff, 0xff };
const uint8_t BSONLITERAL_TRUE[5] = { 0xf8, 0xff, 0xff, 0xff, 0x01 };
const uint8_t BSONLITERAL_FALSE[5] = { 0xf8, 0xff, 0xff, 0xff, 0x00 };
const uint8_t BSONLITERAL_EMPTYOBJ[5] = { 0x05, 0x00, 0x00, 0x00, 0x00 };
const uint8_t BSONLITERAL_EMPTYARRAY[9] = { 0xfc, 0xff, 0xff, 0xff, 0x05, 0x00, 0x00, 0x00, 0x00 };
const uint8_t BSONLITERAL_EMPTYSTRING[9] = { 0xfe, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00 };
const uint8_t BSONLITERAL_ZERO_INT32[8] = { 0xf0, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 };
const uint8_t BSONLITERAL_ZERO_INT64[12] = { 0xef, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const uint8_t BSONLITERAL_ZERO_DOUBLE[12] = { 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const uint8_t BSONLITERAL_ZERO_DATETIME[12] = { 0xf7, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

void BSONLITERAL_MAKE_INT32(uint8_t *buffer, int32_t v)
{
	memcpy(buffer, BSONLITERAL_ZERO_INT32, 4);
	v=BSON_SWAP_HTOB32(v);
	memcpy(buffer+4, &v, 4);
}

void BSONLITERAL_MAKE_INT64(uint8_t *buffer, int64_t v)
{
	memcpy(buffer, BSONLITERAL_ZERO_INT64, 4);
	v=BSON_SWAP_HTOB64(v);
	memcpy(buffer+4, &v, 8);
}

void BSONLITERAL_MAKE_DOUBLE(uint8_t *buffer, double v)
{
	bsondouble_t uv;
	uv.doublevalue=v;
	uv.intvalue=BSON_SWAP_HTOB64(uv.intvalue);

	memcpy(buffer, BSONLITERAL_ZERO_DOUBLE, 4);
	memcpy(buffer+4, &v, 8);
}

void BSONLITERAL_MAKE_BOOL(uint8_t *buffer, bool v)
{
	memcpy(buffer, v ? BSONLITERAL_TRUE : BSONLITERAL_FALSE, sizeof(BSONLITERAL_TRUE));
}

void BSONLITERAL_MAKE_DATETIME(uint8_t *buffer, int64_t msv)
{
	memcpy(buffer, BSONLITERAL_ZERO_INT64, 4);
	msv=BSON_SWAP_HTOB64(msv);
	memcpy(buffer+4, &msv, 8);
}

void BSONLITERAL_MAKE_STRING(uint8_t *buffer, uint8_t *utf8bytes, size_t len)
{
	/* swap size plus one for term null */
	int32_t lenswp=BSON_SWAP_HTOB32((int32_t)(len+1));

	/* type header */
	buffer[0]=0xfe;
	buffer[1]=0xff;
	buffer[2]=0xff;
	buffer[3]=0xff;

	/* string len */
	memcpy(buffer+4, &lenswp, 4);

	/* data */
	memcpy(buffer+8, utf8bytes, len);

	/* add term null */
	buffer[len+8]=0;
}
