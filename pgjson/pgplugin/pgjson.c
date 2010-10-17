#include "pgjson.h"
#include "json/bsonliteral.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

/*** support function ***/
/**
 * Allocates a vardata structure and returns Datum for the undefined value
 */
Datum pgjson_return_undefined_value()
{
	void *binarydata;

	binarydata=palloc(sizeof(BSONLITERAL_UNDEFINED) + VARHDRSZ);
	SET_VARSIZE(binarydata, sizeof(BSONLITERAL_UNDEFINED)+VARHDRSZ);
	memcpy(VARDATA(binarydata), BSONLITERAL_UNDEFINED, sizeof(BSONLITERAL_UNDEFINED));

	PG_RETURN_POINTER(binarydata);
}

Datum pgjson_return_null_value()
{
	void *binarydata;

	binarydata=palloc(sizeof(BSONLITERAL_NULL) + VARHDRSZ);
	SET_VARSIZE(binarydata, sizeof(BSONLITERAL_NULL)+VARHDRSZ);
	memcpy(VARDATA(binarydata), BSONLITERAL_NULL, sizeof(BSONLITERAL_NULL));

	PG_RETURN_POINTER(binarydata);
}

/**
 * Returns an "undefined" string as a cstring
 */
Datum pgjson_return_undefined_cstring()
{
	char *retval=palloc(10);
	strcpy(retval, "undefined");
	PG_RETURN_CSTRING(retval);
}

Datum pgjson_return_undefined_text()
{
	void *textdata=palloc(10+VARHDRSZ);
	SET_VARSIZE(textdata, 9+VARHDRSZ);
	strcpy(VARDATA(textdata), "undefined");
	PG_RETURN_TEXT_P(textdata);
}
