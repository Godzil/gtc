/*
 * GTools C compiler
 * =================
 * source file :
 * analyzer
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
//#undef OPT_ROMCALLS

/*
 * this module will step through the parse tree and find all optimizable
 * expressions. at present these expressions are limited to expressions that
 * are valid throughout the scope of the function. the list of optimizable
 * expressions is:
 *
 * constants
 * global and static addresses
 * auto addresses
 * contents of auto addresses.
 *
 * contents of auto addresses are valid only if the address is never referred to
 * without dereferencing.
 *
 * scan will build a list of optimizable expressions which opt1 will replace
 * during the second optimization pass.
 */

void scan(struct snode *block);
void repcse(struct snode *block);

#define INIT_EXEC_COUNT 16
FILE		   *list CGLOB;
int 			regs_used CGLOB;
struct enode   *regexp[REGEXP_SIZE] CGLOBL;
xstatic unsigned int exec_count CGLOB;
xstatic struct cse *olist CGLOB;		/* list of optimizable expressions */

int equalnode(struct enode *node1, struct enode *node2) {
/*
 * equalnode will return 1 if the expressions pointed to by node1 and node2
 * are equivalent.
 */
	if (node1 == 0 || node2 == 0)
		return 0;
	if (node1->nodetype != node2->nodetype)
		return 0;
	switch (node1->nodetype) {
#ifndef AS
	  case en_labcon:
		  /* FALLTHROUGH  -  HACK : v.i==(long)v.ensp */
	  case en_icon:
#else
	  case en_labcon:
		return (node1->v.enlab == node2->v.enlab);
	  case en_icon:
#endif
	  case en_autocon:
	  case en_tempref: // note: this one is not necessary when doing CSE, but can be useful in gen68k.c
		/*infunc("chk_curword")
			if (node2->v.i=(long)(char)0xCD)
				bkpt();*/
		return (node1->v.i == node2->v.i);
	  case en_nacon:
		return (!strcmp(node1->v.ensp, node2->v.ensp));
	  case en_ref:
	  case en_fieldref:
		return equalnode(node1->v.p[0], node2->v.p[0]);
	  default:
		return 0;
	}
}


static struct cse *searchnode(struct enode *node) {
/*
 * searchnode will search the common expression table for an entry that
 * matches the node passed and return a pointer to it.
 * the top level of equalnode is code inline here for speed
 */
	register struct cse *csp;
	register struct enode *ep;
	if (node == 0)
		return 0;
	csp = olist;
	while (csp != 0) {
		ep = csp->exp;
		if (ep != 0 && node->nodetype == ep->nodetype) {
			switch (node->nodetype) {
			  case en_icon:
#ifndef AS
			  case en_labcon:
#endif
			  case en_autocon:
				if (node->v.i == ep->v.i)
					return csp;
				break;
#ifndef AS
			  case en_nacon:
				if (!strcmp(node->v.ensp, ep->v.ensp))
					return csp;
				break;
#endif
#ifdef AS
			  case en_labcon:
			  case en_nacon:
				if (node->v.enlab == ep->v.enlab)
					return csp;
				break;
#endif
			  case en_ref:
				if (equalnode(node->v.p[0], ep->v.p[0])) {
					/* 05/09/03: added check for size ; struct or unions
					 *           won't work either since Gen68k needs their address
					 */
					if (node->esize != ep->esize || bt_aggregate(node->etype))
						csp->voidf=1;
					return csp;
				}
				break;
			}
		}
		csp = csp->next;
	}
	return 0;
}

struct enode *copynode(struct enode *node) {
/*
 * copy the node passed into a new enode so it wont get corrupted during
 * substitution.
 */
	struct enode   *temp;
	if (node == 0)
		return 0;
	temp = (struct enode *) xalloc((int) sizeof(struct enode), ENODE+COPYNODE);
	*temp = *node;
	return temp;
}



