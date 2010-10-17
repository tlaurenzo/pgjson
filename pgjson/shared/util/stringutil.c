#include "util/setup.h"
#include "util/stringutil.h"


/**
 * The escape table is a classification table for characters that controls
 * how they should be escaped.  Each entry is a single character signifying
 * how it should be interpreted:
 *    - 0: Printable ascii character with no special meaning in a string literal
 *    - 1: Special character which should be escape with a backslash (\\)
 *    - 2: Double quote
 *    - 3: Single quote
 *    - 4: Illegal
 *    - 5: Non-printing character which should be escaped with a numeric escape
 *    - 6: UTF-8 2-byte sequence start
 *    - 7: UTF-8 3-byte sequence start
 *    - 8: UTF-8 4-byte sequence start
 * Any entries >= 0x10 should be interpreted as the actual escape character.
 */
static const char JSON_ESCAPE_TABLE[256]={
/*  00 - 1F */
/*   0   1  2  3  4  5  6  7   8    9    A    B    C    D   E  F 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F */
	'0', 5, 5, 5, 5, 5, 5, 5, 'b', 't', 'n', 'v', 'f', 'r', 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
/*  20 - 3F */
/*  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F */
	0, 0, 2, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*  40 - 5F */
/*  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
/*  60 - 7F */
/*  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
/*  80 - 9F */
/*  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F */
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
/*  A0 - BF */
/*  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F */
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
/*  C0 - DF */
/*  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F */
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
/*  E0 - FF */
/*  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F */
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 4, 4, 4, 4, 4, 4, 4, 4,
};

static const char *UTF8_INVALID="\\uFFFD";
static const uint8_t UTF8_MODNULL[]={ 0xc0, 0x80 };

/**** stringwriter ****/
void stringwriter_init(stringwriter_t *self, size_t initial_capacity)
{
	self->string=0;
   self->capacity=initial_capacity;
	self->pos=0;
	self->alloc_error=false;
}

void stringwriter_detach(stringwriter_t *self)
{
   self->string=0;
   self->capacity=0;
   self->pos=0;
}

void stringwriter_destroy(stringwriter_t *self)
{
   uint8_t *curbuffer=self->string;
   if (STRINGWRITER_HEADER_SIZE!=0) {
      if (curbuffer) curbuffer-=STRINGWRITER_HEADER_SIZE;
   }
   
	if (curbuffer) {
		free(curbuffer);
	}
}

void stringwriter_clear(stringwriter_t *self)
{
	self->pos=0;
}

bool stringwriter_allocate(stringwriter_t *self, size_t requested_capacity)
{
   size_t  new_capacity;
   uint8_t *curbuffer=self->string;
   uint8_t *newbuffer;

   if (self->alloc_error) return false;
   
   
   /* grow the current capacity by a power of two.  If still less than requested_capacity,
      then just accept requested_capacity */
   new_capacity=self->capacity;
   if (new_capacity < requested_capacity) {
      new_capacity*=2;
      if (new_capacity<requested_capacity) new_capacity=requested_capacity;
   }
   
   /* rebase the pointer - see setuppgalloc.h */
   /* this will be optimized out if not needed */
   if (STRINGWRITER_HEADER_SIZE!=0) {
      if (curbuffer) curbuffer-=STRINGWRITER_HEADER_SIZE;
   }
   
   /* realloc */
   newbuffer=realloc(curbuffer, new_capacity);
   if (!newbuffer) {
      /* alloc error */
      if (curbuffer) free(curbuffer);
      self->alloc_error=true;
      self->string=0;
      self->capacity=0;
      return false;
   }

   /* rebase the pointer - will be optimized out if not needed */
   if (STRINGWRITER_HEADER_SIZE!=0) {
      newbuffer+=STRINGWRITER_HEADER_SIZE;
   }
   
   /* setup new capacity */
   self->capacity=new_capacity;
   self->string=newbuffer;   
   
   return true;
}

void *stringwriter_get_alloc_buffer(stringwriter_t *self)
{
   uint8_t *curbuffer=self->string;
   if (STRINGWRITER_HEADER_SIZE!=0) {
      if (curbuffer) curbuffer-=STRINGWRITER_HEADER_SIZE;
   }
   
   return curbuffer;
}


#define VERIFY_ALLOC(self, addl_capacity) \
   if ((!self->string || self->capacity < self->pos+addl_capacity) && !stringwriter_allocate(self, self->pos+addl_capacity)) return false;
   

