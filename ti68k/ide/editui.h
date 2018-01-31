// Text editing User Interface routines

/* Common routines */
int repeated=0;	// was the last key repeated?
int WaitLoop(int *blink) {
	int key=0;	// this is necessary when blinking
	unsigned int ST_f;
	DrawStatus();
	LCD_restore(Port);
 in_loop:
	ST_f=ST_flagsL;
	while (OSdequeue(&key,kbdq) && (!blink || !OSTimerExpired(CURSOR_TIMER))
		&& !OSTimerExpired(APD_TIMER) && ST_f==ST_flagsL)
			idle();
	if (!key) {	// to avoid a potential race condition that would 'forget' that keypress
		if (ST_f!=ST_flagsL) {
			DrawStatus();
			LCD_restore(Port);
			goto in_loop;
		} else if (OSTimerExpired(APD_TIMER)) {
			Off();
			OSTimerRestart(APD_TIMER);
			goto in_loop;
		}
	}
	OSTimerRestart(CURSOR_TIMER);
	if (blink) *blink=~(*blink);
#ifndef DONT_USE_KBDQ
	if (key) OSTimerRestart(APD_TIMER);
	repeated=key&0x0800;
	return key&0xF7FF;
#else
	if (!key) return 0;
	OSTimerRestart(APD_TIMER);
	return ngetchx();
#endif
}
typedef struct {
	char *buf;
	int sz;
	int x0,y,w;
	WIN_RECT win;
	SCROLL scr;
} EDIT_LINE;
void newEditLine(EDIT_LINE *e,char *buf,int sz,int x0,int x1,int y) {
	int w=x1-x0;
	e->buf=buf; buf[0]=0; e->sz=sz;
	e->x0=x0; e->y=y; e->w=w;
	e->win.x0=virtual_to_physical(x0)-2;
	e->win.x1=virtual_to_physical(x1)+1;
	e->win.y0=y-2;
	e->win.y1=y+char_height+2;
	e->scr.n=1; e->scr.h=0;
	ScrollInit(&e->scr);
}
int insEditLine(EDIT_LINE *e,int n) {
	if (e->scr.n+n<=e->sz) {
		memmove(e->buf+e->scr.sel+n,e->buf+e->scr.sel,e->scr.n-e->scr.sel);
		e->scr.n+=n;
		e->scr.sel+=n;
		return 1;
	}
	return 0;
}
#define getEditLinePos(e) ((e)->buf+(e)->scr.sel)
int delEditLine(EDIT_LINE *e,int n) {
	if (e->scr.sel>=n) {
		e->scr.n-=n;
		e->scr.sel-=n;
		memmove(e->buf+e->scr.sel,e->buf+e->scr.sel+n,e->scr.n-e->scr.sel);
		return 1;
	}
	return 0;
}
void updEditLine(EDIT_LINE *e) {	/* to be called when buffer content changes */
	int l=strlen(e->buf);
	e->scr.n=l+1;
	if (l>e->w) l=e->w;
	e->scr.h=l+1;
}
void DrawVLine(void *p,int x,int y1,int y2,int a) {
	FastDrawLine(p,x,y1,x,y2,a);
}
void drawEditLine(EDIT_LINE *e,int inactive,int blink) {
//	asm("0:jbra 0b");
	updEditLine(e);
	ScrollUpdNoWrapDots(&e->scr);
	VariableDStrC(e->x0,e->y,e->buf+e->scr.scr,e->w);
	if (inactive) return;
	DrawClipRect(&e->win,ScrRect,A_NORMAL);
	if (!blink) DrawVLine(Port,((e->x0-e->scr.scr+e->scr.sel)<<2)-1,e->y,e->y+char_height-2,A_XOR);
}
void drawCheckBox(char *s,int v,int f,int x,int y,int inactive) {
	if (!inactive) VariableDStr(x,y,"[ ]");
	VariableDChar(x+virtual_char_width,y,(v&f)?3:2);
	VariableDStr(x+3*virtual_char_width,y,s);
}

