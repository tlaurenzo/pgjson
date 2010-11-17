#include "hexdump.h"

bool hexdump(FILE *out, uint8_t *buffer, uint32_t len)
{
	uint32_t para;
	uint32_t index;
	char c;

	for (para=0; ; para+=16) {
		if (para>=len) break;

		/* output the address */
		if (fprintf(out, "%08x  ", para)<0) return false;

		/* output the hex dump */
		for (index=0; index<16; index++) {
			if ((para+index)>=len) {
				if (fprintf(out, "   ")<0) return false;
			} else {
				if (fprintf(out, "%02x ", (uint32_t)(buffer[para+index]))<0) return false;
			}

			if (index==7) {
				if (fprintf(out, "  ")<0) return false;
			}
		}

		/* output the ascii */
		if (fprintf(out, "  ")<0) return false;
		for (index=0; index<16; index++) {
			if ((para+index)>=len) {
				if (fprintf(out, " ")<0) return false;
			} else {
				c=(char)buffer[para+index];
				if (c<32 || c>126) c='.';
				if (fprintf(out, "%c", c)<0) return false;
			}
		}

		if (fprintf(out, "\n")<0) return false;
	}

	return true;
}
