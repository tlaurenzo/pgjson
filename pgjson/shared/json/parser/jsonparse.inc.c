/**
 * jsonparse.inc.c
 *
 * Includable C module for invoking the JSON parser.
 */
#include "jsonparse.h"

static const uint8_t JSON_IDENT_TRUE[] = { 't', 'r', 'u', 'e' };
static const uint8_t JSON_IDENT_FALSE[] = { 'f', 'a', 'l', 's', 'e' };
static const uint8_t JSON_IDENT_NULL[] = { 'n', 'u', 'l', 'l' };
static const uint8_t JSON_IDENT_UNDEFINED[] = { 'u', 'n', 'd', 'e', 'f', 'i', 'n', 'e', 'd' };

#define JSON_TOKEN_IS_IDENTIFIER(ident) \
	(sizeof(ident)==parsestate->lexstate.buffer_pos && memcmp(ident, parsestate->lexstate.buffer, sizeof(ident))==0)

/**** Forward declarations ****/
JSON_FDECLP bool jsonparse_object(jsonparseinfo_arg parsestate);
JSON_FDECLP bool jsonparse_array(jsonparseinfo_arg parsestate);
JSON_FDECLP bool jsonparse_value(jsonparseinfo_arg parsestate, jsonlex_token_t token);


/**
 * Parses an object, assuming that the opening jsonlex_lbrace
 * has already been consumed.
 */
JSON_FDECLP bool jsonparse_object(jsonparseinfo_arg parsestate)
{
	jsonlex_token_t token;

	#ifdef JSONPARSE_ACTION_OBJECT_START
	JSONPARSE_ACTION_OBJECT_START();
	#endif

	token=jsonlex_next_token(&parsestate->lexstate);
	if (token==jsonlex_rbrace) {
		/* end of object (will only hit this for empty object) */
		#ifdef JSONPARSE_ACTION_OBJECT_END
		JSONPARSE_ACTION_OBJECT_END();
		#endif
		return true;
	}

	for (;;) {
		/* token 1: either end the object or label the field */
		if (token==jsonlex_identifier||token==jsonlex_string) {
			/* treat either a string or identifier as a field label */
			#ifdef JSONPARSE_ACTION_OBJECT_LABEL
			JSONPARSE_ACTION_OBJECT_LABEL(parsestate->lexstate.buffer, parsestate->lexstate.buffer_pos);
			#endif
		} else {
			#ifdef JSONPARSE_ACTION_ERROR
			JSONPARSE_ACTION_ERROR("Expected object element", token);
			#endif
			return false;
		}

		/* token 2: colon */
		token=jsonlex_next_token(&parsestate->lexstate);
		if (token!=jsonlex_colon) {
			#ifdef JSONPARSE_ACTION_ERROR
			JSONPARSE_ACTION_ERROR("Expected colon", token);
			#endif
			return false;
		}

		/* token 3: value */
		token=jsonlex_next_token(&parsestate->lexstate);
		if (!jsonparse_value(parsestate, token)) {
			/* error already set */
			return false;
		}

		/* token 4: either comma (continue loop) or rbrace (end object) */
		token=jsonlex_next_token(&parsestate->lexstate);
		if (token==jsonlex_comma) {
			token=jsonlex_next_token(&parsestate->lexstate);
			continue;
		} else if (token==jsonlex_rbrace) {
			#ifdef JSONPARSE_ACTION_OBJECT_END
			JSONPARSE_ACTION_OBJECT_END();
			#endif
			return true;
		} else {
			#ifdef JSONPARSE_ACTION_ERROR
			JSONPARSE_ACTION_ERROR("Expected next object field or end of object", token);
			#endif
			return false;
		}
	}
}

/**
 * Parses an array, assuming that the opening jsonlex_lbracket
 * has already been consumed
 */
JSON_FDECLP bool jsonparse_array(jsonparseinfo_arg parsestate)
{
	jsonlex_token_t token;

	#ifdef JSONPARSE_ACTION_ARRAY_START
	JSONPARSE_ACTION_ARRAY_START();
	#endif

	/* first token */
	token=jsonlex_next_token(&parsestate->lexstate);
	if (token==jsonlex_rbracket) {
		#ifdef JSONPARSE_ACTION_ARRAY_END
		JSONPARSE_ACTION_ARRAY_END();
		#endif
		return true;
	}

	for (;;) {
		if (!jsonparse_value(parsestate, token)) {
			return false;
		}

		/* comma */
		token=jsonlex_next_token(&parsestate->lexstate);
		if (token==jsonlex_comma) {
			token=jsonlex_next_token(&parsestate->lexstate);
			continue;
		}

		/* end of multi element array */
		if (token==jsonlex_rbracket) {
			/* end array */
			#ifdef JSONPARSE_ACTION_ARRAY_END
			JSONPARSE_ACTION_ARRAY_END();
			#endif
			return true;
		}

		/* unexpected token */
		#ifdef JSONPARSE_ACTION_ERROR
		JSONPARSE_ACTION_ERROR("Expected end of array of additional element", token);
		#endif

		return false;
	}
}

