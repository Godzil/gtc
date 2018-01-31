//void *Port=0;
#define DrawSStr(x,y,s) DStr((x)>>2,y,s)
/*void DrawSStr(int x,int y,char *s) {
	DStr(x>>2,y,s);
}*/

typedef struct {
	int i,t;
	void *d;
} XP_S;
typedef struct {
	XP_S *s;
	int sh,sel,msel;
} XP_C;

int XpMove(XP_C *xc,int n) {
	XP_S *xs=xc->s+xc->sel,*lp=xc->s;
	int d=(n>=0?1:-1);
	while (n || xs->i<=0) {
		if (!xs->i) return lp-xc->s;
		if (xs->i>0) n-=d,lp=xs;
		xs+=d;
	}
	return xs-xc->s;
}
char XpSprite[8*5]={
	0,0,0,0,0,0,0,0,
	0b0110000,
	0b1001100,
	0b1110010,
	0b1001110,
	0b1000010,
	0b0111100,
	0,0,
	0b0000000,
	0b0110000,
	0b1001110,
	0b1000010,
	0b1000010,
	0b0111100,
	0,0,
	0b0000000,
	0b1111100,
	0b1000110,
	0b1000010,
	0b1000010,
	0b1111110,
	0,0,
	0b0000000,
	0b1111100,
	0b1000110,
	0b1011010,
	0b1000010,
	0b1111110,
	0,0,
};

/*	[IFT_TEXT] 4,
	[IFT_RES]  5,
	[IFT_C]    6,
	[IFT_H]    7,
	[IFT_PRGM] 8,
	[IFT_FUNC] 8,
	[IFT_XFUNC]8,
	[IFT_PACK] 9,
	[IFT_EXPR] 10,*/

#define XP_X1 24
#define XP_X2 136
#define XP_N 8
#define XP_H 6
/*#define XP_Y1 14
#define XP_Y2 (XP_Y1+8*XP_N)*/
void XpDisp(XP_C *xc,int y) {
	XP_S *xs=xc->s+xc->sh;
	int r=xc->sel-xc->sh,n=XP_N;
	if (xc->sh) DrawChar(XP_X1+1,y,UPARR,A_NORMAL);
	do {
		if (!xs->i) return;
		if (xs->i>0) {
			int x=XP_X1+2-8+6+(xs->i<<3);
			Sprite8(x,y,8,XpSprite+(xs->t<<3),Port,A_NORMAL);
			DrawSStr(x+8,y,(char *)xs->d);
			if (!r) {
				SCR_RECT sr={{XP_X1,y,XP_X2-1,y+XP_H-1}};
				ScrRectFill(&sr,ScrRect,A_XOR);
			}
			y+=XP_H;
			n--;
		}
		xs++; r--;
	} while (n || xs->i<=0);
	DrawChar(XP_X1+1,y-XP_H,DNARR,A_XOR);
}

void XpNeg(XP_C *xc) {
	XP_S *xs=xc->s+xc->sel;
	int z,z0=xs->i,d=0x8000|(0x8000u>>z0);
	xs++;
	z=xs->i;
	if (z<0) d=~d;
	while ((z>0?xs->i:-xs->i)>z0) {
		if (z>0) xs->i|=d;
		else {
			xs->i&=d;
			if (xs->i-8>0) xs->i|=0x8000;
		}
		xs++;
	}
}

