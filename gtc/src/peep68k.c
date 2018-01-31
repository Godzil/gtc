/*
 * GTools C compiler
 * =================
 * source file :
 * peephole optimizations
 *
 * Copyright 2001-2004 Paul Froissart.
 * Credits to Christoph van Wuellen and Matthew Brandt.
 * All commercial rights reserved.
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 */

#define NEWLAB
//#define NO_PEEP

#include	"define.h"
_FILE(__FILE__)
#include	"c.h"
#include	"expr.h"
#include	"gen.h"
#include	"cglbdec.h"
#ifndef NOFLOAT
#include	"ffplib.h"
#endif

#ifdef MC680X0

extern struct enode *regexp[REGEXP_SIZE];
struct ocode *_peep_head CGLOB;
extern struct ocode *scope_head;
#ifdef SPEED_OPT
int speed_opt_value CGLOB;
#endif
#ifndef VCG
struct ocode *peep_head CGLOB;
#else
struct ocode *vcg_peep_head[VCG_MAX+1] CGLOBL;
struct ocode *vcg_peep_tail[VCG_MAX+1] CGLOBL;
int vcg_lvl CGLOB;
#define peep_head vcg_peep_head[vcg_lvl]
#define peep_tail vcg_peep_tail[vcg_lvl]
#endif
xstatic struct ocode *next_ip CGLOB;
xstatic struct ocode *last_param_pop CGLOB;

#define getop1(__ip) (__ip->oper1->offset->v.i)
#define getoplab(__ip) (__ip->oper1->offset->v.enlab)
#define getop2(__ip) (__ip->oper2->offset->v.i)
#ifdef NEWLAB
#define getlab(__ip) (((struct lbls *)__ip)->lab)
#else
#define getlab(__ip) (__ip->oper1->offset->v.i)
#endif
/*static enum(e_op) revcond[] = { op_bne, op_beq, op_bge, op_bgt, op_ble, op_blt,
							   op_bls, op_blo, op_bhs, op_bhi };*/
xstatic readonly enum(e_op) revcond[] =
	{
		op_bne, op_beq, op_blo, op_blt, op_bls, op_ble, op_bhi, op_bgt, op_bhs, op_bge
	};

void add_peep(struct ocode *new_ocode);
void opt3(void);

void g_code(enum(e_op) op, int len, struct amode *ap1, struct amode *ap2) {
/*
 * generate a code sequence into the peep list.
 */
	struct ocode   *new_ocode;
	new_ocode = (struct ocode *) xalloc((int) sizeof(struct ocode), OCODE+G_CODE);
	new_ocode->opcode = op;
	new_ocode->length = len;
	if ((unsigned int)len>4 || len==3)
		ierr(G_CODE,1);
	new_ocode->oper1 = ap1;
	new_ocode->oper2 = ap2;
#ifdef PC
/*	if (op>=op_add && op<=op_subq && ap1->mode>am_areg && ap2->mode>am_areg
		&& ap1->mode!=am_immed)
		fatal("INVALID INSTRUCTION");
	if ((long)new_ocode==0x7a6314)
		bkpt();*/
/*	if (lineid==1157)
		bkpt();*/
#endif
#ifdef DB_POSSIBLE
	new_ocode->line=lineid;
#endif
	add_peep(new_ocode);
}

void g_coder(enum(e_op) op, int len, struct amode *ap1, struct amode *ap2) {
/*
 * generate a code sequence and put it in front of the list
 */
	struct ocode   *new_ocode;
	new_ocode = (struct ocode *) xalloc((int) sizeof(struct ocode), OCODE+G_CODE);
	new_ocode->opcode = op;
	new_ocode->length = len;
	new_ocode->oper1 = ap1;
	new_ocode->oper2 = ap2;
#ifdef DB_POSSIBLE
	new_ocode->line=lineid;
#endif
	if (peep_head == 0) {
		/* must use add_peep to take care of peep_tail */
		add_peep(new_ocode);
	} else {
		new_ocode->back  = 0;
		new_ocode->fwd   = peep_head;
		peep_head->back = new_ocode;
		peep_head  = new_ocode;
		/* peep_tail does not change */
	}
}

/* should be in add_peep(), but SConvert would put it in the text */
/*  segment, preventing it from being modified */
#ifndef VCG
xstatic struct ocode *peep_tail CGLOB;
#endif

void add_peep(struct ocode *new_ocode) {
/*
 * add the ocode pointed to by new to the peep list.
 */
	if (peep_head == 0) {
		peep_head = peep_tail = new_ocode;
	} else {
		new_ocode->back = peep_tail;
		peep_tail->fwd = new_ocode;
		peep_tail = new_ocode;
	}
#ifndef NOFLOAT
	if (new_ocode->opcode==op_jsr && new_ocode->oper1->mode==en_nacon) {
		struct enode *ep=new_ocode->oper1->offset;
		char *s=ep->v.ensp;
		if (!strcmp("iround",s) || !strcmp("uround",s) || !strcmp("ifloor",s)) {
			int lab=nxtlabel();
#ifdef AS
			ep->v.enlab=label(
#endif
				ep->v.ensp=strsave(s[0]=='i'?str(ffpftol):str(ffpftou))
#ifdef AS
			)
#endif
			;
			g_code(s[1]=='r'?op_bhs:op_bge, 1, mk_label(lab), NIL_AMODE);
			g_code(s[1]=='r'?op_addq:op_subq, 4, mk_immed(1),
				(struct amode *) xalloc((int) sizeof(struct amode), AMODE));
			#ifdef NO_CALLOC
			#error Above line bugs.
			#endif
			g_label(lab);
		}
	}
#endif
}

