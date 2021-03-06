#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "hexdump.h"
#include "dynbuffer.h"
#include "jsonutil.h"

clock_t starttime;
int iterations=0;
uint8_t *source;
size_t sourcelen;

void start_timer()
{
	starttime=clock();
}

void print_stats()
{
	clock_t endtime=clock();
	clock_t totaltime=endtime-starttime;
	double avgMicros=totaltime / (double)iterations;
	double times=totaltime / (double)CLOCKS_PER_SEC;

	printf("Total runtime %f.  Avg %fus\n", times, avgMicros);
}

void test_json_to_json()
{
	dynbuffer_t dest=dynbuffer_init();
	dynbuffer_ensure(&dest, sourcelen*2);

	if (!json_transcode_json_to_json(source, sourcelen, &dest)) {
		printf("Error transcoding\n");
		exit(2);
	}
	dynbuffer_destroy(&dest);
}

void test_json_to_binary()
{
	dynbuffer_t dest=dynbuffer_init();
	dynbuffer_ensure(&dest, sourcelen*2);

	if (!json_transcode_json_to_binary(source, sourcelen, &dest)) {
		printf("Error transcoding\n");
		exit(2);
	}
	dynbuffer_destroy(&dest);
}

void dummy()
{
	void *target=malloc(sourcelen);
	memcpy(target, source, sourcelen);
	free(target);
}

void test_validate()
{
	if (!json_validate_json(source, sourcelen)) {
		printf("Validation failed\n");
		exit(3);
	}
}

void setup_binary_to_json()
{
	dynbuffer_t bin=dynbuffer_init();
	if (!json_transcode_json_to_binary(source, sourcelen, &bin)) {
		printf("Could not parse to binary\n");
		exit(2);
	}
	source=bin.contents;
	sourcelen=bin.pos;
}

void test_json_from_binary()
{
	dynbuffer_t dest=dynbuffer_init();
	dynbuffer_ensure(&dest, sourcelen*2);

	if (!json_transcode_binary_to_json(source, sourcelen, &dest)) {
		printf("Error transcoding\n");
		exit(2);
	}
	dynbuffer_destroy(&dest);
}

int main(int argc, char **argv)
{
	dynbuffer_t sourcebuf=dynbuffer_init();
	dynbuffer_t dest=dynbuffer_init();
	void (*testproc)();
	char *testname;

	if (argc<3) {
		printf("Expected test and input file\n");
		return 3;
	}

	testname=argv[2];

	stdin=fopen(argv[1], "r");
	if (!stdin) {
		printf("Error opening file\n");
		return 3;
	}

	dynbuffer_read_file(&sourcebuf, stdin);
	source=sourcebuf.contents;
	sourcelen=sourcebuf.pos;

	if (strcmp("tojson", testname)==0) testproc=test_json_to_json;
	else if (strcmp("tobinary", testname)==0) testproc=test_json_to_binary;
	else if (strcmp("dummy", testname)==0) testproc=dummy;
	else if (strcmp("validate", testname)==0) testproc=test_validate;
	else if (strcmp("frombinary", testname)==0) {
		setup_binary_to_json();
		testproc=test_json_from_binary;
	}
	else {
		printf("Unrecognized test name\n");
		return 4;
	}

	testproc();

	start_timer();
	for (iterations=0; iterations<100000; iterations++) {
		testproc();
	}
	print_stats();
	return 0;
}
