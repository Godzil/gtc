/*
 * GTools C compiler
 * =================
 * source file :
 * function routines
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
#include	"define.h"
_FILE(__FILE__)
#include	"c.h"
#include	"expr.h"
#include	"gen.h"
#include	"cglbdec.h"

int 			is_leaf_function CGLOB, uses_link CGLOB, pushed CGLOB;
XLST_TYPE		reg_size CGLOB;


void funcbottom(void);
void block(struct sym *sp); /* CAUTION : always requires a compound_done() after call */

/* function compilation routines */

#ifdef REGPARM
SYM *parmsp[CONVENTION_MAX_DATA+1+CONVENTION_MAX_ADDR+1+1];
#endif
void funcbody(SYM *sp, char *names[], int nparms) {
/*
 * funcbody starts with the current symbol being the begin for the local
 * block or the first symbol of the parameter declaration
 */
	long			poffset;
	int 			i, j;
	struct sym	   *sp1, *mk_int();
	int 			old_global;
	long			old_spvalue;
	XLST_TYPE	   *p;
#ifdef REGPARM
	int regs=sp->tp->rp_dn+sp->tp->rp_an;
#endif

#ifdef VERBOSE
	time_t			ltime;
#endif							/* VERBOSE */

#ifdef VERBOSE
	times(&tms_buf);
	ltime = tms_buf.tms_utime;
#endif							/* VERBOSE */
/*	uses_structassign=0;*/
	lc_auto = 0;
	max_scratch = 0;
	is_leaf_function = 1;
	uses_link = 0;
	pushed = 0;
	init_node = 0;
	old_global = global_flag;
	global_flag = 0;
	poffset = 8;				/* size of return block */
#ifdef REGPARM
	memset(parmsp,0,sizeof(parmsp));
#endif
	/*if (!strcmp(declid,"MnuSel"))
		printf("sgio");*/
	if (bt_aggregate(sp->tp->btp->type))
		poffset = 12;
	if (!start_block(0))
		dodecl(sc_parms);		/* declare parameters */
	/* undeclared parameters are int's */
	if (!lc_auto)	/* otherwise there are declared vars that aren't parameters : error */
		for (i = 0; i < nparms; ++i) {
			if (!(sp1 = search(names[i], -1, &lsyms))) {
				sp1 = mk_int(names[i]);
	//			do_warning("argument '%s' implicitly declared 'int'",names[i]);
			}
			old_spvalue = sp1->value.i;
			sp1->value.i = poffset;
			sp1->storage_class = sc_auto;
	#ifdef REGPARM
			if (regs) {
				int siz;
				if (sp1->tp->type==bt_pointer || sp1->tp->type==bt_func) {
					/*
					 * arrays and functions are never passed. They are really
					 * Pointers
					 */
					if (sp1->tp->val_flag != 0) {
						TYP *tp1 = (TYP *) xalloc((int) sizeof(TYP), _TYP+FUNCBODY);
						*tp1 = *(sp1->tp);
						sp1->tp = tp1;
						tp1->st_flag = 0;
						sp1->tp->val_flag = 0;
						sp1->tp->size = 4;
					}
				}
				siz=sp1->tp->size;
		#ifdef AS
				sp1->value.i = -(lc_auto+siz+(siz&1)/* dirty hack, but it works */);
		#else
				sp1->value.i = -(lc_auto+siz+(siz&1)/* dirty hack, but it works */);
//				if (siz&1) printf("[crp]"),getchar();
		#endif
				siz+=siz&1;
				lc_auto+=siz;
				parmsp[i]=sp1;
				regs--;
//				goto ok_param;
				poffset -= siz;	/* to undo the effect of the following statements */
			}
	#endif
			/*
			 * char, unsigned char, short, unsigned short, enum have been widened
			 * to int by the caller. This has to be un-done The same is true for
			 * float/double but float==double actually convert x[] to *x by
			 * clearing val_flag
			 * 
			 * It is shown here how to do this correctly, but if we know something
			 * about the data representation, it can be done much more
			 * effectively. Therefore, we define MC680X0 and do the cast
			 * by hand. This means that we can retrieve a char, widened to short
			 * and put at machine address n, at machine address n+1. This should
			 * work on most machines. BIGendian machines can do it like it is
			 * shown here, LOWendian machines must not adjust sp1->value.i The
			 * function castback is still needed if someone decides to have a
			 * double data type which is not equivalent to float.
			 * This approach is, of course, ugly since some
			 * assumptions on the target machine enter the front end here, but
			 * they do anyway through the initial value of poffset which is the
			 * number of bytes that separate the local block from the argument
			 * block. On a 68000 this are eight bytes since we do a link a6,...
			 * always.
			 */
			switch (sp1->tp->type) {
			  case bt_char:
			  case bt_uchar:
	#ifdef MC680X0
				if (short_option) {
					sp1->value.i += 1;
					poffset += 2;
				} else {
					sp1->value.i += 3;
					poffset += 4;
				}
	#endif
	#ifdef INTEL_386
				/* note that we only support 32-bit integers */
				poffset += 4; /* byte already right there */
	#endif
				break;
			  case bt_short:
			  case bt_ushort:
	#ifdef MC680X0
				if (short_option) {
					poffset += 2;
				} else {
					sp1->value.i += 2;
					poffset += 4;
				}
	#endif
	#ifdef INTEL_386
				poffset += 4; /* word already right there */
	#endif
				break;
			  case bt_float:
	#ifdef MC68000
				/* float is the same as double in the 68000 implementation */
				poffset += float_size;
	#endif
	#ifdef INTEL_386
				castback(poffset, &tp_double, &tp_float);
				poffset += 8;
	#endif
				break;
			  case bt_pointer:
			  case bt_func:
				poffset += 4;
				/*
				 * arrays and functions are never passed. They are really
				 * Pointers
				 */
				if (sp1->tp->val_flag != 0) {
					TYP 		   *tp1 = (TYP *) xalloc((int) sizeof(TYP), _TYP+FUNCBODY);
					*tp1 = *(sp1->tp);
					sp1->tp = tp1;
					tp1->st_flag = 0;
					sp1->tp->val_flag = 0;
					sp1->tp->size = 4;
				}
				break;
			  default:
				poffset += sp1->tp->size;
				break;
			}
//		  ok_param:
			/*
			 * The following code updates the reglst and autolst arrays
			 * old_spvalue is zero for undeclared parameters (see mk_int), so it
			 * works. We do a linear search through the reglst and autolst array,
			 * so this is inefficient. Howewer, these arrays are usually short
			 * since they only contain the function arguments at moment. In
			 * short, function argument processing need not be efficient.
			 */
			p=reglst; j=regptr; while (j--) {
				if ((XLST_TYPE)*p++ == (XLST_TYPE)old_spvalue) {
					p[-1] = sp1->value.i;
					break;
				}
			}
			p=autolst; j=autoptr; while (j--) {
				if ((XLST_TYPE)*p++ == (XLST_TYPE)old_spvalue) {
					p[-1] = sp1->value.i;
					break;
				}
			}
		}
	else
		/*
		 * Check if there are declared parameters missing in the argument list.
		 *  (this needs to be done iff lc_auto!=0, so with DETAILED_ERR not defined
		 *   this is just OK)
		 */
#ifdef DETAILED_ERR
		for (i=0;i<N_HASH;i++) {
			sp1 = lsyms.h[i].tail;
			while (sp1 != 0) {
				/*
				 * assume that value.i is negative for normal auto variables
				 * and positive for parameters -- in fact this is correct either
				 * with REGPARM not defined, or before the parameter scan whose code is
				 * just above this
				 */
				if (sp1->value.i <= 0)
#endif
					error(ERR_ARG);
#ifdef DETAILED_ERR
				sp1 = sp1->prev;
			}
		}
#endif
#ifdef REGPARM
	reg_size=lc_auto;
#endif
	if (!start_block(0))
		error(ERR_BLOCK);
	else {
		block(sp);
		funcbottom();
	}
	global_flag = old_global;
	compound_done();					// due to block(sp)
#ifdef VERBOSE
	times(&tms_buf);
	parse_time += tms_buf.tms_utime - ltime;
#endif							/* VERBOSE */
}

