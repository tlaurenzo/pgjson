/**
 * jsoncast.c
 * Provides type casts between various postgres types and their json equivilents.
 *
 * The following BSON types have direct equivilents in PostgreSQL:
 * 		BSON Type		PG Type
 * 		---------		-------
 * 		INT32			integer
 * 		INT64			bigint
 * 		DOUBLE			double precision
 * 		STRING			text
 * 		SYMBOL			text
 * 		BINARY			bytea
 * 		BOOL			boolean
 * 		DATETIME		timestamp without timezone (some precision is lost)
 *
 * Casting to and from equivilent types is done with no further
 * interpretation.
 *
 */
#include "pgjson.h"
#include "util/setup.h"
#include <math.h>
#include "json/bsonliteral.h"
#include "json/bsonparser.h"

/** Preamble from json_to_* functions.  Decodes the input json value
 *  and provides error/null handling.
 */
#define JSON_TO_PREAMBLE \
	void *jsondata; \
	bsonvalue_t inputvalue; \
	\
	if (PG_ARGISNULL(0)) PG_RETURN_NULL(); \
	jsondata=PG_DETOAST_DATUM_PACKED(PG_GETARG_DATUM(0)); \
	if (!bsonvalue_load(&inputvalue, VARDATA_ANY(jsondata), VARSIZE_ANY_EXHDR(jsondata), BSONMODE_ROOT)) { \
		ereport(ERROR, ( \
				errcode(ERRCODE_INVALID_PARAMETER_VALUE), \
				errmsg("Unable to parse BSON"))); \
		return; \
	} \
	if (inputvalue.type==BSONTYPE_NULL || inputvalue.type==BSONTYPE_UNDEFINED) \
		PG_RETURN_NULL();

#define JSON_FROM_PREAMBLEP(type, literal_size) \
	type value; \
	void *resultdata; \
	uint8_t bsonliteral[literal_size]; \
	if (PG_ARGISNULL(0)) return pgjson_return_null_value();

#define JSON_FROM_SUFFIXP(converter) \
	converter(bsonliteral, value); \
	resultdata=palloc(sizeof(bsonliteral) + VARHDRSZ); \
	memcpy(VARDATA(resultdata), bsonliteral, sizeof(bsonliteral)); \
	SET_VARSIZE(resultdata, sizeof(bsonliteral)+VARHDRSZ); \
	PG_RETURN_POINTER(resultdata);


static int64_t ecma_parse_integer(char *sz)
{
	/* todo probably need to pay more attention to this */
	return strtoll(sz, NULL, 10);
}

static double ecma_parse_double(char *sz)
{
	/* todo probably need to pay more attention to this */
	return strtod(sz, NULL);
}

static void *cstring_to_vartext(char *sz)
{
	size_t len=strlen(sz);
	void *data=palloc(len+VARHDRSZ);
	memcpy(VARDATA(data), sz, len);
	SET_VARSIZE(data, len+VARHDRSZ);
	return data;
}

/*** to/from integer ***/
PG_FUNCTION_INFO_V1(pgjson_json_to_integer);
Datum
pgjson_json_to_integer(PG_FUNCTION_ARGS)
{
	JSON_TO_PREAMBLE;

	switch (inputvalue.type) {
	case BSONTYPE_INT32:
		PG_RETURN_INT32(inputvalue.value.int32_value);
		break;
	case BSONTYPE_INT64:
		PG_RETURN_INT32((int32_t)inputvalue.value.int64_value);
		break;
	case BSONTYPE_DOUBLE:
		PG_RETURN_INT32((int32_t)inputvalue.value.double_value);
		break;
	case BSONTYPE_STRING:
	case BSONTYPE_SYMBOL:
		/* presume contents to be null terminated per bson spec */
		PG_RETURN_INT32((int32_t)ecma_parse_integer(inputvalue.value.string.contents));
		break;
	case BSONTYPE_DATETIME:
		PG_RETURN_INT32((int32_t)inputvalue.value.timestamp_value);
		break;
	}

	/* unrecognized */
	PG_RETURN_INT32(0);
}
PG_FUNCTION_INFO_V1(pgjson_json_from_integer);
Datum
pgjson_json_from_integer(PG_FUNCTION_ARGS)
{
	JSON_FROM_PREAMBLEP(int32_t, BSONLITERAL_SIZE_INT32);

	value=PG_GETARG_INT32(0);

	JSON_FROM_SUFFIXP(BSONLITERAL_MAKE_INT32);
}

