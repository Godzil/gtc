/*
 * GTools C compiler
 * =================
 * source file :
 * statement generator
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

#ifndef PC
#define checkstack() while (0)
#endif

int				nextlabel CGLOB;
/*int 			uses_structassign CGLOB;*/
TYP 		   *ret_type CGLOB;
XLST_TYPE		lc_auto CGLOB;
XLST_TYPE		max_scratch CGLOB;
#ifdef MC680X0
unsigned int	save_mask CGLOB;
#endif
xstatic unsigned int breaklab CGLOB;
xstatic unsigned int contlab CGLOB;
xstatic unsigned int retlab CGLOB;

void genstmt(struct snode *stmt);

struct amode *mk_reg(int r) {
/*
 * make an address reference to a register.
 */
	struct amode   *ap;
	ap = (struct amode *) xalloc((int) sizeof(struct amode), AMODE+MK_REG);
#ifdef MC680X0
	if (r < AREGBASE) {
		ap->mode = am_dreg;
		ap->preg = r;
	} else {
		ap->mode = am_areg;
		ap->preg = r - AREGBASE;
	}
#endif
#ifdef INTEL_386
	ap->mode = am_reg;
	ap->preg = r;
#endif
	return ap;
}


#ifdef MC680X0
struct amode *mk_rmask(unsigned int mask) {
/*
 * generate the mask address structure.
 */
	struct amode   *ap;
	ap = (struct amode *) xalloc((int) sizeof(struct amode), AMODE+MK_MASK);
	ap->mode = am_mask1;
	ap->offset = mk_icon((long) mask);
	return ap;
}

struct amode *mk_smask(unsigned int mask) {
/*
 * generate the mask address structure.
 */
	struct amode   *ap;
	ap = (struct amode *) xalloc((int) sizeof(struct amode), AMODE+MK_MASK);
	ap->mode = am_mask2;
	ap->offset = mk_icon((long) mask);
	return ap;
}
#endif

struct amode *mk_strlab(char *s) {
/*
 * generate a direct reference to a string label.
 */
	struct amode   *ap;
	ap = (struct amode *) xalloc((int) sizeof(struct amode), AMODE+MK_STRLAB);
	ap->mode = am_direct;
	ap->offset = mk_node(en_nacon, NIL_ENODE, NIL_ENODE);
	ap->offset->v.ensp = s;
#ifdef AS
	ap->offset->v.enlab = label(s);
#endif
	return ap;
}

static void genwhile(struct snode *stmt) {
/*
 * generate code to evaluate a while statement.
 */
	unsigned int	lab1, lab2;
	initstack();				/* initialize temp registers */
	lab1 = contlab; 			/* save old continue label */
	lab2 = breaklab;			/* save old break label */
	contlab = nxtlabel();		/* new continue label */
#ifdef ICODE
	if (icode_option)
		fprintf(icode, "\tL%u:\n", contlab);
#endif
	g_label(contlab);
	if (stmt->s1 != 0) {		/* has block */
		breaklab = nxtlabel();
		initstack();
#ifdef ICODE
		if (icode_option) {
			fprintf(icode, "*falsejp L%u\n", breaklab);
			g_icode(stmt->exp);
		}
#endif
		falsejp(stmt->exp, breaklab);
		checkstack();
		genstmt(stmt->s1);
		g_code(op_bra, 0, mk_label(contlab), NIL_AMODE);
		g_label(breaklab);
#ifdef ICODE
		if (icode_option) {
			fprintf(icode, "\tbranch_unconditional\tL%u\n", contlab);
			fprintf(icode, "\tL%u:\n", breaklab);
		}
#endif
		breaklab = lab2;		/* restore old break label */
	} else {					/* no loop code */
		initstack();
#ifdef ICODE
		if (icode_option) {
			fprintf(icode, "*truejp L%u\n", contlab);
			g_icode(stmt->exp);
		}
#endif
		truejp(stmt->exp, contlab);
		checkstack();
	}
	contlab = lab1; 			/* restore old continue label */
}

