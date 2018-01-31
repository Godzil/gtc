/*
 * GTools C compiler
 * =================
 * source file :
 * memory management
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
#define USE_MEMMGT
#include	"cglbdec.h"

int		glbsize CGLOB,	/* size left in current global block */
		locsize CGLOB,	/* size left in current local block */
		tmpsize CGLOB,	/* size left in current temp block */
		glbindx CGLOB,	/* global index */
		locindx CGLOB,	/* local index */
		tmpindx CGLOB;	/* temp index */

int 	temp_mem CGLOB,temp_local CGLOB;
#ifdef MIN_TEMP_MEM
int		min_temp_mem CGLOB;
#endif

#ifdef PC
int glbmem CGLOB,locmem CGLOB;
#endif

int		max_mem CGLOB,	/* statistics... */
		glo_mem CGLOB;

struct blk *locblk CGLOB,	/* pointer to local block list */
	   *glbblk CGLOB,		/* pointer to global block list */
	   *tmpblk CGLOB;		/* pointer to temporary block list */

#ifdef PC
//void *calloc();
#endif
static void _err_attr error_memory() {
	uerrc2("not enough memory to compile function '%s'",func_sp->name);
}
int *_xalloc(int siz) {
	struct blk *bp;
	char *rv;
	if (temp_local)	/* this is NOT perfect, but it's close to being so :) */
		goto glb_xalloc;
/*	if (locmem>=50000)
		bkpt();*/
/*	if (siz>=1000)
		bkpt();*/
#ifdef MIN_TEMP_MEM
	if (temp_mem<=min_temp_mem) {
#else
	if (!temp_mem) {
#endif
	glb_xalloc:
		if (global_flag) {
	#ifdef PC
			glbmem += siz;
	#endif
			if (glbsize >= siz) {
				rv = &(glbblk->m[glbindx]);
				glbsize -= siz;
				glbindx += siz;
/*				if ((long)rv==0x789388)
					bkpt();*/
				return (int *) rv;
			} else {
	//			bp = (struct blk *) calloc(1, (int) sizeof(struct blk));
				bp = (struct blk *) malloc(sizeof(struct blk));
				if (!bp) {
#ifdef PC
					msg("not enough memory.\n");
					_exit(1);
#else
					error_memory();
					fatal("Not enough global memory");
#endif
				}
				memset(bp, 0, sizeof(struct blk));
				glo_mem++;
				bp->next = glbblk;
				glbblk = bp;
				glbsize = BLKLEN - siz;
				glbindx = siz;
				return (int *) glbblk->m;
			}
		} else {						/* not global */
	#ifdef PC
			locmem += siz;
	#endif
			if (locsize >= siz) {
				rv = &(locblk->m[locindx]);
				locsize -= siz;
				/*if ((long)rv==0x7f86bc)
					bkpt();*/
				locindx += siz;
	#ifdef PC
				if (0x80000000&(long)rv)
					fatal("A DIRTY HACK FAILED ON YOUR CONFIG. "
						"LOOK AT OUT68K_AS.C TO SOLVE THE PROBLEM");
	#endif
/*				if (func_sp) infunc("chk_curword")
					if ((long)rv==0x7b4bf8)
						bkpt();*/
/*				if ((long)rv==0x7e5cd0)
					bkpt();*/
				return (int *) rv;
			} else {
	//			bp = (struct blk *) calloc(1, (int) sizeof(struct blk));
				bp = (struct blk *) malloc(sizeof(struct blk));
				if (!bp) {
#ifdef PC
					msg("not enough local memory.\n");
					_exit(1);
#else
					error_memory();
					fatal("Not enough local memory");
#endif
				}
				memset(bp, 0, sizeof(struct blk));
				bp->next = locblk;
				locblk = bp;
				locsize = BLKLEN - siz;
				locindx = siz;
				return (int *) locblk->m;
			}
		}
	} else {
		if (tmpsize >= siz) {
			rv = &(tmpblk->m[tmpindx]);
			tmpsize -= siz;
			tmpindx += siz;
			return (int *) rv;
		} else {
//			bp = (struct blk *) calloc(1, (int) sizeof(struct blk));
			bp = (struct blk *) malloc(sizeof(struct blk));
			if (!bp) {
#ifdef PC
				msg("not enough temporary memory.\n");
				_exit(1);
#else
				error_memory();
				fatal("Not enough temporary memory");
#endif
			}
/*			if (bp==0x789364)
				bkpt();*/
			memset(bp, 0, sizeof(struct blk));
			bp->next = tmpblk;
			tmpblk = bp;
			tmpsize = BLKLEN - siz;
			tmpindx = siz;
			return (int *) tmpblk->m;
		}
	}
}

int blk_free(struct blk *bp1) {
	int blkcnt = 0;
	struct blk *bp2;
	while (bp1) {
		bp2 = bp1->next;
		(void) free((char *) bp1);
		blkcnt++;
		bp1 = bp2;
	}
	return blkcnt;
}

#ifdef DUAL_STACK
typedef struct _ds_block {
	void *lo,*hi;
	struct _ds_block *pop;
	void *popstackptr;
} DS_BLOCK;
static DS_BLOCK *ds_current CGLOB;
void *dualstack CGLOB;
void *ds_currentlo CGLOB,*ds_currenthi CGLOB;

#ifdef PC
int n_ds_allocations CGLOB;
#endif
void ds_allocatleast(unsigned int size) {
	size+=DS_BSIZE;
	DS_BLOCK *ds_new = malloc(sizeof(DS_BLOCK)+size);
	ds_new->lo = (void *)(ds_new+1);
	ds_new->hi = (void *)((char *)ds_new->lo+size);
	ds_new->pop = ds_current;
	ds_new->popstackptr = dualstack;
	ds_current = ds_new;
	ds_currentlo = ds_current->lo;
	ds_currenthi = ds_current->hi;
	dualstack = ds_currentlo;
#ifdef PC
	n_ds_allocations++;
#endif
}
void ds_free(void) {
	DS_BLOCK *ds_old = ds_current;
	ds_current = ds_old->pop;
	ds_currentlo = ds_current ? ds_current->lo : 0;
	ds_currenthi = ds_current ? ds_current->hi : 0;
	dualstack = ds_old->popstackptr;
	free(ds_old);
}
void rel_dualstack(void) {
	while (ds_current)
		ds_free();
#ifdef PC
	//msg2("  performed %d dual-stack-related allocations\n",n_ds_allocations);
#endif
}
#endif

#define VERBOSE
#ifdef GARBAGE_COLLECT
void d_enode(struct enode **node) {
	struct enode *dest,*ep=*node;
	if (!ep) return;
	if (ep->nodetype==en_icon) { *node=mk_icon(ep->v.i); return; }
	dest=xalloc((int)sizeof(struct enode), ENODE);
	memcpy(dest,ep,sizeof(struct enode));
	*node=dest;
	switch (ep->nodetype) {
		case en_land: case en_lor:
		case en_asand: case en_asor: case en_asxor:
		case en_aslsh: case en_asrsh:
		case en_asmul: case en_asmod: case en_asdiv:
		case en_asadd: case en_assub:
		case en_gt: case en_ge:
		case en_lt: case en_le:
		case en_eq: case en_ne:
		case en_cond: case en_assign:
		case en_and: case en_or: case en_xor:
		case en_lsh: case en_rsh:
		case en_mul: case en_mod: case en_div:
		case en_add: case en_sub:
		case en_fcall: case en_void:
			d_enode(&ep->v.p[1]);
		case en_deref:
		case en_ainc: case en_adec:
		case en_uminus: case en_not: case en_compl:
		case en_ref:
		case en_cast:	/* hum hum */
		case en_fieldref:
			d_enode(&ep->v.p[0]);
		case en_tempref:
		case en_autocon:
		case en_labcon:
		case en_nacon:
		case en_fcon:
			return;
		case en_compound:
			d_snode(&ep->v.st);
			return;
		default:
			ierr(D_ENODE,1);
	}
}

void d_snode(struct snode **node) {
	struct snode *dest,*block=*node;
	if (!block) return;
	dest=xalloc((int)sizeof(struct snode), SNODE);
	memcpy(dest,block,sizeof(struct snode));
	*node=dest;
	while (block != 0) {
		switch (block->stype) {
		  case st_return:
		  case st_expr:
			d_enode(&block->exp);
			break;
		  case st_loop:
			d_enode(&block->exp);
			d_snode(&block->s1);
			d_enode(&block->v2.e);
			break;
		  case st_while:
		  case st_do:
			d_enode(&block->exp);
			d_snode(&block->s1);
			break;
		  case st_for:
			d_enode(&block->exp);
			d_enode(&block->v1.e);
			d_snode(&block->s1);
			d_enode(&block->v2.e);
			break;
		  case st_if:
			d_enode(&block->exp);
			d_snode(&block->s1);
			d_snode(&block->v1.s);
			break;
		  case st_switch:
			d_enode(&block->exp);
			d_snode(&block->v1.s);
			break;
		  case st_case:
		  case st_default:
			d_snode(&block->v1.s);
			break;
		  case st_compound:
		  case st_label:
			d_snode(&block->s1);
		  case st_goto:
		  case st_break:
		  case st_continue:
			break;
		  case st_asm:
			d_amode(&(struct amode *)block->v1.i);
			break;
		  default:
			ierr(D_SNODE,1);
		}
		block = block->next;
	}
}

void d_amode(struct amode **node) {
	struct amode *dest,*mode=*node;
	if (!mode) return;
	dest=xalloc((int)sizeof(struct amode), AMODE);
	memcpy(dest,mode,sizeof(struct amode));
	*node=dest;
	d_enode(&dest->offset);
}
extern struct ocode *_peep_head;
void d_ocodes(void) {
	struct ocode *dest=NULL,**node=&_peep_head,*instr;
	while ((instr=*node)) {
		instr->back=dest;
		dest=xalloc((int)sizeof(struct ocode), OCODE);
		memcpy(dest,instr,sizeof(struct ocode));
		*node=dest;
		d_amode(&dest->oper1);
		d_amode(&dest->oper2);
		node=&dest->fwd;
	}
	*node=NULL;
}

extern struct snode *dump_stmt;
void collect(int g) {
	struct blk *blk=glbblk;
	glbsize=glbindx=0;
	glbblk=NIL_BLK;
	alloc_dump(g);
	// dump snodes & most enodes
//	d_snode(&dump_stmt);
	// dump ocodes & amodes & the rest of enodes
//	d_ocodes();
	// dump most syms & most strs & typs & tables & the rest of syms & a few strs
	d_table(&gsyms); d_table(&gtags); d_table(&defsyms);
	// dump slits & the rest of strs
//	
	// no dump is needed for cse's since it is a cooperative garbage-collection
}
#endif

int _k_=0;
void tmp_use() {
	temp_mem++;
	_k_++;
}
void tmp_free() {
	if (!--temp_mem && tmpblk) {
#ifdef PC
		int n=0;
#endif
#ifndef LIFO_TMP_FREE
		/* Maybe I'm wrong, but I think that this solution
		 * is quite bad for the TIOS heap manager... */
		struct blk *nxt;
		while ((nxt=tmpblk->next))
	#ifdef PC
			n++,
	#endif
				free(tmpblk),tmpblk=nxt;
#else
		struct blk *nxt=tmpblk->next,*tofree;
		while (nxt)
	#ifdef PC
			n++,
	#endif
				tofree=nxt, nxt=nxt->next, free(tofree);
#endif
#ifdef _DBG_SHOW_TEMPMEM
		printf("*");
#ifdef PC
		printf("(%dk)",n*BLKLEN/1024);
//		getchar();
#endif
#endif
		tmpsize = BLKLEN;
		tmpindx = 0;
		memset(tmpblk,0,BLKLEN);
		/*blk_free(tmpblk);
		tmpblk = 0;
		tmpsize = 0;*/
	}
}

void rel_local() {
	unsigned int mem;
#ifndef AS
#define pos 0
#else
	extern unsigned int pos;
#endif
#ifdef PC
#ifdef GARBAGE_COLLECT
	collect(0);
#endif
#ifdef LISTING
	alloc_dump(0);
#endif
#endif
	if ((mem=blk_free(locblk))+glo_mem+(2*pos)/BLKLEN > (unsigned int)max_mem) {
		max_mem = mem+glo_mem+(2*pos)/BLKLEN;
		/*if (max_mem>330/4)
			bkpt();*/
	}
#ifndef AS
#undef pos
#endif
#ifdef PC
	locmem = 0;
#endif
	locblk = 0;
	locsize = 0;
//#ifndef PC
#ifndef GTDEV
#ifdef PC
	if (verbose)
#endif
		msg2("%d kb\n", (int)(mem * BLKLEN/1024));
#endif
//#endif
#ifdef VERBOSE
#ifdef LISTING
#if 0
	if (list_option)
		msg2(" releasing %2d kb local tables.\n",
				mem * BLKLEN/1024);
#endif
#endif
#endif
}

void rel_global() {
	int mem;
	if ((mem=blk_free(glbblk)) > max_mem)
		max_mem = mem;
#ifdef PC
	glbmem = 0;
#endif
	glo_mem = 0;
	glbblk = 0;
	glbsize = 0;
	blk_free(tmpblk);
	tmpblk = 0;
	tmpsize = 0;
#ifdef PC
#ifdef LISTING
	alloc_dump(1);
#endif
#endif
#ifdef VERBOSE
#ifdef LISTING
	if (list_option)
		msg2(" releasing %2d kb global tables.\n",
				mem * BLKLEN/1024);
#endif
#ifdef DUAL_STACK
	//if (ds_current)
	//	msg("oops, a dual stack remains\n");
	rel_dualstack();
#endif
#ifndef GTDEV
#ifdef PC
	if (verbose) {
		msg2("Max memory request : %d kb\n",
				(int)(max_mem * BLKLEN/1024));
		msg("\n");
		if (max_mem * BLKLEN/1024<150)
			msg("On-calc portability : very good\n");
		else if (max_mem * BLKLEN/1024<230)
			msg("On-calc portability : good\n");
		else if (max_mem * BLKLEN/1024<290)
			msg("On-calc portability : questionable\n");
		else if (max_mem * BLKLEN/1024<360)
			msg("On-calc portability : difficult without splitting\n");
		else
			msg("On-calc portabilty : impossible without splitting\n");
		msg("\n");
	}
#endif
#endif
#endif
	max_mem = 0;
}

void clean_up() {
	blk_free(tmpblk);
	blk_free(locblk);
	blk_free(glbblk);
#ifdef DUAL_STACK
	//if (ds_current)
	//	msg("oops, a dual stack remains\n");
	rel_dualstack();
#endif
	temp_mem = 0;
	glbblk = locblk = tmpblk = 0;
/*#ifdef OPTIMIZE_BSS
	free(bssdata);
#endif*/
}

#undef VERBOSE
// vim:ts=4:sw=4
