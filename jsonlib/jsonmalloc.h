#ifndef __JSONMALLOC_H__
#define __JSONMALLOC_H__

#ifdef JSON_USE_PALLOC
#include <postgres.h>
#ifndef JSON_malloc
#define JSON_malloc palloc
#endif

#ifndef JSON_free
#define JSON_free pfree
#endif

#ifndef JSON_realloc
#define JSON_realloc repalloc
#endif
#else
#include <stdlib.h>

/** Provide malloc/realloc/free defines **/
#ifndef JSON_malloc
#define JSON_malloc malloc
#endif

#ifndef JSON_free
#define JSON_free free
#endif

#ifndef JSON_realloc
#define JSON_realloc realloc
#endif
#endif

#endif
