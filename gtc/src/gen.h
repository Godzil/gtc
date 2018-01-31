/*
 * GTools C compiler
 * =================
 * source file :
 * code generator
 *
 * Copyright 2001-2004 Paul Froissart.
 * Credits to Christoph van Wuellen and Matthew Brandt.
 * All commercial rights reserved.
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 */

#ifndef GEN_H
#define GEN_H

/* addressing modes */

#ifdef MC680X0
#define F_DREG	1				/* data register direct mode allowed */
#define F_AREG	2				/* address register direct mode allowed */
#define F_MEM	4				/* memory alterable modes allowed */
#define F_IMMED 8				/* immediate mode allowed */
#define F_ALT	7				/* alterable modes */
#define F_DALT	5				/* data-alterable modes */
#define F_ALL	15				/* all modes allowed */
#define F_VOL	16				/* need volatile operand */
#define F_NOVALUE 32			/* dont need result value */
#define F_USES	  64			/* need result value more than once */
								/* (this forbids autoincrement modes) */
#define F_DEREF 128				/* for derefenced use only (eg allow #L1) */
#define F_SRCOP 256				/* for source operand only (eg allow L1(pc)) */
#endif /* MC680X0 */

#ifdef INTEL_386
#define F_REG		1			/* a register, like %eax, %edi etc. */
#define F_MEM		2			/* direct, indirect, indexed */
#define F_IMMED 	4			/* immedate */
#define F_FPSTACK	8			/* top of floating-point stack */
#define F_NOVALUE  16			/* dont need result value */
#define F_VOL	   32			/* need scratch register */
#define F_ALL	   15			/* any mode */
#define F_NOEDI    64			/* do not use %edi and %esi */
#endif /* INTEL_386 */

/* The instructions */

#ifdef MC680X0
/*
 * The order of the branch instructions must not be changed
 * since array revcond[] in the peephole optimizer and 
 * truejp/falsejp in the code generator rely on them.
 * Beware of OUT_AS too.
 * In addition, peep::SPEED_OPT assumes that (op_jmp,op_rts)<op_bra<_op_cond
 */
enum e_op {
	// 0x00
	op_move, op_moveq, op_clr, op_lea,	/* destructive instructions */
	op_add, op_addi, op_addq, op_sub, op_subi,
	op_subq, op_muls, op_mulu, op_divs, op_divu, op_and, op_andi,
	// 0x10
	op_or, op_ori, op_eor, op_eori, op_lsl, op_lsr, op_jmp, op_jsr, op_movem,
	op_rts, op_bra, op_bsr, op_beq, op_bne,
	// 0x1E
	op_bhs, op_bge, op_bhi, op_bgt, op_bls, op_ble, op_blo, op_blt,	/* unsigned, signed */
	op_tst, op_ext, op_swap,
	// 0x29
	op_neg, op_not, op_cmp, op_link, op_unlk, op_label,
	op_pea, op_cmpi, op_dbra, op_asr, op_bset, op_bclr, op_bchg, _op_asm, _op_adj,
#ifdef INFINITE_REGISTERS
	_op_cleanup_for_external_call,
#endif
	// 0x38
#ifdef ASM
	/* ASM-only */
	op_asl, op_rol, op_ror, op_roxl, op_roxr, op_btst, op_exg, op_dc, op_ds, op_dcb,
	// 0x42
	op_bvs, op_bvc, op_bpl, op_bmi, op_trap, op_negx, op_addx, op_subx,
	// 0x4A
	op_chk, op_even,
	/*op_dbt,*/ /*op_dbf,*/ op_dbeq, op_dbne,
#ifndef LIGHT_DBXX_AND_SXX
	op_dbhs, op_dbge, op_dbhi, op_dbgt, op_dbls, op_dble, op_dblo, op_dblt,/* unsigned, signed */
	op_dbvs, op_dbvc, op_dbpl, op_dbmi,
	op_st,  op_sf,  op_seq, op_sne,
	op_shs, op_sge, op_shi, op_sgt, op_sls, op_sle, op_slo, op_slt,	/* unsigned, signed */
	op_svs, op_svc, op_spl, op_smi,
	op_tas,
#endif
#endif
};
#define op_destroy(__op) (__op<=op_lea)
#define _op_bxx_min op_bra
#define _op_bcond_min op_beq
#define _op_bxx_max op_blt
#ifdef ASM
#define op_bxx op_bra: case op_bsr: case op_beq: case op_bne: case op_bhs: case op_bge: case op_bhi: case op_bgt: case op_bls: case op_ble: case op_blo: case op_blt: case op_bpl: case op_bmi: case op_bvc: case op_bvs
#else
#define op_bxx op_bra: case op_bsr: case op_beq: case op_bne: case op_bhs: case op_bge: case op_bhi: case op_bgt: case op_bls: case op_ble: case op_blo: case op_blt
#endif
#define op_bcond op_beq: case op_bne: case op_bhs: case op_bge: case op_bhi: case op_bgt: case op_bls: case op_ble: case op_blo: case op_blt
#define op_dbxx op_dbra: case op_dbeq: case op_dbne
#ifdef ASM
#ifndef LIGHT_DBXX_AND_SXX
#define _op_max op_tas
#else
#define _op_max op_dbne
#endif
#else
#define _op_max _op_adj
#endif
#endif

