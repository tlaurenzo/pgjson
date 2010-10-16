#include "util/setup.h"
#include "json/bsonliteral.h"

const uint8_t BSONLITERAL_UNDEFINED[4] = { 0xfa, 0xff, 0xff, 0xff };
const uint8_t BSONLITERAL_NULL[4] = { 0xf6, 0xff, 0xff, 0xff };
const uint8_t BSONLITERAL_TRUE[5] = { 0xf8, 0xff, 0xff, 0xff, 0x01 };
const uint8_t BSONLITERAL_FALSE[5] = { 0xf8, 0xff, 0xff, 0xff, 0x00 };
const uint8_t BSONLITERAL_EMPTYOBJ[5] = { 0x05, 0x00, 0x00, 0x00, 0x00 };
const uint8_t BSONLITERAL_EMPTYARRAY[9] = { 0xfc, 0xff, 0xff, 0xff, 0x05, 0x00, 0x00, 0x00, 0x00 };
const uint8_t BSONLITERAL_EMPTYSTRING[9] = { 0xfe, 0xff, 0xff, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00 };
const uint8_t BSONLITERAL_ZERO_INT32[8] = { 0xf0, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 };
const uint8_t BSONLITERAL_ZERO_INT64[12] = { 0xef, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };