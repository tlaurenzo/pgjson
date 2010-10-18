#include "util/setup.h"
#include "util/stringutil.h"
#include "json/jsonserializer.h"

/* Constants */
static const char *MSG_ILLEGAL_SEQUENCE="Illegal visitor call sequence";
static const char *MSG_OOM="Out of memory";
static const char *MSG_OUTPUT_ERROR="Output error";
static const char *MSG_ENCODING_ERROR="Encoding error";

/* Forward defines */
static const jsonvisitor_vft jsonserializer_vft;
static const size_t INITIAL_STATE_STACK_CAPACITY=10;

/* Quick type cast to ourself */
#define SELF ((jsonserializer_t*)self)
#define DECLARATIONS \
	struct jsonserializer_state_t *curstate;\
	if (SELF->has_error) return false;\
	curstate=SELF->state_stack+SELF->state_stack_top;

#define CHECKED_fwrite(ptr, size, nitems) \
	if (fwrite(ptr,size,nitems,SELF->output)!=nitems) {\
		SELF->has_error=true;\
		SELF->error_msg=MSG_OUTPUT_ERROR;\
		return false;\
	}
#define CHECKED_fprintf(format,...)\
	if (fprintf(SELF->output,format,##__VA_ARGS__)==-1) {\
		SELF->has_error=true;\
		SELF->error_msg=MSG_OUTPUT_ERROR;\
		return false;\
	}

#define INDENT\
	if (SELF->outputformat.pretty_print && curstate->type!='O') {\
		indent_value(SELF);\
	}

#define OUTDENT\
	if (SELF->outputformat.pretty_print) {\
		outdent_block(SELF);\
	}

#define INTRODUCE_VALUE\
	if (curstate->type!='O') {\
		if (curstate->has_member) CHECKED_fprintf(",");\
		INDENT;\
	}\
	curstate->has_member=true;

void jsonserializer_initialize(jsonserializer_t *self, FILE *output)
{
	/* Initialize VFT */
	self->jsonvisitor.vft=&jsonserializer_vft;

	/* Defaults */
	self->outputformat.pretty_print=false;
	self->outputformat.indent="  ";
	self->state_stack=malloc(sizeof(struct jsonserializer_state_t)*INITIAL_STATE_STACK_CAPACITY);
	self->state_stack_capacity=INITIAL_STATE_STACK_CAPACITY;
	self->state_stack_top=0;
	if (!self->state_stack) {
		/* OOM */
		self->has_error=true;
		self->error_msg=MSG_OOM;
	}

	/* Initial state */
	self->has_error=false;
	self->state_stack[0].type='X';
	self->state_stack[0].has_member=false;

	/* Initialize from arguments */
	self->output=output;
}

void jsonserializer_destroy(jsonserializer_t *self)
{
	if (self->state_stack) {
		free(self->state_stack);
		self->state_stack=0;
	}
}

inline static struct jsonserializer_state_t* push_state_stack(jsonserializer_t *self, char type)
{
	struct jsonserializer_state_t *newstate;

	size_t newtop=++self->state_stack_top;
	if (newtop>=self->state_stack_capacity) {
		self->state_stack=realloc(self->state_stack,
				sizeof(struct jsonserializer_state_t) * self->state_stack_capacity*2);
		if (!self->state_stack) {
			/* OOM */
			self->has_error=true;
			self->error_msg=MSG_OOM;
			return 0;
		}
	}

	newstate=self->state_stack + newtop;
	newstate->type=type;
	newstate->has_member=false;

	return newstate;
}

inline static void pop_state(jsonserializer_t *self, char type)
{
	if (self->state_stack_top==0 || self->state_stack[self->state_stack_top].type!=type) {
		/* error */
		self->has_error=true;
		self->error_msg=MSG_ILLEGAL_SEQUENCE;
		return;
	}

	self->state_stack_top-=1;
}

static bool indent_value(jsonserializer_t *self)
{
	size_t level=self->state_stack_top;
	size_t i;
	if (level==0) return true;

	CHECKED_fprintf("\n");
	for (i=0; i<level; i++) {
		CHECKED_fprintf("%s", self->outputformat.indent);
	}
	return true;
}

static bool outdent_block(jsonserializer_t *self)
{
	int level=self->state_stack_top;
	int i;

	CHECKED_fprintf("\n");

	for (i=0; i<level-1; i++) {
		CHECKED_fprintf("%s", self->outputformat.indent);
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
	DECLARATIONS;

	/* Can only output a label if in the "O" state */
	if (curstate->type!='O') {
		SELF->has_error=true;
		SELF->error_msg=MSG_ILLEGAL_SEQUENCE;
		return false;
	}

	/* Introduce a new value */
	if (curstate->has_member) CHECKED_fprintf(",");
	if (SELF->outputformat.pretty_print) {
		indent_value(SELF);
	}

	CHECKED_fprintf("\"");
	if (!stringutil_escapejson(SELF->output,
			(const uint8_t*)label->bytes,
			label->length,
			STRINGESCAPE_ASCII,
			false, true))
	{
		/* Error */
		SELF->has_error=true;
		SELF->error_msg=MSG_ENCODING_ERROR;
		return false;
	}
	CHECKED_fprintf("\":");
	return true;
}
static bool start_object(jsonvisitor_t *self)
{
	DECLARATIONS;
	INTRODUCE_VALUE;

	/* introduce new state */
	push_state_stack(SELF, 'O');

	CHECKED_fprintf("{");
	return true;
}
static bool end_object(jsonvisitor_t *self)
{
	DECLARATIONS;

	OUTDENT;
	CHECKED_fprintf("}");

	pop_state(SELF, 'O');
	return true;
}
static bool start_array(jsonvisitor_t *self)
{
	DECLARATIONS;
	INTRODUCE_VALUE;

	/* introduce new state */
	push_state_stack(SELF, 'A');

	CHECKED_fprintf("[");
	return true;
}
static bool end_array(jsonvisitor_t *self)
{
	DECLARATIONS;

	OUTDENT;
	CHECKED_fprintf("]");

	pop_state(SELF, 'A');
	return true;
}
static bool add_empty_object(jsonvisitor_t *self)
{
	DECLARATIONS;
	INTRODUCE_VALUE;

	CHECKED_fprintf("{}");
	return true;
}
static bool add_empty_array(jsonvisitor_t *self)
{
	DECLARATIONS;
	INTRODUCE_VALUE;

	CHECKED_fprintf("[]");
	return true;
}
static bool add_bool(jsonvisitor_t *self, bool bvalue)
{
	DECLARATIONS;
	INTRODUCE_VALUE;

	CHECKED_fprintf(bvalue ? "true" : "false");
	return true;
}
static bool add_int32(jsonvisitor_t *self, int32_t intvalue)
{
	DECLARATIONS;
	INTRODUCE_VALUE;

	CHECKED_fprintf("%d", intvalue);
	return true;
}
static bool add_int64(jsonvisitor_t *self, int64_t intvalue)
{
	DECLARATIONS;
	INTRODUCE_VALUE;

	CHECKED_fprintf(PRId64, intvalue);
	return true;
}
static bool add_double(jsonvisitor_t *self, double doublevalue)
{
	DECLARATIONS;
	INTRODUCE_VALUE;

	CHECKED_fprintf("%f", doublevalue);
	return true;
}
static bool add_string(jsonvisitor_t *self, jsonstring_t *svalue)
{
	DECLARATIONS;
	INTRODUCE_VALUE;

	CHECKED_fprintf("\"");
	if (!stringutil_escapejson(SELF->output,
			(const uint8_t*)svalue->bytes,
			svalue->length,
			STRINGESCAPE_ASCII,
			false, true))
	{
		/* Error */
		SELF->has_error=true;
		SELF->error_msg=MSG_ENCODING_ERROR;
		return false;
	}
	CHECKED_fprintf("\"");

	return true;
}
static bool add_null(jsonvisitor_t *self)
{
	DECLARATIONS;
	INTRODUCE_VALUE;

	CHECKED_fprintf("null");
	return true;
}
static bool add_undefined(jsonvisitor_t *self)
{
	DECLARATIONS;
	INTRODUCE_VALUE;

	CHECKED_fprintf("undefined");
	return true;
}

static const jsonvisitor_vft jsonserializer_vft = {
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