/*** to/from bigint ***/
PG_FUNCTION_INFO_V1(pgjson_json_to_bigint);
Datum
pgjson_json_to_bigint(PG_FUNCTION_ARGS)
{
	JSON_TO_PREAMBLE;

	switch (inputvalue.type) {
	case BSONTYPE_INT32:
		PG_RETURN_INT64(inputvalue.value.int32_value);
		break;
	case BSONTYPE_INT64:
		PG_RETURN_INT64((int64_t)inputvalue.value.int64_value);
		break;
	case BSONTYPE_DOUBLE:
		PG_RETURN_INT64((int64_t)inputvalue.value.double_value);
		break;
	case BSONTYPE_STRING:
	case BSONTYPE_SYMBOL:
		/* presume contents to be null terminated per bson spec */
		PG_RETURN_INT64((int64_t)ecma_parse_integer(inputvalue.value.string.contents));
		break;
	case BSONTYPE_DATETIME:
		PG_RETURN_INT64(inputvalue.value.timestamp_value);
		break;
	}

	/* unrecognized */
	PG_RETURN_INT64(0);
}
PG_FUNCTION_INFO_V1(pgjson_json_from_bigint);
Datum
pgjson_json_from_bigint(PG_FUNCTION_ARGS)
{
	JSON_FROM_PREAMBLEP(int64_t, BSONLITERAL_SIZE_INT64);

	value=PG_GETARG_INT64(0);

	JSON_FROM_SUFFIXP(BSONLITERAL_MAKE_INT64);
}

/*** to/from double precision ***/
PG_FUNCTION_INFO_V1(pgjson_json_to_doubleprecision);
Datum
pgjson_json_to_doubleprecision(PG_FUNCTION_ARGS)
{
	JSON_TO_PREAMBLE;

	switch (inputvalue.type) {
	case BSONTYPE_INT32:
		PG_RETURN_FLOAT8(inputvalue.value.int32_value);
		break;
	case BSONTYPE_INT64:
		PG_RETURN_FLOAT8((int32_t)inputvalue.value.int64_value);
		break;
	case BSONTYPE_DOUBLE:
		PG_RETURN_FLOAT8((int32_t)inputvalue.value.double_value);
		break;
	case BSONTYPE_STRING:
	case BSONTYPE_SYMBOL:
		/* presume contents to be null terminated per bson spec */
		PG_RETURN_FLOAT8(ecma_parse_double(inputvalue.value.string.contents));
		break;
	}
	/* unrecognized */
	PG_RETURN_FLOAT8(NAN);
}
PG_FUNCTION_INFO_V1(pgjson_json_from_doubleprecision);
Datum
pgjson_json_from_doubleprecision(PG_FUNCTION_ARGS)
{
	JSON_FROM_PREAMBLEP(double, BSONLITERAL_SIZE_DOUBLE);

	value=PG_GETARG_FLOAT8(0);

	JSON_FROM_SUFFIXP(BSONLITERAL_MAKE_DOUBLE);
}

/*** text ***/
PG_FUNCTION_INFO_V1(pgjson_json_to_text);
Datum
pgjson_json_to_text(PG_FUNCTION_ARGS)
{
	char scratch[32];
	void *textdata=0;
	JSON_TO_PREAMBLE;

	switch (inputvalue.type) {
	case BSONTYPE_STRING:
	case BSONTYPE_SYMBOL:
		/* todo utf8 to db encoding */
		textdata=palloc(inputvalue.value.string.length+VARHDRSZ);
		memcpy(VARDATA(textdata), inputvalue.value.string.contents,
				inputvalue.value.string.length);
		SET_VARSIZE(textdata, inputvalue.value.string.length - 1 + VARHDRSZ);	/* take null off */
		break;
	case BSONTYPE_BOOL:
		textdata=cstring_to_vartext(inputvalue.value.bool_value ?
				"true" :
				"false");
		break;
	case BSONTYPE_INT32:
		sprintf(scratch, "%d", inputvalue.value.int32_value);
		textdata=cstring_to_vartext(scratch);
		break;
	case BSONTYPE_INT64:
		sprintf(scratch, PRId64, inputvalue.value.int64_value);
		textdata=cstring_to_vartext(scratch);
		break;
	case BSONTYPE_TIMESTAMP:
		sprintf(scratch, PRId64, inputvalue.value.datetime_value);
		textdata=cstring_to_vartext(scratch);
		break;
	}

	if (!textdata) PG_RETURN_NULL();
	else PG_RETURN_TEXT_P(textdata);
}
PG_FUNCTION_INFO_V1(pgjson_json_from_text);
Datum
pgjson_json_from_text(PG_FUNCTION_ARGS)
{
	void *textdatain;
	uint8_t *bsonstring;
	size_t   bsonstringlen;
	void *jsonout;

	if (PG_ARGISNULL(0)) return pgjson_return_null_value();
	textdatain=PG_GETARG_TEXT_P(0);

	/* todo convert from db encoding to UTF8 */
	bsonstring=VARDATA(textdatain);
	bsonstringlen=VARSIZE(textdatain)-VARHDRSZ;

	jsonout=palloc(BSONLITERAL_SIZE_STRING(bsonstringlen)+VARHDRSZ);
	SET_VARSIZE(jsonout, BSONLITERAL_SIZE_STRING(bsonstringlen)+VARHDRSZ);
	BSONLITERAL_MAKE_STRING(VARDATA(jsonout), bsonstring, bsonstringlen);

	PG_RETURN_POINTER(jsonout);
}