static struct cse *enternode(struct enode *node, int duse) {
/*
 * enternode will enter a reference to an expression node into the common
 * expression table. duse is a flag indicating whether or not this reference
 * will be dereferenced.
 */
	struct cse	   *csp;
	if ((csp = searchnode(node)) == 0) {		/* add to tree */
		csp = (struct cse *) xalloc((int) sizeof(struct cse), CSE+ENTERNODE);
		csp->next = olist;
#ifdef NO_CALLOC
		csp->uses = 0;
		csp->duses = 0;
		csp->voidf = 0;
#endif
		csp->exp = copynode(node);
		olist = csp;
		if (bt_aggregate(node->etype))
			csp->voidf++;
	} else {
		/*
		 * Integer constants may be in the table with different sizes -- keep
		 * the maximum size
		 */
		if (node->nodetype == en_icon && node->esize > csp->exp->esize)
			csp->exp->esize = node->esize;
	}
/*	infunc("chk_curword")
		if (node->nodetype != en_icon && node->esize==1)
			bkpt();*/
#ifdef REGALLOC_FOR_SIZE
#define exec_count INIT_EXEC_COUNT
#endif
	csp->uses+=exec_count;
	if (duse)
		csp->duses+=exec_count;
#ifdef REGALLOC_FOR_SIZE
#undef exec_count
#endif
	return csp;
}

static struct cse *voidauto(struct enode *node) {
/*
 * voidauto will void an auto dereference node which points to the same auto
 * constant as node.
 */
	struct cse	   *csp;
	csp = olist;
	while (csp != 0) {
		if (csp->exp->nodetype==en_ref && equalnode(node, csp->exp->v.p[0])) {
			if (csp->voidf)
				return 0;
			csp->voidf = 1;
			return csp;
		}
		csp = csp->next;
	}
	return 0;
}

