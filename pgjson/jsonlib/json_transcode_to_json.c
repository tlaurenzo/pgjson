#include "jsonutil.h"

#define JSONPARSE_EXTRA_DECL \
	bool pretty; \
	const char *indent; \
	int level; \
	dynbuffer_t *dest; \
	char error_message[256];

#define DEST (parsestate->dest)

#ifdef JSONOUT_DISABLE_PRETTY
#define JSONOUT_PRETTY_PRINT 0
#else
#define JSONOUT_PRETTY_PRINT 1
#endif


/* actions */
#define JSONPARSE_ACTION_OBJECT_START() {\
	if (JSONOUT_PRETTY_PRINT&&parsestate->pretty) parsestate->level+=1; \
	dynbuffer_append_byte(DEST, '{'); \
	}
#define JSONPARSE_ACTION_OBJECT_LABEL(fieldindex, s, len) { \
	if (fieldindex) dynbuffer_append_byte(DEST, ','); \
	if (JSONOUT_PRETTY_PRINT&&parsestate->pretty) jsonout_indent(parsestate->dest, parsestate->level, parsestate->indent); \
	dynbuffer_append_byte(DEST, '\"'); \
	dynbuffer_append(DEST, s, len); \
	dynbuffer_append(DEST, "\":", 2); \
	}
#define JSONPARSE_ACTION_OBJECT_END() {\
	if (JSONOUT_PRETTY_PRINT&&parsestate->pretty) { \
		parsestate->level-=1; \
		jsonout_indent(parsestate->dest, parsestate->level, parsestate->indent); \
	} \
	dynbuffer_append_byte(DEST, '}'); \
	}


#define JSONPARSE_ACTION_ARRAY_START() {\
	dynbuffer_append_byte(DEST, '['); \
	if (JSONOUT_PRETTY_PRINT&&parsestate->pretty) parsestate->level+=1; \
	}
#define JSONPARSE_ACTION_ARRAY_ELEMENT(elementindex) { \
	if (elementindex) dynbuffer_append_byte(DEST, ','); \
	if (JSONOUT_PRETTY_PRINT&&parsestate->pretty) jsonout_indent(parsestate->dest, parsestate->level, parsestate->indent); \
	}
#define JSONPARSE_ACTION_ARRAY_END() { \
	if (JSONOUT_PRETTY_PRINT&&parsestate->pretty) { \
		parsestate->level-=1; \
		jsonout_indent(parsestate->dest, parsestate->level, parsestate->indent); \
	} \
	dynbuffer_append_byte(DEST, ']'); \
	}

#define JSONPARSE_ACTION_VALUE_NULL() \
	dynbuffer_append(DEST, "null", 4);

#define JSONPARSE_ACTION_VALUE_UNDEFINED() \
	dynbuffer_append(DEST, "undefined", 9);

#define JSONPARSE_ACTION_VALUE_BOOL(bl) {\
	if (bl) dynbuffer_append(DEST, "true", 4); \
	else dynbuffer_append(DEST, "false", 5); \
	}

#define JSONPARSE_ACTION_VALUE_INTEGER(s, len) \
	dynbuffer_append(DEST, s, len);

#define JSONPARSE_ACTION_VALUE_NUMERIC(s, len) \
	dynbuffer_append(DEST, s, len);

#define JSONPARSE_ACTION_VALUE_STRING(s, len) {\
	dynbuffer_append_byte(DEST, '\"'); \
	json_escape_string(DEST, s, len, true, '"'); \
	dynbuffer_append_byte(DEST, '\"'); \
	}

#define JSONPARSE_ACTION_ERROR(msg, got) \
	snprintf(parsestate->error_message, sizeof(parsestate->error_message), \
			"Error: %s (got %s)", msg, jsonlex_token_str(got));

static void jsonout_indent(dynbuffer_t *dest, int level, const char *indent)
{
	int i, len=strlen(indent);

	dynbuffer_append_byte(dest, '\n');
	for (i=0; i<level; i++) {
		dynbuffer_append(dest, indent, len);
	}
}

#include "jsonlex.inc.c"
#include "jsonparse.inc.c"

bool json_transcode_json_to_json(uint8_t *source, size_t sourcelen, dynbuffer_t *dest, const char *indent)
{
	bool result;
	jsonparseinfo_t parseinfo;
	
	/* init the lexer */
	jsonlex_init_io(&parseinfo.lexstate, source, sourcelen);
	parseinfo.dest=dest;
	parseinfo.error_message[0]=0;
	parseinfo.pretty=indent!=0;
	parseinfo.indent=indent;
	parseinfo.level=0;

	result=jsonparse(&parseinfo);

	if (!result) {
		dest->pos=0;
		dynbuffer_append(dest, parseinfo.error_message, strlen(parseinfo.error_message));
		dynbuffer_append_byte(dest, 0);
	}

	/* destroy */
	jsonlex_destroy(&parseinfo.lexstate);

	return result;
}

