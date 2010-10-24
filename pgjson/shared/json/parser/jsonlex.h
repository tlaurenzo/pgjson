#ifndef __JSONLEX_H__
#define __JSONLEX_H__

/**
 * JSONLEX_STATICBUFFER_SIZE
 * If defined, then a static initial lex buffer will be
 * defined.  If the capacity exceeds this static buffer
 * then dynamic allocation will be done.
 *
 * Defaults to 256
 */
#ifndef JSONLEX_STATICBUFFER_SIZE
#define JSONLEX_STATICBUFFER_SIZE 256
#endif

/**
 * JSONLEX_TRACK_POSITION
 * Define as 0 or 1 to indicate whether input position should be tracked.
 * If 1, then start_line, start_col, end_line, end_col will be maintained
 *
 * Default to 1
 */
#ifndef JSONLEX_TRACK_POSITION
#define JSONLEX_TRACK_POSITION 1
#endif

/**
 * JSONLEX_EXTRA_DECL
 * Macro is expanded within the jsonlex_state_t structure
 * to add additional user defined declarations.
 *
 * Defaults to empty definition
 */
#ifndef JSONLEX_EXTRA_DECL
#define JSONLEX_EXTRA_DECL
#endif

/**
 * JSON_DECLP
 * Prefix to include on function declarations.
 * Defaults to static
 */
#ifndef JSON_FDECLP
#define JSON_FDECLP static
#endif

/**
 * JSON_DDECLP
 * Prefix to include on data definitions
 * Defaults to static
 */
#ifndef JSON_DDECLP
#define JSON_DDECLP static
#endif

/**
 * JSON_FWDDDECLP
 * Prefix to add to forward data definitions.
 * Defaults to JSON_DDECLP
 */
#ifndef JSON_FWDDDECLP
#define JSON_FWDDDECLP JSON_DDECLP
#endif

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

typedef enum {
	/* Terminal character tokens */
	jsonlex_lbrace=0x01,
	jsonlex_rbrace=0x02,
	jsonlex_colon=0x03,
	jsonlex_comma=0x04,
	jsonlex_lbracket=0x05,
	jsonlex_rbracket=0x06,
	jsonlex_lparen=0x07,
	jsonlex_rparen=0x08,

	/* Non-terminal tokens */
	jsonlex_identifier=0x10,	/* includes null, true, false, undefined */
	jsonlex_integer=0x11,		/* string of digits, optionally preceeded by minus */
	jsonlex_numeric=0x12,		/* full numeric, with a decimal point or exponent */
	jsonlex_string=0x13,

	/* pseudo tokens */
	jsonlex_unknown=0xfe,
	jsonlex_eof=0xff
} jsonlex_token_t;

typedef struct {
	/* holds token extra data.  initially null */
	uint8_t *buffer;
	size_t   buffer_capacity;
	size_t   buffer_pos;
	#if JSONLEX_STATICBUFFER_SIZE > 0
	uint8_t  buffer_static[JSONLEX_STATICBUFFER_SIZE];
	#endif

	/* position */
	#if JSONLEX_TRACK_POSITION
	int start_line;
	int start_col;
	int end_line;
	int end_col;
	#endif

	/* additional user-defined declarations */
	JSONLEX_EXTRA_DECL
} jsonlex_state_t, *jsonlex_state_arg;

/**
 * Type of the char class bitmask
 */
typedef uint16_t jsonlex_charclass_t;

/**
 * Forward declare the character classification table
 */
JSON_FWDDDECLP const jsonlex_charclass_t JSONLEX_CC_TABLE[256];

/**
 * JSONLEX_CC_CLASSMASK
 * Defines a 4bit mask to extract the primary character classification
 * in the default lex state.
 */
#define JSONLEX_CC_CLASSMASK	((jsonlex_charclass_t)0x000f)
#define JSONLEX_CC_WHITE		((jsonlex_charclass_t)0x1)
#define JSONLEX_CC_TERMCHAR		((jsonlex_charclass_t)0x2)
#define JSONLEX_CC_IDCHAR		((jsonlex_charclass_t)0x3)
#define JSONLEX_CC_NUMERIC		((jsonlex_charclass_t)0x4)
#define JSONLEX_CC_QUOTE		((jsonlex_charclass_t)0x5)

/**
 * For character class JSONLEX_CC_TERMCHAR, access the
 * nibble containing the actual terminal character token
 * value with
 * 	token=(cc>>JSONLEX_CC_TERMCHAR_SHIFT)&JSONLEX_CC_TERMCHAR_MASK
 *
 */
#define JSONLEX_CC_TERMCHAR_MASK ((jsonlex_charclass_t)0xf)
#define JSONLEX_CC_TERMCHAR_SHIFT 4
#define JSONLEX_MAKE_TERMCHAR(token) (JSONLEX_CC_TERMCHAR | (jsonlex_charclass_t)token << JSONLEX_CC_TERMCHAR_SHIFT)

/* Additional character class attribute bits (upper byte) */

/**
 * JSONLEX_CC_ATTR_IDCHAR_CONT
 * Valid continuation character for an identifier
 */
#define JSONLEX_CC_ATTR_IDCHAR_CONT		((jsonlex_charclass_t)0x100)
#define JSONLEX_CC_ATTR_DIGIT			((jsonlex_charclass_t)0x200)
#define JSONLEX_CC_ATTR_INT_SEP			((jsonlex_charclass_t)0x400)

#endif
