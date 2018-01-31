/*
 *  GTC source file explorer
 */

#if defined(_89)||defined(_92)
#define BATCH_COMPILE
#endif

#define TITLEBAR
//#define OPTIMIZE_ROM_CALLS
//#define USE_KERNEL
#define ONE_CALC

#ifndef USE_KERNEL
#define SAVE_SCREEN
#endif
#ifdef BATCH_COMPILE
#define RELEASE
#else
//#define RELEASE
/*#define _ONE_CALC_ONLY
#define _92_ONLY*/
#define PEDROM
//#define _89
#endif

#if defined(_89)&&defined(_92)
#error oops, both models specified...
#endif

#ifdef _89
#define USE_TI89
#define X 160
#define Y 100
#define q89(x,y) (x)
#else
#define USE_TI92PLUS
#define USE_V200
#define X 240
#define Y 128
#define q89(x,y) (y)
#endif
#ifdef RELEASE
#define NDEBUG
#endif
#ifndef PEDROM
#define MIN_AMS 205
#else
#define MIN_AMS 101
#endif
#include <tigcclib.h>
//#include <kernel.h>
#include <extgraph.h>
#define GTC_IDE

typedef unsigned long TI_LONG;
typedef unsigned short TI_SHORT;
#ifdef USE_ABSOLUTE_PATHS
#include "E:\Paul\89\_Projex\GtC\pch.h"
#include "E:\Paul\89\Ti-GCC\Projects\Gt-Dev\apphdr.h"
#include "E:\Paul\89\Ti-GCC\Projects\Gt-Dev\SecureComm.h"
#include "E:\Paul\89\Ti-GCC\Projects\Gt-Dev\Plugins.h"
#else
#include "pch.h"
#include "gtdev-apphdr.h"
#include "gtdev-securecomm.h"
#include "gtdev-plugins.h"
#endif

/*#undef ST_flags
#define ST_flags _ROM_CALL_443
extern unsigned long ST_flags;*/
void *kbdq=0;
//#define ST_flags (*(unsigned long *)_rom_call_addr(443))
//#define ST_flagsL ((unsigned int)ST_flags)
#ifndef PEDROM
#define ST_flagsL (*(unsigned int *)(_rom_call_addr(443)+2))
#else
unsigned int ST_flagsL=0;
#endif

char sUnpackBuf[500];
#ifndef ONE_CALC
int calc=0;
void init_calc() {
	calc=CALCULATOR;
}
#endif
#undef CALCULATOR
#ifdef _89
#define CALCULATOR 0
#define _89_92(x,y) (x)
#else
#define CALCULATOR 1
#define _89_92(x,y) (y)
#endif
#define HD2
#include "Util.h"
#include "PchUtil.h"
#include "Display.h"
#include "Edit.h"
#include "Handlers.h"
#include "EditUI.h"
#include "Plugins.h"
#include "Editor.h"

#include "autoint_fix.h"

char _ti89[0],_ti92plus[0];

void memquit() {
	ST_helpMsg("Not enough memory!");
	ngetchx();
}

