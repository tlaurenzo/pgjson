/**
 * jsonlex.inc.c
 *
 * Provide the guts of a JSON parser as an includable C file.
 */
#include "jsonlex.h"


/**
 * JSONLEX_GETC
 * Define the macro that reads a character from the input stream.
 * The character should be returned as an unsigned char converted to an
 * int.  -1 should be returned on EOF.
 *
 * This macro has access to the lexstate variable (jsonlex_state_arg)
 */
#ifndef JSONLEX_GETC
#error JSONLEX_GETC must be defined prior to inclusion
#endif
/**
 * JSONLEX_UNGETC
 * Define the macro that ungets a character from the input stream.
 * Note that this will only ever be called with the same character
 * as returned by the previous call to JSONLEX_GETC.  This allows
 * read-only buffers to ignore the character and just decrement
 * a position.
 *
 * This macro has access to the lexstate variable (jsonlex_state_arg)
 */
#ifndef JSONLEX_UNGETC
#error JSONLEX_UNGETC must be defined prior to inclusion
#endif


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
	case jsonlex_illegal_string: return "<BAD ESCAPE SEQUENCE IN STRING>";
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

#ifndef JSONLEX_DISABLE_IO
JSON_FDECLP void jsonlex_init_io(jsonlex_state_arg lexstate, uint8_t *source, size_t len)
{
	jsonlex_init(lexstate);
	lexstate->source=source;
	lexstate->sourcelimit=source+len;
}
#endif

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

JSON_FDECLP bool jsonlex_unicode_escape(jsonlex_state_arg lexstate)
{
	int i;
	uint32_t nibble;
	uint32_t codepoint=0;
	int digit;

	/* decode digits into codepoint */
	for (i=0; i<4; i++) {
		digit=JSONLEX_GETC();
		if (digit>='0' && digit<='9') {
			nibble=digit - '0';
		} else if (digit>='a' && digit <= 'f') {
			nibble=digit - 'a';
		} else if (digit>='A' && digit <= 'F') {
			nibble=digit - 'A';
		} else {
			return false;
		}

		codepoint = (codepoint << 4) | nibble;
	}

	/* output codepoint as utf-8 */
	if (codepoint>=0 && codepoint<=0x7f) {
		JSONLEX_BUFFER_BYTE(codepoint);
	} else if (codepoint<=0x7ff) {
		JSONLEX_BUFFER_BYTE(0xc0 | ((codepoint>>6)&0x1f));
		JSONLEX_BUFFER_BYTE(0x80 | (codepoint&0x3f));
	} else if (codepoint<=0xffff) {
		JSONLEX_BUFFER_BYTE(0xe0 | ((codepoint>>12)&0xff));
		JSONLEX_BUFFER_BYTE(0x80 | ((codepoint>>6)&0x3f));
		JSONLEX_BUFFER_BYTE(0x80 | ((codepoint&0x3f)));
	} else if (codepoint<=0x10ffff) {
		JSONLEX_BUFFER_BYTE(0xf0 | ((codepoint>>18)&0x7));
		JSONLEX_BUFFER_BYTE(0x80 | ((codepoint>>12)&0x3f));
		JSONLEX_BUFFER_BYTE(0x80 | ((codepoint>>6)&0x3f));
		JSONLEX_BUFFER_BYTE(0x80 | ((codepoint&0x3f)));
	} else {
		/* Unicode replacement char uFFFD as UTF-8 */
		/*
		JSONLEX_BUFFER_BYTE(0xef);
		JSONLEX_BUFFER_BYTE(0xbf);
		JSONLEX_BUFFER_BYTE(0xbd);
		*/
		return false;
	}


	return true;
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
				if (charin<32) return jsonlex_illegal_string;

				/* string termination */
				if (charin==quotechar) return jsonlex_string;

				/* escape */
				if (charin=='\\') {
					/* process escape */
					charin=JSONLEX_GETC();
					if (charin=='u') {
						/* unicode escape */
						if (!jsonlex_unicode_escape(lexstate)) {
							return jsonlex_illegal_string;
						}
					} else {
						/* escape char */
						switch (charin) {
						case '\'': charin='\''; break;
						case '"':  charin='"';  break;
						case '\\': charin='\\'; break;
						case '/':  charin='/'; break;
						case 'n': charin='\n'; break;
						case 'r': charin='\r'; break;
						case 't': charin='\t'; break;
						case 'b': charin='\b'; break;
						case 'f': charin='\f'; break;
						case '0': charin=0; break;
						default:
							return jsonlex_illegal_string;
						}
						JSONLEX_BUFFER_BYTE(charin);
					}
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
