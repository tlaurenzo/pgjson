#include "util/setup.h"
#include "util/hexdump.h"
#include "json/jsonparser.h"
#include "json/bsonparser.h"
#include "json/bsonserializer.h"
#include "json/jsonpath.h"
#include "json/jsonoutputter.h"

FILE *arg_input;
FILE *arg_output;
char  arg_input_format;  /* 'J' or 'B' */
char **arg_expressions;
int  arg_expression_count;
bool arg_debug=false;

bsonvalue_t bson_value;
stringwriter_t bsonbuffer=STRINGWRITER_INIT(8192);

int report_result(bsonvalue_t *value)
{
	jsonoutputter_t outputter;

	stringwriter_t outputbuffer=STRINGWRITER_INIT(8192);

	jsonoutputter_open_json_buffer(&outputter, &outputbuffer);
	bsonvalue_visit(value, jsonoutputter_getvisitor(&outputter));
	jsonoutputter_close(&outputter);

	fwrite(outputbuffer.string, outputbuffer.pos, 1, arg_output);
	fprintf(arg_output, "\n");

	stringwriter_destroy(&outputbuffer);

	return 0;
}

int process_expression(char *exprtext)
{
	int rc=0;
	stringwriter_t exprbin;
	stringwriter_t exprser;
	jsonpathiter_t iter;
	bsonvalue_t bson_result;

	if (arg_debug) fprintf(stderr, "Parsing expression: %s\n", exprtext);

	stringwriter_init(&exprbin, 512);
	if (!jsonpath_parse(&exprbin, exprtext, strlen(exprtext))) {
		fprintf(stderr, "Error parsing expression: %s\n", exprbin.string);
		stringwriter_destroy(&exprbin);
		return 4;
	}

	if (arg_debug) {
		/* dump the binary */
		fprintf(stderr, "Expression binary:\n");
		hexdump(stderr, exprbin.string, exprbin.pos);
		fprintf(stderr, "Expression text:\n");
		stringwriter_init(&exprser, 512);

		jsonpath_iter_begin(&iter, exprbin.string, exprbin.pos);
		if (jsonpath_serialize(&exprser, &iter)) {
			stringwriter_append_byte(&exprser, 0);
			fprintf(stderr, "\t%s\n", exprser.string);
		} else {
			fprintf(stderr, "ERROR: Could not serialize expression\n");
		}

		stringwriter_destroy(&exprser);
	}

	/* evaluate */
	jsonpath_iter_begin(&iter, exprbin.string, exprbin.pos);
	if (!jsonpath_evaluate(&bson_value, &iter, &bson_result)) {
		fprintf(stderr, "ERROR: Could not evaluate\n");
		rc=6;
	} else {
		rc=report_result(&bson_result);
	}

	stringwriter_destroy(&exprbin);
	return rc;
}

int process_input_json()
{
	jsonparser_t parser;
	bsonserializer_t serializer;

	/* initialize */
	jsonparser_init(&parser);
	jsonparser_inputfile(&parser, arg_input);
	bsonserializer_init(&serializer, &bsonbuffer);

	/* parse */
	jsonparser_parse(&parser, &serializer.jsonvisitor);
	if (parser.has_error || serializer.has_error) {
		fprintf(stderr, "Error parsing input\n");
		return 3;
	}

	/* dont deallocate bsonserializer because we're keeping its buffer */

	return 0;
}

int process_input_bson()
{
	if (!stringwriter_slurp_file(&bsonbuffer, arg_input)) return 3;
	return 0;
}

int process_input()
{
	int rc;
	if (arg_input_format=='J' || arg_input_format==' ') {
		/* Parse json */
		rc=process_input_json();
	} else if (arg_input_format=='B') {
		/* Parse bson */
		rc=process_input_bson();
	} else {
		return 3;
	}

	if (rc==0) {
		/* dump binary data */
		if (arg_debug) {
			fprintf(stderr, "Parsed Input:\n");
			hexdump(stderr, bsonbuffer.string, bsonbuffer.pos);
		}

		/* initialize bson value */
		if (!bsonvalue_load(&bson_value, bsonbuffer.string, bsonbuffer.pos, BSONMODE_ROOT)) {
			fprintf(stderr, "ERROR: bsonvalue_load could not interpret the parsed results\n");
			rc=5;
		}
	}

	return rc;
}

char parse_format(char *fmt)
{
	if (fmt[0]=='J' || fmt[0]=='j') return 'J';
	else if (fmt[0]=='B' || fmt[0]=='b') return 'B';
	else return ' ';
}

void print_syntax()
{
	fprintf(stderr, "syntax: jsonquery [-it json|bson] [-e expression] [-debug] [input file]\n");
}

int main(int argc, char **argv)
{
	char *scratch;
	int rc;
	int i;

	arg_input=stdin;
	arg_output=stdout;
	arg_input_format=' ';
	arg_expressions=malloc(sizeof(char**)*argc);
	arg_expression_count=0;

	if (argc==1) {
		print_syntax();
		return 1;
	}

	/* parse arguments */
	for (argv++; *argv; argv++) {
		if (strcmp(*argv, "-it")==0) {
			argv++;
			if (!*argv) {
				print_syntax();
				return 1;
			}
			arg_input_format=parse_format(*argv);
			if (arg_input_format==' ') {
				print_syntax();
				return 1;
			}
		} else if (strcmp(*argv, "-e")==0) {
			argv++;
			if (!*argv) {
				print_syntax();
				return 1;
			}
			arg_expressions[arg_expression_count++]=*argv;
		} else if (strcmp(*argv, "-debug")==0) {
			arg_debug=true;
		} else if (*argv[0]!='-') {
			if (arg_input!=stdin) {
				fprintf(stderr, "Multiple input files specified\n");
				print_syntax();
				return 1;
			}
			arg_input=fopen(*argv, "r");
			if (!arg_input) {
				fprintf(stderr, "Could not open input file %s\n", *argv);
				return 2;
			}

			if (arg_input_format==' ') {
				scratch=strrchr(*argv, '.');
				if (scratch) arg_input_format=parse_format(scratch+1);
			}
		} else {
			fprintf(stderr, "Unrecognized argument %s\n", *argv);
			return 1;
		}
	}

	/* read the input */
	rc=process_input();
	if (rc) {
		fprintf(stderr, "Error processing input\n");
		return rc;
	}

	/* process expressions */
	for (i=0; i<arg_expression_count; i++) {
		rc=process_expression(arg_expressions[i]);
		if (rc) return rc;
	}

	return 0;
}
