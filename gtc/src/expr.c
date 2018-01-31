/*
 * GTools C compiler
 * =================
 * source file :
 * expressions
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
#ifndef NOFLOAT
#include	"ffplib.h"
#endif
#ifdef PC
#ifdef SHORT_INT
#undef int
#endif
#include		<ctype.h>
#ifdef SHORT_INT
#define int short
#endif
#else
#include "define.h"
#endif

long inittype();

unsigned char sizeof_flag CGLOB;
unsigned char id_are_zero CGLOB;
#ifdef MID_DECL_IN_EXPR
struct enode	 *md_expr CGLOB;
struct typ		 *md_type CGLOB;
#endif
/*
 * expression evaluation
 *
 * this set of routines builds a parse tree for an expression. no code is
 * generated for the expressions during the build, this is the job of the
 * codegen module. for most purposes expression() is the routine to call. it
 * will allow all of the C operators. for the case where the comma operator
 * is not valid (function parameters for instance) call exprnc().
 *
 * each of the routines returns a pointer to a describing type structure. each
 * routine also takes one parameter which is a pointer to an expression node
 * by reference (address of pointer). the completed expression is returned in
 * this pointer. all routines return either a pointer to a valid type or NULL
 * if the hierarchy of the next operator is too low or the next symbol is not
 * part of an expression.
 */

static TYP *primary();
static TYP *unary();
static TYP *multops();
static TYP *addops();
#ifndef isscalar
static int isscalar();
#endif
TYP *force_cast_op();

int not_lvalue(struct enode *node) {
	while (node->nodetype==en_cast) node=node->v.p[0];
	return node->nodetype-en_ref;
}

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
/*	if (i==0x4E0003)
		__HALT;*/
/*	if (global_flag && !temp_mem)
		printf("gfdiog");*/
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

TYP *deref(struct enode **node, TYP *tp) {
/*
 * build the proper dereference operation for a node using the type pointer
 * tp.
 */
	switch (tp->type) {
	  case bt_void:
	  case bt_char:
	  case bt_uchar:
	  case bt_short:
	  case bt_ushort:
	  case bt_long:
	  case bt_pointer:
	  case bt_ulong:
	  case bt_struct:
	  case bt_union:
	  case bt_float:
#ifdef DOUBLE
	  case bt_double:
#endif
		*node = mk_node(en_ref, *node, NIL_ENODE);
		(*node)->etype = tp->type;
		(*node)->esize = tp->size;
		break;
	  case bt_bitfield:
		*node = mk_node(en_fieldref, *node, NIL_ENODE);
		(*node)->bit_width = tp->bit_width;
		(*node)->bit_offset = tp->bit_offset;
		/*
		 * maybe it should be 'unsigned'
		 */
		(*node)->etype = tp_int.type;
		(*node)->esize = tp_int.size;
		tp = (TYP *)&tp_int;
		break;
	  default:
		error(ERR_DEREF);
		break;
	}
	return tp;
}

TYP *cond_deref(struct enode **node, TYP *tp) {
	TYP 		   *tp1;
	/*
	 * dereference the node if val_flag is zero. If val_flag is non_zero and
	 * tp->type is bt_pointer (array reference), set the size field to the
	 * pointer size (if this code is not executed on behalf of a sizeof
	 * operator)
	 */

/*	infunc("chk_curword")
		bkpt();*/
	if (tp->val_flag == 0)
		return deref(node, tp);
	if (tp->type == bt_pointer && sizeof_flag == 0) {
		tp1 = tp->btp;
		tp = mk_type(bt_pointer, 4);
		tp->btp = tp1;
	}
	return tp;
}

TYP *nameref(struct enode **node) {
/*
 * nameref will build an expression tree that references an identifier. if
 * the identifier is not in the global or local symbol table then a
 * look-ahead to the next character is done and if it indicates a function
 * call the identifier is coerced to an external function name. non-value
 * references generate an additional level of indirection.
 */
	struct sym	   *sp;
	TYP 		   *tp;
	sp = lastsp;
	if (sp == 0) {
		getcache(id);
		if (cached_sym == openpa && !cached_flag) {
			uwarn("function '%s' not defined; assuming extern returning int", lastid);
			++global_flag;
			sp = (struct sym *) xalloc((int) sizeof(struct sym), _SYM+NAMEREF);
			sp->tp = (TYP *)&tp_func;
			sp->name = strsave(lastid);
			sp->storage_class = sc_external;
			append(&sp, &gsyms);
			--global_flag;
			tp = (TYP *)&tp_func;
			*node = mk_node(en_nacon, NIL_ENODE, NIL_ENODE);
			(*node)->v.ensp = sp->name;
#ifdef AS
			(*node)->v.enlab = splbl(sp);
#endif
			sp->used = 1;
			(*node)->etype = bt_pointer;
			(*node)->esize = 4;
			if (asm_zflag) (*node)->etype = bt_ulong;
		} else {
			if (lastid[0]=='_' && lastid[1]=='R' && !strncmp(lastid+3,"M_CALL_",7)) {
				++global_flag;
				sp = (struct sym *) xalloc((int) sizeof(struct sym), _SYM+NAMEREF);
				sp->tp = (TYP *)&tp_void_ptr;
				sp->name = strsave(lastid);
				sp->storage_class = sc_external;
				append(&sp, &gsyms);
				--global_flag;
				goto ok_sp;
			} else {
				if (!asm_xflag) {
					tp = 0;
					uerr(ERR_UNDEFINED,lastid);
				} else {
					tp = (TYP *)&tp_ulong;
					*node = mk_node(en_nacon, NIL_ENODE, NIL_ENODE);
					(*node)->v.ensp = strsave(lastid);
		#ifdef AS
					(*node)->v.enlab = label(lastid);
		#endif
					(*node)->etype = bt_ulong;
					(*node)->esize = 4;
				}
			}
		}
	} else {
	  ok_sp:
		if ((tp = sp->tp) == 0) {
			uerr(ERR_UNDEFINED,lastid);
			return 0;			/* guard against untyped entries */
		}
		switch (sp->storage_class) {
		  case sc_static:
			*node = mk_node(en_labcon, NIL_ENODE, NIL_ENODE);
#ifdef AS
			(*node)->v.enlab = splbl(sp);
#else
			(*node)->v.enlab = sp->value.i;
#endif
			(*node)->etype = bt_pointer;
			(*node)->esize = 4;
			break;
		  case sc_global:
		  case sc_external:
			*node = mk_node(en_nacon, NIL_ENODE, NIL_ENODE);
			(*node)->v.ensp = sp->name;
#ifdef AS
			(*node)->v.enlab = splbl(sp);
#endif
			sp->used = 1;
			(*node)->etype = bt_pointer;
			(*node)->esize = 4;
			break;
		  case sc_const:
			*node = mk_icon((long) sp->value.i);
			(*node)->etype = tp_econst.type;
			(*node)->esize = tp_econst.size;
			break;
		  default:				/* auto and any errors */
			if (sp->storage_class != sc_auto)
				error(ERR_ILLCLASS);
			*node = mk_node(en_autocon, NIL_ENODE, NIL_ENODE);
			(*node)->v.i = sp->value.i;
			(*node)->etype = bt_pointer;
			(*node)->esize = 4;
/*			infunc("chk_curword")
				if (sp->value.i==(long)(char)0xCF)
					bkpt();*/
/*			infunc("chk_curword")
				if (*(long *)node==0x7b4bf8)
					bkpt();*/
			break;
		}
		if (asm_zflag) {
			if ((*node)->etype == bt_pointer)
				(*node)->etype = bt_ulong;
			if (tp->type == bt_pointer || tp->type == bt_struct || tp->type == bt_union)
				tp = (TYP *)&tp_ulong;
		} else tp = cond_deref(node, tp);
	}
	getsym();
	return tp;
}

#ifndef NO_VARARG_FUNC
extern char variable_arg_name[];
#define is_variable_arg(sp) ((sp)->name==variable_arg_name)
#define mk_variable_arg(sp) ((sp)->name=variable_arg_name)
#endif

