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

/*#include "define.h"
#include "c.h"
#include "cglbdec.h"*/
#include <stdarg.h>
#include <setjmp.h>

extern int pause_on_error;

#ifndef GTDEV
//#ifdef PC
void _err_attr err_int(int m) {
	clean_up();
	msg2("\nAn internal error occurred (error ID #%04d)\n"
		"Please report this bug to paul.froissart@libertysurf.fr\n",m);
#ifdef PC
	fflush(outstr);
	if (pause_on_error)
		getchar();
	exit(16);
#else
	ngetchx();
	exit(0);
#endif
}
void err_usr(int m,...) {
	va_list args;
	va_start(args,m);
#ifdef PC
	if (verbose)
#endif
		msg("\nCompilation error:\n");
	if (err_force_line IS_INVALID && err_assembly)
		err_force_line=err_cur_line;
	locate();
	if (m==ERR_OTH) {
		char *s=va_arg(args,char *);
		vmsg(s,args);
	} else
		vmsg((char *)__err[m],args);
	msg("\n");
	va_end(args);
	clean_up();
#ifdef PC
	fflush(outstr);
	if (pause_on_error)
		getchar();
	exit(16);
#else
	ngetchx();
	exit(0);
#endif
}
void _err_attr warn_int(int m) {
	msg2("\nAn unexpected event occurred (event ID #%04d)\n"
		"It might be possible to continue, but the generated code may "
		"contain bugs.\nDo you want to continue [y/n]?",m);
#ifdef PC
	fflush(outstr);
	if ((pause_on_error && (msg("\nn\n"),1)) || getchar()=='n') {
		clean_up();
		exit(16);
	}
	msg("\n");
#else
	while (!kbhit());
	if (ngetchx()=='n') {
		clean_up();
		exit(0);
	} else msg("\n");
#endif
}
void warn_usr(char *s,...) {
	va_list args;
	va_start(args,s);
	locate();
	msg("warning: ");
	vmsg(s,args);
	msg("\n");
	va_end(args);
}
//#endif
#else
void _err_attr error_do(char *msg,int type) {
	char _func[100],file[40];
	char *func=0;
	int line,chr=0;
	if (func_sp) strcpy(func=_func,func_sp->name);
	strcpy(file,curname);
	if (err_force_line IS_INVALID && err_assembly)
		err_force_line=err_cur_line;
	if (err_force_line IS_VALID)
		line=err_force_line;
	else line=lineno,chr=0/*to be improved*/;
	if (et_iserror(type))
		clean_up();
	msg_process(msg,type,func,file,line,chr);
	if (et_iserror(type))
		exit(0);
}
void _noreturn assert_terminated();
asm("assert_terminated:\n rts\n");
void _err_attr err_int(int m) {
	char b[30];
	sprintf(b,"error ID #%04d",m);
	error_do(b,ET_INTERNAL_FAILURE);
	assert_terminated();
}
void err_usr(int m,...) {
	va_list args;
	va_start(args,m);
	char b[200];
	if (m==ERR_OTH) {
		char *s=va_arg(args,char *);
		vsprintf(b,s,args);
	} else
		vsprintf(b,(char *)__err[m],args);
	va_end(args);
	error_do(b,ET_ERROR);
	assert_terminated();
}
void _err_attr warn_int(int m) {
	char b[30];
	sprintf(b,"event ID #%04d",m);
	error_do(b,ET_INTERNAL_WARNING);
}
void warn_usr(char *s,...) {
	va_list args;
	va_start(args,s);
	char b[200];
	vsprintf(b,s,args);
	va_end(args);
	error_do(b,ET_WARNING);
}
#endif
// vim:ts=4:sw=4