extern unsigned int pos;
void g_label(unsigned int labno) {
/*
 * add a compiler-generated label to the peep list.
 */
#ifdef NEWLAB
	struct lbls *new_ocode;
/*	if (pos==0x25D2 && labno==1)
		printf("ghio");*/
	new_ocode = (struct lbls *) xalloc((int) sizeof(struct lbls), OCODE+G_LABEL);
	new_ocode->opcode = op_label;
	new_ocode->lab = labno;
	add_peep((struct ocode *)new_ocode);
#else
	struct ocode   *new_ocode;
	new_ocode = (struct ocode *) xalloc((int) sizeof(struct ocode), OCODE+G_LABEL);
	new_ocode->opcode = op_label;
	new_ocode->oper1 = mk_immed((long) labno);
	add_peep(new_ocode);
#endif
}

void flush_peep() {
/*
 * output all code and labels in the peep list.
 */
#ifndef AS
	register struct ocode *ip;
	register int line;
#endif
#ifdef SHOWSTEP
	printf("\npeep");
#endif
	opt3(); 					/* do the peephole optimizations */
#ifdef SHOWSTEP
	printf(" ok ");
	printf("\nflush");
#endif
	_peep_head=peep_head;
#ifndef AS
	ip = peep_head;
	line = -1;
	if (DEBUG && func_sp)
		fprintf(output,"; function '%s'\n",func_sp->name);
	while (ip != 0) {
		if (ip->opcode == op_label)
			put_label((unsigned int) getlab(ip));
		else {
	#ifdef DB_POSSIBLE
			if (DEBUG && line!=ip->line)
				fprintf(output,"; line %d\n",line=ip->line);
	#endif
			put_code(ip->opcode, ip->length, ip->oper1, ip->oper2);
		}
		ip = ip->fwd;
	}
	if (DEBUG)
		fprintf(output,"; end of function\n");
#else
	scope_head = peep_head;
#endif
	peep_head = 0;
#ifdef SHOWSTEP
	printf(" ok ");
#endif
}

static void peep_delete(struct ocode *ip) {
/*
 * delete an instruction referenced by ip
 */
	if (ip == 0)
		ierr(PEEP_DELETE,1);

	if (ip->back) {
		if ((ip->back->fwd = ip->fwd) != 0)
			ip->fwd->back = ip->back;
		next_ip = ip->back;
	} else {
		if ((peep_head = ip->fwd) != 0)
			peep_head->back = 0;
		next_ip = peep_head;
	}
}

static void peep_pea(struct ocode *ip) {
/*
 * changes lea <ea>,An + pea (An) to pea <ea>
 *	The value of An is not needed (An is scratch register)
 * CAVEAT code generator modifier!
 */
	struct ocode   *prev;
	if (ip->oper1->mode != am_ind)
		return;
	if ((prev = ip->back) == 0)
		return;
	if (prev->opcode == op_lea && prev->oper2->preg == ip->oper1->preg
		&& ip->oper1->preg <= MAX_ADDR) {
		prev->opcode = op_pea;
		prev->oper2 = 0;
		peep_delete(ip);
	}
}

static void peep_lea(struct ocode *ip) {
/*
 * changes lea <ea>,An + move.l An,Am   to lea <ea>,Am
 *  -- the value of An is not needed (An scratch, Am typically tempref)
 * and     lea <ea>,An + move.* (An),An to move.* <ea>,An
 * CAVEAT code generator modifier!
 */
	struct ocode   *next;
	reg_t reg;
  restart:
	if ((next = ip->fwd) == 0
#if defined(AS) && defined(ASM)
		|| next->opt
#endif
	)
		return;
	if (next->opcode == op_move && ip->oper2->preg <= MAX_ADDR &&
		next->oper1->mode == am_areg && next->oper1->preg == ip->oper2->preg
		&& next->oper2->mode == am_areg && next->oper2->preg > MAX_ADDR) {
		ip->oper2 = next->oper2;
		peep_delete(next);
		goto restart;
	}
	/* lea <ea>,An + move.* (An),An => move.* <ea>,An */
	if (next->opcode == op_move && next->oper1->mode == am_ind)
		if (((reg=next->oper1->preg) == ip->oper2->preg) && next->oper2->mode == am_areg &&
		(reg==next->oper2->preg)) {
		ip->opcode = op_move;
		ip->length = next->length;
		peep_delete(next);
	}
}