struct amode *g_deref();
static void genloop(struct snode *stmt) {
/*
 * generate code to evaluate a loop statement.
 */
	unsigned int	lab1, lab2;
	struct amode *counter,*ap2;
	struct enode *node;
	initstack();				/* initialize temp registers */
	lab1 = contlab; 			/* save old continue label */
	lab2 = breaklab;			/* save old break label */
	node=stmt->exp->v.p[0];
	counter = (struct amode *) xalloc((int) sizeof(struct amode), AMODE+GENLOOP);
	switch (node->nodetype) {
		case en_autocon:
			counter->mode = am_indx;
			counter->preg = FRAMEPTR - AREGBASE;		/* frame pointer */
			counter->offset = node; /* use as constant node */
			break;
		case en_tempref:
			if (node->v.i < AREGBASE) {
				counter->mode = am_dreg;
				counter->preg = (reg_t)node->v.i;
			} else {
				fatal("LOOP can't use aregs");
/*				counter->mode = am_areg;
				counter->preg = (reg_t)(node->v.i - AREGBASE);*/
			}
			break;
		case en_nacon:
		case en_labcon:
			counter->mode = am_direct;
			counter->offset = node;
			break;
		case en_ref:
			counter=g_deref(node->v.p[0],node->etype,F_MEM|F_USES,node->esize);
			break;
		default:
			ierr(GENLOOP,1);
	}
//	ap2 = g_expr(stmt->exp, F_ALL);
	ap2 = g_expr(stmt->exp->v.p[1], F_ALL | F_SRCOP);
	g_code(op_move, 2, ap2, counter);
	freeop(ap2);
	if (counter->mode==am_dreg && counter->preg<=MAX_DATA)
		ierr(GENLOOP,2);
	contlab = nxtlabel();		/* new continue label */
#ifdef ICODE
	if (icode_option)
		fprintf(icode, "\tL%u:\n", contlab);
#endif
	g_label(contlab);
	breaklab = nxtlabel();
	if (stmt->s1 != 0)		/* has block */
		genstmt(stmt->s1);
	if (stmt->v2.e != 0) {		/* has 'until' condition */
#ifdef ICODE
	if (icode_option) {
		fprintf(icode, "*truejp L%u\n", breaklab);
		g_icode(stmt->v1.e);
	}
#endif
		truejp(stmt->v2.e, breaklab);
	}
	if (counter->mode==am_dreg) g_code(op_dbra, 0, counter, mk_label(contlab));
	else {
		struct amode *ap1;
		ap1 = (struct amode *) xalloc((int) sizeof(struct amode), AMODE+GENLOOP);
		ap1->mode = am_immed;
		ap1->offset = mk_icon(1L);
		g_code(op_subq, 2, ap1, counter);
		g_code(op_bhs, 0, mk_label(contlab), NIL_AMODE);
	}
	g_label(breaklab);
#ifdef ICODE
	if (icode_option) {
		fprintf(icode, "\tbranch_unconditional\tL%u\n", contlab);
		fprintf(icode, "\tL%u:\n", breaklab);
	}
#endif
	breaklab = lab2;		/* restore old break label */
	contlab = lab1; 			/* restore old continue label */
}

#ifndef ASM
static void genasm(struct snode *stmt) {
	g_code(_op_asm, 0, (struct amode *)stmt->v1.i, NIL_AMODE);
}
#else
#ifdef VCG
extern int vcg_lvl;
extern int vcg_aborted[];
#endif
static void genasm(struct snode *stmt) {
	struct ocode *ip=(struct ocode *)stmt->v1.i;
#ifdef VCG
	if (vcg_lvl!=VCG_MAX)
		vcg_aborted[vcg_lvl]++;
	else
#endif
		while (ip) {
			add_peep(ip);
			ip=ip->fwd;
		}
}
#endif

static void g_for(struct snode *stmt) {
/*
 * generate code to evaluate a for loop
 */
	unsigned int	old_break, old_cont, exit_label, loop_label;
	old_break = breaklab;
	old_cont = contlab;
	loop_label = nxtlabel();
	exit_label = nxtlabel();
	if (stmt->v2.e != 0)
		contlab = nxtlabel();
	else
		contlab = loop_label;
#ifdef ICODE
	if (icode_option) {
		if (stmt->exp != 0)
			g_icode(stmt->exp);
		fprintf(icode, "\tL%u:\n", loop_label);
	}
#endif
	initstack();
	if (stmt->exp != 0) {
		(void) g_expr(stmt->exp, F_ALL | F_SRCOP | F_NOVALUE);
		checkstack();
	}
	g_label(loop_label);
	initstack();
	if (stmt->v1.e != 0) {
#ifdef ICODE
		if (icode_option) {
			fprintf(icode, "*falsejp L%u\n", exit_label);
			g_icode(stmt->v1.e);
		}
#endif
		falsejp(stmt->v1.e, exit_label);
		checkstack();
	}
	if (stmt->s1 != 0) {
		breaklab = exit_label;
		genstmt(stmt->s1);
	}
#ifdef ICODE
	if (icode_option) {
		if (stmt->v2.e != 0) {
			fprintf(icode, "\tL%u:\n", contlab);
			g_icode(stmt->v2.e);
		}
		fprintf(icode, "\tbranch_unconditional\tL%u\n", loop_label);
	}
#endif
	initstack();
	if (stmt->v2.e != 0) {
		g_label(contlab);
		(void) g_expr(stmt->v2.e, F_ALL | F_SRCOP | F_NOVALUE);
		checkstack();
	}
	g_code(op_bra, 0, mk_label(loop_label), NIL_AMODE);
	breaklab = old_break;
	contlab = old_cont;
	g_label(exit_label);
#ifdef ICODE
	if (icode_option)
		fprintf(icode, "\tL%u:\n", exit_label);
#endif
}