/*** bytea ***/
PG_FUNCTION_INFO_V1(pgjson_json_to_bytea);
Datum
pgjson_json_to_bytea(PG_FUNCTION_ARGS)
{
	/* todo */
	PG_RETURN_NULL();
}
PG_FUNCTION_INFO_V1(pgjson_json_from_bytea);
Datum
pgjson_json_from_bytea(PG_FUNCTION_ARGS)
{
	/* todo */
	PG_RETURN_NULL();
}

/*** bool ***/
PG_FUNCTION_INFO_V1(pgjson_json_to_boolean);
Datum
pgjson_json_to_boolean(PG_FUNCTION_ARGS)
{
	bool bvalue=true;
	bsonvalue_t child;
	JSON_TO_PREAMBLE;

	switch (inputvalue.type) {
	case BSONTYPE_BOOL:
		bvalue=inputvalue.value.bool_value;
		break;
	case BSONTYPE_NULL:
	case BSONTYPE_UNDEFINED:
		bvalue=false;
		break;
	case BSONTYPE_INT32:
		bvalue=!!inputvalue.value.int32_value;
		break;
	case BSONTYPE_INT64:
		bvalue=!!inputvalue.value.int64_value;
		break;
	case BSONTYPE_DOUBLE:
		bvalue=!(inputvalue.value.double_value==0.0 ||
				 isnan(inputvalue.value.double_value));
		break;
	case BSONTYPE_STRING:
	case BSONTYPE_SYMBOL:
		/* presume contents to be null terminated per bson spec */
		bvalue=inputvalue.value.string.contents[0]!=0;
		break;
	case BSONTYPE_DATETIME:
		bvalue=true;
		break;
	case BSONTYPE_DOCUMENT:
	case BSONTYPE_ARRAY:
		bvalue=bsonvalue_load_firstchild(&inputvalue, &child) &&
				child.type!=0;
		break;
	}

	/* unrecognized */
	PG_RETURN_BOOL(bvalue);
}
PG_FUNCTION_INFO_V1(pgjson_json_from_boolean);
Datum
pgjson_json_from_boolean(PG_FUNCTION_ARGS)
{
	JSON_FROM_PREAMBLEP(bool, BSONLITERAL_SIZE_BOOL);

	value=PG_GETARG_BOOL(0);

	JSON_FROM_SUFFIXP(BSONLITERAL_MAKE_BOOL);
}

/*** timestamp ***/
PG_FUNCTION_INFO_V1(pgjson_json_to_timestamp);
Datum
pgjson_json_to_timestamp(PG_FUNCTION_ARGS)
{
	JSON_TO_PREAMBLE;

	switch (inputvalue.type) {
	case BSONTYPE_DATETIME:
		PG_RETURN_INT64(inputvalue.value.timestamp_value);
		break;
	case BSONTYPE_INT32:
		PG_RETURN_INT64(inputvalue.value.int32_value);
		break;
	case BSONTYPE_INT64:
		PG_RETURN_INT64((int64_t)inputvalue.value.int64_value);
		break;
	case BSONTYPE_DOUBLE:
		PG_RETURN_INT64((int64_t)inputvalue.value.double_value);
		break;
	case BSONTYPE_STRING:
	case BSONTYPE_SYMBOL:
		/* presume contents to be null terminated per bson spec */
		PG_RETURN_INT64((int64_t)ecma_parse_integer(inputvalue.value.string.contents));
		break;
	}

	/* unrecognized */
	PG_RETURN_INT64(0);
}
PG_FUNCTION_INFO_V1(pgjson_json_from_timestamp);
Datum
pgjson_json_from_timestamp(PG_FUNCTION_ARGS)
{
	JSON_FROM_PREAMBLEP(int64_t, BSONLITERAL_SIZE_INT64);

	/* todo this is not right - just getting the syntax in place */
	value=PG_GETARG_INT64(0);
	value /= 1000;

	JSON_FROM_SUFFIXP(BSONLITERAL_MAKE_INT64);
}


