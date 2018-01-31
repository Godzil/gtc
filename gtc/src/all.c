/*
 * GTools C compiler
 * =================
 * source file :
 * (on-calc) main file
 *
 * Copyright 2001-2004 Paul Froissart.
 * Credits to Christoph van Wuellen and Matthew Brandt.
 * All commercial rights reserved.
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 */

#define DECLARE
#define FLASH_VERSION
#define GTDEV
#include	"define.h"
#ifdef OPTIMIZE_BSS
register void *bssdata asm("a5");
#endif
#define dseg() while (0)
#define cseg() while (0)
#define nl() while (0)
#define compound_done getsym
#include	"c.h"
#include	"expr.h"
#include	"gen.h"
#include	"cglbdec.h"

#include	"flashhdr.h"
#ifdef GTDEV
#include "GtDevComm.h"
#endif
#include	"flashhdr.c"

#ifndef debug
void debug(int c) {
	printf("%c ",c);
	ngetchx();
}
#endif

#undef exit
#define exit(k) asm(" move.l exit_a7,%sp\n rts")
extern char *curname;
extern void fatal(char *s) __attribute__ ((noreturn));

#include "out68k_as.c"

#include "decl.c"
#include "expr.c"
#ifdef ASM
#include "getasm.c"
#endif
#include "getsym.c"
#include "init.c"
#include "intexpr.c"
//#include "memmgt.c"	// xalloc is common to both modules
#include "preproc.c"
#include "searchkw.c"
#include "stmt.c"
#include "symbol.c"

void *exit_a7 CGLOB;
#if 0
void _exit(int code) {
	clean_up();
	printf("\nerror code %d",code);
	while (!kbhit());
	asm(" move.l exit_a7,%sp; rts");
//	exit(0);
}
#endif

void _noreturn assert_terminated();
#ifdef GTDEV
void fatal(char *message) {
	clean_up();
	msg_process(message, ET_FATAL, "", "", -1, -1);
	exit(0);
	assert_terminated();
}
#else
void fatal(char *message) {
	clean_up();
	msg2("Fatal error!\n%s\n", message);
//	msg("There may be a syntax error or a compiler bug.\n");
//	while (!kbhit());
	ngetchx();
	asm(" move.l exit_a7,%sp; rts");
	assert_terminated();
//	exit(0);
}
#endif

void fatal(char *s) __attribute__ ((noreturn));

#define mk_node near_mk_node
#define mk_icon near_mk_icon
struct enode *mk_node(enum(e_node) nt, struct enode *v1, struct enode *v2) {
/*
 * build an expression node with a node type of nt and values v1 and v2.
 */
	struct enode   *ep;
	ep = (struct enode *) xalloc((int) sizeof(struct enode), ENODE+MK_NODE);
	ep->nodetype = nt;
	ep->etype = bt_void;
	ep->esize = -1;
	ep->v.p[0] = v1;
	ep->v.p[1] = v2;
	return ep;
}

struct enode *mk_icon(long i) {
/*
 * build an expression node forming an integer constant
 */
	struct enode   *ep;
//	ep = (struct enode *) xalloc((int) sizeof(struct enode), ENODE+MK_ICON);
	ep = (struct enode *) xalloc((int) sizeof(struct xcon), ENODE+MK_ICON);
	ep->nodetype = en_icon;
	ep->etype = bt_void;
#ifdef NO_CALLOC
	ep->esize = 0;
#endif
	ep->v.i = i;
	return ep;
}

#include "analyze.c"
#include "func.c"
#include "gen68k.c"
#include "genffp.c"
#include "genstmt.c"
#include "memmgt.c"	// xalloc is common to both modules
#include "optimize.c"
#include "peep68k.c"
#include "reg68k.c"

/* this is reimplemented because old versions of TIGCC didn't check for _F_WRIT */
void my_fclose(FILE *f) {
	if (f->flags&_F_WRIT)
		HeapRealloc(f->handle,*(unsigned short*)f->base+2);
	HeapUnlock(f->handle);
	free(f);
}

asm("bcopy:\n"
"	move.w (12,%sp),-(%sp)\n"
"	clr.w -(%sp)\n"
"	move.l (8,%sp),-(%sp)\n"
"	move.l (16,%sp),-(%sp)\n"
"	movea.l 0xC8,%a0\n"
"	movea.l (%a0,0x26A*4),%a0\n"
"	jsr (%a0)                  /* memcpy */\n"
"	lea (%sp,12),%sp\n"
"	rts\n"

"__mulsi3:\n"
"	move.l (4,%sp),%d1\n"
"	move.l (8,%sp),%d2\n"
"	move.l %d2,%d0\n"
"	mulu %d1,%d0\n"
"	swap %d2\n"
"	mulu %d1,%d2\n"
"	swap %d1\n"
"	mulu (10,%sp),%d1\n"
"	add.w %d1,%d2\n"
"	swap %d2\n"
"	clr.w %d2\n"
"	add.l %d2,%d0\n"
"	rts\n"

"__modsi3:\n"
"	move.w #0x2A9*4,%d2        /* _ms32s32 */\n"
"	bra.s __div_entry\n"
"__divsi3:\n"
"	move.w #0x2A8*4,%d2        /* _ds32s32 */\n"
"	bra.s __div_entry\n"
"__umodsi3:\n"
"	move.w #0x2A9*4,%d2        /* _mu32u32 */\n"
"	bra.s __div_entry\n"
"__udivsi3:\n"
"	move.w #0x2A8*4,%d2        /* _du32u32 */\n"
"	bra.s __div_entry\n"
"	nop\n"
"__div_entry:\n"
"	move.l (4,%sp),%d1\n"
"	move.l (8,%sp),%d0\n"
"	movea.l 0xC8,%a0\n"
"	movea.l (%a0,%d2.w),%a0\n"
"	jsr (%a0)\n"
"	move.l %d1,%d0\n"
"	rts");

#include	"cmain.c"

#include	"flashend.c"
// vim:ts=4:sw=4
