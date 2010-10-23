/**
 * jsonserializer.h
 * Declares an object that is an implementation of jsonvisitor_t
 * and will serialize a json event stream as a normal json text
 * output.
 */
#ifndef __JSONSERIALIZER_H__
#define __JSONSERIALIZER_H__

#include <stdio.h>
#include <stdbool.h>
#include "json/jsonaccess.h"

struct jsonserializer_state_t {
	char type;
	bool has_member;
};

typedef struct jsonserializer_t {
	/**
	 * By making this the first element, this type can be used
	 * interchangeably as the jsonvisitor.
	 */
	jsonvisitor_t jsonvisitor;

	/**
	 * The stream to send output to
	 */
	FILE *output;

	struct {
		bool pretty_print;
		const char *indent;
	} outputformat;

	/* State stack.  Dynamically allocated array.  Grows as needed. */
	bool has_error;
	const char *error_msg;

	struct jsonserializer_state_t *state_stack;
	size_t state_stack_top;
	size_t state_stack_capacity;
} jsonserializer_t;

// Init/destroy instance
void jsonserializer_initialize(jsonserializer_t *self, FILE *output);
void jsonserializer_destroy(jsonserializer_t *self);


#endif
