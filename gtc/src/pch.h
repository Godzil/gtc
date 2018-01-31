/*
 * GTools C compiler
 * =================
 * source file :
 * PCH management definitions
 *
 * Copyright 2001-2004 Paul Froissart.
 * Credits to Christoph van Wuellen and Matthew Brandt.
 * All commercial rights reserved.
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 */

#ifndef PCH_H
#define PCH_H

typedef struct {
	TI_LONG magic;
	TI_SHORT h_off;	// offset to .H section
	TI_SHORT ext_off;	// offset to .EXT table (int ext_table[nExt])
	TI_SHORT dic_off;	// offset to dictionnary ((char[]) sPk_table[])
	TI_SHORT nID;
} PCH_HEAD;

#define PCH_HEAD_SIZE 12

#define PCHID_MASK 0xFFF
#define PCHID_MACRO 0x8000
#define PCHID_VAMAC 0x4000
#define PCHID_PACKED 0x2000

#ifndef PC
char *__attribute__((stkparm)) sUnpack(char *in,char *out,char *dic);
#else
char *sUnpack(char *in,char *out,char *dic);
#endif

/* GTC-only */
extern FILE			*pchfile[];
extern char			*pchdata[];
extern char			*pchtab[];
extern char			pchname[][15];
extern char         pchrequired[];
extern int 			pchnum;
#define pchhead ((PCH_HEAD **)pchdata)

/* PchMaker-only */
extern int			def_is_packed;

#endif
// vim:ts=4:sw=4
