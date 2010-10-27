#include "jsonutil.h"

/**
 * The escape table is a classification table for characters that controls
 * how they should be escaped.  Each entry is a single character signifying
 * how it should be interpreted:
 *    - 0: Printable ascii character with no special meaning in a string literal
 *    - 1: Special character which should be escape with a backslash (\\)
 *    - 2: Single or Double quote
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
	0, 0, 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
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


bool json_escape_string(dynbuffer_t *dest, const uint8_t *source, size_t len, bool ascii_only, uint8_t quote_char)
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
		cur=source[index];
		type=JSON_ESCAPE_TABLE[cur];

		/* Is it literal printable */
		if (type==0 || (type==2&&cur!=quote_char))
			continue;	/* Accumulate */

		/* Special char - dump what we have */
		if (index>mark) dynbuffer_append(dest, source+mark, index-mark);

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
				utf8_2=source[index];
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
				utf8_2=source[index-1];
				utf8_3=source[index];

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
				utf8_2=source[index-2];
				utf8_3=source[index-1];
				utf8_4=source[index];

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
		dynbuffer_append(dest, replacement, replacement_count);

		/* Advance the mark */
		mark=index+1;
	}

	/* Write final tail */
	if (len>mark) {
		if (len>mark) dynbuffer_append(dest, source+mark, len-mark);
	}

	return true;
}
