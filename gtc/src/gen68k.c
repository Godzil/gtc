/*
 * GTools C compiler
 * =================
 * source file :
 * code generator for the 68000
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
#ifndef NOFLOAT
#include	"ffplib.h"
#endif

#ifdef VCG
extern int vcg_lvl;
#endif

#ifdef MC680X0
readonly struct amode	push = {am_adec, 0, init_reg_t(STACKPTR - AREGBASE), nil_reg_t, 0},
				pop =  {am_ainc, 0, init_reg_t(STACKPTR - AREGBASE), nil_reg_t, 0};
#endif
XLST_TYPE		act_scratch CGLOB;

#ifdef FLINE_RC
int				fline_rc CGLOB;
#endif

/*
 * this module contains all of the code generation routines for evaluating
 * expressions and conditions.
 */

#ifdef MC680X0

void swap_nodes(struct enode *node);
void opt_compare(struct enode *node);

struct amode *mk_scratch(long size) {
/*
 * returns addressing mode of form offset(FRAMEPTR)
 * size is rounded up to AL_DEFAULT
 */
				struct amode			 *ap;

				/* round up the request */
				if (size % AL_DEFAULT)
								size += AL_DEFAULT - (size % AL_DEFAULT);

				/* allocate the storage */
				act_scratch += size;

/*
 * The next statement could be deferred and put into the
 * routine checkstack(), but this is just safer.
 */
				if (act_scratch > max_scratch)
								max_scratch = act_scratch;

				ap = (struct amode *) xalloc((int) sizeof(struct amode),AMODE+MK_SCRATCH);
				ap->mode = am_indx;
				ap->preg = FRAMEPTR - AREGBASE;
				ap->offset = mk_icon((long) -(lc_auto+act_scratch));
				return ap;
}


struct amode *mk_label(unsigned int lab) {
/*
 * construct a reference node for an internal label number.
 */
				struct enode			 *lnode;
				struct amode			 *ap;
				lnode = mk_node(en_labcon, NIL_ENODE, NIL_ENODE);
				lnode->v.enlab = lab;
				ap = (struct amode *) xalloc((int) sizeof(struct amode),AMODE+MK_LABEL);
				ap->mode = am_direct;
				ap->offset = lnode;
				return ap;
}

struct amode *mk_immed(long i) {
/*
 * make a node to reference an immediate value i.
 */
				struct amode			 *ap;
				struct enode			 *ep;
				ep = mk_icon(i);
/*				ep = mk_icon(0l);
				ep->v.i = i;*/
				ap = (struct amode *) xalloc((int) sizeof(struct amode), AMODE+MK_IMMED);
				ap->mode = am_immed;
				ap->offset = ep;
				return ap;
}

struct amode *mk_offset(struct enode *node) {
/*
 * make a direct reference to a node.
 */
				struct amode			 *ap;
				ap = (struct amode *) xalloc((int) sizeof(struct amode), AMODE+MK_OFFSET);
				ap->mode = am_direct;
				ap->offset = node;
				return ap;
}

struct amode *mk_legal(struct amode *ap, int flags, long size) {
/*
 * mk_legal will coerce the addressing mode in ap1 into a mode that is
 * satisfactory for the flag word.
 */
				struct amode			 *ap2;

	if (flags & F_NOVALUE) {
		freeop(ap);
		return 0;
	}

	if (size==1)
		flags &= ~F_AREG;

	switch (ap->mode) {
		case am_immed:
			if (!(flags & F_DEREF) && ap->offset
#ifdef EXE_OUT
				//&& !exestub_mode		-> because it remains profitable to do this
				//						even with very efficient relocations...
#endif
				&& (ap->offset->nodetype == en_nacon
				 || ap->offset->nodetype == en_labcon)
#ifdef AS
				&& !external(ap->offset->v.enlab)
#else
#if defined(PC) && !defined(DISABLE_OPTS)
				&& ((long)ap->offset->v.ensp>0x1000 ? internal(ap->offset->v.ensp) : 1)
#endif
#endif
					) {
					ap->mode = am_direct;
					ap2 = temp_addr();		/* allocate to areg */
					g_code(op_lea, 0, ap, ap2);
					if (flags & F_AREG)
							return ap2;
					freeop(ap2);
					ap = temp_data();
					g_code(op_move, (int) (size+(size&1)), ap2, ap); /* if we want a byte, do a move.w, not a move.b */
					if (flags & F_DREG)
							return ap;
					ierr(MK_LEGAL,1);
			}
			if (flags & F_IMMED) {
				return ap;		/* mode ok */
			}
			break;
		case am_areg:
			if (flags & F_AREG && (!(flags & F_VOL) || ap->preg <= MAX_ADDR))
				return ap;
			break;
		case am_dreg:
			if (flags & F_DREG && (!(flags & F_VOL) || ap->preg <= MAX_DATA))
				return ap;
			break;
		case am_direct:
/*				infunc("DrawColorLine")
					bkpt();*/
			if (!(flags & F_SRCOP) && !(flags & F_DEREF) && ap->offset
#ifdef EXE_OUT
				//&& !exestub_mode		-> because it remains rentable to do this
				//						even with very efficient relocations...
#endif
				&& (ap->offset->nodetype == en_nacon
				 || ap->offset->nodetype == en_labcon)
#ifdef AS
				&& !external(ap->offset->v.enlab)
#else
#if defined(PC) && !defined(DISABLE_OPTS)
				&& ((long)ap->offset->v.ensp>0x1000 ? internal(ap->offset->v.ensp) : 1)
#endif
#endif
				&& (flags & F_MEM)
					 ) {
					ap2 = temp_addr();		// allocate to areg
					g_code(op_lea, 0, ap, ap2);
					ap2 = copy_addr(ap2);
					ap2->mode = am_ind;
//					if (flags & F_MEM)
							return ap2;
/*					freeop(ap2);
					if ((flags&(F_AREG|F_DEREF))==(F_AREG|F_DEREF) || !(flags&F_DREG))
						ap=temp_addr();
					else ap=temp_data();
//					ap = temp_data();
					g_code(op_move, (int) size, ap2, ap);
//					if (flags & F_DREG)
							return ap;
					ierr(MK_LEGAL,2);*/
			}
			/* FALL THROUGH */
		case am_ind:
		case am_indx:
		case am_indx2:
		case am_indx3:
		case am_ainc:
			if (flags & F_MEM)
				return ap;
			break;
	}
	if ((flags & F_DREG) && (flags & F_AREG)) {
		/* decide which mode is better */
		if (ap->mode == am_immed) {
						if (isbyte(ap->offset))
						flags &= F_DREG;
						else if (isshort(ap->offset) && size == 4)
						flags &= F_AREG;
		}
	}
	if (flags & F_DREG) {
		freeop(ap); 			/* maybe we can use it... */
		ap2 = temp_data();		/* allocate to dreg */
		g_code(op_move, (int) size, ap, ap2);
		return ap2;
	}
	if (!(flags & F_AREG))
		ierr(MK_LEGAL,3);
	if (size < 2)
		ierr(MK_LEGAL,4);
	freeop(ap);
	ap2 = temp_addr();
	g_code(op_move, (int) size, ap, ap2);
	return ap2;
}

int isshort(struct enode *node) {
/*
 * return true if the node passed can be generated as a short offset.
 */
				return node->nodetype == en_icon &&
				(node->v.i >= -32768 && node->v.i <= 32767);
}

int isbyte(struct enode *node) {
				return node->nodetype == en_icon &&
				(node->v.i >= -128 && node->v.i <= 127);
}


struct amode *copy_addr(struct amode *ap) {
/*
 * copy an address mode structure.
 */
	struct amode			 *newap;
	if (ap == 0)
		ierr(COPY_ADDR,1);
	newap = (struct amode *) xalloc((int) sizeof(struct amode), AMODE+COPY_ADDR);
	*newap = *ap;
	return newap;
}


struct amode *g_index(struct enode *node) {
/*
 * generate code to evaluate an index node and return the addressing
 * mode of the result.
 */
	struct amode			 *ap1, *ap2;
	char size=4;
#ifndef FORBID_TABLES
	if (node->v.p[0]->nodetype == en_tempref &&
		node->v.p[1]->nodetype == en_tempref &&
			(node->v.p[0]->v.i >= AREGBASE || node->v.p[1]->v.i >= AREGBASE)) {
		/* both nodes are registers, one is address */
		if (node->v.p[0]->v.i < AREGBASE) {	/* first node is data register */
			ap1 = g_expr(node->v.p[1], F_AREG);
			ap1 = copy_addr(ap1);
			ap1->sreg = (reg_t)node->v.p[0]->v.i;
			ap1->mode = am_indx2; /* 0(Ax,Dx) */
			ap1->slen = 4;
			ap1->offset = mk_icon(0l);
			return ap1;
		} else {						/* first node is address register */
			ap1 = g_expr(node->v.p[0], F_AREG);
			ap2 = g_expr(node->v.p[1], F_AREG | F_DREG);
			validate(ap1);
			ap1 = copy_addr(ap1);
			if (ap2->mode == am_dreg) {
				/* 0(Ax,Dx) */
				ap1->mode = am_indx2;
				ap1->sreg = ap2->preg;
				ap1->slen = 4;
			} else {
				/* 0(Ax,Ay) */
				ap1->mode = am_indx3;
				ap1->sreg = ap2->preg;
				ap1->slen = 4;
			}
			ap1->offset = mk_icon(0l);
			return ap1;
		}
	}
#endif
	/* The general case (no tempref) */
	/* put address temprefs first place, data temprefs second place */
	if (node->v.p[1]->nodetype == en_tempref && node->v.p[1]->v.i >= AREGBASE)
		swap_nodes(node);
	else if (node->v.p[0]->nodetype == en_tempref && node->v.p[0]->v.i < AREGBASE)
		swap_nodes(node);
	else if (node->v.p[1]->etype==bt_pointer && node->v.p[0]->etype!=bt_pointer)
		swap_nodes(node);

	ap1 = g_expr(node->v.p[0], F_AREG | F_IMMED);
	if (ap1->mode == am_areg) {
		struct enode *ep=node->v.p[1],*tempep=ep;
		while (tempep->nodetype==en_cast)	/* avoid useless op_ext's */
			if ((tempep=tempep->v.p[0])->esize==2)
				size=2, ep=tempep;
		ap2 = g_expr(ep, F_AREG | F_DREG | F_IMMED);
		validate(ap1);
	} else {
		ap2 = ap1;
		ap1 = g_expr(node->v.p[1], F_AREG/* | F_IMMED*/);
		validate(ap2);
	}
	/*
	 * possible combinations:
	 * 
	 * F_AREG + F_IMMED, F_AREG + F_DREG and F_AREG + F_AREG
	 * (F_IMMED + F_IMMED is now impossible, since it would mean a bug
	 *  in the optimizer, and it does no harm except for optimization
	 *  if such a bug does exist)
	 */


	/* watch out for: tempref(addr) + temp_addr, tempref(addr) + temp_data */

	if (/*ap1->mode == am_areg && */ap1->preg > MAX_ADDR) {
		/* ap1 = tempref address register */
		ap1 = copy_addr(ap1);
#ifndef FORBID_TABLES
		if (ap2->mode == am_dreg) {
						/* 0(Ax,Dy) */
						ap1->mode = am_indx2;
						ap1->sreg = ap2->preg;
						ap1->slen = size;
						ap1->deep = ap2->deep;
						ap1->offset = mk_icon(0l);
						return ap1;
		}
		if (ap2->mode == am_areg) {
						/* 0(Ax,Ay) */
						ap1->mode = am_indx3;
						ap1->sreg = ap2->preg;
						ap1->slen = size;
						ap1->deep = ap2->deep;
						ap1->offset = mk_icon(0l);
						return ap1;
		}
#endif
#ifndef FORBID_TABLES
		if (ap2->mode == am_immed &&			/* if FORBID_TABLES, then we */
				(!isshort(ap2->offset)))		/* always want ap1 to be F_VOL */
#endif
						/* we want to add to ap1 later... */
						ap1 = mk_legal(ap1, F_AREG | F_VOL, 4l);
	}
	/* watch out for: temp_addr + tempref(data) */

#ifndef FORBID_TABLES
	if (/*ap1->mode == am_areg && */ap2->mode == am_dreg && ap2->preg > MAX_DATA) {
		ap1 = copy_addr(ap1);
		ap1->mode = am_indx2;
		ap1->sreg = ap2->preg;
		ap1->slen = size;
		ap1->offset = mk_icon(0l);
		return ap1;
	}
#endif
/*	if (ap1->mode == am_immed && ap2->mode == am_immed) {
	ap1 = copy_addr(ap1);
	ap1->offset = mk_node(en_add, ap1->offset, ap2->offset);
	ap1->mode = am_direct;
	return ap1;
	}*/
	if (ap2->mode == am_immed && isshort(ap2->offset)) {
		ap1 = mk_legal(ap1, F_AREG, 4l);
		ap1 = copy_addr(ap1);
		ap1->mode = am_indx;
		ap1->offset = ap2->offset;
		return ap1;
	}
	/* ap1 is volatile ... */
	g_code(op_add, size, ap2, ap1);/* add left to address reg */
	ap1 = copy_addr(ap1);
	ap1->mode = am_ind; 			/* mk_ indirect */
	freeop(ap2);							/* release any temps in ap2 */
	return ap1; 							/* return indirect */
}

struct amode *g_deref(struct enode *node, enum(e_bt) type, int flags, long size) {
/*
 * return the addressing mode of a dereferenced node.
 */
	struct amode			 *ap1;
/*
 * If a reference to an aggregate is required, return a pointer to the
 * struct instead
 */
	if (bt_aggregate(type)) {
		return g_expr(node, /*F_ALL*/flags); // possibly BUGGY
	}
#ifdef PREFER_POS_VALUES
	if (node->nodetype == en_sub && node->v.p[1]->nodetype==en_icon)
		node->nodetype = en_add, node->v.p[1]->v.i=-node->v.p[1]->v.i;
#endif
	if (node->nodetype == en_add) {
		return g_index(node);
	}
	if (node->nodetype == en_autocon) {
#ifdef BIGSTACK
		if (node->v.i >= -32768 && node->v.i <= 32767) {
#endif
			ap1 = (struct amode *) xalloc((int) sizeof(struct amode),
					AMODE+G_DEREF);
			ap1->mode = am_indx;
			ap1->preg = FRAMEPTR-AREGBASE;
			ap1->offset = mk_icon((long) node->v.i);
#ifdef BIGSTACK
		} else {
			ap1 = temp_addr();
			g_code(op_move, 4, mk_immed((long) node->v.i), ap1);
			g_code(op_add, 4, mk_reg(FRAMEPTR), ap1);
			ap1 = copy_addr(ap1);
			ap1->mode = am_ind;
		}
#endif
		return ap1;
	}
	/*				if (lineid==309)
					bkpt();*/
	/* special 68000 instructions */
	if (node->nodetype == en_ainc
			&& (size ==1 || size ==2 || size ==4)
			&& node->v.p[1]->v.i == size
			&& node->v.p[0]->nodetype == en_tempref
			&& node->v.p[0]->v.i >= AREGBASE
			&& !(flags & F_USES)) {
		/* (An)+ */
		ap1 = (struct amode *) xalloc((int) sizeof(struct amode), AMODE+G_DEREF);
		ap1->mode = am_ainc;
		ap1->preg = (reg_t)(node->v.p[0]->v.i - AREGBASE);
		return ap1;
	}
	if (node->nodetype == en_assub
			&& (size ==1 || size ==2 || size ==4)
			&& node->v.p[1]->v.i == -size
			&& node->v.p[0]->nodetype == en_tempref
			&& node->v.p[0]->v.i >= AREGBASE
			&& !(flags & F_USES)) {
		/* -(An) */
		ap1 = (struct amode *) xalloc((int) sizeof(struct amode), AMODE+G_DEREF);
		ap1->mode = am_adec;
		ap1->preg = (reg_t)(node->v.p[0]->v.i - AREGBASE);
		return ap1;
	}
	ap1 = g_expr(node, F_AREG | F_IMMED | F_DEREF); /* generate address */
	ap1 = copy_addr(ap1);
#ifdef PC
	if (ap1->mode != am_areg && ap1->mode != am_immed)
		ierr(G_DEREF,1);
#endif
	am_doderef(ap1->mode);
	return mk_legal(ap1,flags,size); // mk_legal was missing here : F_VOL wasn't
	// taken into a count...
}

