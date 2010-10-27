/**
 * transcode_jsontext.h
 * Transcodes from json text to json text, essentially
 * normalizing a json stream.
 */
#ifndef __TRANSCODE_JSONTEXT_H__
#define __TRANSCODE_JSONTEXT_H__
#include <stdint.h>
#include <stdbool.h>

#include "dynbuffer.h"

/**
 * Parse source and transcode it to JSON text in the dest buffer.
 * @return true on success, false on error
 */
bool json_transcode_to_json(uint8_t *source, size_t sourcelen, dynbuffer_t *dest, const char *indent);

#endif

