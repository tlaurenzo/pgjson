/* This header file is included as part of the compiler options */
/* including postgres.h defines autoconf headers as well - so clear them here */
#include <postgres.h>

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

#define malloc palloc
#define free pfree
#define realloc repalloc

/* Set the stringwriter header size to the VARHDRSZ - this way
   stringwriter buffers can be returned directly as vardata
*/
#define STRINGWRITER_HEADER_SIZE VARHDRSZ


