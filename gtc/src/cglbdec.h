/*
 * GTools C compiler
 * =================
 * source file :
 * C global declarations
 *
 * Copyright 2001-2004 Paul Froissart.
 * Credits to Christoph van Wuellen and Matthew Brandt.
 * All commercial rights reserved.
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 */

#ifndef CGLBDEC_H
#define CGLBDEC_H
#ifdef PC
#define outstr stderr
#define msg(s) fprintf(outstr,s)
#define msg2(s,a) fprintf(outstr,s,a)
#define msg3(s,a,b) fprintf(outstr,s,a,b)
#define msg4(s,a,b,c) fprintf(outstr,s,a,b,c)
#define msg5(s,a,b,c,d) fprintf(outstr,s,a,b,c,d)
#define msg6(s,a,b,c,d,e) fprintf(outstr,s,a,b,c,d,e)
#define vmsg(s,va) vfprintf(outstr,s,va)
#else
#define msg printf
#define msg2(s,a) printf(s,a)
#define msg3(s,a,b) printf(s,a,b)
#define msg4(s,a,b,c) printf(s,a,b,c)
#define msg5(s,a,b,c,d) printf(s,a,b,c,d)
#define msg6(s,a,b,c,d,e) printf(s,a,b,c,d,e)
#undef vcbprintf
#define vcbprintf ({register long __a=32+*(long*)(*(long*)0xC8+0x14C); \
  (void(*)(void(*)(char,void**),void**,char*,void*))(__a+*(short*)__a);})
#define vmsg(s,va) vcbprintf((void(*)(char,void**))fputchar,NULL,s,va)
#endif

#define int_bits 16
#define tp_int tp_short
#define tp_uint tp_ushort
#define fpu_option 0
#define short_option 1
#ifdef LISTING
#define list_option 1
#else
#define list_option 0
#endif
#define reg_option 1
#define opt_option 1
#define warn_option 1
#define trans_option 0
#ifdef ICODE
#define icode_option 1
#else
#define icode_option 0
#endif
enum e_xt {
	X_MID_DECL=0x1, X_COMP_STRING=0x2, X_LANG_EXT=0x4,
};
#define flags_fullgtc -1 /* all flags, even those that break compatibility */
#define flags_basegtc (X_MID_DECL|X_COMP_STRING) /* all flags, except those that break compatibility */
#define flags_cplusplus X_MID_DECL
#define flags_ansi 0
extern int		flags;
#ifdef SPEED_OPT
extern int		speed_opt_value;
extern int		default_speed_opt_value;
#endif
extern int		verbose;
#ifdef PC
enum {
	VERBOSITY_PRINT_SEARCH_DIRS=0x1,
	VERBOSITY_PRINT_INCLUDES=0x2,
};
extern int		verbosity;
#endif

extern FILE    *input, *list, *output;
extern char *outputname,*calcname;
extern char proj_file[];
extern char export_pfx[]; extern short_size export_pfx_len;
#ifdef ICODE
extern FILE    *icode;
#endif
extern int		nextlabel;
extern int		lastch;
extern enum(e_sym) lastst;
#ifdef PC
extern enum(e_sym) cached_sym;
#else
extern enum(e_sym) _cached_sym;
#endif
extern int		cached_flag;
extern char 	lastid[MAX_ID_LEN+1];
extern int		lastcrc;
extern int		lastreg;
extern struct sym *lastsp;
extern char 	laststr[MAX_STRLEN + 1];
extern int		lstrlen;
extern unsigned long ival;
#ifndef NOFLOAT
extern double	rval;
#endif

extern HTABLE	gsyms, lsyms, labsyms, gtags, ltags;

extern struct slit *strtab;
extern struct slit *datatab;
extern XLST_TYPE 	lc_auto;
extern XLST_TYPE 	reg_size;
extern XLST_TYPE 	max_scratch;
extern XLST_TYPE 	act_scratch;
extern long 	lc_bss; 		/* local bss counter */
extern int		global_flag;
extern int		global_strings;
extern int		temp_mem,temp_local,min_temp_mem;
extern int		asm_flag,asm_xflag,asm_zflag;
extern int		nostub_mode;
extern int		exestub_mode;
#ifdef MC680X0
extern unsigned int save_mask;		/* register save mask */
#endif
extern char    *declid;
extern int		nparms;
extern int		store_lst;
extern int		pch_init;
extern int		middle_decl;
extern char    *names[MAX_PARAMS];
extern int		bit_offset; 	/* the actual offset */
extern int		bit_width;		/* the actual width */
extern int		bit_next;		/* offset for next variable */
extern int		decl1_level;

