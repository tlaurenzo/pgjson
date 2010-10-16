/**
 * jsonevalfunctions.c
 *
 * Functions and operators enabling the evaluation of jsonpath
 * types and type coercion.
 */
#include "pgjson.h"
#include "util/setup.h"
#include "json/jsonpath.h"
#include "json/jsonoutputter.h"

/**
 * Parameters: json, jsonpath
 * Return: json
 * Strict
 */
PG_FUNCTION_INFO_V1(pgjson_jsonpath_eval);
Datum
pgjson_jsonpath_eval(PG_FUNCTION_ARGS)
{
	void *jsondata=PG_DETOAST_DATUM_PACKED(PG_GETARG_DATUM(0));
	void *pathdata=PG_DETOAST_DATUM_PACKED(PG_GETARG_DATUM(1));
	void *resultdata;

	bsonvalue_t rootvalue;
	jsonpathiter_t pathiter;
	bsonvalue_t resultvalue;
	jsonoutputter_t resultoutputter;
	uint8_t *resultbuffer;
	size_t resultsize;
	const char *error_msg;

	/* rehydrate the bsonvalue_t */
	if (!bsonvalue_load(&rootvalue, VARDATA_ANY(jsondata), VARSIZE_ANY_EXHDR(jsondata),
			BSONMODE_ROOT))
	{
		/* bson format error */
		PGJSON_DEBUG(LEVEL_WARN, "Illegal BSON value");
		return pgjson_return_undefined_value();
	}

	/* rehydrate the jsonpath into an iterator */
	jsonpath_iter_begin(&pathiter, VARDATA_ANY(pathdata), VARSIZE_ANY_EXHDR(pathdata));

	if (!jsonpath_evaluate(&rootvalue, &pathiter, &resultvalue))
	{
		PGJSON_DEBUG(LEVEL_WARN, "Internal error evaluating JSON path");
		return pgjson_return_undefined_value();
	}

	/* output a standalone value */
	jsonoutputter_open_bson_buffer(&resultoutputter, VARSIZE_ANY_EXHDR(jsondata));
	if (!bsonvalue_visit(&resultvalue, jsonoutputter_getvisitor(&resultoutputter)))
	{
		/* error generating output */
		PGJSON_DEBUG(LEVEL_WARN, "Internal error generating stand-alone BSON stream");
		jsonoutputter_close(&resultoutputter);
		return pgjson_return_undefined_value();
	}

	/* serialization error ? */
	if (jsonoutputter_has_error(&resultoutputter, &error_msg))
	{
		/* serialization error */
		PGJSON_DEBUGF(LEVEL_WARN, "Internal error serializaing BSON: %s", error_msg);
		return pgjson_return_undefined_value();
	}

	/* copy buffer and return */
	jsonoutputter_get_buffer(&resultoutputter, &resultbuffer, &resultsize);
	resultdata=palloc(resultsize+VARHDRSZ);
	SET_VARSIZE(resultdata, resultsize+VARHDRSZ);
	memcpy(VARDATA(resultdata), resultbuffer, resultsize);
	jsonoutputter_close(&resultoutputter);

	PG_RETURN_POINTER(resultdata);
}
