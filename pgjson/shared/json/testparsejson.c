#include "util/setup.h"

#define JSONLEX_GETC() getc(stdin)
#define JSONLEX_UNGETC(c) ungetc(c, stdin)

#include "parser/jsonlex.inc.c"
#include "parser/jsonlex.tab.c"

int main(int argc, char **argv)
{
	jsonlex_token_t token;
	const char *tokenstr;
	jsonlex_state_t lexstate;
	jsonlex_init(&lexstate);
	char *bufferz;

	if (argc>1) {
		stdin=fopen(argv[1], "r");
	}

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

	jsonlex_destroy(&lexstate);
	return 0;
}
