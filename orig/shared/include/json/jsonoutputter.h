/**
 * jsonoutputter.h
 * High level interface for generating json output in a variety
 * of formats.  The facilities here wrap lower level bits such
 * as the jsonserializer and bsonserializer with additional IO
 * support so that getting either format is just a matter of
 * specifying the format versus dealing with the divergent
 * interfaces of the two.
 */
#ifndef __JSONOUTPUTTER_H__
#define __JSONOUTPUTTER_H__

#include "util/stringutil.h"
#include "json/jsonserializer.h"
#include "json/bsonserializer.h"

typedef struct jsonoutputter_t {
	/**
	 * Include the different serializers as part of a union.
	 */
	union {
		jsonserializer_t json;
		bsonserializer_t bson;
	} serializer;
	int serializer_type;

	/**
	 * For the jsonserializer, it natively outputs to a FILE.  We allow writing to a
	 * buffer by opening a FILE against an output_buffer or by passing through to a
	 * caller supplied FILE.
	 *
	 * For bsonserializer, stream output is not possible because backreferences need
	 * to be set during processing.  As such, we always use its attached buffer and
	 * ignore the following for buffer access.  For stream access, we dump the serializer's
	 * internal buffer to the stream prior to close.
	 */
	FILE *output_stream;
	bool output_stream_is_owned;

	stringwriter_t *outputbuffer;
	bool outputbufferisowned;

	jsonvisitor_t *debug_visitor;
} jsonoutputter_t;

/* Use one of the following constructors */
bool jsonoutputter_open_json_buffer(jsonoutputter_t *self, stringwriter_t *outputbuffer);
bool jsonoutputter_open_bson_buffer(jsonoutputter_t *self, stringwriter_t *outputbuffer);

/**
 * If outputting json text, set whether pretty printing is desired
 * and optionally set the indent (if not null)
 */
void jsonoutputter_set_json_options(jsonoutputter_t *self, bool pretty_print, char *indent);

/* Get a reference to the visitor used to drive serialization */
jsonvisitor_t *jsonoutputter_getvisitor(jsonoutputter_t *self);

/* Get a reference to a visitor that logs debug messages as it goes */
jsonvisitor_t *jsonoutputter_getdebugvisitor(jsonoutputter_t *self);

/**
 * Gets the serializer's error info
 */
bool jsonoutputter_has_error(jsonoutputter_t *self, const char **error_msg);

/**
 * Close the outputter, flushing any pending IO and freeing resources.  If it was
 * opened in buffer mode, the buffer will be freed.  If in file mode, internal state
 * will be flushed but the disposition of the FILE handle is left to the caller.
 */
bool jsonoutputter_close(jsonoutputter_t *self);

#endif
