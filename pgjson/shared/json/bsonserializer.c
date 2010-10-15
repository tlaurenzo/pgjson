#include "util/setup.h"
#include <limits.h>

#include "json/bsonserializer.h"
#include "json/bsonconst.h"

/* Error msg constants */
static const char *MSG_ILLEGAL_SEQUENCE="Illegal visitor call sequence";
static const char *MSG_DATA_TOO_LARGE="Data size exceeds format limits";
static const char *MSG_OUT_OF_MEMORY="Out of memory or illegal allocation request";

/* Constant BSON values */
static const char BSON_EMPTYOBJECT[]={
		0x05, 0x00, 0x00, 0x00, 0x00
};

/* Forward defines */
static const jsonvisitor_vft bsonserializer_vft;
static const size_t INITIAL_STATE_STACK_CAPACITY=10;
static const size_t INITIAL_LABEL_LOOKASIDE_CAPACITY=256;

/** Class methods **/
void bsonserializer_init(bsonserializer_t *self, size_t buffer_capacity)
{
	self->jsonvisitor.vft=&bsonserializer_vft;
	self->has_error=false;
	self->error_msg=0;
	self->docstate_stack_capacity=INITIAL_STATE_STACK_CAPACITY;
	self->docstate_stack_index=0;
	self->docstate_stack=malloc(sizeof(bsondocstate_t)*INITIAL_STATE_STACK_CAPACITY);

	self->label_lookaside.bytes=malloc(INITIAL_LABEL_LOOKASIDE_CAPACITY);
	self->label_lookaside.length=0;
	self->label_lookaside_capacity=INITIAL_LABEL_LOOKASIDE_CAPACITY;

	/* Initial state */
	self->docstate_stack[0].offset=0;
	self->docstate_stack[0].member_count=0;
	self->docstate_stack[0].type='X';

	stringwriter_init(&self->buffer, buffer_capacity);
}

void bsonserializer_destroy(bsonserializer_t *self)
{
	if (self->docstate_stack) free(self->docstate_stack);
	stringwriter_destroy(&self->buffer);
	free(self->label_lookaside.bytes);
}

uint8_t *bsonserializer_getbuffer(bsonserializer_t *self, size_t *out_length)
{
	if (out_length) *out_length=self->buffer.pos;
	return self->buffer.string;
}

/* Quick type cast to ourself */
#define SELF ((bsonserializer_t*)self)

inline static bsondocstate_t *add_docstate(jsonvisitor_t *self)
{
	int newcapacity;

	SELF->docstate_stack_index+=1;
	if (SELF->docstate_stack_index>=SELF->docstate_stack_capacity) {
		/* Resize */
		newcapacity=SELF->docstate_stack_capacity*2;
		SELF->docstate_stack=realloc(SELF->docstate_stack, newcapacity * sizeof(bsondocstate_t));
		SELF->docstate_stack_capacity=newcapacity;
	}

	return SELF->docstate_stack+SELF->docstate_stack_index;
}

/**
 * This function is called by everything that writes a value to prepend
 * the element prefix.  Not inlining it for now as reduced code
 * size will probably be more cache friendly.
 */
static bool introduce_element(jsonvisitor_t *self, bsontype_t value_type)
{
	bsondocstate_t *curdoc=SELF->docstate_stack+SELF->docstate_stack_index;
	char curdoc_type=curdoc->type;
	char numeric_index[16];
	int numeric_index_len;

	/* Write the type byte */
	stringwriter_append_byte(&SELF->buffer, value_type);
	if (curdoc_type=='A') {
		/* Generate a numeric index */
		numeric_index_len=sprintf(numeric_index, "%d", curdoc->member_count);
		stringwriter_append(&SELF->buffer, numeric_index, numeric_index_len+1);
	} else if (curdoc_type=='O') {
		/* Dump the label lookaside as modified UTF-8 */
		stringwriter_append_modified_utf8z(&SELF->buffer,
				SELF->label_lookaside.bytes,
				SELF->label_lookaside.length);
	} else {
		/* Attempt to add value at root */
		SELF->has_error=true;
		SELF->error_msg=MSG_ILLEGAL_SEQUENCE;
		return false;
	}

	curdoc->member_count+=1;

	return true;
}

