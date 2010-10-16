#include <limits.h>

#include "util/setup.h"
#include "util/stringutil.h"
#include "json/bsonparser.h"
#include "json/bsonconst.h"

/* Macros to do structred read from the buffer.  Most platforms allow unaligned
 * access, but if not, replace these macros that are safe for unaligned access
 */
#define READ_INT32(ptr) \
	*((int32_t*)(ptr))
#define READ_INT64(ptr) \
	*((int64_t*)(ptr))


bool bsonvalue_load(bsonvalue_t *bsv, uint8_t *source, size_t source_len, bsonvalue_mode_t mode)
{
	int32_t length;
	bsontype_t type;
	uint8_t *limit=source+source_len;
	uint8_t *scratch;
	size_t remain;
	uint8_t *value_start;

	/* determine the tag and label start - this depends on mode */
	/* either this block will return directly or will populate the following:
	 *   type
	 *   bsv->type
	 *   bsv->label
	 *   bsv->value_start
	 */
	if (mode==BSONMODE_ROOT) {
		/* root */
		if (source_len<4) return false;
		length=BSON_SWAP_BTOH32(READ_INT32(source));

		if (length<0) {
			/* root literal with inverted tag*/
			type=bsv->type=-length;
			bsv->label=0;
			bsv->value_start=source+4;
			if (type<=0 || type>BSONTYPE_MAX_KNOWN) {
				/* nope - invalid */
				return false;
			}
		} else {
			/* root document */
			/* detect a single root document (min length == 5) */
			if (length>=5) {
				/* root document - just deal with it all here */
				if (length>source_len) {
					/* short doc */
					return false;
				}
				bsv->type=BSONTYPE_DOCUMENT;
				bsv->label=0;
				bsv->remaining=0;
				bsv->value_next=source+length;
				bsv->value_start=source;
				bsv->value.document.docsize=length-4;
				bsv->value.document.docstart=source+4;
				return true;
			} else {
				/* illegal document length */
				return false;
			}
		}
	} else if (mode==BSONMODE_ELEMENT) {
		if (source_len<1) return false;
		type=bsv->type=source[0];
		if (type!=0) {
			/* ! last member of object */
			if (source_len<2) return false;
			/* Find limit of label string */
			scratch=source+1;
			bsv->label=(char*)scratch;
			while (scratch<limit && *scratch) scratch++;
			if (scratch==limit) return false;

			bsv->value_start=scratch+1;
		} else {
			/* last member of object */
			bsv->label=0;
			bsv->value_start=source+1;
		}
	} else {
		/* unknown mode */
		return false;
	}

	/* interpret the tag */
	remain=limit-bsv->value_start;
	value_start=bsv->value_start;

	switch (type) {
	case 0:
		/* End of iteration */
		return true;
	case BSONTYPE_DATETIME:
	case BSONTYPE_INT64:
	case BSONTYPE_TIMESTAMP:
	case BSONTYPE_DOUBLE:
		if (remain<8) return false;
		bsv->value.int64_value=BSON_SWAP_BTOH64(READ_INT64(value_start));
		value_start+=8;
		break;
	case BSONTYPE_SYMBOL:
	case BSONTYPE_STRING:
		if (remain<5) return false;
		length=BSON_SWAP_BTOH32(READ_INT32(value_start));
		value_start+=4;
		bsv->value.string.length=length;
		bsv->value.string.contents=value_start;
		value_start+=length;
		if (length<1 || value_start>limit) return false;
		break;
	case BSONTYPE_ARRAY:
	case BSONTYPE_DOCUMENT:
		if (remain<5) return false;
		length=BSON_SWAP_BTOH32(READ_INT32(value_start));
		bsv->value.document.docsize=length-4;
		bsv->value.document.docstart=value_start+4;
		value_start+=length;
		if (length<5 || value_start>limit || *(value_start-1)!=0) return false;
		break;
	case BSONTYPE_BINARY:
		if (remain<5) return false;
		length=BSON_SWAP_BTOH32(READ_INT32(value_start));
		value_start+=4;
		bsv->value.binary.length=length;
		bsv->value.binary.subtype=*(value_start++);
		bsv->value.binary.contents=value_start;
		if (length<0 || value_start>limit) return false;
		break;
	case BSONTYPE_NULL:
	case BSONTYPE_UNDEFINED:
		/* nothing to do */
		break;
	case BSONTYPE_OBJID:
		if (remain<12) return false;
		memcpy(bsv->value.object_id_value, value_start, 12);
		value_start+=12;
		break;
	case BSONTYPE_BOOL:
		if (remain<1) return false;
		bsv->value.bool_value=*value_start;
		value_start+=1;
		break;
	case BSONTYPE_REGEX:
		bsv->value.regex.expression=value_start;
		while (value_start<limit && *value_start) value_start++;
		if (value_start==limit) return false;
		value_start++;
		bsv->value.regex.options=value_start;
		while (value_start<limit && *value_start) value_start++;
		if (value_start==limit) return false;
		break;
	case BSONTYPE_DBPOINTER:
		/* deprecated - don't store anything, just traverse */
		if (remain<5) return false;
		length=BSON_SWAP_BTOH32(READ_INT32(value_start));
		value_start+=4;
		value_start+=length+12;
		if (length<1 || value_start>limit) return false;
		break;
	case BSONTYPE_JSCODE:
	case BSONTYPE_JSCODE_WSCOPE:
		if (remain<5) return false;
		length=BSON_SWAP_BTOH32(READ_INT32(value_start));
		value_start+=4;
		bsv->value.jscode_wscope.length=length;
		bsv->value.jscode_wscope.contents=value_start;
		value_start+=length;
		if (length<1 || value_start>limit) return false;
		if (type==BSONTYPE_JSCODE_WSCOPE) {
			bsv->value.jscode_wscope.document=value_start;
			if ((limit-value_start)<5) return false;
			length=BSON_SWAP_BTOH32(READ_INT32(value_start));
			value_start+=length;
			if (length<5 || value_start>limit) return false;
		}
		break;
	case BSONTYPE_INT32:
		if (remain<4) return false;
		bsv->value.int32_value=BSON_SWAP_BTOH32(READ_INT32(value_start));
		value_start+=4;
		break;

	default:
		/* unknown tag */
		return false;
	}

	/* value_start is positioned at the first byte of the next value */
	bsv->value_next=value_start;
	bsv->remaining=limit-value_start;

	return true;
}