struct enode *parmlist(SYM *f) {
/*
 * parmlist will build a list of parameter expressions in a function call and
 * return a pointer to the last expression parsed. since parameters are
 * generally pushed from right to left we get just what we asked for...
 */
	struct enode   *ep1, *ep2;
	TYP 		   *tp;
	ep1 = 0;
	if (lastst != closepa)
		for (;;) {
			tp = exprnc(&ep2);		/* evaluate a parameter */
			if (tp == 0)
				error(ERR_EXPREXPECT);
#ifdef INTEL_386
			/* trap struct assigns */
			if (isaggregate(tp))
				uses_structassign=1;
#endif
			/*
			 * do the default promotions
			 */
			if (f) {
				if (tp->const_flag && f->tp->type==bt_pointer && !f->tp->btp->const_flag)
					uwarn("read-only arg might be modified by function");
				tp = cast_op(&ep2, tp, f->tp);
			}
#ifndef NOFLOAT
#ifdef DOUBLE
			if (tp->type == bt_float)
				tp = cast_op(&ep2, tp, (TYP *)&tp_double);
#endif
#endif
			if (short_option) {
				if (tp->type == bt_char || tp->type == bt_uchar)
					(void) cast_op(&ep2, tp, (TYP *)&tp_short);
			} else {
				if (tp->type == bt_uchar || tp->type == bt_char ||
						tp->type == bt_short || tp->type == bt_ushort)
					(void) cast_op(&ep2, tp, (TYP *)&tp_long);
			}
			ep1 = mk_node(en_void, ep2, ep1);
			if (lastst != comma)
				break;
			getsym();
			if (f && !(f=f->next)) error(ERR_TOOMPARAMS);
#ifndef NO_VARARG_FUNC
			if (f && is_variable_arg(f)) f=NULL;	// switch to K&R mode for variable args...
#endif
		}
	if (f && f->next && !is_variable_arg(f->next)) error(ERR_TOOFPARAMS);
	return ep1;
}

int castbegin(enum(e_sym) st) {
/*
 * return 1 if st in set of [ kw_char, kw_short, kw_long, kw_float,
 * kw_double, kw_struct, kw_union ] CVW change: or kw_void CVW change: or an
 * type identifier
 */
	if (st == kw_char || st == kw_short || st == kw_int ||
		st == kw_long || st == kw_float || st == kw_double ||
		st == kw_struct || st == kw_union || st == kw_unsigned ||
		st == kw_void || st == kw_enum || st == kw_typeof ||
		st == kw_signed || st == kw_const || st == kw_volatile)
		return 1;
	if (st == id && lastsp != 0 &&
		lastsp->storage_class == sc_typedef)
		return 1;
	return 0;
}

static TYP *primary(struct enode **node) {
/*
 * primary will parse a primary expression and set the node pointer returning
 * the type of the expression parsed. primary expressions are any of:
 * id
 * constant
 * string
 * ( expression )
 * primary[ expression ]
#ifndef OLD_PRIOR
 * primary++
 * primary--
#endif
 * primary.id
 * primary->id
 * primary( parameter list )
 * -- or just a semicolon, yields empty expression --
 *
 */
	struct enode   *pnode, *qnode, *rnode;
	struct sym	   *sp;
	TYP 		   *tptr;
	TYP 		   *tp1,*tp2;
	switch (lastst) {

	  case id:
		if (id_are_zero) {
			pnode = mk_icon(0l);
			tptr = (TYP *)&tp_long;			// dans #if, le type par défaut est 'long'
			pnode->etype = tp_long.type;
			pnode->esize = tp_long.size;
			getsym();
			break;
		}
		tptr = nameref(&pnode);
		if (tptr == 0)
			break;
		/*
		 * function names alone are pointers to functions.
		 * If followed by '(', the reference is stripped off
		 * later.
		 */
		if (tptr->type == bt_func) {
		   tp1 = mk_type(bt_pointer, 4);
		   tp1->btp = tptr;
		   tptr = tp1;
		}
		break;
	  case iconst:
	  case uconst:
	  case lconst:
	  case ulconst:
		pnode = mk_icon(0l);
		pnode->v.i = ival;
		if (lastst == uconst) {
			tptr = (TYP *)&tp_uint;
			pnode->etype = tp_uint.type;
			pnode->esize = tp_uint.size;
		} else if (lastst == lconst) {
			tptr = (TYP *)&tp_long;
			pnode->etype = bt_long;
			pnode->esize = 4;
		} else if (lastst == ulconst) {
			tptr = (TYP *)&tp_ulong;
			pnode->etype = bt_ulong;
			pnode->esize = 4;
		} else {
			tptr = (TYP *)&tp_int;
			pnode->etype = tp_int.type;
			pnode->esize = tp_int.size;
		}
		getsym();
		break;
#ifndef NOFLOAT
	  case rconst:
		tptr = (TYP *)&tp_double;
		pnode = mk_node(en_fcon, NIL_ENODE, NIL_ENODE);
		pnode->v.f = rval;
		pnode->etype = tp_double.type;
		pnode->esize = tp_double.size;
		getsym();
		break;
#endif
	  case sconst:
		if (sizeof_flag) {
			tptr = mk_type(bt_pointer, 0);
			tptr->size = lstrlen + 1;
			tptr->btp = (TYP *)&tp_char;
			tptr->val_flag = 1;
		} else
			tptr = (TYP *)&tp_string;
		pnode = mk_node(en_labcon, NIL_ENODE, NIL_ENODE);
		if (sizeof_flag == 0)
			pnode->v.enlab = stringlit(laststr, lstrlen);
		pnode->etype = bt_pointer;
		pnode->esize = 4;
		getsym();
		break;
	  case kw_softcast:
		getsym();
		if (!castbegin(lastst)) {
			error(ERR_SYNTAX);
			tptr = 0; // just to avoid a compiler warning =)
		} else {
			struct typ *local_head = head, *local_tail=tail;
			decl((HTABLE *)NULL);	/* do cast declaration */
			decl1();
			tptr = head;
			needpunc(closepa);
			if (!(tp1 = unary(&pnode))) {
				error(ERR_IDEXPECT);
				tptr = 0;
			} else
				/* do the cast */
				tptr = cast_op(&pnode, tp1, tptr);
			head = local_head;
			tail = local_tail;
		}
		break;
	  case openpa:
		getsym();
		if (!castbegin(lastst)) {
			if (lastst==begin) {	/* compound expression */
				struct snode *snp;
				struct enode *old_init_node = init_node;
				int old_middle_decl = middle_decl;
				getsym();
				init_node = 0;
				middle_decl = 0;
				snp = compound(0);
				init_node = old_init_node;
				middle_decl = old_middle_decl;
				tptr = lastexpr_tp;
				pnode = mk_node(en_compound, (struct enode *)snp, NIL_ENODE);
				pnode->etype=lastexpr_type;
				pnode->esize=lastexpr_size;
			} else tptr = expression(&pnode);
			needpunc(closepa);
		} else {				/* cast operator or middle declaration */
			struct typ *local_head = head, *local_tail=tail;
#ifdef MID_DECL_IN_EXPR
			struct enode *old_md_expr = md_expr;
			struct typ *old_md_type = md_type;
			md_expr = 0;
			middle_decl++;
			dodecl(sc_auto);		/* if it's a real cast, same as decl(NULL);decl1(); */
			middle_decl--;
#else
			decl((HTABLE *)NULL);	/* do cast declaration */
			decl1();
#endif
			tptr = head;
			needpunc(closepa);
#ifdef MID_DECL_IN_EXPR
			if ((pnode = md_expr))
				tptr = md_type;	/* we can't use head since parsing md_expr might change it */
			md_expr = old_md_expr;
			md_type = old_md_type;
			if (pnode)
				break;
#endif
			if (lastst==begin) {	/* cast constructor */
				int lab=nxtlabel();
				TYP *tp=tptr;
				nl();
				dseg();
				put_align(alignment(tptr));
				put_label(lab);
				inittype(tp,&tp);
				pnode = mk_node(en_labcon, NIL_ENODE, NIL_ENODE);
				pnode->v.enlab=lab;
				pnode->etype=bt_pointer;
				pnode->esize=4;
				tptr=cond_deref(&pnode,tp);
			} else if ((tp1 = unary(&pnode)) == 0) {
				error(ERR_IDEXPECT);
				tptr = 0;
			} else {
				/* do the cast */
				tptr = force_cast_op(&pnode, tp1, tptr);
			}
			head = local_head;
			tail = local_tail;
		}
		break;
	  default:
		tptr=0;
		break;
	}
	if (tptr == 0)
		return 0;
	for (;;) {
		switch (lastst) {
		  case openbr:			/* build a subscript reference */
			getsym();
/*
 * a[b] is defined as *(a+b), such exactly one of (a,b) must be a pointer
 * and one of (a,b) must be an integer expression
 */
			if (tptr->type == bt_pointer) {
				tp2 = expression(&rnode);
				tp1 = tptr;
			} else {
				tp2 = tptr;
				rnode = pnode;
				tp1 = expression(&pnode);
				tptr = tp1;
			}

/*
 * now, pnode and tp1 describe the pointer,
 *		rnode and tp2 describe the integral value
 */

			if (tptr->type != bt_pointer)
				error (ERR_NOPOINTER);
			else
				tptr = tptr->btp;

			if (tptr->size==1) {	/* doing this : 1) saves RAM; 2) allows file_ESI[filesize] */
				cast_op(&rnode, tp2, (TYP *)&tp_long);	/*			(even with fast_array on)  */
				cast_op(&rnode, (TYP *)&tp_long, tp1);
				qnode=rnode;
			} else {
				qnode = mk_icon((long) tptr->size);
	#ifdef NO_SECURE_POINTERS
				qnode->etype = bt_short;
				qnode->esize = 2;
	#else
				qnode->etype = bt_long;
				qnode->esize = 4;
	#endif
	/*
	 * qnode is the size of the referenced object
	 */
	#ifdef NO_SECURE_POINTERS
				cast_op(&rnode, tp2, (TYP *)&tp_short);
	#else
				cast_op(&rnode, tp2, (TYP *)&tp_long);
	#endif
				/*
				 * we could check the type of the expression here...
				 */
	#ifdef NO_SECURE_POINTERS
				qnode = mk_node(en_mul, qnode, rnode);
				qnode->etype = bt_short;
				qnode->esize = 2;
				cast_op(&qnode, (TYP *)&tp_short, (TYP *)&tp_long);
				cast_op(&qnode, (TYP *)&tp_long, tp1);
	#else
				qnode = mk_node(en_mul, qnode, rnode);
				qnode->etype = bt_long;
				qnode->esize = 4;
				cast_op(&qnode, (TYP *)&tp_long, tp1);
	#endif
			}
			pnode = mk_node(en_add, pnode, qnode);
//			pnode = mk_node(en_add, qnode, pnode);
			pnode->etype = bt_pointer;
			pnode->esize = 4;
			tptr = cond_deref(&pnode, tptr);
			needpunc(closebr);
			break;
		  case pointsto:
			if (tptr->type != bt_pointer)
				error(ERR_NOPOINTER);
			else
				tptr = tptr->btp;
			/*
			 * tptr->type should be bt_struct or bt_union tptr->val_flag
			 * should be 0 the ref node will be stripped off in a minute
			 */
			tptr = cond_deref(&pnode, tptr);
			/*
			 * fall through to dot operation
			 */
		  case dot:
			getsym();			/* past -> or . */
			if (lastst != id)
				error(ERR_IDEXPECT);
			else {
				if (tptr->type!=bt_struct && tptr->type!=bt_union)
					uerrc("not a structure/union");
				{
				TABLE *tab=&tptr->lst;
				long offs=0;
			search_again:
				sp = search(lastid, lastcrc, (HTABLE *)tab);
				if (sp == 0) {
					sp = search("__unnamed__", -1, (HTABLE *)tab);
					if (!sp || (sp->tp->type!=bt_union && sp->tp->type!=bt_struct))
						uerr(ERR_NOMEMBER,lastid);
					else {
						offs += sp->value.i;
						tab = &sp->tp->lst;
						goto search_again;
					}
				} else {
					/* strip off the en_ref node on top */
					if (lvalue(pnode))
						pnode = pnode->v.p[0];
					else {
						pnode = mk_node(en_deref, pnode, NIL_ENODE);
						pnode->etype = bt_pointer;
						pnode->esize = 4;
					}
					tptr = sp->tp;
					qnode = mk_icon((long) (offs+sp->value.i));
					qnode->etype = bt_long;
					qnode->esize = 4;
					pnode = mk_node(en_add, pnode, qnode);
					pnode->etype = bt_pointer;
					pnode->esize = 4;
					tptr = cond_deref(&pnode, tptr);
				}
				}
				getsym();		/* past id */
			}
			break;
		  case openpa:			/* function reference */
#ifdef ASM
#ifndef OLD_AMODE_INPUT
			if (asm_zflag) goto fini;
#endif
#endif
			getsym();
			/*
			 *	the '*' may be ommitted with pointers to functions
			 *	we have included another indirection (see above, case id:)
			 */
			if (tptr->type == bt_pointer)
				tptr = tptr->btp;
			if (tptr->type != bt_func)
				error(ERR_NOFUNC);

			/*
			 * This hack lets us remember that this function itself calls
			 * other functions.
			 * The code generator might use this information to generate
			 * safer register-pop-off code.
			 */
			is_leaf_function = 0;

			pnode = mk_node(en_fcall, pnode, parmlist(tptr->lst.head));
#ifdef REGPARM
			pnode->rp_dn = tptr->rp_dn;
			pnode->rp_an = tptr->rp_an;
#endif
			tptr = tptr->btp;
			pnode->etype = tptr->type;
			pnode->esize = tptr->size;
			needpunc(closepa);
			break;
		  case autoinc:
		  case autodec:
			if (g_lvalue(pnode)) {
				qnode = mk_icon((tptr->type==bt_pointer?(long)tptr->btp->size:1L));
				pnode = mk_node(lastst==autoinc?en_ainc:en_adec, pnode, qnode);
				pnode->etype = tptr->type;
				pnode->esize = tptr->size;
			} else
				error(ERR_LVALUE);
			getsym();
			break;
		  default:
			goto fini;
		}
	}
fini:
	*node = pnode;
	return tptr;
}