inline bool stringwriter_append(stringwriter_t *self, const void *source, size_t bytes)
{
   VERIFY_ALLOC(self, bytes);

	memcpy(self->string + self->pos, source, bytes);
	self->pos+=bytes;

	return true;
}

inline bool stringwriter_append_byte(stringwriter_t *self, uint8_t byte)
{
   VERIFY_ALLOC(self, 1);
	self->string[self->pos++]=byte;
	return true;
}

bool stringwriter_append_escape_char(stringwriter_t *self, char escape)
{
	char replace;

	VERIFY_ALLOC(self, 1);
	switch (escape) {
	case '\'': replace='\''; break;
	case '"':  replace='"';  break;
	case '\\': replace='\\'; break;
	case 'n': replace='\n'; break;
	case 'r': replace='\r'; break;
	case 't': replace='\t'; break;
	case 'b': replace='\b'; break;
	case 'f': replace='\f'; break;
	case '0': replace=0; break;
	default:
		replace=escape;
	}

	self->string[self->pos++]=replace;

	return true;
}

/**
 * TODO: This method needs testing.  I wrote it from memory and a scratch sheet
 * of paper.
 */
bool stringwriter_append_codepoint(stringwriter_t *self, uint32_t codepoint)
{
	char codepoints[4];
	size_t size;

	if (codepoint>=0 && codepoint<=0x7f) {
		codepoints[0]=codepoint;
		size=1;
	} else if (codepoint<=0x7ff) {
		codepoints[0]=0xc0 | ((codepoint>>6)&0x1f);
		codepoints[1]=0x80 | (codepoint&0x3f);
		size=2;
	} else if (codepoint<=0xffff) {
		codepoints[0]=0xe0 | ((codepoint>>12)&0xff);
		codepoints[1]=0x80 | ((codepoint>>6)&0x3f);
		codepoints[2]=0x80 | ((codepoint&0x3f));
		size=3;
	} else if (codepoint<=0x10ffff) {
		codepoints[0]=0xf0 | ((codepoint>>18)&0x7);
		codepoints[1]=0x80 | ((codepoint>>12)&0x3f);
		codepoints[2]=0x80 | ((codepoint>>6)&0x3f);
		codepoints[2]=0x80 | ((codepoint&0x3f));
		size=4;
	} else {
		// Unicode replacement char uFFFD as UTF-8
		codepoints[0]=0xef;
		codepoints[1]=0xbf;
		codepoints[2]=0xbd;
		size=3;
	}

	return stringwriter_append(self, codepoints, size);
}

size_t stringwriter_skip(stringwriter_t *self, size_t delta)
{
	size_t original_pos=self->pos;
	self->pos+=delta;

	return original_pos;
}

bool stringwriter_write(stringwriter_t *self, size_t pos, const void *source, size_t bytes)
{
	bool rc;
	size_t original_pos=self->pos;
	self->pos=pos;
	rc=stringwriter_append(self, source, bytes);
	self->pos=original_pos;
	return rc;
}

bool stringwriter_append_modified_utf8z(stringwriter_t *self, uint8_t* source, size_t bytes)
{
	/* Write in span sized traunches to avoid alloc calls */
	size_t mark=0;
	size_t i;

	for (i=0; i<bytes; i++) {
		if (source[i]==0) {
			/* Dump from mark */
			if (mark<i) {
				stringwriter_append(self, source+mark, i-mark);
				stringwriter_append(self, UTF8_MODNULL, 2);
				mark=i+1;
			}
		}
	}

	/* Dump last span */
	if (mark<bytes) {
		stringwriter_append(self, source+mark, bytes-mark);
	}

	/* terminating null */
	stringwriter_append_byte(self, 0);

	return !self->alloc_error;
}

bool stringwriter_unpack_modified_utf8z(stringwriter_t *self, const char *source, size_t maxbytes)
{
	int i;
	uint8_t c;
	const uint8_t *bytes=(const uint8_t*)source;

	/* reserve all needed space so we can just write direct to buffer
	 * with no further bounds checks
	 */
	VERIFY_ALLOC(self, maxbytes+1);

	for (i=0; i<maxbytes; i++) {
		c=bytes[i];
		if (c==0) break;
		if (c==0xc0 && (i+1)<maxbytes && bytes[i+1]==0x80) {
			self->string[self->pos++]=0;
			i++;
		} else {
			self->string[self->pos++]=c;
		}
	}

	return true;
}

