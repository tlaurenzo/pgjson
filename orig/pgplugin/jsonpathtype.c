/**
 * pgjsonpath.c
 * Implement functions for the jsonpath datatype
 */
#include "pgjson.h"
#include "util/setup.h"
#include "json/jsonpath.h"

/**
 * Convert cstring to jsonpath type
 */
PG_FUNCTION_INFO_V1(pgjson_jsonpath_in);
Datum
pgjson_jsonpath_in(PG_FUNCTION_ARGS)
{
	char *inputtext=PG_GETARG_CSTRING(0);
	size_t inputtextlen=strlen(inputtext);
	void *binarydata;
	size_t binarysize;

	stringwriter_t buffer;

	stringwriter_init(&buffer, inputtextlen*2);
	if (jsonpath_parse(&buffer, inputtext, inputtextlen)) {
		/* success */
		binarydata=stringwriter_get_alloc_buffer(&buffer);
		SET_VARSIZE(binarydata, buffer.pos + VARHDRSZ);
		stringwriter_detach(&buffer);
		PG_RETURN_POINTER(binarydata);
	} else {
		/* fail */
		ereport(ERROR, (
				errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("JSON Path syntax error: %s", (char*)buffer.string)));
		stringwriter_destroy(&buffer);
	}
}

/**
 * Convert jsonpath type to cstring
 */
PG_FUNCTION_INFO_V1(pgjson_jsonpath_out);
Datum
pgjson_jsonpath_out(PG_FUNCTION_ARGS)
{
	void *binarydata;
	size_t binarysize;
	stringwriter_t textbuffer;
	jsonpathiter_t iter;
	char *rettext;

	/* get the binary parameter */
	binarydata=PG_DETOAST_DATUM_PACKED(PG_GETARG_DATUM(0));
	binarysize=VARSIZE_ANY_EXHDR(binarydata);

	/* setup serialization */
	jsonpath_iter_begin(&iter, VARDATA(binarydata), binarysize);
	stringwriter_init(&textbuffer, binarysize*2);

	if (jsonpath_serialize(&textbuffer, &iter)) {
		/* success - return text */
		rettext=palloc(textbuffer.pos+1);
		memcpy(rettext, textbuffer.string, textbuffer.pos);
		rettext[textbuffer.pos]=0;
		stringwriter_destroy(&textbuffer);
		PG_RETURN_CSTRING(rettext);
	} else {
		/* failure - shouldn't happen but may be internal failure */
		stringwriter_destroy(&textbuffer);
		ereport(ERROR, (
				errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("Internal error serializing JSON Path")));
	}
}

