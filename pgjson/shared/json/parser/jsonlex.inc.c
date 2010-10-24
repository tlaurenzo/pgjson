/**
 * parsejson.inc.c
 *
 * Provide the guts of a JSON parser as an includable C file.
 * The interface to the parser is via pre-processor defines.
 */
#include "jsonlex.h"


/**
 * Returns a string representation of the given token.  If it is an illegal
 * value, returns "UNKNOWN"
 */
JSON_FDECLP const char* jsonlex_token_str(jsonlex_token_t t)
{
	switch (t) {
	case jsonlex_lbrace: return "LEFT BRACE";
	case jsonlex_rbrace: return "RIGHT BRACE";
	case jsonlex_colon:  return "COLON";
	case jsonlex_comma:  return "COMMA";
	case jsonlex_lbracket: return "LEFT BRACKET";
	case jsonlex_rbracket: return "RIGHT BRACKET";
	case jsonlex_lparen:   return "LEFT PARENTHESIS";
	case jsonlex_rparen:   return "RIGHT PARENTHESIS";
	case jsonlex_identifier: return "IDENTIFIER";
	case jsonlex_integer: return "INTEGER";
	case jsonlex_numeric: return "NUMERIC";
	case jsonlex_string:  return "STRING";
	case jsonlex_eof:     return "<EOF>";
	default:
		return "<UNKNOWN>";
	}
}

JSON_FDECLP void jsonlex_realloc_buffer(jsonlex_state_arg lexstate)
{
	size_t newcapacity=lexstate->buffer_capacity*2;
	if (!newcapacity) newcapacity=256;

	#if JSONLEX_STATICBUFFER_SIZE > 0
		if (lexstate->buffer==lexstate->buffer_static) {
			/* just alloc a new one */
			lexstate->buffer=malloc(newcapacity);
		} else {
			lexstate->buffer=realloc(lexstate->buffer, newcapacity);
		}
	#else
		lexstate->buffer=realloc(lexstate->buffer, newcapacity);
	#endif
}

#define JSONLEX_BUFFER_BYTE(c) \
		if (lexstate->buffer_pos==lexstate->buffer_capacity) \
			jsonlex_realloc_buffer(lexstate); \
		lexstate->buffer[lexstate->buffer_pos++]=c;

#define JSONLEX_BUFFER_CLEAR() lexstate->buffer_pos=0;

JSON_FDECLP void jsonlex_init(jsonlex_state_arg lexstate)
{
	#if JSONLEX_STATICBUFFER_SIZE > 0
		lexstate->buffer=lexstate->buffer_static;
		lexstate->buffer_capacity=JSONLEX_STATICBUFFER_SIZE;
	#else
		lexstate->buffer=0;
		lexstate->buffer_capacity=0;
	#endif

	lexstate->buffer_pos=0;

	#if JSONLEX_TRACK_POSITION
		lexstate->start_line=1;
		lexstate->start_col=0;
		lexstate->end_line=1;
		lexstate->end_col=0;
	#endif
}

JSON_FDECLP void jsonlex_destroy(jsonlex_state_arg lexstate)
{
	#if JSONLEX_STATICBUFFER_SIZE > 0
		if (lexstate->buffer!=lexstate->buffer_static)
			free(lexstate->buffer);
	#else
		if (lexstate->buffer)
			free(lexstate->buffer);
	#endif
}