struct amode *g_fderef(struct enode *node, struct amode *ap0, int flags) {
/*
 * get a bitfield value
 */
	struct amode *ap, *ap1;
	long mask;
	int width = node->bit_width;
	int offs = (node->esize<<3)-node->bit_offset-width;

	if (!ap0)
		ap = g_deref(node->v.p[0], node->etype, flags, node->esize);
	else ap = ap0;
	ap = mk_legal(ap, F_DREG|F_VOL, node->esize);
	if (offs > 0) {
		if (offs <= 8) {
			/* can shift with quick constant */
			g_code(op_lsr, (int) node->esize,
				mk_immed((long) offs), ap);
		} else {
			/* must load constant first */
			ap1 = temp_data();
			g_code(op_moveq, 0, mk_immed((long) offs), ap1);
			g_code(op_lsr, (int) node->esize, ap1, ap);
			freeop(ap1);
		}
	}
	/*mask = 0;
	while (width--)
		mask = mask + mask + 1;*/
	mask = (1<<width)-1;
	g_code(op_and, (int) node->esize, mk_immed(mask), ap);

	return mk_legal(ap, flags, node->esize);
}


struct amode *g_unary(struct enode *node, int flags, enum(e_op) op) {
/*
 * generate code to evaluate a unary minus or complement. float: unary minus
 * calls a library function
 */
	struct amode			 *ap;
#ifdef DOUBLE
	long		i;
#endif
	if (flags & F_NOVALUE) {
		return g_expr(node->v.p[0], flags);
	}
	switch (node->etype) {
		case bt_uchar:
		case bt_char:
		case bt_short:
		case bt_ushort:
		case bt_long:
		case bt_ulong:
		case bt_pointer:
			ap = g_expr(node->v.p[0], F_DREG | F_VOL);
			g_code(op, (int) node->esize, ap, NIL_AMODE);
			return mk_legal(ap, flags, node->esize);
#ifndef NOFLOAT
		case bt_float:
#ifdef DOUBLE
		case bt_double:
#endif
			if (op == op_neg) {
#ifdef DOUBLE
				temp_inv();
				i = push_param(node->v.p[0]);
				call_library(str(ffpneg));
				return func_result(flags, i);
#else
				int null_lab = nxtlabel();
				struct amode *ap = g_expr(node->v.p[0], F_DREG | F_VOL);
				g_code(op_tst, 4, ap, NIL_AMODE);
				g_code(op_beq, 0, mk_label(null_lab), NIL_AMODE);
				g_code(op_neg, 4, ap, NIL_AMODE);
				g_label(null_lab);
				return mk_legal(ap, flags, node->esize);
#endif
			}
#endif
	}
	ierr(G_UNARY,1);
	/* NOTREACHED */
	return 0; // make the compiler happy
}

struct amode *g_addsub(struct enode *node, int flags, enum(e_op) op) {
/*
 * generate code to evaluate a binary node and return the addressing mode of
 * the result.
 */
	struct amode			 *ap1, *ap2;
	long											i;
	if (flags & F_NOVALUE) {
		g_expr(node->v.p[0], flags);
		return g_expr(node->v.p[1], flags);
	}
	switch (node->etype) {
		case bt_uchar:
		case bt_char:
		case bt_short:
		case bt_ushort:
		case bt_long:
		case bt_ulong:
		case bt_pointer:
			flags &= (F_DREG | F_AREG);
#ifdef LONG_ADDS_IN_AREG
			if (node->v.p[1]->nodetype==en_icon && node->esize==4
#if 1
					&& !((unsigned long)(i=node->v.p[1]->v.i)+8<=16)
					&& (long)(short)i==i
#else
					&& (i=node->v.p[1]->v.i,(long)(short)i==i)
#endif
					&& (flags&F_AREG))
				flags = F_AREG;
#endif
#ifdef TWOBYTE_ARRAY_EXTRA_OPT
			if (node->v.p[1]->nodetype==en_lsh && node->v.p[1]->v.p[1]->nodetype==en_icon
					&& node->v.p[1]->v.p[1]->v.i==1) {
				ap1 = g_expr(node->v.p[0], F_VOL | flags);
				ap2 = g_expr(node->v.p[1]->v.p[0], F_ALL | F_SRCOP | F_USES);
				validate(ap1);					/* in case push occurred */
				g_code(op, (int) node->esize, ap2, ap1);
				g_code(op, (int) node->esize, ap2, ap1);
			} else {
#endif
				ap1 = g_expr(node->v.p[0], F_VOL | flags);
				ap2 = g_expr(node->v.p[1], F_ALL | F_SRCOP);
				validate(ap1);					/* in case push occurred */
				g_code(op, (int) node->esize, ap2, ap1);
#ifdef TWOBYTE_ARRAY_EXTRA_OPT
			}
#endif
			freeop(ap2);
			return mk_legal(ap1, flags, node->esize);
#ifndef NOFLOAT
		case bt_double:
			temp_inv();
			i = push_param(node->v.p[1]);
			i += push_param(node->v.p[0]);
			switch (op) {
				case op_add:
					call_library(str(ffpadd));
					break;
				case op_sub:
					call_library(str(ffpsub));
					break;
			}
			return func_result(flags, i);
#endif
	}

	ierr(G_ADDSUB,1);
	/* NOTREACHED */
	return 0; // make the compiler happy
}

struct amode *g_xbin(struct enode *node, int flags, enum(e_op) op) {
/*
 * generate code to evaluate a restricted binary node and return the
 * addressing mode of the result.
 * these are bitwise operators so don't care about the type.
 * This needs to be revised with scalar types longer than 32 bit
 */
				struct amode			 *ap1, *ap2;
				ap1 = g_expr(node->v.p[0], F_VOL | F_DREG);
				ap2 = g_expr(node->v.p[1], F_DREG);
				validate(ap1);					/* in case push occurred */
				g_code(op, (int) node->esize, ap2, ap1);
				freeop(ap2);
				return mk_legal(ap1, flags, node->esize);
}

struct amode *g_shift(struct enode *node, int flags, enum(e_op) op);
int equalnode(struct enode *node1, struct enode *node2);
struct amode *g_ybin(struct enode *node, int flags, enum(e_op) op) {
/*
 * generate code to evaluate a restricted binary node and return the
 * addressing mode of the result.
 */
				struct amode			 *ap1, *ap2;
#ifdef GENERATE_ROL_ROR
	if (
			(op==op_or || op==op_eor) &&
			node->v.p[0]->nodetype==en_lsh && node->v.p[1]->nodetype==en_rsh && bt_uns(node->v.p[1]->etype) &&
			node->v.p[0]->v.p[1]->nodetype==en_icon && node->v.p[1]->v.p[1]->nodetype==en_icon &&
			equalnode(node->v.p[0]->v.p[0],node->v.p[1]->v.p[0])
			) {
		long lsh = node->v.p[0]->v.p[1]->v.i;
		long rsh = node->v.p[1]->v.p[1]->v.i;
		long sz = lsh+rsh;
		if (lsh>=0 && rsh>=0 && sz==8*node->esize) {
			int need_swap = 0;
			if (lsh>8 && rsh>8) {
				need_swap = 1;
				if (lsh>=16)
					lsh-=16,rsh+=16;
				else
					rsh-=16,lsh+=16;
			}
			if (!lsh || !rsh) {
				ap1 = g_expr(node->v.p[0]->v.p[0], F_DREG | F_VOL);
			} else {
				int shift_left = lsh<=8;
				struct enode *ep1,*ep2;
				ep1 = mk_icon(shift_left?lsh:rsh);
				ep1->etype = bt_uchar;
				ep1->esize = 1;
				ep2 = mk_node(en_lsh, node->v.p[0]->v.p[0], ep1);
				ep2->etype = node->etype;
				ep2->esize = node->esize;
				ap1 = g_shift(ep2, F_DREG|F_VOL, shift_left?op_rol:op_ror);
			}
			if (need_swap)
				g_code(op_swap, (int)node->esize, ap1, NIL_AMODE);
			return mk_legal(ap1, flags, node->esize);
		} else {
			// we could do better, but hey, we're lazy, aren't we?
		}
	}
#endif
				ap1 = g_expr(node->v.p[0], F_VOL | F_DREG);
				ap2 = g_expr(node->v.p[1], (F_ALL & ~F_AREG) | F_SRCOP);
				validate(ap1);					/* in case push occurred */
				g_code(op, (int) node->esize, ap2, ap1);
				freeop(ap2);
				return mk_legal(ap1, flags, node->esize);
}
#ifdef VCG
typedef struct amode *(*COMMUTATIVE_G)(struct enode *node,int flags,enum(e_op) op);
typedef struct amode *(*REVERSAL_G)(struct amode *ap, struct enode *ep,int flags);
//struct amode *symmetric(struct amode *ap, struct enode *ep,int flags) {
//	if (ap==NULL) return (struct amode *)(void *)flags;
//	return ap;
//}
//struct amode *antisymmetric(struct amode *ap, struct enode *ep,int flags) {
//	if (ap==NULL) return (struct amode *)(void *)(F_DREG|F_VOL);
//	g_code(op_neg, ep->esize, ap, NIL_AMODE);
//	return mk_legal(ap,flags,ep->esize);
//}
enum { symmetric = 0 }; // antisymmetric is not supported
struct amode *g_commute(void *func,struct enode *node,int flags,enum(e_op) op,int dummy/*void *reversal*/) {
	if (vcg_init()) {
		struct amode *ap = NULL;
		struct enode *n0=node->v.p[0],*n1=node->v.p[1];
		int o1,o2;
		//struct amode *ap;
		((COMMUTATIVE_G)func)(node,flags,op);
		o1=vcg_done();
		node->v.p[0]=n1,node->v.p[1]=n0;
		vcg_init();
		((COMMUTATIVE_G)func)(node,flags,op);
		//((REVERSAL_G)reversal)(((COMMUTATIVE_G)func)
		//		(node,(int)((REVERSAL_G)reversal)(NULL,node,flags),op),node,flags);
		o2=vcg_done();
		if (o2<=o1)
			ap = ((COMMUTATIVE_G)func)(node,flags,op);
			//ap = ((REVERSAL_G)reversal)(((COMMUTATIVE_G)func)
			//	(node,(int)((REVERSAL_G)reversal)(NULL,node,flags),op),node,flags);
		node->v.p[0]=n0,node->v.p[1]=n1; /* always restore the original node, even when reverse is better! */
		if (ap)
			return ap;
	}
	return ((COMMUTATIVE_G)func)(node,flags,op);
}
#endif

struct amode *g_shift(struct enode *node, int flags, enum(e_op) op) {
/*
 * generate code to evaluate a shift node and return the address mode of the
 * result.
 */
	struct amode *ap1,*ap2;

	if (op == op_lsl && node->v.p[1]->nodetype == en_icon
		&& (node->v.p[1]->v.i == 1
#ifndef HARDCORE_FOR_UNPACKED_SIZE
#ifdef SPEED_OPT
		|| ((speed_opt_value>=0
#ifdef EXE_OUT
				|| exestub_mode
#endif
			) && node->v.p[1]->v.i == 2)
#else
		|| node->v.p[1]->v.i == 2
#endif
#endif
		)) {
		int i;
		ap1 = g_expr(node->v.p[0], F_VOL | (flags & (F_DREG | F_AREG)));
		i = node->v.p[1]->v.i-1;
		do
			g_code(op_add, (int) node->esize, ap1, ap1);
		while (i--);
	} else {
		ap1 = g_expr(node->v.p[0], F_DREG | F_VOL);
		ap2 = g_expr(node->v.p[1], F_DREG | F_IMMED);
		validate(ap1);

		/* quick constant only legal if 1<=const<=8 */
		if (ap2->mode == am_immed && ap2->offset->nodetype == en_icon
			&& (ap2->offset->v.i > 8 || ap2->offset->v.i < 1)) {
			if (ap2->offset->v.i <= 0 || ap2->offset->v.i > 32)
				uwarn("shift constant out of range");
#ifdef USE_SWAP_FOR_SHIFT16
			if ((unsigned int)ap2->offset->v.i <= 16 && node->esize==4) {
				/* validate(ap1); -> useless since ap2 is am_immed :) */
				g_code(op_swap, 0, ap1, NIL_AMODE);
				if ((ap2->offset->v.i -= 16)) {
					ap2 = mk_immed(-ap2->offset->v.i);
					op = (op==op_lsl?op_ror:op_rol); /* since op might be op_asr too */
					g_code(op, (int) node->esize, ap2, ap1);
				}
				#error The following bugs... ((long)x)<<16 => ext/swap/ext #roll#
				/* to be improved since it normally results from 'long' shifts and surrounding
					g_cast does not recurse down to the shift node...
					The current optimization is nevertheless useful with respect to speed. */
				if (node->esize==4) g_code(op_ext, (int) node->esize, ap1, NIL_AMODE);
				goto ok_shift;
			} else
#endif
				ap2 = mk_legal(ap2, F_DREG, 1l);
		}
		g_code(op, (int) node->esize, ap2, ap1);
#ifdef USE_SWAP_FOR_SHIFT16
	ok_shift:
#endif
		freeop(ap2);
	}
	return mk_legal(ap1, flags, node->esize);
}

struct amode *g_div(struct enode *node, int flags) {
/*
 * generate code to evaluate a divide operator
 */
	struct amode			 *ap1, *ap2;
	long											i;
	switch (node->etype) {
		case bt_short:
		case bt_ushort:
			ap1 = g_expr(node->v.p[0], F_DREG | F_VOL);
			ap2 = g_expr(node->v.p[1], F_ALL | F_SRCOP);
			validate(ap1);
			if (node->etype == bt_short) {
				g_code(op_ext, 4, ap1, NIL_AMODE);
				g_code(op_divs, 0, ap2, ap1);
			} else {
				g_code(op_and, 4, mk_immed(65535l), ap1);
				g_code(op_divu, 0, ap2, ap1);
			}
			freeop(ap2);
			return mk_legal(ap1, flags, 2l);
		case bt_long:
		case bt_ulong:
		case bt_double:
			temp_inv();
			i = push_param(node->v.p[1]);
			i += push_param(node->v.p[0]);
			switch (node->etype) {
				case bt_long:
					call_library("__divsi3");
					break;
				case bt_ulong:
				case bt_pointer:
					call_library("__udivsi3");
					break;
#ifndef NOFLOAT
				case bt_double:
					call_library(str(ffpdiv));
					break;
#endif
			}
			return func_result(flags, i);
	}
	ierr(G_DIV,1);
	/* NOTREACHED */
	return 0; // make the compiler happy
}

