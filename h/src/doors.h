#ifndef DOORS
#define DOORS

#ifdef NOSTUB

#error "doors.h" must not be included in "nostub" mode!
#undef DOORS

#else

#ifdef EXE_OUT
#error As of now, you cannot generate EXEs in kernel mode.
#endif

#include <default.h>

//#undef _rom_call_addr_concat
//#define _rom_call_addr_concat(intindex,romindex) (&romindex)
#undef _rom_call
#undef _rom_call_addr
#define _rom_call(t,a,i) (*((t(*__ATTR_TIOS__)a)(&_ROM_CALL_##i)))
#define _rom_call_addr(i) (&_ROM_CALL_##i)

#define _main() __main()

/* Begin Auto-Generated Part */
#define _ram_call(i,t) ((t)(&_RAM_CALL_##i))
#define _ram_call_addr(i) (&_RAM_CALL_##i)
/* End Auto-Generated Part */

#ifdef RETURN_VALUE
#define _NEED_COMPLEX_MAIN
#endif

#ifndef NO_EXIT_SUPPORT
#define _NEED_COMPLEX_MAIN
#endif

#if defined (_NEED_COMPLEX_MAIN) || defined (_NEED_AMS_CHECK) || defined (_NEED_CALC_DETECT)
#define NO_VSIMPLE_MAIN
#endif

#if defined(ENABLE_ERROR_RETURN) && defined(_NEED_COMPLEX_MAIN)
#define SPECIAL_ERROR_RETURN
#endif
#endif
#endif
