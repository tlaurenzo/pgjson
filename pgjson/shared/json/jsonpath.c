#include "util/setup.h"
#include "json/jsonpath.h"

void jsonpath_append(stringwriter_t *buffer, jsonpathtype_t type, uint8_t *utf8string, size_t stringlen)
{
	/* remove terminating null if present */
	if (buffer->pos && buffer->string[buffer->pos]==0) buffer->pos--;

	/* add type */
	stringwriter_append_byte(buffer, (uint8_t)type);

	/* write modified utf8 */
	stringwriter_append_modified_utf8z(buffer, utf8string, stringlen);

	/* add sequence terminating null */
	stringwriter_append_byte(buffer, 0);
}

void jsonpath_iter_begin(jsonpathiter_t *iter, const uint8_t *buffer, size_t bufferlen)
{
	/* setup the buffer */
	iter->buffer=buffer;
	iter->bufferlimit=buffer+bufferlen;

	if (bufferlen>3) iter->next_value=iter->buffer;
	else {
		/* invalid */
		iter->next_value=0;
	}

	iter->current_type=0;
	iter->current_value=0;
}

bool jsonpath_iter_next(jsonpathiter_t *iter)
{
	jsonpathtype_t type;
	const uint8_t *bufferlimit=iter->bufferlimit;
	const uint8_t *this_value=iter->next_value;
	const uint8_t *value_start;

	/* did we clear next_value on previous run or short buffer - return false */
	if ((this_value>=bufferlimit) || (bufferlimit - this_value) < 3 || !*(this_value)) return false;

	/* this_value points to the start of the next block */

	/* get the type byte */
	type=(jsonpathtype_t)*this_value;
	if (type<0 || type>JSONPATHTYPE_MAXVALUE)
		return false;	/* if adding more types, extend check */
	this_value++;

	/* fast-forward through the value */
	value_start=this_value;
	while (this_value<bufferlimit && *this_value) this_value++;
	if (*this_value) return false;	/* short buffer */
	this_value++;

	/* update iterator */
	iter->current_type=type;
	iter->current_value=value_start;
	iter->next_value=this_value;
	iter->current_value_length=this_value-value_start-1;

	return true;
}

/*** parser bits ***/
/* pull in declarations from lexer and parser */
void yyjsonpathlex_init(void **scanner);
void yyjsonpathlex_destroy(void *scanner);
void yyjsonpathset_extra(jsonpath_parsestate_t *state, void *scanner);
void yyjsonpathparse(jsonpath_parsestate_t *state);

/**
 * referenced from the lexer
 */
int yyjsonpath_read(jsonpath_parsestate_t *state, char *buf, size_t max_size)
{
	size_t remain=state->streamlimit-state->stream;
	if (state->stream>=state->streamlimit) return 0;

	if (remain>max_size) remain=max_size;

	memcpy(buf, state->stream, remain);
	state->stream+=remain;
	return remain;
}

void yyjsonpatherror(jsonpath_parsestate_t *state, char *err)
{
	state->has_error=true;
	if (state->error_msg) free(state->error_msg);
	state->error_msg=malloc(strlen(err)+1);
	strcpy(state->error_msg, err);
}

bool jsonpath_parse(stringwriter_t *bufferout, uint8_t *utf8stream, size_t streamlen)
{
	/* setup */
	jsonpath_parsestate_t state;

	state.bufferout=bufferout;
	state.stream=utf8stream;
	state.streamlimit=utf8stream+streamlen;
	state.error_msg=0;
	state.has_error=false;

	yyjsonpathlex_init(&state.scanner);
	yyjsonpathset_extra(&state, state.scanner);
	stringwriter_init(&state.string_accum, 256);

	/* parse */
	yyjsonpathparse(&state);

	/* cleanup */
	yyjsonpathlex_destroy(state.scanner);
	stringwriter_destroy(&state.string_accum);

	/* handle error */
	if (state.has_error) {
		bufferout->pos=0;
		if (state.error_msg) stringwriter_append(bufferout, state.error_msg, strlen(state.error_msg));
		stringwriter_append_byte(bufferout, 0);
		return false;
	}
	return true;
}

/*** serializer ***/
bool jsonpath_serialize(stringwriter_t *textout, jsonpathiter_t *iter)
{
	bool first=true;

	while (jsonpath_iter_next(iter)) {
		switch (iter->current_type) {
		case JSONPATHTYPE_IDENTIFIER:
			if (first) first=false;
			else stringwriter_append_byte(textout, '.');
			if (!stringwriter_unpack_modified_utf8z(textout, iter->current_value, iter->current_value_length+1)) return false;
			break;
		case JSONPATHTYPE_NUMERIC:
			stringwriter_append_byte(textout, '[');
			if (!stringwriter_unpack_modified_utf8z(textout, iter->current_value, iter->current_value_length+1)) return false;
			stringwriter_append_byte(textout, ']');
			break;
		case JSONPATHTYPE_STRING:
			stringwriter_append(textout, "['", 2);
			if (!stringwriter_append_jsonescape(textout, iter->current_value, iter->current_value_length, STRINGESCAPE_ASCII, true, false)) return false;
			stringwriter_append(textout, "']", 2);
			break;
		default:
			return false;
		}
	}

	return true;
}

