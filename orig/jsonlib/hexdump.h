#ifndef __HEXDUMP_H__
#define __HEXDUMP_H__
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * Create a hex dump of the buffer and write it to out
 */
bool hexdump(FILE *out, uint8_t *buffer, uint32_t len);

#endif