typedef long DLG_ICON[8];
DLG_ICON DlgIcons[4]={
	{0x03000480,0x08400860,0x13201330,0x23102318,0x4308430C,0x80048306,0x8306400E,0x3FFC1FF8},
	{0x07801FE0,0x3FF077B8,0x631CF03C,0xF87EF87E,0xF03E631E,0x77BC3FFC,0x1FF80FF0,0x03C00000},
	{0x0FC03030,0x4308430C,0x80048706,0x83068306,0x430E478C,0x301C1E38,0x073000B0,0x00700030},
	{0x0FC03030,0x47884CCC,0x8CC48186,0x83068006,0x430E430C,0x301C1E38,0x073000B0,0x00700030},
};
enum { ICO_WARN=0x8, ICO_ERROR=0x9, ICO_INFO=0xA, ICO_QUEST=0xB, ICO_MASK=0xF, ICO_RMASK=0x7 };

void DrawText(char *p,int x,int y,int w,int h0,int hM,int *wp,int *hp,int draw) {
																									/* NB: x&y only needed in draw */
	char c,*word=p; int cx=x,cy=y-h0,yM=y+hM;
	if (cy<y) draw=-draw;
	w+=x;
	while ((c=*p++)) {
		if (c=='\n') { cx=x,cy+=6,(*hp)+=6; if (cy>=yM) return; else if (cy==y) draw=-draw; }
//		else if (c==' ')
		else {
			if (draw>0) DChar(cx,cy,c);
			cx++; if (!draw && *wp<cx) (*wp)=cx;
			if (cx>=w) { cx=x,cy+=6,(*hp)+=6; if (cy>=yM) return; else if (cy==y) draw=-draw; }
		}
	}
}
void dialog(char *title,int width,int height,int attr,WIN_RECT *w);
#define MAX_TEXT_LINENUM 0x1000
#define MAXH 10
asm("identity: rts");
int __attribute__((regparm)) identity(int x);
int SimpleDlg(char *title,char *text,int attr,int wattr) {
	int dummy1,dummy2; WIN_RECT win;
	int w=116/4,h=6,rh;
	int h0=0,xsh=0,key;
	if (wattr&ICO_MASK) w-=5;
	DrawText(text,0,0,w,0,MAX_TEXT_LINENUM,&w,&h,0);
	rh=h;
	if (h>MAXH*6) h=MAXH*6,xsh=2;
	if (wattr&ICO_MASK) xsh+=5;
	if (!(wattr&W_NOKEY)) PushScr();
	while (1) {
		dialog(title,(w+xsh)<<2,h,attr|B_ROUNDED,&win);
		DrawText(text,(win.x0>>2)+xsh,win.y0,w,h0,h,&dummy1,&dummy2,1);
		if (wattr&ICO_MASK) {
			// 'identity 'fixes TIGCC bug :p
			Sprite16(((win.x0>>2)+xsh-5)<<2,win.y0,16,(short *)(DlgIcons[identity(wattr&ICO_RMASK)]),Port,A_OR);
		}
		if (h0) DChar(win.x0>>2,win.y0,UPARR);
		if (h0+MAXH*6<rh) DChar(win.x0>>2,win.y0+(MAXH-1)*6,DNARR);
		if (wattr&W_NOKEY) return 0;
		key=WaitLoop(NULL);
		if (key==KEY_UP && h0) h0-=6;
		else if (key==KEY_DOWN && h0+MAXH*6<rh) h0+=6;
		if (key==KEY_ENTER) {
			if (!(wattr&W_NOKEY)) PopScr();
			return -1;
		} else if (key==KEY_ESC) {
			if (!(wattr&W_NOKEY)) PopScr();
			return 0;
	#define KEY_F0 (KEY_F1-1)
		} else if (key>=KEY_F1 && key<=KEY_F5 && (wattr&(0x10<<(key-KEY_F0)))) return key-KEY_F0;
	}
}
int YesNoDlg(char *title,char *text) {
	char ntext[300];
	strcpy(ntext,text);
	strcat(ntext,"\n\n [ENTER]=Yes\n [ESC]  =No");
	return SimpleDlg(title,ntext,B_CENTER,W_NORMAL|ICO_QUEST);
}
#ifdef NEED_DEBUG_MSG
void debug_msg(char *s) {
	char b[200];
	sprintf(b,"\n %s \n",s);
	SimpleDlg("Debug",b,B_CENTER,W_NORMAL|ICO_INFO);
}
#endif