static void genif(struct snode *stmt) {
/*
 * generate code to evaluate an if statement.
 */
	unsigned int	lab1, lab2, oldbreak;
	lab1 = nxtlabel(); 		/* else label */
	lab2 = nxtlabel(); 		/* exit label */
	oldbreak = breaklab;		/* save break label */
#ifdef ICODE
	if (icode_option) {
		fprintf(icode, "*falsejp L%u\n", lab1);
		g_icode(stmt->exp);
	}
#endif
	initstack();				/* clear temps */
	falsejp(stmt->exp, lab1);
	checkstack();
	if (stmt->s1 != 0 && stmt->s1->next != 0) {
		if (stmt->v1.s != 0)
			breaklab = lab2;
		else
			breaklab = lab1;
	}
	genstmt(stmt->s1);
	if (stmt->v1.s != 0) {		/* else part exists */
		g_code(op_bra, 0, mk_label(lab2), NIL_AMODE);
		g_label(lab1);
#ifdef ICODE
		if (icode_option) {
			fprintf(icode, "\tbranch_unconditional\tL%u\n", lab2);
			fprintf(icode, "\tL%u:\n", lab1);
		}
#endif
		if (stmt->v1.s == 0 || stmt->v1.s->next == 0)
			breaklab = oldbreak;
		else
			breaklab = lab2;
		genstmt(stmt->v1.s);
		g_label(lab2);
#ifdef ICODE
		if (icode_option)
			fprintf(icode, "\tL%u:\n", lab2);
#endif
	} else {					/* no else code */
		g_label(lab1);
#ifdef ICODE
		if (icode_option)
			fprintf(icode, "\tL%u:\n", lab1);
#endif
	}
	breaklab = oldbreak;
}

static void gendo(struct snode *stmt) {
/*
 * generate code for a do - while loop.
 */
	unsigned int	oldcont, oldbreak, dolab;
	oldcont = contlab;
	oldbreak = breaklab;
	dolab = nxtlabel();
	contlab = nxtlabel();
	breaklab = nxtlabel();
	g_label(dolab);
#ifdef ICODE
	if (icode_option)
		fprintf(icode, "\tL%u:\n", dolab);
#endif
	genstmt(stmt->s1);			/* generate body */
	g_label(contlab);
#ifdef ICODE
	if (icode_option) {
		fprintf(icode, "\tL%u:\n", contlab);
		fprintf(icode, "*truejp L%u\n", dolab);
		g_icode(stmt->exp);
		fprintf(icode, "\tL%u:\n", breaklab);
	}
#endif
	initstack();
	truejp(stmt->exp, dolab);
	checkstack();
	g_label(breaklab);
	breaklab = oldbreak;
	contlab = oldcont;
}

void call_library(char *lib_name) {
/*
 * generate a call to a library routine.
 * it is assumed that lib_name won't be clobbered
 */
	struct sym	   *sp;
	sp = gsearch(lib_name, -1);
	if (sp == 0) {
		pchsearch(lib_name,PCHS_ADD);
		sp = gsearch(lib_name, -1);
		if (sp == 0) {
			++global_flag;
			sp = (struct sym *) xalloc((int) sizeof(struct sym), _SYM);
	#ifdef NO_CALLOC
			sp->tp = 0;
	#endif
			sp->name = strsave(lib_name);
			sp->storage_class = sc_external;
			append(&sp, &gsyms);
			--global_flag;
		}
	}
	sp->used = 1;
#ifdef MC680X0
	g_code(op_jsr, 0, mk_strlab(sp->name), NIL_AMODE);
#endif
#ifdef INTEL_386
	g_code(op_call, 0, mk_strlab(sp->name), NIL_AMODE);
#endif
}

