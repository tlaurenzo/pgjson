/**
 * jsonparse.h
 *
 * Interface to simple recursive descent parser for JSON with actions defined via
 * pre-processor macros.
 */
#ifndef __JSONPARSE_H__
#define __JSONPARSE_H__
#include "jsonlex.h"

typedef struct {
	/**
	 * lexstate as the first member means that jsonparse_t
	 * effectively extends jsonlex_state_t.
	 */
	jsonlex_state_t lexstate;


} jsonparseinfo_t, *jsonparseinfo_arg;


#endif