static TYP *unary(struct enode **node) {
/*
 * unary evaluates unary expressions and returns the type of the expression
 * evaluated. unary expressions are any of:
 *
 * primary
#ifdef OLD_PRIOR
 * primary++
 * primary--
#endif
 * !unary
 * ~unary
 * ++unary
 * --unary
 * -unary
 * *unary
 * &unary
 * (typecast)unary
 * sizeof(typecast)
 * sizeof unary
 *
 */
	TYP 		   *tp, *tp1;
	struct enode   *ep1, *ep2;
	int 			flag;
//	long			i;
	flag = 0;
	switch (lastst) {
	  case autodec:
		flag = 1;
		/* fall through to common increment */
	  case autoinc:
		getsym();
		tp = unary(&ep1);
		if (tp == 0) {
			error(ERR_IDEXPECT);
			return 0;
		}
		if (g_lvalue(ep1)) {
			if (tp->type == bt_pointer)
				ep2 = mk_icon((long) tp->btp->size);
			else {
				ep2 = mk_icon(1l);
				if (!integral(tp))
					error(ERR_INTEGER);
			}

			ep2->etype = bt_long;
			ep2->esize = 4;
			ep1 = mk_node(flag ? en_assub : en_asadd, ep1, ep2);
			ep1->etype = tp->type;
			ep1->esize = tp->size;
		} else
			error(ERR_LVALUE);
		break;
	  case minus:
		getsym();
		tp = unary(&ep1);
		if (tp == 0) {
			error(ERR_IDEXPECT);
			return 0;
		}
		ep1 = mk_node(en_uminus, ep1, NIL_ENODE);
		ep1->etype = tp->type;
		ep1->esize = tp->size;
		break;
	  case not:
		getsym();
		tp = unary(&ep1);
		if (tp == 0) {
			error(ERR_IDEXPECT);
			return 0;
		}
		if (!bt_comparable(tp->type))
			uerrc("cannot test expression");
		ep1 = mk_node(en_not, ep1, NIL_ENODE);
		tp = (TYP *)&tp_int;
		ep1->etype = tp_int.type;
		ep1->esize = tp_int.size;
		break;
	  case compl:
		getsym();
		tp = unary(&ep1);
		if (tp == 0) {
			error(ERR_IDEXPECT);
			return 0;
		}
		ep1 = mk_node(en_compl, ep1, NIL_ENODE);
		ep1->etype = tp->type;
		ep1->esize = tp->size;
		if (!integral(tp))
			error(ERR_INTEGER);
		break;
	  case star:
		getsym();
		tp = unary(&ep1);
		if (tp == 0) {
			error(ERR_IDEXPECT);
			return 0;
		}
		if (tp->type!=bt_pointer)
			error(ERR_DEREF);
		else
			tp = tp->btp;
		tp = cond_deref(&ep1, tp);
		break;
	  case and:
		getsym();
		/*if (lineid==0x216)
			bkpt();*/
		tp = unary(&ep1);
		if (tp == 0) {
			error(ERR_IDEXPECT);
			return 0;
		}
		if (lvalue(ep1)) {	/* TODO: adapt this to g_lvalue */
/*			if (ep1->nodetype==en_cast	*/
			ep1 = ep1->v.p[0];
			tp1 = mk_type(bt_pointer, 4);
			tp1->st_flag = 0;
			tp1->btp = tp;
			tp = tp1;
		} else if (tp->type == bt_pointer && tp->btp->type == bt_func) {
			uwarn("'&' operator ignored");
		} else
			error(ERR_LVALUE);
		break;
	  case kw_c: {
		int zf=asm_zflag;
		asm_zflag=0;
		getsym();
		needpunc(openpa);
		tp=expression(&ep1);
		asm_zflag=zf;
		needpunc(closepa);
	   } break;
	  case kw_defined:
		skipspace();
		{ int parenth=0;
		if (lastch=='(') getch(),parenth=1;
		skipspace();
		if ((lastch>='A'&&lastch<='Z') || (lastch>='a'&&lastch<='z')
			|| lastch=='_' || lastch=='$') {
			getidstr();
			ep1=mk_icon((long)!!search(lastid,lastcrc,&defsyms));
			tp = (TYP *)&tp_int;
			ep1->etype = tp_int.type;
			ep1->esize = tp_int.size;
			getsym();
		} else {
			error(ERR_IDEXPECT);
			tp = 0; // just to avoid a compiler warning =)
		}
		if (parenth)
			needpunc(closepa);
		}
		break;
	  case kwb_constant_p:
		getsym();
		needpunc(openpa);
		if (!expression(&ep1)) error(ERR_EXPREXPECT);
		needpunc(closepa);
		opt0(&ep1);
		ep1=mk_icon((long)(ep1->nodetype==en_icon || ep1->nodetype==en_fcon));
		tp = (TYP *)&tp_int;
		ep1->etype = tp_int.type;
		ep1->esize = tp_int.size;
		break;
	  case kw_alloca:
		getsym();
		needpunc(openpa);
		if (!(tp=expression(&ep1))) error(ERR_EXPREXPECT);
		needpunc(closepa);
		uses_link = 1;
		cast_op(&ep1,tp,(TYP *)&tp_short);
		ep1 = mk_node(en_alloca,ep1,NIL_ENODE);
		tp = (TYP *)&tp_void_ptr;
		ep1->etype = tp_void_ptr.type;
		ep1->esize = tp_void_ptr.size;
		break;
	  case kw_sizeof:
		getsym();
		if (lastst == openpa) {
			flag = 1;
			getsym();
		}
		if (flag && castbegin(lastst)) {
			/*
			 * save head and tail, since we may be called from inside decl
			 * imagine: int x[sizeof(...)];
			 */
			tp = head;
			tp1 = tail;
			decl((HTABLE *) 0);
			decl1();
			if (head != 0) {
				ep1 = mk_icon((long) head->size);
				/*
				 * Guard against the size of not-yet-declared struct/unions
				 */
				if (head->size ==  0) {
					uwarn("sizeof(item) = 0");
				}
			} else
				ep1 = mk_icon(1l);
			head = tp;
			tail = tp1;
		} else {
/*
 * This is a mess.
 * Normally, we treat array names just as pointers, but with sizeof,
 * we want to get the real array size.
 * sizeof_flag != 0 tells cond_deref not to convert array names to pointers
 */
			sizeof_flag++;
			tp = unary(&ep1);
			sizeof_flag--;
			if (tp == 0) {
				error(ERR_SYNTAX);
				ep1 = mk_icon(1l);
			} else
				ep1 = mk_icon((long) tp->size);
		}
/*		if (short_option && ep1->v.i > 65535)
			do_warning("'sizeof' value greater than 65535\n");*/
		tp = (TYP *)&tp_uint;
		ep1->etype = tp_uint.type;
		ep1->esize = tp_uint.size;
		if (flag)
			needpunc(closepa);
		break;
	  default:
		tp = primary(&ep1);
		break;
	}
	*node = ep1;
	return tp;
}

