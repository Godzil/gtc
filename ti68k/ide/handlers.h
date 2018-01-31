// Keyboard Handlers

enum {
	TM_MULTILINE=0x1, TM_DIALOG=0x2,
};
void KeyProcess(int key,int tmode) {
	int *kp;
	int k,n;
#ifdef EVHOOK
	int EDH_proc=0;
 keyproc:
#endif
	kp=get_keylist();
	/* UP/DOWN  mode=0/2nd/Dmd/Shift/2nd+Shift/Dmd+Shift */
	if (key==*kp++) {
		if (ac_disp) { ac_on=1; ScrollUp(&ac_s); }
		else if (tmode&TM_MULTILINE) cpos=up(curpos,xcur),tab_chk_r(cpos),selpos=0;
/*		else if (tmode&TM_DIALOG)
			DialogMsg(DLG_UP);*/
	} else if (key==*kp++) {
		if (ac_disp) { if (ac_on) ScrollDn(&ac_s); else ac_on=1; }
		else if (tmode&TM_MULTILINE) cpos=down(curpos,xcur),tab_chk_r(cpos),selpos=0;
/*		else if (tmode&TM_DIALOG)
			DialogMsg(DLG_DN);*/
	} else if (key==*kp++ && (tmode&TM_MULTILINE)) {
		n=NLINES; cpos=curpos; while (n--) cpos=up(cpos,xcur);
		tab_chk_l(cpos); selpos=0;
	} else if (key==*kp++ && (tmode&TM_MULTILINE)) {
		n=NLINES; cpos=curpos; while (n--) cpos=down(cpos,xcur);
		tab_chk_l(cpos); selpos=0;
	} else if (key==*kp++ && (tmode&TM_MULTILINE)) {
		cpos=1; tab_chk_l(cpos); selpos=0;
	} else if (key==*kp++ && (tmode&TM_MULTILINE)) {
		cpos=curpos; while (tptr[cpos]) cpos=down(cpos,xcur);
		tab_chk_l(cpos); selpos=0;
	} else if (key==*kp++) selpos=up(curpos,xcur),tab_chk_r(selpos);
	else if (key==*kp++) selpos=down(curpos,xcur),tab_chk_r(selpos);
	else if (key==*kp++ && (tmode&TM_MULTILINE)) {
		n=NLINES; selpos=curpos; while (n--) selpos=up(selpos,xcur);
		tab_chk_l(selpos);
	} else if (key==*kp++ && (tmode&TM_MULTILINE)) {
		n=NLINES; selpos=curpos; while (n--) selpos=down(selpos,xcur);
		tab_chk_l(selpos);
	} else if (key==*kp++ && (tmode&TM_MULTILINE)) {
		selpos=1; tab_chk_l(selpos);
	} else if (key==*kp++ && (tmode&TM_MULTILINE)) {
		selpos=curpos; while (tptr[selpos]) selpos=down(selpos,xcur);
		tab_chk_l(selpos);
	}
	/* LEFT/RIGHT  mode=0/2nd/Shift/2nd+Shift */
	else if (key==*kp++) {
		if (sel1) cpos=sel1, selpos=0;
		else if (curpos>1) {
			cpos=curpos;
			cpos--; if (tptr[cpos-1]==NEWLINE) cpos--;
			selpos=0; tab_chk_l(cpos);
		}
	} else if (key==*kp++) { 
		if (sel2) cpos=sel2;
		else if (cpos<size) { if (tptr[cpos]==NEWLINE) cpos++; cpos++; }
		selpos=0; tab_chk_r(cpos);
	} else if (key==*kp++) {
		cpos=curpos; while (cpos>1 && tptr[cpos-2]!=NEWLINE) cpos--; selpos=0;
	} else if (key==*kp++) {
		cpos=curpos; while (tptr[cpos] && tptr[cpos]!=NEWLINE) cpos++; selpos=0;
	} else if (key==*kp++) { if (curpos>1) {
		selpos=curpos; selpos--; if (tptr[selpos-1]==NEWLINE) selpos--; tab_chk_l(selpos);
	} } else if (key==*kp++) { if (curpos<size) {
		selpos=curpos; if (tptr[selpos]==NEWLINE) selpos++; selpos++; tab_chk_r(selpos);
	} } else if (key==*kp++) {
		selpos=curpos; while (selpos>1 && tptr[selpos-2]!=NEWLINE) selpos--;
	} else if (key==*kp++) {
		selpos=curpos; while (tptr[selpos] && tptr[selpos]!=NEWLINE) selpos++;
	} else if ((k=0,key==*kp++) || (k++,key==*kp++)) {	/* BACKSPACE / DELETE */
		last_act=-1;
		if (sel1!=sel2) sel_del();
		else {
			if (k && cpos<size) { if (tptr[cpos]==NEWLINE || !tab_chk(cpos+1)) cpos++; cpos++; }
			if (cpos>1) {
				int n=tptr[cpos-2]==NEWLINE || !tab_chk(cpos-1)?2:1;
				memmove(tptr+cpos-n,tptr+cpos,size+1+1-cpos);
				size-=n; cpos-=n;
			}
		}
	} else if ((k=0,key==*kp++) || (k=1,key==*kp++)) {	/* CUT/COPY */
		if (sel1!=sel2) {
			CB_replaceTEXT(tptr+sel1,sel2-sel1,0);
			if (!k) sel_del();
		}
	} else if (key==*kp++) {	/* PASTE */
		HANDLE hd; long len;
		if (CB_fetchTEXT(&hd,&len))
			sel_del(), insert((unsigned short)len),
				memcpy(tptr+(cpos-(unsigned short)len),HeapDeref(hd),(unsigned short)len);	// multitask-unsafe !
	} else if (key==KEY_ENTER) {
		if (tmode&TM_MULTILINE) {
			unsigned int dpos=0;
			if (ac_on) ac_do();
			else {
				sel_del();
				n=whitespace(cpos,cpos);
				if (tptr[cpos-1]=='{') {
					if (whitespace(cpos+2,-1)<=n) {
						int i=n+2;
						insert(2+i), tptr[cpos-i-2]=NEWLINE;
						do tptr[cpos-i-1]=' '; while (i--);
						dpos=cpos;
					} else n+=2;
				}
				insert(2+n), tptr[cpos-n-2]=NEWLINE;
				do tptr[cpos-n-1]=' '; while (n--);
				if (dpos) cpos=dpos;
			}
		} /*else if (tmode&TM_DIALOG)
			DialogMsg(DLG_OK);*/
	} else if (key==KEY_ESC && ac_disp) {
		key=0; ac_disp=ac_on=0;
	} else if (key==KEY_CLEAR) {
		int n;
		if (ac_on) ac_do();
		if (linebegin(sel1)!=linebegin(sel2)) {	/* tabify selection */
			msg="Not implemented yet";
		} else
			sel_del(), insert(n=even(cpos)+1), tptr[cpos-n]=' ', tptr[cpos-1]=' ';
	} else if (key=='(' || key=='[' || key=='{') {
		int x=key=='('?0:(key=='['?1:2);
		if (ac_on) ac_do();
		text_enclose(((char *[]){"(","[","{"})[x],((char *[]){")","]","}"})[x]);
	} else if (key>=256 && ((k=key&~(32|4096|8192|16384))=='X' || k=='Y' || k=='Z' || k=='T')) {
		k-='X'; if (k<0) k=3; k+=k;
		if (key&4096) k++;
		text_enclose(((char *[]){"if (","if (","for (","for (","while (","while (","else ","else {"})[k],
		#ifdef PERFECT
			((char *[]){")\n\t\n\b",") {\n\t\n\b}\n"})[k&1]);
		#else
											#if 0
												(
											#endif
			((char *[]){") ",") {}",") ",") {}",") ",") {}","","}"})[k]);
		#endif
	} else if (k==KEY_ON && key!=k) {
		if (key&4096) key=KEY_ESC;
		Off();
	} else if ((k=(key&*kp++))>='1' && k<='9' && key>=256) {	/* kbdprgm# - custom shortcuts */
		
	} else if (key==*kp++) {												/* CATALOG */
		char *p;
		if ((p=Catalog()))
			sel_del(), insert(n=strlen(p)), memcpy(tptr+cpos-n,p,n);
	} else if (key==*kp++) {												/* Find text */
		if (SearchDlg(0)) DoSearch();
	} else if (key==*kp++) {												/* Replace text */
		if (SearchDlg(1)) DoSearch();
	} else if ((k=(key-KEY_F1+1))>=1 && k<=8) {	/* Function key F# */
		/*if (k==1) TestDisp();
		else*/
		if (k==3) {
			if (*text_find) DoSearch();
			else SearchDlg(0);
		} else if (k==5) {
			editor_exit_requested=EDEX_COMPILE;
		}
	} else if (key==*kp++) {												/* HOME */
		
	} else if (key==*kp++) {												/* CHAR */
		char c;
		if ((c=CharDlg()) && c>NEWLINE)
			sel_del(), insert(1), tptr[cpos-1]=c;
	} else {																				/* other keys */
		char *str=charmap;
		while (*kp) {
			if (key==*kp++) {
				sel_del(), insert(n=strlen(str)), memcpy(tptr+cpos-n,str,n);
				break;
			}
			while (*str++);
		}
		if (!*str && key>=14 && key<=255) {
			if (!isidch(key) && ac_on) ac_do();
			sel_del(), insert(1), tptr[cpos-1]=key;
		}
	}
#ifdef EVHOOK
	else if (key!=KEY_ESC && !EDH_proc && (key&0xFFF0)!=0x0150 /* arrow keys [they seem to bug] */) {
		int k2=0;
		void EVh(EVENT *ev) {
			if (ev->Type==CM_KEYPRESS) k2=ev->extra.Key.Code+1;
			else if (ev->Type==CM_STRING) {
				char *p=ev->extra.pasteText; int n;
				sel_del(), insert(n=strlen(p)), memcpy(tptr+cpos-n,p,n);
			}
			ER_throwVar(1);
		}
		EVENT e;
		while ((k=*kp++)) if (key==k) goto keydone;
		e.Type=CM_KEYPRESS;
		e.extra.Key.Code=key;
//			e.extra.Key.Mod =key&0xF000;
		EV_captureEvents((EVENT_HANDLER)EVh);
		PortRestore();
		TRY
			EV_defaultHandler(&e);
		ONERR
			EV_captureEvents(NULL);
		ENDTRY
		PortSet(Port,239,127);
		key=k2;
		EDH_proc=1;
		goto keyproc;
/*		} else {
		char b[100];
		sprintf(b,"[0x%x]",key);
		text_enclose(b,"");*/
	}
 keydone:
#endif
	curpos=selpos?:cpos;
	if (selpos) {
		if (selpos<cpos) sel1=selpos, sel2=cpos;
		else sel2=selpos, sel1=cpos;
	} else sel1=0, sel2=0;
	if (sel1==sel2) sel1=0, sel2=0;
	view_cursor();
}