#ifdef INTEL_386
enum e_op {
	op_movsbl, op_movzbl, op_movswl, op_movzwl,
	op_movsbw, op_movzbw,
	op_cltd,
	op_mov, op_lea,
	op_not, op_neg,
	op_add, op_sub, op_imul, op_idiv,
	op_and, op_or, op_xor,
	op_inc, op_dec,
	op_cmp,
	op_push, op_pop,
	op_jmp, op_bra, op_call, op_leave, op_ret,
	op_test,
	op_je, op_jne,
	op_jl, op_jle, op_jg, op_jge,
	op_ja, op_jae, op_jb, op_jbe,
	op_rep, op_smov,
	op_shl, op_shr, op_asr,
	op_fadd, op_fsub, op_fdiv, op_fmul,
	op_fsubr, op_fdivr,
	op_fld, op_fst, op_fstp, op_fpop,
	op_fild, op_ftst,
	op_fchs, op_fcompp, op_fnstsw, op_sahf,
	op_label
};
#endif

#ifdef MC680X0
/* CAUTION : changing this order requires changing sz_table, */
/*  modifying out68k_as::Pass1()                             */
/*  and requires am_immed to be right before am_direct and   */
/*  am_areg right before am_ind                              */
/*  - in addition, changing am_dreg is not recommended... -  */
enum e_am {
	am_dreg, am_areg, am_ind, am_ainc, am_adec, am_indx, am_indx2,
	am_indx3, am_immed, am_direct, am_mask1, am_mask2,
	/* AS-only (subcases of am_direct) : */
	am_pcrel, am_dirw, am_dirl,
	/* ASM-only */
	am_sr, am_ccr, am_usp,
};
#define am_deref(x) ((x)+1)
#define am_doderef(x) (++(x))
#define am_is_increment(x) ((x)==am_ainc || (x)==am_adec)
#endif

#ifdef INTEL_386
enum e_am {
	am_reg, am_indx, am_indx2, am_direct, am_immed, am_star, am_fpstack
};
#endif

#ifdef INFINITE_REGISTERS
#if 0
typedef struct {
	int n;
} reg_t;
#define init_reg_t(x) {x}
#define nil_reg_t {-1}
#else
typedef int reg_t;
#define init_reg_t(x) (x)
#define nil_reg_t (-1)
#endif
#define FIRSTREG    1000
#define MAX_ADDR  100000
#define MAX_DATA  100000
#define AREGBASE  400000
#define TDREGBASE 300000
#define TAREGBASE 700000
#define reg_t_to_regexp(x) ((x)>=AREGBASE?2*((x)-AREGBASE-MAX_ADDR)+1:2*((x)-MAX_DATA))
#define REGEXP_SIZE 20000
#else
#define reg_t char
#define init_reg_t(x) (x)
#define nil_reg_t 0
#define AREGBASE 8
#define FIRSTREG 0
#define reg_t_to_regexp(x) (x)
#define REGEXP_SIZE 16
#define TDREGBASE 0
#endif
#define CONVENTION_MAX_DATA 2
#define CONVENTION_MAX_ADDR 1

/* addressing mode structure */