JSON_FDECLP jsonlex_token_t jsonlex_next_token(jsonlex_state_arg lexstate)
{
	int charin;
	int quotechar;
	jsonlex_charclass_t cc;

	JSONLEX_BUFFER_CLEAR();
	for (;;) {
		charin=JSONLEX_GETC();
		if (charin<0) return jsonlex_eof;
		cc=JSONLEX_CC_TABLE[charin];

		/* switch statement consists of small integer cases,
		 * allowing the compiler to use a jump table
		 */
		switch (cc&JSONLEX_CC_CLASSMASK) {
		case JSONLEX_CC_WHITE:
			continue;
		case JSONLEX_CC_TERMCHAR:
			return (cc>>JSONLEX_CC_TERMCHAR_SHIFT)&JSONLEX_CC_TERMCHAR_MASK;
		case JSONLEX_CC_IDCHAR:
			/* introduce identifier */
			/* identifier continues until first non IDCHAR or DIGIT */
			for (;;) {
				JSONLEX_BUFFER_BYTE(charin);
				charin=JSONLEX_GETC();
				if (charin<0) break;	/* eof - identifier is still valid token */
				cc=JSONLEX_CC_TABLE[charin];
				if (!(cc&JSONLEX_CC_ATTR_IDCHAR_CONT)) {
					/* character is not valid identifier character */
					JSONLEX_UNGETC(charin);
					break;
				}
			}

			return jsonlex_identifier;
		case JSONLEX_CC_NUMERIC:
			/* introduce numeric value */
			/* loop is optimized for recognizing integers */
			for (;;) {
				JSONLEX_BUFFER_BYTE(charin);
				charin=JSONLEX_GETC();
				if (charin<0) return jsonlex_integer;	/* eof while in int parse state */
				cc=JSONLEX_CC_TABLE[charin];
				if (!(cc&JSONLEX_CC_ATTR_DIGIT)) break; /* non-integer digit */
			}

			/* fall-through loop1 on first non digit (excluding leading minus) */
			if (!(cc&JSONLEX_CC_ATTR_INT_SEP)) {
				/* Not an "integer separator" indicating parse as numeric */
				JSONLEX_UNGETC(charin);
				return jsonlex_integer;
			}

			/* if charin is a decimal point, accumulate the fractional bits */
			if (charin=='.') {
				for (;;) {
					JSONLEX_BUFFER_BYTE(charin);
					charin=JSONLEX_GETC();
					if (charin<0) {
						/* this lets a numeric with a trailing decimal point
						 * through, but let higher level logic deal with that
						 * if needed
						 */
						return jsonlex_numeric;
					}
					cc=JSONLEX_CC_TABLE[charin];
					if (!(cc&JSONLEX_CC_ATTR_DIGIT)) break;	/* non-digit */
				}
			}

			/* fall-through loop2 on first non-digit after decimal point */
			if (charin!='e' && charin!='E') {
				/* no exponent - just numeric */
				JSONLEX_UNGETC(charin);
				return jsonlex_numeric;
			}

			/* final loop - collect exponent */
			charin=JSONLEX_GETC();
			if (charin<0) {
				/* technically lets an unadorned E through */
				return jsonlex_numeric;
			}

			/* normalize the exponent char to uppercase 'E' */
			cc=JSONLEX_CC_TABLE[charin];
			if ((cc & JSONLEX_CC_CLASSMASK)!=JSONLEX_CC_NUMERIC) {
				/* non-numeric following exponent - end of number */
				JSONLEX_UNGETC(charin);
				return jsonlex_numeric;
			}
			JSONLEX_BUFFER_BYTE('E');
			for (;;) {
				JSONLEX_BUFFER_BYTE(charin);
				charin=JSONLEX_GETC();
				if (charin<0) {
					/* eof as numeric */
					return jsonlex_numeric;
				}
				cc=JSONLEX_CC_TABLE[charin];
				if (!(cc&JSONLEX_CC_ATTR_DIGIT)) {
					/* char is not part of numeric */
					JSONLEX_UNGETC(charin);
					return jsonlex_numeric;
				}
			}

			break;
		case JSONLEX_CC_QUOTE:
			/* introduce string value */
			quotechar=charin;
			for (;;) {
				charin=JSONLEX_GETC();

				/* detect control char or eof in one go */
				if (charin<32) return jsonlex_unknown;

				/* string termination */
				if (charin==quotechar) return jsonlex_string;

				/* escape */
				if (charin=='\\') {
					/* process escape */
					continue;
				}

				/* default case - copy char */
				JSONLEX_BUFFER_BYTE(charin);
			}
			break;

		default:
			return jsonlex_unknown;
		}
	}
}