struct sym *mk_int(char *name) {
	struct sym	   *sp;
	TYP 		   *tp;
	sp = (struct sym *) xalloc((int) sizeof(struct sym), _SYM+MK_INT);
	tp = (TYP *) xalloc((int) sizeof(TYP),_TYP+MK_INT);
	if (short_option) {
		tp->type = bt_short;
		tp->size = 2;
	} else {
		tp->type = bt_long;
		tp->size = 4;
	}
#ifdef NO_CALLOC
	tp->btp = 0;
	tp->lst.tail = tp->lst.head = 0; tp->lst.hash = 0;
	tp->vol_flag = tp->val_flag = tp->const_flag = tp->st_flag = 0;
#ifdef LISTING
	tp->sname = 0;
#endif
#endif
	sp->name = name;
	sp->storage_class = sc_auto;
	sp->tp = tp;
#ifdef NO_CALLOC
	sp->value.i = 0;			/* dummy -> means param is undeclared */
#endif
	append(&sp, &lsyms);
	return sp;
}

void check_table(HTABLE *table) {
	SYM *head;
	struct htab *ptr; int i;
	ptr=&table->h[0];
	i=N_HASH;
	while (i--) {
		head=ptr->head;
		while (head != 0) {
			if (head->storage_class == sc_ulabel) {
				uerrc2("undefined label '%s'",head->name);
//				msg2("*** UNDEFINED LABEL - %s\n", head->name);
#ifdef LISTING
				if (list_option)
					fprintf(list, "*** UNDEFINED LABEL - %s\n", head->name);
#endif
			}
			head = head->next;
		}
		ptr++;
	}
}