/* CHAR LIST */
char CharDlg() {
	static int sel=32;
	while (1) {
		int scr=(sel>=128);
		int c=scr?128:0,maxc=c+128; int y,key; WIN_RECT w;
		char title[]="Char list - Page x";
		title[sizeof("Char list - Page x")-2]='1'+scr;
		dialog(title,(16+1)*8,6*8/* =48 */,B_CENTER|B_NORMAL,&w);
		y=w.y0;
		DChar(w.x0>>2,y+(scr?0:6*(8-1)),(scr?UPARR:DNARR));
		while (c<maxc) {
			int x=(w.x0>>2)+3,n=16;
			while (n--) {
				if (c>=16) {
					if (c==sel)
						DCharS(x,y,c),DChar(x-1,y,0);
					else DChar(x,y,c);
				}
				c++,x+=2;
			}
			y+=6;
		}
		if (!(key=WaitLoop(NULL))) continue;
		if (key==KEY_ESC) break;
		else if (key==KEY_LEFT) sel--;
		else if (key==KEY_RIGHT) sel++;
		else if (key==KEY_UP) sel-=16;
		else if (key==KEY_DOWN) sel+=16;
		else if (key==KEY_ENTER)
			return sel;
		if (sel<16) sel+=256-16;
		if (sel>=256) sel-=256-16;
	}
	return 0;
}


/* CATALOG */
/*int __attribute__((__regparm__(2))) cat_ins(char *p,char **match,int n) {
	char *s1,*s2,c;
	int a=0,b=n-1,m;
	while (a<=b) {
		m=(a+b)>>1;
		s1=p;
		s2=match[m];
		do {
			if (!(c=*s2++) && !*s1)
				return n;
		} while (*s1++==c);
		if (s1[-1]<c)	// il faut aller plus haut
			b=m-1; //,sup=m;
		else a=m+1;
	}
	if ((++b)==n) b--;
	match+=b;
	memmove(match+1,match,(n-b)*4);
	*match=p;
	return n+1;
}*/
void __attribute__((__regparm__(2))) memmove4_plus4(long *p,int n) {
	long *q;
	p+=n;
	q=p+1;
	while (n--)
		*--q=*--p;
}
void __attribute__((__regparm__(3))) dual_memmove4_plus4(long *p,long *r,int n) {
	long *q,*s;
	p+=n,r+=n;
	q=p+1,s=r+1;
	while (n--)
		*--q=*--p,*--s=*--r;
}

#define PUSH(x) asm("move.w %0,-(%%sp)"::"r" ((int)x))
#define POP(x) asm("move.w (%%sp)+,%d0":"=d" (z))
/*  --- CAUTION :                                         */
/* USAGE OF PUSH AND POP MAKES IT VERY COMPILER-DEPENDENT */
/* (for example, stack frames are necessary)              */
int cat_prep(int MM,char **match,char **mdata) {
	int n=pchnum,nm=1;
	*match="ÿ";
	while (n--) {
		unsigned char *p0=pchdata[n],*p; int z=PCH_HEAD_SIZE;
		PUSH(0);
		do {
			p=p0+z;
			if (*p) {
				char *s1,*s2,c; char **mp=match;
				int a=0,b=nm-1,m;
				#define OR_VAL 0x20
				while (a<=b) {
					m=(a+b)>>1;
					s1=p;
					s2=mp[m];
/*					asm volatile(
					"comp_loop:"
						"move.b (%1)+,%%d0;"
						"beq.s check_dup;"
					"check_dup_false:"
						"move.b (%2)+,%%d1;"
						"eor.b %%d1,%%d0;"
						"and.b %5,%%d0;"
						"beq.s comp_loop;"
						"bpl.s change_a;"
					"change_b:"
						"subq.w #1,%6;"
						"move.w %6,%4;"
						"bra ok_comp;"
					"check_dup:"
						"tst.b (%2);"
						"bne check_dup_false;"
						"bra cont_lbl;"
					"change_a:"
						"subq.w #1,%6;"
						"move.w %6,%3;"
					"ok_comp:"
						: "+&a" (s1),"+&a" (s2), "+g" (a), "+g" (b)
						: "d" ((char)0xDF), "d" (m)
						: "d0","d1");*/
					do {
						if (!(c=*s2++) && !*s1) {
							if (!(c=strcmp(mp[m],p)))
								goto cont;
							c+=OR_VAL;	// very important to be '+' !!!
							break;
						}
						c|=OR_VAL;
					} while ((*s1++|OR_VAL)==c);
					if ((s1[-1]|OR_VAL)<c)	// il faut aller plus haut
						b=m-1;
					else a=m+1;
				}
				if ((++b)==nm) b--;
				mp+=b;
				//memmove4_plus4((long *)mp,nm-b); // stands for memmove(mp+1,mp,(nm-b)*4), with (nm-b)>=0
				dual_memmove4_plus4((long *)mp,(long *)&mdata[b],nm-b);
				*mp=p; mdata[b]=p0;
				if (++nm>=MM) return nm-1;
			}
/*			if (*p && (nm=cat_ins(p,match,nm))>=MM)
				return 0;*/
//		asm volatile("cont_lbl: ziorjieog");
		cont:
			while (*p++);
			z=(p[2]<<8)|p[3];
			if (z) PUSH(z);
			z=(*p++)<<8; z+=*p;
			if (!z) POP(z);
		} while (z);
	}
	return nm-1;
}

