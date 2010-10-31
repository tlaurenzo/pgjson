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

/**
 * Parse source and transcode it to JSON text in the dest buffer.
 * On error, the dest buffer is filled with a zero terminated error
 * message.
 * @return true on success, false on error
 */
bool json_transcode_json_to_json(uint8_t *source, size_t sourcelen, dynbuffer_t *dest, const char *indent);

/**
 * Transcode json text to binary in the dest buffer.
 * On error, the dest buffer is filled with a zero terminated error
 * message.
 * @return true on success, false on error
 */
bool json_transcode_json_to_binary(uint8_t *source, size_t sourcelen, dynbuffer_t *dest);

#endif
