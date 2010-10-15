%option noyywrap nodefault yylineno
%option prefix="yyjson"
%option reentrant
%option bison-bridge
%option bison-locations
%{
#include "util/setup.h"
#include <errno.h>
#include <math.h>

#include "json/jsonparser.h"
#include "jsonparser.tab.h"

#define YY_EXTRA_TYPE struct jsonparser_t*
#define YY_USER_ACTION yylloc->first_line = yylineno;

#define YY_INPUT(buf, result, max_size) { \
	result=yyextra->read_input(yyextra, buf, max_size); \
}

inline double convert_float(YYSTYPE *target, char *text);
inline int convert_integer(YYSTYPE* target, char* text, int base);

/**
 * Attempt to decode a suspected integer into the smallest type
 * that can parse using the given base.  Set the appropriate
 * value in target and return the token type parsed.
 */
inline int convert_integer(YYSTYPE* target, char* text, int base)
{
	errno=0;
	target->intval=strtol(text, 0, base);
	if (errno==0) {
		return INTEGER;
	}
	
	errno=0;
	target->bigintval=strtoll(text, 0, base);
	if (errno==0) {
		return BIGINT;
	}
	
	if (base==10) {
		errno=0;
		target->floatval=strtod(text, 0);
		if (errno==0) {
			return FLOAT;
		}
	}
	
	return UNRECOGNIZED;
}

inline double convert_float(YYSTYPE *target, char *text)
{
	errno=0;
	target->floatval=strtod(text, 0);
	if (errno==0) return FLOAT;
	else return UNRECOGNIZED;
}

inline uint32_t convert_octal_escape(char *t)
{
	return strtoul(t+1, 0, 8);
}

inline uint32_t convert_hex_escape(char *t)
{
	return strtoul(t+2, 0, 16);
}

inline uint32_t convert_uni_escape(char *t)
{
	return strtoul(t+2, 0, 16);
}

%}

%x STRINGLITERAL

%%
	/* Literals */
"{"		{ return LBRACE; }
"}"		{ return RBRACE; }
":"		{ return COLON; }
"["		{ return LBRACKET; }
"]" 		{ return RBRACKET; }
","		{ return COMMA; }
"true"	{ yylval->boolval=1; return BOOL_LITERAL; }
"false"	{ yylval->boolval=0; return BOOL_LITERAL; }
"null"	{ return NULL_LITERAL; }
"undefined" 	{ return UNDEFINED_LITERAL; }

	/* Strings */
"\""				{ BEGIN STRINGLITERAL; yyextra->string_mode='"';  stringwriter_clear(&yyextra->string_accum); }
"\'"				{ BEGIN STRINGLITERAL; yyextra->string_mode='\''; stringwriter_clear(&yyextra->string_accum); }

	/* STRINGLITERAL state */
<STRINGLITERAL>[^\"\'\\]+				{ stringwriter_append(&yyextra->string_accum, yytext, yyleng); }
<STRINGLITERAL>\\[0-7]{3}			{ stringwriter_append_codepoint(&yyextra->string_accum, convert_octal_escape(yytext)); }
<STRINGLITERAL>\\x[0-9a-fA-F]{2}	{ stringwriter_append_codepoint(&yyextra->string_accum, convert_hex_escape(yytext)); }
<STRINGLITERAL>\\u[0-9a-fA-F]{4}	{ stringwriter_append_codepoint(&yyextra->string_accum, convert_uni_escape(yytext)); }
<STRINGLITERAL>\\. 					{ stringwriter_append_escape_char(&yyextra->string_accum, yytext[1]); }
<STRINGLITERAL>[\"\']				{
										if (yyextra->string_mode==yytext[0]) {
											/* string end */
											BEGIN INITIAL; 
											yylval->stringval.length=yyextra->string_accum.pos; 
											yylval->stringval.bytes=yyextra->string_accum.string;
											return STRING; 
										} else {
											/* literal char */
											stringwriter_append_byte(&yyextra->string_accum, yytext[0]);
										}
									}
<STRINGLITERAL>.					{ BEGIN INITIAL; return UNRECOGNIZED; }


	/* Numbers */
(\-)?0[0-7]+	{ return convert_integer(yylval, yytext, 8); }
(\-)?[0-9]+	{ return convert_integer(yylval, yytext, 10); }
(\-)?0[xX][0-9a-fA-F]+	{ return convert_integer(yylval, yytext, 16); }
(\-)?[0-9]*\.[0-9]+(e\-?[0-9]+)?	{ return convert_float(yylval, yytext); }
(\-)?[0-9]+\.[0-9]*(e\-?[0-9]+)?	{ return convert_float(yylval, yytext);  }
"NaN"			{ yylval->floatval=NAN; return FLOAT; }
"Infinity"		{ yylval->floatval=INFINITY; return FLOAT; }
"-Infinity"		{ yylval->floatval=-INFINITY; return FLOAT; }

	/* Identifier */
[a-zA-Z_][0-9a-zA-Z_]*	{
	/** Make copy in accumulator **/
	stringwriter_clear(&yyextra->string_accum);
	stringwriter_append(&yyextra->string_accum, yytext, yyleng);
	
	/** And then set the yylval **/
	yylval->stringval.length=yyleng;
	yylval->stringval.bytes=yyextra->string_accum.string;
	
	return IDENTIFIER; 
	}

	/* Whitespace and unrecognized */
[ \t\r\n]
.		{ fprintf(stderr, "Unrecognized character: '%s'\n", yytext); 
			return UNRECOGNIZED; }

%%

/* Provide definitions for init and destroy methods on jsonparser_t */
void jsonparser_init(struct jsonparser_t *parser)
{
	/* Init memory */
	memset(parser, 0, sizeof(struct jsonparser_t));
	jsonparser_inputdestroy(parser);
	
	stringwriter_init(&parser->string_accum, 256);
	parser->has_error=false;
	parser->error_msg=0;
	
	/* Init scanner */
	yylex_init(&parser->scanner);
	yyset_extra(parser, parser->scanner);
}

void jsonparser_destroy(struct jsonparser_t *parser)
{
	stringwriter_destroy(&parser->string_accum);
	yylex_destroy(parser->scanner);
	jsonparser_inputdestroy(parser);
	if (parser->error_msg) free(parser->error_msg);
}

