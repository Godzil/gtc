/*
 * GTools C compiler
 * =================
 * source file :
 * statement handling
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
#ifdef PC
#ifdef SHORT_INT
#undef int
#endif
#include	<ctype.h>
#ifdef SHORT_INT
#define int short
#endif
#endif

struct enode   *init_node CGLOB;

TYP *lastexpr_tp CGLOB;
int lastexpr_size CGLOB, lastexpr_type CGLOB;

/*
 * the statement module handles all of the possible c statements and builds a
 * parse tree of the statements.
 *
 * each routine returns a pointer to a statement parse node which reflects the
 * statement just parsed.
 */


int start_block(int m) {	// used in block() (func.c)
	if (m) {
		needpunc(begin);
		return 1; // we don't care about the result in this case
	} else
		return lastst==begin;
}

unsigned int getconst(enum(e_sym) s,enum(e_sym) e) {
	struct enode *en;
	getsym();
	if (lastst != s)
		error(ERR_EXPREXPECT);
	else {
		getsym();
		if (exprnc(&en) == 0)
			error(ERR_EXPREXPECT);
		needpunc(e);
		opt0(&en);
		if (en->nodetype != en_icon)
			error(ERR_CONSTEXPECT);
		else if ((unsigned long)en->v.i>=65536)
			error(ERR_OUTRANGE);
		else return en->v.i;
	}
	return 0; // make the compiler happy
}
unsigned int getconst2(enum(e_sym) e) {
	struct enode *en;
	if (exprnc(&en) == 0)
		error(ERR_EXPREXPECT);
	needpunc(e);
	opt0(&en);
	if (en->nodetype != en_icon)
		error(ERR_CONSTEXPECT);
	else if ((unsigned long)en->v.i>=65536)
		error(ERR_OUTRANGE);
	else return en->v.i;
	return 0; // make the compiler happy
}

struct snode *whilestmt(void) {
/*
 * whilestmt parses the c while statement.
 */
	struct snode   *snp;
	snp = (struct snode *) xalloc((int) sizeof(struct snode), SNODE);
	snp->stype = st_while;
	snp->count = 3;
	getsym();
	if (lastst != openpa)
		error(ERR_EXPREXPECT);
	else {
		getsym();
		if (expression(&(snp->exp)) == 0)
			error(ERR_EXPREXPECT);
#ifdef DB_POSSIBLE
		snp->line=lineid;
#endif
		needpunc(closepa);
		if (lastst == kw_count) snp->count=getconst(openpa,closepa);
		snp->s1 = statement();
	}
	return snp;
}

struct snode *asmstmt();
#if !defined(AS) && !defined(ASM)
struct snode *asmstmt(void) {
/*
 * asmstmt parses the gtc c asm statement.
 */
	struct snode   *snp;
	snp = (struct snode *) xalloc((int) sizeof(struct snode), SNODE);
	snp->stype = st_asm;
	snp->count = 1;
	getsym();
	if (lastst != openpa)
		error(ERR_EXPREXPECT);
	else {
		getsym();
		if (lastst != sconst)
			error(ERR_SYNTAX);
		snp->v1.i = (long)strsave(laststr);
		getsym();
		if (lastst == colon) {
			getsym();
			uwarn("only asm(\"...\") is supported yet");
			error(ERR_SYNTAX);
		}
		needpunc(closepa);
		needpunc(semicolon);
	}
#ifdef DB_POSSIBLE
	snp->line=lineid;
#endif
	return snp;
}
#endif