void scanexpr(struct enode *node, int duse) {
/*
 * scanexpr will scan the expression pointed to by node for optimizable
 * subexpressions. when an optimizable expression is found it is entered into
 * the tree. if a reference to an autocon node is scanned the corresponding
 * auto dereferenced node will be voided. duse should be set if the
 * expression will be dereferenced.
 */
	struct cse	   *csp, *csp1;
	if (node == 0)
		return;
	switch (node->nodetype) {
	  case en_nacon:
#ifdef FLINE_RC
		if (fline_rc && node->v.ensp &&
			node->v.ensp[1]=='R' && node->v.ensp[0]=='_' &&
			!memcmp(node->v.ensp+2,"OM_CALL_",8))
			break;
#endif
		/*@fallthrough@*/
	  case en_labcon:
#ifndef NO_ICONST_SCAN
	  case en_icon:
#endif
		(void) enternode(node, duse);
		break;
	  case en_autocon:
		/*
		 * look if the dereferenced use of the node is in the list, remove it
		 * in this case
		 */
		if ((csp = voidauto(node)) != 0) {
			csp1 = enternode(node, duse);
			csp1->duses += csp->uses;
		} else if (duse>=0)				// don't enter it otherwise (we can already access it)
			(void) enternode(node, duse);
		break;

	  case en_ref:
	  case en_fieldref:
		if (node->v.p[0]->nodetype == en_autocon) {
			/*infunc("ChargerAdressesData")
				bkpt();*/
			{
			int first = (searchnode(node) == 0);
/*			infunc("chk_curword") if (node->v.p[0]->v.i==(long)(char)0xCD)
				bkpt();*/
			csp = enternode(node, duse);
			/*
			 * take care: the non-derereferenced use of the autocon node may
			 * already be in the list. In this case, set voidf to 1
			 */
			if (searchnode(node->v.p[0]) != 0) {
				csp->voidf = 1;
				scanexpr(node->v.p[0], 1);
			} else if (first) {
				/* look for register nodes */
				int 			i = 0;
				XLST_TYPE		j = (XLST_TYPE)node->v.p[0]->v.i, *p;
				p=reglst; while (i < regptr) {
					if (*p++ == j) {
						csp->voidf--;	/* this is not in auto_lst */
//						if (regalloc[i]>=0) csp->reg=regalloc[i];
						csp->uses += 256 * INIT_EXEC_COUNT * (100 - i);
						//csp->duses += 30 * (100 - i);	// so we know we don't always need an areg
						break;
					}
					++i;
				}
				/* set voidf if the node is not in autolst */
				csp->voidf++;
				i = autoptr;
				p=autolst;
				while (i--)
					if (*p++ == j) {
						csp->voidf--;
						break;
					}
				/*
				 * even if that item must not be put in a register,
				 * it is legal to put its address therein
				 */
				if (csp->voidf)
					scanexpr(node->v.p[0], 1);
			}
			}
#ifdef OPT_ROMCALLS
		} else if (node->v.p[0]->nodetype == en_icon && node->v.p[0]->v.i == 0xC8) {
			enternode(node, 1);
#endif
		} else {
			scanexpr(node->v.p[0], 1);
		}
		break;
	  case en_uminus:
	  case en_compl:
	  case en_ainc:
	  case en_adec:
	  case en_not:
	  case en_cast:
	  case en_deref:
		scanexpr(node->v.p[0], duse);
		break;
	  case en_alloca:
		scanexpr(node->v.p[0], 0);
		break;
	  case en_asadd:
	  case en_assub:
	  case en_add:
	  case en_sub:
#ifndef NO_IMPROVED_CSE_SCAN
		if (duse) {
			if (node->v.p[0]->etype==bt_pointer
				|| (node->v.p[1]->etype!=bt_pointer && node->v.p[0]->esize==4)
				|| (node->v.p[1]->esize!=4)) {
					if (node->v.p[1]->nodetype!=en_icon)
						scanexpr(node->v.p[0], 1),
						scanexpr(node->v.p[1], 0);
					else
						scanexpr(node->v.p[0], -1);
			} else scanexpr(node->v.p[0], 0), scanexpr(node->v.p[1], 1);
		} else
#endif
			scanexpr(node->v.p[0], 0), scanexpr(node->v.p[1], 0);
/*		scanexpr(node->v.p[0], duse);	// why ??
		scanexpr(node->v.p[1], duse);*/
		break;
	  case en_mul:
	  case en_div:
	  case en_lsh:
	  case en_rsh:
	  case en_mod:
	  case en_and:
	  case en_or:
	  case en_xor:
	  case en_lor:
	  case en_land:
	  case en_eq:
	  case en_ne:
	  case en_gt:
	  case en_ge:
	  case en_lt:
	  case en_le:
	  case en_asmul:
	  case en_asdiv:
	  case en_asmod:
	  case en_aslsh:
	  case en_asrsh:
	  case en_asand:
	  case en_asor:
	  case en_asxor:
	  case en_cond:
	  case en_void:
	  case en_assign:
		scanexpr(node->v.p[0], 0);
		scanexpr(node->v.p[1], 0);
		break;
	  case en_fcall:
		scanexpr(node->v.p[0], 1);
		scanexpr(node->v.p[1], 0);
		break;
	  case en_compound:
		scan(node->v.st);
		break;
	}
}

void scan(struct snode *block) {
/*
 * scan will gather all optimizable expressions into the expression list for
 * a block of statements.
 */
	while (block != 0) {
		unsigned int old=exec_count;
		switch (block->stype) {
		  case st_return:
		  case st_expr:
			opt4(&block->exp);
			scanexpr(block->exp, 0);
			break;
		  case st_loop:
			opt4(&block->exp);
			scanexpr(block->exp, 0);
			exec_count*=block->count;
			scanexpr(block->exp->v.p[0],0);	// increase desirability of counter reg allocation
			scan(block->s1);
			if (block->v2.e) {
				opt4(&block->v2.e);
				scanexpr(block->v2.e, 0);
			}
			break;
		  case st_while:
		  case st_do:
			exec_count*=block->count;
			exec_count++;
			opt4(&block->exp);
			scanexpr(block->exp, 0);
			exec_count--;
			scan(block->s1);
			break;
		  case st_for:
			opt4(&block->exp);
			scanexpr(block->exp, 0);
			exec_count*=block->count;
			opt4(&block->v1.e);
			scanexpr(block->v1.e, 0);
			scan(block->s1);
			opt4(&block->v2.e);
			scanexpr(block->v2.e, 0);
			break;
		  case st_if:
			opt4(&block->exp);
			scanexpr(block->exp, 0);
			exec_count=(unsigned int)(((unsigned long)exec_count*(unsigned long)block->count+32768)>>16);
			scan(block->s1);
			exec_count=old-exec_count;
			scan(block->v1.s);
			break;
		  case st_switch:
			opt4(&block->exp);
			scanexpr(block->exp, 0);
			exec_count>>=1;
			scan(block->v1.s);
			break;
		  case st_case:
		  case st_default:
			scan(block->v1.s);
			break;
		  case st_compound:
		  case st_label:
			scan(block->s1);
			break;
		}
		block = block->next;
		exec_count=old;
	}
}