struct amode {
	enum(e_am)		mode;
	struct enode   *offset;
/*
 * these chars may be unsigned...
 */
	reg_t			preg, sreg;
	char			deep, slen;
};

#define NIL_AMODE ( (struct amode *) 0)

/* output code structure */

struct ocode {
	enum(e_op)		opcode;
	struct ocode   *fwd, *back;
	struct amode   *oper1, *oper2;
	int 			length;
#ifdef DB_POSSIBLE
	int				line;
#endif
#ifdef AS
	char			sz,opt;
#endif
};
struct lbls {
	enum(e_op)		opcode;
	struct ocode   *fwd, *back;
	int 			lab;
};

/* register naming, special registers */

#ifdef MC680X0
#ifdef INFINITE_REGISTERS
#define RESULT	  (TDREGBASE+0) 			/* register returning function results */
#define PRESULT   (TAREGBASE+0) 			/* register returning pointer function results */
#ifdef USE_LINK
#define FRAMEPTR  (TAREGBASE+6)			/* frame pointer register */
#else
#define TRUE_FRAMEPTR (TAREGBASE+6)
#define FRAMEPTR  (TAREGBASE+8)			/* frame pointer pseudo-register */
#endif
#define STACKPTR  (TAREGBASE+7)			/* system stack pointer register */
#else
#define RESULT	  0 			/* register returning function results */
#define PRESULT   8 			/* register returning pointer function results */
#ifdef USE_LINK
#define FRAMEPTR  14			/* frame pointer register */
#else
#define TRUE_FRAMEPTR 14
#define FRAMEPTR  16			/* frame pointer pseudo-register */
#endif
#define STACKPTR  15			/* system stack pointer register */
#endif
#endif /* MC680X0 */

#ifdef INTEL_386
#define RESULT			0		/* reg ret. integer-type function results */
#define EAX 			0
#define EDX 			1
#define EBX 			2
#define ECX 			3
#define ESI 			4
#define EDI 			5
#define ESP 			6
#define STACKPTR		6		/* system stack pointer */
#define EBP 			7		/* frame pointer */
#define FRAMEPTR		7
/* attention: same order as above */
#define AX				8
#define DX				9
#define BX				10
#define CX				11
#define SI				12
#define DI				13
/* attention: same order as above */
#define AL				14
#define DL				15
#define BL				16
#define CL				17

#define NUMREG			REG8(EBP)

/*
 * The code generator does not distinguish between %eax, %ax, %al
 * because some assemblers want it strict, the real register names
 * are determined when the assembly instruction is PRINTED, e.g.
 * code generator produces movb junk,%eax,
 * assembly code printer prints movb junk,%al
 * The conversion is done by the following macros
 */
#define REG16(X) ((X)-EAX+AX)
#define REG8(X)  ((X)-EAX+AL)
#endif

#ifdef MC680X0
#ifndef INFINITE_REGISTERS
#define 		MAX_REG_STACK	30

#define MAX_ADDR 1				/* max. scratch address register (A1) */
#define MAX_DATA 2				/* max. scratch data	register (D2) */
#endif
#endif
#ifdef INTEL_386
#define MAX_REG  1				/* scratch registers: %eax..%edx */
#endif

struct reg_struct {
	enum(e_am)		mode;
	int 			reg;
	int 			flag;		/* flags if pushed or corresponding reg_alloc
								 * number */
};

struct amode   *g_expr();
struct amode   *mk_reg();
#ifdef MC680X0
struct amode   *temp_data();
struct amode   *temp_addr();
#endif
#ifdef INTEL_386
struct amode   *get_register();
#endif
struct amode   *mk_offset();
struct amode   *g_cast();
struct amode   *g_fcall();
struct amode   *func_result();
struct amode   *as_fcall();
#ifdef MC68000
struct amode   *mk_rmask();
struct amode   *mk_smask();
#endif
struct amode   *mk_label();
struct amode   *mk_strlab();
struct amode   *mk_immed(long i);
struct amode   *g_offset();
struct amode   *mk_legal();
struct amode   *copy_addr();
#endif
void truejp();
void falsejp();

void g_code(enum(e_op) op, int len, struct amode *ap1, struct amode *ap2);

void initstack();
void freeop(struct amode *ap);
// vim:ts=4:sw=4
