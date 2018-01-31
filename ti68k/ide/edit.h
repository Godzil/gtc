// Text editing routines
#include "limits.h"
#ifdef _89
#define WIDTH 39
#else
#define WIDTH 59
#endif
#define NEWLINE 13

enum { EDEX_NONE=0, EDEX_COMPILE };
int editor_exit_requested=0;

#define MAX_CURWORD 31
char *tptr=0; HANDLE hd=0;
char *msg=0,curword[MAX_CURWORD+1]={0},parword[MAX_CURWORD+1]={0};
unsigned int spos=0,cpos=0,curpos=0,selpos=0,sel1=0,sel2=0,size=0,hsize=0/*,is_nl=0*/;
unsigned int par1=0,par2=0,cwpos=0;
int xcur=0/*,ycur=0*/;
int YMAX=0,NLINES=0;
#define is_nl (!spos || (tptr[spos-1]==NEWLINE))
#define TAB
#ifdef TAB
#define tab_chk_l(x) ({if (!tab_chk(x)) (x)--;(void)0;})
#define tab_chk_r(x) ({if (!tab_chk(x)) (x)++;(void)0;})
int even(unsigned int pos) {
	char *p=&tptr[pos];
	int n=0,z=pos;
	while (*--p!=NEWLINE && z) n++,z--;
	/* NEWLINE " " "  123456"
	 *               ^ here we are */
	return n&1;
}
int tab_chk(unsigned int pos) {
	char *p=&tptr[pos];
	if (*p==' ' && p[-1]==' ') {	/* we must be on an odd position and between two spaces */
		int n=0,z=pos;
		while (z && *--p!=NEWLINE) n++,z--;
		/* NEWLINE " " "  123456"
		 *               ^ here we are */
		return n&1;
	}
	return 1;
}
int whitespace(unsigned int pos,unsigned int maxpos) {
	unsigned int cur;
	while (pos>1 && tptr[pos-2]!=NEWLINE) pos--;
	cur=pos;
	while (cur<maxpos && tptr[cur]==' ') cur++;
	return cur-pos;
}
#else
#define tab_chk_l(x) ({(void)0;})
#define tab_chk_r(x) ({(void)0;})
#endif
unsigned int __attribute__((__regparm__(1))) linebegin(unsigned int pos) {
	while (pos>1 && tptr[pos-2]!=NEWLINE) pos--;
	return pos;
}
char * __attribute__((__regparm__(1))) nonwhite(char *p) {
	while (isspace(*p)) p++;
	return p;
}
unsigned int down(unsigned int pos,int xd) {
	char c,*p=tptr+pos;
	int d=WIDTH,n=0;
	while ((c=*p++)) {
		if (--d<=0) return p-1-tptr;
		if (c==NEWLINE) {
			if (n || xd>d) return p-1-tptr;
			p++;
			n++; d=xd;
		}
	}
	return p-1-tptr;
}
unsigned int up(unsigned int pos,int xd) {
	char c,*p=tptr+pos;
	int d=WIDTH-1,w,z;
	while ((c=*--p),p>tptr) {
		if (--d<=0 && p[-1]!=NEWLINE) return p-tptr;
		if (c==NEWLINE)
			goto loop2;
	}
	return 1;
loop2:
	if (p==tptr+1)
		return 1;
	w=-1; z=0;
	while ((c=*--p),p>tptr) {
		w++;
		if (c==NEWLINE) {
			w--;
			break;
		}
	}
	if (p==tptr) w++;
	while (w>=WIDTH-1)
		w-=WIDTH-1, z+=WIDTH-1;
	if (w>=xd-1) w=xd-1;
	return w+z+(p==tptr?1:2+(p-tptr));
}
/*void scroll(int d) {
	
}*/
int need_go_down() {
	char c,*p=tptr+spos;
	int x=1,y=2,d=curpos+1-spos;
#ifdef TITLEBAR
	y=8;
#endif
	if (is_nl) x=0;
	while ((c=*p++)) {
		if (!--d) return 0;
		if (c==NEWLINE) {
			x=0; if ((y+=6)>=YMAX) return 1;
		} else if (++x>=WIDTH) {
			x=1;
			if ((y+=6)>=YMAX) return 1;
		}
	}
	return --d;
}
unsigned int go_down() {
	char c,*p=tptr+spos;
	int x=1,y=0,d=curpos+1-spos;
	if (is_nl) x=0;
	while ((c=*p++)) {
		if (!--d) return 0;
		if (c==NEWLINE) {
			x=0; if (++y>=(NLINES+1)/2) return p-1-tptr;
		} else if (++x>=WIDTH) {
			x=1;
			if (++y>=(NLINES+1)/2) return p-1-tptr;
		}
	}
#ifndef NDEBUG
	if (--d) msg="GO_DOWN";
#endif
	return 0;
}
void view_cursor() {
	if (is_nl) spos++;
	while (curpos<spos) spos=up(spos,1);
	if (spos+WIDTH*20<curpos) spos=linebegin(curpos-WIDTH*20);
//	while (need_go_down()) spos=down(spos,1);
	{
		unsigned int pos,zpos=spos,zzpos=zpos;
		while ((pos=go_down())) zzpos=zpos,zpos=spos,spos=pos;
		spos=zzpos;
		while (need_go_down()) spos=down(spos,1);
	}
	if (spos==1 || tptr[spos-2]==NEWLINE) spos--;
}
enum {
	CF_TYPE=0x1, CF_PREPROC=0x2,
	CF_C=0x4, CF_ASM=0x8, CF_LIBFUNCS=0x10,
	CF_GLOBAL=0x20, CF_DECL=0x40,
	CF_EXPR=0x80, CF_STMT=0x100,
};
#define valid(x) ((int)(x)>=0)
int get_zone(unsigned int pos) {
	int z=0;
	char *lb=tptr+linebegin(pos),*nwlb=nonwhite(lb);
//	char pline=(lb>tptr+1?lb[-2]:0);
//	unsigned int accol[20],accoldeep=0;
	if (*nwlb=='#') z|=CF_PREPROC;
	if (nwlb==tptr+pos) z|=CF_DECL;
	if (nwlb==lb) z|=CF_GLOBAL;
	return z;
}
#ifdef TITLEBAR
char curFileName[8+1],curFileExt[4+1],curFileFullName[17+1];
int numOpenFiles=0,curFileNum=0;
int CompUnderWay=0;
/* 89 title bar :
 *  [--------------------------------------]
 *  [GT-Dev - 12345678.text     Compiling..]
 *  [GT-Dev - 12345678.prgm           10/12]
 *  [--------------------------------------]
 */