struct amode *g_mod(struct enode *node, int flags) {
/*
 * generate code to evaluate a mod operator
 */
	struct amode			 *ap1, *ap2;
	long											i;
	switch (node->etype) {
		case bt_short:
		case bt_ushort:
			ap1 = g_expr(node->v.p[0], F_DREG | F_VOL);
			ap2 = g_expr(node->v.p[1], F_ALL | F_SRCOP);
			validate(ap1);
			if (node->etype == bt_short) {
				g_code(op_ext, 4, ap1, NIL_AMODE);
				g_code(op_divs, 0, ap2, ap1);
			} else {
				g_code(op_and, 4, mk_immed(65535l), ap1);
				g_code(op_divu, 0, ap2, ap1);
			}
			g_code(op_swap, 0, ap1, NIL_AMODE);
			freeop(ap2);
			return mk_legal(ap1, flags, 2l);
		case bt_long:
		case bt_ulong:
			temp_inv();
			i = push_param(node->v.p[1]);
			i += push_param(node->v.p[0]);
			if (node->etype == bt_long)
				call_library("__modsi3");
			else
				call_library("__umodsi3");
			return func_result(flags, i);
	}
	ierr(G_MOD,1);
	/* NOTREACHED */
	return 0; // make the compiler happy
}

void swap_nodes(struct enode *node) {
/*
 * exchange the two operands in a node.
 */
	struct enode			 *temp;
	temp = node->v.p[0];
	node->v.p[0] = node->v.p[1];
	node->v.p[1] = temp;
}

struct amode *g_mul(struct enode *node, int flags) {
	struct amode			 *ap1, *ap2;
#ifdef OLD_GCCLIKE_MULSI3
	long											i;
#endif
/*				switch (node->etype) {
								case bt_long:
								case bt_ulong:
				if (node->v.p[0]->nodetype == en_icon)
								swap_nodes(node);
				ap1 = g_expr(node->v.p[0], F_DREG | F_VOL);
				ap2 = g_expr(node->v.p[1], F_ALL | F_SRCOP);
				validate(ap1);
				if (node->etype == bt_long)
								g_code(op_muls, 0, ap2, ap1);
				else
								g_code(op_mulu, 0, ap2, ap1);
				freeop(ap2);
				return mk_legal(ap1, flags, 2l);
								case bt_pointer:
								case bt_double:
				temp_inv();
				i = push_param(node->v.p[1]);
				i += push_param(node->v.p[0]);
				if (node->etype == bt_ulong)
								call_library(".ulmul");	// obsolete
				else if (node->etype == bt_double)
								call_library(".fpmult");	// obsolete
				else
								call_library(".lmul");	// obsolete
				return func_result(flags, i);
				}*/
	switch (node->etype) {
		case bt_short:
		case bt_ushort:
			if (node->v.p[0]->nodetype == en_icon)
				swap_nodes(node);
			ap1 = g_expr(node->v.p[0], F_DREG | F_VOL);
			ap2 = g_expr(node->v.p[1], F_ALL | F_SRCOP);
			validate(ap1);
			if (node->v.p[0]->etype == bt_short)
				g_code(op_muls, 0, ap2, ap1);
			else
				g_code(op_mulu, 0, ap2, ap1);
			freeop(ap2);
			return mk_legal(ap1, flags, 2l);
		case bt_ulong:
		case bt_long:
		case bt_pointer:
		case bt_double:
			temp_inv();
#ifdef OLD_GCCLIKE_MULSI3
			i = push_param(node->v.p[1]);
			i += push_param(node->v.p[0]);
#ifndef NOFLOAT
			if (node->etype == bt_double)
				call_library(str(ffpmul));
			else
#endif
				call_library("__mulsi3");	/* it's the same function for ulong & long :) */
			return func_result(flags, i);
#else
			{ char *libfunc="__mulsi3_rp";
				if (tst_ushort(node->v.p[0]))
					swap_nodes(node);
				if (tst_ushort(node->v.p[1])) {	/* favour ushort rather than short! */
					node->v.p[1]->etype=bt_ushort;
					node->v.p[1]->esize=2;
					libfunc="__mulsi3ui2_rp";
				} else {
					if (tst_short(node->v.p[0]))
						swap_nodes(node);
					if (tst_short(node->v.p[1])) {
						node->v.p[1]->etype=bt_short;
						node->v.p[1]->esize=2;
						libfunc="__mulsi3si2_rp";
					}
				}
				ap1 = g_expr(node->v.p[0], F_DREG | F_VOL);	/* put it in D0 */
				ap2 = g_expr(node->v.p[1], F_DREG | F_VOL);	/* put it in D1 */
				validate(ap1);
				call_library(libfunc);
				freeop(ap2);
				return mk_legal(ap1, flags, 4l);
			}
#endif
	}
	ierr(G_MUL,1);
	/* NOTREACHED */
	return 0; // make the compiler happy
}

/*
 * Returns a complexity measure (in terms of time) of the given node, if it's constant.
 * If it's not, returns -128.
 */
int complexity(struct enode *ep) {
	int comp=1;
	if (!ep) return 0;
restart:
	switch (ep->nodetype) {
		case en_icon:
			if (ep->v.i) comp++;
		case en_tempref:
			comp--;
		case en_fcon:
		case en_nacon:
		case en_labcon:
		case en_autocon:
			return comp;
		case en_fcall:
		case en_assign:
		case en_asadd: case en_assub:
		case en_asmul: case en_asmod: case en_asdiv:
		case en_aslsh: case en_asrsh:
		case en_asand: case en_asor: case en_asxor:
		case en_ainc: case en_adec:
		case en_alloca:
			return -128;
		case en_cond:
//			msg("DEBUG: complexity\n");
			return -128;
		case en_compound:
//			msg("DEBUG: complexity\n");
			return -128;
		case en_mod: case en_div:
			comp+=2;
		case en_mul:
			comp+=2;
		case en_void:
		case en_add: case en_sub:
		case en_lsh: case en_rsh:
		case en_and: case en_or: case en_xor:
		case en_eq: case en_ne:
		case en_lt: case en_le: case en_gt: case en_ge:
		case en_land: case en_lor:
			comp+=complexity(ep->v.p[0]);
			if (comp<0) return -128;
			{
				int comp2=complexity(ep->v.p[1]);
				if (comp2<0) return -128;
				return comp+comp2;
			}
		case en_ref:
		case en_fieldref:
		case en_deref:
		case en_uminus: case en_not:
		case en_compl:
		case en_cast:
			comp++;
			ep=ep->v.p[0];
			goto restart;
		default:
			fatal("COMPLEXITY");
			return 0; // make the compiler happy
	}
}

struct amode *g_hook(struct enode *node, int flags) {
/*
 * generate code to evaluate a condition operator node (?:)
 */
			struct amode			 *ap1, *ap2;
			unsigned int			false_label, end_label;
			int 							flagx;
			int 							result_is_void;
			long size=node->esize;
			struct enode *vnode=node->v.p[1];
#ifdef VCG
			int alternatives = complexity(node->v.p[0]);
			int best_flagx;
			unsigned int cost1=-1,cost2=-1;
#endif
			end_label = nxtlabel();

			result_is_void = (node->etype == bt_void);
			if (bt_aggregate(node->etype)) {
							size = 4;
			}

			if (!result_is_void) {
							flagx = (flags & (F_DREG | F_AREG)) == F_AREG ?
											F_AREG | F_VOL : F_DREG | F_VOL;
#ifdef VCG
							best_flagx = vcg_lvl?-1:flagx;
#endif
			} else {
#ifdef VCG
					best_flagx =
#endif
							flagx = F_ALL | F_SRCOP | F_NOVALUE;
			}

//			temp_inv(); /* I do not think I can avoid that */

#ifdef VCG
			best_flagx=flagx;
			while (1) {
				if (best_flagx<0)
					vcg_init();	// always succeeds, otherwise best_flagx would be flagx
#endif
			if (vnode->v.p[0]) {
				/* all scratch registers are void */
#ifdef OPTIMIZED_HOOK
				struct ocode *old_peep_tail=peep_tail;
				int old_max_reg=max_reg;
				max_reg=0;
#endif
#ifdef VCG
				if (vcg_lvl && alternatives IS_VALID) {
					int alt1,alt2,norm;
				  if (complexity(vnode->v.p[1])>=0) {
				   vcg_init();
					ap1 = g_expr(vnode->v.p[1], flagx);
					falsejp(node->v.p[0], end_label);
					if (ap_hasbeenpushed(ap1)) {
						validate(ap1);
						freeop(ap1);
						vcg_done();
						best_flagx=0;
						goto classic_fashion;
					}
					freeop(ap1);
					alt1=vcg_cost()>>3;
					ap2 = g_expr(vnode->v.p[0], flagx);
					alt1+=vcg_done();
				  } else alt1=32767;
					swap_nodes(vnode);
				  if (complexity(vnode->v.p[1])>=0) {
				   vcg_init();
					ap1 = g_expr(vnode->v.p[1], flagx);
					truejp(node->v.p[0], end_label);
					if (ap_hasbeenpushed(ap1))
						ierr(G_HOOK,3);
					freeop(ap1);
					alt2=vcg_cost()>>3;
					ap2 = g_expr(vnode->v.p[0], flagx);
					alt2+=vcg_done();
				  } else alt2=32767;
					swap_nodes(vnode);
				   vcg_init();
					false_label = nxtlabel();
					falsejp(node->v.p[0], false_label);
					norm=vcg_cost()>>3;
					ap1 = g_expr(vnode->v.p[0], flagx);
					freeop(ap1);
					g_code(op_bra, 0, mk_label(end_label), NIL_AMODE);
					g_label(false_label);
					ap2 = g_expr(vnode->v.p[1], flagx);
					norm+=vcg_done();

					if (norm>alt1 && norm>alt2) {
						if (alt2<alt1) swap_nodes(vnode);
						ap1 = g_expr(vnode->v.p[1], flagx);
						(alt2>=alt1?falsejp:truejp)
							(node->v.p[0], end_label);
						freeop(ap1);
						ap2 = g_expr(vnode->v.p[0], flagx);
						if (alt2<alt1) swap_nodes(vnode);	// we absolutely need this!!!
						goto done;							// (hardcore TI-Chess+VTILog debug)
					}
				}
#endif
#if defined(ALTERNATE_HOOK) || defined(OPTIMIZED_HOOK)
				int c1=complexity(vnode->v.p[0]),c2=complexity(vnode->v.p[1]);
				if (c1>=0 || c2>=0) {
					if ((unsigned)c1<(unsigned)c2)
						swap_nodes(vnode);
					ap1 = g_expr(vnode->v.p[1], flagx);
					((unsigned)c1<(unsigned)c2?truejp:falsejp)
						(node->v.p[0], end_label);
					freeop(ap1);
					ap2 = g_expr(vnode->v.p[0], flagx);
					if ((unsigned)c1<(unsigned)c2)
						swap_nodes(vnode);
				} else {
#endif
				classic_fashion:
					false_label = nxtlabel();
					falsejp(node->v.p[0], false_label);

					/* all scratch registers are void */
					ap1 = g_expr(vnode->v.p[0], flagx);
					freeop(ap1);

					/* all scratch registers are void */
					g_code(op_bra, 0, mk_label(end_label), NIL_AMODE);

					g_label(false_label);

					ap2 = g_expr(vnode->v.p[1], flagx);
#if defined(ALTERNATE_HOOK) || defined(OPTIMIZED_HOOK)
				}
#endif
			done:
				if (!result_is_void && !equal_address(ap1,ap2))
					ierr(G_HOOK,1);

			} else {
				ap1 = g_expr(node->v.p[0], flagx);
				/* all scratch registers are void */
				g_code(op_tst, (int)node->v.p[0]->esize, ap1, NIL_AMODE);
				freeop(ap1);
				g_code(op_bne, 0, mk_label(end_label), NIL_AMODE);
				ap2 = g_expr(vnode->v.p[1], flagx);
				if (!result_is_void && !equal_address(ap1,ap2))
					ierr(G_HOOK,2);
			}
				g_label(end_label);

#ifdef VCG
			if (best_flagx>=0)
#endif
				return mk_legal(ap2, flags, size);
#ifdef VCG
			if (best_flagx<0) {
				cost1=cost2;
				cost2=vcg_done();
				if (!(flags&F_AREG) || size==1 || flagx==(F_AREG|F_VOL))
					best_flagx = cost1<cost2?(F_DREG|F_VOL):flagx;
				else flagx=F_AREG|F_VOL;
			}
		}
#endif
}

struct amode *g_asadd(struct enode *node, int flags, enum(e_op) op) {
/*
 * generate a plus equal or a minus equal node.
 */
	int 											f;
	struct amode			 *ap1, *ap2;
	switch (node->etype) {
		case bt_char:
		case bt_uchar:
		case bt_short:
		case bt_ushort:
		case bt_long:
		case bt_ulong:
		case bt_pointer:
			if (flags & F_NOVALUE)
				f = F_ALL;
			else
				f = F_ALL | F_USES;
			ap1 = g_expr(node->v.p[0], f);
			if (ap1->mode==am_dreg || ap1->mode==am_areg)
				f = F_ALL | F_SRCOP;
			else f = F_DREG | F_IMMED;
			ap2 = g_expr(node->v.p[1], f);
			validate(ap1);
			g_code(op, (int) node->esize, ap2, ap1);
			freeop(ap2);
			return mk_legal(ap1, flags, node->esize);
#ifndef NOFLOAT
		case bt_float:
#ifdef DOUBLE
		case bt_double:
#endif
			if (op == op_add)
				return as_fcall(node, flags, str(ffpadd));
			else
				return as_fcall(node, flags, str(ffpsub));
#endif
	}
	ierr(G_ASADD,1);
	/* NOTREACHED */
	return 0; // make the compiler happy
}