#ifndef NO_TYPE_STR
int type_str_pos CGLOB;
typedef char type_str_content_type[32];
type_str_content_type type_str_content[4];
char *type_str(TYP *tp) {
	char *p,*q;
	int n;
	switch (tp->type) {
	  case bt_char:
		return "char";
	  case bt_uchar:
		return "unsigned char";
	  case bt_short:
		return "short";
	  case bt_ushort:
		return "unsigned short";
	  case bt_long:
		return "long";
	  case bt_ulong:
		return "unsigned long";
	  case bt_float:
		return "float";
#ifdef DOUBLE
	  case bt_double:
		return "double";
#endif
	  case bt_void:
		return "void";
	  case bt_struct:
#ifdef LISTING
		if (tp->sname) return tp->sname;
#endif
		return "<struct>";
	  case bt_union:
		return "<union>";
	  case bt_func:
		return "<func>";
	  case bt_bitfield:
		return "<bitfield>";
	  case bt_pointer:
		n=type_str_pos;
		q=p=type_str(tp->btp);
		if (n==type_str_pos) {
			type_str_pos++;
			strcpy(type_str_content[n],p);
			q=p=type_str_content[n];
			while (*p++);
			p[-1]=' '; *p++=0;
		} else while (*p++);
		if (!tp->val_flag) p[-1]='*';
		else p[-1]='[',*p++=']';
		*p=0;
		return q;
	}
	return "<unknown>";
}
#endif

TYP *forcefit(struct enode **node1, TYP *tp1, struct enode **node2, TYP *tp2) {
/*
 * forcefit will coerce the nodes passed into compatible types and return the
 * type of the resulting expression.
 */
	/* cast short and char to int */
	if (short_option) {
		if (tp1->type == bt_char || tp1->type == bt_uchar)
			tp1 = cast_op(node1, tp1, (TYP *)&tp_short);
		if (tp2->type == bt_char || tp2->type == bt_uchar)
			tp2 = cast_op(node2, tp2, (TYP *)&tp_short);
	} else {
		if (tp1->type == bt_char || tp1->type == bt_uchar ||
			tp1->type == bt_short || tp1->type == bt_ushort)
			tp1 = cast_op(node1, tp1, (TYP *)&tp_long);

		if (tp2->type == bt_char || tp2->type == bt_uchar ||
			tp2->type == bt_short || tp2->type == bt_ushort)
			tp2 = cast_op(node2, tp2, (TYP *)&tp_long);
	}

#ifndef NOFLOAT
	/* cast float to double */
#ifdef DOUBLE
	if (tp1->type == bt_float)
		tp1 = cast_op(node1, tp1, (TYP *)&tp_double);
	if (tp2->type == bt_float)
		tp2 = cast_op(node2, tp2, (TYP *)&tp_double);
#endif

	if (tp1->type == bt_double && isscalar(tp2))
		tp2 = cast_op(node2, tp2, (TYP *)&tp_double);
	else if (tp2->type == bt_double && isscalar(tp1))
		tp1 = cast_op(node1, tp1, (TYP *)&tp_double);
#endif

	if (tp1->type == bt_ulong && isscalar(tp2))
		tp2 = cast_op(node2, tp2, tp1);
	else if (tp2->type == bt_ulong && isscalar(tp1))
		tp1 = cast_op(node1, tp1, tp2);

	if (tp1->type == bt_long && isscalar(tp2))
		tp2 = cast_op(node2, tp2, tp1);
	else if (tp2->type == bt_long && isscalar(tp2))
		tp1 = cast_op(node1, tp1, tp2);

	if (tp1->type == bt_ushort && isscalar(tp2))
		tp2 = cast_op(node2, tp2, tp1);
	else if (tp2->type == bt_ushort && isscalar(tp2))
		tp1 = cast_op(node1, tp1, tp2);

	if (isscalar(tp1) && isscalar(tp2))
		return (tp1);

	/* pointers may be combined with integer constant 0 */
	if (tp1->type == bt_pointer && (*node2)->nodetype == en_icon &&
		(*node2)->v.i == 0)
		return tp1;
	if (tp2->type == bt_pointer && (*node1)->nodetype == en_icon &&
		(*node2)->v.i == 0)
		return tp2;

	if (tp1->type == bt_pointer && tp2->type == bt_pointer)
		return tp1;

	/* report mismatch error */

	uerr(ERR_MISMATCH,type_str(tp1),type_str(tp2));
	return tp1;
}

