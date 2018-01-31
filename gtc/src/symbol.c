/*
 * GTools C compiler
 * =================
 * source file :
 * symbol handling (insertion & search)
 *
 * Copyright 2001-2004 Paul Froissart.
 * Credits to Christoph van Wuellen and Matthew Brandt.
 * All commercial rights reserved.
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 */

#include	"define.h"
_FILE(__FILE__)
#include	"c.h"
#include	"expr.h"
#include	"gen.h"
#include	"cglbdec.h"
#ifdef PCH
#include	"pch.h"
#endif

#ifdef EXE_OUT
#ifndef AS
int exestub_mode CGLOB;
#endif
#endif

HTABLE	gsyms CGLOBL, gtags CGLOBL, lsyms CGLOBL, labsyms CGLOBL, ltags CGLOBL;

void concat(TABLE *dest,TABLE *src) {
	SYM *sp=src->tail,*sp2;
	while (sp) {
		sp2=(SYM *)xalloc((int)sizeof(SYM), _SYM+SYMBOL);
		*sp2=*sp;
		insert(sp2,(HTABLE *)dest);
		sp=sp->prev;
	}
}

void hashinit(HTABLE *t) {
	t->hash=0x2489;
	memset(t->h,0,sizeof(t->h));
}

void insert(SYM *sp,HTABLE *table) {
/*	if (!strcmp(sp->name,"wrong_calc"))
		uwarn("def!");*/
	if (!table->hash) {
		if (!search(sp->name,0,table)) {
			TABLE *tab=(TABLE *)table;
			if (!tab->head) {
				tab->head = tab->tail = sp;
				sp->prev = 0;
				sp->used = 0;
			} else {
				tab->tail->next = sp;
				sp->prev = tab->tail;
				tab->tail = sp;
				sp->used = 0;
				tab->tail = sp;
			}
			sp->next = 0;
		}
		else uerr(ERR_DUPSYM,sp->name);
	} else {
		int crc=crcN(sp->name);
#ifdef EXE_OUT
		if (table==&defsyms && sp->name[0]=='E' && !strcmp(sp->name,"EXE_OUT"))
			exestub_mode=1;
#endif
		if (!search(sp->name,crc,table)) {
			struct htab *tab=&table->h[crc];
			if (!tab->head) {
				tab->head = tab->tail = sp;
				sp->prev = 0;
				sp->used = 0;
			} else {
				tab->tail->next = sp;
				sp->prev = tab->tail;
				tab->tail = sp;
				sp->used = 0;
				tab->tail = sp;
			}
			sp->next = 0;
		}
		else uerr(ERR_DUPSYM,sp->name);
	}
}

struct sym *symremove(char *na, HTABLE *tab) {
	SYM *ttail,**prv;
	struct htab *root;
	char  *s1,*s2,c;
	if (!tab->hash)
		ierr(TABLE_HASH,2);
#ifdef PC
	if (tab->hash!=0x2489)
		ierr(TABLE_HASH,1);
#endif
	prv=&((root=&tab->h[crcN(na)])->tail);
	while ((ttail=*prv)) {
		s1 = ttail->name;
		s2 = na;
		while (*s1++==(c=*s2++)) {
			if (!c) {
//				*prv = ttail->prev;
				if (ttail->next) ttail->next->prev = ttail->prev;
				if (ttail->prev) ttail->prev->next = ttail->next;
//				ttail->next = prv;
				if (root->tail==ttail) root->tail=ttail->prev;
				if (root->head==ttail) root->head=ttail->next;
/*				if (!root->tail)
					root->head=0;*/
				return ttail;
			}
		}
		prv = &(ttail->prev);
	}
	return 0;
}

#ifdef PC
int crcN(char *na) {
	unsigned long crc=-1; int n;
	unsigned char c;
	c=*na++;
	do {
		crc^=c;
		n=crc&7;
		crc=(crc>>n)+(crc<<(32-n));
	} while ((c=*na++));
	crc^=crc>>16;
	crc^=crc>>8;
	return crc & (N_HASH-1);
}
#else
int crcN(char *na);
asm("crcN:\n"
"	move.l	4(%sp),%a0\n"
"	moveq	#-1,%d0\n"
"	move.b	(%a0)+,%d1\n"
"crcN_loop:\n"
"	eor.b	%d1,%d0\n"
"	moveq	#7,%d1\n"
"	and.w	%d0,%d1\n"
"	ror.l	%d1,%d0\n"
"	move.b	(%a0)+,%d1\n"
"	bne	crcN_loop\n"
"	move.w	%d0,%d1\n"
"	swap	%d0\n"
"	eor.w	%d1,%d0\n"
"	move.w	%d0,-(%sp)\n"
"	move.b	(%sp)+,%d1\n"
"	eor.b	%d1,%d0\n"
"	and.w	#" N_HASH_AND ",%d0\n"
"	rts");
#endif

