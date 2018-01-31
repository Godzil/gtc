#if 0
void SearchDlg(int replace,int key) {
	int r;
	if ((r=DialogProcess(key)))
		switch (r) {
			case DLG_OK:
				ResetSearch();
				if (!DoSearch(replace))
					return 0;
				break;
		}
	return 1;
}
#endif

#include "aXplore.h"

void boxProcess(EDIT_LINE *e,int key) {
	if (key==KEY_BACKSPACE) delEditLine(e,1);
	else if (key>=14 && key<=255) { if (insEditLine(e,1)) getEditLinePos(e)[-1]=(char)key; }
	else if (key==KEY_LEFT) ScrollUp(&e->scr);
	else if (key==KEY_RIGHT) ScrollDn(&e->scr);
}
int checkProcess(int v,int a,int key) {
	if (key==KEY_LEFT || key==KEY_RIGHT)
		return v^a;
	return v;
}

/* Search dialog box :
Text to find	[...]
Replace with	[...]
[*] Whole word only
[*] Case-sensitive matching/Match case
[*] Search all files
[*] Create search results form
[*] Regular expression
*/
#define SDLG_BUF_SIZE 128
#define SDLG_LEFT_MARGIN 14
#define SDLG_NUM_OPTS 3
void boxDisp(EDIT_LINE *e,
		char *s,int x,int y,int sel,int blink,char *buf,char *inittext,int x0,int x1,int init) {
	if (init) {
		newEditLine(e,buf,SDLG_BUF_SIZE,x0,x1-1,y+1);
		insEditLine(e,strlen(inittext));
		strcpy(buf,inittext);
	}
	VariableDStr(x,y+1,s);
	updEditLine(e);
	drawEditLine(e,sel,blink);
}

enum { SDLG_WW=0x1, SDLG_CS=0x2, SDLG_SA=0x4, SDLG_ALL=0x40, SDLG_REPL=0x80, };
char text_find[SDLG_BUF_SIZE],text_repl[SDLG_BUF_SIZE];
int search_attr=0;
int search_coord=0;
int SearchDlg(int rep) {
	WIN_RECT win;
	int key;
	int n_it=1+rep+SDLG_NUM_OPTS,sel_it=0,blink=0;
	int a=search_attr,s;
	char buf_find[SDLG_BUF_SIZE],buf_repl[SDLG_BUF_SIZE];
	EDIT_LINE box_find,box_repl;
	int init=1,x,y;
	PushScr();
	while (1) {
		dialog((rep?"Replace text":"Find text"),120,8+(rep?8:0)+6*SDLG_NUM_OPTS,B_MOVEAROUND|B_ROUNDED,&win);
		x=physical_to_virtual(win.x0); y=win.y0;
		boxDisp(&box_find,"Text to find:",x,y,sel_it-0,blink,
			buf_find,text_find,x+SDLG_LEFT_MARGIN,win.x1>>2,init), y+=8;
		if (rep)
			boxDisp(&box_repl,"Replace with:",x,y,sel_it-1,blink,
				buf_repl,text_repl,x+SDLG_LEFT_MARGIN,win.x1>>2,init), y+=8;
		s=sel_it-rep-1;
		drawCheckBox("Whole word only", a,SDLG_WW,x,y,s-0), y+=6;
		drawCheckBox("Case-sensitive",  a,SDLG_CS,x,y,s-1), y+=6;
		drawCheckBox("Search all files",a,SDLG_SA,x,y,s-2), y+=6;
		init=0;
		key=WaitLoop(&blink);
		if (!key) continue;
		blink=0;
		if (key==KEY_UP && sel_it) sel_it--;
		else if (key==KEY_DOWN && sel_it<n_it-1) sel_it++;
		if (sel_it<=rep)
			boxProcess(sel_it?&box_repl:&box_find,key);
		else
			a=checkProcess(a,1<<(sel_it-rep-1),key);
		if (key==KEY_ENTER) {
			strcpy(text_find,buf_find);
			if (rep) strcpy(text_repl,buf_repl);
			search_attr=(a&~(SDLG_REPL|SDLG_ALL))|(rep?SDLG_REPL:0);
			return 1;
		} else if (key==KEY_ESC) return 0;
	}
	PopScr();
}
void InitSearch() {
	text_find[0]=0;
	text_repl[0]=0;
	search_attr=0;
}