static void peep_move(struct ocode *ip) {
/*
 * peephole optimization for move instructions.
 * makes quick immediates when possible (redundant for most assemblers).
 * changes move #0 to clr except on address registers
 * changes move #0 to address registers to sub An,An
 * changes long moves to address registers to short when possible.
 * changes move immediate/areg to stack to pea.
 * changes move immediate to An to lea.
 * deletes move <ea>,<ea> (The code generator does not know that this sets
 *						   the flags, and it is necessary for peep_and
 *						   to work correctly if ea is am_dreg)
 */
	struct enode   *ep;
#ifdef ADVANCED_MEMORY_MOVE
	struct ocode *next;
#endif

	if (equal_address(ip->oper1, ip->oper2) &&
/*
 * move.w An,An changes the contents of An through sign extension
 */
	   (ip->oper1->mode != am_areg || ip->length != 2)) {
		peep_delete(ip);
		return;
	}
	if ((ip->oper1->mode == am_areg || ip->oper1->mode == am_immed)
		&& ip->oper2->mode == am_adec && ip->oper2->preg == 7
		&& ip->length == 4) {
		ip->opcode = op_pea;
		ip->length = 0;
		ip->oper1 = copy_addr(ip->oper1);
		am_doderef(ip->oper1->mode);
		ip->oper2 = 0;
		return;
	}
//#if 0
#ifdef ADVANCED_TEMP_REGS
#ifndef INFINITE_REGISTERS
	if (ip->oper1->mode==am_dreg && ip->oper1->preg<=MAX_DATA
		&& ip->oper2->mode==am_dreg) {
		struct ocode *ip_save=ip;
		reg_t a=ip->oper1->preg,b=ip->oper2->preg,s=ip->length;
		struct amode *ap,*destap=ip->oper2;
		/* BUGGY: we should check if da isn't used afterwards...
		 *  Currently, g_assign does in such way that it returns Db
		 * rather than Da, so it should work fine in most cases,
		 * but it might fail eg with custom asm statements */
		ip=ip->back;
		while (ip) {
			switch (ip->opcode) {
			  case op_label:
			  case op_jsr:
			  case op_bxx:
			  case op_dbxx:
			  case op_dc:	// this might mean op_jsr...
				return;
			  case op_ext:
				if (ip->oper1->mode==am_dreg && ip->oper1->preg==a) {
					s=ip->length>>1;
					break;
				}
				if ((ap=ip->oper1) && ap->mode==am_dreg && ap->preg==b) {
#ifdef PC
					if (ip->length && ip->length<=s)
						fatal("OP DISCARDED BY ASSIGNMENT");
					else
#endif
						return;
				}
				break;
			  case op_clr:
				ap=ip->oper1;
				goto check_ok;
			  case op_move:
			  case op_moveq:
			  case op_lea:
				ap=ip->oper2;
			check_ok:
				if (ap->mode==am_dreg && ap->preg==a) {
					if (ip->length && ip->length<s) return;
					else goto ok_atr;
				}
				//break;
			  default:
				if ((ap=ip->oper1) && ((ap->mode==am_dreg && ap->preg==b) ||
						(ap->mode==am_indx2 && ap->sreg==b)))
					return;
				if ((ap=ip->oper2) && ap->mode==am_indx2 && ap->sreg==b)
					return;
				if ((ap=ip->oper2) && ap->mode==am_dreg && ap->preg==b) {
#ifdef PC
					if (ip->length && ip->length<=s)
						fatal("OP DISCARDED BY ASSIGNMENT");
					else
#endif
						return;
				}
			}
			ip=ip->back;
		}
		ierr(UNINIT_TEMP,1);
	  ok_atr:
		next_ip=ip;
		ip=ip_save;
		while (ip!=next_ip) {
			if (ip->oper1 && ip->oper1->mode==am_dreg && ip->oper1->preg==a)
				ip->oper1=destap;
			if (ip->oper2 && ip->oper2->mode==am_dreg && ip->oper2->preg==a)
				ip->oper2=destap;
			ip=ip->back;
		}
		if (ip->opcode==op_clr)
			ip->oper1=destap;
		else ip->oper2=destap;
		return;
	}
#endif
#endif
#ifdef ADVANCED_MEMORY_MOVE
	/* try to use post-increment where possible                           */
	/* (might be improved to allow for interstitial instructions, but
	    beware of modifications of _any_ of _both_ indexing registers...) */
	if (ip->oper2->mode == am_indx && (next=ip->fwd) && next->opcode==op_move
		&& next->oper2->mode == am_indx && next->oper2->preg == ip->oper2->preg
		&& next->oper2->offset->v.i-ip->oper2->offset->v.i==ip->length) {
		int reg=-1;
#ifndef INFINITE_REGISTERS
		if (arsearch(0,ip))
			reg=8;
		else if (arsearch(1,ip))
			reg=9;
#endif
		if (reg IS_VALID) {
			struct ocode *load = xalloc(sizeof(struct ocode),OCODE);
			long offs; struct amode *ap;
			load->opcode=op_lea;
			load->oper1=ip->oper2;
			load->oper2=mk_reg(reg);
			load->line=ip->line;
			load->back=ip->back;
			load->fwd=ip;
			load->back->fwd=load;
			ip->back=load;
			next=ip;
			reg=ip->oper2->preg;
			offs=ip->oper2->offset->v.i;
			ap=mk_reg(reg); ap->mode=am_ainc;
			do {
				next->oper2=ap;
				offs+=next->length;
				next=next->fwd;
			} while (next->opcode==op_move && next->oper2->preg==reg
				&& next->oper2->mode==am_indx
				&& next->oper2->offset->v.i==offs);
		}
	}
#endif
	if (ip->oper1->mode != am_immed)
		return;
#ifdef ADVANCED_MEMORY_MOVE
	/* optimize contiguous -(an)/(an)+ assignments */
   restart_advmove:
	if (am_is_increment(ip->oper2->mode) && ip->length<=2 && (next=ip->fwd)
		&& next->opcode==op_move && ip->oper1->offset->nodetype==en_icon
		&& next->oper1->mode == am_immed && next->oper2->mode==ip->oper2->mode
		&& next->length==ip->length
		&& next->oper1->offset->nodetype==en_icon) {
		unsigned short a,b;	/* very important *not* to be long's !!! */
		a=(unsigned short)ip->oper1->offset->v.i,b=(unsigned short)next->oper1->offset->v.i;
		if (ip->oper2->mode==am_adec)
			b=a,a=(unsigned short)next->oper1->offset->v.i;
		ip->oper1=copy_addr(ip->oper1);
		ip->oper1->offset =
			mk_icon((ip->length==2?((long)a<<16)|b:(long)(unsigned short)((a<<8)|(unsigned char)b)));
		ip->length<<=1;
		peep_delete(next);
		if (ip->back->opcode==op_move) {
			next_ip=ip->back;
			return;
		} else goto restart_advmove;
		/*
		 move.b #0,(a0)+ <-
		 move.b #0,(a0)+
		 move.b #0,(a0)+
		 move.b #0,(a0)+
		->
		 move.w #0,(a0)+ <-
		 move.b #0,(a0)+
		 move.b #0,(a0)+
		->
		 clr.w (a0)+
		 move.b #0,(a0)+ <-
		 move.b #0,(a0)+
		->
		 clr.w (a0)+     <-
		 move.w #0,(a0)+
		->
		 clr.w (a0)+     <-
		 move.w #0,(a0)+
		->
		 move.l #0,(a0)+ <-
		->
		 clr.l (a0)+     <-
		*/
	}
#endif

	ep = ip->oper1->offset;
	if (ip->oper2->mode == am_areg && ep->nodetype == en_icon) {
		if (ep->v.i == 0) {
			ip->length = 4;
			ip->opcode = op_sub;
			ip->oper1 = ip->oper2;
		} else if (-32768 <= ep->v.i && ep->v.i <= 32767)
			ip->length = 2;
		return;
	}
	if (ip->oper2->mode == am_dreg && ep->nodetype == en_icon
		&& -128 <= ep->v.i && ep->v.i <= 127) {
		ip->opcode = op_moveq;
		ip->length = 0;
		return;
	}
	if (ep->nodetype == en_icon && ep->v.i == 0) {
		ip->opcode = op_clr;
		ip->oper1 = ip->oper2;
		ip->oper2 = 0;
		return;
	}
	if (ip->oper2->mode == am_areg && ip->length == 4) {
		next_ip = ip;
		ip->opcode = op_lea;
		ip->length = 0;
		ip->oper1 = copy_addr(ip->oper1);
		ip->oper1->mode = am_direct;
		return;
	}
}

