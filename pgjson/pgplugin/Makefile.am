AM_CPPFLAGS = -I../include $(POSTGRESQL_SERVER_CFLAGS)

MODULE_NAME=libpgjson-$(PGJSON_MAJOR_VERSION).$(PGJSON_MINOR_VERSION).$(PGJSON_MICRO_VERSION).so
MODULE_PATH="`pwd`/$(MODULE_NAME)"

lib_LTLIBRARIES = libpgjson.la

libpgjson_la_LDFLAGS = -shared
libpgjson_la_LIBADD = ../shared/libjson.la
libpgjson_la_SOURCES = \
	pgjson.c \
	jsontype.c \
	jsonpathtype.c \
	pgutil.c

$(MODULE_NAME): libpgjson.la
	@cp .libs/"`egrep '^dlname' libpgjson.la | sed -e 's/^dlname=.//' -e 's/.$$//'`" $(MODULE_NAME)	
	@echo "Module path=$(MODULE_PATH)"

%.sql: %.sql.in
	sed -e "s,MODULE_PATHNAME,${MODULE_PATH},g" $< >$@
	
all-local: $(MODULE_NAME) pgjson.sql

clean-local:
	test -z $(MODULE_NAME) || rm -f $(MODULE_NAME)
	test -z pgjson.sql  || rm -f pgjson.sql
