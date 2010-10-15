/**
 * jsonaccess.h
 * Abstracts access to json-like persistent representations in an oo-like fashion.
 * This module provides some supporting infrastructure but actual storage is left
 * to others.
 *
 * There are several interfaces for interacting with a json store:
 *   - jsonwriter_t: A visitor interface for building a json store
 *     by calling methods on the object in a depth-first traversal
 *     of the json object.
 *
 * The "base" json object is always represented as a jsonobject_t, which is a
 * "polymorphic" structure where all access is sent through its vft (first member).
 */
#ifndef __JSONACCESS_H__
#define __JSONACCESS_H__
#include <stdbool.h>

/**
 * jsonstring_t
 * It is expected that json strings are UTF-8 encoded byte arrays with an explicit length.
 * Memory management is up to the caller.  Depending on the context, instances may just
 * be buffers with no encoding.
 */
typedef struct jsonstring_t {
	size_t length;
	char *bytes;
} jsonstring_t;

/**** jsonvisitor_t class ****/
struct jsonvisitor_vft;
typedef struct jsonvisitor_t {
	struct jsonvisitor_vft const *vft;

} jsonvisitor_t;
typedef struct jsonvisitor_vft {
	/**
	 * Tracks whether an error has occurred in visiting, optionally storing a message in *out_msg.
	 * Return true/false
	 */
	bool (*has_error)(jsonvisitor_t *self, const char **out_msg);

	/**
	 * Signals that the next value to be stored is a labeled field in an object and this
	 * is its label.
	 */
	bool (*push_label)(jsonvisitor_t *self, jsonstring_t *label);

	/**
	 * Starts a new value as a JSON object literal.  Must be balanced by a call
	 * to end_object.
	 */
	bool (*start_object)(jsonvisitor_t *self);

	/**
	 * Ends a JSON literal object initiated with a call to start_object
	 */
	bool (*end_object)(jsonvisitor_t *self);

	/**
	 * Starts a new value as a JSON array literal.  Must be balanced by a call
	 * to end_array.
	 */
	bool (*start_array)(jsonvisitor_t *self);

	/**
	 * Ends a JSON literal array initiated with a call to start_array
	 */
	bool (*end_array)(jsonvisitor_t *self);

	bool (*add_empty_object)(jsonvisitor_t *self);
	bool (*add_empty_array)(jsonvisitor_t *self);
	bool (*add_bool)(jsonvisitor_t *self, bool bvalue);
	bool (*add_int32)(jsonvisitor_t *self, int32_t intvalue);
	bool (*add_int64)(jsonvisitor_t *self, int64_t intvalue);
	bool (*add_double)(jsonvisitor_t *self, double doublevalue);
	bool (*add_string)(jsonvisitor_t *self, jsonstring_t *svalue);
	bool (*add_null)(jsonvisitor_t *self);
	bool (*add_undefined)(jsonvisitor_t *self);
} jsonvisitor_vft;

/**** jsonobject_t base class ****/
struct jsonobject_vft;
typedef struct jsonobject_t {
	struct jsonobject_vft *vft;

} jsonobject_t;
typedef struct jsonobject_vft {
	/**
	 * Delete the instance
	 */
	void (*destroy)(jsonobject_t *self);

	/**
	 * Invokes visitor methods on the given visitor
	 */
	void (*visit)(jsonobject_t *self, jsonvisitor_t *visitor);

} jsonobject_vft;


#endif
