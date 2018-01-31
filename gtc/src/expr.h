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

#ifndef EXPR_H
#define EXPR_H

/* expression tree descriptions  */

//enum e_node {
//	en_void,
//	en_icon/*1*/, en_fcon, en_labcon/*3*/, en_nacon/*4*/, en_autocon/*5*/,
//	en_fcall, en_tempref, en_add, en_sub, en_mul, en_mod,
//	en_div, en_lsh, en_rsh, en_cond, en_assign/*16*/, en_eq, en_ne,
//	en_asadd, en_assub, en_asmul, en_asdiv, en_asmod, en_asrsh,
//	en_asxor,
//	en_aslsh, en_asand, en_asor, en_uminus, en_not, en_compl,
//	en_lt/*32*/, en_le, en_gt, en_ge, en_and, en_or, en_land, en_lor,
//	en_xor, en_ainc, en_adec,
//	en_ref, en_cast, en_deref,
//	en_fieldref,
//};
/* BEWARE!!! en_add must be even, and en_sub=en_add+1 (used in opt0) */
enum e_node {
	// 0x00
	en_icon, en_fcon, en_labcon, en_nacon, en_autocon,
	// 0x05
	en_tempref, en_ref, en_fieldref,
	// 0x08
	en_void,
	en_compound,
	// 0x0A
	en_fcall, en_cast, en_add, en_sub, en_mul, en_mod,
	// 0x10
	en_div, en_lsh, en_rsh, en_cond, en_assign, en_eq, en_ne,
	// 0x17
	en_asadd, en_assub, en_asmul, en_asdiv, en_asmod, en_asrsh,
	en_asxor,
	// 0x1E
	en_aslsh, en_asand, en_asor, en_uminus, en_not, en_compl,
	en_lt, en_le, en_gt, en_ge, en_and, en_or, en_land, en_lor,
	en_xor, en_ainc, en_adec,
	en_deref,
// extensions
	en_alloca,
};
/* below is the list of all nodes that need to have finer type considerations than below :
  - char/uchar
  - short/ushort
  - long/ulong/pointer
  - float
  - struct
  - union
  - void
  (these are considerations about the *parent* node, not about its children, whose type
  may safely be accessed)
 */
#define needs_trivial_casts(x) ( \
	/* pointer vs long/ulong */ \
	(x)==en_fcall || \
	/* signed vs unsigned */ \
	(x)==en_div || (x)==en_mod || (x)==en_mul || (x)==en_rsh || \
	(x)==en_asdiv || (x)==en_asmod || (x)==en_asmul || (x)==en_asrsh || \
	0)

/* statement node descriptions	 */

enum e_stmt {
	st_expr, st_while, st_for, st_do, st_if, st_switch,
	st_case, st_goto, st_break, st_continue, st_label,
	st_return, st_compound, st_default,
	// GTC extensions
	st_loop, st_asm,
};

struct xcon {
	enum(e_node) 	nodetype;
	enum(e_bt)		etype;
	long			esize;
	union {
		long			i;
#ifndef XCON_DOESNT_SUPPORT_FLOATS
#ifndef NOFLOAT
		double			f;
#endif
#endif
	}				v;
};

struct enode {
	enum(e_node) 	nodetype;
	enum(e_bt)		etype;
	long			esize;
	union {
		long			i;
#ifndef NOFLOAT
		double			f;
#endif
		int				enlab;
		char		   *__sp[2];
		struct enode   *p[2];
		struct snode   *st;
	}				v;
	char			bit_width, bit_offset; /* possibly unsigned char */
};

#ifdef AS
	#define ensp __sp[1]
#else
	#define ensp __sp[0]
#endif
int not_lvalue(struct enode *node);
#define g_lvalue(node) (!not_lvalue(node))
#define lvalue(node) (!not_lvalue(node))
#define NIL_ENODE ( (struct enode *) 0)

struct snode {
	enum(e_stmt) 	stype;
	struct snode   *next;		/* next statement */
	struct enode   *exp;		/* condition or expression */
	struct snode   *s1; 		/* internal statement */
	union {
		struct enode   *e;		/* condition or expression */
		struct snode   *s;		/* internal statement (else) */
		long			i;		/* (case)label or flag */
	}				v1, v2;
	unsigned int	count;		/* execution count */
#ifdef DB_POSSIBLE
	int				line;
#endif
};

#define NIL_SNODE ( (struct snode *) 0)


struct cse {
	struct cse	   *next;
	struct enode   *exp;		/* optimizable expression */
	unsigned long	uses;		/* number of uses */
	unsigned long	duses;		/* number of dereferenced uses */
	short			voidf;		/* cannot optimize flag */
#ifdef INFINITE_REGISTERS
	int 			reg;		/* allocated register */
#else
	short			reg;		/* allocated register */
#endif
};

struct enode   *mk_node(enum(e_node) nt, struct enode *v1, struct enode *v2);
struct enode   *mk_icon(long v);
struct enode   *parmlist();
struct enode   *copynode();
struct snode   *statement();
struct snode   *compound();

void opt4(struct enode **);
#endif
// vim:ts=4:sw=4