bool stringwriter_append_jsonescape(stringwriter_t *self, const uint8_t *utf8_string, size_t len,
		stringutil_escapemode_t escape_mode,
		bool escape_single_quote, bool escape_double_quote)
{
	/* Instead of going char by char, we preseume that we are going to encounter
	 * printing characters more than not and try to reserve a span of them to
	 * write as a traunch, reducing the number of IO calls.
	 */
	size_t mark=0, index=0;
	uint8_t cur;
	char type;
	char replacement[10];
	size_t replacement_count=0;
	uint32_t utf8_1, utf8_2, utf8_3, utf8_4;
	uint32_t codepoint;

	for (; index<len; index++) {
		cur=utf8_string[index];
		type=JSON_ESCAPE_TABLE[cur];

		/* Is it literal printable */
		if (type==0 || (type==2&&!escape_double_quote) || (type==3&&!escape_single_quote))
			continue;	/* Accumulate */

		/* Special char - dump what we have */
		if (index>mark && !stringwriter_append(self, utf8_string+mark, index-mark)) return false;


		if (type>=0x20) {
			/* Replacement is type */
			replacement[0]='\\';
			replacement[1]=type;
			replacement_count=2;
		} else {
			switch (type) {
			case 1:
				/* Literal escape */
				replacement[0]='\\';
				replacement[1]=(char)cur;
				replacement_count=2;
				break;
			case 2:
				/* Double quote */
				replacement[0]='\\';
				replacement[1]='"';
				replacement_count=2;
				break;
			case 3:
				/* Single quote */
				replacement[0]='\\';
				replacement[1]='\'';
				replacement_count=2;
				break;
			case 4:
				/* Illegal */
				strcpy(replacement, UTF8_INVALID);
				replacement_count=strlen(replacement);
				break;
			case 5:
				/* Numeric escape */
				sprintf(replacement, "\\x%02x", cur);
				replacement_count=4;
				break;
			case 6:
				/* UTF-8 2-byte */
				index++;
				if (index>=len) continue;	/* Short sequence */
				replacement_count=6;
				utf8_1=cur;
				utf8_2=utf8_string[index];
				if ((utf8_2>>6)!=2) {
					/* Illegal */
					strcpy(replacement, UTF8_INVALID);
				} else {
					/* Legal */
					codepoint=((utf8_1&0x1f)<<5) | (utf8_2&0x3f);
					sprintf(replacement, "\\u%04x", codepoint);
				}

				break;
			case 7:
				/* UTF-8 3 byte */
				index+=2;
				if (index>=len) continue; /* Short sequence */
				replacement_count=6;
				utf8_1=cur;
				utf8_2=utf8_string[index-1];
				utf8_3=utf8_string[index];

				if ((utf8_2>>6)!=2 || (utf8_3>>6)!=2) {
					/* Illegal */
					strcpy(replacement, UTF8_INVALID);
				} else {
					/* Legal */
					codepoint=((utf8_1&0xf)<<12) | ((utf8_2&0x3f)<<6) | (utf8_3&0x3f);
					sprintf(replacement, "\\u%04x", codepoint);
				}
				break;
			case 8:
				/* UTF-8 4 byte */
				index+=3;
				if (index>=len) continue; /* Short sequence */
				replacement_count=6;
				utf8_1=cur;
				utf8_2=utf8_string[index-2];
				utf8_3=utf8_string[index-1];
				utf8_4=utf8_string[index];

				if ((utf8_2>>6)!=2 || (utf8_3>>6)!=2 || (utf8_4>>6)!=2) {
					/* Illegal */
					strcpy(replacement, UTF8_INVALID);
				} else {
					/* Legal syntactically */
					codepoint=((utf8_1&0x7)<<18) | ((utf8_2&0x3f)<<12) | ((utf8_3&0x3f)<<6) | (utf8_1&0x3f);
					if (codepoint>0xffff) {
						/* JavaScript only has escape for first ffff codepoints */
						strcpy(replacement, UTF8_INVALID);
					} else {
						sprintf(replacement, "\\u%04x", codepoint);
					}
				}
			}
		}

		/* Write the replacement */
		if (!stringwriter_append(self, replacement, replacement_count)) return false;

		/* Advance the mark */
		mark=index+1;
	}

	/* Write final tail */
	if (len>mark) {
		if (len>mark && !stringwriter_append(self, utf8_string+mark, len-mark)) return false;
	}

	return true;
}