static void genswitch(struct snode *stmt) {
/*
 * generate a linear search switch statement.
 */
	unsigned int	curlab;
	unsigned int	deflab;
	unsigned int	tablab;
	long			i;
	struct snode   *defcase, *s;
	struct amode   *ap, *ap1;
#ifndef USE_WORDS_IN_SWITCH
	struct amode   *ap2;
#endif
	struct enode   *ep;
	long			size;
	enum(e_bt)		type;
	long			min_caselabel, max_caselabel, nxt_caselabel;
	int 			number_of_cases;
	defcase = 0;
#ifdef ICODE
	if (icode_option) {
		fprintf(icode, "$switchexp\n");
		g_icode(stmt->exp);
	}
#endif
	initstack();
#ifdef MC680X0
	ap = g_expr(stmt->exp, F_DREG | F_VOL);
#endif
#ifdef INTEL_386
	ap = g_expr(stmt->exp, F_REG);
#endif
	size = stmt->exp->esize;
	type = stmt->exp->etype;
	stmt = stmt->s1;
	/*
	 * analyze the switch statement
	 */
	s = stmt;
	if (s != 0 && s->stype == st_default) {
		defcase = s;
		s = s->s1;
	}
	/* s points to the first case label */
	number_of_cases = 1;
	if (s != 0) {
		max_caselabel = min_caselabel = s->v2.i;
		s = s->s1;
		while (s != 0) {
			if (s->stype == st_default) {
				defcase = s;
			} else {
				if (s->v2.i > max_caselabel)
					max_caselabel = s->v2.i;
				else if (s->v2.i < min_caselabel)
					min_caselabel = s->v2.i;
				number_of_cases++;
			}
			s = s->s1;
		}
	}
	if (number_of_cases > 7 &&
		(max_caselabel - min_caselabel) / number_of_cases <= 5) {
		/*
		 * we need the case label as a 32-bit item.
		 */
#ifdef MC680X0
		switch (type) {
		  case bt_char:
			g_code(op_ext, 2, ap, NIL_AMODE);
#ifndef USE_WORDS_IN_SWITCH
			/*FALLTHROUGH*/
		  case bt_short:
			g_code(op_ext, 4, ap, NIL_AMODE);
			break;
		  case bt_uchar:
			g_code(op_and, 4, mk_immed(255l), ap);
			break;
		  case bt_ushort:
			g_code(op_and, 4, mk_immed(65535l), ap);
#else
			break;
		  case bt_uchar:
			g_code(op_and, 2, mk_immed(255l), ap);
			break;
#endif
		}
#endif
#ifdef INTEL_386
		switch (type) {
		  case bt_char:
			g_code(op_movsbl, 0, ap, ap);
			break;
		  case bt_short:
			g_code(op_movswl, 0, ap, ap);
			break;
		  case bt_uchar:
			g_code(op_movzbl, 0, ap, ap);
			break;
		  case bt_ushort:
			g_code(op_movzwl, 0, ap, ap);
		}
#endif
		/*
		 * move the interval
		 */
#ifdef MC680X0
#ifndef USE_WORDS_IN_SWITCH
#define switch_size 4
#else
#define switch_size 2
#endif
		if (min_caselabel != 0) {
			g_code(op_sub, switch_size, mk_immed(min_caselabel), ap);
			g_code(op_cmp, switch_size, mk_immed(max_caselabel - min_caselabel), ap);
		} else
			g_code(op_cmp, (int) size,
						   mk_immed(max_caselabel-min_caselabel), ap);
#endif
#ifdef INTEL_386
		if (min_caselabel != 0) {
			g_code (op_sub, 4, mk_immed(min_caselabel), ap);
			g_code(op_cmp, 4, mk_immed(max_caselabel-min_caselabel), ap);
		} else
			g_code(op_cmp, (int) size,
						   mk_immed(max_caselabel-min_caselabel), ap);
#endif
		if (defcase == 0)
			deflab = breaklab;
		else {
			deflab = nxtlabel();
			defcase->v2.i = deflab;
		}
#ifdef MC680X0
		g_code(op_bhi, 0, mk_label(deflab), NIL_AMODE);
#endif
#ifdef INTEL_386
		g_code(op_ja, 0, mk_label(deflab), NIL_AMODE);
#endif
		tablab = nxtlabel();
#ifdef MC680X0
#ifndef USE_WORDS_IN_SWITCH
		g_code(op_add, 4, ap, ap);
		g_code(op_add, 4, ap, ap);
#else
		g_code(op_add, 2, ap, ap);
#endif
		ap1 = temp_addr();
		g_code(op_lea, 0, mk_label(tablab), ap1);
#ifndef USE_WORDS_IN_SWITCH
		g_code(op_add, 4, ap, ap1);
#endif
		freeop(ap1);
		ap1 = copy_addr(ap1);
#ifndef USE_WORDS_IN_SWITCH
		ap1->mode = am_ind;
		ap2 = temp_addr();
		g_code(op_move, 4, ap1, ap2);
		ap2 = copy_addr(ap2);
		ap2->mode = am_ind;
		g_code(op_jmp, 0, ap2, NIL_AMODE);
		freeop(ap2);
#else
		ap1->mode=am_indx2;
		ap1->sreg=ap->preg;
		ap1->slen=2;
		ap1->offset=mk_icon(0l);
		g_code(op_move, 2, ap1, ap);
		g_code(op_jmp, 0, ap1, NIL_AMODE);
#endif
		freeop(ap);
#endif
#ifdef INTEL_386
		g_code(op_shl, 4, mk_immed(2l), ap);
		ep = mk_node(en_labcon, NIL_ENODE, NIL_ENODE);
		ep->v.enlab = tablab;
		ap1 = copy_addr(ap);
		ap1->mode = am_indx;
		ap1->offset = ep;
		g_code(op_mov, 4, ap1, ap);
		ap1=copy_addr(ap);
		ap1->mode = am_star;
/*
 * DO NOT USE OP_BRA here....
 * op_bra is reserved for jumps to internal labels.
 * This keeps things easy in the peephole optimizer
 * While producing assembler output, op_bra and op_jmp yield
 * the same
 */
		g_code(op_jmp, 0, ap1, NIL_AMODE);
		freeop(ap);
#endif
/*
 * now, ship out the switch table
 *	we have just generated the following code (MC68000 expample)
 *
 * sub.l		#min_caselabel,d0
 * cmp.l		#max_caselabel-min_caselabel,d0
 * bhi			deflab
 * add.l		d0,d0
 * add.l		d0,d0
 * lea			table,a0
 * add.l		d0,a0
 * move.l		(a0),a0
 * jmp			(a0)
 */
/*
 * search the cases: look for min_caselabel. nxt_caselabel is set to
 * the label looked for in the next pass
 */
#ifdef ICODE
		if (icode_option)
			fprintf(icode, "default             ==> L%u\n",
					(unsigned int) deflab);
#endif
//#ifdef AS
		g_label(tablab);
//#else
//		cseg();
//		nl();
//		put_align(AL_POINTER);
//		put_label(tablab);
//#endif
		ep = mk_node(en_labcon, NIL_ENODE, NIL_ENODE);
		ep->v.enlab=deflab;
		{
#ifdef USE_WORDS_IN_SWITCH
		struct enode *ep0 = mk_node(en_labcon, NIL_ENODE, NIL_ENODE);
		struct amode *ap0;
		ep0->v.enlab=tablab;
		ap0=mk_offset(mk_node(en_sub,ep,ep0));
#endif
		while (min_caselabel <= max_caselabel) {
			nxt_caselabel = max_caselabel + 1;
			s = stmt;
			while (s != 0) {
				if (s->stype != st_default) {
					if (s->v2.i == min_caselabel) {
						ep = mk_node(en_labcon, NIL_ENODE, NIL_ENODE);
						s->v2.i = ep->v.enlab = nxtlabel();
						/* remove statment from the list */
						s->stype = st_default;
#ifndef USE_WORDS_IN_SWITCH
//#ifdef AS
						g_code(op_dc, 4, mk_offset(ep), NIL_AMODE);
						// this is because AS can't handle local references in 'genptr'
//#else
//						genptr(ep);
//#endif
#else
						ep=mk_node(en_sub,ep,ep0);
						g_code(op_dc, 2, mk_offset(ep), NIL_AMODE);
#endif
#ifdef ICODE
						if (icode_option)
							fprintf(icode, "case-value %8ld ==> L%u\n",
									min_caselabel, (unsigned int) (s->v2.i));
#endif
					} else if (nxt_caselabel > s->v2.i)
						nxt_caselabel = s->v2.i;
				}
				s = s->s1;
			}
			/* fill the holes */
			for (i = min_caselabel + 1; i < nxt_caselabel; i++) {

#ifndef USE_WORDS_IN_SWITCH
				ep = mk_node(en_labcon, NIL_ENODE, NIL_ENODE);
				ep->v.enlab = deflab;
//#ifdef AS
				g_code(op_dc, 4, mk_offset(ep), NIL_AMODE);
				// this is because AS can't handle local references in 'genptr'
//#else
//				genptr(ep);
//#endif
#else
				g_code(op_dc, 2, ap0, NIL_AMODE);
#endif
#ifdef ICODE
				if (icode_option)
					fprintf(icode, "case-value %8ld ==> L%u\n",
							i, (unsigned int) deflab);
#endif
			}
			min_caselabel = nxt_caselabel;
		}
		}
		nl();
	} else {
		/*
		 * generate sequential compare instructions this is not very
		 * intelligent, but quicker than a lookup in a (value,label) table
		 * and has no startup time, this makes it fast if there are few
		 * labels.
		 */
		while (stmt != 0) {
			curlab = nxtlabel();
			if (stmt->stype == st_default) {	/* default case ? */
				defcase = stmt;
			} else {
#ifdef MC680X0
				g_code(op_cmp, (int) size, mk_immed((long) stmt->v2.i), ap);
				g_code(op_beq, 0, mk_label(curlab), NIL_AMODE);
#endif
#ifdef INTEL_386
				g_code(op_cmp, (int) size, mk_immed((long) stmt->v2.i), ap);
				g_code(op_je, 0, mk_label(curlab), NIL_AMODE);
#endif
#ifdef ICODE
				if (icode_option)
					fprintf(icode, "*ifeq\t%ld,L%u\n", stmt->v2.i, curlab);
#endif
			}
			stmt->v2.i = curlab;
			stmt = stmt->s1;
		}
		if (defcase == 0) {
			g_code(op_bra, 0, mk_label(breaklab), NIL_AMODE);
#ifdef ICODE
			if (icode_option)
				fprintf(icode, "\tbranch_unconditional\tL%u\n", breaklab);
#endif
		} else {
			g_code(op_bra, 0, mk_label((unsigned int) defcase->v2.i),
				   NIL_AMODE);
#ifdef ICODE
			if (icode_option)
				fprintf(icode, "\tbranch_unconditional\tL%u\n",
						(unsigned int) defcase->v2.i);
#endif
		}
		freeop(ap);
	}
	checkstack();
}