TYP *forceft2(struct enode **node1, TYP *tp1, struct enode **node2, TYP *tp2) {
/*
 * ,,forcefit'' for comparisons:
 * When comparing two char's, it is not necessary to cast
 * both of them to long in advance
 *
 * Perhaps not strictly K&R, but more efficient.
 * If you don't like it, use forcefit in ALL cases
 */

	/* short cut: */
	if (tp1->type == tp2->type)
		return tp1;

	/* comparison with integer constant */

	if ((*node1)->nodetype == en_icon) {
		struct enode  **node = node1;
		TYP 		   *tp = tp1;
		node1 = node2;
		tp1 = tp2;
		node2 = node;
		tp2 = tp;
	}
	opt4(node2);
	if ((*node2)->nodetype == en_icon) {
		long			val = (*node2)->v.i;
		enum(e_bt)		typ1 = tp1->type;
		if ((typ1 == bt_char && -128 <= val && val <= 127) ||
			(typ1 == bt_uchar && 0 <= val && val <= 255) ||
			(typ1 == bt_short && -32768 <= val && val <= 32767) ||
			(typ1 == bt_ushort && 0 <= val && val <= 65535) ||
			(typ1 == bt_pointer && val == 0))
			return cast_op(node2, tp2, tp1);
	}

	switch (tp1->type) {
		/* Type of first operand */
	  case bt_char:
	  case bt_uchar:
		switch (tp2->type) {
		  case bt_char:
		  case bt_uchar:
			(void) cast_op(node1, tp1, (TYP *)&tp_short);
			return cast_op(node2, tp2, (TYP *)&tp_short);
		  case bt_short:
		  case bt_long:
		  case bt_ushort:
		  case bt_ulong:
		  case bt_float:
#ifdef DOUBLE
		  case bt_double:
#endif
			return cast_op(node1, tp1, tp2);
		}
		break;
	  case bt_short:
	  case bt_ushort:
		switch (tp2->type) {
		  case bt_char:
		  case bt_uchar:
			return cast_op(node2, tp2, tp1);
		  case bt_ushort:
			if (short_option)
			   return cast_op (node1, tp1, (TYP *)&tp_ushort);
			else {
			   (void) cast_op (node1, tp1, (TYP *)&tp_long);
			   return cast_op (node2, tp2, (TYP *)&tp_long);
			}
		  case bt_short:
			if (short_option)
				return cast_op(node2, tp2, (TYP *)&tp_ushort);
			else {
			   (void) cast_op (node1, tp1, (TYP *)&tp_long);
			   return cast_op (node2, tp2, (TYP *)&tp_long);
			}
		  case bt_long:
		  case bt_ulong:
		  case bt_float:
#ifdef DOUBLE
		  case bt_double:
#endif
			return cast_op(node1, tp1, tp2);
		}
		break;
	  case bt_long:
	  case bt_ulong:
		switch (tp2->type) {
		  case bt_char:
		  case bt_uchar:
		  case bt_short:
		  case bt_ushort:
			return cast_op(node2, tp2, tp1);
		  case bt_long:
			return cast_op(node2, tp2, tp1);
		  case bt_ulong:
			return cast_op(node1, tp1, tp2);
		  case bt_float:
#ifdef DOUBLE
		  case bt_double:
#endif
			return cast_op(node1, tp1, tp2);
		}
		break;
	  case bt_float:
#ifdef DOUBLE
	  case bt_double:
#endif
		switch (tp2->type) {
		  case bt_char:
		  case bt_uchar:
		  case bt_short:
		  case bt_ushort:
		  case bt_long:
		  case bt_ulong:
		  case bt_float:
			return cast_op(node2, tp2, tp1);
#ifdef DOUBLE
		  case bt_double:
			return cast_op(node1, tp1, tp2);
#endif
		}
		break;
		/*
		 * pointers are equivalent to function names
		 */
	  case bt_pointer:
		if (tp2->type == bt_func)
			return cast_op(node2, tp2, tp1);
		break;
	  case bt_func:
		if (tp2->type == bt_pointer)
			return cast_op(node1, tp1, tp2);
		break;
	}
	uerr(ERR_MISMATCH,type_str(tp1),type_str(tp2));
	return 0;
}

#ifndef isscalar
static int isscalar(TYP *tp) {
/*
 * this function returns true when the type of the argument is a scalar type
 * (enum included)
 */
/*	return tp->type == bt_char ||
		tp->type == bt_uchar ||
		tp->type == bt_ushort ||
		tp->type == bt_short ||
		tp->type == bt_long ||
		tp->type == bt_ulong ||
		tp->type == bt_float ||
		tp->type == bt_double; */
	return bt_scalar(tp->type);
}
#endif

static TYP *multops(struct enode **node) {
/*
 * multops parses the multiply priority operators. the syntax of this group
 * is:
 *
 * unary multop * unary multop / unary multop % unary
 */
	struct enode   *ep1, *ep2;
	TYP 		   *tp1, *tp2;
	enum(e_sym)		oper;
	tp1 = unary(&ep1);
	if (tp1 == 0)
		return 0;
	while (lastst == star || lastst == divide || lastst == modop) {
		oper = lastst;
		getsym();				/* move on to next unary op */
/*		if (lineid==77)
			bkpt();*/
		tp2 = unary(&ep2);
		if (tp2 == 0) {
			error(ERR_IDEXPECT);
			*node = ep1;
			return tp1;
		}
#ifdef INTEL_386
		tp1 = forcefit(&ep1, tp1, &ep2, tp2);
#endif
		switch (oper) {
		  case star:
				/*{extern SYM *func_sp;
				if (!strcmp(func_sp->name,"bonus"))
					printf("gjio");}*/
/*					if (lineid==1189)
						bkpt();*/
				tp1 = forcefit(&ep1, tp1, &ep2, tp2);
#ifndef NOFLOAT
//#warning STUDY ME!!!
//			  uwarn("!!! study this !!!");
//			if (tp1->type==bt_float || tp2->type==bt_float)
//				tp1 = forcefit(&ep1, tp1, &ep2, tp2);
//			else
#endif
#ifdef MC68000
//			{
//				tp1 = cast_op(&ep1, tp1, (TYP *)&tp_long);
//				tp2 = cast_op(&ep2, tp2, (TYP *)&tp_long);
//			}
#endif
			ep1 = mk_node(en_mul, ep1, ep2);
#ifndef NOFLOAT
			if (tp1->type!=bt_float)
#endif
#ifdef MC68000
//			tp1 = (TYP *)&tp_long;
#endif
/*			if (bt_integral(tp1->type)) {
				if (bt_uns(tp1->type)) tp1=(TYP *)&tp_ulong;
				else tp1=(TYP *)&tp_long;
			}*/
			break;
		  case divide:
		  case modop:
#ifndef NOFLOAT
			if (tp1->type==bt_float || tp2->type==bt_float)
				tp1 = forcefit(&ep1, tp1, &ep2, tp2);
			else
#endif
#ifdef MC68000
			{
#ifdef OLD_STUPID_DIVIDE	/* !!! n'importe quoi !!! */
				tp1 = cast_op(&ep1, tp1, (TYP *)&tp_long);
				if (tp2->type==bt_long || tp2->type==bt_ulong) {
					ep1 = mk_node(oper==divide?en_div:en_mod, ep1, ep2);
					tp1 = (TYP *)&tp_long;
					break;
				}
				tp2 = cast_op(&ep2, tp2, (TYP *)&tp_short);
#else
				tp1 = forcefit(&ep1, tp1, &ep2, tp2);
#endif
			}
#endif
			ep1 = mk_node(oper==divide?en_div:en_mod, ep1, ep2);
#ifdef OLD_STUPID_DIVIDE	/* !!! n'importe quoi !!! */
#ifndef NOFLOAT
			if (tp1->type!=bt_float)
#endif
#ifdef MC68000
				tp1 = (TYP *)&tp_short;
#endif
#endif
			break;
		}
		ep1->etype = tp1->type;
		ep1->esize = tp1->size;
	}
	*node = ep1;
	return tp1;
}