//struct enode lp_one = { en_icon, bt_ushort, 2, {1L}, 0, 0};
xstatic struct enode *lp_one CGLOB;
struct snode *loopstmt(void) {
/*
 * loopstmt parses the gtc c loop statement.
 */
	struct snode *snp; TYP *tp;
	struct enode *exp;
	int has_count;
	snp = (struct snode *) xalloc((int) sizeof(struct snode), SNODE);
	snp->stype = st_loop;
	snp->count = 3;
	has_count = 0;
	getsym();
	if (lastst != openpa)
		error(ERR_EXPREXPECT);
	else {
		getsym();
		if (lastst != id)
			error(ERR_IDEXPECT);
		if (!nameref(&(snp->v1.e))) error(ERR_EXPREXPECT);
		if (lastst != assign)
			error(ERR_SYNTAX);
		getsym();
		if (!(tp=expression(&(snp->exp))))
			error(ERR_EXPREXPECT);
		cast_op(&(snp->exp),tp,(TYP *)&tp_ushort);
#ifdef DB_POSSIBLE
		snp->line=lineid;
#endif
		needpunc(closepa);
		if (lastst == kw_count) { snp->count=getconst(openpa,closepa); has_count=1; }
		opt0(&(snp->exp));
		if (snp->exp->nodetype==en_icon) {
			unsigned int c=snp->exp->v.i;
			if (has_count) {
				if (c!=snp->count)
					uwarn("loop has an effective count of %u whereas %u precised",
						c,snp->count);
			} else snp->count=c;
		}
		lp_one = mk_icon(1L);
		lp_one->etype = bt_ushort; lp_one->esize = 2;
		snp->exp = exp = mk_node(en_sub,snp->exp,lp_one);
		exp->etype = bt_ushort; exp->esize = 2;
		opt4(&(snp->exp));
		snp->exp = exp = mk_node(en_assign,snp->v1.e,snp->exp);
		exp->etype = bt_ushort; exp->esize = 2;
		snp->s1 = statement();
		if (lastst == kw_until) {
			getsym();
			if (!expression(&(snp->v2.e))) error(ERR_EXPREXPECT);
		} else snp->v2.e=NULL;
	}
	return snp;
}

struct snode *dostmt(void) {
/*
 * dostmt parses the c do-while construct.
 */
	struct snode   *snp;
	snp = (struct snode *) xalloc((int) sizeof(struct snode), SNODE);
	snp->stype = st_do;
	snp->count = 3;
	getsym();
	snp->s1 = statement();
#ifdef DB_POSSIBLE
	snp->line=lineid;
#endif
	if (lastst != kw_while)
		error(ERR_WHILEXPECT);
	else {
		getsym();
		if (lastst != openpa)
			error(ERR_EXPREXPECT);
		else {
			getsym();
			if (expression(&(snp->exp)) == 0)
				error(ERR_EXPREXPECT);
			needpunc(closepa);
			if (lastst == kw_count) snp->count=getconst(openpa,closepa);
		}
		if (lastst != end)
			needpunc(semicolon);
	}
	return snp;
}

struct snode *forstmt(void) {
	struct snode   *snp;
	snp = (struct snode *) xalloc((int) sizeof(struct snode), SNODE);
	snp->stype = st_for;
	snp->count = 3;
	getsym();
	needpunc(openpa);
	/*if (*/expression(&(snp->exp))/* == 0)
		snp->exp = 0*/;
	needpunc(semicolon);
	/*if (*/expression(&(snp->v1.e))/* == 0)
		snp->v1.e = 0*/;
	needpunc(semicolon);
	/*if (*/expression(&(snp->v2.e))/* == 0)
		snp->v2.e = 0*/;
#ifdef DB_POSSIBLE
	snp->line=lineid;
#endif
	needpunc(closepa);
	if (lastst == kw_count) snp->count=getconst(openpa,closepa);
	snp->s1 = statement();
	return snp;
}

struct snode *ifstmt(void) {
/*
 * ifstmt parses the c if statement and an else clause if one is present.
 */
	struct snode   *snp;
	snp = (struct snode *) xalloc((int) sizeof(struct snode), SNODE);
	snp->stype = st_if;
	snp->count=32768;
	getsym();
	if (lastst != openpa)
		error(ERR_EXPREXPECT);
	else {
		getsym();
		if (expression(&(snp->exp)) == 0)
			error(ERR_EXPREXPECT);
#ifdef DB_POSSIBLE
		snp->line=lineid;
#endif
		needpunc(closepa);
		if (lastst == kw_count) {
			unsigned int i=getconst(openpa,comma),j=getconst2(closepa);
			snp->count=((unsigned long)i<<16)/(i+j);
		}
		snp->s1 = statement();
		if (lastst == kw_else) {
			getsym();
			snp->v1.s = statement();
		} else {
#ifdef NO_CALLOC
			snp->v1.s = 0;
#endif
		}
	}
	return snp;
}