static void gencase(struct snode *stmt) {
/*
 * generate the label for the case statement.
 */
	g_label((unsigned int) stmt->v2.i);
#ifdef ICODE
	if (icode_option)
		fprintf(icode, "\tL%u:\n", (unsigned int) stmt->v2.i);
#endif
}

static void genxswitch(struct snode *stmt) {
/*
 * analyze and generate best switch statement.
 */
	int 			oldbreak;
	oldbreak = breaklab;
	breaklab = nxtlabel();
	genswitch(stmt);
	genstmt(stmt->v1.s);
	g_label(breaklab);
#ifdef ICODE
	if (icode_option)
		fprintf(icode, "\tL%u:\n", breaklab);
#endif
	breaklab = oldbreak;
}

static void genreturn(struct snode *stmt) {
/*
 * generate a return statement.
 */
	struct amode   *ap;
	struct enode   *ep,*ep1;
//	long			stack_offset;
	if (stmt != 0 && stmt->exp != 0) {
#ifdef ICODE
		if (icode_option) {
			fprintf(icode, "$return_value\n");
			g_icode(stmt->exp);
		}
#endif
		initstack();
		switch (ret_type->type) {
		  case bt_struct:
		  case bt_union:
		  aggregate:
/*			uses_structassign = 1;*/
#ifdef SHORT_STRUCT_PASSING
			if (ret_type->size<=4) {
				if (stmt->exp->nodetype!=en_ref)
					ierr(GENRETURN,1);
				ap = g_expr(stmt->exp->v.p[0], F_AREG);
				ap = copy_addr(ap);
				ap->mode = am_ind;
				g_code(op_move,ret_type->size,ap,mk_reg(RESULT));
				freeop(ap);
				break;
			}
#endif
			/* assign structure */
			ep = mk_node(en_autocon, NIL_ENODE, NIL_ENODE);
			ep->v.i = 8;
			ep->esize = 4;
			ep->etype = bt_pointer;
			ep = mk_node(en_ref, ep, NIL_ENODE);
			ep->etype = bt_pointer;
			ep->esize = 4;
			ep1 = mk_node(en_ref, ep, NIL_ENODE);
			ep1->etype = ret_type->type;
			ep1->esize = ret_type->size;
			ep1 = mk_node(en_assign, ep1, stmt->exp);
			ep1->etype = ret_type->type;
			ep1->esize = ret_type->size;
			(void) g_expr(ep1, F_ALL | F_SRCOP | F_NOVALUE);
#ifdef MC680X0
			/* move pointer to A0 */
			ap = g_expr(ep, F_ALL | F_SRCOP);
			g_code(op_move, 4, ap, mk_reg(PRESULT));
			freeop(ap);
#endif
#ifdef INTEL_386
			/* move pointer to eax */
			ap = g_expr(ep, F_ALL | F_SRCOP);
			g_code(op_mov, 4, ap, mk_reg(RESULT));
			freeop(ap);
#endif
			break;
		  case bt_float:
#ifdef DOUBLE
		  case bt_double:
#endif
#ifdef INTEL_386
			/* return floating point value on top of fpu stack */
			(void) g_expr(stmt->exp, F_FPSTACK);
#ifdef FUNCS_USE_387
			if (!fpu_option)
/*
 *	.fpgetstack may pop off the top of the software stack into
 *	the hardware stack. Thus this function can be linked with
 *	code that expexts the result value in the hardware stack.
 */
			   call_library(".fpgetstack");	// obsolete
#endif
			/* DO NOT call freeop since this might pop the value off */
			break;
#endif
#ifdef MC68000
#ifndef BCDFLT
			/* fallthrough, return value in RESULT register */
#else
			goto aggregate;
#endif
#endif
		  case bt_char:
		  case bt_uchar:
		  case bt_short:
		  case bt_ushort:
		  case bt_long:
		  case bt_ulong:
#ifdef MC680X0
			/* evaluate a parameter in D0 */
			ap = g_expr(stmt->exp, (F_ALL | F_SRCOP)&~F_AREG);
			g_code(op_move, (int) stmt->exp->esize, ap, mk_reg(RESULT));
			freeop(ap);
#endif
#ifdef INTEL_386
		  case bt_pointer:
			/* evaluate a parameter in eax */
			ap = g_expr(stmt->exp, F_ALL);
			g_code(op_mov, (int) stmt->exp->esize, ap, mk_reg(RESULT));
			freeop(ap);
#endif
			break;
#ifdef MC680X0
		  case bt_pointer:
			/* evaluate a parameter in A0 */
			ap = g_expr(stmt->exp, (F_ALL | F_SRCOP)&~F_DREG);
			g_code(op_move, (int) stmt->exp->esize, ap, mk_reg(PRESULT));
			freeop(ap);
			break;
#endif
		  case bt_void:
			uwarn("return <val> in function returning void");
			break;
		  default:
			ierr(GENRETURN,1);
		}
		checkstack();
	}

/*
 * The epilogue is now postponed till the end of the function body since
 * we have to know the final value of max_scratch
 * If stmt is zero, this is the implicit return statement at the end of
 * the function
 */
	if (stmt != 0) {
		if (retlab == 0)
			retlab = nxtlabel();
		g_code(op_bra, 0, mk_label(retlab), NIL_AMODE);
#ifdef ICODE
		if (icode_option)
			fprintf(icode, "\tbranch_unconditional\tL%u\n", retlab);
#endif
	} else {
		if (retlab != 0) {
			g_label(retlab);
#ifdef ICODE
			if (icode_option)
				fprintf(icode, "\tL%u:\n", retlab);
#endif
		}
/*
 * Note that if the function has called other functions (imagine alloca!),
 * the only safe reference to the saved registers is via the framepointer,
 * not the stackpointer.
 */
#ifdef MC680X0
		if (regs_used > 0) {
#ifndef USE_LINK
			if (!uses_link)
#endif
				g_code(op_movem, 4, pop_am, mk_smask(save_mask));
#ifndef USE_LINK
			else {
				int stack_offset = lc_auto + max_scratch + 4*regs_used;
#ifdef BIGSTACK
				if (stack_offset <= 32768l) {
#endif
					ap=mk_reg(TRUE_FRAMEPTR);
					ap->mode = am_indx;
					ap->offset = mk_icon(-stack_offset);
					g_code(op_movem, 4, ap, mk_smask(save_mask));
#ifdef BIGSTACK
				} else {
					ap = mk_reg(STACKPTR);
					g_code(op_move, 4, mk_reg(TRUE_FRAMEPTR), ap);
					g_code(op_sub,	4, mk_immed(stack_offset), ap);
					g_code(op_movem, 4, pop_am, mk_smask(save_mask));
				}
#endif
			}
#endif
		}
#ifdef USE_LINK
		g_code(op_unlk, 0, mk_reg(FRAMEPTR), NIL_AMODE);
#else
		if (uses_link)
			g_code(op_unlk, 0, mk_reg(TRUE_FRAMEPTR), NIL_AMODE);
		else if (lc_auto || max_scratch)
			g_code(op_add, 4, mk_immed(lc_auto + max_scratch), mk_reg(STACKPTR));
#endif
		g_code(op_rts, 0, NIL_AMODE, NIL_AMODE);
#endif
#ifdef INTEL_386
		/*
		 * Adjust stack pointer to initial value
		 */
		if (regs_used > 0 && !is_leaf_function) {
			stack_offset = lc_auto + max_scratch + 4*regs_used;
			ap = mk_reg(EBP);
			ap->mode = am_indx;
			ap->offset = mk_icon(-stack_offset);
			g_code(op_lea, 4, ap, mk_reg(ESP));
		}
/*
 * pop clobbered register variables
 */
		if (esi_used)
			g_code(op_pop, 4, mk_reg(ESI), NIL_AMODE);
		if (edi_used)
			g_code(op_pop, 4, mk_reg(EDI), NIL_AMODE);
		if (ebx_used)
			g_code(op_pop, 4, mk_reg(EBX), NIL_AMODE);
		g_code(op_leave, 0, NIL_AMODE, NIL_AMODE);
		g_code(op_ret, 0, NIL_AMODE, NIL_AMODE);
#endif
#ifdef ICODE
		if (icode_option)
			fprintf(icode, "\t$leave\n");
#endif
	}
}

