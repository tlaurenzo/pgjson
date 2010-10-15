/* json.g
   Simple bison grammar for json
*/
%name-prefix="yyjson"
%pure-parser
%locations
%defines
%error-verbose
%parse-param { struct jsonparser_t *context }
%parse-param { jsonvisitor_t *visitor }

%lex-param { void* scanner }

%{
#include "util/setup.h"
#include "json/jsonparser.h"
#include "json/jsonaccess.h"
%}

/* Declare Tokens */
%token LBRACE RBRACE
%token COLON COMMA
%token LBRACKET RBRACKET
%token <boolval> BOOL_LITERAL 
%token NULL_LITERAL UNDEFINED_LITERAL
%token <stringval> IDENTIFIER
%token <stringval> STRING
%token <intval> INTEGER
%token <bigintval> BIGINT
%token <floatval> FLOAT
%token UNRECOGNIZED

%type <stringval> field_name; 

%union {
	/* Number values */
	int 	intval;		/* token INTEGER */
	int64_t	bigintval;	/* token BIGINT */
	double 	floatval;	/* token FLOAT */
	bool 	boolval;	/* token BOOL_LITERAL 1=True, 0=False */
	
	/* String/Buffer Values: STRING, IDENTIFIER */
	jsonstring_t stringval;
}


%{

/* Redefine some things for the reentrant parser.  Note that the %union decl above
   forces early definition of YYSTYPE/YYLTYPE, allowing this to compile */
int yyjsonlex(YYSTYPE* lvalp, YYLTYPE* llocp, void* scanner);
static void yyjsonerror(YYLTYPE* locp, struct jsonparser_t* context, jsonvisitor_t *visitor, const char* err);

#define scanner context->scanner
%}

%%
/* Rules */

document
		:	object
		;

object	
		:	LBRACE 	{ visitor->vft->start_object(visitor); }
				object_elements 
			RBRACE	{ visitor->vft->end_object(visitor); }
		|	LBRACE RBRACE	{ visitor->vft->add_empty_object(visitor); }
		;

object_elements
		:	object_element	
		|	object_element 
			COMMA
			object_elements
		;
		
object_element
		:	field_name	{ visitor->vft->push_label(visitor, &$1); }
			COLON
			field_value
		;

/* field_name always returns a stringval (special case) */		
field_name
		:	IDENTIFIER	{ $$=$1; }
		|	STRING		{ $$=$1; }
		;
		
field_value
		:	STRING				{ visitor->vft->add_string(visitor, &$1); }
		|	INTEGER				{ visitor->vft->add_int32(visitor, $1); }
		| 	BIGINT				{ visitor->vft->add_int64(visitor, $1); }
		|	FLOAT				{ visitor->vft->add_double(visitor, $1); }
		|	BOOL_LITERAL		{ visitor->vft->add_bool(visitor, $1); }
		|	NULL_LITERAL		{ visitor->vft->add_null(visitor); }
		|	UNDEFINED_LITERAL	{ visitor->vft->add_undefined(visitor); }
		|	object				/* Will be added by the object non-terminal */
		|	array				/* Will be added by the array non-terminal */
		;
		
array
		:	LBRACKET 			{ visitor->vft->start_array(visitor); }
			array_elements 		/* Elements will be added by later terminal */
			RBRACKET			{ visitor->vft->end_array(visitor); }
		|	LBRACKET RBRACKET	{ visitor->vft->add_empty_array(visitor); }
		;
		
array_elements
		:	field_value
		|	field_value
			COMMA 
			array_elements
		;
%%


static void yyjsonerror(YYLTYPE* locp, struct jsonparser_t* context, jsonvisitor_t *visitor, const char* err) {
	size_t max_size=strlen(err)+128;
	context->has_error=true;
	if (context->error_msg) free(context->error_msg);
	context->error_msg=malloc(max_size);
	snprintf(context->error_msg, max_size, "JSON syntax error(Line %d, Col %d): %s", locp->first_line, locp->first_column+1, err);
}