#define is_c_idch(c) ((c>='A' && c<='Z') || (c>='a' && c<='z') || (c>='0' && c<='9') || c=='_' || c=='$')
int DoSearch() {
	int a=search_attr,f=0;
	if (*text_find) {
		char *p=tptr+curpos,*q=text_find;
		char v=*q++,v2=(!(a&SDLG_CS) && isalpha(v)?v^0x20:v),c,d;
		int n,need_nonidch=(a&SDLG_WW?is_c_idch(v):0);
		while (*p) {
			if ((*p==v && (p++,1)) || *p++==v2) {
				char *s=p;
				if (need_nonidch && is_c_idch(p[-2])) continue;
				if (a&SDLG_CS) {
					while ((c=*q++))
						if (*p++!=c) goto not_here;
				} else {
					while ((c=*q++))
						if ((d=*p++)!=c) {
							if ((d^=c)!=0x20) goto not_here;
							c|=d;
							if (!(c>='a' && c<='z')) goto not_here;
						}
				}
				if ((a&SDLG_WW) && is_c_idch(p[-1]) && is_c_idch(*p)) goto not_here;
				// we have a match :)
				f++;
				sel1=cpos=(s-tptr)-1;
				sel2=curpos=selpos=cpos+strlen(text_find);
				if (a&SDLG_REPL) {
					sel_del();	// this is legal because we updated sel1 & sel2
					insert(n=strlen(text_repl));
					selpos=cpos;
					cpos=selpos-n;
					memcpy(tptr+cpos,text_repl,n);
				}
				if (!(a&SDLG_ALL)) return 1;
			not_here:
				p=s;
				q=text_find+1;
			}
		}
	}
	if (f) return -1;
	PushScr();
	SimpleDlg("Find text","\nText not found!\n",B_CENTER,W_NORMAL);
	PopScr();
	return 0;
}

char wzTitle[40];
char *wzStepPtr=0;
#define wzStep (*wzStepPtr)
int wzSuccess=0;
int wzText(char *prompt,char *dest,int n) {
	WIN_RECT win;
	int key;
	int blink=0;
	EDIT_LINE box;
	int init=1,x,y;
	char buf[n+1];
	wzStep++;
	PushScr();
	while (1) {
		dialog(wzTitle,120,12,B_CENTER|B_ROUNDED,&win);
		x=win.x0>>2; y=win.y0+2;
		boxDisp(&box,prompt,x,y,0,blink,
			buf,dest,x+strlen(prompt)+1,win.x1>>2,init), y+=8;
		init=0;
		key=WaitLoop(&blink);
		if (!key) continue;
		blink=0;
		boxProcess(&box,key);
		if (key==KEY_ENTER) {
			strcpy(dest,buf);
			PopScr();
			return 1;
		} else if (key==KEY_ESC) {
			wzStep-=2;
			PopScr();
			return 0;
		}
	}
}
int wzTextClr(char *prompt,char *dest,int n) {
	dest[0]=0;
	return wzText(prompt,dest,n);
}
int wzChoice(char *prompt,char **choice,int choicen,int *dest) {
	wzStep++;
	XP_C *xc=XpLoadList(choice,choicen);
	int h=8;
	WIN_RECT win;
	PushScr();
	dialog(wzTitle,120,h+XP_H*XP_N,B_CENTER|B_ROUNDED,&win);
	VariableDStr(physical_to_virtual(win.x0),win.y0,prompt);
	xc->sel=*dest;
	if (!XpLoop(xc,win.y0+h,NULL,NULL)) {
		PopScr();
		wzStep-=2;
		free(xc);
		return 0;
	} else {
		PopScr();
		*dest=xc->sel;
		free(xc);
		return 1;
	}
}
int wzChoiceDeflt(char *prompt,char **choice,int choicen,int *dest,int v) {
	*dest=v;
	return wzChoice(prompt,choice,choicen,dest);
}
void wzStart(char *title,char *numSteps) {
	strcpy(wzTitle,title);
	strcat(wzTitle," - Step ");
	wzStepPtr=wzTitle+strlen(wzTitle);
	strcat(wzTitle,"x of ");
	strcat(wzTitle,numSteps);
	wzSuccess=0;
	wzStep='0';
}
void wzDone(char *text) {
	wzSuccess=1;
	wzTitle[strlen(wzTitle)-sizeof("- Step x of y")]='\0';
//	asm("0:bra 0b");
	PushScr();
	SimpleDlg(wzTitle,text,B_CENTER,W_NORMAL|ICO_INFO);
	PopScr();
}
#define wzList(a...) ((char *[]){a}),sizeof((char *[]){a})/sizeof(char *)
#define wzErr(m) ({SimpleDlg(wzTitle,m,B_CENTER,W_NORMAL|ICO_ERROR);continue;})