/*#ifdef PC*/
struct sym *search(char *na, int crc, HTABLE *tab) {
	char  *s1,*s2,c;
	if (!tab->hash) {
		SYM *ttail=((TABLE *)tab)->tail;
		while (ttail) {
			s1 = ttail->name;
			s2 = na;
			while (*s1++==(c=*s2++)) {
				if (!c) return ttail;
			}
			ttail = ttail->prev;
		}
		return 0;
	} else {
		SYM *ttail;
#ifdef PC
		if (tab->hash!=0x2489)
			ierr(TABLE_HASH,1);
#endif
		if (crc<0) crc=crcN(na);
		ttail=tab->h[crc].tail;
		while (ttail) {
			s1 = ttail->name;
			s2 = na;
			while (*s1++==(c=*s2++)) {
				if (!c) return ttail;
			}
			ttail = ttail->prev;
		}
		return 0;
	}
}
/*#else
asm("search:
	move.l	%a2,-(%sp)
	move.l	8(%sp),%d1
	move.l	%d1,%a1
	tst.b	(%a1)
0:	beq		0b
	move.l	12(%sp),%a0
	move.l	%a0,%d0
	beq	search_end
search_lp:
	move.l	(%a0)+,%a2
	move.l	%d1,%a1
	cmpm.b	(%a1)+,(%a2)+
	bne	search_nxt
search_lp2:
	move.b	(%a1)+,%d0
	beq	search_fnd
	cmp.b	(%a2)+,%d0
	beq	search_lp2
search_nxt:
	move.l	(%a0)+,%a0
	move.l	%a0,%d0
	bne	search_lp
search_end:
	move.l	(%sp)+,%a2
	rts
search_fnd:
	tst.b	(%a2)+
	bne		search_nxt
	subq.l	#4,%a0
	move.l	(%sp)+,%a2
	rts");
#endif*/

struct sym *gsearch(char *na,int crc) {
	struct sym	   *sp;
	if (!(sp = search(na, crc, &lsyms)))
		sp = search(na, crc, &gsyms);
	return sp;
}

void append(struct sym **ptr_sp, HTABLE *table) {
	struct sym	   *sp1, *sp = *ptr_sp;
	if (table == &gsyms && (sp1 = search(sp->name, -1, table)) != 0) {
		/*
		 * The global symbol table has only one level, this means that we
		 * only check if the new declaration is compatible with the old one
		 */

		if (!eq_type(sp->tp, sp1->tp))
			uerr(ERR_REDECL,sp->name);
		/*
		 * The new storage class depends on the old and on the new one.
		 */
		if (sp->storage_class == sp1->storage_class) {
			if (sp->storage_class == sc_global) {
				/*
				 * This hack sets sp->used to -1 so that decl.c knows to
				 * suppress storage allocation
				 */
				uwarn("global redeclaration of '%s'", sp->name);
				sp1->used = -1;
			}
			*ptr_sp = sp1;		/* caller uses old entry */
			return;
		}
		/*
		 * since we use compiler generated label for static data, we must
		 * retain sc_static
		 */
		if (sp1->storage_class == sc_static) {
			*ptr_sp = sp1;		/* caller uses old entry */
			return;
		}
		/*
		 * if the new storage class is global, we must update sp1 to generate
		 * the .globl directive at the very end and perhaps the size (e.g.
		 * for arrays)
		 */
		if (sp->storage_class == sc_global) {
			sp1->storage_class = sc_global;
			sp1->tp = sp->tp;
			*ptr_sp = sp1;		/* caller uses old entry */
			return;
		}
		/*
		 * if the new storage class is static, set it to global (since we may
		 * have used the ,real' name and cannot use compiler generated names
		 * for this symbol from now on) and set sp->value.i to -1 to prevent
		 * it from being exported via .globl directives
		 */
		if (sp->storage_class == sc_static) {
			sp1->storage_class = sc_global;
			sp1->value.i = -1;
			*ptr_sp = sp1;		/* caller uses old entry */
			return;
		}
		/*
		 * last case: global declaration followed by external decl.: just do
		 * nothing
		 */
		*ptr_sp = sp1;			/* caller uses old entry */
		return;
	}
/*	if (table->head == 0) {
		// The table is empty so far...
		table->head = table->tail = sp;
		sp->next = sp->prev = 0;
		sp->used = 0;
	} else {
		table->tail->next = sp;
		sp->prev = table->tail;
		table->tail = sp;
		sp->next = 0;
		sp->used = 0;
	}*/
	if (!table->hash) {
		TABLE *tab=(TABLE *)table;
		if (!tab->head) {
			tab->head = tab->tail = sp;
			sp->prev = 0;
			sp->used = 0;
		} else {
			tab->tail->next = sp;
			sp->prev = tab->tail;
			tab->tail = sp;
			sp->used = 0;
			tab->tail = sp;
		}
		sp->next = 0;
	} else {
		struct htab *tab=&table->h[crcN(sp->name)];
		if (!tab->head) {
			tab->head = tab->tail = sp;
			sp->prev = 0;
			sp->used = 0;
		} else {
			tab->tail->next = sp;
			sp->prev = tab->tail;
			tab->tail = sp;
			sp->used = 0;
			tab->tail = sp;
		}
		sp->next = 0;
	}
}
// vim:ts=4:sw=4
