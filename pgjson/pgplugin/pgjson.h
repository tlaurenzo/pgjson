/* postgres server includes */
#include <postgres.h>
#include <fmgr.h>
#include <utils/builtins.h>

#define LEVEL_DEBUG 10
#define LEVEL_WARN 5

#define PGJSON_DEBUG(level, msg) \
	do { \
		ereport(NOTICE, (errmsg_internal("[%s:%s:%d] " msg, __FILE__, __func__, __LINE__))); \
	} while (0);

#define PGJSON_DEBUGF(level, msg, ...) \
	do { \
		ereport(NOTICE, (errmsg_internal("[%s:%s:%d] " msg, __FILE__, __func__, __LINE__, __VA_ARGS__))); \
	} while (0);


