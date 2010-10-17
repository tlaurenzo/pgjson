#include "util/setup.h"
#include "json/jsonoutputter.h"

#define SERTYPE_JSON 1
#define SERTYPE_BSON 2

/* Forward defines */
static const jsonvisitor_vft jsondebugvisitor_vft;
typedef struct {
	const jsonvisitor_vft *vft;
	jsonvisitor_t *parent;
	FILE *out;
} jsondebugvisitor_t;

bool jsonoutputter_open_json_buffer(jsonoutputter_t *self, stringwriter_t *outputbuffer)
{
	self->debug_visitor=0;
	self->serializer_type=SERTYPE_JSON;

	/* Setup the buffer */
	self->outputbuffer=outputbuffer;

	/* Get the FILE* handle to the buffer */
	self->output_stream_is_owned=true;
	self->output_stream=stringwriter_openfile(self->outputbuffer);

	jsonserializer_initialize(&self->serializer.json, self->output_stream);
	return true;
}

bool jsonoutputter_open_bson_buffer(jsonoutputter_t *self, stringwriter_t *outputbuffer)
{
	self->debug_visitor=0;
	self->serializer_type=SERTYPE_BSON;

	self->output_stream=0;
	self->output_stream_is_owned=false;
	self->outputbuffer=outputbuffer;

	bsonserializer_init(&self->serializer.bson, outputbuffer);
	return true;
}

void jsonoutputter_set_json_options(jsonoutputter_t *self, bool pretty_print, char *indent)
{
	if (self->serializer_type==SERTYPE_JSON) {
		self->serializer.json.outputformat.pretty_print=true;
		if (indent) {
			self->serializer.json.outputformat.indent=indent;
		}
	}
}

/* Get a reference to the visitor used to drive serialization */
jsonvisitor_t *jsonoutputter_getvisitor(jsonoutputter_t *self)
{
	switch (self->serializer_type) {
	case SERTYPE_JSON: return &self->serializer.json.jsonvisitor;
	case SERTYPE_BSON: return &self->serializer.bson.jsonvisitor;
	default:
		return 0;
	}
}

jsonvisitor_t *jsonoutputter_getdebugvisitor(jsonoutputter_t *self)
{
	jsondebugvisitor_t *v;

	if (!self->debug_visitor) {
		v=malloc(sizeof(jsondebugvisitor_t));
		v->vft=&jsondebugvisitor_vft;
		v->out=stderr;
		v->parent=jsonoutputter_getvisitor(self);
		self->debug_visitor=(jsonvisitor_t*)v;
	}
	return self->debug_visitor;
}

bool jsonoutputter_has_error(jsonoutputter_t *self, const char **error_msg)
{
	switch (self->serializer_type) {
	case SERTYPE_JSON:
		return self->serializer.json.jsonvisitor.vft->has_error(&self->serializer.json.jsonvisitor, error_msg);
	case SERTYPE_BSON:
		return self->serializer.bson.jsonvisitor.vft->has_error(&self->serializer.bson.jsonvisitor, error_msg);
	}

	return false;
}


/**
 * Close the outputter, flushing any pending IO and freeing resources.  If it was
 * opened in buffer mode, the buffer will be freed.  If in file mode, internal state
 * will be flushed but the disposition of the FILE handle is left to the caller.
 */
bool jsonoutputter_close(jsonoutputter_t *self)
{
	bool rc=true;
	size_t buffer_length;

	switch (self->serializer_type) {
	case SERTYPE_JSON:
		jsonserializer_destroy(&self->serializer.json);
		break;
	case SERTYPE_BSON:
		bsonserializer_destroy(&self->serializer.bson);
		break;
	}

	if (self->output_stream_is_owned) {
		fclose(self->output_stream);
	}
	if (self->debug_visitor) {
		free(self->debug_visitor);
	}
}


/* debug visitor */
/* VFT Functions */
#define SELF ((jsondebugvisitor_t*)self)
static bool has_error(jsonvisitor_t *self, const char **out_msg)
{
	return SELF->parent->vft->has_error(SELF->parent, out_msg);
}
static bool push_label(jsonvisitor_t *self, jsonstring_t *label)
{
	char *s=malloc(label->length+1);
	memcpy(s, label->bytes, label->length);
	s[label->length]=0;

	fprintf(SELF->out, "JSON(push_label): '%s' Length=%zu\n", s, label->length);
	free(s);

	return SELF->parent->vft->push_label(SELF->parent, label);
}
static bool start_object(jsonvisitor_t *self)
{
	fprintf(SELF->out, "JSON(start_object)\n");
	return SELF->parent->vft->start_object(SELF->parent);
}
static bool end_object(jsonvisitor_t *self)
{
	fprintf(SELF->out, "JSON(end_object)\n");
	return SELF->parent->vft->end_object(SELF->parent);
}
static bool start_array(jsonvisitor_t *self)
{
	fprintf(SELF->out, "JSON(start_array)\n");
	return SELF->parent->vft->start_array(SELF->parent);
}
static bool end_array(jsonvisitor_t *self)
{
	fprintf(SELF->out, "JSON(end_array)\n");
	return SELF->parent->vft->end_array(SELF->parent);
}
static bool add_empty_object(jsonvisitor_t *self)
{
	fprintf(SELF->out, "JSON(add_empty_object)\n");
	return SELF->parent->vft->add_empty_object(SELF->parent);
}
static bool add_empty_array(jsonvisitor_t *self)
{
	fprintf(SELF->out, "JSON(add_empty_array)\n");
	return SELF->parent->vft->add_empty_array(SELF->parent);
}
static bool add_bool(jsonvisitor_t *self, bool bvalue)
{
	fprintf(SELF->out, "JSON(add_bool): %d\n", (int)bvalue);
	return SELF->parent->vft->add_bool(SELF->parent, bvalue);

}
static bool add_int32(jsonvisitor_t *self, int32_t intvalue)
{
	fprintf(SELF->out, "JSON(add_int32): %d\n", intvalue);
	return SELF->parent->vft->add_int32(SELF->parent, intvalue);
}
static bool add_int64(jsonvisitor_t *self, int64_t intvalue)
{
	fprintf(SELF->out, "JSON(add_int64): %lld\n", intvalue);
	return SELF->parent->vft->add_int32(SELF->parent, intvalue);
}
static bool add_double(jsonvisitor_t *self, double doublevalue)
{
	fprintf(SELF->out, "JSON(add_double): %f\n", doublevalue);
	return SELF->parent->vft->add_int32(SELF->parent, doublevalue);
}
static bool add_string(jsonvisitor_t *self, jsonstring_t *svalue)
{
	char *s=malloc(svalue->length+1);
	memcpy(s, svalue->bytes, svalue->length);
	s[svalue->length]=0;

	fprintf(SELF->out, "JSON(add_string): '%s'\n", s);
	free(s);
	return SELF->parent->vft->add_string(SELF->parent, svalue);
}
static bool add_null(jsonvisitor_t *self)
{
	fprintf(SELF->out, "JSON(add_null)\n");
	return SELF->parent->vft->add_null(SELF->parent);
}
static bool add_undefined(jsonvisitor_t *self)
{
	fprintf(SELF->out, "JSON(add_undefined)\n");
	return SELF->parent->vft->add_undefined(SELF->parent);
}

static const jsonvisitor_vft jsondebugvisitor_vft = {
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


