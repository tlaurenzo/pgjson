/**
 * json.parser.h
 * Parser utilities to interface between bison and our datastructures
 */
#ifndef __JSON_PARSER_H__
#define __JSON_PARSER_H__
#include <stdlib.h>
#include <stdio.h>
#include "util/stringutil.h"
#include "json/jsonaccess.h"

typedef struct jsonparser_t {
	void *scanner;
	void *input;

	/**
	 * Scratch pad for assembling strings.  It stays persistent
	 * during the parse so as to avoid realloc overhead.
	 */
	stringwriter_t string_accum;

	/**
	 * One of ' or " depending on the quote style that opened the string
	 */
	char string_mode;

	bool has_error;
	char *error_msg;

	/**
	 * read_input method
	 */
	size_t (*read_input)(struct jsonparser_t *self, char *buf, size_t max_size);
	void (*input_destructor)(struct jsonparser_t *self);
} jsonparser_t;

/** The ctor and dtor are defined in the scanner epilog **/
void jsonparser_init(struct jsonparser_t *self);
void jsonparser_destroy(struct jsonparser_t *self);

/**
 * Perform the parse run
 */
void jsonparser_parse(struct jsonparser_t *self, jsonvisitor_t *visitor);

/**
 * Sets up a normal FILE handle as the input for the jsonparser.  It is up to the
 * caller to close the file on end of parse.
 */
void jsonparser_inputfile(struct jsonparser_t *self, FILE *input);

/**
 * Sets up a string as the input for the jsonparser using the given length and
 * optionally making a copy of the input.
 */
void jsonparser_inputstring(struct jsonparser_t *self, char *input, size_t input_length, int copy);

/**
 * Deallocates memory associated with a previous call to jsonparser_input*.  This does not actually
 * close the backing resource.  This will be invoked automatically when the entire parser
 * is destroyed.
 */
void jsonparser_inputdestroy(struct jsonparser_t *self);

#endif
