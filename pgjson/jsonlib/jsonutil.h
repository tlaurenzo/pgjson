/**
 * jsonutil.h
 * Common utilities for handling json
 */
#ifndef __JSONUTIL_H__
#define __JSONUTIL_H__
#include <stdint.h>
#include <stdbool.h>
#include "dynbuffer.h"

/**
 * Escape a source buffer into a JSON string.
 * If ascii_only is true, then everything but printable ascii will be escaped (maximum
 * portability).  Otherwise, non-printing ascii characters will be be escaped but valid
 * UTF-8 sequences will be output verbatim.
 * If quote char is a " or ', then the corresponding quote char will be escaped.
 * @return true on success, false on encoding error
 */
bool json_escape_string(dynbuffer_t *dest, const uint8_t *source, size_t len, bool ascii_only, uint8_t quote_char);

#endif