unsigned long desire(struct cse *csp) {
/*
 * returns the desirability of optimization for a subexpression.
 */
#ifdef INFINITE_REGISTERS
	return 4294967295;
#endif
	if (csp->voidf || (csp->exp->nodetype == en_icon &&
//					   (unsigned int)csp->exp->v.i+8 <= 16))
					   csp->exp->v.i < 16 && csp->exp->v.i >= 0))
		return 0;
#ifdef REGPARM_OLD
			{ struct enode *ep;
	if (csp->exp->nodetype == en_ref && (ep=csp->exp->v.p[0])->nodetype == en_autocon
		&& ep->v.i>0 && (ep->v.i&REG_PARAMS))
		return 10000000;	// if it's a register param, then it *must* be in a long-live reg
			}
#endif
	if (g_lvalue(csp->exp))
		return 2 * csp->uses;
	return csp->uses;
}

void bsort(struct cse **list) {
/*
 * bsort implements a bubble sort on the expression list.
 */
	struct cse	   *csp1, *csp2;
	csp1 = *list;
	if (csp1 == 0 || csp1->next == 0)
		return;
	bsort(&(csp1->next));
	while (csp1 != 0 && (csp2 = csp1->next) != 0 && desire(csp1) < desire(csp2)) {
		*list = csp2;
		csp1->next = csp2->next;
		csp2->next = csp1;
		list = &(csp2->next);
	}
}

