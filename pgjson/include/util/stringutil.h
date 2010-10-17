/**
 * stringutil.h
 * some common string utilities to get us through the day,
 * coded once again from scratch.
 */
#ifndef __STRINGUTIL_H__
#define __STRINGUTIL_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/**** String writing utilities ****/
typedef enum stringutil_escapemode_t {
	/**
	 * Escapes all non ASCII characters and control characters.
	 */
	STRINGESCAPE_ASCII=0,

	/**
	 * Escapes a string that is destined for a UTF-8 safe medium.
	 * This will escape control characters but will allow non-ascii
	 * characters through unescaped.
	 */
	STRINGESCAPE_UTF8=1
} stringutil_escapemode_t;


/**** stringwriter class ****/
typedef struct stringwriter_t {
	uint8_t* string;
	size_t pos;
	size_t capacity;
	bool alloc_error;
} stringwriter_t;

/**
 * Stringwriter initializer as a macro
 */
#define STRINGWRITER_INIT(initial_capacity) { .capacity = initial_capacity }

/**
 * Initialize a stringwriter with an initial size.
 */
void stringwriter_init(stringwriter_t *self, size_t initial_capacity);

/**
 * Detach the underlying buffer and reset.  Future calls to destroy
 * will not deallocate the buffer.
 */
void stringwriter_detach(stringwriter_t *self);

/**
 * Deallocate a stringwriter's memory
 */
void stringwriter_destroy(stringwriter_t *self);

/**
 * If the STRINGWRITER_HEADER_SIZE macro was set when the object was compiled,
 * then the stringwriter always allocates memory with the given header size
 * prefixed.  This is typically transparent to callers.  The buffer actually
 * allocated can be obtained with this call.  It may return null if the
 * buffer had an allocation error or was never allocated.
 */
void *stringwriter_get_alloc_buffer(stringwriter_t *self);

/**
 * Clears the string without releasing memory
 */
void stringwriter_clear(stringwriter_t *self);

/**
 * Append len bytes from the given source to the stringwriter.
 * The source is expected to be in a UTF-8 encoding.
 * return size written or 0 on failure
 */
bool stringwriter_append(stringwriter_t *self, const void* source, size_t bytes);

/**
 * Append a single byte.
 * Return 1 on success, 0 on failure
 */
bool stringwriter_append_byte(stringwriter_t *self, uint8_t byte);

/**
 * Writes a single escape char as defined by C-like languages
 * return !0 if success
 */
bool stringwriter_append_escape_char(stringwriter_t *self, char escape);

/**
 * Advances the stringwriter by the given amount, returning the offset
 * in the buffer prior to advancement.  Used to "make a hole" that
 * will be filled in later.
 */
size_t stringwriter_skip(stringwriter_t *self, size_t delta);

/**
 * Writes the contents of source into the buffer at the given position
 * without modifying the current position.  Used to "fill in a hole"
 * created with stringwriter_skip.  Correctly handles writing past
 * the end of the current allocated space.
 */
bool stringwriter_write(stringwriter_t *self, size_t pos, const void *source, size_t bytes);

/**
 * Appends a unicode codepoint to the string
 * return !0 if success
 */
bool stringwriter_append_codepoint(stringwriter_t *self, uint32_t codepoint);

/**
 * Appends the source as a "modified UTF-8" string where nulls are encoded
 * as 0xc0 0x80.  Appends a null to the end.
 * Returns 0 on failure
 */
bool stringwriter_append_modified_utf8z(stringwriter_t *self, uint8_t* source, size_t bytes);

/**
 * Writes an escaped JSON string.  This does
 * not physically write quotes.
 */
bool stringwriter_append_jsonescape(stringwriter_t *self, const uint8_t *utf8_string, size_t len,
		stringutil_escapemode_t escape_mode,
		bool escape_single_quote, bool escape_double_quote);

/**
 * Reads a zero terminated string that has been encoded with modified UTF-8,
 * converting the 0xc080 escape sequence to nulls, decoding at most maxbytes.
 * Returns 0 on failure or encountering maxbytes without finding a null.
 * Otherwise, returns the number of bytes read (including null)
 */
bool stringwriter_unpack_modified_utf8z(stringwriter_t *self, const char *source, size_t maxbytes);

/**
 * Fills this stringwriter by reading every available byte from input_file
 */
bool stringwriter_slurp_file(stringwriter_t *self, FILE *input_file);

/**
 * Opens a C stream that can be used to output to this writer.
 * The stream only supports sequential output.
 */
FILE *stringwriter_openfile(stringwriter_t *self);

/**
 * Writes an escaped JSON string to an output FILE.  This does
 * not physically write quotes.
 */
bool stringutil_escapejson(FILE* out, const uint8_t *utf8_string, size_t len,
		stringutil_escapemode_t escape_mode,
		bool escape_single_quote, bool escape_double_quote);

#endif
