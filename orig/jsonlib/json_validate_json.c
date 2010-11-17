#include "jsonutil.h"

#include "jsonlex.inc.c"
#include "jsonparse.inc.c"

bool json_validate_json(uint8_t *source, size_t sourcelen)
{
	bool result;
	jsonparseinfo_t parseinfo;

	/* init the lexer */
	jsonlex_init_io(&parseinfo.lexstate, source, sourcelen);

	result=jsonparse(&parseinfo);

	/* destroy */
	jsonlex_destroy(&parseinfo.lexstate);

	return result;
}

