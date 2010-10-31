AM_CPPFLAGS = $(POSTGRESQL_SERVER_CFLAGS)

bin_PROGRAMS = testparsejson benchmark

testparsejson_CFLAGS = -O2
testparsejson_SOURCES = \
	testparsejson.c \
	json_transcode_to_json.c \
	json_transcode_json_to_binary.c \
	json_validate_json.c \
	dynbuffer.c \
	jsonlex.tab.c \
	jsonutil.c \
	hexdump.c

benchmark_CFLAGS = -O2
benchmark_SOURCES = \
	benchmark.c \
	json_transcode_to_json.c \
	json_transcode_json_to_binary.c \
	json_validate_json.c \
	dynbuffer.c \
	jsonlex.tab.c \
	jsonutil.c \
	hexdump.c