/*
 * consider the following piece of code:
 *
 *		switch (i) {
 *				case 1:
 *						if (j) {
 *								.....
 *						} else
 *				case 2:
 *						....
 *		}
 *
 * case statements may be deep inside, so we need a global variable
 * last_case to link them
 */
xstatic struct snode *last_case CGLOB; /* last case statement within this switch */

struct snode *casestmt(void) {
/*
 * cases are returned as seperate statements. for normal cases label is the
 * case value and v1.i is zero. for the default case v1.i is nonzero.
 */
	struct snode   *snp,*snp0;
	snp0 = snp = (struct snode *) xalloc((int) sizeof(struct snode), SNODE);
#ifdef NO_CALLOC
	snp->s1 = NIL_SNODE;
#endif
	if (lastst == kw_case) {
		long v;
		getsym();
		snp->stype = st_case;
		snp->v2.i = v = intexpr();
		if (lastst == dots) {	// TODO : make only one 'case' label
			long max;
			getsym();
			if ((max = intexpr()) < v) error(ERR_CASERANGE);
			else while (v != max) {
				last_case = last_case->s1 = snp;
				snp = (struct snode *) xalloc((int) sizeof(struct snode), SNODE);
				last_case->v1.s = snp;
#ifdef NO_CALLOC
				snp->s1 = NIL_SNODE;
#endif
				snp->stype = st_case;
				snp->v2.i = ++v;
			}
		}
	} else {
		/* lastst is kw_default */
		getsym();
		/* CVW: statement type needed for analyze etc. */
		snp->stype = st_default;
	}
	last_case = last_case->s1 = snp;
	needpunc(colon);
	if (lastst != end)
		snp->v1.s = statement();
	return snp0;
}

int checkcases(struct snode *head) {
/*
 * checkcases will check to see if any duplicate cases exist in the case list
 * pointed to by head.
 */
	struct snode   *top, *cur;
	cur = top = head;
	while (top != 0) {
		cur = top->s1;
		while (cur != 0) {
			if (cur->stype != st_default && top->stype != st_default
				&& cur->v2.i == top->v2.i) {
				uwarn("duplicate case label for value %ld", cur->v2.i);
				return 1;
			}
			if (cur->stype == st_default && top->stype == st_default) {
				uwarn("duplicate default label");
				return 1;
			}
			cur = cur->s1;
		}
		top = top->s1;
	}
	return 0;
}

struct snode *switchstmt(void) {
	struct snode   *snp;
	struct snode   *local_last_case;
	local_last_case = last_case;
	snp = (struct snode *) xalloc((int) sizeof(struct snode), SNODE);
	last_case = snp;
#ifdef NO_CALLOC
	snp->s1 = 0;
#endif
	snp->stype = st_switch;
	getsym();
	needpunc(openpa);
	if ((expression(&(snp->exp))) == 0)
		error(ERR_EXPREXPECT);
#ifdef DB_POSSIBLE
	snp->line=lineid;
#endif
	needpunc(closepa);
	needpunc(begin);
	snp->v1.s = compound(1);
	if (checkcases(snp->s1))
		error(ERR_DUPCASE);
	last_case = local_last_case;
	return snp;
}

struct snode *retstmt(void) {
	struct snode   *snp;
	TYP 		   *tp;
	snp = (struct snode *) xalloc((int) sizeof(struct snode), SNODE);
	snp->stype = st_return;
	getsym();
	tp = expression(&(snp->exp));
	if (snp->exp != 0)
		(void) cast_op(&(snp->exp), tp, ret_type);
	if (lastst != end)
		needpunc(semicolon);
#ifdef DB_POSSIBLE
	snp->line=prevlineid;
#endif
	return snp;
}