#define TBAR_HEIGHT 7
void TBarDisp() {
	char b[100];
	memset(Port,0,30*TBAR_HEIGHT);
	sprintf(b,"GT-Dev - %s.%s",curFileName,curFileExt);
	SmallDStr(1,1,b);
	sprintf(b,"%d/%d",curFileNum+1,numOpenFiles);
	if (CompUnderWay) sprintf(b,"Compiling"DOTS);
	SmallDStr(_89_92(39,59)-strlen(b),1,b);
	ScrRectFill2(&(SCR_RECT){{0,0,239,TBAR_HEIGHT-1}},A_XOR);
	DrawPix(0,0,A_XOR);
	DrawPix(0,TBAR_HEIGHT-1,A_XOR);
	DrawPix(_89_92(159,239),0,A_XOR);
	DrawPix(_89_92(159,239),TBAR_HEIGHT-1,A_XOR);
}
#endif
void display(int z) {
	char c,*p=tptr+spos;
	int x=1,y=2,d=curpos+1-spos;
#ifdef TITLEBAR
	TBarDisp();
	y=TBAR_HEIGHT+1;
#endif
	unsigned int j=spos-1;
	int newline=is_nl;
#ifndef RELEASE
	xcur=-1;
#endif
	while ((c=*p++)) {
		if (!--d) { xcur=x,ycur=y;
			if (!z) FastDrawLine(Port,virtual_to_physical(x)-1,y,virtual_to_physical(x)-1,y+char_height-2,A_XOR)
				/*FastDrawVLine(Port,virtual_to_physical(x)-1,y,y+4,A_XOR)*/; }
		j++;
		if (newline) {
			VariableDChar(0,y,':'), newline=0;
			continue;
		}
		if (c==NEWLINE) {
			if (j>=sel1 && j<sel2) {
#ifndef FORCE_SMALL_FONT
#error hoooo
#endif
				int n; char *p;
				VariableDChar(x-virtual_char_width,y,0);
				VariableDCharS(x,y,' ');
				x++; x>>=1;
				p=Port+(y*30+x);
				n=6; while (n--) {
					int i=30-x;
					while (i--) *p++=0xFF;
					p+=x;
				}
			}
			x=1; if ((y+=6)>=YMAX) return;
			newline=1;
		} else {
			if ((j>=sel1 && j<sel2) || j==par1 || j==par2) {
				VariableDCharS(x,y,c);
				if (j==sel1 || x==1) VariableDChar(x-virtual_char_width,y,0);
			}
			else VariableDChar(x,y,c);
			if (++x>=WIDTH) {
				x=1;
				if ((y+=6)>=YMAX) return;
			}
		}
	}
	if (!--d) { xcur=x,ycur=y; if (!z) FastDrawLine(Port,virtual_to_physical(x)-1,y,virtual_to_physical(x)-1,y+char_height-2,A_XOR); }
}
int last_act=0;
void sel_del() {
	int n=sel2-sel1;
	if (!n) {
	   selpos=0;
	   return;
	}
	curpos=sel1;
	view_cursor();		/* otherwise spos might be in the middle of a line */
	memmove(tptr+sel1,tptr+sel2,size+1+1-sel2);
	size-=n; curpos=cpos=sel1; selpos=0;
}
void insert(int n) {
	size+=n;
	if (size+2+1+1>hsize)
		HeapUnlock(hd), HeapRealloc(hd,2+(hsize=size+100+2+1+1)), tptr=4+HLock(hd);
	memmove(tptr+cpos+n,tptr+cpos,size-n+1+1-cpos);
	cpos+=n;
	last_act=1;
}
void text_enclose(char *l,char *r) {
	int n=sel2-sel1;
	int a=strlen(l),b=strlen(r);
	unsigned int cps;
	if (!n) insert(a+b),cpos-=b,memcpy(tptr+(cpos-a),l,a),memcpy(tptr+cpos,r,b);
	else {
		cps=cpos;
		cpos=sel1;
		insert(a); sel2+=a;
		memcpy(tptr+sel1,l,a); sel1+=a;
		cpos=sel2;
		insert(b);
		sel2-=b;
		memcpy(tptr+sel2+1,r,b);
		cpos=cps+a;
		selpos+=a;
	}
}
#define isidch(x) ({char __x=(x); \
	(__x>='0' && __x<='9') || (__x>='A' && __x<='Z') || (__x>='a' && __x<='z') || __x=='_' \
	|| __x=='$' || __x=='#';})
