/**
 * jsonpath.h
 *
 * Provides evaluation mechanisms for traversing a JSON object.
 *
 * This module provides four facilities:
 *   1. Binary structure for representing a "JSON Path"
 *   2. Parser to convert a text "JSON Path" language into the binary structure
 *   3. Serializer to convert a binary JSON Path to a text representation
 *   3. Evaluator to apply a JSON Path to a bsonvalue_t
 *
 * Unlike more full featured languages like XPATH, JSON Path is
 * not intended to be a stand-alone, full featured language and is expected
 * to be embedded into a more full featured "big brother".
 *
 * It is really just a simple expression of JavaScript "dot and bracket" notation for
 * drilling into a nested data structure.
 *
 * The text format is best illustrated with examples:
 *   - account.first_name
 *   - accounts[1].first_name
 *   - account['first_name']
 *   - accounts.length
 *
 * There is no functional difference between using dots and brackets.  Syntactically,
 * the bracket notation can represent any value whereas the dot literal notation cannot
 * include special characters.
 *
 * The binary format is simple:
 *   stream ::= expression* "\x00"
 *
 *   expression ::= <TYPECODE> cstring
 *
 *   cstring ::= modified UTF-8 null terminated string
 *
 *   TYPECODE ::=
 *              "\x01"     (simplename)
 *
 * Evaluating JSON Path is a simple matter of descending the object tree looking
 * for matches.  Some value types expose "virtual" fields:
 *   - array: "length"
 *   - binary: "length", integral access to contents
 *   - string: "length", integral access to contents
 * Evaluation stops at the first undefined field, returning undefined.
 */
#ifndef __JSONPATH_H__
#define __JSONPATH_H__

#include "util/stringutil.h"
#include "json/bsonparser.h"

typedef enum {
	JSONPATHTYPE_IDENTIFIER=1,
	JSONPATHTYPE_STRING=2,
	JSONPATHTYPE_NUMERIC=3,
	/* refers to the special "length" property of some types.  otherwise, treated as identifier */
	JSONPATHTYPE_LENGTH=4,

	/* for range checks */
	JSONPATHTYPE_MAXVALUE=4
} jsonpathtype_t;

typedef struct {
	/* source */
	const uint8_t *buffer;
	const uint8_t *bufferlimit;

	/* current value */
	jsonpathtype_t current_type;
	/* current_value modified utf8 stringz */
	const uint8_t *current_value;
	/* length of current_value (not counting null) */
	size_t current_value_length;

	/* next value */
	const uint8_t *next_value;
} jsonpathiter_t;

/* construction */
/**
 * Appends another path element to a buffer.  This will both remove an existing terminating
 * null (if present) and add one at the new end.
 */
void jsonpath_append(stringwriter_t *buffer, jsonpathtype_t type, uint8_t *utf8string, size_t stringlen);

/* iteration */
/**
 * Begin iteration of a jsonpath.
 * The proper idom for use is:
 * 		jsonpath_iter_begin(...);
 * 		while (jsonpath_iter_next(...)) {
 * 			// do something
 * 		}
 */
void jsonpath_iter_begin(jsonpathiter_t *iter, const uint8_t *buffer, size_t bufferlen);
/**
 * Position the iterator at the next component of the path.  Return true if there is
 * a next.
 */
bool jsonpath_iter_next(jsonpathiter_t *iter);


/* parsing and serialization */

/**
 * Internal parsestate structure used to coordinate with lex and yacc.
 */
typedef struct {
	/* output buffer passed to jsonpath_parse */
	stringwriter_t *bufferout;

	void *scanner;
	bool has_error;
	char *error_msg;

	/* input stream */
	uint8_t *stream;
	uint8_t *streamlimit;

	/* accumulates strings */
	char string_mode;
	stringwriter_t string_accum;
} jsonpath_parsestate_t;

/**
 * Parse the given text representation into a binary representation, written to buffer.
 * Return true on successful parse.  If unsuccessful parse, then false is returned
 * and the bufferout contains the error message.
 */
bool jsonpath_parse(stringwriter_t *bufferout, uint8_t *utf8stream, size_t streamlen);

/**
 * Serializes the iterator to textout.  Return true on success, False otherwise.
 */
bool jsonpath_serialize(stringwriter_t *textout, jsonpathiter_t *iter);

/* evaluation */
/**
 * Evaluates the given path iterator against the given root bsonvalue, returning true
 * on successful evaluation (success also includes resolving to undefined).
 * On success, the bsonvalue_t structure pointed to by resultout will be populated
 * with the matched value.  In some cases, this may be a "synthetic" value not
 * backed by anything that actually exists in the root buffer, so the only fields
 * of universal relevence are type and value.
 */
bool jsonpath_evaluate(bsonvalue_t *root, jsonpathiter_t *iter, bsonvalue_t *resultout);

#endif