#if 0
typedef struct {
	int t,id,x,y;
	union {
		TEXT txt;
		SCROLL sc;
		char *s;
	} d;
} DLG_ITEM;
typedef struct {
	SCROLL sel;
	char *title;
	int w,h,a;	// width, height, attr
	DLG_ITEM i[];
} DLG;

int DialogProcess(int key) {
	DLG *d=cur_dlg;
	DLG_ITEM *i;
	int s;
	if (key==KEY_UP)
		ScrollUp(&d->sel);
	else if (key==KEY_DOWN)
		ScrollDn(&d->sel);
	else if (key==KEY_ENTER)
		return DLG_OK;
	else if (key==KEY_ESC)
		return DLG_ESC;
	ScrollUpdNoWrap(&d->sel);
	s=d->sel.sel;
	i=&d->i[s];
	if (i->t<=0)
		return DialogProcess(key);
	switch (i->t) {
		case DT_TBOX:
			cur_text=i->d.txt;
			return KeyProcess(key,TM_DIALOG);
		case DT_MLTBOX:
			cur_text=i->d.txt;
			return KeyProcess(key,TM_DIALOG|TM_MULTILINE);
		case DT_CBOX:
		case DT_DROPDOWN:
			if (key==KEY_LEFT)
				ScrollUp(&i->d.sc);
			else if (key==KEY_RIGHT)
				ScrollDn(&i->d.sc);
			ScrollUpd(&i->d.sc);
			break;
		case DT_TEXT:
			break;
	}
	return 0;
}

void DialogDisp() {
	DLG *d=cur_dlg;
	int n=cur_dlg->sel.n;
	DLG_ITEM *i=d->i;
	dialog(d->title,d->w,d->h,d->a);
	while (n--) {
		switch (i->t) {
			case DT_TEXT:
				VariableDStr(i->x,i->y,i->d.s);
				break;
		}
		i++;
	}
}

enum {
#define DL_SKIP(x) -(x)
	DL_END=0,
	
};
int DialogLoadSub(int *p,DLG_ITEM *i,int x0) {
	int n=0,x=x0,y=0,z;
	while (*p) {
		switch ((z=*p++)) {
			case DL_TEXT:
				if (i) {
					i->t=DT_TEXT;
					i->id=*p++;
					i->x=x; i->y=y;
					i->d.s=p;
					while (*p++);
				} else { p++; while (*p++); }
				n++;
				y+=char_height+1;
				break;
			default:	// DL_SKIP(-z)
				p-=z;
				break;
		}
	}
}
void DialogLoad(int *p) {
}
#endif