/** string writing utilities **/
/** this method is deprecated **/
bool stringutil_escapejson(FILE* out, const uint8_t *utf8_string, size_t len,
		stringutil_escapemode_t escape_mode,
		bool escape_single_quote, bool escape_double_quote)
{
	/* Instead of going char by char, we preseume that we are going to encounter
	 * printing characters more than not and try to reserve a span of them to
	 * write as a traunch, reducing the number of IO calls.
	 */
	size_t mark=0, index=0;
	uint8_t cur;
	char type;
	char replacement[10];
	size_t replacement_count=0;
	uint32_t utf8_1, utf8_2, utf8_3, utf8_4;
	uint32_t codepoint;

	for (; index<len; index++) {
		cur=utf8_string[index];
		type=JSON_ESCAPE_TABLE[cur];

		/* Is it literal printable */
		if (type==0 || (type==2&&!escape_double_quote) || (type==3&&!escape_single_quote))
			continue;	/* Accumulate */

		/* Special char - dump what we have */
		if (fwrite(utf8_string+mark, index-mark, 1, out)!=1) return false;

		if (type>=0x20) {
			/* Replacement is type */
			replacement[0]='\\';
			replacement[1]=type;
			replacement_count=2;
		} else {
			switch (type) {
			case 1:
				/* Literal escape */
				replacement[0]='\\';
				replacement[1]=(char)cur;
				replacement_count=2;
				break;
			case 2:
				/* Double quote */
				replacement[0]='\\';
				replacement[1]='"';
				replacement_count=2;
				break;
			case 3:
				/* Single quote */
				replacement[0]='\\';
				replacement[1]='\'';
				replacement_count=2;
				break;
			case 4:
				/* Illegal */
				strcpy(replacement, UTF8_INVALID);
				replacement_count=strlen(replacement);
				break;
			case 5:
				/* Numeric escape */
				sprintf(replacement, "\\x%02x", cur);
				replacement_count=4;
				break;
			case 6:
				/* UTF-8 2-byte */
				index++;
				if (index>=len) continue;	/* Short sequence */
				replacement_count=6;
				utf8_1=cur;
				utf8_2=utf8_string[index];
				if ((utf8_2>>6)!=2) {
					/* Illegal */
					strcpy(replacement, UTF8_INVALID);
				} else {
					/* Legal */
					codepoint=((utf8_1&0x1f)<<5) | (utf8_2&0x3f);
					sprintf(replacement, "\\u%04x", codepoint);
				}

				break;
			case 7:
				/* UTF-8 3 byte */
				index+=2;
				if (index>=len) continue; /* Short sequence */
				replacement_count=6;
				utf8_1=cur;
				utf8_2=utf8_string[index-1];
				utf8_3=utf8_string[index];

				if ((utf8_2>>6)!=2 || (utf8_3>>6)!=2) {
					/* Illegal */
					strcpy(replacement, UTF8_INVALID);
				} else {
					/* Legal */
					codepoint=((utf8_1&0xf)<<12) | ((utf8_2&0x3f)<<6) | (utf8_3&0x3f);
					sprintf(replacement, "\\u%04x", codepoint);
				}
				break;
			case 8:
				/* UTF-8 4 byte */
				index+=3;
				if (index>=len) continue; /* Short sequence */
				replacement_count=6;
				utf8_1=cur;
				utf8_2=utf8_string[index-2];
				utf8_3=utf8_string[index-1];
				utf8_4=utf8_string[index];

				if ((utf8_2>>6)!=2 || (utf8_3>>6)!=2 || (utf8_4>>6)!=2) {
					/* Illegal */
					strcpy(replacement, UTF8_INVALID);
				} else {
					/* Legal syntactically */
					codepoint=((utf8_1&0x7)<<18) | ((utf8_2&0x3f)<<12) | ((utf8_3&0x3f)<<6) | (utf8_1&0x3f);
					if (codepoint>0xffff) {
						/* JavaScript only has escape for first ffff codepoints */
						strcpy(replacement, UTF8_INVALID);
					} else {
						sprintf(replacement, "\\u%04x", codepoint);
					}
				}
			}
		}

		/* Write the replacement */
		if (fwrite(replacement, replacement_count, 1, out)!=1) return false;

		/* Advance the mark */
		mark=index+1;
	}

	/* Write final tail */
	if (len>mark) {
		if (fwrite(utf8_string+mark, len-mark, 1, out)!=1) return false;
	}

	return true;
}

bool stringwriter_slurp_file(stringwriter_t *self, FILE *input_file)
{
	size_t r;
	
	for (;;) {
	   r=self->capacity - self->pos;
	   if (r==0) r=1024;
	   VERIFY_ALLOC(self, r);
	   
		r=fread(self->string+self->pos, 1, r, input_file);
		if (r==0) {
			if (feof(input_file)) break;
			return 0;
		}
		self->pos+=r;
	}
	return true;
}