static bool end_document(jsonvisitor_t *self, char assertoftype)
{
	size_t doclength;
	uint32_t doclengthswapped;

	bsondocstate_t *state=SELF->docstate_stack+SELF->docstate_stack_index;
	char doctype=state->type;

	/* Only allow ending an object */
	if (doctype!=assertoftype) {
		SELF->has_error=true;
		SELF->error_msg=MSG_ILLEGAL_SEQUENCE;
		return false;
	}

	/* Write terminating null */
	stringwriter_append_byte(&SELF->buffer, 0);

	/* Go back and write the document size: current position - state.offset */
	doclength=SELF->buffer.pos - state->offset;

	/* Bounds check for 64bit platforms or large unsigned size_t */
	if (doclength>INT_MAX) {
		SELF->has_error=true;
		SELF->error_msg=MSG_DATA_TOO_LARGE;
		return false;
	}

	doclengthswapped=BSON_SWAP_HTOB32(doclength);
	stringwriter_write(&SELF->buffer, state->offset, &doclengthswapped, sizeof(doclengthswapped));

	/* pop the state */
	SELF->docstate_stack_index-=1;

	/* do a memory check on the stringwriter.  just doing it at key points
	 * keeps us from having to check after every call
	 */
	if (SELF->buffer.capacity==0) {
		SELF->has_error=true;
		SELF->error_msg=MSG_OUT_OF_MEMORY;
		return false;
	}

	return true;
}


/* VFT Functions */
static bool has_error(jsonvisitor_t *self, const char **out_msg)
{
	if (SELF->has_error) {
		if (out_msg) *out_msg=SELF->error_msg;
		return true;
	}
	return false;
}
static bool push_label(jsonvisitor_t *self, jsonstring_t *label)
{
	size_t new_capacity;

	/* Stash the label in the label lookaside for later access */
	if (SELF->label_lookaside_capacity<label->length) {
		/* Resize */
		new_capacity=label->length*2;
		SELF->label_lookaside.bytes=realloc(SELF->label_lookaside.bytes, new_capacity);
		SELF->label_lookaside_capacity=new_capacity;
	}
	memcpy(SELF->label_lookaside.bytes, label->bytes, label->length);
	SELF->label_lookaside.length=label->length;
	return true;
}
static bool start_object(jsonvisitor_t *self)
{
	bsondocstate_t *state=SELF->docstate_stack+SELF->docstate_stack_index;
	char doctype=state->type;

	if (doctype=='X') {
		/* This object becomes the root document (but only if one hasn't been set) */
		if (state->member_count==0) {
			state->member_count=1;
		} else {
			SELF->has_error=true;
			SELF->error_msg=MSG_ILLEGAL_SEQUENCE;
			return false;
		}
	} else {
		/* This object is part of another document */
		if (!introduce_element(self, BSONTYPE_DOCUMENT)) return false;
	}

	/* Push a new state and write the document header */
	state=add_docstate(self);
	state->type='O';
	state->member_count=0;

	/* Current position is start of doc. Advance by 4 bytes. */
	state->offset=stringwriter_skip(&SELF->buffer, 4);

	return true;
}
static bool end_object(jsonvisitor_t *self)
{
	return end_document(self, 'O');
}
static bool start_array(jsonvisitor_t *self)
{
	bsondocstate_t *state;

	if (!introduce_element(self, BSONTYPE_ARRAY)) return false;

	/* Push a new state */
	state=add_docstate(self);
	state->type='A';
	state->member_count=0;

	/* Current position is start of doc. Advance by 4 bytes. */
	state->offset=stringwriter_skip(&SELF->buffer, 4);

	return true;
}
static bool end_array(jsonvisitor_t *self)
{
	return end_document(self, 'A');
}
static bool add_empty_object(jsonvisitor_t *self)
{
	bsondocstate_t *state=SELF->docstate_stack+SELF->docstate_stack_index;
	char doctype=state->type;

	if (doctype=='X') {
		/* The entire document consists of this single empty object */
		if (state->member_count==0) {
			state->member_count=1;
		} else {
			/* Already object at root */
			SELF->has_error=true;
			SELF->error_msg=MSG_ILLEGAL_SEQUENCE;
			return false;
		}
	} else {
		/* Empty object part of a document */
		if (!introduce_element(self, BSONTYPE_DOCUMENT)) return false;
	}

	/* Write the empty object bytes */
	stringwriter_append(&SELF->buffer, BSON_EMPTYOBJECT, sizeof(BSON_EMPTYOBJECT));

	return true;
}
static bool add_empty_array(jsonvisitor_t *self)
{
	if (!introduce_element(self, BSONTYPE_ARRAY)) return false;

	stringwriter_append(&SELF->buffer, BSON_EMPTYOBJECT, sizeof(BSON_EMPTYOBJECT));

	return true;
}