struct amode *lastexpr_am;
int need_res;

void genstmt(struct snode *stmt) {
/*
 * genstmt will generate a statement and follow the next pointer until the
 * block is generated.
 */
	while (stmt != 0) {
#ifdef DB_POSSIBLE
		lineid=stmt->line;
#endif
		switch (stmt->stype) {
		  case st_label:
			g_label((unsigned int) stmt->v2.i);
#ifdef ICODE
			if (icode_option)
				fprintf(icode, "\tL%u:\n", (unsigned int) stmt->v2.i);
#endif
			genstmt(stmt->s1);
			break;
		  case st_goto:
			g_code(op_bra, 0, mk_label((unsigned int) stmt->v2.i), NIL_AMODE);
#ifdef ICODE
			if (icode_option)
				fprintf(icode, "\tbranch_unconditional\tL%u\n",
						(unsigned int) stmt->v2.i);
#endif
			break;
		  case st_expr: {
			int flgs;
#ifdef ICODE
			if (icode_option)
				g_icode(stmt->exp);
#endif
			initstack();
/*			if ((long)stmt->exp==0x7b82cc)
				bkpt();*/
			lastexpr_am=g_expr(stmt->exp, flgs = 
				(!(stmt->next) && need_res ? F_ALL | F_SRCOP : F_ALL | F_SRCOP | F_NOVALUE));
			if (flgs & F_NOVALUE) checkstack();
			break;
		  }
		  case st_return:
			genreturn(stmt);
			break;
		  case st_if:
			genif(stmt);
			break;
		  case st_asm:
			genasm(stmt);
			break;
		  case st_loop:
			genloop(stmt);
			break;
		  case st_while:
			genwhile(stmt);
			break;
		  case st_do:
			gendo(stmt);
			break;
		  case st_for:
			g_for(stmt);
			break;
		  case st_continue:
			g_code(op_bra, 0, mk_label(contlab), NIL_AMODE);
#ifdef ICODE
			if (icode_option)
				fprintf(icode, "\tbranch_unconditional\tL%u\n", contlab);
#endif
			break;
		  case st_break:
			g_code(op_bra, 0, mk_label(breaklab), NIL_AMODE);
#ifdef ICODE
			if (icode_option)
				fprintf(icode, "\tbranch_unconditional\tL%u\n", breaklab);
#endif
			break;
		  case st_switch:
			genxswitch(stmt);
			break;
		  case st_compound:
			genstmt(stmt->s1);
			break;
		  case st_case:
		  case st_default:
			gencase(stmt);
			genstmt(stmt->v1.s);
			break;
		  default:
			ierr(GENSTMT,1);
			break;
		}
		stmt = stmt->next;
	}
}

