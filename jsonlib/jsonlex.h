/**
 * jsonlex.h
 *
 * A JSON lexer, configurable via preprocessor definitions.  This file, along
 * with jsonlex.inc.c and jsonlex.tab.c provide the "guts" of a lexer that
 * is configurable via pre-processor definitions.  By default, it should
 * be included statically in an implementation module.  Therefore, everything
 * is declared statically.
 *
 * If using multiple lexers, it would be beneficial to link the JSONLEX_CC_TABLE
 * separately as an extern to save 1kb per instance.
 */
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
 * JSONLEX_DISABLE_IO
 * Define this macro to bring your own IO.  In this case, you must define JSONLEX_IO_DECL
 * to expand as necessary into the struct and JSONLEX_GETC and JSONLEX_UNGETC.  The standard
 * IO is a buffer of source and sourcelimit and macros to read characters from it.
 */
#ifndef JSONLEX_DISABLE_IO
#define JSONLEX_IO_DECL \
	uint8_t *source; \
	uint8_t *sourcelimit;
#ifndef JSONLEX_GETC
#define JSONLEX_GETC() (lexstate->source<lexstate->sourcelimit ? *(lexstate->source++) : -1)
#endif

#ifndef JSONLEX_UNGETC
#define JSONLEX_UNGETC(c) (lexstate->source-=1)
#endif
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
	jsonlex_illegal_string=0xfd,
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

	/* include IO declarations */
	#ifdef JSONLEX_IO_DECL
	JSONLEX_IO_DECL
	#endif
} jsonlex_state_t, *jsonlex_state_arg;

/**
 * Type of the char class bitmask
 */
typedef uint16_t jsonlex_charclass_t;

/**
 * Forward declare the character classification table
 */
extern const jsonlex_charclass_t JSONLEX_CC_TABLE[256];

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

/**** Function forwards ***/
JSON_FDECLP jsonlex_token_t jsonlex_next_token(jsonlex_state_arg lexstate);
JSON_FDECLP void jsonlex_init(jsonlex_state_arg lexstate);
#ifndef JSONLEX_DISABLE_IO
	JSON_FDECLP void jsonlex_init_io(jsonlex_state_arg lexstate, uint8_t *source, size_t len);
#endif
JSON_FDECLP void jsonlex_destroy(jsonlex_state_arg lexstate);


#endif
