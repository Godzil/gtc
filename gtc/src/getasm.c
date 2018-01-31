/*
 * GTools C compiler
 * =================
 * source file :
 * inline assembly extraction routines
 *
 * Copyright 2001-2004 Paul Froissart.
 * Credits to Christoph van Wuellen and Matthew Brandt.
 * All commercial rights reserved.
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 */

#ifdef ASM
#define DECLARE
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

#define newline semicolon

#ifndef AS
#define label(__l) 12345
#endif

xstatic char * readonly asmreservedlist[] = {
	"a0",
	"a1",
	"a2",
	"a3",
	"a4",
	"a5",
	"a6",
	"a7",
	"b",
	"ccr",
	"d0",
	"d1",
	"d2",
	"d3",
	"d4",
	"d5",
	"d6",
	"d7",
	"l",
	"s",
	"sp",
	"sr",
	"usp",
	"w",
};

xstatic readonly struct oplst_elem {
	char		   *s;
	enum(e_op)		ov;
	short			val;
}				oplist[] =

{{
	"add", op_add
}, {
	"adda", op_add
}, {
	"addi", op_addi
}, {
	"addq", op_addq
}, {
	"addx", op_addx
}, {
	"and", op_and
}, {
	"andi", op_andi
}, {
	"asl", op_asl
}, {
	"asr", op_asr
}, {
	"bcc", op_bhs
}, {
	"bchg", op_bchg
}, {
	"bclr", op_bclr
}, {
	"bcs", op_blo
}, {
	"beq", op_beq
}, {
	"bge", op_bge
}, {
	"bgt", op_bgt
}, {
	"bhi", op_bhi
}, {
	"bhs", op_bhs
}, {
	"ble", op_ble
}, {
	"blo", op_blo
}, {
	"bls", op_bls
}, {
	"blt", op_blt
}, {
	"bmi", op_bmi
}, {
	"bne", op_bne
}, {
	"bpl", op_bpl
}, {
	"bra", op_bra
}, {
	"bset", op_bset
}, {
	"bsr", op_bsr
}, {
	"btst", op_btst
}, {
	"bvs", op_bvs
}, {
	"bvc", op_bvc
}, {
	"chk", op_chk
}, {
	"clr", op_clr
}, {
	"cmp", op_cmp
}, {
	"cmpi", op_cmpi
}, {
#ifndef LIGHT_DBXX_AND_SXX
	"dbcc", op_dbhs
}, {
	"dbcs", op_dblo
}, {
#endif
	"dbeq", op_dbeq
}, {
	"dbf", op_dbra
}, {
#ifndef LIGHT_DBXX_AND_SXX
	"dbge", op_dbge
}, {
	"dbgt", op_dbgt
}, {
	"dbhi", op_dbhi
}, {
	"dbhs", op_dbhs
}, {
	"dble", op_dble
}, {
	"dblo", op_dblo
}, {
	"dbls", op_dbls
}, {
	"dblt", op_dblt
}, {
	"dbmi", op_dbmi
}, {
#endif
	"dbne", op_dbne
}, {
#ifndef LIGHT_DBXX_AND_SXX
	"dbpl", op_dbpl
}, {
#endif
	"dbra", op_dbra
}, {
#ifndef LIGHT_DBXX_AND_SXX
	"dbvs", op_dbvs
}, {
	"dbvc", op_dbvc
}, {
#endif
	"dc", op_dc
}, {
	"dcb", op_dcb
}, {
	"divs", op_divs
}, {
	"divu", op_divu
}, {
	"ds", op_ds
}, {
	"eor", op_eor
}, {
	"eori", op_eori
}, {
	"even", op_even
}, {
	"exg", op_exg
}, {
	"ext", op_ext
}, {
	"jmp", op_jmp
}, {
	"jsr", op_jsr
}, {
	"lea", op_lea
}, {
	"link", op_link
}, {
	"lsl", op_lsl
}, {
	"lsr", op_lsr
}, {
	"move", op_move
}, {
	"movea", op_move
}, {
	"movem", op_movem
}, {
	"moveq", op_moveq
}, {
	"muls", op_muls
}, {
	"mulu", op_mulu
}, {
	"neg", op_neg
}, {
	"negx", op_negx
}, {
	"nop", op_dc, 0x4e71
}, {
	"not", op_not
}, {
	"or", op_or
}, {
	"ori", op_ori
}, {
	"pea", op_pea
}, {
	"rol", op_rol
}, {
	"ror", op_ror
}, {
	"roxl", op_roxl
}, {
	"roxr", op_roxr
}, {
	"rte", op_dc, 0x4e73
}, {
	"rtr", op_dc, 0x4e77
}, {
	"rts", op_rts
}, {
#ifndef LIGHT_DBXX_AND_SXX
	"scc", op_shs
}, {
	"scs", op_slo
}, {
	"seq", op_seq
}, {
	"sf", op_sf
}, {
	"sge", op_sge
}, {
	"sgt", op_sgt
}, {
	"shi", op_shi
}, {
	"shs", op_shs
}, {
	"sle", op_sle
}, {
	"slo", op_slo
}, {
	"sls", op_sls
}, {
	"slt", op_slt
}, {
	"smi", op_smi
}, {
	"sne", op_sne
}, {
	"spl", op_spl
}, {
	"st", op_st
}, {
#endif
	"sub", op_sub
}, {
	"suba", op_sub
}, {
	"subi", op_subi
}, {
	"subq", op_subq
}, {
	"subx", op_subx
}, {
#ifndef LIGHT_DBXX_AND_SXX
	"svs", op_svs
}, {
	"svc", op_svc
}, {
#endif
	"swap", op_swap
}, {
	"tas", op_tas
}, {
	"trap", op_trap
}, {
	"trapv", op_dc, 0x4e76
}, {
	"tst", op_tst
}, {
	"unlk", op_unlk
}};
#ifndef LIGHT_DBXX_AND_SXX
#define asm_N 118
#else
#define asm_N 85
#endif