int equal_address(struct amode *ap1, struct amode *ap2) {
/*
 * compare two address nodes and return true if they are equivalent.
 */
	if (ap1 == 0 || ap2 == 0)
		return 0;
	if (ap1->mode != ap2->mode)
		return 0;
	switch (ap1->mode) {
	  case am_areg:
	  case am_dreg:
	  case am_ind:
		return ap1->preg == ap2->preg;
	  case am_indx:
		return ap1->preg == ap2->preg &&
			   ap1->offset->nodetype == en_icon &&
			   ap2->offset->nodetype == en_icon &&
			   ap1->offset->v.i == ap2->offset->v.i;
	  case am_indx2:
	  case am_indx3:
		return
			ap1->preg == ap2->preg &&
			ap1->sreg == ap2->sreg &&
			ap1->slen == ap2->slen &&
			ap1->offset->nodetype == en_icon &&
			ap2->offset->nodetype == en_icon &&
			ap1->offset->v.i == ap2->offset->v.i;
	}
	return 0;
}

static void peep_movem(struct ocode *ip) {
/*
 * peephole optimization for movem instructions. movem instructions are used
 * to save registers on the stack. if 1 or 2 registers are being saved we
 * can convert the movem to move instructions.
 */
	int i,mask,n,t,a,b=0;
	struct ocode *root;

#ifdef PC
	a=1234;	/* prevent a warning from being output - we can afford this on PC :) */
#endif

	if (ip->oper1->mode == am_mask1)
		mask = getop1(ip);
	else mask = getop2(ip);

	t=1,n=2,i=16;
	while (i--) {
		if (mask & t) {
			b=a,a=i;
			if (--n<0) return;
		}
		t<<=1;
	}
	root=ip;
	ip->opcode = op_move;
	if ((i=1-n)) {
		struct ocode *new_ocode = (struct ocode *)
			xalloc((int) sizeof(struct ocode), OCODE);
		new_ocode->opcode = op_move;
		new_ocode->length = ip->length;
		new_ocode->oper1 = copy_addr(ip->oper1);
		new_ocode->oper2 = copy_addr(ip->oper2);
#ifdef DB_POSSIBLE
		new_ocode->line=ip->line;
#endif
		new_ocode->back = ip;
		if ((new_ocode->fwd=ip->fwd))
			ip->fwd->back=new_ocode;
		ip->fwd=new_ocode;
	}
	do {
		if (ip->oper1->mode == am_mask1) {
				if ((ip->oper1->preg = a) >= 8) {
					ip->oper1->mode = am_areg;
					ip->oper1->preg -= 8;
#ifdef INFINITE_REGISTERS
					ip->oper1->preg += TAREGBASE-AREGBASE;
#endif
				} else {
					ip->oper1->mode = am_dreg;
#ifdef INFINITE_REGISTERS
					ip->oper1->preg += TDREGBASE;
#endif
				}
			} else {
				if ((ip->oper2->preg = 15 - a) >= 8) {
					ip->oper2->mode = am_areg;
					ip->oper2->preg -= 8;
#ifdef INFINITE_REGISTERS
					ip->oper2->preg += TAREGBASE-AREGBASE;
#endif
				} else {
					ip->oper2->mode = am_dreg;
#ifdef INFINITE_REGISTERS
					ip->oper2->preg += TDREGBASE;
#endif
				}
		}
		ip=ip->fwd;
		a=b;
	} while (i--);
	next_ip=root;	/* optimize move.l An,-(a7) => pea (An) */
}

