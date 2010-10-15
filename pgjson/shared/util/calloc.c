/**
 * Alias pgjson_malloc/free/realloc to the standard c equivilents.
 * Note that we do not include setup.h here because the macros
 * would trash what we're trying to do.
 */
#include <stdlib.h>

/** GCC aliases - Other compilers may need different bits **/
/*void pgjson_malloc() __attribute__ ((weak, alias("malloc")));*/


