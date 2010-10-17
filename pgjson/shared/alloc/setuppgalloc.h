/* This header file is included as part of the compiler options */
/* including postgres.h defines autoconf headers as well - so clear them here */
#include <postgres.h>

/*#define PGMEMDEBUG*/

#ifdef PGMEMDEBUG
#include <stdio.h>
#endif

/* including postgres.h defines autoconf junk as well - so clear them here */
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

/* Redefine malloc/free/realloc */
#ifdef malloc
#undef malloc
#endif

#ifdef free
#undef free
#endif

#ifdef realloc
#undef realloc
#endif

#ifdef PGMEMDEBUG
static inline void pgjsondebugfree(void *ptr, char *file, int line)
{
	if (!ptr) {
		fprintf(stderr, "Attempt to free null pointer: %s(%d)\n", file, line);
		return;
	}
	pfree(ptr);
}
/* debug allocation */
#define malloc palloc
#define free(ptr) pgjsondebugfree(ptr, __FILE__, __LINE__)
/* repalloc does not accept null pointers as input - munge it so it is a suitable replacement for realloc */
#define realloc(ptr,size) (ptr ? repalloc(ptr,size) : malloc(size))

#else

/* non debug allocation */
#define malloc palloc
/* pfree does not accept null pointers but C free does.  generally,
 * we should avoid freeing null pointers, but bison parsers do this.
 * emulate behavior of C free
 */
#define free(ptr) (ptr ? pfree(ptr), 0 : 0)
/* repalloc does not accept null pointers as input - munge it so it is a suitable replacement for realloc */
#define realloc(ptr,size) (ptr ? repalloc(ptr,size) : malloc(size))

#endif


/* Set the stringwriter header size to the VARHDRSZ - this way
   stringwriter buffers can be returned directly as vardata
*/
#define STRINGWRITER_HEADER_SIZE 4
