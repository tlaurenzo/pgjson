%option noyywrap nodefault yylineno
%option prefix="yyjsonpath"
%option reentrant
%option bison-bridge
%{
#include "util/setup.h"
#include "json/jsonpath.h"
#include "jsonpath.tab.h"

/* forward define - this is in jsonpath.c */
int yyjsonpath_read(jsonpath_parsestate_t * parse_state, char *buf, size_t max_size);

#define YY_EXTRA_TYPE jsonpath_parsestate_t*
#define YY_INPUT(buf, result, max_size) { \
   result=yyjsonpath_read(yyextra, buf, max_size); \
}

static inline uint32_t convert_octal_escape(char *t)
{
	return strtoul(t+1, 0, 8);
}

static inline uint32_t convert_hex_escape(char *t)
{
	return strtoul(t+2, 0, 16);
}

static inline uint32_t convert_uni_escape(char *t)
{
	return strtoul(t+2, 0, 16);
}

%}

%x STRINGLITERAL

%%
"["		{ return LBRACKET; }
"]" 		{ return RBRACKET; }
"."      { return DOT; }

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

[0-9]+                        { yylval->stringval.length=yyleng; yylval->stringval.bytes=yytext; return NUMERIC; }
[a-zA-Z_\$][0-9a-zA-Z_\$]*	   { yylval->stringval.length=yyleng; yylval->stringval.bytes=yytext; return IDENTIFIER; }

   /* Whitespace */
[ \t\r\n]                     { /* nothing */ }
.                             { /*fprintf(stderr, "Unrecognized character: '%s'\n", yytext);*/ 
			                        return UNRECOGNIZED; }

%%


