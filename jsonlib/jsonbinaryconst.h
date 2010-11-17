/**
 * jsonbinaryconst.h
 * Constants for the json binary representation
 */
#ifndef __JSONBINARYCONST_H__
#define __JSONBINARYCONST_H__

#define JSONBINARY_TYPE_OBJECT (0x00)
#define JSONBINARY_TYPE_ARRAY (0x01)
#define JSONBINARY_TYPE_STRING (0x02)
#define JSONBINARY_TYPE_NUMBER (0x03)
#define JSONBINARY_TYPE_SS (0x04)
#define JSONBINARY_TYPE_TSTRING (0x05)
#define JSONBINARY_TYPE_SBINARY (0x06)
#define JSONBINARY_TYPE_RESERVED (0x07)
#define JSONBINARY_TYPE_SHIFT 5

/**
 * Length continuation bit for the type byte
 */
#define JSONBINARY_TYPE_LENGTHCONT 0x10

/**
 * When writing a simple scalar, write this byte followed
 * by the JSONBINARY_SS_DATA_* constant
 */
#define JSONBINARY_SS_PREFIX (0x81)
#define JSONBINARY_SS_DATA_FALSE (0x00)
#define JSONBINARY_SS_DATA_TRUE (0x01)
#define JSONBINARY_SS_DATA_NULL (0x02)
#define JSONBINARY_SS_DATA_UNDEFINED (0x03)

#endif