static TYP *addops(struct enode **node) {
/*
 * addops handles the addition and subtraction operators.
 */
	struct enode   *ep1, *ep2, *ep3;
	TYP 		   *tp1, *tp2;
	int 			oper;
	tp1 = multops(&ep1);
	if (tp1 == 0)
		return 0;
	while (lastst == plus || lastst == minus) {
/*		if (asm_zflag)
			printf("gfi");*/
/*		if (lineid==852)
			bkpt();*/
		oper = (lastst - minus);
		getsym();
		tp2 = multops(&ep2);
		if (tp2 == 0) {
			error(ERR_IDEXPECT);
			*node = ep1;
			return tp1;
		}
		if (tp1->type == bt_pointer && tp2->type == bt_pointer
			&& tp1->btp->size == tp2->btp->size && (!oper)) {
			/* pointer subtraction */
			ep1 = mk_node(en_sub, ep1, ep2);
			ep1->etype = bt_long;
			ep1->esize = 4;
			/* divide the result by the size */
			ep2 = mk_icon((long) tp1->btp->size);
			ep2->etype = bt_long;
			ep2->esize = 4;
			ep1 = mk_node(en_div, ep1, ep2);
			ep1->etype = bt_long;
			ep1->esize = 4;
//#ifdef POINTER_DIFF_IS_SHORT
			/*
			 * cast the result to ,,int''. K&R says that pointer subtraction
			 * yields an int result so I do it although it is not sensible on
			 * an 68000 with 32-bit pointers and 16-bit ints. In my opinion,
			 * it should remain ,,long''.
			 */
//			if (short_option) do_warning("pointer difference casted to 16-bit 'int'\n");
			if (tp1->btp->size!=1)	/* if size is 1 maybe we want it bijective... */
				tp1 = cast_op(&ep1, (TYP *)&tp_long, (TYP *)&tp_int);
			else tp1 = (TYP *)&tp_long;
//#endif
			*node = ep1;
			continue;
		}
		if (tp2->type == bt_pointer && oper) {
			TYP *tpi;
			/* integer + pointer */
			tpi=tp1;
			tp1=tp2;
			tp2=tpi;
			ep3=ep1;
			ep1=ep2;
			ep2=ep3;
		}
		if (tp1->type == bt_pointer) {
			/* pointer +/- integer */
			unsigned int s = tp1->btp->size;// int p;
			if (!integral(tp2))
				error(ERR_INTEGER);
			/*if ((p=pwrof2(s))>=0) {
				if (tp2->type == bt_char || tp2->type == bt_uchar)
					tp2 = cast_op(&ep2, tp2, (TYP *)&tp_short);
				if (p) {
					ep3 = mk_icon((long)p);
					ep3->etype = bt_short;
					ep3->esize = 2;
					ep2 = mk_node(en_lsh, ep3, ep2);
					ep2->etype = bt_short;
					ep2->esize = 2;
				}
				ep1 = mk_node(oper ? en_add : en_sub, ep1, ep2);
				ep1->etype = bt_pointer;
				ep1->esize = 4;
				continue;
			} else {*/
				cast_op(&ep2, tp2, (TYP *)&tp_long);	// g_xmul will restore all this to short if
				ep3 = mk_icon(s);				// necessary :)
				ep3->etype = bt_long;
				ep3->esize = 4;
				ep2 = mk_node(en_mul, ep3, ep2);
				ep2->etype = bt_long;
				ep2->esize = 4;
//				cast_op(&ep2, (TYP *)&tp_short, (TYP *)&tp_long);
				ep1 = mk_node(oper ? en_add : en_sub, ep1, ep2);
				ep1->etype = bt_pointer;
				ep1->esize = 4;
				continue;
			/*}*/
		}
		tp1 = forcefit(&ep1, tp1, &ep2, tp2);
		ep1 = mk_node(oper ? en_add : en_sub, ep1, ep2);
		ep1->etype = tp1->type;
		ep1->esize = tp1->size;
	}
	*node = ep1;
	return tp1;
}

TYP *shiftop(struct enode **node) {
/*
 * shiftop handles the shift operators << and >>.
 */
	struct enode   *ep1, *ep2;
	TYP 		   *tp1, *tp2;
	int 			oper;
	tp1 = addops(&ep1);
	if (tp1 == 0)
		return 0;
	while (lastst == lshift || lastst == rshift) {
		oper = (lastst == lshift);
		getsym();
		tp2 = addops(&ep2);
		if (tp2 == 0)
			error(ERR_IDEXPECT);
		else {
#ifndef MINIMAL_SIZES
			tp1 = forcefit(&ep1, tp1, &ep2, tp2);
#else
			/*
			tp1 = forcefit(&ep1, tp1, &ep2, tp2);	-> this is wrong: C89 specifies the type depends only on the left op.
			cast_op(&ep2, tp2, (TYP *)&tp_char);
			*/
			if (tp1->type == bt_char || tp1->type == bt_uchar)
				tp1 = cast_op(&ep1, tp1, (TYP *)&tp_short);
			cast_op(&ep2, tp2, (TYP *)&tp_char);
#endif
			ep1 = mk_node(oper ? en_lsh : en_rsh, ep1, ep2);
			ep1->etype = tp1->type;
			ep1->esize = tp1->size;
			if (!integral(tp1))
				error(ERR_INTEGER);
		}
	}
	*node = ep1;
	return tp1;
}

TYP *relation(struct enode **node) {
/*
 * relation handles the relational operators < <= > and >=.
 */
	struct enode   *ep1, *ep2;
	TYP 		   *tp1, *tp2;
	enum(e_node) 	nt;
	tp1 = shiftop(&ep1);
	if (tp1 == 0)
		return 0;
	for (;;) {
		switch (lastst) {
		  case lt:
			nt = en_lt;
			break;
		  case gt:
			nt = en_gt;
			break;
		  case leq:
			nt = en_le;
			break;
		  case geq:
			nt = en_ge;
			break;
		  default:
			goto fini;
		}
		getsym();
		tp2 = shiftop(&ep2);
		if (tp2 == 0)
			error(ERR_EXPREXPECT);
		else {
			tp1 = forceft2(&ep1, tp1, &ep2, tp2);
			if (!iscomparable(tp1))
				uerr(ERR_ILLTYPE);
			ep1 = mk_node(nt, ep1, ep2);
			tp1 = (TYP *)&tp_int;
			ep1->etype = tp_int.type;
			ep1->esize = tp_int.size;
			if (lastst>=lt && lastst<=geq) {
				if (flags & X_COMP_STRING) {
					uwarn("not implemented\n");
				} else uwarn("() suggested to clarify priority");
			}
		}
	}
fini:*node = ep1;
	return tp1;
}

TYP *equalops(struct enode **node) {
/*
 * equalops handles the equality and inequality operators.
 */
	struct enode   *ep1, *ep2;
	TYP 		   *tp1, *tp2;
	int 			oper;
	tp1 = relation(&ep1);
	if (tp1 == 0)
		return 0;
	while (lastst == eq || lastst == neq) {
		oper = (lastst == eq);
		getsym();
		tp2 = relation(&ep2);
		if (tp2 == 0)
			error(ERR_IDEXPECT);
		else {
			tp1 = forceft2(&ep1, tp1, &ep2, tp2);
			ep1 = mk_node(oper ? en_eq : en_ne, ep1, ep2);
			tp1 = (TYP *)&tp_int;
			ep1->etype = tp_int.type;
			ep1->esize = tp_int.size;
		}
	}
	*node = ep1;
	return tp1;
}

TYP *binop(struct enode **node, TYP *(*xfunc)(), enum(e_node) nt, enum (e_sym) sy) {
/*
 * binop is a common routine to handle all of the legwork and error checking
 * for bitand, bitor, bitxor
 */
	struct enode   *ep1, *ep2;
	TYP 		   *tp1, *tp2;
	tp1 = (*xfunc) (&ep1);
	if (tp1 == 0)
		return 0;
	while (lastst == sy) {
		getsym();
		tp2 = (*xfunc) (&ep2);
		if (tp2 == 0)
			error(ERR_IDEXPECT);
		else {
			tp1 = forceft2(&ep1, tp1, &ep2, tp2);
			ep1 = mk_node(nt, ep1, ep2);
			ep1->etype = tp1->type;
			ep1->esize = tp1->size;
			if (!integral(tp1))
				error(ERR_INTEGER);
		}
	}
	*node = ep1;
	return tp1;
}

TYP *binlog(struct enode **node, TYP *(*xfunc)(), enum(e_node) nt, enum(e_sym) sy) {
/*
 * binlog is a common routine to handle all of the legwork and error checking
 * for logical and, or
 */
	struct enode   *ep1, *ep2;
	TYP 		   *tp1, *tp2;
	tp1 = (*xfunc) (&ep1);
	if (tp1 == 0)
		return 0;
	while (lastst == sy) {
		getsym();
		tp2 = (*xfunc) (&ep2);
		if (tp2 == 0)
			error(ERR_IDEXPECT);
		else {
			ep1 = mk_node(nt, ep1, ep2);
			tp1 = (TYP *)&tp_int;
			ep1->etype = tp_int.type;
			ep1->esize = tp_int.size;
		}
	}
	*node = ep1;
	return tp1;
}

TYP *bitand(struct enode **node) {
/*
 * the bitwise and operator...
 */
	return binop(node, equalops, en_and, and);
}

TYP *bitxor(struct enode **node) {
	return binop(node, bitand, en_xor, uparrow);
}

TYP *bitor(struct enode **node) {
	return binop(node, bitxor, en_or, or);
}

TYP *andop(struct enode **node) {
	return binlog(node, bitor, en_land, land);
}

TYP *orop(struct enode **node) {
	return binlog(node, andop, en_lor, lor);
}

