// Text Editor
#include "Workspace.h"
int onopen_gotoline=-1;
char *onopen_gotofile=NULL;
//void Edit(SYM_STR sym_text,char *text_name) {
void Edit(char *text_path) {
	if (!(Port=malloc(LCD_SIZE)))
		return;
	PortSet(Port,239,127);
	onopen_gotoline=-1;

editor_reopen:
	if (onopen_gotofile!=NULL)
	    text_path = onopen_gotofile, onopen_gotofile = NULL;
	char sym_buf[1+8+1+8+1];
	*sym_buf = 0;
	strncpy(sym_buf+1,text_path,sizeof(sym_buf)-1);
	SYM_STR sym_text = sym_buf+1;
	while (*sym_text) sym_text++;
	char *text_name = strrchr(text_path,'\\');
	if (!text_name)
	    text_name = text_path;

editor_reopen_samefile: {
	int key,k,n,z=0;
	void *ptr; int *kp;
	unsigned int ST_f;
	#ifdef HD2
	#define hdx hd2
	HANDLE hd2;
	#else
	#define hdx hd
	#endif
	int CU_state,KID,BKD,EDH_proc;
	HSym hs; int arch=0;
	if (!(hs=SymFind(sym_text)).folder)
		goto editor_quit;
	hdx=DerefSym(hs)->handle;
	NLINES=(LCD_HEIGHT-10)/6;
	YMAX=2+6*NLINES;
#ifdef TITLEBAR
	NLINES--;
#endif
	hs=SymFind(sym_text);	// because malloc may cause garbage collection (shouldn't change HSym, but...)
	if (DerefSym(hs)->flags.bits.archived)
		arch=1
	#ifndef HD2
			, EM_moveSymFromExtMem(sym_text,HS_NULL), hdx=DerefSym(hs)->handle
	#endif
				;
	#ifdef HD2
	hsize=*(unsigned int *)HeapDeref(hdx);
	hd=HeapAlloc(2+hsize);
	if (!hd) return;
	ptr=HLock(hd);
	memcpy(ptr,HeapDeref(hdx),2+hsize);
	#else
	ptr=HLock(hd);
	#endif
	tptr=ptr+4;
	spos=0; selpos=0; sel1=0; sel2=0;
	curpos=cpos=((unsigned int *)ptr)[1];
	if (onopen_gotoline>=0) {
		cpos=1;
		while (--onopen_gotoline)
			while (tptr[cpos]) {
			    if (tptr[cpos]==NEWLINE) {
				cpos+=2;
				break;
			    }
			    cpos++;
			}
			//cpos=down(cpos,0);
		curpos=cpos;
		onopen_gotoline=-1;
	}
/*	tptr=" test-123123123123123123123123123123123123123\n \n \n OK :)"
		"\n \n \n end\n \n \n of\n \n \n file"
		"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
		"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";*/
/*	tptr=" test-123123123123123123123123123123123123123\n \n \n \n \n OK :)"
		"\n \n \n \n \n end\n \n \n \n \n of\n \n \n \n \n file\0\0\0\0\0\0\0";*/
	size=strlen(tptr); hsize=*((unsigned int *)ptr);
	if (!cpos || cpos>size)
		cpos=1;
	InitSearch();
//	is_nl=1;
	CU_state=CU_start();
	scr_sptr=scr_stk;
#ifndef PEDROM
	KID=OSInitKeyInitDelay(cKID);
	BKD=OSInitBetweenKeyDelay(cBKD);
#endif
	view_cursor();
	msg="Welcome to GTC IDE !";
	curword[0]=0;
	pchnum=0;
	last_act=0; ac_on=0; ac_disp=0;
	add_all_pch();
	//add_pch("std");
	//add_pch("stdhead");
	//add_pch("gen");
	//add_pch("extgraph");
	//add_pch("graphx");
	//add_pch("keywords");
	OSTimerRestart(APD_TIMER);
	{
#if 0
		char *p=(char *)sym_text;
		while (*--p);
		strcpy(curFileFullName,p+1);
#else
		strcpy(curFileFullName,text_path);
#endif
	}
	strcpy(curFileName,text_name);
	strcpy(curFileExt,GetFTString(hdx));
	strtolower(curFileExt);
//	WspNew();
//	WspNew();
	editor_exit_requested=0;	// as the init in the loop is not sufficient...
	do {
		unsigned int par1s=par1;
		memset(Port,0,LCD_SIZE);
		par1=0; par2=0;
		if (!sel1) {
			unsigned int pos=cpos;
			int deep=0;
			while (pos>1 && tptr[pos-2]!=NEWLINE) {
				pos--;
				if (tptr[pos]==')') deep--;
				if (tptr[pos]=='(' && (++deep)>0) { par1=pos; break; }
			}
			if (par1) {
				while (tptr[pos] && tptr[pos]!=NEWLINE) {
					pos++;
					if (tptr[pos]=='(') deep++;
					if (tptr[pos]==')' && (--deep)<=0) { par2=pos; break; }
				}
				if (!par2) par1=0;
			}
		}
		if (chk_curword(curword,cpos,1) || (!curword[0] && par1!=par1s)) {
//			char *tp;
			nmatch=0;
			if (curword[0] && (msg=pch_search(curword,strlen(curword)>=2,MAX_MATCH,match)))
				msg=msg;
			else if (chk_curword(parword,par1,0) && (msg=pch_search(parword,0,0,NULL))) msg=msg;
			else msg=0;
			if (msg) { msg=memcpy(msgBuf,msg,58); msg[58]=0; }
			ac_disp=nmatch?last_act:0;
			ac_on=0;
		}
		display(z);
		if (ac_disp) ac_display(ac_on);
#ifndef RELEASE
		if (xcur<0) msg="Cursor not encountered", xcur=1;
		if (!curpos || tptr[curpos-1]==NEWLINE) msg="Invalid curpos";
#endif
		DrawStatus();
		LCD_restore(Port);
	 in_loop:
		ST_f=ST_flagsL;
		while (!(key=kbhit()) && !OSTimerExpired(CURSOR_TIMER)
			&& !OSTimerExpired(APD_TIMER) && ST_f==ST_flagsL)
				idle();
		if (ST_f!=ST_flagsL) {
			DrawStatus();
			LCD_restore(Port);
			goto in_loop;
		} else if (OSTimerExpired(APD_TIMER)) {
			Off();
			OSTimerRestart(APD_TIMER);
			continue;
		}
		OSTimerRestart(CURSOR_TIMER);
		z=~z;
		if (!key) continue;
		OSTimerRestart(APD_TIMER);
		last_act=0;
		key=ngetchx();
		editor_exit_requested=0;
		KeyProcess(key,TM_MULTILINE);
		z=0;
	} while (key!=KEY_ESC && !editor_exit_requested);
	close_pch();
#ifndef PEDROM
	OSInitKeyInitDelay(KID);
	OSInitBetweenKeyDelay(BKD);
#endif
	if (!CU_state) CU_stop();
	((unsigned int *)tptr)[-1]=cpos;
	((unsigned int *)tptr)[-2]=size+2+1+1;
	FreeMem();
	#ifdef HD2
	if (memcmp(tptr,2+2+HeapDeref(hdx),size+1+1)) {
		SimpleDlg(NULL,"\n Saving file"DOTS" \n",B_CENTER,W_NOKEY);
		LCD_restore(Port);
		hs=SymFind(sym_text);
		if (arch)
			EM_moveSymFromExtMem(sym_text,HS_NULL), hdx=DerefSym(hs)->handle;
		HeapFree(hdx);
		HeapUnlock(hd);
		HeapRealloc(hd,size+2+2+1+1);
		hs=SymFind(sym_text);
		DerefSym(hs)->handle=hd;
		if (arch)
			if (!EM_moveSymToExtMem(sym_text,HS_NULL))
				ST_helpMsg("!!! MEMORY !!!"),ngetchx();
	} else HeapUnlock(hd), HeapFree(hd);
	#else
	HeapUnlock(hd);
	HeapRealloc(hd,size+2+2+1+1);
	if (arch)
		EM_moveSymToExtMem(sym_text,HS_NULL);
	#endif
	if (editor_exit_requested==EDEX_COMPILE) {
		SimpleDlg(NULL,"\n Compiling project"DOTS" \n",B_CENTER,W_NOKEY);
		LCD_restore(Port);
		Compile(curFileFullName,PLUGIN_GTC);
		goto editor_reopen;
	}
	}
editor_quit:
	PortRestore();
	free(Port);
}