#ifdef REGPARM_OLD
extern struct typ *head;
#define parm_val (_parm_val+1)
XLST_TYPE	_parm_val[1+MAX_DATA+1+MAX_ADDR+1];
int			parm_reg[MAX_DATA+1+MAX_ADDR+1];
#endif
void allocate(void) {
/*
 * allocate will allocate registers for the expressions that have a high
 * enough desirability.
 */
	struct cse	   *csp;
	struct enode   *exptr/*,*ep*/;
	reg_t			datareg, addreg;
#ifdef AREG_AS_DREG
	int				areg_possible;
#endif
	unsigned int	mask, rmask;
	struct amode   *ap, *ap2;
//#define __DEBUG__
/*#ifdef __DEBUG__
	int i=0;*
#endif*/
	regs_used = 0;
	datareg = MAX_DATA+1;
	addreg = MAX_ADDR+1 + AREGBASE;
	mask = 0;
	rmask = 0;
	bsort(&olist);				/* sort the expression list */
	csp = olist;
/*	infunc("play")
		bkpt();*/ //autoptr=0,regptr=0;
	while (csp != 0) {
/*#ifdef __DEBUG__
	infunc("play")
		if (i++==2) {
			csp->voidf=1;
			regs_used++;
			rmask = rmask | (1 << (15 - datareg));
			mask = mask | (1 << datareg);
			datareg++;
		}
#endif*/
/*
 * If reg_option is not true, the 'desire' value must be at least
 * 5000, which I hope can only be achieved by the 'register' attribute
 */
		if (desire(csp) < 3*INIT_EXEC_COUNT || (!reg_option && desire(csp) < 5000))
			csp->reg = -1;
#ifndef AREG_AS_DREG
		else if (csp->duses &&
#else
		else if ((areg_possible=
#endif
				/* && (csp->duses * 3) > csp->uses */
				addreg <
							#ifdef USE_LINK
										FRAMEPTR
							#else
										STACKPTR - uses_link
							#endif
			/*
			 * integer constants may have different types
			 */
				 && (csp->exp->nodetype == en_icon
			/*
			 * the types which are fine in address registers
			 */
				 || (csp->exp->etype == bt_short ||
					 csp->exp->etype == bt_long ||
					 csp->exp->etype == bt_ulong ||
					 csp->exp->etype == bt_pointer))
#ifndef AREG_AS_DREG
					)
#else
					) && csp->duses)
#endif
			csp->reg = addreg++;
		else if (datareg < AREGBASE && csp->exp->nodetype!=en_autocon)
			csp->reg = datareg++;
#ifdef AREG_AS_DREG
		else if (areg_possible)
			csp->reg = addreg++;
#endif
		else
			csp->reg = -1;
/*		infunc("chk_curword")
			if (csp->reg==AREGBASE+2 || csp->reg==AREGBASE+3 || csp->reg==4)
				bkpt();*/
		if (csp->reg IS_VALID) {
			regexp[reg_t_to_regexp(csp->reg)] = csp->exp;
			regs_used++;
			rmask = rmask | (1 << (15 - csp->reg));
			mask = mask | (1 << csp->reg);
		}
		csp = csp->next;
	}
#ifdef ADD_REDUNDANCY
	if (regs_used>=6) {	/* >=6 : won't increase the push/pop time very much */
		mask = 0x7CF8, rmask = 0x1F3E;
		regs_used = 10;
	/* (note that we could use less than 6 as a threshold, but the benefit lies around
	    +0.5% while the runtime cost is rather high for such functions)
	   (note too that the benefit for the optimization with threshold 6 is around 0.5%
	    only, but it is so inexpensive that we really can afford it :] ) */
	}
#endif
	if (mask != 0) {
		g_code(op_movem, 4, mk_rmask(rmask), push_am);
#ifdef ICODE
		if (icode_option)
			fprintf(icode, "\t$regpush %04x\n", rmask);
#endif
	}
	save_mask = mask;
	csp = olist;
	while (csp != 0) {
		if (csp->reg IS_VALID) {	/* see if preload needed */
			exptr = csp->exp;
#ifdef REGPARM_OLD
			/* the following code works since REGPARM's are always
			 * handled first -- their desire() is highest */
			if (exptr->nodetype==en_ref && (ep=exptr->v.p[0])->nodetype==en_autocon
				&& ep->v.i>0 && (ep->v.i&REG_PARAMS)) {
				ap = mk_reg(ep->v.i>>REG_PARAMLOG);
				ap2 = mk_reg(csp->reg);
				g_code(op_move, (int) exptr->esize, ap, ap2);
			} else
#endif
			if (exptr->nodetype!=en_ref
#ifdef OPT_ROMCALLS
				|| exptr->v.p[0]->nodetype!=en_autocon
#endif
#ifdef REGPARM
				|| ((XLST_TYPE)exptr->v.p[0]->v.i >= -reg_size)) {
#else
				|| ((XLST_TYPE)exptr->v.p[0]->v.i > 0)) {
#endif
				initstack();
				ap = g_expr(exptr, F_ALL | F_SRCOP);
				ap2 = mk_reg(csp->reg);
				g_code(op_move, (int) exptr->esize, ap, ap2);
				freeop(ap);
#ifdef ICODE
				if (icode_option) {
					fprintf(icode, "$reg ");
					if (csp->reg < AREGBASE)
						fprintf(icode, "D%d\n", csp->reg);
					else
						fprintf(icode, "A%d\n", csp->reg - AREGBASE);
					g_icode(exptr);
				}
#endif
			}
		}
		csp = csp->next;
	}
}