TYP *conditional(struct enode **node) {
/*
 * this routine processes the hook operator.
 */
	TYP 		   *tp1, *tp2, *tp3;
	struct enode   *ep1, *ep2, *ep3;
	tp1 = orop(&ep1);			/* get condition */
	if (tp1 == 0)
		return 0;
	if (lastst == hook) {
		getsym();
		if (lastst==colon) {
			ep2=NULL; tp2=tp1;
		} else if ((tp2 = expression(&ep2)) == 0) {
			error(ERR_IDEXPECT);
			return 0;
		}
		needpunc(colon);
		if ((tp3 = exprnc(&ep3)) == 0) {
			error(ERR_IDEXPECT);
			return 0;
		}
/*
 * If either type is void and the other is not, cast the other one to void.
 * I dare not doing this in forceft2
 * Strict ANSI does not allow that only one part of the sentence is void,
 * that is what gcc -pedantic tells me.
 * But since such conditionals occur even in GNU Software (look at obstack.h),
 * I allow such constructs here.
 */
		if (tp2->type == bt_void && tp3->type != bt_void)
			tp3 = cast_op(&ep3, tp3, tp2);
		else if (tp3->type == bt_void && tp2->type != bt_void && ep2!=NULL)
			tp2 = cast_op(&ep2, tp2, tp3);

		if (ep2==NULL)
			tp1 = forceft2(&ep1, tp1, &ep3, tp3);
		else tp1 = forceft2(&ep2, tp2, &ep3, tp3);
		if (tp1 == 0)
			return 0;
		ep2 = mk_node(en_void, ep2, ep3);
		ep1 = mk_node(en_cond, ep1, ep2);
		ep1->etype = tp1->type;
		ep1->esize = tp1->size;
	}
	*node = ep1;
	return tp1;
}

TYP *asnop(struct enode **node) {
/*
 * asnop handles the assignment operators.
 */
	struct enode   *ep1, *ep2, *ep3;
	TYP 		   *tp1, *tp2, *tp0;
	enum(e_node) 	op;
	tp0 = NULL;
	tp1 = conditional(&ep1);
	if (tp1 == 0)
		return 0;
	for (;;) {
		switch (lastst) {
		  case assign:
			op = en_assign;
	ascomm:	getsym();
			tp2 = asnop(&ep2);
	ascomm2:
			if (tp2 == 0)
				break;
			if (ep1->nodetype != en_ref && ep1->nodetype != en_fieldref) {
				if (ep1->nodetype==en_cast) {	/* CAUTION : works well only with en_assign */
					enum(e_node) nt;
					if ((nt=ep1->v.p[0]->nodetype) != en_ref && nt != en_fieldref)
						error(ERR_LVALUE);
					tp2 = cast_op(&ep2, tp2, tp1);
					tp1 = (TYP *)ep1->v.p[1];
/*				} else if (ep1->nodetype==en_cond) {
					struct enode *epvoid=ep1->v.p[1];
					
					tp1 = cast_op(&epvoid->v.p[0], tp2, tp1);
					tp1 = cast_op(&epvoid->v.p[1], tp2, tp1);*/
				} else error(ERR_LVALUE);
			}
			if (tp1->const_flag)
				uwarn("assignment of read-only lvalue");
			if (tp0 != 0) tp1 = force_cast_op(&ep2, tp2, tp1);
			else tp1 = cast_op(&ep2, tp2, tp1);
			if (tp1 == 0)
				break;
			if (op!=en_assign && !iscomparable(tp1))
				uerr(ERR_ILLTYPE);
			ep1 = mk_node(op, ep1, ep2);
			ep1->etype = tp1->type;
			ep1->esize = tp1->size;
			if (tp0 != 0) tp1 = cast_op(&ep1, tp1, tp0);
#ifdef INTEL_386
			/* trap struct assigns */
			if (isaggregate(tp1))
				uses_structassign=1;
#endif
			break;
		  case asplus:
			op = en_asadd;
	ascomm3:
			getsym();
			tp2 = asnop(&ep2);
			if (tp2 == 0)
				break;
			if (tp1->type == bt_pointer && integral(tp2)) {
#ifdef NO_SECURE_POINTERS
				if (tp1->btp->size!=1) {
					cast_op(&ep2, tp2, (TYP *)&tp_short);
					ep3 = mk_icon((long) tp1->btp->size);
					ep3->etype = bt_short;
					ep3->esize = 2;
					ep2 = mk_node(en_mul, ep2, ep3);
					ep2->etype = bt_short;
					ep2->esize = 2;
					tp2 = cast_op(&ep2, (TYP *)&tp_short, (TYP *)&tp_long);
				} else
					tp2 = cast_op(&ep2, tp2, (TYP *)&tp_long);
				tp2 = cast_op(&ep2, (TYP *)&tp_long, tp1);
#else
				cast_op(&ep2, tp2, (TYP *)&tp_long);
				ep3 = mk_icon((long) tp1->btp->size);
				ep3->etype = bt_long;
				ep3->esize = 4;
				ep2 = mk_node(en_mul, ep2, ep3);
				ep2->etype = bt_long;
				ep2->esize = 4;
				tp2 = cast_op(&ep2, (TYP *)&tp_long, tp1);
#endif
			}
			goto ascomm2;
		  case asminus:
			op = en_assub;
			goto ascomm3;
		  case astimes:
			op = en_asmul;
			goto ascomm;
		  case asdivide:
			op = en_asdiv;
			goto ascomm;
		  case asmodop:
			op = en_asmod;
			goto ascomm;
		  case aslshift:
			op = en_aslsh;
			goto ascomm;
		  case asrshift:
			op = en_asrsh;
			goto ascomm;
		  case asand:
			op = en_asand;
			goto ascomm;
		  case asor:
			op = en_asor;
			goto ascomm;
		  case asuparrow:
			op = en_asxor;
			goto ascomm;
		  default:
			goto asexit;
		}
	}
asexit:
   *node = ep1;
	return tp1;
}

TYP *exprnc(struct enode **node) {
/*
 * evaluate an expression where the comma operator is not legal.
 */
	TYP *tp;
	tp = asnop(node);
	if (tp == 0)
		*node = 0;
	return tp;
}

TYP *commaop(struct enode **node) {
/*
 * evaluate the comma operator. comma operators are kept as void nodes.
 */
	TYP 		   *tp1;
	struct enode   *ep1, *ep2;
	tp1 = asnop(&ep1);
	if (tp1 == 0)
		return 0;
	if (lastst == comma) {
		getsym();
		tp1 = commaop(&ep2);
		if (tp1 == 0) {
			error(ERR_IDEXPECT);
			goto coexit;
		}
		ep1 = mk_node(en_void, ep1, ep2);
		ep1->esize = ep2->esize;
		ep1->etype = ep2->etype;
	}
coexit:*node = ep1;
	return tp1;
}

TYP *expression(struct enode **node) {
/*
 * evaluate an expression where all operators are legal.
 */
	TYP 		   *tp;
	tp = commaop(node);
	if (tp == 0)
		*node = 0;
	return tp;
}

int cast_ok(TYP *tp1, TYP *tp2, int need_physically_compatible) {
/*
 * This is used to tell whether an implicit cast will generate a warning
 *
 * If need_physically_compatible is 0, then we do not make a difference between
 * short[123], short[124], short[0] and short* (otherwise we do because they're
 * not stored in memory the same way).
 */
	if (tp1 == 0 || tp2 == 0)
		return 0;

	if (tp1 == tp2 || tp2->type == bt_void)
		return 1;
	if (bt_integral(tp1->type))
		return (tp1->type^tp2->type)<=1;

	if (tp1->type != tp2->type)
		return 0;

	/* from now on, tp1->type==tp2->type */
	if (tp1->type == bt_pointer) {
		if (need_physically_compatible)
			if (tp1->val_flag!=tp2->val_flag ||
				(tp1->val_flag && tp1->size!=tp2->size))
			return 0;
		return tp1->btp->type == bt_void || cast_ok(tp1->btp, tp2->btp, 1);
	}
	if (tp1->type == bt_func) {
		SYM *sp=tp1->lst.head,*sp2=tp2->lst.head;
		if (!sp || !sp2)	// one is only a prototype : OK
			return eq_type(tp1->btp, tp2->btp);
		while (sp && sp2) {
			if (!eq_type(sp->tp,sp2->tp)) return 0;
			sp=sp->next; sp2=sp2->next;
		}
		if (sp || sp2) return 0;	// != # of args
		return eq_type(tp1->btp, tp2->btp);
	}
	if (tp1->type == bt_struct || tp1->type == bt_union)
		return (tp1->lst.head == tp2->lst.head);

	return 1;
}

