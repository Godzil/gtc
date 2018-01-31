/*
 * GTools C compiler
 * =================
 * source file :
 * binary to TI calculator format conversion
 *
 * Copyright 2001-2004 Paul Froissart.
 * Credits to Christoph van Wuellen and Matthew Brandt.
 * All commercial rights reserved.
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "define.h"
#include "gtpack/gtpack.c"

// This structure comes from ttools.
typedef struct {
    char          signature[8]; // "**TI92P*" or "**TI89**"
    unsigned char fill1[2];     // 01 00
    char          folder[8];    // folder name
    char          desc[40];     // ---- not used ----
    unsigned char fill2[6];     // 01 00 52 00 00 00
    char          name[8];      // varname
    unsigned char type[4];      // 0C 00 00 00
    unsigned char size[4];      // complete file size (including checksum)
    unsigned char fill3[6];     // A5 5A 00 00 00 00
    unsigned char datasize[2];  // data size
} TI_FILE_HDR;


static void put_little_endian(unsigned char *dest, unsigned long val, int len) {
    while (len--) {
	*dest++ = (unsigned char)(val&0xFF);
	val >>= 8;
    }
}
static void put_big_endian(unsigned char *dest, unsigned long val, int len) {
    dest += len;
    while (len--) {
	*--dest = (unsigned char)(val&0xFF);
	val >>= 8;
    }
}

char *create_ti_file(int calc,int tigl_type,char *name,char *folder,char *content,unsigned long content_size,unsigned long *ti_file_size) {
    TI_FILE_HDR h;
    memset(&h, 0, sizeof h);
    memcpy(h.signature, "********", 8);
#define sig(x) memcpy(h.signature+2,x,sizeof(x)-1)
    switch (calc) {
	case 0:
	    sig("TI89");
	    break;
	case 1:
	    sig("TI92P");
	    break;
	case 2:
	    sig("TI92P");
	    break;
	default:
	    fatal("unknown calc type");
    }
#undef sig
    h.fill1[0] = 1;
    strncpy(h.folder, folder?folder:"main", 8);
    h.fill2[0] = 1; h.fill2[2] = 0x52;
    strncpy(h.name, name, 8);
    h.type[0] = tigl_type; h.type[2] = 3;
    h.fill3[0] = 0xA5; h.fill3[1] = 0x5A;
    {
	unsigned long oncalc_size = 88+content_size+2;
	put_little_endian(h.size, oncalc_size, 4);
    }
    put_big_endian(h.datasize, content_size, 2);

    if (content_size>=65520)
	fatal("TIOS variables can't be more than 65520 bytes long.");

    {
	char *ret = malloc(sizeof(h)+content_size+2);
	if (!ret)
	    fatal("Memory");
	if (ti_file_size)
	    *ti_file_size = sizeof(h)+content_size+2;
	memcpy(ret, &h, sizeof(h));
	memcpy(ret+sizeof(h), content, content_size);
	{
	    unsigned int crc = h.datasize[0]+h.datasize[1];
	    unsigned long n = content_size;
	    while (n--)
		crc += 0xff & (unsigned char)*content++;
	    put_little_endian(ret+sizeof(h)+content_size, crc, 2);
	}
	return ret;
    }
}