bool bsonvalue_next(bsonvalue_t *bsv)
{
	bool rc;
	if (bsv->remaining==0) return false;
	rc=bsonvalue_load(bsv, bsv->value_next, bsv->remaining, BSONMODE_ELEMENT);
	return rc;
}


/* forward defines */
static bool visit_value(jsonvisitor_t *visitor, bsonvalue_t *bsv, stringwriter_t *scratch);
static bool visit_document(jsonvisitor_t *visitor, bsonvalue_t *bsv, stringwriter_t *scratch);
static bool visit_array(jsonvisitor_t *visitor, bsonvalue_t *bsv, stringwriter_t *scratch);

int32_t bsonparser_parse(uint8_t *buffer, size_t len, jsonvisitor_t *visitor)
{
	bsonvalue_t rootbsv;
	int32_t rc;
	stringwriter_t scratch;
	stringwriter_init(&scratch, 256);

	if (!bsonvalue_load(&rootbsv, buffer, len, BSONMODE_ROOT)) rc=-1;
	else {
		rc=rootbsv.value_next-buffer;
		if (!visit_value(visitor, &rootbsv, &scratch)) rc=-1;
	}

	stringwriter_destroy(&scratch);
	return rc;
}

static bool visit_value(jsonvisitor_t *visitor, bsonvalue_t *bsv, stringwriter_t *scratch)
{
	const jsonvisitor_vft *vft=visitor->vft;
	jsonstring_t jsonstring;

	switch (bsv->type) {
	case 0:
		/* should not encounter a null term here - should be caught in iteration */
		return false;
	case BSONTYPE_DOUBLE:
		vft->add_double(visitor, bsv->value.double_value);
		break;
	case BSONTYPE_STRING:
	case BSONTYPE_SYMBOL:
		jsonstring.length=bsv->value.string.length-1;
		jsonstring.bytes=bsv->value.string.contents;
		vft->add_string(visitor, &jsonstring);
		break;
	case BSONTYPE_DOCUMENT:
		return visit_document(visitor, bsv, scratch);
		break;
	case BSONTYPE_ARRAY:
		return visit_array(visitor, bsv, scratch);
		break;
	case BSONTYPE_UNDEFINED:
		vft->add_undefined(visitor);
		break;
	case BSONTYPE_BOOL:
		vft->add_bool(visitor, bsv->value.bool_value);
		break;
	case BSONTYPE_NULL:
		vft->add_null(visitor);
		break;
	case BSONTYPE_INT32:
		vft->add_int32(visitor, bsv->value.int32_value);
		break;
	case BSONTYPE_INT64:
		vft->add_int64(visitor, bsv->value.int64_value);
		break;
	default:
		/* write an undefined for unsupported types */
		vft->add_undefined(visitor);
	}

	return true;
}

static bool visit_document(jsonvisitor_t *visitor, bsonvalue_t *bsv, stringwriter_t *scratch)
{
	const jsonvisitor_vft *vft=visitor->vft;
	bsonvalue_t childbsv;
	jsonstring_t jsonstring;

	if (bsv->value.document.docsize==1 && bsv->value.document.docstart[0]==0) {
		/* empty document */
		vft->add_empty_object(visitor);
		return true;
	}

	/* init the child cursor */
	if (!bsonvalue_load(&childbsv, bsv->value.document.docstart, bsv->value.document.docsize, BSONMODE_ELEMENT)) {
		return false;
	}

	/* iterate over children */
	vft->start_object(visitor);
	do {
		/* end of iteration */
		if (childbsv.type==0) break;

		/* unpack modified utf8 label */
		scratch->pos=0;
		if (!stringwriter_unpack_modified_utf8z(scratch, childbsv.label, childbsv.value_start-(uint8_t*)childbsv.label)) return false;
		jsonstring.length=scratch->pos;
		jsonstring.bytes=scratch->string;
		vft->push_label(visitor, &jsonstring);

		/* and emit the value */
		if (!visit_value(visitor, &childbsv, scratch)) return false;
	} while (bsonvalue_next(&childbsv));
	vft->end_object(visitor);

	return true;
}

static bool visit_array(jsonvisitor_t *visitor, bsonvalue_t *bsv, stringwriter_t *scratch)
{
	const jsonvisitor_vft *vft=visitor->vft;
	bsonvalue_t childbsv;
	jsonstring_t jsonstring;

	if (bsv->value.document.docsize==1 && bsv->value.document.docstart[0]==0) {
		/* empty document */
		vft->add_empty_array(visitor);
		return true;
	}

	/* init the child cursor */
	if (!bsonvalue_load(&childbsv, bsv->value.document.docstart, bsv->value.document.docsize, BSONMODE_ELEMENT)) {
		return false;
	}

	/* iterate over children */
	vft->start_array(visitor);
	do {
		/* end of iteration */
		if (childbsv.type==0) break;

		/* and emit the value */
		if (!visit_value(visitor, &childbsv, scratch)) return false;
	} while (bsonvalue_next(&childbsv));
	vft->end_array(visitor);

	return true;
}
