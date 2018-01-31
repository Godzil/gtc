/*
 * GTools C compiler
 * =================
 * source file :
 * #define's
 *
 * Copyright 2001-2004 Paul Froissart.
 * Credits to Christoph van Wuellen and Matthew Brandt.
 * All commercial rights reserved.
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 */

//#define SHOWSTEP

#ifndef DEFINE_H
#define DEFINE_H
#if 0
	#ifdef PC
	#define debugcls() (void)0
	#define debugf printf
	#else
	#define debugcls clrscr
	extern int progr;
	#define debugf(f...) (printf(f),progr_process(func_sp?func_sp->name:0,curname,progr),ngetchx())
	#endif
	#define debugfq printf
#else
	#define debugcls() (void)0
	#if defined(PC) && !defined(__GNUC__)
	/*@unused@*/ static void debugf(char *s,...) {}
	/*@unused@*/ static void debugfq(char *s,...) {}
	#else
	#define debugf(f...) (void)0
	#define debugfq(f...) (void)0
	#endif
#endif
#if !defined(PC) || !defined(DEBUGGING)
#define enum(tp) unsigned int // some compilers make enums signed, make them happy
#else
#define enum(tp) enum tp // we want nice debugging output
#endif
#define IS_VALID >=0
#define IS_INVALID <0
#define nxtlabel() (short)(nextlabel++)
#ifdef AS
	#define nxtglabel() (short)(glblabel++)
#else
	#define nxtglabel() (short)(nextlabel++)
#endif
#define local(__l) (__l>=0)
#define global(__l) (((unsigned short)__l)<0xC000) // only valid if !local(__l)
#define external(__l) (((unsigned short)__l)>=0xC000)
#ifdef PC
#define splbl(__sp) (__sp->value.splab?__sp->value.splab:(__sp->value.splab=label(__sp->name)))
#else
#define splbl(__sp) (__sp->value.splab?:(__sp->value.splab=label(__sp->name)))
#endif
#ifdef PC
#define TWIN_COMPILE
#endif
#define DUAL_STACK	// save space on the stack by allocating on a temporary buffer just the size we need
#ifdef PC
#define OBJ_OUT
#endif

//#define INFINITE_REGISTERS // compile for a virtual machine with an arbitrarily high number of registers
#ifdef DISABLE_OPTS
#undef VCG
#endif
#define USE_WORDS_IN_SWITCH // use dc.w instead of dc.l in 'real' switch statements
#define NEW_ATTR	// allows TI-GCC-like attribute declaration
#ifdef PC
#ifndef DISABLE_OPTS
#define ADV_OPT		// is it worth the extra exe size & comp time ? => disabled on-calc
#endif
#endif
#ifdef VCG
#define GENERATE_ROL_ROR	// optimize (x<<12)|(x>>4) into a ror; depends on VCG because commutativity is not tested
#endif
//#define NO_ICONST_SCAN
//#define ALTERNATE_HOOK
#define OPT_ROMCALLS	// assumes that *(void **)0xC8 is never modified
//#define DEFAULT_STKPARM
#define TWOBYTE_ARRAY_EXTRA_OPT	// {add.w d3,a0;add.w d3,a0} instead of
								// {move.w d3,d0;add.w d0,d0;add.w d0,a0}
//#define AREG_AS_DREG	// some fine-tune should be done before this switch provides any benefit
#define PREFER_POS_VALUES	// allows for better CSE collection (seems like there's a bug in
							//  Optimize which is fixed or reduced by this switch too...)
#define ADVANCED_TEMP_REGS	// to be fit one day in Gen68k in order to optimize free temp regs
#if 0
#define ADVANCED_MEMORY_MOVE	// optimize contiguous sequences of move's (< auto_init/g_fcall)
								// (rather time-consuming, for very low consequences...
								//   => disabled, even on PC)
#endif
#ifdef PC	// only valid on PC!
#define REQ_MGT	// enable required files management
#endif
#define MULTIFILE	// enable multiple file support
#define OPTIMIZED_AINCDEC_TEST	// allows for dbf too
#define LONG_ADDS_IN_AREG	// so that lea can be used rather than addi.l
//#define NO_OFFSET_AUTOCON	// useless as of now
#define ADD_REDUNDANCY	// increases compression ratios at the cost of function-overhead speed
#define REGALLOC_FOR_SIZE
//#define HARDCORE_FOR_UNPACKED_SIZE	// favour size wherever possible, with little improvement
								// of packed size and even more probably regression (lsl.w #2...)