extern SYM *func_sp;
extern SYM *parmsp[CONVENTION_MAX_DATA+1+CONVENTION_MAX_ADDR+1+1];
void genfuncbegin(void) {
#ifdef REGPARM
	int				dregs=func_sp->tp->rp_dn, aregs=func_sp->tp->rp_an;
	int				dreg=RESULT, areg=PRESULT;
#endif

#ifdef BIGSTACK
	if (lc_auto < 32768l) {
#ifdef REGPARM
#error This codes needs a little fix if REGPARM is defined...
#endif
#endif
#ifdef USE_LINK
		g_coder(op_link, 0, mk_reg(FRAMEPTR), mk_immed(-lc_auto));
#else
#ifndef REGPARM
		if (uses_link)
			g_coder(op_link, 0, mk_reg(TRUE_FRAMEPTR), mk_immed(-lc_auto));
		else if (lc_auto) g_coder(op_sub, 4, mk_immed(lc_auto), mk_reg(STACKPTR));
#else
		if (!uses_link && lc_auto!=reg_size)
			g_coder(op_sub, 4, mk_immed(lc_auto-reg_size), mk_reg(STACKPTR));
#endif
#endif
#ifdef BIGSTACK
	} else {
		g_coder(op_sub, 4, mk_immed(lc_auto), mk_reg(STACKPTR));
#ifdef USE_LINK
		if (uses_link)
			g_coder(op_link, 0, mk_reg(FRAMEPTR), mk_immed(0l));
#endif
	}
#endif
	/* now generate pushing code for reg params */
#ifdef REGPARM
  { struct amode *apl[CONVENTION_MAX_DATA+1+CONVENTION_MAX_ADDR+1];
	int aps[CONVENTION_MAX_DATA+1+CONVENTION_MAX_ADDR+1],i=0;
	SYM **spp=parmsp;
	while (*spp) {
		SYM *sp1 = *spp++;
		if (dregs || aregs) {
			apl[i]=mk_reg(((sp1->tp->type==bt_pointer && aregs) || !dregs)
				?(aregs--, areg++):(dregs--, dreg++));
			aps[i]=sp1->tp->size;
			aps[i]+=aps[i]&1;
			if (uses_link) {
				struct amode *ap = (struct amode *) xalloc((int) sizeof(struct amode),
					AMODE+G_DEREF);
				ap->mode = am_indx;
				ap->preg = FRAMEPTR-AREGBASE;
				ap->offset = mk_icon(sp1->value.i);
				g_coder(op_move, aps[i], apl[i], ap);
			}
			i++;
		} else break;
	}
	if (!uses_link) {
//		i=nparms; if (i>dregs+aregs) i=dregs+aregs;
		while (i--) g_coder(op_move, aps[i], apl[i], push_am);
	} else g_coder(op_link, 0, mk_reg(TRUE_FRAMEPTR), mk_immed(-lc_auto));
  }
#endif
}

