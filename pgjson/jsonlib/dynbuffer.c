#include "dynbuffer.h"

#define CAPACITY_MIN 256

void dynbuffer_realloc(dynbuffer_t *buffer, size_t needed_capacity)
{
	size_t allocheader=buffer->allocheader;
	/* figure a new capacity that gives us additional growth room
	 * without realloc
	 */
	size_t new_capacity=buffer->capacity*2;
	if (new_capacity<needed_capacity) new_capacity=needed_capacity*2;
	if (new_capacity<CAPACITY_MIN) new_capacity=CAPACITY_MIN;
	
	if (buffer->contents) {
		buffer->contents=realloc(buffer->contents-allocheader, new_capacity+allocheader)+allocheader;
	} else {
		buffer->contents=malloc(new_capacity+allocheader)+allocheader;
	}
	buffer->capacity=new_capacity;
}

bool dynbuffer_read_file(dynbuffer_t *buffer, FILE *input)
{
	size_t r;

	for (;;) {
	   r=buffer->capacity - buffer->pos;
	   if (r==0) {
		   r=1024;
		   dynbuffer_ensure_delta(buffer, r);
	   }


		r=fread(buffer->contents+buffer->pos, 1, r, input);
		if (r==0) {
			if (feof(input)) break;
			return false;
		}
		buffer->pos+=r;
	}
	return true;
}
