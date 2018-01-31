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

#ifndef __GTDEVCOMM_H
#define __GTDEVCOMM_H

//#include "E:\Paul\89\Ti-GCC\Projects\GT-Dev\SecureCommDef.h"
#include "securecommdef.h"

#define ET_FATAL -2
#define ET_WARNING -1
#define ET_ERROR 0
#define ET_INTERNAL_WARNING 1
#define ET_INTERNAL_FAILURE 2
#define et_isinternal(x) ((x)>0)
#define et_iserror(x) !((x)&1)
#if __TIGCC_BETA__*100+__TIGCC_MINOR__>=94
#define CALLBACK __ATTR_TIOS_CALLBACK__
#else
#define CALLBACK
#endif
typedef void (CALLBACK*Msg_Callback_t)(char *message,int err_type,char *func,char *file,int line,int chr);
//typedef _Msg_Callback_t *Msg_Callback_t;
#define MAX_PROGRESS 65535
typedef void (CALLBACK*Progr_Callback_t)(char *func,char *file,unsigned int fprogress);
//typedef _Progr_Callback_t *Progr_Callback_t;

extern char *in_file,*out_file;
extern Msg_Callback_t msg_process;
extern Progr_Callback_t progr_process;
#endif
// vim:ts=4:sw=4