#define isnum(x) ({char __x=(x); __x>='0' && __x<='9';})
int chk_curword(char *curword,unsigned int pos,int store_cwpos) {
	char w[MAX_CURWORD+2],*p=tptr+pos;
	int res;
	if (!sel1) {
		while (isidch(*--p) && p>tptr);
		p++;
		if (p==tptr || p[-1]==NEWLINE) p++;	/* since we may have the sequence : #NEWLINE "C" ... */
		while (isnum(*p)) p++;
		if (store_cwpos) {
			cwpos=p-tptr;
			if (cwpos>pos) goto invalid;	/* may appear with numbers, for example : 123|456 */
		}																/* (note : if store_cwpos==0, then it isn't a number :] ) */
		memcpy(w,p,MAX_CURWORD+2);
		p=w;
		p[MAX_CURWORD+1]=0;
		while (isidch(*p++));
		p[-1]=0;
		if (p==&w[MAX_CURWORD+2]) goto invalid;
		res=strcmp(curword,w);
		if (res) memcpy(curword,w,MAX_CURWORD+1);
		return res;
	}
invalid:
	if (store_cwpos) cwpos=0;
	res=curword[0];
	curword[0]=0;
	return res;
}

FILE *pchfile[MAX_OPEN_PCH];
char *pchdata[MAX_OPEN_PCH];
#undef pchhead
#define pchhead ((PCH_HEAD **)pchdata)
int pchnum = 0;
int add_pch(char *name) {
	if (pchnum>=MAX_OPEN_PCH)
		return 0;
	char b[30];
	sprintf(b,"zheader\\%s",name);
	if (!(pchfile[pchnum] = fopen(b, "rb"))) return 0;
#ifdef PC
	pchdata[pchnum] = malloc(MAX_PCHDATA_SIZE);
	fread(pchdata[pchnum],1,MAX_PCHDATA_SIZE,pchfile[pchnum]);
	if (((PCH_HEAD *)pchdata[pchnum])->magic!=
		((long)'P'<<0)+((long)'C'<<8)+((long)'H'<<16))
		return 0;
#else
	if (((PCH_HEAD *)(pchdata[pchnum] = *(char **)(pchfile[pchnum])))->magic!=
		((long)'P'<<24)+((long)'C'<<16)+((long)'H'<<8)) {
		fclose(pchfile[pchnum]);
		return 0;
	}
#endif
	pchnum++;
	return 1;
}
void add_all_pch() {
    /* first add standard headers: if MAX_CATALOG_ENTRIES is reached then we still have those */
    add_pch("keywords");
    add_pch("stdhead");
    SYM_ENTRY *se = SymFindFirst($(zheader), FO_SINGLE_FOLDER);
    while (se)
	add_pch(se->name), se = SymFindNext();
}
void close_pch(void) {
	while (pchnum) {
		pchnum--;
#ifdef PC
		free(pchdata[pchnum]);
#endif
		fclose(pchfile[pchnum]);
	}
}
#define MAX_MATCH 16
int nmatch=0;
char *match[MAX_MATCH]={0};
//char *str="SSORT";
void ssort(char **match,int n) {
	char **zsp=&match[n-1];
	while (zsp>match) {
		#define cs (*csp)
		char **csp=match,*ms=cs,**msp=csp;
		/* ms=max string */
		/* cs=current string */
		do {
			csp++;
			if (strcmp(ms,cs)<0) msp=csp,ms=cs;
		} while (csp<zsp);
		#undef cs
		*msp=*zsp;
		*zsp=ms;
		zsp--;
	}
}
#ifdef NO_PCH_UTIL
		char *pch_help(char *p,char *p0) {
			char *args=NULL,*defn,*proto;
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
				strcpy(d,") = \xA0\x01");
			} else if (*defn) {
				char *d=sUnpackBuf;
				if (flags&PCHID_PACKED) sUnpack(defn,d,p0);
				else strcpy(d,defn);
			} else return "";
			return sUnpackBuf;
		}
