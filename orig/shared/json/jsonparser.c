#include "util/setup.h"
#include "json/jsonparser.h"

static size_t read_input_noop(struct jsonparser_t *self, char *buf, size_t max_size)
{
	return 0;
}

static size_t read_input_file(struct jsonparser_t *self, char *buf, size_t max_size)
{
	size_t act_read;
	act_read=fread(buf, 1, max_size, (FILE*)self->input);
	/*buf[act_read]=0;*/
	/*fprintf(stderr, "Read %d bytes: %s\n", act_read, buf);*/

	return act_read;
}

/**
 * When called, the self->input is just a FILE pointer
 */
void jsonparser_inputfile(struct jsonparser_t *self, FILE *input)
{
	/* Deallocate existing */
	jsonparser_inputdestroy(self);

	self->input=input;
	self->read_input=read_input_file;
	/* No destructor is needed */
}

struct stringinput_t {
	char* buffer;
	size_t buffer_length;
	size_t index;
	int is_copy;
};

static size_t read_input_string(struct jsonparser_t *self, char* buf, size_t max_size)
{
	struct stringinput_t *input=self->input;
	size_t remaining=input->buffer_length-input->index;
	if (remaining>0) {
		if (remaining>max_size) remaining=max_size;
		memcpy(buf, input->buffer + input->index, remaining);
		input->index+=remaining;
	}
	return remaining;
}

static void destroy_input_string(struct jsonparser_t *self)
{
	struct stringinput_t *input=self->input;

	if (input) {
		if (input->is_copy) {
			free(input->buffer);
		}
		if (input) free(self->input);
	}
}

void jsonparser_inputstring(struct jsonparser_t *self, char* input, size_t input_length, int copy)
{
	struct stringinput_t *string_input;

	jsonparser_inputdestroy(self);

	string_input=(struct stringinput_t*)malloc(sizeof(struct stringinput_t));
	string_input->buffer_length=input_length;
	string_input->index=0;

	if (copy) {
		string_input->is_copy=1;
		string_input->buffer=malloc(input_length);
		memcpy(string_input->buffer, input, input_length);
	} else {
		string_input->is_copy=0;
		string_input->buffer=input;
	}

	self->input=string_input;
	self->read_input=read_input_string;
	self->input_destructor=destroy_input_string;
}

void jsonparser_inputdestroy(struct jsonparser_t *self)
{
	if (self->input_destructor) {
		self->input_destructor(self);
		self->input_destructor=0;
	}
	self->input=0;
	self->read_input=read_input_noop;
}

/** Forward declare to avoid warning **/
extern void yyjsonparse(struct jsonparser_t *context, jsonvisitor_t *visitor);

void jsonparser_parse(struct jsonparser_t *self, jsonvisitor_t *visitor)
{
	yyjsonparse(self, visitor);
}

