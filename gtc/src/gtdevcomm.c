/*
 * GTools C compiler
 * =================
 * source file :
 * (on-calc) communication with the GT-Dev IDE
 *
 * Copyright 2001-2004 Paul Froissart.
 * Credits to Christoph van Wuellen and Matthew Brandt.
 * All commercial rights reserved.
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 */

#include "GtDevComm.h"

extern int has_error;

char *in_file CGLOB,*out_file CGLOB;
Msg_Callback_t msg_process CGLOB;
Progr_Callback_t progr_process CGLOB;
#include "identity.h"
void _gtdevmain(void);
int Compile(char *in,char *out,Msg_Callback_t _msg_process,Progr_Callback_t _progr_process) {
	void *old_a5=bssdata;
	int res;
	bssdata=malloc(BSS_SIZE);
	if (!bssdata) return;
	memset(bssdata,0,BSS_SIZE);
	in_file=in; out_file=out;
	msg_process=_msg_process;
	progr_process=_progr_process;
	_gtdevmain();
	bssdata=identity(bssdata);
	if (!bssdata)
		return 2;
	res=has_error;
	free(bssdata);
	bssdata=old_a5;
	return res;
}
// vim:ts=4:sw=4