int drsearch(int r,struct ocode *ip) {
	while ((ip=ip->fwd)) {
		struct amode *ap=ip->oper1;
		int n=0;
		do {
			switch (ap->mode) {
				case am_dreg:
					if (ap->preg==r)
						return n && op_destroy(ip->opcode);
				case am_areg:
				case am_ind: case am_adec: case am_ainc: case am_indx:
				case am_indx3:
					break;
				case am_indx2:
					if (ap->sreg==r)
						return 0;
					break;
			}
			n++;
		} while ((ap=ip->oper2));
	}
	return 1;
}

int arsearch(int r,struct ocode *ip) {
	struct amode *ap; int n,z;
//	printf("$%d",r);
	z=0;
	while ((ip=ip->fwd) && ip->opcode!=op_rts) {
		if (ip->opcode==op_label) continue;
#ifdef INFINITE_REGISTERS
		if (ip->opcode==_op_cleanup_for_external_call) continue;
#endif
		z++;
		ap=ip->oper1;
		n=0;
		do {
			switch (ap->mode) {
				case am_areg:
					if (ap->preg==r) {
//						printf("*%d ",z);
						return n && op_destroy(ip->opcode);
					}
				case am_dreg:
					break;
				case am_indx3:
					if (ap->sreg==r) {
//						printf("-%d ",z);
						return 0;
					}
					/* FALL THROUGH */
				case am_ind: case am_adec: case am_ainc: case am_indx:
				case am_indx2:
					if (ap->preg==r) {
//						printf("-%d ",z);
						return 0;
					}
					break;
			}
			n++;
		} while ((ap=ip->oper2) && n==1);
	}
//	printf("/%d ",z);
	return 1;
}

static void peep_addsub(struct ocode *ip, enum (e_op) op) {
/*
 * peephole optimization for add/sub instructions.
A* changes add/sub.* Rn,An; move (An),X			to move 0(An,+/- Rn.*),X
A* changes add/sub.* Rn,An; move x(An),X		to move x(An,+/- Rn.*),X
A* changes add/sub   #x,An; move 0(An,Rn.*),X	to move x(An,+/- Rn.*),X
 * makes quick immediates out of small constants (redundant for most as's).
 * change add/sub immediate to address registers to lea where possible
A* and try to avoid operand volatilizing when doing so
 */
	struct enode   *ep;
	struct ocode   *next=ip->fwd;
#ifdef ADV_OPT
	int reg;
	if (next
#ifdef AS
		&& !next->opt
#endif
		&& next->opcode==op_move
		&& ip->oper2->mode==am_areg && (reg=ip->oper2->preg)==next->oper1->preg
		&& arsearch(reg,next)) {
		int sub=op-op_add,ok=0,sz=ip->length,sreg=ip->oper1->preg;
		long offs=next->oper1->offset?getop1(next):0;
/*		infunc("InitLevel")
			bkpt();*/
		ok=next->oper1->mode;
		if (ip->oper1->mode==am_dreg || ip->oper1->mode==am_areg) {
			if (ok==am_ind)
				offs=0;
			else if (ok!=am_indx)
				ok=0;
			/* if we do the conversion, select am_indx2/3 appropriately */
			if (ok)
				ok=ip->oper1->mode-am_dreg+am_indx2;
		} else if (ip->oper1->mode==am_immed) {
			sz=next->oper1->slen;
			sreg=next->oper1->sreg;
			offs=getop1(ip);
			if (sub) offs=-offs;
			sub=0;
			if (ok==am_indx3)
				sreg+=AREGBASE;
			else if (ok!=am_indx3)
				ok=0;
		} else ok=0;
		if (ok && offs>=-128 && offs<128) {
			if (sub) ip->opcode=op_neg,ip->oper2=NULL;
			else peep_delete(ip);
			next->oper1=copy_addr(next->oper1);
			next->oper1->mode=ok;
			next->oper1->slen=sz;
			next->oper1->sreg=sreg;
			next->oper1->offset=mk_icon(offs);
			return;
		}
	}
#endif
	if (ip->oper2->preg == STACKPTR-AREGBASE && ip->length == 2) {	/* alloca... */
		last_param_pop = NULL;
	}
	if (ip->oper1->mode == am_immed) {
		ep = ip->oper1->offset;
		if (ip->oper2->mode != am_areg)
			ip->opcode = op+op_addi-op_add;
		if (ep->nodetype != en_icon)
			return;
#ifdef POP_OPT
		if (ip->oper2->preg == STACKPTR-AREGBASE && 
			ip->oper2->mode == am_areg && ip->length == 4) {	/* parameter popping */
			if (last_param_pop)
				ep->v.i+=getop1(last_param_pop), peep_delete(last_param_pop);
			last_param_pop = ip;
		}
#endif
/*		if (!ep->v.i) {			// unnecessary
			peep_delete(ip);
			return;
		}*/
		if (1 <= ep->v.i && ep->v.i <= 8) {
			ip->opcode = op+op_addq-op_add;
			return;
		}
		if (-8 <= ep->v.i && ep->v.i <= -1) {
			ip->opcode = op==op_add?op_subq:op_addq;
			ep->v.i = -ep->v.i;
			return;
		}
		if (ip->oper2->mode == am_areg && isshort(ep)) {
			ip->oper1 = copy_addr(ip->oper1);
			ip->oper1->mode = am_indx;
			ip->oper1->preg = ip->oper2->preg;
			if (op==op_sub) ip->oper1->offset=mk_icon(-getop1(ip));
			ip->length = 0;
			ip->opcode = op_lea;
			next_ip = ip;
#ifdef ADV_OPT
			if (ip->back && ip->back->opcode==op_move && ip->back->oper2->mode==am_areg
				&& ip->back->oper2->preg==ip->oper1->preg
				&& ip->back->oper1->mode==am_areg
				&& arsearch(ip->oper1->preg,ip))
					ip->oper1->preg=ip->back->oper1->preg, peep_delete(ip->back);
#endif
			return;
		}
	}
}