/*int cat_prep(int MM,char **match) {
	int nm_old=nmatch,n;
	pch_search("",1,MM,match);
	n=nmatch;
	nmatch=nm_old;
	return n;
}*/
void cat_list(SCROLL *s,char **match,int x,int w,int y) {
	int i,h; char **cs;
	ScrollUpd(s);
	cs=&match[i=s->scr];
	h=s->h;
	while (h--) {
		if (i!=s->sel) DStr(x,y,*cs++);
		else DStrS(x,y,*cs++,w);
		y+=6; i++;
	}
}
#define SEARCH_FUNC(name,TYPE,comp,conv) void cat_##name##_search(SCROLL *s,char **match,TYPE c) { \
	char **p=&match[s->sel]; int sel=s->sel; \
	c=conv(c); \
	if (comp(c,*p)<0) p=match,sel=0; \
	while (comp(c,*p)>0) p++,sel++; \
	if (sel>=s->n) sel=s->n-1; \
	if (s->sel!=sel) s->scr=s->n-s->h/* (max value for s->scr) */,s->sel=sel; \
}
#define toindiff(x) ((x)|0x20)
int strcmpindiff(char *a,char *b) {
	char c;
	do {
		c=*a++;
		if (toindiff(c)-toindiff(*b++))
			return toindiff(c)-toindiff(b[-1]);
	} while (c);
	return 0;
}
#define letter_comp(ch,s) ((ch)-toindiff(*(s)))
SEARCH_FUNC(letter,char,letter_comp,toindiff)
#define identity(x) (x)
SEARCH_FUNC(str,char *,strcmpindiff,identity)

//char str[]="CATALOG";
int flags(char *p) {
	while (*p++);
	return ((char *)p)[2*3-2]<<8;
}
#ifdef NO_PCH_UTIL
		//char pchUnpackBuf[500];
		char *pch_unpack(char *p,char *p0) {
			char *name=p,*args=NULL,*defn,*proto;
			int flags;
			while (*p++);
			p+=2*3;
			if ((flags=((char *)p)[-2]<<8)&PCHID_MACRO) {
				args=p;
				while (*p)
					while (*p++);
				p++;
			}
			defn=p;
			while (*p++);
			proto=p;
			p0+=((PCH_HEAD *)p0)->dic_off;
			if (*proto)
				return sUnpack(proto,sUnpackBuf,p0);
			if (args) {
				char *d=sUnpackBuf;
				*d++='_';
				*d++='(';
				p=args;
				while (*p) {
					while ((*d++=*p++));
					d--;
				}
				strcpy(d,") = " DOTS);
			} else if (*defn) {
				char *d=sUnpackBuf;
				strcpy(d,"_ = "); d+=4;
				if (flags&PCHID_PACKED) sUnpack(defn,d,p0);
				else strcpy(d,defn);
			} else return "[no prototype]";
			return sUnpackBuf;
		}
		char *pch_unpack2(char *p,char *p0) {
			char *name=p,*args=NULL,*defn,*proto;
			int flags; char *d;
			while (*p++);
			p+=2*3;
			if ((flags=((char *)p)[-2]<<8)&PCHID_MACRO) {
				args=p;
				while (*p)
					while (*p++);
				p++;
			}
			defn=p;
			while (*p++);
			proto=p;
			p0+=((PCH_HEAD *)p0)->dic_off;
			if (*proto)
				return sUnpack(proto,sUnpackBuf,p0);
			d=sUnpackBuf;
			d+=sprintf(d," > Implementation :\n");
			if (args || *defn) {
				d+=sprintf(d,"#define %s",name);
				if (args) {
					*d++='(';
					p=args;
					while (*p) {
						while ((*d++=*p++));
						d--;
					}
					*d++=')';
				}
				*d++=' ';
				if (flags&PCHID_PACKED) sUnpack(defn,d,p0);
				else strcpy(d,defn);
				while (*d++); d--;
			}
			if (*proto) {
				*d++='\n';
				sUnpack(proto,d,p0);
			}
			return sUnpackBuf;
		}