struct snode *breakstmt(void) {
	struct snode   *snp;
	snp = (struct snode *) xalloc((int) sizeof(struct snode), SNODE);
	snp->stype = st_break;
	getsym();
	if (lastst != end)
		needpunc(semicolon);
#ifdef DB_POSSIBLE
	snp->line=prevlineid;
#endif
	return snp;
}

struct snode *contstmt(void) {
	struct snode   *snp;
	snp = (struct snode *) xalloc((int) sizeof(struct snode), SNODE);
	snp->stype = st_continue;
	getsym();
	if (lastst != end)
		needpunc(semicolon);
#ifdef DB_POSSIBLE
	snp->line=prevlineid;
#endif
	return snp;
}

struct snode *exprstmt(void) {
/*
 * exprstmt is called whenever a statement does not begin with a keyword. the
 * statement should be an expression.
 */
	struct snode   *snp;
	debug('u');
	snp = (struct snode *) xalloc((int) sizeof(struct snode), SNODE);
	snp->stype = st_expr;
/*
 * I have a problem here.
 * If expression() fails on the first character and does not do a getsym(),
 * there may be an infinite loop since we will continue coming up here.
 * Since the compiler will stop after MAX_ERROR_COUNT calls to error(),
 * this might not be THAT much of a problem.
 */
	if (!(lastexpr_tp=expression(&(snp->exp))))
		error(ERR_EXPREXPECT);
	else {
		lastexpr_type=snp->exp->etype;
		lastexpr_size=snp->exp->esize;
	}
	debug('v');
	if (lastst != end)
		needpunc(semicolon);
#ifdef DB_POSSIBLE
	snp->line=prevlineid;
#endif
	debug('w');
	return snp;
}

#ifdef AUTOINIT_PAD
//long auto_init(long offs,TYP *typ,int brace_level,int offmod);
long auto_pad(long offs,int len,int offmod) {
	long nbytes=0;
	while (len>0) {
		int size;
		if ((offs+offmod)&1)
			size=1;
		else if (len<=2)
			size=len;
		else if (len==3)
			size=2;
		else size=4;

		{
		struct enode *ep1,*ep2=mk_icon(0L),*ep3;
		ep2->esize = size;	/* don't care about the etype, it's set to char */

		ep1 = mk_node(en_autocon, NIL_ENODE, NIL_ENODE);
		ep1->v.i = offs;
		ep1->etype = bt_pointer;
		ep1->esize = 4;

		ep3 = mk_icon((long)offmod);
		ep3->etype = bt_long;
		ep3->esize = 4;

		ep1 = mk_node(en_add, ep1, ep3);
		ep1->etype = bt_pointer;
		ep1->esize = 4;

		ep1 = mk_node(en_ref, ep1, NIL_ENODE);
		ep1->esize = size;	/* don't care about the etype, it's set to char */

		ep1 = mk_node(en_assign, ep1, ep2);
		ep1->esize = size;	/* don't care about the etype, it's set to char */

		if (init_node == 0) {
			init_node = ep1;
		} else {
			init_node = mk_node(en_void, init_node, ep1);
		}
		}

		len-=size;
		nbytes+=size;
		offmod+=size;
	}
	return nbytes;
}
#endif