int lastreg CGLOB;

void asm_getsym() {
	int l=lineid;
	getsym();
	if (lineid!=l)
		cached_sym=lastst, cached_lineid = lineid, lastst=newline;
}

struct ocode *asm_head CGLOB;
xstatic struct ocode *asm_tail CGLOB;
struct ocode *new_code(int s) {
	struct ocode   *new_ocode;
	new_ocode = (struct ocode *) xalloc(s, OCODE);
#ifdef NO_CALLOC
	new_ocode->fwd = 0;
#endif
	if (!asm_head) {
		asm_head = new_ocode;
#ifdef NO_CALLOC
		new_ocode->back = 0;
#endif
	} else {
		new_ocode->back = asm_tail;
		asm_tail->fwd = new_ocode;
	}
	return asm_tail = new_ocode;
}

void add_code(enum(e_op) op, int len, struct amode *ap1, struct amode *ap2, int line) {
/*
 * generate a code sequence into the asm list.
 */
	struct ocode   *new_ocode = new_code(sizeof(struct ocode));
	new_ocode->opcode = op;
	new_ocode->length = len;
	if ((unsigned int)len>4 || len==3)
		ierr(ADD_CODE,1);
	new_ocode->oper1 = ap1;
	new_ocode->oper2 = ap2;
#ifdef DB_POSSIBLE
	new_ocode->line=line;
#endif
#ifdef AS
	new_ocode->opt = 1;
#endif
}

void add_label(int lab) {
	struct lbls *new_ocode = (struct lbls *)new_code(sizeof(struct lbls));
	new_ocode->opcode = op_label;
	new_ocode->lab = lab;
}

