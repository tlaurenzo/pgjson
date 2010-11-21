#include <postgres.h>
#include <fmgr.h>

#include "jsonlib/dynbuffer.h"
#include "jsonlib/jsonutil.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

#define PG_RETURN_DYNBUFFER(dynbuffer) \
	{ \
		dynbuffer_ensure(&dynbuffer, 0); \
		SET_VARSIZE(dynbuffer_allocbuffer(&dynbuffer), dynbuffer.pos + VARHDRSZ); \
		PG_RETURN_POINTER(dynbuffer_allocbuffer(&dynbuffer)); \
	}

/*** json io ***/
PG_FUNCTION_INFO_V1(pgjson_json_in);
Datum
pgjson_json_in(PG_FUNCTION_ARGS)
{
	char *input_text=PG_GETARG_CSTRING(0);
	size_t input_length=strlen(input_text);
	dynbuffer_t buffer=dynbuffer_init_allocheader(VARHDRSZ);
	bool success;

	success=json_transcode_json_to_binary((uint8_t*)input_text, input_length, &buffer);
	if (!success) {
		ereport(ERROR, (
				errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("JSON parse error: %s", (char*)buffer.contents)
				));
		return 0;
	}

	PG_RETURN_DYNBUFFER(buffer);
}

PG_FUNCTION_INFO_V1(pgjson_json_out);
Datum
pgjson_json_out(PG_FUNCTION_ARGS)
{
	void *input_data;
	size_t input_length;
	dynbuffer_t buffer=dynbuffer_init_allocheader(0);
	bool success;

	input_data=PG_DETOAST_DATUM_PACKED(PG_GETARG_DATUM(0));
	input_length=VARSIZE_ANY_EXHDR(input_data);

	success=json_transcode_binary_to_json((uint8_t*)VARDATA_ANY(input_data), input_length, &buffer);
	if (!success) {
		ereport(ERROR, (
				errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("Corrupt binary json data")
				));
		return 0;
	}

	dynbuffer_append_byte(&buffer, 0);
	PG_RETURN_CSTRING(buffer.contents);
}

PG_FUNCTION_INFO_V1(pgjson_json_recv);
Datum
pgjson_json_recv(PG_FUNCTION_ARGS)
{
	PG_RETURN_DATUM(PG_GETARG_DATUM(0));
}

PG_FUNCTION_INFO_V1(pgjson_json_send);
Datum
pgjson_json_send(PG_FUNCTION_ARGS)
{
	PG_RETURN_DATUM(PG_GETARG_DATUM(0));
}

/* json support functions */
// JsonAsBinary(json) as Bytea
PG_FUNCTION_INFO_V1(pgjson_json_as_binary);
Datum
pgjson_json_as_binary(PG_FUNCTION_ARGS)
{
	PG_RETURN_DATUM(PG_GETARG_DATUM(0));
}