long auto_init(long offs,TYP *typ,TYP **tpp,int brace_level,int offmod,int stroff) {
/*
 * generated assignment statements for initialization of auto and register
 * variables. The initialization is generated like comma operators so a
 * single statement does all the initializations
 */
	struct enode   *ep1, *ep2;
	struct typ	   *tp;
	int 			brace_seen = 0;
	long			nbytes = 0;

auto_init_restart:
	if (lastst == begin) {
		brace_level++;
		brace_seen++;
		getsym();
	}
	if (typ->type==bt_struct && brace_level) {
		struct sym	   *sp;
		sp = typ->lst.head;			/* start at top of symbol table */
		while (sp != 0) {
/*			infunc("DrawPacmanLogoAndHandleMenu")
				bkpt();*/
			nbytes+=auto_init(offs,sp->tp,NULL,brace_level,(int)(offmod+sp->value.i),-1);
			if (lastst == comma)
				getsym();
			if (lastst == end || lastst == semicolon)
				break;
			sp = sp->next;
		}
#ifdef AUTOINIT_PAD
		nbytes+=auto_pad(offs,(int)(typ->size-nbytes),offmod+nbytes);	/* negative args are OK for auto_pad */
#endif
	} else if (typ->type==bt_union) {
		typ = (typ->lst.head)->tp;
		goto auto_init_restart;
	} else if (typ->val_flag) {
//		if (!brace_level) error(ERR_SYNTAX);
		if (lastst != end) {
			int stroff=-1;
			if (lastst == sconst && !brace_seen)
				stroff=0;
			while (1) {
				nbytes+=auto_init(offs,typ->btp,NULL,brace_level,(int)(offmod+nbytes),stroff);
				if (stroff>=0) {
					stroff++;
					if (stroff>lstrlen || (stroff==lstrlen && typ->size)) {
						getsym();
						break;
					}
				} else {
					if (lastst == comma)
						getsym();
					if (lastst == end || lastst == semicolon) break;
				}
			}
		}
		if (typ->size && nbytes > typ->size)
			error(ERR_INITSIZE);
		if (nbytes != typ->size && tpp) {
			/* fix the symbol's size, unless tpp=0 (item of a struct/union or array) */
			typ = copy_type(typ);
			*tpp = typ;
			typ->size = nbytes;
		}
#ifdef AUTOINIT_PAD
		nbytes+=auto_pad(offs,(int)(typ->size-nbytes),offmod+nbytes);	/* negative args are OK for auto_pad */
#endif
	} else {
		if (stroff<0) {
		   if (!(tp = exprnc(&ep2)))
			   error(ERR_ILLINIT);
		} else
			ep2=mk_icon(stroff<lstrlen?laststr[stroff]:0), tp=(TYP *)&tp_char;
		(void) cast_op(&ep2, tp, typ);
		nbytes=bt_size(typ->type);

/*		if (offs==0xfffffe4e)
			bkpt();*/

		ep1 = mk_node(en_autocon, NIL_ENODE, NIL_ENODE);
		ep1->v.i = offs;
/*		ep1->etype = typ->type; // this is a ridiculous bug that I spent hours finding...
		ep1->esize = typ->size; */
		ep1->etype = bt_pointer;
		ep1->esize = 4;

		if (offmod) {
			struct enode *ep3 = mk_icon((long)offmod);
			ep3->etype = bt_long;
			ep3->esize = 4;

			ep1 = mk_node(en_add, ep1, ep3);
			ep1->etype = bt_pointer;
			ep1->esize = 4;
		}

		ep1 = mk_node(en_ref, ep1, NIL_ENODE);
		ep1->etype = typ->type;
		ep1->esize = typ->size;

		ep1 = mk_node(en_assign, ep1, ep2);
		ep1->etype = typ->type;
		ep1->esize = typ->size;

#ifdef MID_DECL_IN_EXPR
		if (middle_decl)
			md_expr = ep1, md_type = typ;
			#error fix needed around here...
		else {
#endif
			if (init_node == 0) {
				init_node = ep1;
			} else {
				init_node = mk_node(en_void, init_node, ep1);
			}
#ifdef MID_DECL_IN_EXPR
		}
#endif
	}
	while (brace_seen--)
		needpunc(end);
	return nbytes;
}

