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

#endif
