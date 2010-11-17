/**
 * setup.h
 */
#ifndef __STDINCLUDE_H__
#define __STDINCLUDE_H__

/* including postgres.h defines autoconf headers as well - so clear them here */
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

#include "pgjson_config.h"

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define ATTR_PACKED_STRUCTURE __attribute__((__packed__))

#endif
