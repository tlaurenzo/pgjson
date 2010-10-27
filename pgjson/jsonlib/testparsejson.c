#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dynbuffer.h"
#include "transcode_jsontext.h"

int main(int argc, char **argv)
{
	dynbuffer_t source=dynbuffer_init();
	dynbuffer_t dest=dynbuffer_init();

	if (argc>1) {
		stdin=fopen(argv[1], "r");
		if (!stdin) {
			printf("Error opening file\n");
			return 3;
		}
	}

	dynbuffer_read_file(&source, stdin);

	if (!json_transcode_to_json(source.contents, source.pos, &dest, "\t")) {
		printf("Error transcoding: %s\n", (char*)dest.contents);
		return 1;
	}

	fwrite(dest.contents, dest.pos, 1, stdout);

	fprintf(stdout, "\n");

	return 0;
}