readonly xstatic int bits_for_size[4]={7,15,0,31};
void g_bitmancode(enum(e_op) op,int size,struct amode *ap1,struct amode *ap2) {
	struct amode *ap3=0;
	if (ap2->mode!=am_dreg && size && size!=1) {
		int bits_to_skip;
		if (ap1->mode!=am_immed || ap1->offset->nodetype!=en_icon)
			uerrc("illegal address mode");
		bits_to_skip=bits_for_size[size-1]-ap1->offset->v.i;
		while (bits_to_skip>=8) {
			if (!ap3) ap3=copy_addr(ap2),ap3->offset=copynode(ap3->offset);
			if (ap3->mode==am_ind)
				ap3->mode=am_indx,ap3->offset=mk_icon(0);
			if (ap3->offset->nodetype==en_icon)
				ap3->offset->v.i++;
			else ap3->offset=mk_node(en_add,ap3->offset,mk_icon(1)),opt4(&ap3->offset);
			bits_to_skip-=8;
		}
		ap1=mk_immed(7-bits_to_skip);
	}
	if (!ap3) ap3=ap2;
	g_code(op,ap2->mode==am_dreg?4:1,ap1,ap3);
}
#ifdef BITWISE_REDUCE
/* the latter relies on the big-endianness of the 68k */
int bitwise_reduction(unsigned long x,int *size) {
	if (!x) return;	// anyway the constant folder will already have taken care :)
	int offs=0,end_offs=0;
	unsigned long v=0xFF<<((*size-1)*8);
	while (!(x&v)) offs++,x<<=8;
	if (!(x&(v>>8))) {
		if (!(x&(v>>16)) && !(x&(v>>24)))
			*size=4;
		else *size=2;
	} else *size=1;
	if (*size==1) {
		if (offs==1 && v!=0xFF) /* avoid such a case... (unless *size _was_ already 1) */
			offs=0,*size=2;
	} else if (offs&1)	/* too bad... we can do nothing */
		offs=0,*size=4;
	return offs;
}
readonly int deflt_types[5]={0,bt_uchar,bt_ushort,0,bt_ulong};
void bitwise_optimize(struct enode *ep,long mode) {
	long *ref=0;
	/* we may not call swap_nodes here! VCG needs to preserve the order */
	if (ep->v.p[0]->nodetype==en_icon && ep->v.p[1]->nodetype==en_ref)
		ref=&ep->v.p[0]->v.i;
	else if (ep->v.p[1]->nodetype==en_icon && ep->v.p[0]->nodetype==en_ref)
		ref=&ep->v.p[1]->v.i;
	if (ref) {
		int offs=bitwise_reduction((*ref)^mode,&ep->esize);
		ep->etype=deflt_types[ep->esize];
		ep->v.p[0]->etype=
#endif

struct amode *g_asxor(struct enode *node, int flags) {
/*
 * generate an ^= node
 */
	int 											f;
	struct amode			 *ap1, *ap2;
	switch (node->etype) {
		case bt_char:
		case bt_uchar:
		case bt_short:
		case bt_ushort:
		case bt_long:
		case bt_ulong:
		case bt_pointer:
			if (flags & F_NOVALUE)
				f = F_ALL;
			else
				f = F_ALL | F_USES;
			ap1 = g_expr(node->v.p[0], f);
			if (ap1->mode==am_dreg || ap1->mode==am_areg)
				f = F_ALL | F_SRCOP;
			else f = F_DREG | F_IMMED;
			ap2 = g_expr(node->v.p[1], f);
			validate(ap1);
			if (ap2->mode==am_immed && ap1->mode!=am_dreg && ap1->mode!=am_areg) {
				int i,n=0,j=0; long z=ap2->offset->v.i;
				for (i=0;i<8*node->esize;i++)
					if (z&(1<<i)) { n++; j=i; }
				if (n==1) {
					g_bitmancode(op_bchg, (int) node->esize, mk_immed(j), ap1);
				} else g_code(op_eor, (int) node->esize, ap2, ap1);
			} else g_code(op_eor, (int) node->esize, ap2, ap1);
			freeop(ap2);
			return mk_legal(ap1, flags, node->esize);
	}
	ierr(G_ASXOR,1);
	/* NOTREACHED */
	return 0; // make the compiler happy
}

struct amode *g_aslogic(struct enode *node, int flags, enum(e_op) op) {
	/*
	 * generate a&= or a|=
	 */
	int 											f;
	struct amode			 *ap1, *ap2, *ap3;
	if (flags & F_NOVALUE)
		f = F_ALL;
	else
		f = F_ALL | F_USES;
	ap1 = g_expr(node->v.p[0], f);
	if (ap1->mode==am_dreg || ap1->mode==am_areg)
		f = (F_ALL & ~F_AREG) | F_SRCOP;
	else f = F_DREG | F_IMMED;
	ap2 = g_expr(node->v.p[1], f);
	validate(ap1);
	if (ap1->mode != am_areg) {
		if (/*(op==op_and || op==op_or || op==op_eor) && */ap2->mode==am_immed
				&& ap1->mode!=am_dreg && ap1->mode!=am_areg
				&& (node->esize==1/* || (ap1->mode!=am_ainc && ap1->mode!=am_adec)*/)) {
			int i,n=-1,j=0,and=(op==op_and); long z=ap2->offset->v.i;
			for (i=0;i<8*node->esize;i++)
				if ((!!(z&(1<<i)))^and) { n++; j=i; }
			if (!n) {
				ap3 = ap1;
				/*								if (node->esize!=1) {
												ap3 = copy_addr(ap1);
												switch (ap1->mode) {
												case am_ind:
												ap3->mode=am_indx;
												ap3->offset=mk_immed(
												}*/
				g_bitmancode(and?op_bclr:(op==op_or?op_bset:op_bchg),
						0/* ALWAYS .B !!! */, mk_immed(j), ap3);
				goto asdone;
			}
		}
		g_code(op, (int) node->esize, ap2, ap1);
	} else {
		ap3 = temp_data();
		g_code(op_move, 4, ap1, ap3);
		g_code(op, (int) node->esize, ap2, ap3);
		g_code(op_move, (int) node->esize, ap3, ap1);
		freeop(ap3);
	}
asdone:
	freeop(ap2);
	return mk_legal(ap1, flags, node->esize);
}

struct amode *g_asshift(struct enode *node, int flags, enum(e_op) op) {
	/*
	 * generate shift equals operators.
	 */
	int f;
	struct amode *ap1, *ap2, *ap3;
	switch (node->etype) {
		case bt_uchar:
		case bt_char:
		case bt_ushort:
		case bt_short:
		case bt_ulong:
		case bt_long:
		case bt_pointer:
			if (flags & F_NOVALUE)
				f = F_ALL;
			else
				f = F_ALL | F_USES;
			ap1 = g_expr(node->v.p[0], f);
			if (ap1->mode != am_dreg) {
				ap3 = temp_data();
				g_code(op_move, (int) node->esize, ap1, ap3);
			} else
				ap3 = ap1;
			ap2 = g_expr(node->v.p[1], F_DREG | F_IMMED);

			/* add if const=1 and op is << */
			if (op==op_lsl && ap2->mode == am_immed && ap2->offset->nodetype == en_icon
					&& ap2->offset->v.i == 1) {
				op=op_add; ap2=ap3;
			}
			/* quick constant if 2<=const<=8 */
			if (ap2->mode == am_immed && ap2->offset->nodetype == en_icon
					&& (ap2->offset->v.i > 8 || ap2->offset->v.i < 1)) {
				/*if (ap2->offset->v.i <= 0)
				  uwarn("negative shift constant");*/
				ap2 = mk_legal(ap2, F_DREG, 1l);
			}
			validate(ap3);
			g_code(op, (int) node->esize, ap2, ap3);
			if (ap2 != ap3)
				freeop(ap2);
			if (ap3 != ap1) {
				g_code(op_move, (int) node->esize, ap3, ap1);
				freeop(ap3);
			}
			return mk_legal(ap1, flags, node->esize);
	}
	ierr(G_ASSHIFT,1);
	/* NOTREACHED */
	return 0; // make the compiler happy
}

struct amode *g_asmul(struct enode *node, int flags) {
	/*
	 * generate a *= node.
	 */
	struct amode			 *ap1, *ap2, *ap3;
	enum(e_op)		op = op_mulu;
	switch (node->etype) {
		case bt_char:
			ap1 = g_expr(node->v.p[0], F_ALL | F_USES);
			if (ap1->mode != am_dreg) {
				ap2 = temp_data();
				g_code(op_move, 1, ap1, ap2);
			} else
				ap2 = ap1;
			g_code(op_ext, 2, ap2, NIL_AMODE);
			ap3 = g_expr(node->v.p[1], F_DREG | F_IMMED);
			if (ap3->mode == am_dreg)
				g_code(op_ext, 2, ap3, NIL_AMODE);
			validate(ap2);
			g_code(op_muls, 0, ap3, ap2);
			freeop(ap3);
			if (ap2 != ap1) {
				validate(ap1);
				g_code(op_move, 1, ap2, ap1);
				freeop(ap2);
			}
			return mk_legal(ap1, flags, node->esize);
		case bt_uchar:
			ap1 = g_expr(node->v.p[0], F_ALL | F_USES);
			if (ap1->mode != am_dreg) {
				ap2 = temp_data();
				g_code(op_move, 1, ap1, ap2);
			} else
				ap2 = ap1;
			g_code(op_and, 2, mk_immed(255l), ap2);
			ap3 = g_expr(node->v.p[1], F_DREG | F_IMMED);
			if (ap3->mode == am_dreg)
				g_code(op_and, 2, mk_immed(255l), ap3);
			validate(ap2);
			g_code(op_mulu, 0, ap3, ap2);
			freeop(ap3);
			if (ap2 != ap1) {
				validate(ap1);
				g_code(op_move, 1, ap2, ap1);
				freeop(ap2);
			}
			return mk_legal(ap1, flags, node->esize);
		case bt_short:
			op = op_muls;
		case bt_ushort:
			ap1 = g_expr(node->v.p[0], F_ALL | F_USES);
			ap2 = g_expr(node->v.p[1], F_ALL);
			validate(ap1);
			if (ap1->mode != am_dreg) {
				ap3 = temp_data();
				g_code(op_move, 2, ap1, ap3);
				g_code(op, 0, ap2, ap3);
				freeop(ap2);
				freeop(ap3);
				g_code(op_move, 2, ap3, ap1);
			} else {
				g_code(op_muls, 0, ap2, ap1);
				freeop(ap2);
			}
			return mk_legal(ap1, flags, node->esize);
		case bt_long:
		case bt_ulong:
		case bt_pointer:
			return as_fcall(node, flags, "__mulsi3");
#ifndef NOFLOAT
		case bt_float:
#ifdef DOUBLE
		case bt_double:
#endif
			return as_fcall(node, flags, str(ffpmul));
#endif
	}
	ierr(G_ASMUL,1);
	/* NOTREACHED */
	return 0; // make the compiler happy
}

struct amode *g_asdiv(struct enode *node, int flags) {
/*
 * generate /= and %= nodes.
 */
	struct amode	 *ap1, *ap2, *ap3;
	switch (node->etype) {
		case bt_char:
			ap1 = g_expr(node->v.p[0], F_ALL | F_USES);
			if (ap1->mode != am_dreg) {
				ap2 = temp_data();
				g_code(op_move, 1, ap1, ap2);
			} else
				ap2 = ap1;
			g_code(op_ext, 2, ap2, NIL_AMODE);
			g_code(op_ext, 4, ap2, NIL_AMODE);
			ap3 = g_expr(node->v.p[1], F_DREG | F_IMMED);
			if (ap3->mode == am_dreg)
				g_code(op_ext, 2, ap3, NIL_AMODE);
			validate(ap2);
			g_code(op_divs, 0, ap3, ap2);
			freeop(ap3);
			if (ap2 != ap1) {
				validate(ap1);
				g_code(op_move, 1, ap2, ap1);
				freeop(ap2);
			}
			return mk_legal(ap1, flags, node->esize);
		case bt_uchar:
			ap1 = g_expr(node->v.p[0], F_ALL | F_USES);
			if (ap1->mode != am_dreg) {
				ap2 = temp_data();
				g_code(op_move, 1, ap1, ap2);
			} else
				ap2 = ap1;
			g_code(op_and, 4, mk_immed(255l), ap2);
			ap3 = g_expr(node->v.p[1], F_DREG | F_IMMED);
			if (ap3->mode == am_dreg)
				g_code(op_and, 2, mk_immed(255l), ap3);
			validate(ap2);
			g_code(op_divu, 0, ap3, ap2);
			freeop(ap3);
			if (ap2 != ap1) {
				validate(ap1);
				g_code(op_move, 1, ap2, ap1);
				freeop(ap2);
			}
			return mk_legal(ap1, flags, node->esize);
		case bt_short:
		case bt_ushort:
			ap1 = temp_data();
			ap2 = g_expr(node->v.p[0], F_ALL | F_USES);
			validate(ap1);
			g_code(op_move, 2, ap2, ap1);
			ap3 = g_expr(node->v.p[1], F_ALL & ~F_AREG);
			validate(ap2);
			validate(ap1);
			if (node->etype == bt_short) {
				g_code(op_ext, 4, ap1, NIL_AMODE);
				g_code(op_divs, 0, ap3, ap1);
			} else {
				g_code(op_and, 4, mk_immed(65535l), ap1);
				g_code(op_divu, 0, ap3, ap1);
			}
			freeop(ap3);
			g_code(op_move, 2, ap1, ap2);
			freeop(ap2);
			return mk_legal(ap1, flags, 2l);
		case bt_long:
			return as_fcall(node, flags, "__divsi3");
		case bt_ulong:
		case bt_pointer:
			return as_fcall(node, flags, "__udivsi3");
#ifndef NOFLOAT
#ifdef DOUBLE
		case bt_double:
#endif
		case bt_float:
			return as_fcall(node, flags, str(ffpdiv));
#endif
	}
	ierr(G_ASDIV,1);
	/* NOTREACHED */
	return 0; // make the compiler happy
}

struct amode *g_asmod(struct enode *node, int flags) {
/*
 * generate /= and %= nodes.
 */
	struct amode	 *ap1, *ap2, *ap3;
	switch (node->etype) {
		case bt_short:
		case bt_ushort:
			ap1 = temp_data();
			ap2 = g_expr(node->v.p[0], F_ALL | F_USES);
			validate(ap1);
			g_code(op_move, 2, ap2, ap1);
			ap3 = g_expr(node->v.p[1], F_ALL & ~F_AREG);
			validate(ap2);
			validate(ap1);
			if (node->etype == bt_short) {
				g_code(op_ext, 4, ap1, NIL_AMODE);
				g_code(op_divs, 0, ap3, ap1);
			} else {
				g_code(op_and, 4, mk_immed(65535l), ap1);
				g_code(op_divu, 0, ap3, ap1);
			}
			g_code(op_swap, 0, ap1, NIL_AMODE);
			freeop(ap3);
			g_code(op_move, 2, ap1, ap2);
			freeop(ap2);
			return mk_legal(ap1, flags, 2l);
		case bt_long:
			return as_fcall(node, flags, ".lrem");
		case bt_ulong:
		case bt_pointer:
			return as_fcall(node, flags, ".ulrem");
	}
	ierr(G_ASMOD,1);
	/* NOTREACHED */
	return 0; // make the compiler happy
}

void structassign(struct amode *ap1, struct amode *ap2, long size, int mode) {
/*
 * assign structure from ap1 to ap2
 * ap1, ap2 are scratch address registers
 */
#ifdef BIGMEM
				long	loop_c;
#else
				int		loop_c;
#endif
				int 	rest;
				struct amode	 *ap3;
				unsigned int			label;

				ap1 = copy_addr(ap1);
				ap2 = copy_addr(ap2);
				ap1->mode = mode;
				ap2->mode = mode;
				loop_c = size >> 2;
				rest = (int) (size & 3);
				if (loop_c <= 5) 				/* loop-unrolling */
					while (loop_c--)
								g_code(op_move, 4, ap1, ap2);
				else {
				loop_c--; 								/* for dbra */
				ap3 = temp_data();
				freeop(ap3);
				label = nxtlabel();
#ifdef BIGMEM
				if (loop_c <= 65535) {			// single loop
#endif
								g_code(op_move, 2, mk_immed(loop_c), ap3);
								g_label(label);
								g_code(op_move, 4, ap1, ap2);
								g_code(op_dbra, 0, ap3, mk_label(label));
#ifdef BIGMEM
				} else {								// extended loop
								g_code(op_move, 4, mk_immed(loop), ap3);
								g_label(label);
								g_code(op_move, 4, ap1, ap2);
								g_code(op_dbra, 0, ap3, mk_label(label));
								g_code(op_sub, 4, mk_immed(65536l), ap3);
								g_code(op_bhs, 0, mk_label(label), NIL_AMODE);
				}
#endif
				}
#if AL_DEFAULT!=2
				if (rest >= 2) {
					rest -= 2;
					g_code(op_move, 2, ap1, ap2);
				}

				/* This cannot happen if the size of structures is always even */
				if (rest)
					g_code(op_move, 1, ap1, ap2);
#ifdef SHORT_STRUCT_PASSING
#error "Much of the short struct stuff assumes that structs whose size is under 4 \
	really are short structs (while they aren't, since to do an assignment of a  \
	3-byte struct, you have to do 2 moves, if not 3...)"
#error "So let AL_DEFAULT be 2."
#endif
#else
				if (rest)
					g_code(op_move, 2, ap1, ap2);
#endif
}

struct amode *g_assign(struct enode *node, int flags) {
/*
 * generate code for an assignment node.
 */
	int 			f;
	struct amode   *ap1, *ap2, *ap3;
	struct enode   *ep;
	long			size = node->esize;
	if (flags & F_NOVALUE)
		f = F_ALL;
	else
		f = F_ALL | F_USES;
	if (bt_aggregate(node->etype)) {
#ifdef SHORT_STRUCT_PASSING
		if (node->esize<=4) {
			ap1 = g_expr(node->v.p[1], F_AREG);
			ap2 = g_expr(node->v.p[0], F_AREG);
			validate(ap1);
			structassign(ap1, ap2, (long)node->esize, am_ind);
			freeop(ap2);
			return mk_legal(ap1, flags, 4l);
		}
#endif
		/*
		* Other parts of this module return a pointer to a struct in a register,
		* not the struct itself
		*/
		ap1 = g_expr(node->v.p[1], F_AREG | F_VOL);
		ap2 = g_expr(node->v.p[0], F_AREG | F_VOL);
		validate(ap1);

		/* hacky: save ap1 if needed later, structassign destroys it */
		if (!(flags & F_NOVALUE)) {
			ap3 = temp_addr();
			/* BTW, this code gets eliminated with MAX_ADDR = 1 */
			g_code(op_move, 4, ap1, ap3);
			structassign(ap3, ap2, (long)size, am_ainc);
			freeop(ap3);
			freeop(ap2);
			validate(ap1);
			return mk_legal(ap1, flags, 4l);
		} else {					/* no need to save any registers */
			structassign(ap1, ap2, (long)node->esize, am_ainc);
			freeop(ap2);
			/* mk_legal is a no-op here */
			return mk_legal(ap1, flags, 4l);
		}
	}
	if (node->v.p[0]->nodetype == en_fieldref) {
		long mask;
		int i;
		/*
		 * Field assignment
		 */

#ifdef OLD_FIELD_ASSIGN
		/* get the value */
		ap1 = g_expr(node->v.p[1], F_DREG | F_VOL);
		i = node->v.p[0]->bit_width;
		mask = 0;
		while (i--)
			mask = mask + mask + 1;
		g_code(op_and, (int) size, mk_immed(mask), ap1);
		i = (node->v.p[0]->esize<<3)-node->v.p[0]->bit_offset-node->v.p[0]->bit_width;
		mask <<= i;
		if (!(flags & F_NOVALUE)) {
			/*
			 * result value needed
			 */
			ap3 = temp_data();
			g_code(op_move, 4, ap1, ap3);
		} else
			ap3 = ap1;
		if (i > 0) {
			if (i == 1) {
				/* add dn,dn */
				g_code(op_add, (int) node->esize, ap3, ap3);
			} else if (i <= 8) {
				g_code(op_lsl, (int) size,
					mk_immed(i), ap3);
			} else {
				ap2 = temp_data();
				g_code(op_moveq, 0,
					mk_immed(i), ap2);
				g_code(op_lsl, (int) size, ap2, ap3);
				freeop(ap2);
			}
		}
		ep = mk_node(en_ref, node->v.p[0]->v.p[0], NIL_ENODE);
		ep->esize = 1;
		ap2 = g_expr(ep, F_MEM);
		validate(ap3);
		g_code(op_and, (int) size, mk_immed(~mask), ap2);
		g_code(op_or, (int) size, ap3, ap2);
		freeop(ap2);
		if (!(flags & F_NOVALUE)) {
			freeop(ap3);
			validate(ap1);
		}
#else
		{
			ep = mk_node(en_ref, node->v.p[0]->v.p[0], NIL_ENODE);
			ep->esize = node->v.p[1]->esize;
			ep->etype = node->v.p[1]->etype;
			ap2 = g_expr(ep, F_MEM);

			ep=node->v.p[1];
			/* get the value */
			i = node->v.p[0]->bit_width;
			mask = 0;
			while (i--)
				mask = mask + mask + 1;
			ep = mk_node(en_and, ep, mk_icon(mask));
			ep->esize=ep->v.p[0]->esize;
			ep->etype=ep->v.p[0]->etype;
			ep->v.p[1]->esize=ep->esize;
			ep->v.p[1]->etype=ep->etype;
			i = (node->v.p[0]->esize<<3)-node->v.p[0]->bit_offset-node->v.p[0]->bit_width;
			mask <<= i;
			ep = mk_node(en_lsh, ep, mk_icon(i));
			ep->esize=ep->v.p[0]->esize;
			ep->etype=ep->v.p[0]->etype;
			ep->v.p[1]->esize=1;
			ep->v.p[1]->etype=bt_char;
			opt0(&ep);
			ap1 = g_expr(ep, F_DREG | F_IMMED);
			validate(ap2);
			if (ap1->mode==am_immed && node->v.p[0]->bit_width==1) {
				// note that thus, we need not free/validate anything...
				if (!((~ap1->offset->v.i)&mask)) {
					ap1->offset->v.i = i;
					g_bitmancode(op_bset, (int)size, ap1, ap2);
					freeop(ap2);
					if (!(flags & F_NOVALUE))
						return mk_legal(mk_immed(1L),flags,node->esize);
					return NIL_AMODE;
				} else if (!(ap1->offset->v.i&mask)) {
					ap1->offset->v.i = i;
					g_bitmancode(op_bclr, (int)size, ap1, ap2);
					freeop(ap2);
					if (!(flags & F_NOVALUE))
						return mk_legal(mk_immed(0L),flags,node->esize);
					return NIL_AMODE;
				}
			} else {
				g_code(op_and, (int) size, mk_immed(~mask), ap2);
				g_code(op_or, (int) size, ap1, ap2);
				freeop(ap1);
				if (!(flags & F_NOVALUE))
					return g_fderef(node->v.p[0], ap2, flags);
				freeop(ap2);
				return NIL_AMODE;
			}
		}
#endif
		return mk_legal(ap1, flags, size);
	}
	/*
	 * (uns.) char, (uns.) short, (uns.) long, float
	 * 
	 * we want to pass the right hand side as the expression value. This can't
	 * be done if the left side is a register variable on which the right
	 * hand side addressing mode depends. But if the left side IS a register
	 * variable, it is desirable to pass the left side, so no problem.
	 */
	if (node->v.p[0]->nodetype == en_tempref) {
		/* pass the left side as expr. value */
		ap1 = g_expr(node->v.p[0], f);
		ap2 = g_expr(node->v.p[1], F_ALL | F_SRCOP);
		validate(ap1);
		g_code(op_move, (int) size, ap2, ap1);
		freeop(ap2);
		return mk_legal(ap1, flags, size);
	} else {
		/* pass the right side as expr. value */
		/* normally, this is more efficient */
		ap1 = g_expr(node->v.p[1], f | F_SRCOP);
		ap2 = g_expr(node->v.p[0], F_ALL);
		validate(ap1);
		g_code(op_move, (int) size, ap1, ap2);
		freeop(ap2);
		return mk_legal(ap1, flags, size);
	}
}

struct amode *g_aincdec(struct enode *node, int flags, enum(e_op) op) {
/*
 * generate an auto increment or decrement node. op should be either op_add
 * (for increment) or op_sub (for decrement).
 */
	struct amode	 *ap1, *ap2;
	switch (node->etype) {
		case bt_uchar:
		case bt_char:
		case bt_short:
		case bt_ushort:
		case bt_long:
		case bt_ulong:
		case bt_pointer:
			if (flags & F_NOVALUE) {/* dont need result */
				ap1 = g_expr(node->v.p[0], F_ALL);
				g_code(op, (int) node->esize, mk_immed((long) node->v.p[1]->v.i),
						ap1);
				return mk_legal(ap1, flags, node->esize);
			}
			if (flags & F_DREG)
				ap1 = temp_data();
			else
				ap1 = temp_addr();
			ap2 = g_expr(node->v.p[0], F_ALL | F_USES);
			validate(ap1);
			g_code(op_move, (int) node->esize, ap2, ap1);
			g_code(op, (int) node->esize, mk_immed((long) node->v.p[1]->v.i), ap2);
			freeop(ap2);
			return mk_legal(ap1, flags, node->esize);
	}
	ierr(G_AINCDEC,1);
	/* NOTREACHED */
	return 0; // make the compiler happy
}

long push_param(struct enode *ep) {
/*
 * push the operand expression onto the stack. return the number of bytes
 * pushed
 */
	struct amode	 *ap;
#ifdef OLD_STRUCT_PUSH
	struct amode	 *ap1;
#endif
	long											size = ep->esize;
	int old_pushed;

	/* pushing of aggregates: (short) structures and unions, as well as BCD floats */
	if (bt_aggregate(ep->etype)) {
		if (ep->nodetype==en_ref)
			ep = ep->v.p[0];
			/* all other cases return a pointer to the struct anyway */
#ifdef OLD_STRUCT_PUSH
		/* allocate stack space */
		g_code(op_sub, 4, mk_immed(size), mk_reg(STACKPTR));
		/*
		 * F_VOL was missing in the following line --
		 * it took a hard-core debugging session to find this error
		 */
		ap = g_expr(ep, F_AREG | F_VOL);
		ap1 = temp_addr();
		validate(ap);
		g_code(op_move, 4, mk_reg(STACKPTR), ap1);
		/* now, copy it on stack - the same as structassign */
		structassign(ap, ap1, size, am_ainc);
		freeop(ap1);
		freeop(ap);
#else
		if ((size&1)) size++;	/* otherwise there will be a bunch of problems */

		{
		struct enode *ep2=mk_icon(size);
		ep2->etype=bt_long; ep2->esize=4;
		ep = mk_node(en_add,ep,ep2);
		ep->etype=bt_pointer; ep->esize=4;
		opt0(&ep);
		ap = g_expr(ep, F_AREG | F_VOL);
		/* now, copy it on stack - the same as structassign */
		structassign(ap, mk_reg(STACKPTR), size, am_adec);
		freeop(ap);
		}
#endif
		return size;
	}
	old_pushed = pushed;
	pushed = 0;
	ap = g_expr(ep, F_ALL | F_DEREF);

	/*
	 * This is a hook for the peephole optimizer, which will convert lea
	 * <ea>,An + pea (An) ==> pea <ea>
	 */

#ifdef POP_OPT
	if (old_pushed && pushed)
		g_code(_op_adj, 0, NIL_AMODE, NIL_AMODE);
#endif
	pushed = 1;
	if ((ap->mode == am_areg || ap->mode == am_immed)
		&& size == 4 && ap->preg <= MAX_ADDR) {
		ap = copy_addr(ap);
		am_doderef(ap->mode);
		g_code(op_pea, 0, ap, NIL_AMODE);
	} else
		g_code(op_move, (int) size, ap, push_am);
	freeop(ap);
	return size+(size&1);
}

int req_all_aregs(struct enode *plist,int rp_dn,int rp_an) {
	/* requires all regs if and only if (num_params>=regs_num || num_pointers>=aregs_num) */
	rp_dn+=rp_an;
	while (plist) {
		if (!--rp_dn) return 1;
		if (plist->v.p[0]->etype==bt_pointer && !--rp_an) return 1;
		plist=plist->v.p[1];
	}
	return 0;
}

long g_parms(struct enode *plist
#ifdef REGPARM
	, int rp_dn, int rp_an, struct amode **a1ap
#endif
		) {
/*
 * push a list of parameters onto the stack and return the number of
 * parameters pushed.
 */
#ifdef BIGSTACK
	long i=0;
#else
	int i=0;
#endif
	pushed = 0;
#ifdef REGPARM
#ifdef PC
	if (rp_dn IS_INVALID || rp_an IS_INVALID)
		ierr(G_PARMS,1);
#endif
	if (rp_dn || rp_an) {
		int nr=rp_dn+rp_an,np=0,n;
		struct enode *ep=plist,**p,**dp,**ap,*allocbase[16],*list[16];
		struct amode deep[CONVENTION_MAX_DATA+1+CONVENTION_MAX_ADDR+1],*deepp;
		while (ep) np++, ep=ep->v.p[1];
		/* first, push stack params while all the temp registers are free */
		while (np>nr) {
			i += push_param(plist->v.p[0]);
			plist = plist->v.p[1];
			np--;
		}
		/* store the last params so we can examinate them in the correct order */
		n=np; while (n--) list[n]=plist->v.p[0], plist=plist->v.p[1];
		/* now, fill in 'allocbase' */
		p=list; dp=&allocbase[0]; ap=&allocbase[8];
		n=np; while (n--) {
			ep=*p++;
			if ((ep->etype==bt_pointer && rp_an) || !rp_dn)
				*ap++=ep, rp_an--;
			else *dp++=ep, rp_dn--;
		}
		*ap=NULL; *dp=NULL;
		/* finally, load all the parameters into the correct registers */
		dp=&allocbase[0]; ap=&allocbase[8];
		deepp=deep;
		n=8; while (n--) {	/* push d0/a0/d1/a1/... */
			if (*dp) {
//#ifdef PC
				struct amode *amp=
//#endif
					g_expr(*dp++,F_DREG|F_VOL);
#ifdef INFINITE_REGISTERS
				struct amode *ap2 = (struct amode *) xalloc((int) sizeof(struct amode), 0);
				ap2->mode = am_dreg;
				ap2->preg = TDREGBASE+dp-allocbase-1;
				g_code(op_move, dp[-1]->esize, amp, ap2);
				freeop(amp);
				amp=ap2;
#endif
#ifdef PC
				if (amp->mode!=am_dreg || amp->preg!=TDREGBASE+dp-allocbase-1)
					ierr(REGPARM,1);
#endif
				*deepp++=*amp;
			}
			if (*ap) {
//#ifdef PC
				struct amode *amp=
//#endif
					*a1ap = g_expr(*ap++,F_AREG|F_VOL);
#ifdef INFINITE_REGISTERS
				struct amode *ap2 = (struct amode *) xalloc((int) sizeof(struct amode), 0);
				ap2->mode = am_areg;
				ap2->preg = TDREGBASE+ap-allocbase-1-8;
				g_code(op_move, ap[-1]->esize, amp, ap2);
				freeop(amp);
				amp=ap2;
#endif
#ifdef PC
				if (amp->mode!=am_areg || amp->preg!=TDREGBASE+ap-allocbase-1-8)
					ierr(REGPARM,2);
#endif
				*deepp++=*amp;
			}
		}
		while (deepp>deep)
			validate(--deepp);
	} else {
#endif
		while (plist != 0) {
			i += push_param(plist->v.p[0]);
			plist = plist->v.p[1];
		}
#ifdef REGPARM
	}
#endif
	return i;
}

struct amode *func_result(int flags, long bytes) {
	/*
	 * saves a function call result in D0 it is assumed that flags contain
	 * either F_DREG or F_AREG return value is the addressing mode of the
	 * result bytes is the number of bytes to pop off the stack
	 *
	 * This routine does not use mk_legal and takes care of the stuff itself.
	 */
	struct amode *ap;
	if (bytes != 0)
		/* adjust stack pointer */
		g_code(op_add, 4, mk_immed(bytes), mk_reg(STACKPTR));
	if (flags & F_NOVALUE)
		return 0;
	if (flags & F_DREG) {
		ap = temp_data();
		g_code(op_move, 4, mk_reg(RESULT), ap);
	} else if (flags & F_AREG) {
		ap = temp_addr();
		g_code(op_move, 4, mk_reg(RESULT), ap);
#ifdef PC
	} else {
		ierr(FUNC_RESULT,1);
		return 0; // make the compiler happy
#endif
	}
	return ap; 
}

struct amode *func_result2(int flags, long bytes, int reg) {
	/*
	 * Saves a function call result in REG. It is assumed that flags contain
	 * either F_DREG or F_AREG. Return value is the addressing mode of the
	 * result; bytes is the number of bytes to pop off the stack
	 *
	 * This routine does not use mk_legal and takes care of the stuff itself.
	 */
	struct amode	 *ap;
	if (bytes != 0) /* adjust stack pointer */
		g_code(op_add, 4, mk_immed(bytes), mk_reg(STACKPTR));
	if (flags & F_NOVALUE)
		return 0;
	if ((flags & F_DREG) && reg==RESULT) {		// permet d'éviter move.l a0,d0 /
		ap = temp_data();						//  move.l d0,rn #roll#
		g_code(op_move, 4, mk_reg(reg), ap);
	} else if (flags & F_AREG) {
		ap = temp_addr();
		g_code(op_move, 4, mk_reg(reg), ap);
	} else {
#ifdef PC
		if (flags & F_DREG) {
#endif
		ap = temp_data();
		g_code(op_move, 4, mk_reg(reg), ap);
#ifdef PC
	} else {
		ierr(FUNC_RESULT,2);
		return 0; // make the compiler happy
	}
#endif
	}
	return ap;
}

struct amode *as_fcall(struct enode *node, int flags, char *libname) {
/* assignment operations with library calls */
	long											i;
	struct amode	 *ap1;
	long											size;
	size = node->esize;
	temp_inv();
	i = push_param(node->v.p[1]);
	if (node->v.p[0]->nodetype == en_tempref) {
		/* ap1 cannot be destroyed, no problem */
		ap1 = g_expr(node->v.p[0], F_DREG | F_AREG);
		g_code(op_move, (int) size, ap1, push_am);
		i += size;
		call_library(libname);
		/* ap1 is always valid and not equal to RESULT */
		g_code(op_move, (int) size, mk_reg(RESULT), ap1);
	} else {
		uwarn("possible flaw in lib call");
		ap1 = g_expr(node->v.p[0], F_DREG | F_AREG);
		g_code(op_move, (int) size, ap1, push_am);
		i += size;
		call_library(libname);
		/* ap1 is always valid and not equal to RESULT */
		g_code(op_move, (int) size, mk_reg(RESULT), ap1);
	}
	g_code(op_add, 4, mk_immed((long) i), mk_reg(STACKPTR));
	if (!(flags & F_NOVALUE)) {
		if (flags & F_AREG)
			ap1 = temp_addr();
		else
			ap1 = temp_data();
		g_code(op_move, 4, mk_reg(RESULT), ap1);
		return mk_legal(ap1, flags, size);
	} else
		return 0;
}

#ifndef __HAVE_REGS_IMAGE
#define __HAVE_REGS_IMAGE
typedef struct _regsimg {
#ifndef INFINITE_REGISTERS
	int reg_alloc_ptr,reg_stack_ptr;
	int next_data,next_addr;
#endif
} REGS_IMAGE;
#endif

readonly struct amode am_a1={am_areg,0,1,0,0,0};
readonly struct amode am_a2={am_areg,0,2,0,0,0};
readonly struct amode am_a2ind={am_ind,0,2,0,0,0};

extern int hexatoi(char *s);

struct amode *g_fcall(struct enode *node, int flags) {
/*
 * generate a function call node and return the address mode of the result.
 */
		struct amode	 *ap; enum(e_node) nt;
		long											i;
#ifdef SHORT_STRUCT_PASSING
		int short_struct_return=0;
#endif
#ifdef PC
		// avoid a compiler warning...
		struct amode *struct_ap = 0;
#else
		struct amode *struct_ap = struct_ap;
#endif
	#ifdef REGPARM
//		struct amode	 *regap[(MAX_DATA+1+MAX_ADDR+1)+1],**rapp;
		struct amode *a1ap; struct enode *dep;
		REGS_IMAGE regs_img;
		int allaregs_patch=0;
	#endif
		/* push any used addr&data temps */
		dep = node->v.p[0];
		while (dep->nodetype==en_cast && dep->esize==4) dep=dep->v.p[0];
		nt = dep->nodetype;
//		nt = node->v.p[0]->nodetype;
/*		if (nt==en_nacon && !strcmp(node->v.p[0]->v.ensp,"rand"))
			printf("jdfio");*/
		temp_inv();
	#ifdef REGPARM
		useregs(&regs_img);
#ifndef INFINITE_REGISTERS
		if (node->rp_an>MAX_ADDR && !(nt==en_nacon || nt==en_labcon
				|| (nt==en_tempref && node->v.p[0]->v.i>=AREGBASE))
			&& req_all_aregs(node->v.p[1],node->rp_dn,node->rp_an))
				allaregs_patch=1;
#endif
		i = g_parms(node->v.p[1],node->rp_dn,
			node->rp_an/*-allaregs_patch*/,&a1ap); /* generate parameters */
		if (allaregs_patch) freeop(a1ap);
	#else
		i = g_parms(node->v.p[1]);		/* generate parameters */
	#endif
		/*
		 * for functions returning a structure or a union, push a pointer to the
		 * return value as additional argument The scratch space will be
		 * allocated in the stack frame of the calling function.
		 */
		if (bt_aggregate(node->etype)) {
			struct_ap = mk_scratch(node->esize);
#ifdef SHORT_STRUCT_PASSING
			if (node->esize>4)
#endif
				g_code(op_pea, 0, struct_ap, NIL_AMODE), i += 4l;
#ifdef SHORT_STRUCT_PASSING
			else short_struct_return=1;
#endif
			// freeop(ap); it is useless, as scratch amode's need not be freed
		}
		/* call the function */
		if (nt == en_nacon || nt == en_labcon) {
			/*if (!strcmp(node->v.p[0]->v.ensp,"rand"))
				printf("jdfio");*/
#ifdef FLINE_RC
			if (fline_rc && nt==en_nacon && dep->v.ensp && !strncmp(dep->v.ensp,"_ROM_CALL_",10))
				g_code(op_dc, 2, mk_offset(mk_icon(0xF800+hexatoi(dep->v.ensp+10))), NIL_AMODE);
			else
#endif
			g_code(op_jsr, 0, mk_offset(dep), NIL_AMODE);
//			g_code(op_jsr, 0, mk_offset(node->v.p[0]), NIL_AMODE);
		} else {
#ifdef REGPARM
			if (allaregs_patch) {
				g_code(op_move, 4, (struct amode *)&am_a2, push_am);
				g_code(op_move, 4, (struct amode *)&am_a1, (struct amode *)&am_a2);
			}
#endif
			ap = g_expr(node->v.p[0], F_AREG);
			ap = copy_addr(ap);
			ap->mode = am_ind;
			freeop(ap);
#ifdef REGPARM
			if (allaregs_patch) {
#ifdef PC
				if (ap->preg!=1)
					ierr(REGPARM,3);
#endif
/*				struct amode *ap2 =
					(struct amode *) xalloc((int) sizeof(struct amode), AMODE);*/
				g_code(op_exg, 4, (struct amode *)&am_a1, (struct amode *)&am_a2);
				ap=(struct amode *)&am_a2ind;
/*				ap2->mode = am_areg;
				ap2->preg = node->rp_an-1;
				g_code(op_move, 4, pop_am, ap2);	// always long since it's 'bt_pointer'
				i-=4;*/
			}
#endif
			g_code(op_jsr, 0, ap, NIL_AMODE);
#ifdef REGPARM
			if (allaregs_patch)
				g_code(op_move, 4, pop_am, (struct amode *)&am_a2);
#endif
		}
	#ifdef REGPARM
		/* free register params */
/*		rapp=regap;
		while (*rapp)
			freeop(*rapp++);*/
		freeregs(&regs_img);
	#endif
#ifdef SHORT_STRUCT_PASSING
	if (short_struct_return) {
		if (flags & F_NOVALUE)
			return func_result2(F_NOVALUE,i,0);
		g_code(op_lea,0,struct_ap,mk_reg(PRESULT));	/* note : this *is* commutative with
													 * popping args off as we use a virtual
													 * a6-like register for struct_ap...
													 *   But we had better postpone this lea
													 * as late as possible to take advantage
													 * of peephole optimizations.
													 */
		ap = mk_reg(PRESULT); ap->mode=am_ind;
		g_code(op_move,node->esize,mk_reg(RESULT),ap);
	}
#endif
	return func_result2(flags, i,
		(node->etype>=bt_pointer&&node->etype<=bt_union)?PRESULT:RESULT);
}

struct amode *g_alloca(struct enode *node) {
	struct enode *ep=mk_node(en_add,node->v.p[0],mk_icon(1));
	struct amode *ap1, *ap2;
	ep->etype=node->v.p[0]->etype;
	ep->esize=node->v.p[0]->esize;
	ep->v.p[1]->etype=ep->etype;
	ep->v.p[1]->esize=ep->esize;
	ep=mk_node(en_and,mk_icon(-2),ep);
	ep->etype=ep->v.p[1]->etype;
	ep->esize=ep->v.p[1]->esize;
	ep->v.p[0]->etype=ep->etype;
	ep->v.p[0]->esize=ep->esize;
	ap1 = mk_reg(STACKPTR);
	opt0(&ep);
	ap2 = g_expr(ep, F_DREG | F_IMMED);
	g_code(op_sub, 2, ap2, ap1);
	freeop(ap2);
	return ap1;
}

#define F_GCAST 0
//#define F_GCAST F_USES => seems completely useless...

#ifdef G_CAST2
struct amode *g_cast2(struct enode *ep, enum(e_bt) typ2, int flags) {
/*
 * generates code for a en_cast node
 *
 */
	struct amode	*ap;
	enum(e_bt) typ1=ep->etype;
	if (typ1==bt_long && typ2==bt_short) {	/* useful when using short mul's */
		if (ep->nodetype==en_cast && ep->v.p[0]->etype==bt_short)
			return g_expr(ep->v.p[0], flags);
	}
	ap=g_expr(ep, F_ALL | F_SRCOP | F_GCAST);
	return g_cast(ap, typ1, typ2, flags);
}
#endif

struct amode *g_cast(struct amode *ap, enum(e_bt) typ1, enum(e_bt) typ2, int flags) {
/*
 * generates code for an en_cast node
 *
 */
	struct amode	 *ap1;
	int 									 f;

	if (flags & F_NOVALUE) {
		freeop(ap);
		return 0;
	}

	/* the following code from now on is meaningless :
	 *  'useless' casts are sometimes generated to keep track of
	 *  the previous TYP structure */
#if 0
	if (typ1 == typ2)
		/*
		 * this can happen in with the g_xmul stuff, where a cast from
		 * (u)short to long now casts from (u)short to (u)short for an 68000
		 * mulu or muls instruction.
		 * It is safe to cut things short then.
		 * It should not happen with types other than (u)short, but
		 * it does not harm either.
		 */
		if (typ1 == bt_short || typ1 == bt_ushort)
			return mk_legal(ap, flags, 2l);
		//else
		//	msg("DEBUG: g_cast: typ1 == typ2\n");
#endif

	switch (typ2) {
		/* switch: type to cast to */
		case bt_char:
		case bt_uchar:
			switch (typ1) {
				case bt_uchar:
				case bt_char:
					return mk_legal(ap, flags, 1l);
				case bt_ushort:
				case bt_short:
					if ((ap1 = g_offset(ap, 1)) == 0)
						ap1 = mk_legal(ap, F_DREG, 2l);
					return mk_legal(ap1, flags, 1l);
				case bt_ulong:
				case bt_long:
				case bt_pointer:
					if ((ap1 = g_offset(ap, 3)) == 0)
						ap1 = mk_legal(ap, F_DREG, 4l);
					return mk_legal(ap1, flags, 1l);
				case bt_float:
#ifdef DOUBLE
				case bt_double:
#endif
					return g_cast(g_cast(ap, bt_double, bt_long, F_DREG),
							bt_long, typ2, F_DREG);
			}
			break;
		case bt_ushort:
		case bt_short:
			switch (typ1) {
				case bt_uchar:
					ap = mk_legal(ap, F_DREG | F_VOL, 1l);
					g_code(op_and, 2, mk_immed(255l), ap);
					return mk_legal(ap, flags, 2l);
				case bt_char:
					ap = mk_legal(ap, F_DREG | F_VOL, 1l);	// F_VOL is important here!
					g_code(op_ext, 2, ap, NIL_AMODE);		// (otherwise (short)(char)my_short fails...)
					return mk_legal(ap, flags, 2l);
				case bt_short:
				case bt_ushort:
					return mk_legal(ap, flags, 2l);
				case bt_long:
				case bt_ulong:
				case bt_pointer:
					if ((ap1 = g_offset(ap, 2)) == 0)
						ap1 = mk_legal(ap, F_DREG, 4l);
					return mk_legal(ap1, flags, 2l);
				case bt_float:
#ifdef DOUBLE
				case bt_double:
#endif
					return g_cast(g_cast(ap, bt_double, bt_long, F_DREG),
							bt_long, typ2, F_DREG);
			}
			break;
		case bt_long:
		case bt_ulong:
		case bt_pointer:
			switch (typ1) {
				case bt_uchar:
					ap = mk_legal(ap, F_DREG | F_VOL, 1l);
					g_code(op_and, 4, mk_immed(255l), ap);
					return mk_legal(ap, flags, 4l);
				case bt_char:
					ap = mk_legal(ap, F_DREG | F_VOL, 1l);	// F_VOL is important here!
					g_code(op_ext, 2, ap, NIL_AMODE);		// (otherwise (short)(char)my_short fails...)
					g_code(op_ext, 4, ap, NIL_AMODE);
					return mk_legal(ap, flags, 4l);
				case bt_ushort:
					ap = mk_legal(ap, F_DREG | F_VOL, 2l);
					g_code(op_and, 4, mk_immed(65535l), ap);
					return mk_legal(ap, flags, 4l);
				case bt_short:
					f = flags & (F_DREG | F_AREG);
					if (f == 0) f = F_DREG | F_AREG;
					ap = mk_legal(ap, f | F_VOL, 2l);	// F_VOL is important here!
					if (ap->mode == am_dreg)			// (otherwise (short)(char)my_short fails...)
						g_code(op_ext, 4, ap, NIL_AMODE);
					return mk_legal(ap, flags, 4l);
				case bt_long:
				case bt_ulong:
				case bt_pointer:
					return mk_legal(ap, flags, 4l);
#ifndef NOFLOAT
				case bt_float:
#ifdef DOUBLE
				case bt_double:
#endif
					/* library call */
#ifndef BCDFLT
					freeop(ap);
					temp_inv();
					g_code(op_move, 4, ap, push_am);
					if (typ2 == bt_long)
						call_library(str(ffpftol));
					else
						call_library(str(ffpftou));
					return func_result(flags, 4l);
#else
					fatal(
							"__floatsibf");	// !!!STUDY ME!!!
#endif
#endif
			}
			break;
#ifndef NOFLOAT
		case bt_float:
#ifdef DOUBLE
		case bt_double:
#endif
			switch (typ1) {
				case bt_char:
				case bt_uchar:
				case bt_short:
				case bt_ushort:
					ap = g_cast(ap, typ1, bt_long, F_ALL);
				case bt_long:
				case bt_ulong:
				case bt_pointer:
					/* library call */
#ifndef BCDFLT
					freeop(ap);
					temp_inv();
					g_code(op_move, 4, ap, push_am);
					if (typ1 == bt_ulong || typ1 == bt_pointer)
						call_library(str(ffputof));
					else
						call_library(str(ffpltof));
					return func_result(flags, 4l);
#else
					fatal(
							"__fixbfsi");	// !!!STUDY ME!!!
#endif
				case bt_float:
#ifdef DOUBLE
				case bt_double:
#endif
					return mk_legal(ap, flags, (long)float_size);
			}
#endif
			break;
	}
	ierr(G_CAST,1);
	/* NOTREACHED */
	return 0; // make the compiler happy
}


struct amode *g_offset(struct amode *ap, int off) {
/*
 * return true, if ap can be switched to address a location with a short
 * offset. typical application: cast long -> short: 8(a6) --> 10(a6) offset
 * is a small number (1,2 or 3)
 */
				switch (ap->mode) {
						case am_ind:
				ap = copy_addr(ap);
				ap->mode = am_indx;
				ap->offset = mk_icon((long) off);
				return ap;
						case am_indx:
				if (ap->offset->nodetype == en_icon &&
								off + ap->offset->v.i <= 32767) {
								ap = copy_addr(ap);
								ap->offset->v.i += off;
								return ap;
				}
				break;
						case am_indx2:
						case am_indx3:
				if (ap->offset->nodetype == en_icon &&
								off + ap->offset->v.i <= 127) {
								ap = copy_addr(ap);
								ap->offset->v.i += off;
								return ap;
				}
				break;
						case am_direct:
				ap = copy_addr(ap);
				ap->offset = mk_node(en_add, ap->offset,
																 mk_icon((long) off));
				return ap;
				}
				/* special value indicating that it must be done by hand */
				return 0;
}

struct amode *g_xmul(struct enode *node, int flags, enum(e_op) op) {
/*
 * performs a mixed-mode multiplication
 */
				struct amode	 *ap1, *ap2;

/*				if (lineid==139)
					bkpt();*/
				ap1 = g_expr(node->v.p[1], F_DREG | F_VOL);
				ap2 = g_expr(node->v.p[0], F_ALL & ~F_AREG);
				validate(ap1);

				g_code(op, 0, ap2, ap1);
				freeop(ap2);
				return mk_legal(ap1, flags, node->esize);
}

#ifndef __HAVE_STACK_IMAGE
#define __HAVE_STACK_IMAGE
typedef struct _stackimg {
	int next_data,next_addr;
#ifndef INFINITE_REGISTERS
	int reg_alloc_ptr,reg_stack_ptr;
	char dreg_in_use[MAX_DATA+1];
	char areg_in_use[MAX_ADDR+1];
	struct reg_struct reg_stack[MAX_REG_STACK+1],reg_alloc[MAX_REG_STACK+1];
	int act_scratch;
#endif
} STACK_IMAGE;
#endif

struct amode *g_compound(struct snode *st, int flags) {
	STACK_IMAGE img;
	struct amode *ap1,*ap2;
	int old=need_res;
	need_res=(~flags)&F_NOVALUE;
	temp_inv();		/* we are forced to do so :'( */
	usestack(&img);
	genstmt(st);
	need_res=old;
	if (!(ap1=lastexpr_am)) {
		if (!(flags&F_NOVALUE))
			err_force_line=st->line, uerrc("no value returned in compound expression");
	}
	if (flags&F_NOVALUE) {
		freeop(lastexpr_am);
		return NIL_AMODE;
	}
	freestack(&img);
	/* always one of F_DREG or F_AREG is set */
	if ((flags&(F_AREG|F_DEREF))==(F_AREG|F_DEREF) || !(flags&F_DREG))
		ap2=temp_addr();
	else ap2=temp_data();
	g_code(op_move, 4, ap1, ap2);
	return ap2;	/* we needn't call mk_legal :) */
}

struct amode *g_expr(struct enode *node, int flags) {
/*
 * general expression evaluation. returns the addressing mode of the result.
 *
 * notice how most of the code actually lies in other functions: this is to
 * reduce the stack footprint, which is necessary because calls to g_expr may
 * be deeply nested
 */
	struct amode	 *ap1, *ap2;
	unsigned int			lab0
#ifndef ALTERNATE_HOOK
		, lab1
#endif
		;
	long size;
	enum(e_bt)						type;
	if (node == 0)
		ierr(G_EXPR,1);
/*	if (node==0x7e1b50)
		bkpt();*/
	if (tst_const(node)) {
#ifndef NOBCDFLT
		if (node->nodetype==en_fcon) {
			extern int glblabel;
			int lab = nxtglabel();
			int i;
			dseg();
			put_align(2);
			put_label(lab);
			genfloat(node->v.f);
			//char s[2+BCDLEN];
			//int i;
			//s[0] = (node->v.f.exponent>>8)&255;
			//s[1] = (node->v.f.exponent>>0)&255;
			//for (i=0;i<BCDLEN;i++)
			//	s[i+2] = node->v.f.mantissa[i];
			//lab = stringlit(s,sizeof(s)-1);

			node = mk_node(en_labcon, NIL_ENODE, NIL_ENODE);
			node->v.enlab = lab;
			node->etype = bt_pointer;
			node->esize = 4;
		}
#endif
		ap1 = (struct amode *) xalloc((int) sizeof(struct amode), AMODE+G_EXPR);
		ap1->mode = am_immed;
		ap1->offset = node;
		return mk_legal(ap1, flags, node->esize);
	}
	type = node->etype;
	size = node->esize;
	switch (node->nodetype) {
		case en_autocon:
			ap1 = temp_addr();
#ifdef BIGSTACK
			if (node->v.i >= -32768 && node->v.i <= 32767) {
#endif
				ap2 = (struct amode *) xalloc((int) sizeof(struct amode),
						AMODE+G_EXPR);
				ap2->mode = am_indx;
				ap2->preg = FRAMEPTR - AREGBASE;		/* frame pointer */
				ap2->offset = node; /* use as constant node */
				g_code(op_lea, 0, ap2, ap1);
#ifdef BIGSTACK
			} else {
				g_code(op_move, 4, mk_immed((long) node->v.p[0]->v.i), ap1);
				g_code(op_add, 4, mk_reg(FRAMEPTR), ap1);
				ap1 = copy_addr(ap1);
				ap1->mode = am_ind;
			}
#endif
			return mk_legal(ap1, flags, size);
		case en_ref:
			/*
			 * g_deref uses flags and size only to test F_USES
			 */
			ap1 = g_deref(node->v.p[0], type, flags, node->esize);
			if (bt_aggregate(type))
				return mk_legal(ap1, flags, 4l);
			else
				return mk_legal(ap1, flags, size);
		case en_fieldref:
			return g_fderef(node, NULL, flags);
		case en_tempref:
			ap1 = (struct amode *) xalloc((int) sizeof(struct amode), AMODE+G_EXPR);
			if (node->v.i < AREGBASE) {
				ap1->mode = am_dreg;
				ap1->preg = (reg_t)node->v.i;
			} else {
				ap1->mode = am_areg;
				ap1->preg = (reg_t)(node->v.i - AREGBASE);
			}
			return mk_legal(ap1, flags, size);
		case en_uminus:
			return g_unary(node, flags, op_neg);
		case en_compl:
			return g_unary(node, flags, op_not);
		case en_add:
#ifdef VCG
			return g_commute(g_addsub, node, flags, op_add, symmetric);
#else
			return g_addsub(node, flags, op_add);
#endif
		case en_sub:
			//#ifdef VCG
			//	return g_commute(g_addsub, node, flags, op_sub, antisymmetric);
			//#else
				return g_addsub(node, flags, op_sub);
			//#endif
		case en_and:
#ifdef BITWISE_REDUCE
			bitwise_optimize(node, -1L);
#endif
#ifdef VCG
			return g_commute(g_ybin, node, flags, op_and, symmetric);
#else
			return g_ybin(node, flags, op_and);
#endif
		case en_or:
#ifdef BITWISE_REDUCE
			bitwise_optimize(node, 0L);
#endif
#ifdef VCG
			return g_commute(g_ybin, node, flags, op_or, symmetric);
#else
			return g_ybin(node, flags, op_or);
#endif
		case en_xor:
#ifdef BITWISE_REDUCE
			bitwise_optimize(node, 0L);
#endif
#ifdef VCG
			return g_commute(g_xbin, node, flags, op_eor, symmetric);
#else
			return g_xbin(node, flags, op_eor);
#endif
		case en_mul:
			/*
			 * special optimization possible if there are patterns matching the
			 * 68000 mulu, muls instructions. ugly, but it gives a big
			 * performance increase
			 */

			if (type == bt_long || type == bt_ulong || type == bt_pointer) {
				/* TODO : (char/uchar) * (icon both short & ushort) would be more
				 * efficient with muls instead of mulu (ext.w instead of and.w
				 * #255)
				 */
				if (tst_ushort(node->v.p[0]) && tst_ushort(node->v.p[1])) {
					/*if (node->v.p[0]->esize>2) {*/
					node->v.p[0]->etype = bt_ushort;
					node->v.p[0]->esize = 2;
					/*}
					  if (node->v.p[1]->esize>2) {*/
					node->v.p[1]->etype = bt_ushort;
					node->v.p[1]->esize = 2;
					/*}*/
					return g_xmul(node, flags, op_mulu);
				} else if (tst_short(node->v.p[0]) && tst_short(node->v.p[1])) {
					/*if (node->v.p[0]->esize>2) {*/
					node->v.p[0]->etype = bt_short;
					node->v.p[0]->esize = 2;
					/*}
					  if (node->v.p[1]->esize>2) {*/
					node->v.p[1]->etype = bt_short;
					node->v.p[1]->esize = 2;
					/*}*/
					return g_xmul(node, flags, op_muls);
				}
			}
			return g_mul(node, flags);
		case en_div:
			return g_div(node, flags);
		case en_mod:
			return g_mod(node, flags);
		case en_lsh:
			return g_shift(node, flags, op_lsl);
		case en_rsh:
			if (type==bt_ulong || type==bt_ushort || type==bt_uchar)
				return g_shift(node, flags, op_lsr);
			else
				return g_shift(node, flags, op_asr);
		case en_asadd:
			return g_asadd(node, flags, op_add);
		case en_assub:
			return g_asadd(node, flags, op_sub);
		case en_asand:
			return g_aslogic(node, flags, op_and);
		case en_asor:
			return g_aslogic(node, flags, op_or);
		case en_aslsh:
			return g_asshift(node, flags, op_lsl);
		case en_asrsh:
			if (type==bt_ulong || type==bt_ushort || type==bt_uchar)
				return g_asshift(node, flags, op_lsr);
			else
				return g_asshift(node, flags, op_asr);
		case en_asmul:
			return g_asmul(node, flags);
		case en_asdiv:
			return g_asdiv(node, flags);
		case en_asmod:
			return g_asmod(node, flags);
		case en_asxor:
			return g_asxor(node, flags);
		case en_assign:
			return g_assign(node, flags);
		case en_ainc:
			return g_aincdec(node, flags, op_add);
		case en_adec:
			return g_aincdec(node, flags, op_sub);
		case en_land:
		case en_lor:
		case en_eq:
		case en_ne:
		case en_lt:
		case en_le:
		case en_gt:
		case en_ge:
		case en_not:
#ifndef ALTERNATE_HOOK
			lab0 = nxtlabel();
			lab1 = nxtlabel();
			falsejp(node, lab0);
			ap1 = temp_data();
			g_code(op_moveq, 0, mk_immed(1l), ap1);
			g_code(op_bra, 0, mk_label(lab1), NIL_AMODE);
			g_label(lab0);
			g_code(op_moveq, 0, mk_immed(0l), ap1);
			g_label(lab1);
#else
			lab0 = nxtlabel();
			ap1 = temp_data();
			g_code(op_moveq, 0, mk_immed(0l), ap1);
			falsejp(node, lab0);
			g_code(op_moveq, 0, mk_immed(1l), ap1);
			g_label(lab0);
#endif
			return mk_legal(ap1, flags, size);
		case en_cond:
			return g_hook(node, flags);
		case en_void:
			freeop(g_expr(node->v.p[0], F_ALL | F_SRCOP | F_NOVALUE));
			return g_expr(node->v.p[1], flags);
		case en_fcall:
			return g_fcall(node, flags);
		case en_alloca:
			return mk_legal(g_alloca(node), flags, 4);
		case en_cast:
			/*
			 * On the 68000, suppress all casts between any of
			 * long, unsigned long, pointer
			 */
			if (type == bt_pointer || type == bt_long || type == bt_ulong) {
				type = node->v.p[0]->etype;
				if (type == bt_pointer || type == bt_long || type == bt_ulong)
					return g_expr(node->v.p[0], flags);
			}
			/*
			 * The cast really results in some work
			 */
#ifdef G_CAST2
			return g_cast2(node->v.p[0], node->etype, flags);
#else
			return g_cast(g_expr(node->v.p[0], F_ALL | F_SRCOP | F_GCAST),
					node->v.p[0]->etype,
					node->etype, flags);
#endif
		case en_deref:
			/*
			 * The cases where this node occurs are handled automatically:
			 * g_assign and g_fcall return a pointer to a structure rather than a
			 * structure.
			 */
			return g_expr(node->v.p[0], flags);
		case en_compound:
			return g_compound(node->v.st, flags);
		default:
			uerr(ERR_OTH,"debug: node=$%lx, nodetype=%d, etype=%d, esize=%d",node,node->nodetype,node->etype,node->esize);
			ierr(G_EXPR,2);
			/* NOTREACHED */
			return 0; // make the compiler happy
	}
}

extern struct enode   *regexp[REGEXP_SIZE];
int tst_ushort(struct enode *node) {
/*
 * tests if node is a integer constant falling in the range of uns. short or
 * if node is cast from uns. short, uns. char or char.
 */
				enum(e_bt)						type;

				if (node->nodetype == en_icon && 0 <= node->v.i && node->v.i <= 65535)
				return 1;

				if (node->nodetype == en_tempref)	/* because it could be something
														like a constant in a register */
					return tst_ushort(regexp[reg_t_to_regexp(node->v.i)]);
				if (node->nodetype == en_cast) {
				type = node->v.p[0]->etype;
				if (type == bt_ushort || type == bt_uchar || type == bt_char)
								return 1;
				}
				return 0;
}

int tst_short(struct enode *node) {
/*
 * tests if node is a integer constant falling in the range of short or if
 * node is cast from signed or unsigned short.
 */
				enum(e_bt)						type;

				if (node->nodetype == en_icon && -32768 <= node->v.i && node->v.i <= 32767)
				return 1;

				if (node->nodetype == en_tempref)	/* because it could be something
														like a constant in a register */
					return tst_short(regexp[reg_t_to_regexp(node->v.i)]);
				if (node->nodetype == en_cast) {
				type = node->v.p[0]->etype;
				if (type == bt_short || type == bt_ushort
					|| type == bt_char || type == bt_uchar)
								return 1;
				}
				return 0;
}

int tst_const(struct enode *node) {
/*
 * tests if it is a constant node, that means either en_icon, en_nacon or
 * en_labcon, or sums or differences of such nodes
 */
				enum(e_node) 			typ1 = node->nodetype;
				enum(e_node) 			typ2;
				if (typ1 == en_icon || typ1 == en_nacon || typ1 == en_labcon
				|| typ1 == en_fcon)
				return 1;

				if (typ1 == en_add || typ1 == en_sub) {
				typ1 = node->v.p[0]->nodetype;
				typ2 = node->v.p[1]->nodetype;
				if (((typ1 == en_nacon || typ1 == en_labcon) && typ2 == en_icon) ||
								((typ2 == en_nacon || typ2 == en_labcon) && typ1 == en_icon))
								return 1;
				}
				return 0;
}


static int g_compare(struct enode *node) {
/*
 * generate code to do a comparison of the two operands of node. returns 1 if
 * it was an unsigned comparison
 */
	struct amode	 *ap1, *ap2, *ap3;
#ifndef NOFLOAT
	long											i;
#endif
	switch (node->v.p[0]->etype) {
		case bt_uchar:
		case bt_char:
		case bt_ushort:
		case bt_short:
		case bt_pointer:
		case bt_long:
		case bt_ulong:
			ap2 = g_expr(node->v.p[1], F_ALL | F_SRCOP);
			/* We want to handle the special case 'tst.w pcrel_variable' smoothly,
			 * which is why we set F_SRCOP in the latter
			 * We will unset it later on. */
			if (ap2->mode == am_immed)
				ap1 = g_expr(node->v.p[0], (F_ALL & ~F_IMMED) | F_SRCOP);
			else
				ap1 = g_expr(node->v.p[0], F_AREG | F_DREG);
			validate(ap2);
			/*if (ap1->mode == am_direct
			  && (ap->offset->nodetype == en_nacon
			  || ap->offset->nodetype == en_labcon)
#ifdef AS
&& !external(ap->offset->v.enlab)
#else
#ifdef PC
&& ((long)ap->offset->v.ensp>0x1000 ? internal(ap->offset->v.ensp) : 1)
#endif
#endif
)
t*/
			/*
			 * sorry, no tst.l An on the 68000, but we can move to a data
			 * register if one is free
			 * As there is no tst.l myval(pc), we can do it this way for am_direct's too.
			 */
			if ((ap1->mode == am_areg || ap1->mode==am_direct)
					&& node->v.p[1]->nodetype == en_icon
					&& node->v.p[1]->v.i == 0 && free_data()) {
				ap3 = temp_data();
				g_code(op_move, node->v.p[0]->esize, ap1, ap3);
				/* tst.l ap3 not needed */
				freeop(ap3);
			} else {
				/* the only case where the following != nop is when ap2->mode==am_immed and ap1->mode==am_direct */
				ap1=mk_legal(ap1, F_ALL, node->v.p[0]->esize);
				g_code(op_cmp, (int) node->v.p[0]->esize, ap2, ap1);
			}
			freeop(ap1);
			freeop(ap2);
			if (node->v.p[0]->etype == bt_char ||
					node->v.p[0]->etype == bt_short ||
					node->v.p[0]->etype == bt_long)
				return 0;
			return 1;
		case bt_struct:
			ap1 = g_expr(node->v.p[1], F_AREG | F_VOL);
			ap2 = g_expr(node->v.p[0], F_AREG | F_VOL);
			validate(ap1); {
				int lab=nxtlabel();
				ap1 = copy_addr(ap1);
				ap2 = copy_addr(ap2);
				ap2->mode=ap1->mode=am_ainc;
				ap3 = temp_data();
				freeop(ap3);
				g_code(op_move,2,mk_immed((node->esize-1)>>1),ap3);
				g_label(lab);
				g_code(op_cmp,2,ap1,ap2);
				g_code(op_dbne,0,ap3,mk_label(lab));
			}
			freeop(ap2);
			freeop(ap1);
			return 1;
#ifndef NOFLOAT
		case bt_float:
#ifdef DOUBLE
		case bt_double:
#endif
#ifndef BCDFLT
			if (node->v.p[1]->nodetype == en_fcon && node->v.p[1]->v.f==0) {
				node->etype = bt_long;	/* no conversion func call (raw cast) */
				node=mk_node(en_cast,node,NIL_ENODE);
				node->etype = bt_char;
				node->esize = 1;
				ap1 = g_expr(node, F_DALT);
				g_code(op_tst, (int) node->esize, ap1, NIL_AMODE);
				freeop(ap1);
			} else {
#endif
				temp_inv();
				i = push_param(node->v.p[1]);
				i += push_param(node->v.p[0]);
#ifndef BCDFLT
				call_library(str(ffpcmp));
#else
				call_library("__cmpbf2");
#endif
				g_code(op_add, 4, mk_immed((long) i), mk_reg(STACKPTR));
				return 0;
#ifndef BCDFLT
			}
#endif
#endif
	}
	ierr(G_COMPARE,1);
	/* NOTREACHED */
	return 0; // make the compiler happy
}

#ifndef NOBCDFLT
readonly struct enode __bcd_zero__value={
	en_labcon,
#ifdef DOUBLE
	bt_double,
#else
	bt_float,
#endif
	10
};
#ifdef AS
#define bcd_zero (pchsearch("__bcd_zero",PCHS_ADD), \
	__bcd_zero__value.v.enlab=label("__bcd_zero"), \
	&__bcd_zero__value)
#else
#define bcd_zero (__bcd_zero__value.v.ensp="__bcd_zero", &__bcd_zero__value)
#endif
#endif
void truejp(struct enode *node, unsigned int lab) {
/*
 * generate a jump to lab if the node passed evaluates to a true condition.
 */
	struct amode	 *ap;
	unsigned int			lab0;
	if (node == 0)
		ierr(TRUEJP,1);
	if (node->nodetype == en_icon) {
		if (node->v.i)
			g_code(op_bra, 0, mk_label(lab), NIL_AMODE);
		return;
	}
	opt_compare(node);
	switch (node->nodetype) {
		case en_eq:
			(void) g_compare(node);
			g_code(op_beq, 0, mk_label(lab), NIL_AMODE);
			break;
		case en_ne:
			(void) g_compare(node);
			g_code(op_bne, 0, mk_label(lab), NIL_AMODE);
			break;
		case en_lt:
		case en_le:
		case en_gt:
		case en_ge:
			{
				int n=(en_ge-node->nodetype)*2+op_bhs;
				g_code(g_compare(node)?n:++n, 0, mk_label(lab), NIL_AMODE);
				break;
			}
		case en_fieldref:
			{
				struct enode *ep,*ep2;
				ep=mk_node(en_ref, node->v.p[0], (struct enode *) NIL_AMODE);
				ep->esize=node->esize;
				ep->etype=node->etype;
				ep2=mk_icon(((1<<node->bit_width)-1) <<
						((node->esize<<3)-node->bit_offset-node->bit_width));
				ep2->esize=node->esize;
				ep2->etype=node->etype;
				ep=mk_node(en_and,ep,ep2);
				ep->esize=node->esize;
				ep->etype=node->etype;
				truejp(ep, lab);
				break;
			}

			/*						case en_lt:
									g_compare(node) ?
									g_code(op_blo, 0, mk_label(lab), NIL_AMODE) :
									g_code(op_blt, 0, mk_label(lab), NIL_AMODE);
									break;
									case en_le:
									g_compare(node) ?
									g_code(op_bls, 0, mk_label(lab), NIL_AMODE) :
									g_code(op_ble, 0, mk_label(lab), NIL_AMODE);
									break;
									case en_gt:
									g_compare(node) ?
									g_code(op_bhi, 0, mk_label(lab), NIL_AMODE) :
									g_code(op_bgt, 0, mk_label(lab), NIL_AMODE);
									break;
									case en_ge:
									g_compare(node) ?
									g_code(op_bhs, 0, mk_label(lab), NIL_AMODE) :
									g_code(op_bge, 0, mk_label(lab), NIL_AMODE);
									break;*/
		case en_land:
			lab0 = nxtlabel();
			falsejp(node->v.p[0], lab0);
			truejp(node->v.p[1], lab);
			g_label(lab0);
			break;
		case en_lor:
			truejp(node->v.p[0], lab);
			truejp(node->v.p[1], lab);
			break;
		case en_not:
			falsejp(node->v.p[0], lab);
			break;
#ifdef OPTIMIZED_AINCDEC_TEST
		case en_adec:
			//				struct amode *lblap = mk_label(lab);
			ap = g_expr(node->v.p[0], F_ALL);
			/*				if (ap->mode==am_dreg)
							freeop(ap),
							g_code(op_dbra, 0, ap, lblap);
							else {*/
			g_code(op_sub, (int) node->esize, mk_immed(1), ap);
			freeop(ap);
			g_code(op_bhs, 0, /*lblap*/mk_label(lab), NIL_AMODE);
			/*				}*/
			break;
#endif
		case en_and:
			/*#ifdef PC*/
#define is_powerof2(x) (((x)<<1)==((x)^((x)-1))+1)
			/*#else
#define is_powerof2(x) ({int __x=x;((__x)<<1)==((__x)^((__x)-1))+1;})
#endif*/
			if (node->v.p[1]->nodetype==en_icon) {
				unsigned long v=node->v.p[1]->v.i;
				if (v>=128/*otherwise moveq is cool enough and faster for dregs*/
						&& is_powerof2(v)) {
					ap=g_expr(node->v.p[0],F_DREG|F_MEM);
					g_bitmancode(op_btst,node->v.p[0]->esize,mk_immed(pwrof2(v)),ap);
					freeop(ap);
					g_code(op_bne, 0, mk_label(lab), NIL_AMODE);
					break;
				}
			}
			/* FALL THROUGH */
		default:
#ifndef NOFLOAT
			if (node->etype == bt_float || node->etype == bt_double) {
#ifdef DOUBLE
				long i;
				temp_inv();
				i = push_param(node);
				call_library(".fptst");	// obsolete
				/* The pop-off does not change the condition codes */
				g_code(op_add, 4, mk_immed((long) i), mk_reg(STACKPTR));
			} else {
#else
#ifndef BCDFLT
				node->etype = bt_long;	/* no conversion func call (raw cast) */
				node=mk_node(en_cast,node,NIL_ENODE);
				node->etype = bt_char;
				node->esize = 1;
			}
			{
#else
				g_compare(mk_node(en_void,node,bcd_zero));
			} else {
#endif
#endif
#else
			{
#endif
				ap = g_expr(node, F_DALT|F_SRCOP);
				if (ap->mode==am_direct && free_data()) {
					struct amode *ap2 = temp_data();
					g_code(op_move, (int) node->esize, ap, ap2);
					g_code(op_tst, (int) node->esize, ap2, NIL_AMODE);
					freeop(ap2);
				} else
					g_code(op_tst, (int) node->esize, ap, NIL_AMODE);
				freeop(ap);
			}
			g_code(op_bne, 0, mk_label(lab), NIL_AMODE);
			break;
	}
}

void falsejp(struct enode *node, unsigned int lab) {
/*
 * generate code to execute a jump to lab if the expression passed is
 * false.
 */
	struct amode	 *ap;
	unsigned int			lab0;
	if (node == 0)
		ierr(FALSEJP,1);
	if (node->nodetype == en_icon) {
		if (!node->v.i)
			g_code(op_bra, 0, mk_label(lab), NIL_AMODE);
		return;
	}
	opt_compare(node);
	switch (node->nodetype) {
		case en_eq:
			(void) g_compare(node);
			g_code(op_bne, 0, mk_label(lab), NIL_AMODE);
			break;
		case en_ne:
			(void) g_compare(node);
			g_code(op_beq, 0, mk_label(lab), NIL_AMODE);
			break;
		case en_lt:
		case en_le:
		case en_gt:
		case en_ge:
			{
				int n=(node->nodetype-en_lt)*2+op_bhs;
				g_code(g_compare(node)?n:++n, 0, mk_label(lab), NIL_AMODE);
				break;
			}
		case en_fieldref:
			{
				struct enode *ep,*ep2;
				ep=mk_node(en_ref, node->v.p[0], (struct enode *) NIL_AMODE);
				ep->esize=node->esize;
				ep->etype=node->etype;
				ep2=mk_icon(((1<<node->bit_width)-1) <<
						((node->esize<<3)-node->bit_offset-node->bit_width));
				ep2->esize=node->esize;
				ep2->etype=node->etype;
				ep=mk_node(en_and,ep,ep2);
				ep->esize=node->esize;
				ep->etype=node->etype;
				falsejp(ep, lab);
				break;
			}
			/*						case en_lt:
									g_compare(node) ?
									g_code(op_bhs, 0, mk_label(lab), NIL_AMODE) :
									g_code(op_bge, 0, mk_label(lab), NIL_AMODE);
									break;
									case en_le:
									g_compare(node) ?
									g_code(op_bhi, 0, mk_label(lab), NIL_AMODE) :
									g_code(op_bgt, 0, mk_label(lab), NIL_AMODE);
									break;
									case en_gt:
									g_compare(node) ?
									g_code(op_bls, 0, mk_label(lab), NIL_AMODE) :
									g_code(op_ble, 0, mk_label(lab), NIL_AMODE);
									break;
									case en_ge:
									g_compare(node) ?
									g_code(op_blo, 0, mk_label(lab), NIL_AMODE) :
									g_code(op_blt, 0, mk_label(lab), NIL_AMODE);
									break;*/
		case en_land:
			falsejp(node->v.p[0], lab);
			falsejp(node->v.p[1], lab);
			break;
		case en_lor:
			lab0 = nxtlabel();
			truejp(node->v.p[0], lab0);
			falsejp(node->v.p[1], lab);
			g_label(lab0);
			break;
		case en_not:
			truejp(node->v.p[0], lab);
			break;
#ifdef OPTIMIZED_AINCDEC_TEST
		case en_adec:
			ap = g_expr(node->v.p[0], F_ALL);
			g_code(op_sub, (int) node->esize, mk_immed(1), ap);
			freeop(ap);
			g_code(op_blo, 0, mk_label(lab), NIL_AMODE);
			break;
#endif
		case en_and:
			if (node->v.p[1]->nodetype==en_icon) {
				unsigned long v=node->v.p[1]->v.i;
				if (v>=128/*otherwise moveq is cool enough and faster for dregs*/
						&& is_powerof2(v)) {
					ap=g_expr(node->v.p[0],F_DREG|F_MEM);
					g_bitmancode(op_btst,node->v.p[0]->esize,mk_immed(pwrof2(v)),ap);
					freeop(ap);
					g_code(op_beq, 0, mk_label(lab), NIL_AMODE);
					break;
				}
			}
			/* FALL THROUGH */
		default:
#ifndef NOFLOAT
			if (node->etype == bt_float || node->etype == bt_double) {
#ifdef DOUBLE
				long i;
				temp_inv();
				i = push_param(node);
				call_library(".fptst");	// obsolete
				/* The pop-off does not change the condition codes */
				g_code(op_add, 4, mk_immed((long) i), mk_reg(STACKPTR));
			} else {
#else
#ifndef BCDFLT
				node->etype = bt_long;	/* no conversion func call (raw cast) */
				node=mk_node(en_cast,node,NIL_ENODE);
				node->etype = bt_char;
				node->esize = 1;
			}
			{
#else
				g_compare(mk_node(en_void,node,bcd_zero));
			} else {
#endif
#endif
#else
			{
#endif
				ap = g_expr(node, F_DALT|F_SRCOP);
				if (ap->mode==am_direct && free_data()) {
					struct amode *ap2 = temp_data();
					g_code(op_move, (int) node->esize, ap, ap2);
					g_code(op_tst, (int) node->esize, ap2, NIL_AMODE);
					freeop(ap2);
				} else
					g_code(op_tst, (int) node->esize, ap, NIL_AMODE);
				freeop(ap);
			}
			g_code(op_beq, 0, mk_label(lab), NIL_AMODE);
			break;
	}
}

void opt_compare(struct enode *node) {
	/* temprefs should be the second operand to a cmp instruction */
	enum(e_node) 			t = node->nodetype;
	if ((t == en_eq || t == en_ne || t == en_le || t == en_ge
				|| t == en_lt || t == en_gt)
			&& (node->v.p[1]->nodetype == en_tempref ||
				node->v.p[0]->nodetype == en_icon)) {

		swap_nodes(node);
		/* if you change the operands, change the comparison operator */
		switch (t) {
			case en_le:
				node->nodetype = en_ge;
				break;
			case en_ge:
				node->nodetype = en_le;
				break;
			case en_lt:
				node->nodetype = en_gt;
				break;
			case en_gt:
				node->nodetype = en_lt;
				break;
		}
	}
}
#endif /* MC680X0 */
// vim:ts=4:sw=4
