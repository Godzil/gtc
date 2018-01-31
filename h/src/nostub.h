#ifndef NOSTUB
#define NOSTUB

#ifdef DOORS
#error "nostub.h" must not be included in "Doors" mode!
#undef NOSTUB
#else
#include <default.h>

#if defined(USE_FLINE_ROM_CALLS) || defined(EXE_OUT)
//#undef _rom_call_addr_concat
//#define _rom_call_addr_concat(intindex,romindex) (&romindex)
//#undef _rom_call_addr
//#define _rom_call_addr(index) (__jmp_tbl[0x##index])
#undef _rom_call
#define _rom_call(t,a,i) (*((t(*__ATTR_TIOS__)a)(&_ROM_CALL_##i)))
#undef OPTIMIZE_ROM_CALLS
#endif

#ifdef OPTIMIZE_ROM_CALLS
//#error OPTIMIZE_ROM_CALLS isn't supported yet
#endif

/* Various conditional compilations are defined below, to make an extra  */
/* support code as small as possible. If no RETURN_VALUE and SAVE_SCREEN */
/* options are defined, the code overload is only two extra bytes!       */

#if defined(SAVE_SCREEN) || defined(RETURN_VALUE) || !defined(NO_EXIT_SUPPORT) || defined(ENABLE_ERROR_RETURN)
#define _NEED_COMPLEX_MAIN
#endif

#define _main() __main()
#define _nostub _main

#endif
#endif