#ifdef ASM
extern struct ocode *asm_head;
#endif
#ifdef AS
extern struct ocode *scope_head;
#else
#ifdef ASM
extern struct ocode *peep_head;
#endif
#endif

#ifdef FLINE_RC
extern int fline_rc;
#endif

//extern int	  list_option;
//extern int	  short_option;
//extern int		opt_option;
//extern int	  trans_option;
//extern int		warn_option;
//extern int	fpu_option;
//extern int	reg_option;
//#ifdef ICODE
//extern int		icode_option;
//#endif
extern TYP	   *ret_type;

extern int		regptr;
extern XLST_TYPE reglst[REG_LIST];
extern int		autoptr;
extern XLST_TYPE autolst[AUTO_LIST];

extern struct enode *init_node;
#ifdef MID_DECL_IN_EXPR
extern struct enode *md_expr;
extern struct typ	*md_type;
#endif

#ifdef VERBOSE
extern struct tms tms_buf;
extern long 	decl_time, parse_time, opt_time, gen_time, flush_time;
#endif							/* VERBOSE */

extern readonly TYP		tp_void, tp_long, tp_ulong, tp_char, tp_uchar;
extern readonly TYP		tp_short, tp_ushort, tp_float;
extern readonly TYP		tp_econst, tp_string, tp_void_ptr, tp_int, tp_uint, tp_func;
extern readonly TYP		tp_double;

//extern int	  int_bits;

#ifdef MC680X0
extern readonly struct amode push, pop;
#define push_am (struct amode *)&push
#define pop_am (struct amode *)&pop
#endif
/*extern int uses_structassign;*/
extern int regs_used;
extern int pushed;
extern int is_leaf_function, uses_link;
#ifdef INTEL_386
extern int edi_used, esi_used, ebx_used;
#endif

extern int		lineno,lineid,lastch;
extern int		prevlineid,cached_lineid;

extern int err_force_line,err_cur_line,err_assembly;

#ifdef DECLARE
extern TYP	   *head, *tail;

extern int		total_errors;

/*extern FILE	  *input,
				*list,
				*output,
				*bin;*/

//extern long	  ival;
extern double	rval;

extern SYM			   *lasthead;
extern struct slit	   *strtab;
extern struct slit	   *datatab;
extern int			   lc_static;
//extern int			 lc_auto;
extern struct snode    *bodyptr;
extern int			   global_flag;
//extern int			 save_mask; 		 /* register save mask */
extern SYM				*func_sp;

extern TYP *lastexpr_tp;
extern int lastexpr_size, lastexpr_type;
#endif
extern struct amode *lastexpr_am;
extern int	need_res;

extern SYM *func_sp;

#ifdef TWIN_COMPILE
extern int twinc_necessary;
extern int *twinc_info,*twinc_prev;
#endif

SYM *search(char *na, int crc, HTABLE *tab);
void dodecl(enum(e_sc) defclass);
int eq_type(TYP *tp1, TYP *tp2);

#ifndef integral
int integral(TYP *tp);
#endif
int isshort(struct enode *node);
int isbyte(struct enode *node);

int radix36(char c);

int equal_address();

int crcN(char *na);

#ifdef ASM
void asm_getsym();
#endif

extern HTABLE defsyms;

#define MAX_IF_DEPTH 50
extern int ifdepth,ifreldepth,ifskip,ifsd;
extern int ifcount[MAX_IF_DEPTH];
extern int ifval[MAX_IF_DEPTH];
#endif

#ifndef PCHS_MODES
#define PCHS_MODES
enum { PCHS_ADD=1, PCHS_UNLOAD=-1 };
#endif

#ifdef USE_MEMMGT
/*
 * Memory is allocated in blocks of 4 kb, which form a linked list
 */

#define BLKLEN 4096

#ifndef BLK_STRUCT
#define BLK_STRUCT
struct blk {
	char			m[BLKLEN];	/* memory area */
	struct blk	   *next;
};
#endif

#define NIL_BLK ((struct blk *)0)

extern int		glbsize,	/* size left in current global block */
				locsize,	/* size left in current local block */
				glbindx,	/* global index */
				locindx;	/* local index */

extern int		glbmem,locmem;

extern struct blk *locblk,	/* pointer to local block list */
				*glbblk;		/* pointer to global block list */

void tmp_use();
void tmp_free();
#endif
// vim:ts=4:sw=4