#ifdef NO_EXTENDED_AUTOS
void repexpr(struct enode *node) {
#else
#define repexpr(node) _repexpr(&(node))
void _repexpr(struct enode **ep) {
	struct enode *node=*ep;
#endif
/*
 * repexpr will replace all allocated references within an expression with
 * tempref nodes.
 */
	struct cse	   *csp;
	if (node == 0)
		return;
	switch (node->nodetype) {
	  case en_icon:
	  case en_nacon:
	  case en_labcon:
	  case en_autocon:
		if ((csp = searchnode(node)) != 0) {
			if (csp->reg > 0) {
				node->nodetype = en_tempref;
				node->v.i = csp->reg;
			}
		}
		break;
	  case en_ref:
	  case en_fieldref:
		if ((csp = searchnode(node)) != 0) {
			if (csp->reg > 0) {
				node->nodetype = en_tempref;
				node->v.i = csp->reg;
			} else
				repexpr(node->v.p[0]);
		} else
			repexpr(node->v.p[0]);
		break;
	  case en_uminus:
	  case en_not:
	  case en_compl:
	  case en_ainc:
	  case en_adec:
	  case en_cast:
	  case en_deref:
	  case en_alloca:
		repexpr(node->v.p[0]);
		break;
	  case en_add:
		if (node->v.p[0]->nodetype==en_autocon && node->v.p[1]->nodetype==en_icon) {
			/**ep=mk_icon(node->v.p[0]->v.i+node->v.p[1]->v.i);
			(*ep)->nodetype=en_autocon;
			(*ep)->etype=bt_pointer;
			(*ep)->esize=4;*/
			node->v.p[0]->v.i+=node->v.p[1]->v.i;
			*ep=node->v.p[0];
			return;
		}
		/*@fallthrough@*/
	  case en_sub:
	  case en_mul:
	  case en_div:
	  case en_mod:
	  case en_lsh:
	  case en_rsh:
	  case en_and:
	  case en_or:
	  case en_xor:
	  case en_land:
	  case en_lor:
	  case en_eq:
	  case en_ne:
	  case en_lt:
	  case en_le:
	  case en_gt:
	  case en_ge:
	  case en_cond:
	  case en_void:
	  case en_asadd:
	  case en_assub:
	  case en_asmul:
	  case en_asdiv:
	  case en_asor:
	  case en_asxor:
	  case en_asand:
	  case en_asmod:
	  case en_aslsh:
	  case en_asrsh:
	  case en_fcall:
	  case en_assign:
		repexpr(node->v.p[0]);
		repexpr(node->v.p[1]);
		break;
	  case en_compound:
		repcse(node->v.st);
		break;
	}
}

void repcse(struct snode *block) {
/*
 * repcse will scan through a block of statements replacing the optimized
 * expressions with their temporary references.
 */
	while (block != 0) {
		switch (block->stype) {
		  case st_return:
		  case st_expr:
			repexpr(block->exp);
			break;
		  case st_loop:
			repexpr(block->exp);
			repcse(block->s1);
			repexpr(block->v2.e);
			break;
		  case st_while:
		  case st_do:
			repexpr(block->exp);
			repcse(block->s1);
			break;
		  case st_for:
			repexpr(block->exp);
			repexpr(block->v1.e);
			repcse(block->s1);
			repexpr(block->v2.e);
			break;
		  case st_if:
			repexpr(block->exp);
			repcse(block->s1);
			repcse(block->v1.s);
			break;
		  case st_switch:
			repexpr(block->exp);
			repcse(block->v1.s);
			break;
		  case st_case:
		  case st_default:
			repcse(block->v1.s);
			break;
		  case st_compound:
		  case st_label:
			repcse(block->s1);
			break;
		}
		block = block->next;
	}
}

void opt1(struct snode *block) {
/*
 * opt1 is the externally callable optimization routine. it will collect and
 * allocate common subexpressions and substitute the tempref for all
 * occurrances of the expression within the block.
 *
 */
	if (!opt_option)
		return;
	olist = 0;
	exec_count = INIT_EXEC_COUNT;
	scan(block);				/* collect expressions */
	allocate(); 				/* allocate registers */
	repcse(block);				/* replace allocated expressions */
}
// vim:ts=4:sw=4
