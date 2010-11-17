MODULE_big = pgjson
#DATA_built = pgjson.sql
OBJS = \
	jsonlib/json_transcode_to_json.o \
	jsonlib/json_transcode_json_to_binary.o \
	jsonlib/json_transcode_binary_to_json.o \
	jsonlib/json_validate_json.o \
	jsonlib/dynbuffer.o \
	jsonlib/jsonlex.tab.o \
	jsonlib/jsonutil.o

#PG_CPPFLAGS = -Werror
	
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)


