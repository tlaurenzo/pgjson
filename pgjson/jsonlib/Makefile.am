AM_CPPFLAGS = $(POSTGRESQL_SERVER_CFLAGS)

bin_PROGRAMS = testparsejson

testparsejson_CFLAGS = -O3
testparsejson_SOURCES = \
	testparsejson.c \
	json_transcode_to_json.c \
	json_transcode_json_to_binary.c \
	dynbuffer.c \
	jsonlex.tab.c \
	jsonutil.c \
	hexdump.c

