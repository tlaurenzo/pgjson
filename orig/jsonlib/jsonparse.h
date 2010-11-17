/**
 * jsonparse.h
 *
 * Interface to simple recursive descent parser for JSON with actions defined via
 * pre-processor macros.
 */
#ifndef __JSONPARSE_H__
#define __JSONPARSE_H__
#include "jsonlex.h"

/**
 * JSONPARSE_EXTRA_DECL
 * If defined, add additional declarations to jsonparseinfo_t
 */
#ifndef JSONPARSE_EXTRA_DECL
#define JSONPARSE_EXTRA_DECL
#endif

typedef struct {
	/**
	 * lexstate as the first member means that jsonparse_t
	 * effectively extends jsonlex_state_t.
	 */
	jsonlex_state_t lexstate;

	JSONPARSE_EXTRA_DECL;
} jsonparseinfo_t, *jsonparseinfo_arg;

/** Forward declarations **/
JSON_FDECLP bool jsonparse(jsonparseinfo_t *info);


#endif
