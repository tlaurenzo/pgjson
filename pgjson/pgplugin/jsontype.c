/**
 * jsonbinary.c
 * Implementation of the json and jsonbinary datatypes
 */
#include "pgjson.h"

#include "util/setup.h"
#include "json/bsonparser.h"
#include "json/jsonoutputter.h"
#include "json/jsonparser.h"
#include "json/bsonliteral.h"

#include "pgutil.h"

/*
 * Cleans up an outputter, returning its value as vardata
 */
static inline Datum return_stringwriter_vardata(stringwriter_t *stringwriter)
{
	void    *data;
	size_t   datalength;

	datalength=stringwriter->pos;
	data=stringwriter_get_alloc_buffer(stringwriter);
	stringwriter_detach(stringwriter);
	if (!data) return 0;
	SET_VARSIZE(data, datalength+VARHDRSZ);

	PG_RETURN_POINTER(data);
}


/**
 * convert a bson input buffer to json text and return it as a variable length
 * text Datum.  If raise_error is true, then verbose errors
 * are raised.  Otherwise, no errors are raised.  In either case, on error, the null Datum (0) is returned.
 */
static Datum parse_bson_to_json_as_vardata(void *binarydata, size_t binarysize, bool raise_error)
{
	jsonoutputter_t outputter;
	stringwriter_t outputbuffer=STRINGWRITER_INIT(binarysize*2);
	int32_t rc;

	Datum retval=0;
	const char *outputerrormsg;
	bool outputerror;

	/* initialize outputter */
	jsonoutputter_open_json_buffer(&outputter, &outputbuffer);
	rc=bsonparser_parse(binarydata, binarysize,
			jsonoutputter_getvisitor(&outputter));
	outputerror=jsonoutputter_has_error(&outputter, &outputerrormsg);
	jsonoutputter_close(&outputter);

	if (rc<0) {
		if (raise_error) {
			ereport(ERROR, (
					errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("BSON Parse error")));
		} else {
			PGJSON_DEBUG(LEVEL_WARN, "BSON Parse error");
		}
	} else if (outputerror) {
		/* check json outputter */
		if (raise_error) {
			ereport(ERROR, (
					errcode(ERRCODE_INTERNAL_ERROR),
					errmsg("Unexpected serialization error: %s", outputerrormsg)));
		} else {
			PGJSON_DEBUGF(LEVEL_WARN, "Unexpected serialization error: %s", outputerrormsg);
		}
	} else {
		/* success */
		retval=return_stringwriter_vardata(&outputbuffer);
	}

	stringwriter_destroy(&outputbuffer);

	if (!retval) return pgjson_return_undefined_text();
	else return retval;
}

/**
 * convert a bson input buffer to json text and return it as a c string
 * Datum.  If raise_error is true, then verbose errors
 * are raised.  Otherwise, no errors are raised.  In either case, on error, the null Datum (0) is returned.
 */
static Datum parse_bson_to_json_as_cstring(void *binarydata, size_t binarysize, bool raise_error)
{
	jsonoutputter_t outputter;
	stringwriter_t outputbuffer=STRINGWRITER_INIT(binarysize*2);
	int32_t rc;

	Datum retval=0;
	const char *outputerrormsg;
	bool outputerror;
	char *retcstring;

	/* initialize outputter */
	jsonoutputter_open_json_buffer(&outputter, &outputbuffer);
	rc=bsonparser_parse(binarydata, binarysize,
			jsonoutputter_getvisitor(&outputter));
	outputerror=jsonoutputter_has_error(&outputter, &outputerrormsg);
	jsonoutputter_close(&outputter);

	if (rc<0) {
		if (raise_error) {
			ereport(ERROR, (
					errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("BSON Parse error")));
		} else {
			PGJSON_DEBUG(LEVEL_WARN, "BSON Parse error");
		}
	} else if (outputerror) {
		/* check json outputter */
		if (raise_error) {
			ereport(ERROR, (
					errcode(ERRCODE_INTERNAL_ERROR),
					errmsg("Unexpected serialization error: %s", outputerrormsg)));
		} else {
			PGJSON_DEBUGF(LEVEL_WARN, "Unexpected serialization error: %s", outputerrormsg);
		}
	} else if (outputbuffer.pos>0) {
		/* success */
		retcstring=palloc(outputbuffer.pos+1);
		memcpy(retcstring, outputbuffer.string, outputbuffer.pos);
		retcstring[outputbuffer.pos]=0;
		retval=CStringGetDatum(retcstring);
	}

	stringwriter_destroy(&outputbuffer);

	if (!retval) return pgjson_return_undefined_cstring();
	else return retval;
}