#else
PU_HANDLER (catHelp) {
	char *d=sUnpackBuf;
	d+=sprintf(d," > Implementation :\n");
	if (*args || *defn) {
		d+=sprintf(d,"#define %s%s %s",name,args,defn);
		if (*trueproto) *d++='\n';
	}
	if (*trueproto)
		d+=sprintf(d,"%s",trueproto);
	return sUnpackBuf;
}
PU_HANDLER (catMsg) {
	char *d=sUnpackBuf;
	if (*proto) {
		strcpy(d,proto);
		return d;
	}
	if (*args)
		sprintf(d,"_%s = "DOTS,args);
	else if (*defn)
		sprintf(d,"_ = %s",defn);
	else
		return "[no prototype]";
	return d;
}
#endif
char cat_retbuf[80];
char *Catalog() {
	int n; char *ret=NULL;
	char **match,**mdata;
	if (!catmatch) {
		if (!(catmatch=match=malloc(MAX_CATALOG_ENTRIES*4*2))) return ret;
		mdata=match+MAX_CATALOG_ENTRIES;
		SimpleDlg(NULL,"\n Loading catalog"DOTS" \n",B_CENTER,W_NOKEY);
		LCD_restore(Port);
		catnum=n=cat_prep(MAX_CATALOG_ENTRIES,match,mdata);
	} else n=catnum,match=catmatch,mdata=match+MAX_CATALOG_ENTRIES;
	#ifndef PEDROM
	unsigned char alphaState;
	alphaLockOn(&alphaState);
	#endif
	if (n) {
		int key,height; SCROLL s; char title[100]; WIN_RECT win;
		int last_sel=-1;
		EDIT_LINE search;
		sprintf(title,"Catalog - %d entries",n);
		s.h=min(_89_92(12,16),n); s.n=n; ScrollInit(&s);
		if (curword[0])
			cat_str_search(&s,match,curword);
		// true_h = s.h*6 + 8(title) + 4(border) + 8(search line)
		height=s.h*6;
		while (1) {
			dialog(title,128,height,B_ROUNDED|B_CENTER,&win);
			cat_list(&s,match,win.x0>>2,((win.x1-win.x0)>>2)+1,win.y0);
			if (s.sel!=last_sel)
#ifdef NO_PCH_UTIL
				last_sel=s.sel, memcpy(msgBuf,pch_unpack(match[s.sel],mdata[s.sel]),58), msg=msgBuf, msgBuf[58]=0;
#else
				last_sel=s.sel, memcpy(msgBuf,PchUnpack(catMsg,match[s.sel],mdata[s.sel]),58),
					msg=msgBuf, msgBuf[58]=0;
#endif
			if (!(key=WaitLoop(NULL))) continue;
			if (key==KEY_ESC) break;
			else if (key==KEY_ENTER) {
				strcpy(ret=cat_retbuf,match[s.sel]); break;
			} else if (key==KEY_UP)
				repeated=0, ScrollUp(&s);
			else if (key==KEY_DOWN)
				repeated=0, ScrollDn(&s);
			else if (key==KEY_2ND+KEY_UP)
				ScrollChgWrapOnlyAtEnd(&s,-s.h);
			else if (key==KEY_2ND+KEY_DOWN)
				ScrollChgWrapOnlyAtEnd(&s,s.h);
			else if (key==KEY_DMD+KEY_UP)
				s.sel=0;
			else if (key==KEY_DMD+KEY_DOWN)
				s.sel=-1;
			else if (key>=32 && key<=255 && isidch(key))
				cat_letter_search(&s,match,key);
			else if (key==KEY_F1)
#ifdef NO_PCH_UTIL
				SimpleDlg("Help",pch_unpack2(match[s.sel],mdata[s.sel]),B_CENTER,W_NORMAL);
#else
				SimpleDlg("Help",PchUnpack(catHelp,match[s.sel],mdata[s.sel]),B_CENTER,W_NORMAL);
#endif
			if (repeated)			// happens only if the key was repeated AND 'repeated' wasn't cleared
				ScrollUpdNoWrap(&s);
		}
	}
	#ifndef PEDROM
	SetAlphaStatus(alphaState);
	#endif
	return ret;
}

#include "Dialogs.h"