#if 0
void _main(void) {
	ESI ap;
	char *fold=(char *)$(source);
//	int X=LCD_WIDTH,Y=LCD_HEIGHT;
	char *port;
	char b[50];
	int key,sel=0;
	if (AMS_1xx) {
		ST_showHelp("Works only on AMS 2.0x");
		ngetchx();
		return;
	}
	kbdq=kbd_queue();
	#ifndef ONE_CALC
	init_calc();
	#endif
/*#undef AMS_2xx
#define AMS_2xx 1
	if (AMS_2xx) ST_flags &= ~(0x100000);*/
	InitArgPtr(ap);
	if (GetArgType(ap) == STR_TAG) {
		fold=(char *)GetSymstrArg(ap);
	}
	if (GetArgType(ap) != END_TAG) {
		ST_helpMsg("Syntax: explore(\"folder\")");
		ngetchx();
		return;
	}
	if (!SymFindHome(fold).folder) {
		int ok=1; char c,*p=fold;
		while ((c=*--p)) if (!(c=='_' || c>='a' || c<='z' || c>='0' || c<='9')) ok=0;
		if (ok)
			ok=FolderAdd(fold);
		if (!ok) {
			ST_helpMsg("Invalid folder name");
			ngetchx();
			return;
		}
	}
	if (!(port=malloc(LCD_SIZE))) { memquit(); return; }
	do {
		char *data=malloc(400*9);
		SYM_ENTRY *SymPtr;
		int but[5];
		int n=0,i,st,M;
		void __attribute__((stkparm)) Button(int i,char *t) {
			int x=i*32; char b[100];
			but[i]=1;
			sprintf(b,"F%d %s",i+1,t);
			DrawLine(x+2,Y-9,x+31-2,Y-9,A_NORMAL);
			DrawLine(x,Y-7,x,Y-1,A_NORMAL);
			DrawLine(x+31,Y-7,x+31,Y-1,A_NORMAL);
			DrawPix(x+1,Y-8,A_NORMAL);
			DrawPix(x+31-1,Y-8,A_NORMAL);
			DrawStr(x+2,Y-7,b,A_NORMAL);
		}
		if (!data) { memquit(); break; }
		#define name(x) data+((x)+((x)<<3))
		SymPtr = SymFindFirst(fold,1);
		while (SymPtr) {
			if (SymPtr->handle) {
				unsigned int *p=HLock(SymPtr->handle);
				if (((unsigned char *)p)[*p+1]==TEXT_TAG)
					strcpy(name(n), SymPtr->name), n++;
				HeapUnlock(SymPtr->handle);
			}
			SymPtr = SymFindNext();
		}
		PortSet(port,239,127);
		ClrScr();
		DrawLine(1,0,X-2,0,A_NORMAL);
		DrawLine(0,Y-1,X-1,Y-1,A_NORMAL);
		DrawLine(0,1,0,Y-1,A_NORMAL);
		DrawLine(X-1,1,X-1,Y-1,A_NORMAL);
		memset(but,0,5*2);
		FontSetSys(0);
		Button(0,"New");
		if (n) {
			Button(1,"Setup");
			Button(2,"Build");
			Button(3,"Run");
		}
		Button(4,"About");
		/* 89 : 11 files / 92+/V200 : 14 files */
		FontSetSys(1);
		M=CALCULATOR?14:11;
		st=sel-(M+1)/2;
		if (st>n-M) st=n-M;
		if (st<0) st=0;
		for (i=0;i<M;i++) {
			if (st+i>=n) break;
			if (i==sel-st) {
				SCR_RECT rect={{3,(i<<3)+1,X-3,(i<<3)+9}};
				ScrRectFill(&rect,ScrRect,A_NORMAL);
			}
			DrawStr(3,(i<<3)+2,name(st+i),A_XOR);
		}

		PortRestore();
		LCD_restore(port);
		key=ngetchx();
		if (key==(int)KEY_UP) sel--;
		else if (key==(int)KEY_DOWN) sel++;
		else if ((i=key-KEY_F1)>=0 && i<5 && but[i]) {
			if (i==0) {
				char buffer[21] = {0};
				HANDLE handle = DialogNewSimple (140, 55);
				DialogAddTitle(handle, "NEW PROJECT", BT_OK, BT_CANCEL);
				DialogAddText(handle, 3, 20, "Follow the steps to create a new program");
				DialogAddRequest(handle, 3, 30, "Program name :", 6, 20, 14);
				DialogAddText(handle, 3, 40, "Program type :");
				if (DialogDo(handle, CENTER, CENTER, buffer, NULL) == KEY_ENTER)
					DlgMessage("GREETINGS", buffer, BT_OK, BT_NONE);
				
			} else if (i==4) {
				ST_flags &= ~(0x100000);
				DlgMessage("ABOUT GTC",
					"\n    GTC C Compiler & IDE\n\n(c) 2001-2002 by Paul Froissart\n",
					BT_OK,BT_NONE);
				ST_flags |= 0x100000;
			}
		} else if (key==KEY_ENTER) {
			char *p=fold;
			while (*--p);
			*b=0;
			sprintf(b+1,"%s\\%s",p+1,name(sel));
			free(port);
			Edit(b+1+strlen(b+1),b+1+strlen(p+1)+1);
			port=malloc(LCD_SIZE);
		}
		if (sel>=n) sel-=n;
		if (sel<0) sel+=n;
		free(data);
	} while (key!=264);
	free(port);
//	if (AMS_2xx) ST_flags |= 0x100000;
}
#else
void _main(void) {
#ifndef PEDROM
	if (AMS_1xx) {
		ST_helpMsg("Please upgrade to AMS 2.0x");
		ngetchx();
		return;
	}
#endif
	kbdq=kbd_queue();
	#ifndef ONE_CALC
	init_calc();
	#endif
	ESI ap;
	char cfold[20]; strcpy(cfold,"source");
	InitArgPtr(ap);
	if (GetArgType(ap) == STR_TAG) {
		strcpy(cfold,GetStrnArg(ap));
	}
	FixAutoint();
	while (1) {
		Port=malloc(LCD_SIZE);
		if (!Port) return;
		PortSet(Port,239,127);
		ClrScr();
		XP_C *xc=XpLoadFat(cfold);
		if (!xc) break;
		XP_S *xs=XpLoop(xc,8,std_callback,NULL);
		if (!xs) { free(xc); break; }
		char file[20]; strcpy(file,(char *)xs->d);
		while (xs->t!=1) xs--;
		char fold[20]; strcpy(fold,(char *)xs->d);
		strcpy(cfold,fold);
		free(xc);
		PortRestore();
		free(Port);
#if 0
		char b[100];
		*b=0;
		sprintf(b+1,"%s\\%s",fold,file);
		Edit(b+1+strlen(b+1),file);
#else
		char path[8+1+8+1];
		sprintf(path,"%.8s\\%.8s",fold,file);
		Edit(path);
#endif
	}
	RestoreAutoint();
	PortRestore();
	free(Port);
}
#endif
