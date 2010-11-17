/* jsonpath.tab.g
   Parser for json path expressions
*/

%name-prefix="yyjsonpath"
%pure-parser
%defines
%error-verbose
%parse-param { jsonpath_parsestate_t *context }

%lex-param { void* scanner }

%{
#include "util/setup.h"
#include "json/jsonaccess.h"
#include "json/jsonpath.h"
%}

%token <stringval> STRING
%token <stringval> IDENTIFIER
%token <stringval> NUMERIC
%token <stringval> LENGTH
%token LBRACKET
%token RBRACKET
%token DOT


%token UNRECOGNIZED

%union {
   jsonstring_t stringval;
}

%{
/* Redefine some things for the reentrant parser.  Note that the %union decl above
   forces early definition of YYSTYPE/YYLTYPE, allowing this to compile */
int yyjsonpathlex(YYSTYPE* lvalp, void* scanner);
void yyjsonpatherror(jsonpath_parsestate_t *context, const char* err);

#define scanner context->scanner
%}

%%
/* Rules */

stream
      : pathlist
      | /* nothing */
      ;
      
pathlist
      : pathitem pathtail
      ;

pathtail
      : DOT pathlist pathtail
      | bracketitem pathtail
      | /* nothing */
      ;
      
pathitem
      : IDENTIFIER   { jsonpath_append(context->bufferout, JSONPATHTYPE_IDENTIFIER, $1.bytes, $1.length); }
      | LENGTH       { jsonpath_append(context->bufferout, JSONPATHTYPE_LENGTH, $1.bytes, $1.length); }
      | bracketitem
      ;
     
bracketitem
      : LBRACKET bracketexpr RBRACKET
      ;
      
bracketexpr
      : STRING   { jsonpath_append(context->bufferout, JSONPATHTYPE_STRING, $1.bytes, $1.length); }
      | NUMERIC  { jsonpath_append(context->bufferout, JSONPATHTYPE_NUMERIC, $1.bytes, $1.length); }
      ;



%%

