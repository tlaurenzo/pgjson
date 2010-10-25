#include "util/setup.h"

void PRINT_COUNTED_STRING(char *fmt, uint8_t *mem, size_t len)
{
	char *s;
	s=malloc(len+1);
	memcpy(s, mem, len);
	s[len]=0;
	printf(fmt, s);
	free(s);
}

#define JSONLEX_GETC() getc(stdin)
#define JSONLEX_UNGETC(c) ungetc(c, stdin)

#define JSONPARSE_ACTION_OBJECT_START() printf("OBJECT START: {\n")
#define JSONPARSE_ACTION_OBJECT_END()   printf("}: OBJECT END\n");
#define JSONPARSE_ACTION_OBJECT_LABEL(s, len) \
		PRINT_COUNTED_STRING("  FIELD '%s' = ", s, len)

#define JSONPARSE_ACTION_ARRAY_START()  printf("ARRAY START: [\n");
#define JSONPARSE_ACTION_ARRAY_END()    printf("]: ARRAY END\n");

#define JSONPARSE_ACTION_VALUE_NULL() \
		printf("null\n")
#define JSONPARSE_ACTION_VALUE_UNDEFINED() \
		printf("undefined\n")
#define JSONPARSE_ACTION_VALUE_BOOL(bl) \
		printf("%s\n", bl ? "true": "false")
#define JSONPARSE_ACTION_VALUE_INTEGER(s, len) \
		PRINT_COUNTED_STRING("Integer(%s)\n", s, len)
#define JSONPARSE_ACTION_VALUE_NUMERIC(s, len) \
		PRINT_COUNTED_STRING("Numeric(%s)\n", s, len)
#define JSONPARSE_ACTION_VALUE_STRING(s, len) \
		PRINT_COUNTED_STRING("'%s'\n", s, len)

#include "parser/jsonlex.inc.c"
#include "parser/jsonlex.tab.c"
#include "parser/jsonparse.inc.c"

int main(int argc, char **argv)
{
	jsonlex_token_t token;
	const char *tokenstr;
	jsonparseinfo_t parse;
	jsonlex_init(&parse.lexstate);
	char *bufferz;

	if (argc>1) {
		stdin=fopen(argv[1], "r");
	}

	if (!jsonparse(&parse)) {
		printf("PARSE FAILED\n");
		return 1;
	}

	/*
	for (;;) {
		token=jsonlex_next_token(&lexstate);
		tokenstr=jsonlex_token_str(token);
		bufferz=malloc(lexstate.buffer_pos+1);
		memcpy(bufferz, lexstate.buffer, lexstate.buffer_pos);
		bufferz[lexstate.buffer_pos]=0;
		printf("TOKEN %s (%s)\n", tokenstr, bufferz);
		free(bufferz);

		if (token==jsonlex_unknown) return 1;
		if (token==jsonlex_eof) break;
	}
	*/

	jsonlex_destroy(&parse.lexstate);
	return 0;
}
