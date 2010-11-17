/*
 * bsonserializer.h
 * Defines a jsonobject_t type which allows read/write access to a BSON
 * encoded buffer.  See http://bsonspec.org
 *
 * There are three access mechanisms to BSON documents:
 * 	1. Construction via the jsonvisitor interface
 *  2. Producing jsonvisitor events from a BSON document
 *  3. Querying and iterating over members
 */
#ifndef __BSONSERIALIZER_H__
#define __BSONSERIALIZER_H__

#include "json/jsonaccess.h"
#include "util/stringutil.h"

typedef struct {
	/**
	 * The offset within the buffer where this document starts.
	 */
	size_t offset;

	/**
	 * The type of this document ('O' = Object, 'A' = Array, 'X' = Initial, 'R' = Root).
	 */
	char type;

	/**
	 * Count of members.  Primarily used for array sequence numbering.
	 */
	uint32_t member_count;
} bsondocstate_t;

typedef struct {
	/**
	 * By making this the first element, this type can be used
	 * interchangeably as the jsonvisitor.
	 */
	jsonvisitor_t jsonvisitor;

	/**
	 * Accumulate into the given buffer
	 */
	stringwriter_t *outputbuffer;

	bool has_error;
	const char *error_msg;

	/**
	 * Variable length array of bsondocstate_t for each level we are persisting.
	 */
	bsondocstate_t *docstate_stack;
	int docstate_stack_capacity;
	int docstate_stack_index;

	/**
	 * In the visitor protocol, labels are delivered before the value they label
	 * but in BSON, the label comes after the type specifier.  Therefore, we buffer
	 * labels here.
	 */
	jsonstring_t label_lookaside;
	size_t label_lookaside_capacity;

} bsonserializer_t;

/**
 * bsonserializer constructor destructor
 */
void bsonserializer_init(bsonserializer_t *self, stringwriter_t *outputbuffer);
void bsonserializer_destroy(bsonserializer_t *self);

#endif /* JSONOBJECT_H_ */
