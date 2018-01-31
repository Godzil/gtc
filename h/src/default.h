#ifndef __DEFAULT
#define __DEFAULT

#ifdef USE_TI92P
#define USE_TI92PLUS
#endif
#if !defined (USE_TI89) && !defined (USE_TI92PLUS) && !defined (USE_V200)
#define USE_TI89
#define USE_TI92PLUS
#define USE_V200
#elif defined(USE_TI89) && !defined (USE_TI92PLUS) && !defined (USE_V200)
#define _TI89_ONLY
#define _ONE_CALC_ONLY
#elif !defined(USE_TI89) && defined (USE_TI92PLUS) && !defined (USE_V200)
#define _TI92PLUS_ONLY
#define _ONE_CALC_ONLY
#elif !defined(USE_TI89) && !defined (USE_TI92PLUS) && defined (USE_V200)
#define _V200_ONLY
#define _ONE_CALC_ONLY
#endif
#if defined (USE_TI89) && defined (USE_TI92PLUS) && defined (USE_V200)
#define _SUPPORT_ALL_CALCS
#endif

#ifdef DOORS
#define USE_KERNEL
#endif

#if defined(EXECUTE_IN_GHOST_SPACE) && defined(USE_KERNEL)
#error EXECUTE_IN_GHOST_SPACE does not work in kernel mode yet
#endif

#if !defined (NO_CALC_DETECT) && (!defined (USE_KERNEL) || !defined (_SUPPORT_ALL_CALCS))
#define _NEED_CALC_DETECT
#ifdef _ONE_CALC_ONLY
#ifdef USE_TI89
#define _CALC_NUM 0
#endif
#ifdef USE_TI92PLUS
#define _CALC_NUM 1
#endif
#ifdef USE_V200
#define _CALC_NUM 3
#endif
#else
#ifndef _SUPPORT_ALL_CALCS
#ifndef USE_TI89
#define _CALC_NUM 0
#endif
#ifndef USE_TI92PLUS
#define _CALC_NUM 1
#endif
#ifndef USE_V200
#define _CALC_NUM 3
#endif
#endif
#endif
#endif
#if !defined (_ONE_CALC_ONLY) && !defined (USE_KERNEL)
#define _NEED_CALC_VAR
#endif

#define __ATTR_STD__ __attribute__((__stkparm__))
#define __ATTR_STD_NORETURN__ __attribute__((__stkparm__,__noreturn__))
#define CALLBACK __ATTR_STD__
#define __ATTR_TIOS__ __ATTR_STD__
#define __ATTR_TIOS_NORETURN__ __ATTR_STD_NORETURN__
#define __ATTR_TIOS_CALLBACK__ CALLBACK
#define __ATTR_GCC__ 
#define __ATTR_LIB_C__ __attribute__((__regparm__(1)))
#define __ATTR_LIB_ASM__ __ATTR_STD__
#define __ATTR_LIB_ASM_NORETURN__ __ATTR_STD_NORETURN__
#define __ATTR_LIB_CALLBACK_C__ CALLBACK
#define __ATTR_LIB_CALLBACK_ASM__ CALLBACK

#define __jmp_tbl (*(void***)0xC8)
//#define _rom_call(type,args,index) (*((type(*__ATTR_TIOS__)args)(_rom_call_addr_concat(0x##index,_ROM_CALL_##index))))
//#define _rom_call_addr(index) _rom_call_addr_concat(0x##index,_ROM_CALL_##index)
//#define _rom_call_addr_concat(intindex,romindex) (__jmp_tbl[intindex])
#define _rom_call(t,a,i) (*((t(*__ATTR_TIOS__)a)(__jmp_tbl[0x##i])))
#define _rom_call_addr(i) (__jmp_tbl[0x##i])
#define offsetof(t,m) ((unsigned long)&(((t*)0)->m))
#define OFFSETOF offsetof
#define import_binary(f,i) asm { i: incbin f };

#ifdef _GENERIC_ARCHIVE
#undef OPTIMIZE_ROM_CALLS
#undef USE_FLINE_ROM_CALLS
#undef USE_INTERNAL_FLINE_EMULATOR
#endif

#ifndef MIN_AMS
#define MIN_AMS 101
#endif

#ifdef USE_FLINE_ROM_CALLS
#if !defined (USE_FLINE_EMULATOR) && !defined (USE_INTERNAL_FLINE_EMULATOR)
#if MIN_AMS<204
#error You need to define USE_[INTERNAL_]FLINE_EMULATOR
#endif
#endif
//#error _INCLUDE_PATCH(fline_rom_calls);
#endif

#define TIOS_entries (*(unsigned long*)(__jmp_tbl-1))

#ifdef DOORS
#define AMS_1xx ((ROM_VERSION&0x0F00)==0x100)
#define AMS_2xx ((ROM_VERSION&0x0F00)==0x200)
#else
#define AMS_1xx (TIOS_entries<1000)
#define AMS_2xx (TIOS_entries>1000)
#endif

typedef short *__pshort;
typedef unsigned short *__pushort;
typedef long *__plong;
typedef unsigned long *__pulong;

//extern float __BC()__ATTR_LIB_ASM__;
//#define _tios_float_1(f,x,t) ({typedef float(*__temp__type__)(short,t)__ATTR_LIB_ASM__;((__temp__type__)__BC)(4*0x##f,x);})
//#define _tios_float_2(f,x,y,t1,t2) ({typedef //float(*__temp__type__)(short,t1,t2)__ATTR_LIB_ASM__;((__temp__type__)__BC)(4*0x##f,x,y);})

#if !defined (NOSTUB) && !defined (DOORS)
#ifdef USE_KERNEL
#include <doors.h>
#else
#include <nostub.h>
#endif
#endif

#endif
