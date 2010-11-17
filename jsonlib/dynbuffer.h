/**
 * buffer.h
 * buffer operations
 */
#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
	uint8_t *contents;
	size_t   pos;
	size_t   capacity;
	size_t   allocheader;
} dynbuffer_t;

extern void dynbuffer_realloc(dynbuffer_t *buffer, size_t needed_capacity);

/**
 * Initialize a dynbuffer that starts with zero capacity
 */
#define dynbuffer_init() { }
#define dynbuffer_init_allocheader(header_size) { .allocheader = header_size }
#define dynbuffer_destroy(dynbuffer) \
	if ((dynbuffer)->contents) free((dynbuffer)->contents - (dynbuffer)->allocheader);
	
#define dynbuffer_clear(dynbuffer) (dynbuffer)->pos=0;

#define dynbuffer_ensure(dynbuffer, needed_capacity) {\
	if ((dynbuffer)->capacity<needed_capacity) dynbuffer_realloc(dynbuffer, needed_capacity); }
	
#define dynbuffer_ensure_delta(dynbuffer, more_capacity) \
	dynbuffer_ensure(dynbuffer, (dynbuffer)->pos+more_capacity);
	
#define dynbuffer_allocbuffer(dynbuffer) \
	((dynbuffer)->contents-(dynbuffer)->allocheader)

#define dynbuffer_append(dynbuffer, source, bytes) \
	do { \
	  dynbuffer_ensure_delta(dynbuffer, bytes); \
	  memcpy((dynbuffer)->contents+(dynbuffer)->pos, source, bytes); \
	  (dynbuffer)->pos+=bytes;\
	} while(0)

#define dynbuffer_append_byte(dynbuffer, byte) \
	do { dynbuffer_ensure_delta(dynbuffer, 1); \
	  (dynbuffer)->contents[(dynbuffer)->pos++]=byte; \
	} while(0)

#define dynbuffer_append_byte_nocheck(dynbuffer, byte) \
	do { \
	(dynbuffer)->contents[(dynbuffer)->pos++]=byte; \
	} while (0)

/*** Utility functions ***/
bool dynbuffer_read_file(dynbuffer_t *buffer, FILE *input);

#endif

