/**
 * bsonliteral.h
 * Accessors for some constant BSON structures
 */
#ifndef __BSONLITERAL_H__
#define __BSONLITERAL_H__

extern const uint8_t BSONLITERAL_UNDEFINED[4];
extern const uint8_t BSONLITERAL_NULL[4];
extern const uint8_t BSONLITERAL_TRUE[5];
extern const uint8_t BSONLITERAL_FALSE[5];
extern const uint8_t BSONLITERAL_EMPTYOBJ[5];
extern const uint8_t BSONLITERAL_EMPTYARRAY[9];
extern const uint8_t BSONLITERAL_EMPTYSTRING[9];
extern const uint8_t BSONLITERAL_ZERO_INT32[8];
extern const uint8_t BSONLITERAL_ZERO_INT64[12];
extern const uint8_t BSONLITERAL_ZERO_DOUBLE[12];
extern const uint8_t BSONLITERAL_ZERO_DATETIME[12];

#define BSONLITERAL_SIZE_INT32 sizeof(BSONLITERAL_ZERO_INT32)
void BSONLITERAL_MAKE_INT32(uint8_t *buffer, int32_t v);

#define BSONLITERAL_SIZE_INT64 sizeof(BSONLITERAL_ZERO_INT64)
void BSONLITERAL_MAKE_INT64(uint8_t *buffer, int64_t v);

#define BSONLITERAL_SIZE_DOUBLE sizeof(BSONLITERAL_ZERO_DOUBLE)
void BSONLITERAL_MAKE_DOUBLE(uint8_t *buffer, double v);

#define BSONLITERAL_SIZE_BOOL sizeof(BSONLITERAL_TRUE)
void BSONLITERAL_MAKE_BOOL(uint8_t *buffer, bool v);

#define BSONLITERAL_SIZE_DATETIME sizeof(BSONLITERAL_ZERO_DATETIME)
void BSONLITERAL_MAKE_DATETIME(uint8_t *buffer, int64_t msv);

#define BSONLITERAL_SIZE_STRING(bytes) (8+bytes+1)
void BSONLITERAL_MAKE_STRING(uint8_t *buffer, uint8_t *utf8bytes, size_t len);

#endif