struct snode *compound(int no_init) {
/*
 * compound processes a block of statements and forms a linked list of the
 * statements within the block.
 *
 * compound expects the input pointer to already be past the begin symbol of the
 * block.
 *
 * If no_init is true, auto initializations are not desirable
 */
	struct snode   *head, *tail, *snp;
//	struct sym	   *local_tail, *local_tagtail;
	HTABLE			old_lsyms,old_ltags;
//	hashinit(&symtab);
/*	local_tail = lsyms.tail;
	local_tagtail = ltags.tail;*/
	memcpy(&old_lsyms,&lsyms,sizeof(HTABLE));
	memcpy(&old_ltags,&ltags,sizeof(HTABLE));
	dodecl(sc_auto);
	if (init_node == 0) {
		head = tail = 0;
	} else {
		if (no_init>0) {
			uwarn("auto initialization not reached");
		}
		head = tail = (struct snode *) xalloc((int) sizeof(struct snode), SNODE);
		head->stype = st_expr;
		head->exp = init_node;
#ifdef NO_CALLOC
		head->next = 0;
#endif
#ifdef DB_POSSIBLE
		head->line = prevlineid;
#endif
	}
	init_node = 0;
	while (lastst != end) {
		if (head == 0)
			head = tail = statement();
		else {
			tail->next = statement();
			if (tail->next != 0)
				tail = tail->next;
		}
	}
	if (no_init>=0)
		getsym();
#ifdef LISTING
	if (list_option) {
/*		if (local_tail != lsyms.tail) {
			if (local_tail != 0)
				symtab.head = local_tail->next;
			else
				symtab.head = lsyms.head;
			symtab.tail = lsyms.tail;
			fprintf(list, "\n*** local symbol table ***\n\n");
			list_table(&symtab, 0);
			fprintf(list, "\n");
		}*/
		fprintf(list, "\n*** local symbol table ***\n\n");
		list_table(&lsyms, 0);
		fprintf(list, "\n");
/*		if (local_tagtail != ltags.tail) {
			if (local_tagtail != 0)
				symtab.head = local_tagtail->next;
			else
				symtab.head = ltags.head;
			symtab.tail = ltags.tail;
			fprintf(list, "\n*** local structures and unions ***\n\n");
			list_table(&symtab, 0);
			fprintf(list, "\n");
		}*/
		fprintf(list, "\n*** local structures and unions ***\n\n");
		list_table(&ltags, 0);
		fprintf(list, "\n");
	}
#endif
/*	if (local_tagtail != 0) {
		ltags.tail = local_tagtail;
		ltags.tail->next = 0;
	} else {
		ltags.head = 0;
		ltags.tail = 0;
	}


	if (local_tail != 0) {
		lsyms.tail = local_tail;
		lsyms.tail->next = 0;
	} else {
		lsyms.head = 0;
		lsyms.tail = 0;
	}*/
	memcpy(&lsyms,&old_lsyms,sizeof(HTABLE));
	memcpy(&ltags,&old_ltags,sizeof(HTABLE));

	snp = (struct snode *) xalloc((int) sizeof(struct snode), SNODE);
	snp->stype = st_compound;
	snp->s1 = head;
#ifdef DB_POSSIBLE
	snp->line=lineid;
#endif
	return snp;
}

extern unsigned int pos;
struct snode *labelstmt(void) {
/*
 * labelstmt processes a label that appears before a statement as a seperate
 * statement.
 */
	struct snode   *snp;
	struct sym	   *sp;
	snp = (struct snode *) xalloc((int) sizeof(struct snode), SNODE);
	/*if (pos>=0x25C2)
		printf("jiotrh");*/
	snp->stype = st_label;
	if ((sp = search(lastid, lastcrc, &labsyms)) == 0) {
		sp = (struct sym *) xalloc((int) sizeof(struct sym), _SYM);
		sp->name = strsave(lastid);
		sp->storage_class = sc_label;
#ifdef NO_CALLOC
		sp->tp = 0;
#endif
		sp->value.i = nxtlabel();
		append(&sp, &labsyms);
	} else {
		if (sp->storage_class != sc_ulabel)
			error(ERR_LABEL);
		else
			sp->storage_class = sc_label;
	}
	getsym();					/* get past id */
	needpunc(colon);
	if (sp->storage_class == sc_label) {
		snp->v2.i = sp->value.i;
		if (lastst != end)
			snp->s1 = statement();
		return snp;
	}
	return 0;
}