#else
/*char *PchRCB_editMsg(char *name,char *args,char *defn,char *proto,char *param) {*/
PU_HANDLER (editMsg) {
	char *d=sUnpackBuf;
	if (*proto) {
		strcpy(d,proto);
		return d;
	}
	if (*args)
		sprintf(d,"_%s;",args);
	else if (*defn) {
		if (!strncmp(defn,"_rom_call(",10)) {
			char *s=defn+10;
			d=write_until(d,&s,',');
			if (d[-1]!='*') *d++=' ';
			*d++='_';
			d=write_until(d,&s,',');
			*d++=';';
			*d=0;
		} else
			strcpy(d,defn);
	} else
		return "";
	return sUnpackBuf;
}
#endif
SCROLL ac_s={0,0,0,0};
//char *str="PCH_SEARCH";
#define PUSH(x) asm("move.w %0,-(%%sp)"::"r" ((int)x))
#define POP(x) asm("move.w (%%sp)+,%d0":"=d" (z))
/*  --- CAUTION :                                         */
/* USAGE OF PUSH AND POP MAKES IT VERY COMPILER-DEPENDENT */
/* (for example, stack frames are necessary)              */
char *pch_search(char *curword,int ac_test,int MM,char **match) { /* match may be NULL if ac_test is 0 */
	/* >>> !!! the routine does NOT reset nmatch !!! <<< */
	int n=pchnum,nm=MM; char **mp=match;
	#define search ac_test
	/*int search;
	search=(strlen(curword)<3)?0:ac_test;
	if (ac_test) nmatch=0;*/
	while (n--) {
		unsigned char *p0=pchdata[n],*p,c,*q; int z=PCH_HEAD_SIZE;
		PUSH(0);
		do {
			p=p0+z; q=curword;
			while ((c=*q++) && c==*p++);
			if (!c) {
				if (!*p) {
	/*				p+=1+2*3;
					while (*p++);
					return sUnpack(p,sUnpackBuf,p0+((PCH_HEAD *)p0)->dic_off);*/
#ifdef NO_PCH_UTIL
					return pch_help(p0+z,p0);
#else
					return PchUnpack(editMsg,p0+z,p0);
#endif
				} else if (search) {
					*mp++=p0+z;
					if (!--nm) search=0;
					else {
						while (*p++);
						z=(p[2]<<8)|p[3];
						if (z) PUSH(z);
						goto cont;
					}
				} else p++;
			}
			if ((c=p[-1])) while (*p++);
			if (c<q[-1]) p+=2;
		cont:
			z=(*p++)<<8; z+=*p;
			if (!z) POP(z);
		} while (z);
	}
	if (search)
		nmatch=MM-nm,ssort(match,nmatch),ScrollInit(&ac_s);
	#undef search
	return 0;
}

