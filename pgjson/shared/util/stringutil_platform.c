#include "pgjson_config.h"

#ifdef HAVE_FUNOPEN
/*** Provide buffer opening via BSD funopen function ***/
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

int stringwriter_append(void *self, const char *buffer, size_t n);
static ssize_t stringwriter_stream_write(void *cookie, const char *buffer, size_t n)
{
	if (stringwriter_append(cookie, buffer, n)) {
		return n;
	} else {
		errno=EIO;
		return -1;
	}
}

FILE *stringwriter_openfile(void /*stringwriter_t*/ *self)
{
	return fwopen(self, (void*)stringwriter_stream_write);
}

#elif HAVE_FOPENCOOKIE
/*** Provide buffer opening via GNU/Linux fopencookie function ***/
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

int stringwriter_append(void *self, const char *buffer, size_t n);
static ssize_t stringwriter_stream_write(void *cookie, const char *buffer, size_t n)
{
	if (stringwriter_append(cookie, buffer, n)) {
		return n;
	} else {
		errno=EIO;
		return -1;
	}
}

static cookie_io_functions_t stringwriter_functions = {
	.write = stringwriter_stream_write
};

FILE *stringwriter_openfile(void /*stringwriter_t*/ *self)
{
	return fopencookie(self, "w+", stringwriter_functions);
}
#endif