static void peep_and(struct ocode *ip) {
/*
 * conversion of unsigned data types often yields statements like
 * move.b source,d0 +  andi.l #255,d0
 * which should be converted to
 * clr.l d0 + move.b source,d0
 * deletes and #-1
 */
	struct ocode   *prev;
	int 			size;
	long			arg;
	if (ip->oper1->mode != am_immed ||
		ip->oper1->offset->nodetype != en_icon)
		return;
	arg = getop1(ip);
	/*
	 * and #-1 does only set flags, which the code generator does not know
	 */
	if (arg == -1) {
		peep_delete(ip);
		return;
	}
	if (ip->oper1->mode != am_immed ||
		(arg != 255 && arg != 65535))
		return;

	size = (arg == 255) ? 1 : 2;

	/* move.b <ea>,dn; and.* #$FF,dn	à condition que <ea> != x(An,Dn) */
	/* move.w <ea>,dn; and.* #$FFFF,dn	à condition que <ea> != x(An,Dn) */
	if ((prev = ip->back) == 0 || prev->length != size
		|| prev->opcode != op_move
		|| ip->oper2->mode != am_dreg || prev->oper2->mode != am_dreg
		|| ip->oper2->preg != prev->oper2->preg
		||		(prev->oper1->mode == am_indx2 &&
				 prev->oper1->sreg == prev->oper2->preg))
		return;

	prev->length = ip->length;
	ip->length = size;
	prev->opcode = op_clr;
	ip->opcode = op_move;
	ip->oper1 = prev->oper1;
	prev->oper1 = prev->oper2;
	prev->oper2 = 0;

	next_ip = prev;
}

static void peep_clr(struct ocode *ip) {
/*
 * removes consecutive clr-statements
 *
 */
	struct ocode   *prev;

	if ((prev = ip->back) == 0 || prev->opcode != op_clr ||
		!equal_address(ip->oper1, prev->oper1))
		return;

	if (prev->length < ip->length)
		prev->length = ip->length;

	peep_delete(ip);
}

static void peep_cmp(struct ocode *ip) {
/*
 * peephole optimization for compare instructions.
 * changes compare #0 to tst
 *
 */
	struct enode   *ep;
	if (ip->oper1->mode != am_immed)
		return;
	ep = ip->oper1->offset;
	if (ip->oper2->mode == am_areg)
		/* cmpa.w extents the first argument automatically */
	{
		if (isshort(ep))
			ip->length = 2;
		return;
	}
	ip->opcode = op_cmpi;
	if (ep->nodetype != en_icon || ep->v.i != 0)
		return;
	ip->oper1 = ip->oper2;
	ip->oper2 = 0;
	ip->opcode = op_tst;
	next_ip = ip;
}