void ac_display(int has_sel) {
	int x0=xcur,y,h,n; char **cs;
	y=disp_box(4,35,6*(h=(nmatch>=6?6:nmatch)),B_NORMAL,NULL);
	ac_s.n=nmatch; ac_s.h=h;
	ScrollUpd(&ac_s);
	cs=&match[n=ac_s.scr];
	while (h--) {
		if (!has_sel || n!=ac_s.sel) DStr(5,y,*cs++);
		else DChar(3,y,0),DCharS(4,y,' '),DStrS(5,y,*cs++,35+1-5);
		y+=6; n++;
	}
	if (ScrollHasUpArrow(&ac_s)) DCharX(4,y-6*6,UPARR);
	if (ScrollHasDnArrow(&ac_s)) DCharX(4,y-6*1,DNARR);
}
void ac_do() {
	char *s; int l=strlen(curword),n;
	/* it's reasonable to assume that 'ac_disp' has already been called :) */
	ScrollUpd(&ac_s);
	s=match[ac_s.sel];
	cpos=cwpos+l; selpos=0;
	insert(n=strlen(s)-l);
	memcpy(&tptr[cpos-n],s+l,n);
}

#define SEQ(x) x "\0"
char charmap[]=
		SEQ("'")
		SEQ("_")
		SEQ(";")
		SEQ(":")
		SEQ("\"")
		SEQ("\\")
		SEQ("<")
		SEQ(">")
		SEQ("!")
		SEQ("&")
		SEQ("@")
		SEQ("<=")
		SEQ(">=")
		SEQ("!=")
		SEQ("  // ")
		SEQ("#")
		SEQ(" ")
		SEQ("int ")
		SEQ("long ")
		SEQ("char ")
		SEQ("unsigned ")
		SEQ("void ")
	SEQ("");
