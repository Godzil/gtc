/*
 * GTools C compiler
 * =================
 * source file :
 * (on-calc) flashapp header
 *
 * Copyright 2001-2004 Paul Froissart.
 * Credits to Christoph van Wuellen and Matthew Brandt.
 * All commercial rights reserved.
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 */

//#define TEXT_SEG_SUPPORTED
#ifdef GTDEV
extern const SecureTab SecTab;
const SecureTab *SecTabPtr=&SecTab;
#endif

#ifdef TEXT_SEG_SUPPORTED
/* note that this is only useful to generate the ASM code below */ #ifdef VERYBASIC_OO
/* note that this is only useful to generate the ASM code below */ static void AP_app(pFrame self, EVENT *ev);
/* note that this is only useful to generate the ASM code below */ FRAME(appObj, OO_SYSTEM_FRAME, 0, OO_APP_FLAGS, 3)
/* note that this is only useful to generate the ASM code below */    ATTR(OO_APP_FLAGS, APP_INTERACTIVE)
/* note that this is only useful to generate the ASM code below */    ATTR(OO_APP_NAME, "GTC")
/* note that this is only useful to generate the ASM code below */    ATTR(OO_APP_PROCESS_EVENT, &AP_app)
/* note that this is only useful to generate the ASM code below */ //   ATTR(OO_APP_DEFAULT_MENU, &AppMenu )
/* note that this is only useful to generate the ASM code below */ ENDFRAME
/* note that this is only useful to generate the ASM code below */ #else
/* note that this is only useful to generate the ASM code below */ #define H_Compile 0
/* note that this is only useful to generate the ASM code below */ #define H_HELP 100
/* note that this is only useful to generate the ASM code below */ APP_EXTENSION const extensions[] = {
/* note that this is only useful to generate the ASM code below */ 	/* function name #, help string #, function index */
/* note that this is only useful to generate the ASM code below */ #ifdef HAS_CMDLINE_EXTENSION
/* note that this is only useful to generate the ASM code below */ 	{OO_APPSTRING+H_Compile, OO_APPSTRING+H_HELP+H_Compile, H_Compile },
/* note that this is only useful to generate the ASM code below */ #endif
/* note that this is only useful to generate the ASM code below */ };
/* note that this is only useful to generate the ASM code below */ void _scr_main();
/* note that this is only useful to generate the ASM code below */ APP_EXT_ENTRY const extEntries[] = {
/* note that this is only useful to generate the ASM code below */ 	{_scr_main, APP_EXT_PROGRAM},
/* note that this is only useful to generate the ASM code below */ };
/* note that this is only useful to generate the ASM code below */ void AP_app(pFrame self, EVENT *ev);
/* note that this is only useful to generate the ASM code below */ const char *AP_about(AppID self);
/* note that this is only useful to generate the ASM code below */ FRAME(appObj, OO_SYSTEM_FRAME, 0, OO_APP_FLAGS, 10)
/* note that this is only useful to generate the ASM code below */    ATTR(OO_APP_FLAGS, APP_INTERACTIVE)
/* note that this is only useful to generate the ASM code below */    ATTR(OO_APP_NAME, "GTC")
/* note that this is only useful to generate the ASM code below */ 	ATTR(OO_APP_TOK_NAME, "gtc")
/* note that this is only useful to generate the ASM code below */    ATTR(OO_APP_PROCESS_EVENT, &AP_app)
/* note that this is only useful to generate the ASM code below */ #ifdef HAS_CMDLINE_EXTENSION
/* note that this is only useful to generate the ASM code below */ 	ATTR(OO_APP_EXT_COUNT, 1)
/* note that this is only useful to generate the ASM code below */ #else
/* note that this is only useful to generate the ASM code below */ 	ATTR(OO_APP_EXT_COUNT, 0)
/* note that this is only useful to generate the ASM code below */ #endif
/* note that this is only useful to generate the ASM code below */ 	ATTR(OO_APP_EXTENSIONS, extensions)
/* note that this is only useful to generate the ASM code below */ 	ATTR(OO_APP_EXT_ENTRIES, extEntries)
/* note that this is only useful to generate the ASM code below */    ATTR(OO_APP_ABOUT, &AP_about)
/* note that this is only useful to generate the ASM code below */ #ifdef HAS_CMDLINE_EXTENSION
/* note that this is only useful to generate the ASM code below */ 	ATTR(OO_APPSTRING+H_Compile, "compile")
/* note that this is only useful to generate the ASM code below */ 	ATTR(OO_APPSTRING+H_HELP+H_Compile, "COMPILE A C PROGRAM")
/* note that this is only useful to generate the ASM code below */ #endif
/* note that this is only useful to generate the ASM code below */ ENDFRAME
/* note that this is only useful to generate the ASM code below */ #endif
/* note that this is only useful to generate the ASM code below */ 
/* note that this is only useful to generate the ASM code below */ pFrame pAppObj = (pFrame)&appObj;  /* Must be 1st! */
#else
#define CODESEG ".text"
#define DATASEG ".fdat"
#ifdef VERYBASIC_OO
asm(
"	.globl	appObj\n"
"	" CODESEG "\n"
"	.align 4\n"
"appObj:\n"
"	.long	-16777216\n"
"	.long	0\n"
"	.word	3\n"
"	.long	1\n"
"	.long	3\n"
"appObjAttr:\n"
"	.long	1\n"
"	.long	1\n"
"	.long	2\n"
"	.long	__GTC_str\n"
"	.long	4\n"
"	.long	AP_app\n"
"__GTC_str:\n"
"	.ascii \"GTC\\0\"\n"
"	.align 4\n"
"	.globl	pAppObj\n"
"	" DATASEG "\n"
"	.even\n"
"pAppObj:\n"
"	.long	appObj\n"
"	" CODESEG "\n");
#else
asm(
"	" CODESEG "\n"
"	.even\n"
#ifdef HAS_CMDLINE_EXTENSION
"	.globl	extensions\n"
"extensions:"
"	.long	4096\n"
"	.long	4196\n"
"	.word	0\n"
"	.globl	extEntries\n"
"	.even\n"
"extEntries:"
"	.long	_scr_main\n"
"	.word	0\n"
#endif
"	.globl	appObj\n"
"	.even\n"
"appObj:"
"	.long	-16777216\n"
"	.long	0\n"
"	.word	3\n"
"	.long	1\n"
"	.long	10\n"
"	.even\n"
"appObjAttr:"
"	.long	1\n"
"	.long	1\n"
"	.long	2\n"
"	.long	MC0\n"
"	.long	3\n"
"	.long	MC1\n"
"	.long	4\n"
"	.long	AP_app\n"
"	.long	7\n"
#ifdef HAS_CMDLINE_EXTENSION
"	.long	1\n"
"	.long	8\n"
"	.long	extensions\n"
"	.long	9\n"
"	.long	extEntries\n"
#else
"	.long	0\n"
#endif
"	.long	18\n"
"	.long	AP_about\n"
#ifdef HAS_CMDLINE_EXTENSION
"	.long	4096\n"
"	.long	MC2\n"
"	.long	4196\n"
"	.long	MC3\n"
#endif
"MC0:"
"	.ascii \"GTC\\0\"\n"
"MC1:"
"	.ascii \"gtc\\0\"\n"
#ifdef HAS_CMDLINE_EXTENSION
"MC2:"
"	.ascii \"compile\\0\"\n"
"MC3:"
"	.ascii \"COMPILE A C PROGRAM\\0\"\n"
#endif
"	.globl	pAppObj\n"
"	" DATASEG "\n"
"	.even\n"
"pAppObj:"
"	.long	appObj\n"
"	" CODESEG "\n");
#endif
#endif

void _main();
void _scr_main();
#define EV_quit() asm(".word $F800+1166")
void AP_app(pFrame self, EVENT *ev) {
   switch (ev->Type) {
      case CM_STARTTASK:
		_scr_main();
		EV_quit();
		break;
	}
}

const char *AP_about(AppID self) {
	return "GTC compiler\n\n(c) 2001-2003 by Paul Froissart\n\nInternal beta i1";
}

void _scr_main() {
	_main();
	// now redraw the screen... (thanks to PpHd for this code)
	// 1) the current application
	{ WINDOW w;
	WinOpen(&w,&(WIN_RECT){0,0,239,127},WF_NOBOLD|WF_NOBORDER);
	WinActivate(&w);
	WinClose(&w); }
	// 2) the status bar
	ST_showHelp("");
	ST_eraseHelp();
	// 3) the black line
#if !defined(_92) && !defined(_V200)
	memset(LCD_MEM+30*(100-7),-1,30);
#else
	memset(LCD_MEM+30*(128-7),-1,30);
#endif
}

#ifdef GTDEV
#include "gtdevcomm.c"
const SecureTab SecTab = {
	0,
	Compile
};
#endif
// vim:ts=4:sw=4
