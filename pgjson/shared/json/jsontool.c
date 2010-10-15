#include "util/setup.h"
#include "util/hexdump.h"
#include "json/jsonparser.h"
#include "json/bsonparser.h"
#include "json/jsonoutputter.h"

/* Program options */
enum format_t {
	NONE,
	JSON,
	BSON
};

enum format_t output_format=JSON;
enum format_t input_format=JSON;
bool print_hex=false;

bool output_format_explicit=false, input_format_explicit=false;
bool output_debug=false;

FILE *input_file=NULL;
FILE *output_file=NULL;
FILE *compare_file=NULL;
bool json_pretty_print=false;

/* State */
jsonoutputter_t outputter;

void print_syntax()
{
	fprintf(stderr, "jsontool\n");
	fprintf(stderr, "Syntax:\n");
	fprintf(stderr, "  jsontool [-ot none|json|bson|bsonhex] [-it json|bson] [-pretty]\n");
	fprintf(stderr, "           [-c comparefile] [-o outfile] [-debug] [-hex] [inputfile]\n");
	fprintf(stderr, "Reads JSON from an input file (or stdin) and writes it to stdout.\n");
	fprintf(stderr, "Input and output types are controllable via the -it and -ot options\n");
	fprintf(stderr, "or by the extension of the file (json,bson).\n\n");
	fprintf(stderr, "For json output types, the -pretty parameter enables pretty printing.\n");
	fprintf(stderr, "If a compare file is set, then the results will be compared to this\n");
	fprintf(stderr, "file and the exit status set appropriately.  By default in compare\n");
	fprintf(stderr, "no output is written.\n");
}

int process_json_input()
{
	jsonparser_t parser;

	jsonparser_init(&parser);
	jsonparser_inputfile(&parser, input_file);

	jsonparser_parse(&parser, output_debug ? jsonoutputter_getdebugvisitor(&outputter): jsonoutputter_getvisitor(&outputter));

	return 0;
}

int process_bson_input()
{
	stringwriter_t buffer;
	stringwriter_init(&buffer, 8192);
	if (!stringwriter_slurp_file(&buffer, input_file)) {
		fprintf(stderr, "Error reading input file\n");
		return 3;
	}

	if (bsonparser_parse(buffer.string, buffer.pos,
			output_debug ? jsonoutputter_getdebugvisitor(&outputter): jsonoutputter_getvisitor(&outputter))<0) {
		fprintf(stderr, "Error parsing bson document\n");
		return 4;
	}

	return 0;
}


int handle_output()
{
	uint8_t *buffer;
	size_t len;
	const char *error_msg;

	if (jsonoutputter_has_error(&outputter, &error_msg)) {
		fprintf(stderr, "Serialization error: %s\n", error_msg);
		return 4;
	}

	jsonoutputter_get_buffer(&outputter, &buffer, &len);
	if (print_hex) {
		if (!hexdump(output_file, buffer, len)) return 5;
	} else {
		if (fwrite(buffer, len, 1, output_file)!=1) {
			fprintf(stderr, "Output error\n");
			return 3;
		}
	}

	return 0;
}


bool parse_format(char *name, enum format_t *format)
{
	if (!name) return false;

	if (strcmp(name, "json")==0) {
		*format=JSON;
		return true;
	} else if (strcmp(name, "bson")==0) {
		*format=BSON;
		return true;
	} else if (strcmp(name, "none")==0) {
		*format=NONE;
		return true;
	}

	return false;
}

bool open_file(char *filename, char *mode, FILE** file, enum format_t *format, bool format_is_explicit)
{
	char *ext;

	if (!filename) return false;

	if (!format_is_explicit && format) {
		ext=strrchr(filename, '.');
		ext++;
		if (!parse_format(ext, format)) {
			fprintf(stderr, "Could not detect file type for %s\n", filename);
			return false;
		}
	}

	*file=fopen(filename, mode);
	if (!*file) return false;

	return true;
}

int main(int argc, char** argv) {
	int rc;

	if (argc==1) {
		print_syntax();
		return 1;
	}

	output_file=stdout;
	input_file=stdin;

	/* Parse arguments */
	argv++;
	while (*argv) {
		if (*argv[0]!='-') {
			/* Open input file */
			if (input_file && input_file!=stdin) {
				fprintf(stderr, "Multiple input files\n");
				return 1;
			}
			if (!open_file(*argv, "r", &input_file, &input_format, input_format_explicit)) {
				fprintf(stderr, "Could not open input file\n");
				return 1;
			}
		} else if (strcmp(*argv, "-ot")==0) {
			if (!parse_format(*(++argv), &output_format)) {
				fprintf(stderr, "Bad value for -ot argument\n");
				return 1;
			}
			output_format_explicit=true;
		} else if (strcmp(*argv, "-it")==0) {
			if (!parse_format(*(++argv), &input_format)) {
				fprintf(stderr, "Bad value for -it argument\n");
				return 1;
			}
			input_format_explicit=true;
		} else if (strcmp(*argv, "-o")==0) {
			if (!open_file(*(++argv), "w", &output_file, &output_format, output_format_explicit)) {
				fprintf(stderr, "Could not open output file\n");
				return 1;
			}
		} else if (strcmp(*argv, "-c")==0) {
			if (!open_file(*(++argv), "r", &compare_file, 0, false)) {
				fprintf(stderr, "Could not open compare file\n");
				return 1;
			}
			if (!output_format_explicit) {
				output_format=NONE;
				output_format_explicit=true;
			}
		} else if (strcmp(*argv, "-debug")==0) {
			output_debug=true;
		} else if (strcmp(*argv, "-pretty")==0) {
			json_pretty_print=true;
		} else if (strcmp(*argv, "-hex")==0) {
			print_hex=true;
		} else {
			fprintf(stderr, "Unrecognized option %s\n", *argv);
			return 2;
		}

		argv++;
	}

	/* Open the outputter */
	if (output_format==BSON) {
		jsonoutputter_open_bson_buffer(&outputter, 8192);
	} else if (output_format==JSON) {
		jsonoutputter_open_json_buffer(&outputter, 8192);
		if (json_pretty_print) {
			jsonoutputter_set_json_options(&outputter, true, 0);
		}
	} else {
		fprintf(stderr, "Cannot create outputter\n");
		return 2;
	}

	/* Branch based on input type */
	if (input_format==JSON) {
		rc=process_json_input();
	} else if (input_format==BSON) {
		rc=process_bson_input();
	} else {
		fprintf(stderr, "Unknown input type\n");
		rc=2;
	}

	if (rc==0) {
		rc=handle_output();
	}

	if (output_file==stdout) fprintf(output_file, "\n");
	fclose(output_file);
	return rc;
}