/**
 * parse json text to bson and return a vardata Datum.  If raise_error is true, then verbose errors
 * are raised.  Otherwise, no errors are raised.  In either case, on error, the null Datum (0) is returned.
 */
static Datum parse_json_to_bson_as_vardata(void *textdata, size_t textsize, bool raise_error)
{
	jsonparser_t parser;
	jsonoutputter_t outputter;
	stringwriter_t outputbuffer=STRINGWRITER_INIT(textsize*2);
	int32_t rc;

	Datum retval=0;
	const char *outputerrormsg;
	bool outputerror;

	/* initialize outputter */
	jsonparser_init(&parser);
	jsonparser_inputstring(&parser, textdata, textsize, false);
	jsonoutputter_open_bson_buffer(&outputter, &outputbuffer);
	jsonparser_parse(&parser,
			jsonoutputter_getvisitor(&outputter));
	outputerror=jsonoutputter_has_error(&outputter, &outputerrormsg);
	jsonoutputter_close(&outputter);

	if (parser.has_error) {
		if (raise_error) {
			ereport(ERROR, (
					errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("JSON Parse error: %s", parser.error_msg)));
		} else {
			PGJSON_DEBUGF(LEVEL_WARN, "JSON Parse error", parser.error_msg);
		}
	} else if (outputerror) {
		/* check json outputter */
		if (raise_error) {
			ereport(ERROR, (
					errcode(ERRCODE_INTERNAL_ERROR),
					errmsg("Unexpected serialization error: %s", outputerrormsg)));
		} else {
			PGJSON_DEBUGF(LEVEL_WARN, "Unexpected serialization error: %s", outputerrormsg);
		}
	} else {
		/* success */
		retval=return_stringwriter_vardata(&outputbuffer);
	}

	stringwriter_destroy(&outputbuffer);

	if (!retval) return pgjson_return_undefined_text();
	else return retval;

}

/**** Text and Binary BSON IO ****/
/*
 * BSON is stored internally as a variable length byte array.  We borrow the
 * hex encoding of bytea as the text format.  The binary format is just an
 * image.
 */
PG_FUNCTION_INFO_V1(pgjson_jsonbinary_in);
Datum
pgjson_jsonbinary_in(PG_FUNCTION_ARGS)
{
	char *input_text = PG_GETARG_CSTRING(0);
	size_t byte_count_exp, byte_count_act;
	size_t len;
	void *result;

	len=strlen(input_text);
	if (len>=2 && len%2==0 && input_text[0]=='\\' && input_text[1]=='x') {
		byte_count_exp=(len-2)/2;
		result=palloc(byte_count_exp + VARHDRSZ);
		byte_count_act=pgjson_hex_decode(input_text+2, len-2, VARDATA(result));
		if (byte_count_act==byte_count_exp) {
			SET_VARSIZE(result, byte_count_act + VARHDRSZ);
			PG_RETURN_POINTER(result);
		}
	}

	/* error - malformed */
	ereport(ERROR, (
			errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("JSON binary text encoding syntax error (should be \\x[hexdigits])")));
	return;
}

PG_FUNCTION_INFO_V1(pgjson_jsonbinary_out);
Datum
pgjson_jsonbinary_out(PG_FUNCTION_ARGS)
{
	void *binarydata;
	size_t binarysize;
	char *cstring;
	size_t cstringlen;

	binarydata=PG_DETOAST_DATUM_PACKED(PG_GETARG_DATUM(0));
	binarysize=VARSIZE_ANY_EXHDR(binarydata);

	/* serialize as \x[hexdigits] string size is 2 + (binarysize*2) +1 */
	cstringlen=2+binarysize*2;
	cstring=palloc(2 + cstringlen +1);
	cstring[0]='\\';
	cstring[1]='x';
	pgjson_hex_encode(VARDATA_ANY(binarydata), binarysize, cstring+2);
	cstring[cstringlen]=0;

	PG_RETURN_CSTRING(cstring);
}