static bool add_bool(jsonvisitor_t *self, bool bvalue)
{
	if (!introduce_element(self, BSONTYPE_BOOL)) return false;

	/* Write the value */
	stringwriter_append_byte(&SELF->buffer, bvalue ? 1: 0);

	return true;
}
static bool add_int32(jsonvisitor_t *self, int32_t intvalue)
{
	if (!introduce_element(self, BSONTYPE_INT32)) return false;

	/* Write the value */
	intvalue=BSON_SWAP_HTOB32(intvalue);
	stringwriter_append(&SELF->buffer, &intvalue, sizeof(intvalue));

	return true;
}
static bool add_int64(jsonvisitor_t *self, int64_t intvalue)
{
	if (!introduce_element(self, BSONTYPE_INT64)) return false;

	/* Write the value */
	intvalue=BSON_SWAP_HTOB64(intvalue);
	stringwriter_append(&SELF->buffer, &intvalue, sizeof(intvalue));

	return true;
}
static bool add_double(jsonvisitor_t *self, double doublevalue)
{
	/* Hack the double into a 64bit int for byte swapping */
	uint64_t double_binary=BSON_SWAP_HTOB64(((bsondouble_t*)(&doublevalue))->intvalue);

	if (!introduce_element(self, BSONTYPE_DOUBLE)) return false;

	/* Write the value */
	stringwriter_append(&SELF->buffer, &double_binary, sizeof(doublevalue));

	return true;
}
static bool add_string(jsonvisitor_t *self, jsonstring_t *svalue)
{
	size_t length=svalue->length;
	uint32_t lengthswapped;

	if (!introduce_element(self, BSONTYPE_STRING)) return false;

	/* Bounds check for 64bit platforms or large unsigned size_t */
	if (length>INT_MAX) {
		SELF->has_error=true;
		SELF->error_msg=MSG_DATA_TOO_LARGE;
		return false;
	}

	/* note that for some odd reason, the bson spec calls for the written length to include the terminating null */
	lengthswapped=BSON_SWAP_HTOB32((uint32_t)length+1);
	stringwriter_append(&SELF->buffer, &lengthswapped, sizeof(lengthswapped));
	stringwriter_append(&SELF->buffer, svalue->bytes, length);
	stringwriter_append_byte(&SELF->buffer, 0);

	return true;
}
static bool add_null(jsonvisitor_t *self)
{
	if (!introduce_element(self, BSONTYPE_NULL)) return false;

	return true;
}
static bool add_undefined(jsonvisitor_t *self)
{
	if (!introduce_element(self, BSONTYPE_UNDEFINED)) return false;

	return true;
}

static const jsonvisitor_vft bsonserializer_vft = {
	.has_error = has_error,
	.push_label = push_label,
	.start_object = start_object,
	.end_object = end_object,
	.start_array = start_array,
	.end_array = end_array,
	.add_empty_object = add_empty_object,
	.add_empty_array = add_empty_array,
	.add_bool = add_bool,
	.add_int32 = add_int32,
	.add_int64 = add_int64,
	.add_double = add_double,
	.add_string = add_string,
	.add_null = add_null,
	.add_undefined = add_undefined
};

