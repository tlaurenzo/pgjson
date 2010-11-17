MODULE_big = pgjson
DATA_built = pgjson.sql pgjson.dev.sql
OBJS = \
	jsonlib/json_transcode_to_json.o \
	jsonlib/json_transcode_json_to_binary.o \
	jsonlib/json_transcode_binary_to_json.o \
	jsonlib/json_validate_json.o \
	jsonlib/dynbuffer.o \
	jsonlib/jsonlex.tab.o \
	jsonlib/jsonutil.o \
	pgjson.o

PG_CPPFLAGS = -DJSON_USE_PALLOC -Wimplicit

MODULE_PATHNAME = $(shell pwd)/pgjson.so

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

pgjson.sql: pgjson.sql.in
	sed -e "s,MODULE_PATHNAME,pgjson,g" $< >$@
	
pgjson.dev.sql: pgjson.sql.in
	sed -e "s,MODULE_PATHNAME,$(MODULE_PATHNAME),g" $< >$@
	