PG_FUNCTION_INFO_V1(pgjson_jsontext_in);
Datum
pgjson_jsontext_in(PG_FUNCTION_ARGS)
{
	char *text = PG_GETARG_CSTRING(0);
	size_t len = strlen(text);

	return parse_json_to_bson_as_vardata(text, len, true);
}

PG_FUNCTION_INFO_V1(pgjson_jsontext_out);
Datum
pgjson_jsontext_out(PG_FUNCTION_ARGS)
{
	void *binarydata=PG_DETOAST_DATUM_PACKED(PG_GETARG_DATUM(0));
	return parse_bson_to_json_as_cstring(VARDATA_ANY(binarydata), VARSIZE_ANY_EXHDR(binarydata), false);
}

/* recv binary data returning type.  since the binary representation
 * is the internal representation, just return the argument.
 */
PG_FUNCTION_INFO_V1(pgjson_jsonbinary_recv);
Datum
pgjson_jsonbinary_recv(PG_FUNCTION_ARGS)
{
	PG_RETURN_DATUM(PG_GETARG_DATUM(0));
}

/*
 * send binary data retunring bytes.  since the binary representation
 * is the internal representation, just return the argument.
 */
PG_FUNCTION_INFO_V1(pgjson_jsonbinary_send);
Datum
pgjson_jsonbinary_send(PG_FUNCTION_ARGS)
{
	PG_RETURN_DATUM(PG_GETARG_DATUM(0));
}


/**
 * Returns json as JSON text.  Errors are raised.
 * todo: Be more lenient about the encoding (not just ascii)
 */
PG_FUNCTION_INFO_V1(pgjson_json_asjsontext);
Datum
pgjson_json_asjsontext(PG_FUNCTION_ARGS)
{
	void *vardata;
	if (PG_ARGISNULL(0)) PG_RETURN_NULL();
	vardata=PG_DETOAST_DATUM_PACKED(PG_GETARG_DATUM(0));
	return parse_bson_to_json_as_vardata(VARDATA_ANY(vardata), VARSIZE_ANY_EXHDR(vardata), true);
}

/**
 * Returns json as JSON text.  Errors are ignored and null returned.
 * todo: Be more lenient about the encoding (not just ascii)
 * todo: errors are being returned as 'undefined'::json
 */
PG_FUNCTION_INFO_V1(pgjson_json_asjsontextsilent);
Datum
pgjson_json_asjsontextsilent(PG_FUNCTION_ARGS)
{
	void *vardata;
	if (PG_ARGISNULL(0)) PG_RETURN_NULL();
	vardata=PG_DETOAST_DATUM_PACKED(PG_GETARG_DATUM(0));
	return parse_bson_to_json_as_vardata(VARDATA_ANY(vardata), VARSIZE_ANY_EXHDR(vardata), false);
}

/*
 * parses text, producing a binary output.  on parse error, an exception
 * is raised
 */
PG_FUNCTION_INFO_V1(pgjson_json_parse);
Datum
pgjson_json_parse(PG_FUNCTION_ARGS)
{
	void *vardata;
	if (PG_ARGISNULL(0)) PG_RETURN_NULL();
	vardata=PG_DETOAST_DATUM_PACKED(PG_GETARG_DATUM(0));
	return parse_json_to_bson_as_vardata(VARDATA_ANY(vardata), VARSIZE_ANY_EXHDR(vardata), true);
}

/*
 * parses text, producing a binary output.  on parse error, returns null
 * todo currently returning 'undefined'::json on output
 */
PG_FUNCTION_INFO_V1(pgjson_json_parsesilent);
Datum
pgjson_json_parsesilent(PG_FUNCTION_ARGS)
{
	void *vardata;
	if (PG_ARGISNULL(0)) PG_RETURN_NULL();
	vardata=PG_DETOAST_DATUM_PACKED(PG_GETARG_DATUM(0));
	return parse_json_to_bson_as_vardata(VARDATA_ANY(vardata), VARSIZE_ANY_EXHDR(vardata), false);
}