static void peep_tst(struct ocode *ip) {
/*
 * deletes a tst instruction if the flags are already set.
 */
	struct ocode   *prev,*next;
	enum(e_op) op;
	prev = ip->back;
	if (prev == 0)
		return;
/*
 * All the following cases used to be checked with an obscure
 * if-statement. There was an error in it that caused prev->oper2->mode
 * to be referenced even if prev->opcode is op_clr. (This yields a NIL-
 * pointer reference.
 * I threw all this stuff away and replaced it by this case-statement.
 * This is much more readable.
 */
	switch (prev->opcode) {
		case op_label:
		case op_dc:
		case _op_asm:
		case _op_adj:
/*
 * List all pseudo-instructions here. Of course, they do not do
 * anything.
 */
			return;

		case op_move:
/*
 * A move TO an address register does not set the flags
 */
			if (prev->oper2->mode == am_areg)
				return;
		case op_moveq:
		case op_clr:
		case op_ext:
/*
 * All other move and clr instructions set the flags according to
 * the moved operand, which is prev->oper1
 */
			if (equal_address(prev->oper1, ip->oper1))
				break;
			if (equal_address(prev->oper2, ip->oper1))
				break;
			return;
		case op_btst: case op_bset: case op_bclr: case op_bchg:
		case op_cmp:
			/* these instructions affect the flags in a non-standard way */
		case op_swap:/*[longword test]*/
			/* FALL THROUGH */
		case op_exg:
			/* these instructions don't affect the flags */
			return;
		default:
/*
 * All instructions that have a target set the flags according to the
 * target (prev->oper2)
 * Note that equal_address may be called with a NIL pointer -> OK for clr
 */
			next = ip->fwd;
			if (next && ((op=next->opcode)==op_beq || op==op_bne)
				&& !(next->fwd && (op=next->fwd->opcode)>=_op_bcond_min && op<=_op_bxx_max))
					if (equal_address(prev->oper2, ip->oper1))
						break;
			return;
	}
/*
 * We come here if the flags are already set, thus the tst
 * instruction can be deleted.
 */
	if (ip->length==prev->length)
		peep_delete(ip);
}

static void peep_uctran(struct ocode *ip) {
/*
 * peephole optimization for unconditional transfers. deletes instructions
 * which have no path. applies to bra, jmp, and rts instructions.
 */
	while (ip->fwd != 0 && ip->fwd->opcode != op_label)
		peep_delete(ip->fwd);
	next_ip=ip->fwd;
}

static void peep_bxx(struct ocode *ip) {
/*
 * optimizes conditional branch over a bra.
 */
	struct ocode   *next = ip->fwd;
	if (next && next->opcode == op_bra) {
	   /* peep_uctran increases the 'hit' probability */
		peep_uctran(next);
		while ((next = next->fwd) && next->opcode == op_label)
		  if (getoplab(ip) == getlab(next)) {
			/* bxx \lab | bra \out | ... | \lab:
			 *  => (peep_uctran)
			 * bxx \lab | bra \out | \lab:
			 *  =>
			 * byy \out | \lab:
			 */
			ip->fwd->opcode = revcond[(int)ip->opcode - (int)_op_bcond_min];
			ip=ip->fwd;
			peep_delete(ip->back);
			break;
		  }
	}
	if (ip->opcode==op_bhs && ip->back && ip->back->opcode==op_subq
		&& ip->back->oper1->offset->v.i==1 /* since oper1->mode is am_immed->en_icon */
		&& ip->back->oper2->mode==am_dreg)
			ip->opcode=op_dbra,
			ip->oper2=ip->oper1,
			ip->oper1=ip->back->oper2,
			peep_delete(ip->back);
}

static void peep_label(struct ocode *ip) {
/*
 * if a label is followed by a branch to another label, the
 * branch statement can be deleted when the label is moved
 */
	struct ocode   *prev, *next, *target;
	long			label;
	last_param_pop = NULL;	// reset function parameter popping optimization
	prev = ip->back;

	if ((next = ip->fwd) == 0 || next->opcode != op_bra || next->oper1->offset->nodetype!=en_labcon)
		return;
	/*
	 * To make this fast, assume that the label number is really
	 * getoplab(next)
	 */
	label = getoplab(next);
	if (label == getlab(ip))
		return;
	target = peep_head;
	/*
	 * look for the label
	 */
	while (target != 0) {
		if (target->opcode == op_label
			&& getlab(target) == label)
			break;
		target = target->fwd;
	}
	/* we should have found it */
	if (target == 0) {
#ifdef VCG
		if (vcg_lvl==VCG_MAX)
#endif
			iwarn(PEEP_LABEL,1);
		return;
	}
	/* move label */
	peep_delete(ip);
	ip->fwd = target->fwd;
	ip->back = target;
	target->fwd = ip;
	if (ip->fwd != 0)
		ip->fwd->back = ip;
	/* possibly remove branches */
	/* in fact, prev is always != 0 if peep_delete has succeeded */
	if (prev != 0) {
		if (prev->opcode == op_bra || prev->opcode == op_jmp
			|| prev->opcode == op_rts)
			peep_uctran(prev);
		else if (prev->opcode == op_label)
			next_ip=prev;	/* so that peep_label will be called once again (this label
							 *  might be aliased by other ones) */
	}
}