/**
 * Parse a JSON "value".  Legal tokens in this state are:
 *   - jsonlex_lbrace: introduces an object value
 *   - jsonlex_lbracket: introduces an array value
 *   - jsonlex_identifier: interpreted as a JSON constant (null, undefined, true, false)
 *   - jsonlex_integer
 *   - jsonlex_numeric
 *   - jsonlex_string
 */
JSON_FDECLP bool jsonparse_value(jsonparseinfo_arg parsestate, jsonlex_token_t token)
{
	switch (token) {
	case jsonlex_lbrace:
		/* object */
		return jsonparse_object(parsestate);
	case jsonlex_lbracket:
		return jsonparse_array(parsestate);
	case jsonlex_identifier:
		/* we only support four different identifiers, so just figure which it is */
		if (JSON_TOKEN_IS_IDENTIFIER(JSON_IDENT_NULL)) {
			#ifdef JSONPARSE_ACTION_VALUE_NULL
			JSONPARSE_ACTION_VALUE_NULL();
			#endif
		} else if (JSON_TOKEN_IS_IDENTIFIER(JSON_IDENT_TRUE)) {
			#ifdef JSONPARSE_ACTION_VALUE_BOOL
			JSONPARSE_ACTION_VALUE_BOOL(true);
			#endif
		} else if (JSON_TOKEN_IS_IDENTIFIER(JSON_IDENT_FALSE)) {
			#ifdef JSONPARSE_ACTION_VALUE_BOOL
			JSONPARSE_ACTION_VALUE_BOOL(false);
			#endif
		} else if (JSON_TOKEN_IS_IDENTIFIER(JSON_IDENT_UNDEFINED)) {
			#ifdef JSONPARSE_ACTION_VALUE_BOOL
			JSONPARSE_ACTION_VALUE_UNDEFINED();
			#endif
		} else {
			/* unrecognized token */
			#ifdef JSONPARSE_ACTION_ERROR
			JSONPARSE_ACTION_ERROR("Expected true/false/null/undefined identifier", token);
			#endif
			return false;
		}
		return true;
	case jsonlex_integer:
		#ifdef JSONPARSE_ACTION_VALUE_INTEGER
		JSONPARSE_ACTION_VALUE_INTEGER(parsestate->lexstate.buffer, parsestate->lexstate.buffer_pos);
		#else
			/* caller may just want to treat INTEGER and NUMERIC the same, fallback */
			#ifdef JSONPARSE_ACTION_VALUE_NUMERIC
			JSONPARSE_ACTION_VALUE_NUMERIC(parsestate->lexstate.buffer, parsestate->lexstate.buffer_pos);
			#endif
		#endif
		return true;
	case jsonlex_numeric:
		#ifdef JSONPARSE_ACTION_VALUE_NUMERIC
		JSONPARSE_ACTION_VALUE_NUMERIC(parsestate->lexstate.buffer, parsestate->lexstate.buffer_pos);
		#endif
		return true;
	case jsonlex_string:
		#ifdef JSONPARSE_ACTION_VALUE_STRING
		JSONPARSE_ACTION_VALUE_STRING(parsestate->lexstate.buffer, parsestate->lexstate.buffer_pos);
		#endif
		return true;
	default:
		#ifdef JSONPARSE_ACTION_ERROR
		JSONPARSE_ACTION_ERROR("Expected legal value", token);
		#endif
		return false;
	}
}

/**
 * Parse a JSON text stream, returning true if it is valid, false otherwise.
 * Actions are invoked by expanding JSONPARSE_ACTION_* macros as items are
 * encountered.  Note that the corresponding action may be invoked prior
 * to completely validating, so if this method returns false, any mutations
 * made by JSONPARSE_ACTION_* should be discarded.
 */
JSON_FDECLP bool jsonparse(jsonparseinfo_arg parsestate)
{
	jsonlex_token_t token;

	if (!jsonparse_value(parsestate, jsonlex_next_token(&parsestate->lexstate))) return false;

	token=jsonlex_next_token(&parsestate->lexstate);
	if (token!=jsonlex_eof) {
		/* expected end of file */
		#ifdef JSONPARSE_ACTION_ERROR
		JSONPARSE_ACTION_ERROR("Expected end of file", token);
		#endif
		return false;
	}
	return true;
}