#define KEY_UMINUS 173
#define KEY_STO 22
/*#define KEY_CONVERT 18*/
#define KEY_HOME 0x115
#define KEY_CATALOG 0x116
#define KEY_APPS 0x109
#define KEY_2ND 4096
#define KEY_DMD 16384
#define KEY_SHF 8192
int key89[]={
#define pair(k) 337+k,340+k
	pair(0),pair(KEY_2ND),pair(KEY_DMD),pair(KEY_SHF),pair(KEY_SHF+KEY_2ND),pair(KEY_SHF+KEY_DMD),
#undef pair
#define pair(k) 338+k,344+k
	pair(0),pair(KEY_2ND),pair(KEY_SHF),pair(KEY_SHF+KEY_2ND),
#undef pair
	KEY_BACKSPACE,16384+KEY_BACKSPACE,	/* BACKSPACE/DELETE */
	KEY_DMD+4096,KEY_DMD+8192,KEY_DMD+KEY_ESC,	/* CUT/COPY/PASTE */
	KEY_DMD,	/* for DIAMOND+# (kbdprgm#) */
	KEY_CATALOG,	/* CATALOG */
	KEY_F8,KEY_DMD+KEY_F3,	/* find/replace */
	KEY_HOME,	/* HOME */
	KEY_2ND+'+',	/* CHAR */
	#ifdef EVHOOK
	// Forbidden keys
	KEY_2ND+'6',KEY_2ND+'3',KEY_2ND+'-',KEY_2ND+KEY_ENTER,	/* MEM/UNITS/VAR-LINK/ENTRY */
	KEY_2ND+KEY_HOME,KEY_DMD+'|',	/* CUSTOM/HELPKEYS */
	KEY_DMD+',',KEY_DMD+KEY_UMINUS,	/* SYSDATA/HOMEDATA */
	0,
	#endif
	// Special keys
	KEY_2ND+'=',KEY_DMD+KEY_MODE,KEY_2ND+'9',
	KEY_2ND+'4',KEY_2ND+'1',KEY_2ND+'2',KEY_2ND+'0',
	KEY_2ND+'.',KEY_DMD+'/',KEY_DMD+'*',KEY_DMD+KEY_STO,
	KEY_DMD+'0',KEY_DMD+'.',KEY_DMD+'=',KEY_DMD+')',
	KEY_DMD+',',KEY_UMINUS,
	18/*KEY_2ND+KEY_MODE*/,151/*KEY_2ND+KEY_CATALOG*/,KEY_2ND+KEY_APPS,	/* int/long/char */
	KEY_DMD+KEY_CATALOG,KEY_DMD+KEY_APPS,										/* unsigned/void */
	0
};
#undef KEY_DMD
#undef KEY_SHF
#define KEY_DMD 8192
#define KEY_SHF 16384
#ifdef PEDROM
#define KA_MOD 32
#else
#define KA_MOD 0
#endif
int key92[]={
#define pair(k) 338+k,344+k
	pair(0),pair(KEY_2ND),pair(KEY_DMD),pair(KEY_SHF),pair(KEY_SHF+KEY_2ND),pair(KEY_SHF+KEY_DMD),
#undef pair
#define pair(k) 337+k,340+k
	pair(0),pair(KEY_2ND),pair(KEY_SHF),pair(KEY_SHF+KEY_2ND),
#undef pair
	KEY_BACKSPACE,8192+KEY_BACKSPACE,	/* BACKSPACE/DELETE */
	KEY_DMD+KA_MOD+'X',KEY_DMD+KA_MOD+'C',KEY_DMD+KA_MOD+'V',	/* CUT/COPY/PASTE */
	KEY_DMD,	/* DIAMOND+# (kbdprgm#) */
	KEY_2ND+'2',	/* CATALOG */
	KEY_2ND+KEY_F3,KEY_DMD+KEY_F3,	/* find/replace */
	KEY_DMD+KA_MOD+'Q',	/* HOME */
	KEY_2ND+'+',	/* CHAR */
	#ifdef EVHOOK
	// Forbidden keys
	#error missing for 92
	#endif
	// Special keys
	KEY_2ND+KA_MOD+'B',KEY_2ND+KA_MOD+'P',KEY_2ND+KA_MOD+'M',
	KEY_2ND+136/*key_theta*/,KEY_2ND+KA_MOD+'L',KEY_2ND+'=',KEY_2ND+'0',
	KEY_2ND+'.',KEY_2ND+KA_MOD+'H',KEY_2ND+KA_MOD+'W',KEY_2ND+KA_MOD+'R',
	KEY_DMD+'0',KEY_DMD+'.',KEY_DMD+'=',KEY_2ND+KA_MOD+'X',
	KEY_2ND+KA_MOD+'T',1,
	1,1,1,	// -> disable
	1,1,	// -> disable
//	18/*KEY_2ND+KEY_MODE*/,151/*KEY_2ND+KEY_CATALOG*/,KEY_2ND+KEY_APPS,	/* int/long/char */
//	KEY_DMD+KEY_CATALOG,KEY_DMD+KEY_APPS,										/* unsigned/void */
	0
};
#undef KEY_DMD
#undef KEY_SHF
#define KEY_DMD q89(16384,8192)
#define KEY_SHF q89(8192,16384)


void DrawStatus() {
	int x=64; char *p=icons;
	if (MenuContents && !(ST_flagsL & (x-1))) { DrawMenuBar(); return; }
	char *q0=Port+_89_92((100-7)*30,(128-7)*30),*q;
	memset(q0-30,0xFF,30);
	memset(q0,0,30*7);
	DStr(2,LCD_HEIGHT-6,msg);
	while ((x>>=1)) {
		if (ST_flagsL & x) {
			int n=7;
			q=q0;
			while (n--) *q=*p++, q+=30;
		} else
			p+=7;
	}
}
char msgBuf[58+1]; /* max string length on 92/V200 is 240/4-2=58 */
int *get_keylist() {
	return _89_92(key89,key92);
}
char *Catalog();
char CharDlg();
int cKID=200,cBKD=35;
void Off() {
	off();
	OSInitKeyInitDelay(cKID);
	OSInitBetweenKeyDelay(cBKD);
}
char **catmatch=0; int catnum=0;
void FreeMem() {
	if (catmatch) free(catmatch),catmatch=0;
}

int ac_on=0,ac_disp=0;