void opt3(void) {
/*
 * peephole optimizer. This routine calls the instruction specific
 * optimization routines above for each instruction in the peep list.
 */
//#define NO_PEEP
#ifndef NO_PEEP
	struct ocode   *ip;
	enum(e_op) instr;
	next_ip = peep_head;
	if (!opt_option)
		return;
	instr=-1;
	last_param_pop = NULL;
	while (next_ip != 0) {
/*		if (ip->opcode==instr)
			if (!(ip = ip->fwd)) break;
		instr=ip->opcode;*/
		ip = next_ip;
		next_ip = ip->fwd;
#ifdef AS
		if (ip->opcode!=op_label && ip->opt)
			continue;
#endif
		switch (ip->opcode) {
		  case op_move:
			peep_move(ip);
			break;
		  case op_movem:
			peep_movem(ip);
			break;
		  case op_pea:
			peep_pea(ip);
			break;
		  case op_lea:
			peep_lea(ip);
			break;
		  case op_ext: {	/* ext.l Dn ; add/sub Dn,Am (where Dn is a word) */
			struct ocode *nxt=ip->fwd; reg_t reg;
			if (!nxt || (nxt->opcode!=op_add && nxt->opcode!=op_sub)
#if defined(AS) && defined(ASM)
				|| nxt->opt
#endif
				|| ip->length!=4 || nxt->oper1->mode!=am_dreg
				|| (reg=nxt->oper1->preg)!=ip->oper1->preg
				|| nxt->oper2->mode!=am_areg
				|| (reg>MAX_DATA && regexp[reg_t_to_regexp(reg)]->esize==4))
				break;
			peep_delete(ip);
			nxt->length=2;
			} break;
		  case op_add:
		  case op_sub:
			peep_addsub(ip,ip->opcode);
			//peep_add/peep_sub(ip);
			break;
		  case op_and:
			peep_and(ip);
			break;
		  case op_clr:
			peep_clr(ip);
			break;
		  case op_cmp:
			peep_cmp(ip);
			break;
		  case op_tst:
			peep_tst(ip);
			break;
		  case op_beq:
		  case op_bne:
		  case op_bgt:
		  case op_bge:
		  case op_blt:
		  case op_ble:
		  case op_blo:
		  case op_bls:
		  case op_bhi:
		  case op_bhs:
			peep_bxx(ip);
			/* FALL THROUGH */
			last_param_pop = NULL;	// reset function parameter popping optimization
			break;
		  case op_dbxx:
			last_param_pop = NULL;	// reset function parameter popping optimization
			break;
		  case op_bra:
			last_param_pop = NULL;	// reset function parameter popping optimization
			peep_uctran(ip);
			/* delete branches to the following statement */
			{
				struct ocode   *p = ip->fwd;
				long			label = getoplab(ip);
				while (p != 0 && p->opcode == op_label) {
					if (getlab(p) == label) {
						peep_delete(ip);
						ip = 0;
						break;
					}
					p = p->fwd;
				}
			}
			if (!ip)
				break;
#ifdef SPEED_OPT
			if (next_ip && speed_opt_value>0) {	/* then it's necessarily a label, due to peep_uctran */
				int i,lab=getoplab(ip);
				struct ocode *p=peep_head,*s,*e;
				do {
					if (p->opcode==op_label && getlab(p)==lab) break;
				} while ((p=p->fwd));
				if (!p) break;
				s=p;
				lab=getlab(next_ip);
				i=speed_opt_value;
				/* NOTE : we ought never stop copying on a 'bra', but rather go on */
				do {
					if (!(p=p->fwd)) goto bad;
					if (p->opcode==op_label) i++;	/* don't count it as an instruction */
					else if (p->opcode>=_op_bcond_min && p->opcode<=_op_bxx_max
						&& getoplab(p)==lab) {
						break;
					} else if (p->opcode==op_bra || p->opcode==op_jmp || p->opcode==op_rts)
						break;
#ifndef ALLOW_TWIN_STACK_OPS
					else {
						struct amode *ap=p->oper1;
						if (ap && ap->mode==am_ainc && ap->preg==7)
							goto bad;
						ap=p->oper2;
						if (ap && (ap->mode==am_areg || ap->mode==am_adec) && ap->preg==7)
							goto bad;
					}
#endif
				} while (i--);
				if (i>=0) {
					struct ocode *prv,*nxt;
					e=p;
					p=s;
					prv=ip->back;
					next_ip=prv;
					nxt=ip->fwd;
					peep_delete(ip);
					do {
						p=p->fwd;
						if (p->opcode!=op_label) {
							struct ocode *n=(struct ocode *)
								xalloc(sizeof(struct ocode), OCODE);
							*n=*p;
							if (p==e && p->opcode>op_bra) { /* we assume that op_jmp<op_bra */
															/*  and op_rts<op_bra           */
								struct ocode *pn=p->fwd,*nl;
								int dlab;
								if (pn->opcode==op_label) dlab=getlab(pn);
								else {
									dlab=nxtlabel();
								#ifdef NEWLAB
									nl = (struct ocode *) xalloc((int) sizeof(struct lbls), OCODE);
									nl->opcode = op_label;
									((struct lbls *)nl)->lab = dlab;
								#else
									nl = (struct ocode *) xalloc((int) sizeof(struct ocode), OCODE);
									nl->opcode = op_label;
									nl->oper1 = mk_immed((long)dlab);
								#endif
									p->fwd=nl;
									nl->back=p;
									nl->fwd=pn;
									pn->back=nl;
								}
								n->opcode=revcond[p->opcode-_op_bcond_min];
								n->oper1=mk_label(dlab);
							}
							n->back=prv;
							prv->fwd=n;
							prv=n;
						}
					} while (p!=e);
					prv->fwd=nxt;
					nxt->back=prv;
					next_ip=next_ip->fwd;
				}
			}
bad:
#endif
			break;
		  case op_jmp:
		  case op_rts:
			last_param_pop = NULL;	// reset function parameter popping optimization
			peep_uctran(ip);
			break;
		  case op_label:
			peep_label(ip);
			break;
		  case _op_adj:
			peep_delete(ip);
			/* FALL THROUGH */
		  case _op_asm:
			last_param_pop = NULL;	// reset function parameter popping optimization
			break;
		}
	}
#endif
}
#ifdef VCG
#include "vcg.c"
#endif

#endif /* MC680X0 */
// vim:ts=4:sw=4
