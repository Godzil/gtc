/*
 * GTools C compiler
 * =================
 * source file :
 * error handling
 *
 * Copyright 2001-2004 Paul Froissart.
 * Credits to Christoph van Wuellen and Matthew Brandt.
 * All commercial rights reserved.
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 */

#define error(x) uerr(x)
#define ierr(f,n) err_int(((FUNC_##f)<<8)+n)
#define uerr err_usr
#define uerrc(m) uerr(ERR_OTH,m)
#define uerrc2(m,a) uerr(ERR_OTH,m,a)
#define uerrc3(m,a,b) uerr(ERR_OTH,m,a,b)
#define uerrsys(m) uerr(ERR_SYS,m)
#define iwarn(f,n) warn_int(((FUNC_##f)<<8)+n)
#define uwarn warn_usr

#ifdef PC
#define _noreturn
#define _err_attr
#else
#define _noreturn __attribute__((noreturn))
#define _err_attr
//#define _err_attr __attribute__((regparm(1)))
#endif

void _noreturn              fatal(char *message);
void _noreturn _err_attr	err_int(int e);
void _noreturn				err_usr(int e,...);
void _err_attr				warn_int(int w);
void						warn_usr(char *w,...);

// rules for adding new items : add them *ALWAYS* at the end
enum {
	FUNC_NONE=0,
	
/*$01*/	FUNC_ALIGNMENT, FUNC_MK_LEGAL, FUNC_COPY_ADDR, FUNC_G_DEREF, FUNC_G_UNARY,
/*$06*/	FUNC_G_ADDSUB, FUNC_G_DIV, FUNC_G_MOD, FUNC_G_MUL, FUNC_G_HOOK, FUNC_G_ASADD,
/*$0C*/	FUNC_G_ASXOR, FUNC_G_ASSHIFT, FUNC_G_ASMUL, FUNC_G_ASDIV, FUNC_G_ASMOD, FUNC_G_AINCDEC,
/*$12*/	FUNC_G_PARMS, FUNC_REGPARM, FUNC_FUNC_RESULT, FUNC_G_CAST, FUNC_G_EXPR, FUNC_G_COMPARE,
/*$18*/	FUNC_TRUEJP, FUNC_FALSEJP, FUNC_GENLOOP, FUNC_GENRETURN, FUNC_GENSTMT, FUNC_ADD_CODE, 
/*$1E*/	FUNC_ASM_GETSYM, FUNC_LISTING, FUNC_OPT0, FUNC_PASS3, FUNC_UNINIT_TEMP, __FUNC_UNUSED1,
/*$24*/	FUNC_G_CODE, FUNC_PEEP_DELETE, FUNC_G_PUSH, FUNC_G_POP, FUNC_CHECKSTACK, FUNC_TEMP_DATA, 
/*$2A*/	FUNC_TEMP_ADDR, FUNC_FREEOP, FUNC_DOOPER, FUNC_PEEP_LABEL, FUNC_BBLK_INIT,

/*$2F*/	FUNC_LINK_MISPL, FUNC_TABLE_HASH,
/*$31*/	FUNC_WARN_LC_STK,

};
// vim:ts=4:sw=4
