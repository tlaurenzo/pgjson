/*
 * bsonparser.h
 * Defining value semantics for bson structures.
 *
 * BSON was designed as an interchange format for Documents, which correspond to
 * JSON objects and arrays.  It does have one quirk with respect to this, though
 * in that it is only possible to store an "object" as the root of the tree.  This
 * is due to the fact that the type of an element is determined based on a type
 * code that prefixes the element name and value within a document and there is no
 * type code prefix for the root.
 *
 * If the root were also polymorphic, like elements, then bson would also be a
 * workable format for passing arbitrary document fragments.  Unfortunately, there
 * is no header for a bson document, so on first inspection, there are no bits to
 * hijack in order to make the format more flexible by introducing an escape.  Here
 * is the definition of the root BSON Document:
 *    document ::= int32 e_list "\x00"
 * Upon closer inspection we see that the int32 document length is signed (all ints
 * in bson are signed - presumably in homage to the signed nature of most of the
 * target environments).  Since the value represents a size, negative numbers have
 * no significance -- meaning that we have exactly one bit to introduce an escape. If
 * the high order bit of the document size on the root is 1 (negative value), then
 * it is not a valid BSON document, meaning it can be a valid "something else".
 *
 * Taking some liberties with the BNF from bsonspec.org, the tag_byte is the negated
 * value of the element leading byte from the "element" type.  For example, "\xff"
 * is -1 in decimal and would be a double (normal tag value is "\x01").  Similarly,
 * "\xfe" is -2 and a string.
 *
 * See the extended_bson.txt file in the project root for a full grammar.
 *
 * This makes the values (-255, -1) of the leading int32 reserved, denoting that this
 * is a "bson value" to be interpreted as a single value with the tag -prefix.  For
 * compatibility with existing parsers, the rest of the element structure (including
 * the e_name) is left intact, but the e_name can be just a single null value.
 */

#ifndef __BSONPARSER_H__
#define __BSONPARSER_H__

#include "json/bsonconst.h"
#include "json/jsonaccess.h"

typedef union {
	/**** Primitives ****/
	/**
	 * BSONTYPE_DOUBLE
	 * (only access this field if using an aligned variant)
	 */
	double double_value;

	/** BSONTYPE_OBJID **/
	char object_id_value[12];

	/**
	 * BSONTYPE_BOOL
	 */
	uint8_t bool_value;

	/**
	 * BSONTYPE_DATETIME
	 */
	int64_t datetime_value;

	/**
	 * BSONTYPE_INT32
	 */
	int32_t int32_value;

	/**
	 * BSONTYPE_TIMESTAMP
	 */
	int64_t timestamp_value;

	/**
	 * BSONTYPE_INT64
	 */
	int64_t int64_value;

	/*** Variable sized types ***/

	/**
	 * BSONTYPE_DOCUMENT
	 * BSONTYPE_ARRAY
	 */
	struct {
		/**
		 * The docsize.  The size of the data pointed to by docstart.  The original value
		 * is four bytes bigger to account for the length.
		 */
		int32_t docsize;
		/**
		 * Pointer to the first byte byte of the element
		 * list
		 */
		uint8_t *docstart;
	} document;

	/**
	 * BSONTYPE_STRING
	 * BSONTYPE_SYMBOL
	 */
	struct {
		/**
		 * Number of bytes +1
		 */
		int32_t length;
		/**
		 * length bytes (includes null which is not part of string)
		 */
		uint8_t *contents;
	} string;

	/**
	 * BSONTYPE_JSCODE
	 * BSONTYPE_JSCODE_WSCOPE
	 */
	struct {
		int32_t length;
		uint8_t *contents;
		void *document;
	} jscode_wscope;

	/**
	 * BSONTYPE_REGEX
	 */
	struct {
		char *expression;
		char *options;
	} regex;

	/**
	 * BSONTYPE_BINARY
	 */
	struct {
		int32_t length;
		uint8_t *contents;
		int8_t subtype;
	} binary;
} bsonvariant_t;

typedef struct {
	/**
	 * The type of this value
	 */
	bsontype_t type;

	/**
	 * Pointer to the label.  The label is a zero terminated,
	 * modified UTF-8 string.  This is a pointer directly
	 * into the source buffer.
	 */
	char *label;

	/**
	 * Union of all possible types.  This is an aligned structure.
	 */
	bsonvariant_t value;

	/**
	 * The first byte of the value
	 */
	uint8_t *value_start;
	/**
	 * The first byte past the current value
	 */
	uint8_t *value_next;

	/**
	 * The number of bytes remaining in this value (from the original bsonvalue_load).
	 */
	size_t remaining;
} bsonvalue_t;

typedef enum {
	/**
	 * The given buffer is a "root" buffer
	 */
	BSONMODE_ROOT,

	/**
	 * The given buffer is from an element list
	 */
	BSONMODE_ELEMENT
} bsonvalue_mode_t;
/**
 * Loads the given bsonvalue_t from the given buffer, making sure to not
 * consume more than.
 * Return true if valid
 */
bool bsonvalue_load(bsonvalue_t *bsv, uint8_t *source, size_t source_len, bsonvalue_mode_t mode);

/**
 * Seeks to the next value in the buffer, returning false if no more or invalid.
 */
bool bsonvalue_next(bsonvalue_t *bsv);

/**
 * Parse a given buffer, generating visitor events
 */
int32_t bsonparser_parse(uint8_t *buffer, size_t len, jsonvisitor_t *visitor);

#endif