void funcbottom(void) {
	nl();
	check_table(&labsyms);
#ifdef LISTING
//	if (list_option && lsyms.head != 0) {
		fprintf(list, "\n\n*** argument symbol table ***\n\n");
		list_table(&lsyms, 0);
		fprintf(list, "\n\n\n");
//	}
//	if (list_option && labsyms.head != 0) {
		fprintf(list, "\n\n*** label symbol table ***\n\n");
		list_table(&labsyms, 0);
		fprintf(list, "\n\n\n");
//	}
#endif
#ifdef AS
	local_clean();
#endif
	rel_local();				/* release local symbols */
	hashinit(&lsyms);
	hashinit(&ltags);
	hashinit(&labsyms);
}

extern char *curname;
xstatic struct enode save_sp CGLOBL;
struct snode *dump_stmt;
SYM *func_sp CGLOB;
void block(struct sym *sp) { /* CAUTION : always requires a compound_done() after call */
	struct snode   *stmt;
	int local_total_errors = total_errors;
	int				line0;
#ifdef VERBOSE
	time_t			ltime;
#endif							/* VERBOSE */

//#ifdef VERBOSE
#ifdef AS
	scope_init();
#endif
	func_sp=sp;
	/*infunc("fire")
		bkpt();*/
#ifndef GTDEV
#ifdef PC
	if (verbose)
#endif
		msg2("Compiling '%s'... ", sp->name);
#endif
//#endif							/* VERBOSE */

#ifdef ICODE
	if (icode_option)
		if (sp->storage_class == sc_external || sp->storage_class == sc_global)
			fprintf(icode, "%s:\n", sp->name);
		else
			fprintf(icode, "L%ld:\n", sp->value.i);
#endif
#ifdef SHOWSTEP
	printf("\nparsing");
#endif
	start_block(1);
	line0=prevlineid;
	dump_stmt = stmt = compound(-1);
#ifdef SHOWSTEP
	printf(" ok ");
#endif
#ifdef VERBOSE
	times(&tms_buf);
	ltime = tms_buf.tms_utime;
#endif							/* VERBOSE */
/*
 * If errors so far, do not try to generate code
 */
	if (total_errors > local_total_errors) {
		cseg();
		dumplits();
		return;
	}
	lineid=line0;
	genfunc(stmt);
#ifdef VERBOSE
	times(&tms_buf);
	gen_time += tms_buf.tms_utime - ltime;
	ltime = tms_buf.tms_utime;
#endif							/* VERBOSE */
	cseg();
	dumplits();
	put_align(AL_FUNC);
#ifndef AS
	if (sp->storage_class == sc_external || sp->storage_class == sc_global)
		g_strlab(sp->name);
	else
		put_label((unsigned int) sp->value.i);
#else
	put_label(splbl(sp));
#endif
	if (!strcmp(sp->name,"__main") && !search("NO_EXIT_SUPPORT",-1,&defsyms)) {
		save_sp.nodetype = en_nacon;
		save_sp.etype = bt_pointer;
		save_sp.esize = 4;
		save_sp.v.ensp = "__save__sp__";
#ifdef AS
		save_sp.v.enlab = label("__save__sp__");
#endif
		g_coder(op_move,4,mk_reg(STACKPTR),mk_offset(&save_sp));
	}
	flush_peep();
#ifdef AS
	scope_flush();
#endif
	func_sp=NULL;
#ifdef VERBOSE
	times(&tms_buf);
	flush_time += tms_buf.tms_utime - ltime;
#endif							/* VERBOSE */
}

void castback(long offset, TYP *tp1, TYP *tp2) {
/*
 * cast an argument back which has been widened on the caller's side.
 * append the resulting assignment expression to init_node
 */
	struct enode   *ep1, *ep2;

	ep2 = mk_node(en_autocon, NIL_ENODE, NIL_ENODE);
	ep2->v.i = offset;
	ep2->etype = bt_pointer;
	ep2->esize = 4;

	ep1 = copynode(ep2);

	ep2 = mk_node(en_ref, ep2, NIL_ENODE);
	ep2->etype = tp1->type;
	ep2->esize = tp1->size;

	ep2 = mk_node(en_cast, ep2, NIL_ENODE);
	ep2->etype = tp2->type;
	ep2->esize = tp2->size;

	ep1 = mk_node(en_ref, ep1, NIL_ENODE);
	ep1->etype = tp2->type;
	ep1->esize = tp2->size;

	ep1 = mk_node(en_assign, ep1, ep2);
	ep1->etype = tp2->type;
	ep1->esize = tp2->size;

	if (init_node == 0)
		init_node = ep1;
	else
		init_node = mk_node(en_void, init_node, ep1);
}
// vim:ts=4:sw=4