typedef int (*__attribute__((stkparm)) XPLOOP_CB)(XP_C *xc,int key,void *ctx);
enum { XPLCB_CONT=0, XPLCB_QUIT=-1, XPLCB_BREAK=1, XPLCB_OPENDIR=2 };
#define DEFINE_XPLOOP_CB(n) int __attribute__((stkparm)) n(XP_C *xc,int key,void *ctx)
DEFINE_XPLOOP_CB(std_callback) {
	return XPLCB_CONT;
}
DEFINE_XPLOOP_CB(onlyfiles_callback) {
	return key==KEY_ENTER && xc->s[xc->sel].t==2 ? XPLCB_OPENDIR : XPLCB_CONT;
}
XP_S *XpLoop(XP_C *xc,int y,XPLOOP_CB callback,void *ctx) {
	int k,z;
	if (!xc->s->i) return 0;
	if (!callback) callback=std_callback;
	DrawFrame(XP_X1-1,y-1,XP_X2,y+XP_H*XP_N);
	do {
		int cb_ret;
		SCR_RECT sr={{XP_X1,y,XP_X2-1,y+XP_H*XP_N-1}};
		//LCD_save(Port);
		ScrRectFill2(&sr,A_REVERSE);
		//asm("0: bra 0b");
		XpDisp(xc,y);
		callback(xc,0,ctx);
		k=WaitLoop(NULL);
		if ((cb_ret=callback(xc,k,ctx))) k=0;
		if (cb_ret==XPLCB_OPENDIR) k=KEY_RIGHT;
		#define xps_sel (xc->s[xc->sel])
		if (k==KEY_UP) xc->sel=XpMove(xc,-1);
		else if (k==KEY_DOWN) xc->sel=XpMove(xc,1);
		else if (k==(KEY_2ND|KEY_UP)) xc->sel=XpMove(xc,1-XP_N);
		else if (k==(KEY_2ND|KEY_DOWN)) xc->sel=XpMove(xc,XP_N-1);
		else if (k==KEY_LEFT) {
			if ((z=xps_sel.i)>=2)
				do xc->sel=XpMove(xc,-1); while ((unsigned)xps_sel.i>=(unsigned)z);
			else if (xps_sel.t==1)
				xps_sel.t=2,XpNeg(xc);
		} else if (k==KEY_RIGHT && xps_sel.t==2)
			xps_sel.t=1,XpNeg(xc);
		else if (k==KEY_ENTER)
			return &xps_sel;
		z=xc->sel;
		if (xc->sh>z) xc->sh=z;
		if (xc->sh<(z=XpMove(xc,1-XP_N))) xc->sh=z;
	} while (k!=KEY_ESC);
	return NULL;
}

long XpFatSub(XP_S *xs,char *acc,char *cfold) {
	SYM_ENTRY *e=SymFindFirst(NULL,FO_RECURSE|FO_SKIP_TEMPS);
	int n=0,n2=0,f=1;
	while (e) {
		long i=0x00010002;
		if (!(e->flags.flags_n&SF_FOLDER)) {
			CESI p=HeapDeref(e->handle);
			if (p[1+*(unsigned int *)p]!=TEXT_TAG) goto nxt;
			i=(f>=0?0xC0020003:0x00020003);
/*			if (p[4]=='P') {
				char *q=strchr(p+5,'\r');
				if (q) {
					n2+=((q-(char *)p)-5)+1;
					if (acc) strncpy(acc,p+5,(q-(char *)p)-5),e=(SYM_ENTRY *)acc,acc+=((q-(char *)p)-5),*acc++=0;
				}
			}*/
			f++;
		} else {
			if (!f) {
				n--;
				if (xs) xs--;
			}
			f=0;
			if (!strcmp(e->name,cfold))
				f=0x8000,i--;
		}
		if (xs) {
			*(long *)&xs->i=i;
			xs->d=(void *)e->name;
			xs++;
		}
		n++;
	nxt:
		e=SymFindNext();
	}
	if (!f) {
		n--;
		if (xs) xs--;
	}
	if (xs) xs->i=0;
	return n+((long)n2<<16);
}
XP_C *XpLoadFat(char *cfold) {
	long r=XpFatSub(NULL,NULL,cfold);
	int n=(int)r,n2=(int)(r>>16);
	XP_C *xc=malloc(sizeof(XP_C)+(n+2)*sizeof(XP_S)+n2);
	XP_S *xs;
	if (!xc) return xc;
	xs=(XP_S *)(xc+1);
	xs->i=0; xs++; xc->s=xs;
	XpFatSub(xs,(char *)(xs+(n+1)),cfold);
	xc->sh=0; xc->sel=0; xc->msel=n-1;
	return xc;
}
XP_C *XpLoadList(char **list,int n) {
	XP_C *xc=malloc(sizeof(XP_C)+(n+2)*sizeof(XP_S));
	XP_S *xs;
	if (!xc) return xc;
	xs=(XP_S *)(xc+1);
	xs->i=0; xs++; xc->s=xs;
	xc->sh=0; xc->sel=0; xc->msel=n-1;
	while (n--)
		xs->i=1,xs->t=0,xs->d=*list++,xs++;
	xs->i=0;
	return xc;
}