struct snode *gotostmt(void) {
/*
 * gotostmt processes the goto statement and puts undefined labels into the
 * symbol table.
 */
	struct snode   *snp;
	struct sym	   *sp;
	getsym();
	if (lastst != id) {
		error(ERR_IDEXPECT);
		return 0;
	}
	snp = (struct snode *) xalloc((int) sizeof(struct snode), SNODE);
	if ((sp = search(lastid, lastcrc, &labsyms)) == 0) {
		sp = (struct sym *) xalloc((int) sizeof(struct sym), _SYM);
		sp->name = strsave(lastid);
		sp->value.i = nxtlabel();
		sp->storage_class = sc_ulabel;
#ifdef NO_CALLOC
		sp->tp = 0;
#endif
		append(&sp, &labsyms);
	}
#ifdef DB_POSSIBLE
	snp->line=lineid;
#endif
	getsym();					/* get past label name */
	if (lastst != end)
		needpunc(semicolon);
	if (sp->storage_class != sc_label && sp->storage_class != sc_ulabel)
		error(ERR_LABEL);
	else {
		snp->stype = st_goto;
		snp->v2.i = sp->value.i;
		snp->next = 0;
		return snp;
	}
	return 0;
}

struct snode *statement(void) {
/*
 * statement figures out which of the statement processors should be called
 * and transfers control to the proper routine.
 */
	struct snode   *snp;
/*	if (lineid==0x20A)
		bkpt();*/
	switch (lastst) {
	  case semicolon:
		getsym();
		snp = 0;
		break;
	  case kw_char: case kw_short: case kw_unsigned: case kw_long:
	  case kw_struct: case kw_union: case kw_enum: case kw_void:
	  case kw_float: case kw_double: case kw_int: case kw_typeof:
	  case kw_signed: case kw_const: case kw_volatile:
	  case kw_register: case kw_auto:
	  case kw_static: case kw_typedef: case kw_extern:
middle_decl:
		if (!(flags & X_MID_DECL)) goto default_decl;
#ifdef OLD_MID_DECL
		snp = compound(0);
		if ((int)cached_sym IS_VALID) fatal("CACHE"); // will never happen since no caching is
												// performed when lastst==begin
/*		cached_sym2 = cached_sym;*/
		cached_sym = lastst; cached_lineid = lineid;
		lastst = end;
		break;
#else
#ifdef MID_DECL
		/* the following is much cleaner than the old mid_decl handler */
		dodecl(sc_auto);
		if (init_node) {
			snp = (struct snode *) xalloc((int) sizeof(struct snode), SNODE);
			snp->stype = st_expr;
			snp->exp = init_node;
#ifdef DB_POSSIBLE
			snp->line = prevlineid;
#endif
			init_node = 0;	// compound resets after calling rather than before...
		}					//  I find it pretty weird :) (maybe it should be changed?)
		else snp = 0;
		break;
#endif
#endif
	  case begin:
		getsym();
		snp = compound(0);
		break;
	  case kw_if:
		snp = ifstmt();
		break;
	  case kw_while:
		snp = whilestmt();
		break;
	  case kw_for:
		snp = forstmt();
		break;
	  case kw_return:
		snp = retstmt();
		break;
	  case kw_break:
		snp = breakstmt();
		break;
	  case kw_goto:
		snp = gotostmt();
		break;
	  case kw_continue:
		snp = contstmt();
		break;
	  case kw_do:
		snp = dostmt();
		break;
	  case kw_switch:
		snp = switchstmt();
		break;
	  case kw_case:
	  case kw_default:
		snp = casestmt();
		break;
	  case kw_loop:
		snp = loopstmt();
		break;
	  case kw_count:
		snp = NULL;
		error(ERR_EXPREXPECT);
		break;
	  case kw_asm:
		snp = asmstmt();
		break;
	  case id:
		if (!getcache(id) && cached_sym==colon) {
			snp = labelstmt();
			break;
		}
#if defined(OLD_MID_DECL) || defined(MID_DECL)
		if (lastsp && lastsp->storage_class == sc_typedef)
			goto middle_decl;
#endif
default_decl:
		/* else fall through to process expression */
	  default:
		snp = exprstmt();
		break;
	}
	if (snp != 0)
		snp->next = 0;
	return snp;
}
// vim:ts=4:sw=4