int has_autocon(struct enode *ep) {
	switch (ep->nodetype) {
		case en_icon:
#ifndef NOFLOAT
		case en_fcon:
#endif
		case en_labcon:
		case en_nacon:
			return 0;
		case en_autocon:
			return 1;
		case en_add:
			return has_autocon(ep->v.p[0]) + has_autocon(ep->v.p[1]);
		case en_sub:
			return has_autocon(ep->v.p[0]) - has_autocon(ep->v.p[1]);
		case en_uminus:
			return - has_autocon(ep->v.p[0]);
		default:
			uerrc("invalid expression");
			return 0; // avoid compiler warning
	}
}
void get_offset(struct amode *ap) {
	asm_xflag++;
	asm_zflag++;
/*	if (!strcmp(lastid,"__Int5_body__"))
		printf("fgio");*/
	if (!exprnc(&ap->offset))
		error(ERR_INTEXPR);
	else {
		opt4(&ap->offset);
		if (ap->offset->nodetype==en_deref)
			ap->offset=ap->offset->v.p[0];
#ifdef OLD_AMODE_INPUT
		if (lastst==kw_offs_end) {
#else
		if (lastst==openpa) {
#endif
			asm_getsym();
			if (lastst==kw_areg) {
				ap->preg=lastreg;
				asm_getsym();
				if (lastst==comma) {
					asm_getsym();
					ap->sreg=lastreg;
					if (lastst==kw_dreg)
						ap->mode=am_indx2;
					else if (lastst==kw_areg)
						ap->mode=am_indx3;
					else error(ERR_SYNTAX);
					asm_getsym();
					if (lastst==dot) {
#ifdef EXT_AS_ID
						asm_getsym();
#else
						char c=lastch;
#endif
						ap->slen=0;
#ifdef EXT_AS_ID
						if (lastst==id && lastid[1]==0) {
							char c=lastid[0];
#endif
							if (c=='b' || c=='s') ap->slen=1;
							else if (c=='w') ap->slen=2;
							else if (c=='l') ap->slen=4;
#ifdef EXT_AS_ID
							asm_getsym();
						}
#else
						getch();
						asm_getsym();
#endif
						if (!ap->slen)
							error(ERR_SYNTAX);
					} else ap->slen=2;
				} else ap->mode=am_indx;
				needpunc(closepa);
			} else if (lastst==id && !strcmp("pc",lastid))
				ap->mode=am_pcrel, asm_getsym()/* 'pc' */, asm_getsym()/* ')' */;
		} else
			ap->mode=am_direct;
		if (has_autocon(ap->offset)) {
			if (ap->mode!=am_direct && (ap->mode!=am_indx || ap->preg<TRUE_FRAMEPTR-AREGBASE))
				error(ERR_INTEXPR);
			ap->mode=am_indx, ap->preg=FRAMEPTR-AREGBASE;
		}
	}
	asm_zflag--;
	asm_xflag--;
}

xstatic enum(e_op) lastop CGLOB;
xstatic int lastval CGLOB,second_parm CGLOB;
struct amode *get_asmparam() {
	struct amode *ap = (struct amode *) xalloc((int) sizeof(struct amode), AMODE);
	ap->preg=lastreg;
/*	if ((long)&ap->offset==0x7D53CC)
		printf("gio");*/
	if (lastop==op_movem && (lastst==kw_dreg || lastst==kw_areg)) {
		unsigned int mask=0; int n,d,x;
		if (second_parm) ap->mode=am_mask2,x=0;
		else ap->mode=am_mask1,x=15;
		/* Format : d0=0x8000 in mask1, 0x0001 in mask2 */
		while (lastst==kw_dreg || lastst==kw_areg) {
			d=n=lastreg+((lastst-kw_dreg)<<3);
			asm_getsym();
			if (lastst==minus) {
				asm_getsym();
				if (lastst==kw_dreg || lastst==kw_areg)
					d=lastreg+((lastst-kw_dreg)<<3);
				else error(ERR_SYNTAX);
				asm_getsym();
			}
			if (n>d)
				error(ERR_SYNTAX);
			while (n<=d) {
				mask |= 1<<(n^x);
				n++;
			}
			if (lastst==divide)
				asm_getsym();
		}
		ap->offset=mk_icon(mask);
	} else if (lastst==kw_dreg)
		ap->mode=am_dreg, asm_getsym();
	else if (lastst==kw_areg)
		ap->mode=am_areg, asm_getsym();
	else if (lastst==openpa) {
		asm_getsym();
		if (lastst==kw_areg) {
			ap->preg=lastreg;
			asm_getsym();
			needpunc(closepa);
			if (lastst==plus)
				asm_getsym(), ap->mode=am_ainc;
			else ap->mode=am_ind;
		} else {
			cached_sym=lastst;
			cached_lineid=lineid;
			lastst=openpa;
			get_offset(ap);
		}
	} else if (lastst==minus) {
		if (lastch=='(') {	// a bit dirty, but doesn't matter since you never type '- (an)'
			asm_getsym();	// remove '-'
			asm_getsym();	// remove '('
			if (lastst==kw_areg) {
				ap->preg=lastreg;
				asm_getsym();
				needpunc(closepa);
				ap->mode=am_adec;
			} else {
				get_offset(ap);
				needpunc(closepa);
				ap->offset=mk_node(en_uminus,ap->offset,NIL_ENODE);
				opt4(&ap->offset);
			}
		} else get_offset(ap);
	} else if (lastst==sharp) {
		asm_getsym();
		get_offset(ap);
		if (ap->mode!=am_direct)
			error(ERR_SYNTAX);
		else ap->mode=am_immed;
	} else if (lastst==id && ((!lastid[2] && lastid[1]=='r' && lastid[0]=='s') ||
			(!lastid[3] && ((lastid[2]=='r' && lastid[1]=='c' && lastid[0]=='c') ||
							(lastid[2]=='p' && lastid[0]=='u' && lastid[1]=='s'))))) {
		ap->mode=lastid[2]?(lastid[0]=='u'?am_usp:am_ccr):am_sr;
		getsym();
	} else get_offset(ap);
	return ap;
}

/* dichotomic keyword search */
void asm_searchkw() {
	char *s1,*s2,c;
	const struct oplst_elem *op_ptr;
	int a=0,b=asm_N-1,m;
#ifdef PC
	if (sizeof(oplist)/sizeof(struct oplst_elem)!=asm_N)
		fatal("BUILD ERROR, WRONG ASM KEYWORDS #");
#endif
/*	if (!strcmp("addq",lastid))
		printf("");*/
	while (a<=b) {
		m=(a+b)>>1;
		op_ptr=&oplist[m];
		s1=lastid;
		s2=op_ptr->s;
		do {
			if (!(c=*s2++)) {
				if (!*s1) {
					lastst=kw_instr;
					lastop=op_ptr->ov;
					lastval=op_ptr->val;
					return;
				}
				/* otherwise continue ( < 'addi' doesn't match 'add' but yet exists) */
			}
		} while (*s1++==c);
		if (s1[-1]<c)	// il faut aller plus haut
			b=m-1;
		else a=m+1;
	}
}
int asm_isreserved() {
	char *s1,*s2,c;
	int a=0,b=sizeof(asmreservedlist)/sizeof(char*)-1,m;
	while (a<=b) {
		m=(a+b)>>1;
		s1=lastid;
		s2=asmreservedlist[m];
		do {
			if (!(c=*s2++)) {
				if (!*s1)
					return 1;
				/* otherwise continue ( < 'addi' doesn't match 'add' but yet exists) */
			}
		} while (*s1++==c);
		if (s1[-1]<c)	// il faut aller plus haut
			b=m-1;
		else a=m+1;
	}
	asm_searchkw();
	return (lastst==kw_instr);
}


int getasm_main() {
	int has_lab=0;
  restart:
	if (lastst==newline) {
		has_lab=0;
		asm_getsym();
		goto restart;
	} else if (lastst==kw_instr) {
		int line=lineid,l=0,v=lastval; struct amode *ap1=0,*ap2=0;
		enum(e_op) op=lastop;
		int cont=1;
		asm_getsym();
		if (lastst!=newline) {
			if (lastst==dot) {
#ifndef EXT_AS_ID
				char c=lastch;
				if (c=='b' || c=='s') l=1;
				else if (c=='w') l=2;
				else if (c=='l') l=4;
				getch();
				asm_getsym();
#else
				asm_getsym();
				if (lastst==id && lastid[1]==0) {
					char c=lastid[0];
					if (c=='b' || c=='s') l=1;
					else if (c=='w') l=2;
					else if (c=='l') l=4;
					asm_getsym();
				}
#endif
				if (!l) {
					error(ERR_SYNTAX);
					goto restart;
				}
			}
			if (lastst!=newline && lastst!=end) {
				second_parm=0;
				ap1=get_asmparam();
				if (lineid==line && lastst!=newline && lastst!=end) {
													// since exprnc calls getsym, not asm_getsym
					if (lastst!=comma)
						cont=0;/*error(ERR_PUNCT);*/
					else if (op!=op_dc) {
						asm_getsym();
						if (lastst!=newline)
							second_parm=1,ap2=get_asmparam();
					} else {
						while (lineid==line && lastst==comma) {
							asm_getsym();
							add_code(op,l,ap1,0,line);
							ap1=get_asmparam();
						}
					}
				} else if ((int)cached_sym IS_INVALID)	// otherwise it's because last getsym was
					cached_sym=lastst,			// an asm_getsym, and we already know the 'NL'
						cached_lineid=lineid,
						lastst=newline;
			}
		}
		if (v) {
			if (ap1)
				uerrc("too many arguments");
			ap1=mk_offset(mk_icon(v)), ap2=0;
			l=2;	// always 16-bit declaration
		}
		add_code(op,l,ap1,ap2,line);
		return cont;
	} else if (lastst==id) {	/* it's a label */
		if (has_lab)			// forbid multiple labels on one single line
			error(ERR_SYNTAX);
		has_lab=1;
		add_label(label(lastid));
		asm_getsym();
		if (lastst==colon)
			asm_getsym();
		goto restart;
	} else return 0;
}

struct snode *asmstmt(void) {
/*
 * asmstmt parses the gtc c asm statement.
 */
	struct snode   *snp;
	snp = (struct snode *) xalloc((int) sizeof(struct snode), SNODE);
	snp->stype = st_asm;
	snp->count = 1;
	asm_flag++;
	asm_head = 0;
	getsym();
	if (lastst == kw_volatile)
		getsym();
	if (lastst != begin)
		error(ERR_EXPREXPECT);
	asm_getsym();
	while (getasm_main());
	snp->v1.i=(long)asm_head;
	asm_flag--;
	needpunc(end);
	needpunc(semicolon);
#ifdef DB_POSSIBLE
	snp->line=lineid;
#endif
	return snp;
}
#undef newline
#endif
// vim:ts=4:sw=4
