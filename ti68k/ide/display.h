// Displaying routines
void *Port=0;
int ycur=0;
#ifdef FORCE_SMALL_FONT
#define char_width 4
#define virtual_char_width 1
#define char_height 6
#define physical_to_virtual(px) ((px)>>2)
#define virtual_to_physical(vx) ((vx)<<2)
typedef char CHAR[char_height];
#else
int small_font = -1;
int char_width = 0;
int virtual_char_width = 0;
int char_height = 0;
enum { FONT_MEDIUM = 0, FONT_SMALL = 1 };
void set_font(int font) {
    small_font = font;
    char_width = font==FONT_MEDIUM ? 6 : 4;
    virtual_char_width = font==FONT_MEDIUM ? 6 : 1;
    char_height = font==FONT_MEDIUM ? 8 : 6;
}
int physical_to_virtual(int px) {
    if (small_font)
	return px>>2;
    return px;
}
int virtual_to_physical(int vx) {
    if (small_font)
	return vx<<2;
    return vx;
}
#endif

#ifdef __GTC__
void SmallDChar(int x,int y,char c) __attribute__((__regparm__(3,0)));
void SmallDCharS(int x,int y,char c) __attribute__((__regparm__(3,0)));
void SmallDCharX(int x,int y,char c) __attribute__((__regparm__(3,0)));
#define xregparm(g,d,a) __attribute__((regparm(d,a)))
#else
void __attribute__((__regparm__(3))) SmallDChar(int x,int y,char c);
void __attribute__((__regparm__(3))) SmallDCharS(int x,int y,char c);
void __attribute__((__regparm__(3))) SmallDCharX(int x,int y,char c);
#define xregparm(g,d,a) __attribute__((regparm(g)))
#endif
#ifdef __GTC__
#define SECTION_FONT
asm {
	even
SmallDCharX:
	and.w #0xFF,d2
	add.w d2,d2
	move.w d2,a1
	add.w d2,d2
	add.w a1,d2
	lea font(pc),a1
	add.w d2,a1
	add.w d1,d1
	move.w d1,d2
	lsl.w #4,d1
	sub.w d2,d1
	moveq #0x0000000F,d2
	asr.w #1,d0
	bcs.s \X_ok_d2
	moveq #0xFFFFFFF0,d2
\X_ok_d2:
	move.l Port,a0
	add.w d1,d0
	add.w d0,a0
	move.b (a1)+,d0
	and.b d2,d0
	eor.b d0,(a0)
	move.b (a1)+,d0
	and.b d2,d0
	eor.b d0,30(a0)
	move.b (a1)+,d0
	and.b d2,d0
	eor.b d0,60(a0)
	move.b (a1)+,d0
	and.b d2,d0
	eor.b d0,90(a0)
	move.b (a1)+,d0
	and.b d2,d0
	eor.b d0,120(a0)
	move.b (a1),d0
	and.b d2,d0
	eor.b d0,150(a0)
	rts
	even
SmallDCharS:
	move.l Port,a0
	and.w #0xFF,d2
	add.w d2,d2
	move.w d2,a1
	add.w d2,d2
	add.w a1,d2
	lea font(pc),a1
	add.w d2,a1
	add.w d1,d1
	move.w d1,d2
	lsl.w #4,d1
	sub.w d2,d1
	moveq #0x0000000F,d2
	asr.w #1,d0
	bcs.s \S_ok_d2
	moveq #0xFFFFFFF0,d2
\S_ok_d2:
	add.w d1,d0
	add.w d0,a0
	move.b (a1)+,d0
	not.b d0
	and.b d2,d0
	or.b d0,(a0)
	move.b (a1)+,d0
	not.b d0
	and.b d2,d0
	or.b d0,30(a0)
	move.b (a1)+,d0
	not.b d0
	and.b d2,d0
	or.b d0,60(a0)
	move.b (a1)+,d0
	not.b d0
	and.b d2,d0
	or.b d0,90(a0)
	move.b (a1)+,d0
	not.b d0
	and.b d2,d0
	or.b d0,120(a0)
	move.b (a1),d0
	not.b d0
	and.b d2,d0
	or.b d0,150(a0)
	rts
	even
SmallDChar:
	and.w #0xFF,d2
	add.w d2,d2
	move.w d2,a1
	add.w d2,d2
	add.w a1,d2
	lea font(pc),a1
	add.w d2,a1
	add.w d1,d1
	move.w d1,d2
	lsl.w #4,d1
	sub.w d2,d1
	moveq #0x0000000F,d2
	asr.w #1,d0
	bcs.s \ok_d2
	moveq #0xFFFFFFF0,d2
\ok_d2:
	move.l Port,a0
	add.w d1,d0
	add.w d0,a0
	move.b (a1)+,d0
	and.b d2,d0
	or.b d0,(a0)
	move.b (a1)+,d0
	and.b d2,d0
	or.b d0,30(a0)
	move.b (a1)+,d0
	and.b d2,d0
	or.b d0,60(a0)
	move.b (a1)+,d0
	and.b d2,d0
	or.b d0,90(a0)
	move.b (a1)+,d0
	and.b d2,d0
	or.b d0,120(a0)
	move.b (a1),d0
	and.b d2,d0
	or.b d0,150(a0)
	rts
}
#else
#define ASM_SECTION_FONT //".section fontmanip"
#define SECTION_FONT //__attribute__((section("fontmanip")))
asm(ASM_SECTION_FONT "
	.even
	.globl	SmallDCharX
SmallDCharX:
	and.w #0xFF,%d2
	add.w %d2,%d2
	move.w %d2,%a1
	add.w %d2,%d2
	add.w #262,%d2 /* to be able to use PC-relative font */
	add.w %a1,%d2
	lea font-262(%pc,%d2.w),%a1	/* this offset should be 2 (0 doesn't seem to work) */
	add.w %d1,%d1
	move.w %d1,%d2
	lsl.w #4,%d1
	sub.w %d2,%d1
	moveq #0x0000000F,%d2
	asr.w #1,%d0
	bcs.s DChX_ok_d2
	moveq #0xFFFFFFF0,%d2
DChX_ok_d2:
	move.l Port,%a0
	add.w %d1,%d0
	add.w %d0,%a0
	move.b (%a1)+,%d0
	and.b %d2,%d0
	eor.b %d0,(%a0)
	move.b (%a1)+,%d0
	and.b %d2,%d0
	eor.b %d0,30(%a0)
	move.b (%a1)+,%d0
	and.b %d2,%d0
	eor.b %d0,60(%a0)
	move.b (%a1)+,%d0
	and.b %d2,%d0
	eor.b %d0,90(%a0)
	move.b (%a1)+,%d0
	and.b %d2,%d0
	eor.b %d0,120(%a0)
	move.b (%a1),%d0
	and.b %d2,%d0
	eor.b %d0,150(%a0)
	rts
	.even
	.globl	SmallDCharS
SmallDCharS:
	move.l Port,%a0
	and.w #0xFF,%d2
	addq.w #8,%d2 /* to be able to use PC-relative font */
	add.w %d2,%d2
	move.w %d2,%a1
	add.w %d2,%d2
	add.w %a1,%d2
	lea font-48(%pc,%d2.w),%a1
	add.w %d1,%d1
	move.w %d1,%d2
	lsl.w #4,%d1
	sub.w %d2,%d1
	moveq #0x0000000F,%d2
	asr.w #1,%d0
	bcs.s DChS_ok_d2
	moveq #0xFFFFFFF0,%d2
DChS_ok_d2:
	add.w %d1,%d0
	add.w %d0,%a0
	move.b (%a1)+,%d0
	not.b %d0
	and.b %d2,%d0
	or.b %d0,(%a0)
	move.b (%a1)+,%d0
	not.b %d0
	and.b %d2,%d0
	or.b %d0,30(%a0)
	move.b (%a1)+,%d0
	not.b %d0
	and.b %d2,%d0
	or.b %d0,60(%a0)
	move.b (%a1)+,%d0
	not.b %d0
	and.b %d2,%d0
	or.b %d0,90(%a0)
	move.b (%a1)+,%d0
	not.b %d0
	and.b %d2,%d0
	or.b %d0,120(%a0)
	move.b (%a1),%d0
	not.b %d0
	and.b %d2,%d0
	or.b %d0,150(%a0)
	rts
	.even
	.globl	SmallDChar
SmallDChar:
	and.w #0xFF,%d2
	add.w %d2,%d2
	move.w %d2,%a1
	add.w %d2,%d2
	add.w %a1,%d2
	lea font(%pc,%d2.w),%a1
	add.w %d1,%d1
	move.w %d1,%d2
	lsl.w #4,%d1
	sub.w %d2,%d1
	moveq #0x0000000F,%d2
	asr.w #1,%d0
	bcs.s DCh_ok_d2
	moveq #0xFFFFFFF0,%d2
DCh_ok_d2:
	move.l Port,%a0
	add.w %d1,%d0
	add.w %d0,%a0
	move.b (%a1)+,%d0
	and.b %d2,%d0
	or.b %d0,(%a0)
	move.b (%a1)+,%d0
	and.b %d2,%d0
	or.b %d0,30(%a0)
	move.b (%a1)+,%d0
	and.b %d2,%d0
	or.b %d0,60(%a0)
	move.b (%a1)+,%d0
	and.b %d2,%d0
	or.b %d0,90(%a0)
	move.b (%a1)+,%d0
	and.b %d2,%d0
	or.b %d0,120(%a0)
	move.b (%a1),%d0
	and.b %d2,%d0
	or.b %d0,150(%a0)
	rts
");
#endif

const unsigned short SECTION_FONT font[]={
	#include "font.h"
};
#define DOTS "\xA0\x01"
#define DOTS1 '\xA0'
#define DOTS2 '\x01'
#define UPARR '\x13'
#define DNARR '\x14'
char icons[]={
	0b11111100,
	0b11111100,
	0b11000100,
	0b10110100,
	0b10110100,
	0b11000100,
	0b11111100,

	0b11111100,
	0b11001100,
	0b10110100,
	0b10000100,
	0b10110100,
	0b10110100,
	0b11111100,

	0b00000000,
	0b00000000,
	0b00111000,
	0b01001000,
	0b01001000,
	0b00111000,
	0b00000000,
#ifdef _89
	0b00000000,
	0b00100000,
	0b01110000,
	0b11111000,
	0b01110000,
	0b00100000,
	0b00000000,

	0b00000000,
	0b00100000,
	0b01110000,
	0b11111000,
	0b01110000,
	0b01110000,
	0b00000000,
#else
	0b00000000,
	0b00100000,
	0b01110000,
	0b11111000,
	0b01110000,
	0b01110000,
	0b00000000,

	0b00000000,
	0b00100000,
	0b01110000,
	0b11111000,
	0b01110000,
	0b00100000,
	0b00000000,
#endif
	0b00000000,
	0b00110000,
	0b01001000,
	0b00010000,
	0b00100000,
	0b01111000,
	0b00000000,
};

/*void __attribute__((__regparm__(3))) SmallDChar(int x,int y,char c) {
	char *p=((CHAR *)font)[c&255],*s=Port+(x>>1)+y*30;
	int m;
	if (x&1) m=0x0F;
	else m=0xF0;
	s[30*0]|=(*p++)&m;
	s[30*1]|=(*p++)&m;
	s[30*2]|=(*p++)&m;
	s[30*3]|=(*p++)&m;
	s[30*4]|=(*p++)&m;
	s[30*5]|=(*p++)&m;
}
void __attribute__((__regparm__(3))) SmallDCharS(int x,int y,char c) {
	char *p=((CHAR *)font)[c&255],*s=Port+(x>>1)+y*30;
	int m;
	if (x&1) m=0x0F;
	else m=0xF0;
	s[30*0]|=(~*p++)&m;
	s[30*1]|=(~*p++)&m;
	s[30*2]|=(~*p++)&m;
	s[30*3]|=(~*p++)&m;
	s[30*4]|=(~*p++)&m;
	s[30*5]|=(~*p++)&m;
}
*/

void xregparm(2,2,1) SmallDStr(int x,int y,char *s) {
	while (*s)
		SmallDChar(x,y,*s++),x++;
}
void xregparm(3,3,1) SmallDStrS(int x,int y,char *s,int w) {
	SmallDChar(x-1,y,0);
	while (*s)
		SmallDCharS(x,y,*s++),x++,w--;
	while (--w>0)
		SmallDCharS(x,y,' '),x++;
}
void xregparm(3,3,1) SmallDStrC(int x,int y,char *s,int w) {
	while (*s) {
		if (--w<0) return;
		SmallDChar(x,y,*s++),x++;
	}
	while (--w>0)
		SmallDChar(x,y,' '),x++;
}
void xregparm(3,3,1) SmallDStrCS(int x,int y,char *s,int w) {
	SmallDChar(x-1,y,0);
	while (*s) {
		if (--w<0) return;
		SmallDCharS(x,y,*s++),x++;
	}
	while (--w>0)
		SmallDCharS(x,y,' '),x++;
}
#ifdef FORCE_SMALL_FONT
#define VariableDStr SmallDStr
#define VariableDStrS SmallDStrS
#define VariableDStrC SmallDStrC
#define VariableDStrCS SmallDStrCS
#define VariableDChar SmallDChar
#define VariableDCharS SmallDCharS
#define DChar SmallDChar
#define DCharS SmallDCharS
#define DCharX SmallDCharX
#define DStr SmallDStr
#define DStrS SmallDStrS
#else
void xregparm(2,2,1) MediumDStr(int x,int y,char *s) {
	while (*s)
		MediumDChar(x,y,*s++),x++;
}
void xregparm(3,3,1) MediumDStrS(int x,int y,char *s,int w) {
	MediumDChar(x-1,y,0);
	while (*s)
		MediumDCharS(x,y,*s++),x++,w--;
	while (--w>0)
		MediumDCharS(x,y,' '),x++;
}
void xregparm(3,3,1) MediumDStrC(int x,int y,char *s,int w) {
	while (*s) {
		if (--w<0) return;
		MediumDChar(x,y,*s++),x++;
	}
	while (--w>0)
		MediumDChar(x,y,' '),x++;
}
void xregparm(3,3,1) MediumDStrCS(int x,int y,char *s,int w) {
	MediumDChar(x-1,y,0);
	while (*s) {
		if (--w<0) return;
		MediumDCharS(x,y,*s++),x++;
	}
	while (--w>0)
		MediumDCharS(x,y,' '),x++;
}
void xregparm(2,2,1) VariableDStr(int x,int y,char *s) {
    if (small_font)
	SmallDStr(x,y,s);
    MediumDStr(x,y,s);
}
void xregparm(3,3,1) VariableDStrS(int x,int y,char *s,int w) {
    if (small_font)
	SmallDStrS(x,y,s,w);
    MediumDStrS(x,y,s,w);
}
void xregparm(3,3,1) VariableDStrC(int x,int y,char *s,int w) {
    if (small_font)
	SmallDStrC(x,y,s,w);
    MediumDStrC(x,y,s,w);
}
void xregparm(3,3,1) VariableDStrCS(int x,int y,char *s,int w) {
    if (small_font)
	SmallDStrCS(x,y,s,w);
    MediumDStrCS(x,y,s,w);
}
#endif

void ScrRectFill2(SCR_RECT *scr,int attr) {
	int y=scr->xy.y0,n=scr->xy.y1-y;
	int x0=scr->xy.x0,x1=scr->xy.x1;
	do {
		FastDrawHLine(Port,x0,x1,y,attr); y++;
	} while (n--);
}
void DrawFrame(int x0,int y0,int x1,int y1) {
	FastDrawHLine(Port,x0+1,x1-1,y0,A_NORMAL);
	FastDrawHLine(Port,x0+1,x1-1,y1,A_NORMAL);
	FastDrawLine(Port,x0,y0+1,x0,y1-1,A_NORMAL);
	FastDrawLine(Port,x1,y0+1,x1,y1-1,A_NORMAL);
}

#define B_MOVEAROUND 0
#define B_CENTER 0x8000
int disp_box(int x1,int x2,int height,int attr,WIN_RECT *win) {
	WIN_RECT wbox; SCR_RECT box;
	if (attr&B_ROUNDED) height+=2;
	wbox.x0=virtual_to_physical(x1)-3; wbox.x1=virtual_to_physical(x2)+1;
	if (ycur+6+height+4>=_89_92(100-8,128-8) || (attr&B_CENTER) || ycur>=_89_92((100-8)/2,(128-8)/2)) {
		wbox.y0=ycur-height-4, wbox.y1=ycur-1;
		if (wbox.y0<0 || (attr&B_CENTER))
			wbox.y0=_89_92((100-8)/2-2,(128-8)/2-2)-height/2,wbox.y1=wbox.y0+height+3;
	} else
		wbox.y0=ycur+6, wbox.y1=ycur+5+height+4;
	box.xy.x0=wbox.x0;
	box.xy.x1=wbox.x1;
	box.xy.y0=wbox.y0;
	box.xy.y1=wbox.y1;
	ScrRectFill2(&box,A_REVERSE);
	DrawClipRect(&wbox,ScrRect,A_NORMAL|attr);
	if (win) *win=wbox;
	return box.xy.y0+2;
}

void dialog(char *title,int width,int height,int attr,WIN_RECT *w) {
	WIN_RECT win;
	int h=height,x0=(X-width)/2,x1=x0+width,y0;
	if (title) h+=8;
	x0 = physical_to_virtual(x0);
	x1 = physical_to_virtual(x1-1)+1;
	y0 = disp_box(x0,x1,h,attr,&win);
	x0 = virtual_to_physical(x0);
	x1 = virtual_to_physical(x1);
	if (title) {
		SmallDStr((X/4)/2-strlen(title)/2,y0,title);
		FastDrawHLine(Port,x0-2,x1,y0+6,A_NORMAL);
		y0+=8;
	}
	if (w) w->x0=x0,w->x1=x1,w->y0=y0,w->y1=y0+height; /* I doubt that w->y1 will ever be used, but... */
}

enum {
	W_NORMAL=0,
	W_NOKEY=0x10,
	W_ALLOW_F1=0x10<<1, W_ALLOW_F2=0x10<<2, W_ALLOW_F3=0x10<<3, W_ALLOW_F4=0x10<<4, W_ALLOW_F5=0x10<<5,
};
int SimpleDlg(char *title,char *text,int attr,int wattr);

#define SDLG_BUF_SIZE 128
extern char text_find[SDLG_BUF_SIZE],text_repl[SDLG_BUF_SIZE];
extern int search_attr;
int SearchDlg(int rep);
int DoSearch();
void FindDlg() {
/*	while (!kbhit())
		dialog("Find text",100,40,B_ROUNDED,NULL), LCD_restore(Port);
	ngetchx();*/
	SearchDlg(1);
}

LCD_BUFFER *scr_stk[8],**scr_sptr=0;
#define NOT_MEMORY 2
void PushScr() {
	LCD_BUFFER *p=malloc(LCD_SIZE);
	*scr_sptr++=p;
	if (p) memcpy(p,Port,LCD_SIZE);
}
void PopScr() {
	LCD_BUFFER *p=*--scr_sptr;
	if (p) {
		memcpy(Port,p,LCD_SIZE);
		free(p);
	}
}

/*void TestDisp() {
	void *PortSave=Port;
	Port=LCD_MEM;
	while (!kbhit()) {
		int c='a';
		memset(Port,0,30*92);
		while (c<='z') {
			int y=90;
			do {
				int x=40;
				while (x--) DChar(x,y,c);
			} while ((y-=6)>=0);
			c++;
		}
	}
	Port=PortSave;
}*/

#ifdef _89
long MenuBar[5*8]={
	0x7FFFFFFF,0x7FFFFFFF,0x7FFFFFFF,0x7FFFFFFF,0x7FFFFFFE,
	0x80000000,0x80000000,0x80000000,0x80000000,0x80000001,
	0x90000000,0xB0000000,0xB0000000,0xA0000000,0xB8000001,
	0xB0000000,0x88000000,0x88000000,0xA8000000,0xA0000001,
	0x90000000,0x90000000,0x90000000,0xB8000000,0xB0000001,
	0x90000000,0xA0000000,0x88000000,0x88000000,0x88000001,
	0xB8000000,0xB8000000,0xB0000000,0x88000000,0xB0000001,
	0x80000000,0x80000000,0x80000000,0x80000000,0x80000001,
};
#define NMENU 5
#else
long MenuBar[8*8]={
	0x7FFFFFFF,0x7FFFFFFF,0x7FFFFFFF,0x7FFFFFFF,0x7FFFFFFF,0x7FFFFFFF,0x7FFFFFFE,0,
	0x80000000,0x80000000,0x80000000,0x80000000,0x80000000,0x80000000,0x80000001,0,
	0x90000000,0xB0000000,0xB0000000,0xB0000000,0xB0000000,0xA0000000,0xB8000001,0,
	0xB0000000,0x88000000,0x88000000,0x88000000,0x88000000,0xA8000000,0xA0000001,0,
	0x90000000,0x90000000,0x90000000,0x90000000,0x90000000,0xB8000000,0xB0000001,0,
	0x90000000,0xA0000000,0x88000000,0x88000000,0x88000000,0x88000000,0x88000001,0,
	0xB8000000,0xB8000000,0xB0000000,0xB0000000,0xB0000000,0x88000000,0xB0000001,0,
	0x80000000,0x80000000,0x80000000,0x80000000,0x80000000,0x80000000,0x80000001,0,
};
//#error Adapt 'MenuBar' to the 92+
#define NMENU 7
#endif
void PutLine(void *src,int y) {
	memcpy(Port+30*y,src,30);
}
char **MenuContents=0;
void DrawMenuBar() {
	void *p=MenuBar;
	int y;
	for (y=Y-8;y<Y;y++)
		PutLine(p,y),p+=q89(20,32);
	int i,skip=0;
	char **q=MenuContents;
	for (i=0;i<5;i++)
		if (!*q) {
			for (y=Y-8;y<Y;y++) ((long *)(Port+30*y))[i]=(skip || y==Y-8)?0:0x80000000;
			skip=1; q++;
		} else {
			SmallDStrC((i<<3)+2,Y-6,*q++,8-2/* ==6... */);
			skip=0;
		}
}
