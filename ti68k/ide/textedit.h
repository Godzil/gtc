// Text editing handler
typedef int (*HANDLER)(int);
HANDLER handler=0;
int EDH_proc=0;
int tHandle(int key) {
	int *kp=get_keylist();
	/* UP/DOWN  mode=0/2nd/Shift */
	if (key==*kp++) { if (ac_on) ScrollUp(&ac_s); else cpos=up(curpos,xcur),tab_chk_r(cpos),selpos=0; }
	else if (key==*kp++) { if (ac_on)ScrollDn(&ac_s);else cpos=down(curpos,xcur),tab_chk_r(cpos),selpos=0; }
	else if (key==*kp++) {
		n=NLINES; cpos=curpos; while (n--) cpos=up(cpos,xcur);
		tab_chk_l(cpos); selpos=0;
	} else if (key==*kp++) {
		n=NLINES; cpos=curpos; while (n--) cpos=down(cpos,xcur);
		tab_chk_l(cpos); selpos=0;
	} else if (key==*kp++) selpos=up(curpos,xcur),tab_chk_r(selpos);
	else if (key==*kp++) selpos=down(curpos,xcur),tab_chk_r(selpos);
	/* LEFT/RIGHT  mode=0/2nd/Shift */
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
	} } else if ((k=0,key==*kp++) || (k++,key==*kp++)) {	/* BACKSPACE / DELETE */
		last_act=-1;
		if (sel1!=sel2) sel_del();
		else {
			if (k && cpos<size) { if (tptr[cpos]==NEWLINE) cpos++; cpos++; }
			if (cpos>1) {
				int n=tptr[cpos-2]==NEWLINE?2:1;
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
			int n; unsigned int dpos=0;
			if (ac_on) ac_do();
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
	} else if (key>=14 && key<=255) {
		if (!isidch(key) && ac_on) ac_do();
		sel_del(), insert(1), tptr[cpos-1]=key;
	} else if (key!=KEY_ESC && !EDH_proc && (key&0xFFF0)!=0x0150 /* arrow keys [they seem to bug] */) {
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
		EDH_proc=1;
		return handler(k2);
	} else return 0;
 keydone:
	return 1;
}
int dHandle(int key) {
	} else if (k==KEY_ON && key!=k) {
		if (key&4096) key=KEY_ESC;
		Off();
}

	} else if (key==KEY_CLEAR) {
		int n;
		if (ac_on) ac_do();
		sel_del(), insert(n=even(cpos)+1), tptr[cpos-n]=' ', tptr[cpos-1]=' ';
	} else if (key=='(' || key=='[' || key=='{') {
		int x=key=='('?0:(key=='['?1:2);
		if (ac_on) ac_do();
		text_enclose(((char *[]){"(","[","{"})[x],((char *[]){")","]","}"})[x]);
	} else if ((k=key&~(32|4096|8192|16384))=='X' || k=='Y' || k=='Z' || k=='T') {
		k-='X'; if (k<0) k=3; k+=k;
		if (key&4096) k++;
		text_enclose(((char *[]){"if (","if (","for (","for (","while (","while ("})[k],
			((char *[]){")\n\t\n\b",") {\n\t\n\b}\n"})[k&1]);
	} else if ((k=(key&*kp++))>='1' && k<='9') {	/* kbdprgm# - custom shortcuts */
		
	} else if (key==*kp++) {												/* CATALOG */
		Catalog();
	} else if ((k=(key-KEY_F1+1))>=1 && k<=8) {	/* Function key F# */
		/*if (k==1) TestDisp();
		else*/ if (k==3) FindDlg();
