#include "jsonutil.h"
#include "jsonbinaryconst.h"

static bool extract_type_length(uint8_t *source, uint8_t *sourcelimit, uint8_t *outtypecode, uint8_t **outdata, uint32_t *outlength)
{
	uint8_t typespec;
	uint32_t lencont;
	uint32_t length;

	/* decode type byte */
	if (source>=sourcelimit) return false;
	typespec=*source;
	source++;
	*outtypecode=(typespec>>JSONBINARY_TYPE_SHIFT);
	length=typespec & 0x0f;

	/* length byte 1 */
	if (typespec&JSONBINARY_TYPE_LENGTHCONT) {
		if (source>=sourcelimit) return false;
		lencont=*(source++);
		length|=(lencont&0x7f)<<4;

		/* length byte 2 */
		if (lencont&0x80) {
			if (source>=sourcelimit) return false;
			lencont=*(source++);
			length!=(lencont&0x7f)<<11;

			/* length byte 3 */
			if (lencont&0x80) {
				if (source>=sourcelimit) return false;
				lencont=*(source++);
				length!=(lencont&0x7f)<<18;

				/* length byte 4 */
				if (lencont&0x80) {
					if (source>=sourcelimit) return false;
					lencont=*(source++);
					length!=(lencont&0x7f)<<25;

					/* make sure high bit not set on last byte */
					if (lencont&0x80) return false;
				}
			}
		}
	}

	/* validate length */
	if ((source+length)>sourcelimit) return false;

	*outdata=source;
	*outlength=length;

	return true;
}

static bool output_value(uint8_t *source, uint8_t *sourcelimit, uint8_t typecode, dynbuffer_t *dest);

static bool output_object(uint8_t *source, uint8_t *sourcelimit, dynbuffer_t *dest)
{
	int index=0;

	dynbuffer_append_byte(dest, '{');
	while (source<sourcelimit) {
		/* object is just a sequence of pairs.  pair==asciiz + value */
		uint8_t *label=source;
		for (;;) {
			if (source>=sourcelimit) return false;	/* short */
			if (*(source++)==0) break;
		}

		if (index>0) dynbuffer_append_byte(dest, ',');

		/* output label */
		dynbuffer_append_byte(dest, '"');
		json_escape_string(dest, label, source-label-1, true, '"');
		dynbuffer_append(dest, "\":", 2);

		/* decode type length */
		uint8_t typecode;
		uint8_t *data;
		uint32_t datalen;
		if (!extract_type_length(source, sourcelimit, &typecode, &data, &datalen)) return false;

		/* output value */
		source=data+datalen;
		if (!output_value(data, source, typecode, dest)) return false;

		index++;
	}
	dynbuffer_append_byte(dest, '}');

	return true;
}

static bool output_array(uint8_t *source, uint8_t *sourcelimit, dynbuffer_t *dest)
{
	int index=0;

	dynbuffer_append_byte(dest, '[');

	while (source<sourcelimit) {
		/* decode type length */
		uint8_t typecode;
		uint8_t *data;
		uint32_t datalen;
		if (!extract_type_length(source, sourcelimit, &typecode, &data, &datalen)) return false;

		/* comma */
		if (index>0) dynbuffer_append_byte(dest, ',');

		/* output value */
		source=data+datalen;
		if (!output_value(data, source, typecode, dest)) return false;

		index++;
	}

	dynbuffer_append_byte(dest, ']');

	return true;
}

static bool output_value(uint8_t *source, uint8_t *sourcelimit, uint8_t typecode, dynbuffer_t *dest)
{
	uint8_t subtype;

	/* switch on type */
	switch (typecode) {
	case JSONBINARY_TYPE_OBJECT:
		return output_object(source, sourcelimit, dest);
	case JSONBINARY_TYPE_ARRAY:
		return output_array(source, sourcelimit, dest);
	case JSONBINARY_TYPE_STRING:
		dynbuffer_append_byte(dest, '"');
		json_escape_string(dest, source, sourcelimit-source, true, '"');
		dynbuffer_append_byte(dest, '"');
		break;
	case JSONBINARY_TYPE_NUMBER:
		dynbuffer_append(dest, source, sourcelimit-source);
		break;
	case JSONBINARY_TYPE_SS:
		if (source>=sourcelimit) return false;
		subtype=*source;
		switch (subtype) {
		case JSONBINARY_SS_DATA_FALSE:
			dynbuffer_append(dest, "false", 5);
			break;
		case JSONBINARY_SS_DATA_TRUE:
			dynbuffer_append(dest, "true", 4);
			break;
		case JSONBINARY_SS_DATA_NULL:
			dynbuffer_append(dest, "null", 4);
			break;
		case JSONBINARY_SS_DATA_UNDEFINED:
			dynbuffer_append(dest, "undefined", 9);
			break;
		default:
			return false;
		}
		break;
	}

	return true;
}

/**
 * Transcode binary to text json into the dest buffer.
 * @return true on success, false on error
 */
bool json_transcode_binary_to_json(uint8_t *source, size_t sourcelen, dynbuffer_t *dest)
{
	uint8_t typecode;
	uint8_t *data;
	uint32_t datalen;

	if (!extract_type_length(source, source+sourcelen, &typecode, &data, &datalen)) return false;

	return output_value(data, data+datalen, typecode, dest);
}