#define SPEED_OPT
//#define speed_opt_value 1	// we may add n instruction before the bra (1 : useful for dbf)
//#define G_CAST2	// this might remove some op_ext's : to be tested
#define RAM_CALLS	// support for RAM_CALLs
#ifdef PC
#define FLINE_RC	// support for F-Line ROM_CALLs
#endif
#define SHORT_STRUCT_PASSING	// allows HSym to be defined as a structure
#ifdef PC
#define EXE_OUT		// output an EXE program (implementation incomplete)
#endif
#define NO_SECURE_POINTERS	// assert that all array accesses are less than 32kb
//#define POP_OPT	// still buggy as of now
#define NO_TRIVIAL_CASTS	// suppresses trivial en_cast's ; might add bugs, but not certain
#define NO_VERYTRIVIAL_CASTS // suppresses obvious en_cast's ; was already there, but possibly buggy
#define MID_DECL	// allows for powerful middle decl handler
#define MINIMAL_SIZES	// so that (long)x>>(char)y won't be converted to (long)x>>(long)y
//#define USE_SWAP_FOR_SHIFT16	// use op_swap wherever possible for shifts (buggy, FIXME)
#define AUTOINIT_PAD	// pad auto_init statements ('side' requires this, for example)
#define MACRO_PPDIR
#define MEXP_SUPPORTS_PCH	// this is mandatory for TiGccLib's RR_* support.
							//  The way speed is affected I don't know, but perhaps it's
							//  faster with this option enabled... (slightly more PCH
							//  searches, very slightly fewer HTable searches)
							// Some tests should be performed about this.
// TODO:
// - add FAVOUR_RELATIVE_BSS directive so that once a BSS is CSE'd, all other BSS
//  refer to it rather than to the classical BSS (what's more, CSE::scan should favour
//  such optimizations by doing some sort of most_used_BSS_cse->desire += (other_BSS_cse's)>>1)
//   This would allow for intensive use of BSS :)
// - add RELATIVE_CSE directive so that when constantly using mylib__0000+2,
//  CSE searches for all mylib__0000 occurrences and finds out that it's better to use
//  mylib__0000+2 (replacing should take this into a count)
//   -- but complicated and not *that* useful...
#if !defined(NONSTD_MACRO_CONCAT) && !defined(NONSTD_MACRO_CONCAT_V2)
#define STD_MACRO_CONCAT
#endif
#define EXT_AS_ID	// enabled since the dirty getch() management of .b/.w/.l is no more necessary
#if defined(NOFLOAT) || !defined(BCDFLT)
#define NOBCDFLT
#endif
#ifndef AS
#ifdef AS92p
#define FORBID_TABLES
#endif
#endif
#define push_local_mem_ctx() { int _locsize=locsize,_locindx=locindx; \
	struct blk *_locblk=locblk; locblk=0; locsize=0
#define pop_local_mem_ctx() rel_local(); locsize=_locsize; locindx=_locindx; locblk=_locblk; }
#define put_align(__a) put_align2()
#ifdef VCG
#define VCG_MAX 5
#endif
#ifdef EXE_OUT
#ifdef PC
//#ifndef _DEBUG
#define SIZE_STATS	// include statistics to determine the part of code generated from C code
//#endif
#endif
#endif
#ifdef FLASH_VERSION
#define HAS_CMDLINE_EXTENSION
#endif
#ifdef BIGSTACK
	#define XLST_TYPE long
	#define REG_PARAMS 0xF000
	#define REG_PARAMLOG 12
#else
	#define XLST_TYPE int
	/* limits the param size to 2 kb - not much of a problem ;) */
	#define REG_PARAMS 0x7800
	#define REG_PARAMLOG 11