TYP *cast_op(struct enode **ep, TYP *tp1, TYP *tp2) {
	struct enode   *ep2;

	if (tp1 == 0 || tp2 == 0 || (tp1->type == bt_void && tp2->type != bt_void)) {
		uerr(ERR_CAST,type_str(tp1),type_str(tp2));
		return 0;
	}
	if (tp1->type == tp2->type) {
		if (!cast_ok(tp1, tp2, 0))
			uwarn("conversion between incompatible %stypes",
					(tp1->type == bt_pointer) ? "pointer " : "");
		if (tp1->type == bt_struct || tp1->type == bt_union) {
			if (tp1->size != tp2->size)
				uerr(ERR_CAST,type_str(tp1),type_str(tp2));
		}
#ifdef NO_VERYTRIVIAL_CASTS
		return tp2;
#endif
	}
	opt0(ep);					/* to make a constant really a constant */

	if ((*ep)->nodetype == en_icon) {
		if (integral(tp2) || tp2->type == bt_pointer || tp2->type == bt_void) {
			long			j = (*ep)->v.i;
			long d;
			(*ep)->etype = tp2->type;
			(*ep)->esize = tp2->size;
			/*
			 * The cast may affect the value of (*ep)->v.i
			 */
			(*ep)->v.i = strip_icon(j, tp2->type);
			d = j ^ (*ep)->v.i;
			if (d && d!=0xFFFF0000 && d!=0xFFFFFF00)
				uwarn("cast changed integer constant 0x%08lx to 0x%08lx", j, (*ep)->v.i);
			return tp2;
#ifndef NOFLOAT
		} else if (tp2->type == bt_float || tp2->type == bt_double) {
#ifdef XCON_DOESNT_SUPPORT_FLOATS
#error Fix me now :)
#endif
			(*ep)->nodetype = en_fcon;
			(*ep)->etype = tp2->type;
#ifdef PC
			(*ep)->v.f = (double) (*ep)->v.i;
#else
#ifdef BCDFLT
			(*ep)->v.f = ffpltof((*ep)->v.i);
#else
			(*ep)->v.f = (float) (*ep)->v.i;
#endif
#endif
			(*ep)->esize =
#if defined(DOUBLE) || defined(INTEL_386)
				tp2->type == bt_float ? tp_float.size : tp_double.size;
#else
				float_size;
#endif
			return tp2;
#endif
		} else {
 			uerr(ERR_CAST,type_str(tp1),type_str(tp2));
			return 0;
		}
	}
/*	if (tp2->type != bt_void && tp1->size > tp2->size)
		uwarn("cast to a narrower type loses accuracy");*/
	if (tp1->type == bt_pointer && tp2->type != bt_pointer)
		uwarn("cast from pointer to integer is dangerous");
	if (tp2->type == bt_pointer && tp1->size < 4)
		uwarn("cast from short to pointer is dangerous");
	if ((tp1->type == bt_func && tp2->type != bt_func)
			|| (tp2->type == bt_func && tp1->type != bt_func))
		uwarn("implicit cast from '%s' to '%s'",type_str(tp1),type_str(tp2));
	if ((bt_uncastable(tp1->type) || bt_uncastable(tp2->type)) && tp1->type!=tp2->type)
		uerr(ERR_CAST,type_str(tp1),type_str(tp2));

#ifdef NO_TRIVIAL_CASTS
	if (tp2->size==tp1->size && (tp2->type==tp1->type
			|| ((tp2->type^tp1->type)==1 && bt_integral(tp1->type))
			|| (tp2->type==bt_pointer && (tp1->type==bt_long || tp1->type==bt_ulong))
			|| (tp1->type==bt_pointer && (tp2->type==bt_long || tp2->type==bt_ulong)))) {
		if (needs_trivial_casts((*ep)->nodetype))
			goto trivial_cast_do;
		(*ep)->etype = tp2->type;
		return tp2;
	}
trivial_cast_do:
#endif

	ep2 = mk_node(en_cast, *ep, NIL_ENODE);
	ep2->etype = tp2->type;
	ep2->esize = tp2->size;

	*ep = ep2;

	return tp2;
}

TYP *force_cast_op(struct enode **ep, TYP *tp1, TYP *tp2) {
	struct enode   *ep2;

	if (tp1 == 0 || tp2 == 0 || (tp1->type == bt_void && tp2->type != bt_void)) {
		uerr(ERR_CAST,type_str(tp1),type_str(tp2));
		return 0;
	}
	if (tp1->type == tp2->type) {
		if (tp1->type == bt_struct || tp1->type == bt_union) {
			if (tp1->size != tp2->size)
				uerr(ERR_CAST,type_str(tp1),type_str(tp2));
		}
//		return tp2;	so that an en_cast enode is generated & the old TYP is saved
#ifdef NO_VERYTRIVIAL_CASTS
		return tp2;
#endif
	}
	opt0(ep);					/* to make a constant really a constant */

	if ((*ep)->nodetype == en_icon) {
		if (integral(tp2) || tp2->type == bt_pointer || tp2->type == bt_void) {
			long			j = (*ep)->v.i;
			(*ep)->etype = tp2->type;
			(*ep)->esize = tp2->size;
			/*
			 * The cast may affect the value of (*ep)->v.i
			 */
			(*ep)->v.i = strip_icon(j, tp2->type);
			return tp2;
#ifndef NOFLOAT
		} else if (tp2->type == bt_float || tp2->type == bt_double) {
#ifdef XCON_DOESNT_SUPPORT_FLOATS
#error Fix me now :)
#endif
			(*ep)->nodetype = en_fcon;
			(*ep)->etype = tp2->type;
#ifdef PC
			(*ep)->v.f = (double) (*ep)->v.i;
#else
			(*ep)->v.f = ffpltof((*ep)->v.i);
#endif
			(*ep)->esize =
#if defined(DOUBLE) || defined(INTEL_386)
				tp2->type == bt_float ? tp_float.size : tp_double.size;
#else
				float_size;
#endif
			return tp2;
#endif
		} else {
			uerr(ERR_CAST,type_str(tp1),type_str(tp2));
			return 0;
		}
	}
	if ((bt_uncastable(tp1->type) || bt_uncastable(tp2->type)) && tp1->type!=tp2->type)
		uerr(ERR_CAST,type_str(tp1),type_str(tp2));
/*	if (tp2->type == bt_pointer && tp1->size < 4)
		uwarn("cast from short to pointer is dangerous");*/

#ifdef NO_TRIVIAL_CASTS
	if (tp2->size==tp1->size && (tp2->type==tp1->type
			|| ((tp2->type^tp1->type)==1 && bt_integral(tp1->type))
			|| (tp2->type==bt_pointer && (tp1->type==bt_long || tp1->type==bt_ulong))
			|| (tp1->type==bt_pointer && (tp2->type==bt_long || tp2->type==bt_ulong)))) {
		if (needs_trivial_casts((*ep)->nodetype))
			goto trivial_cast_do;
		(*ep)->etype = tp2->type;
		return tp2;
	}
trivial_cast_do:
#endif

	ep2 = mk_node(en_cast, *ep, (struct enode *)tp1);	// the latter is for (int)x=value;
	ep2->etype = tp2->type;
	ep2->esize = tp2->size;

	*ep = ep2;

	return tp2;
}

#ifndef isscalar
int integral(TYP *tp) {
/* returns true it tp is an integral type */
	return bt_integral(tp->type);
}
#endif

long strip_icon(long i, enum(e_bt) type) {
/*
 * This function handles the adjustment of integer constants upon
 * casts. It forces the constant into the range acceptable for
 * the given type.
 * This code assumes somehow that longs are 32 bit on the
 * machine that runs the compiler, but how do you get this
 * machine independent?
 */
	switch (type) {
	  case bt_uchar:			/* 0 .. 255 */
#ifdef PC
		i &= 0xff;
#else
		i = (long)((unsigned char)i);
#endif
		break;
	  case bt_char: 			/* -128 .. 127 */
#ifdef PC
		i &= 0xff;
		if (i >= 128)
			i |= 0xffffff00;
#else
		i = (long)((char)i);
#endif
		break;
	  case bt_ushort:			/* 0 .. 65535 */
#ifdef PC
		i &= 0xffff;
#else
		i = (long)((unsigned short)i);
#endif
		break;
	  case bt_short:			/* -32768 .. 32767 */
#ifdef PC
		i &= 0xffff;
		if (i >= 32768)
			i |= 0xffff0000;
#else
		i = (long)((short)i);
#endif
		break;
	}
	return i;
}
// vim:ts=4:sw=4