void genfunc(struct snode *stmt) {
/*
 * generate a function body.
 */
#ifdef VERBOSE
	time_t			ltime;
#endif							/* VERBOSE */
	int line0 = lineid;

	retlab = contlab = breaklab = 0;
	max_scratch = 0;
	if (lc_auto % AL_DEFAULT != 0)
		lc_auto += AL_DEFAULT - (lc_auto % AL_DEFAULT);
#ifdef VERBOSE
	times(&tms_buf);
	ltime = tms_buf.tms_utime;
#endif							/* VERBOSE */
#ifdef SHOWSTEP
	printf("\ncse");
#endif
	opt1(stmt);
#ifdef SHOWSTEP
	printf(" ok ");
#endif
#ifdef VERBOSE
	times(&tms_buf);
	opt_time += tms_buf.tms_utime - ltime;
#endif							/* VERBOSE */
	need_res=0;
#ifdef SHOWSTEP
	printf("\ngen");
#endif
	genstmt(stmt);
	genreturn(NIL_SNODE);
#ifdef SHOWSTEP
	printf(" ok ");
#endif
/*
 *	It is desirable to postpone the generation of the
 *	stack fram until here since it it now possible for
 *	the code generation routines to allocate stack space
 *	as they want. A little hack (defining g_coder)
 *	allows us to put the following code to the BEGINNING
 *	of a function body. Note that this means that we have
 *	to generate all code in REVERSE order
 */
	lineid = line0;
	lc_auto += max_scratch;
#ifdef MC680X0
	genfuncbegin();
#endif
#ifdef INTEL_386
	if (lc_auto != 0)
		g_coder(op_sub, 4, mk_immed(lc_auto), mk_reg(STACKPTR));
	g_coder(op_mov, 4, mk_reg(STACKPTR), mk_reg(FRAMEPTR));
	g_coder(op_push, 4, mk_reg(FRAMEPTR), NIL_AMODE);
#endif
#ifdef ICODE
	if (icode_option)
		fprintf(icode, "\t$framesize was %ld\n", lc_auto);
#endif
}
// vim:ts=4:sw=4