#endif
#ifndef HAS_TI_SHORT
#ifdef PC
typedef struct {
	unsigned char x0,x1,x2,x3;
} TI_LONG;
typedef struct {
	unsigned char hi,lo;
} TI_SHORT;
#else
typedef unsigned long TI_LONG;
typedef unsigned short TI_SHORT;
#endif
#ifdef PC
extern unsigned long _w2ul(TI_LONG *p);
extern unsigned short _w2us(TI_SHORT *p);
#define w2ul(x) _w2ul(&(x))
#define w2us(x) _w2us(&(x))
#else
#define w2ul(x) (x)
#define w2us(x) (x)
#endif
#endif
#ifndef NOFLOAT
#ifndef PC
#ifndef BCDFLT
#define double long
#endif
#endif
#endif
#ifndef NOBCDFLT
#define BCDLEN 8
typedef struct bcd {
	unsigned short exponent;
	unsigned char mantissa[BCDLEN];
} BCD;
#endif
#ifdef PC
	#define infunc(f) if (!strcmp(func_sp->name,f))
	#define bkpt() printf("")
	#define DB_POSSIBLE
	#define DEBUG 1
	#ifdef SHORT_INT
		#define int short
	#endif
	#ifdef CMAIN_C
		long alloc[3][0xE000];
	#else
		extern long alloc[][0xE000];
	#endif
	enum myEnum {
		ANONYMOUS=0, COPYNODE=1, ENTERNODE=2, STRINGLIT=3, LITLATE=4,
			STRSAVE=5, MK_TYPE=6, _DECLARE=7, DECLENUM=8, ENUMBODY=9,
			DECLSTRUCT=0xA, SV_TYPE=0xB, MK_NODE=0xC, MK_ICON=0xD,
			NAMEREF=0xE, FUNCBODY=0xF, MK_INT=0x10, MK_SCRATCH=0x11,
			MK_LABEL=0x12, MK_IMMED=0x13, MK_OFFSET=0x14, COPY_ADDR=0x15,
			G_DEREF=0x16, G_EXPR=0x17, MK_REG=0x18, MK_MASK=0x19,
			MK_STRLAB=0x1A, GENLOOP=0x1B, G_CODE=0x1C, G_LABEL=0x1D,
			DODEFINE=0x1E, G_PUSH=0x1F, TEMP_DATA=0x20, TEMP_ADDR=0x21,
			SYMBOL=0x22, REF_ADD=0x23, LAB_ADD_REF=0x24, RT_ADD_REF=0x25,
		ENODE=0x1000, CSE=0x2000, SLIT=0x3000, STR=0x4000, _TYP=0x5000,
			_SYM=0x6000, AMODE=0x7000, OCODE=0x8000, _TABLE=0x9000,
			SNODE=0xA000, _TAB=0xB000, _REF=0xC000, _RT=0xD000,
	};
	#define xalloc(s,c) (alloc[temp_mem?2:!!global_flag][(c)]+=(s)+((s)&1),_xalloc(((s)+3)&-4))
	#define _FILE(x)
	#define dseg() while (0)
	#define cseg() while (0)
	#define compound_done getsym
	#ifdef _MSC_VER
		#pragma warning (disable : 4033 4013 4716 4715 4133)
		#pragma warning (disable : 4996) // strcpy/fopen/... are declared deprecated in VC++ 2005 :D
		#ifdef SHORT_INT
			#pragma warning (disable : 4305 4244 4761)
		#endif
	#endif
	#ifdef SHORT_INT
		#undef int
	#endif
	#define debug(x) ((void)0)
	#include <memory.h>
	#include <string.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <ctype.h>
	#ifndef _MSC_VER
	#ifndef __MINGW32__
	#include <alloca.h>
	#else
	#include <malloc.h>
	#endif
	#else
	#define alloca _alloca
	#endif
	#ifdef SHORT_INT
		#define int short
	#endif
	#ifndef max
		#define max(a,b) ((a)>(b)?(a):(b))
	#endif
    #define getline getline_gtc
	typedef size_t short_size;
#else
	#define infunc(f) if (0)
	#define bkpt() {}
	#define DB_POSSIBLE
	#define DEBUG 0
	#define xalloc(__s,__c) _xalloc(__s+(__s&1))
	//#define _FILE(x) asm(".ascii \""x"\";.even");
	#define _FILE(__x)
	#define cached_sym (int)_cached_sym
	#define AS92p
	#ifndef CMAIN_C
		#define NOSTUB
		#define _rom_call(type,args,ind) (*(type(**)args)(*(long*)0xC8+0x##ind*4))
	#else
		#include	"lib_map.h"
		#define USE_KERNEL
		//#define USE_FULL_LONGMUL_PATCH
		//#define USE_FULL_LONGDIV_PATCH
		int _ti89;
	#endif
	#ifndef gtc_compiled
		#include <tigcclib.h>
	#else
		#define FILE void
		#define NULL (void *)0
	#endif
	#undef off
	//#define stkparm
  #ifndef FLASH_VERSION
	#define debug(x) ((void)0)
	#undef clrscr
	#define clrscr gtcrt__0000
	#undef printf
	#define printf gtcrt__0001
	#undef vcbprintf
	#define vcbprintf gtcrt__0002
	#undef fopen
	#define fopen gtcrt__0008
	FILE *fopen(const char *,const char *);
	#undef fclose
	#define fclose gtcrt__0009
	#undef fputc
	#define fputc gtcrt__000a
	int fputc(int,FILE *);
	#undef fgetc
	#define fgetc gtcrt__000b
	#undef fwrite
	#define fwrite gtcrt__000c
	int fwrite(void *,int,int,FILE *);
	#undef fputs
	#define fputs gtcrt__0010
	#undef fgets
	#define fgets gtcrt__0011
	char *fgets(char *,int,FILE *);
	#undef fprintf
	#define fprintf gtcrt__0014
  #else
	#define debug(x) ((void)0)	// for flash ver
    #undef fclose
    #define fclose my_fclose // fix buggy fclose
	void my_fclose(FILE *f);
	#define OPTIMIZE_BSS
	#define BSS_SIZE 8192
  #endif	/* FLASH_VERSION */

  typedef short short_size;

	//#undef printf
	//#undef fputchar
	//#define printf print
	//#define printf(f...) (((void(*)(void*,char*,...))prtf)(fputchar,##f))
	//#define fputchar ((short(*)(char))ptch)
#endif
#ifndef FLASH_VERSION
	#define CGLOB =0
	#define CGLOBL ={0}
	#define xstatic static
	#define readonly
#else
	#define CGLOB
	#define CGLOBL
	#define xstatic
	#define readonly const
#endif

#include "protos.h"
#ifndef nl
  void nl(void);
#endif
#ifndef PC
char *__attribute__((stkparm)) sUnpack(char *in,char *out,char *dic);
#else
char *sUnpack(char *in,char *out,char *dic);
#endif
#endif
// vim:ts=4:sw=4
