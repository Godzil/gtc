/*
 * GTools C compiler
 * =================
 * source file :
 * assembled binary output
 *
 * Copyright 2001-2004 Paul Froissart.
 * Credits to Christoph van Wuellen and Matthew Brandt.
 * All commercial rights reserved.
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 */

#include	"define.h"
_FILE(__FILE__)
#include	"c.h"
#include	"expr.h"
#include	"gen.h"
#include	"cglbdec.h"

#ifdef PC
void nl(void) {
}
#define nl() while (0)
#endif

#ifdef SECURIZED_ALIGN
#define put_align(__a) while (0)
#else
#define put_align(__a) put_align2()
#endif

#ifdef PC
#define tabs "\t"
#else
#define tabs " "
#endif

#define N_TAB 64
#define N_TAB_LOG 6
typedef struct _ltab {
	struct _ltab *nxt;
	unsigned int t[N_TAB];
} LTAB;
#define N_REF 8
#define N_REF_LOG 3
typedef struct _ref {
	unsigned int r[N_REF];
	struct _ref *nxt;		// very important to be afterwards : if nxt==NULL, then r[N_REF]==0
} REF;
typedef struct _gtab {
	struct _gtab *nxt;
	unsigned int t[N_TAB];
	REF			*r[N_TAB];
} GTAB;

typedef struct _rtype {
	REF *r0,*r;
} RTYPE;
typedef struct _func {
	RTYPE rt;
	unsigned int f;
	struct _func *nxt;
} FUNC;
typedef struct _xtab {
	struct _xtab *nxt;
	RTYPE		*t[N_TAB];
} XTAB;

FILE		   *output CGLOB;
unsigned char  *bin CGLOB;
struct slit    *strtab CGLOB;
long			lc_bss CGLOB;
xstatic int		lc_stk CGLOB,uses_lc_stk CGLOB;
xstatic char		odd CGLOB;			// for put_align
xstatic int		extlabel CGLOB;
int				glblabel CGLOB;
struct ocode   *scope_head CGLOB;
unsigned int	pos CGLOB,bin_size CGLOB;
int				nostub_mode CGLOB;
#ifdef EXE_OUT
int				exestub_mode CGLOB;
#else
#define exestub_mode 0
#endif
LTAB		   *loc_tab CGLOB;
GTAB		   *glb_tab CGLOB;
XTAB		   *ext_tab CGLOB;
LTAB		   *exp_tab CGLOB;	// warning : the content is the exported label (but not the offset)
FUNC		   *rom_funcs CGLOB;
#ifdef RAM_CALLS
FUNC		   *ram_funcs CGLOB;
#endif
RTYPE			reloc_tab CGLOBL,bss_tab CGLOBL;
HTABLE			alsyms CGLOBL;
TABLE			libsyms CGLOBL;	// (FUNC *)sym->value.i

#ifdef SIZE_STATS
xstatic unsigned int c_compiler_sz CGLOB;
#endif

#ifdef MC680X0
/* variable initialization */
enum e_gt {
	nogen, bytegen, wordgen, longgen, floatgen
};
enum e_sg {
	noseg, codeseg, dataseg
};


char		   *outlate();
#ifndef NOFLOAT
#ifdef PC
unsigned long double2ffp();
#else
#define double2ffp(__x) (__x)
#endif
#endif
void wrt2(short x);
void set_label(int lab,unsigned int pos);
unsigned int lab_src(int n);
void put_label(int lab);
void put_align2(void);

enum(e_gt)		gentype;
enum(e_sg)		curseg;
int 			outcol;

#define BIN_STEP 128

void out_init() {
//	fputs("gtc_compiled:\n", output);
	curseg = noseg;
	gentype = nogen;
	outcol = 0;
	lc_stk = 0;
	odd = 0;
	extlabel = -1;
	glblabel = -32768;
	scope_head = 0;
	pos = 4;
	hashinit(&alsyms);
	reloc_tab.r = 0;
	bss_tab.r = 0;
	libsyms.tail = libsyms.head = 0;
	rom_funcs = 0;
#ifdef RAM_CALLS
	ram_funcs = 0;
#endif
	loc_tab = 0;
	glb_tab = 0;
	ext_tab = 0;
	nostub_mode = 0;
#ifdef EXE_OUT
	exestub_mode = 0;
#endif
	bin = malloc(bin_size = BIN_STEP);
	pos = 4;
	put_label(label("gtc_compiled"));
#ifdef SIZE_STATS
	c_compiler_sz=0;
#endif
}

char *unres_name(int lab) {
	SYM *ttail; int crc=N_HASH;;
	while (crc--) {
		ttail=alsyms.h[crc].tail;
		while (ttail) {
			if ((short)ttail->value.splab==(short)lab)
				return ttail->name;
			ttail = ttail->prev;
		}
	}
	return 0;
}
void scan_unresolved() {
	GTAB *t=glb_tab; int l=0x8000;
	while (t) {
		REF **r=t->r; int n;
		n=N_TAB;
		while (n--) {
			if (*r++)
				uerrc2("unresolved symbol '%s'",unres_name(l));
			l++;
		}
		t=t->nxt;
	}
}

int eval_rt(RTYPE *r) {
	int n=1;
	REF *p=r->r0,*q=r->r;
	while (p!=q)
		n+=N_REF,p=p->nxt;
	if (p) {
		unsigned int *a=p->r;
		while (*a++) n++;
	}
	return n;
}
unsigned int eval_functab(FUNC *f) {
	int n=1;
	while (f)
		n+=1+eval_rt(&f->rt),f=f->nxt;
	return n+n;
}
unsigned int eval_libs() {
	int n=2;
	SYM *sp=libsyms.head;
	while (sp)
		n+=10+eval_functab((FUNC *)sp->value.i),sp=sp->next;
//	return 2;
	return n;
}
unsigned int eval_export() {
	int i=N_TAB,n=i;
	int *a;
	if (!exp_tab)
		return 0;
	a=exp_tab->t;
	while (i--)
		if (*a++) n=i;
	return (1+N_TAB-n)<<1;
}

#ifdef PC
xstatic unsigned char *hdr CGLOB,*hdrp CGLOB;
#else
xstatic unsigned short *hdr CGLOB,*hdrp CGLOB;
#endif
xstatic short hshift CGLOB;
#ifdef PC
void _wri(unsigned short x) {
	unsigned char hi=(char)(x>>8),lo=(char)x;
	*hdrp++=hi;
	*hdrp++=lo;
}
void _ovwri(unsigned int i,unsigned short x) {
	unsigned char hi=(char)(x>>8),lo=(char)x;
	if (i&1) fatal("ODD OVWRI");
	hdr[i+0]=hi;
	hdr[i+1]=lo;
}
void _orwri(unsigned int i,unsigned short x) {
	unsigned char hi=(char)(x>>8),lo=(char)x;
	if (i&1) fatal("ODD ORWRI");
	hdr[i+0]|=hi;
	hdr[i+1]|=lo;
}
#define wri(x) (_wri((unsigned short)(x)))
#define ovwri(i,x) (_ovwri(i,(unsigned short)(x)))
#define orwri(i,x) (_orwri(i,(unsigned short)(x)))
#else
#define wri(x) (*hdrp++=x)
#define ovwri(i,x) (*(unsigned short *)((char *)hdr+i)=x)
#define orwri(i,x) (*(unsigned short *)((char *)hdr+i)|=x)
#endif
void write_rt(RTYPE *r) {
	REF *p=r->r0;
	while (p) {
		unsigned int x,*a=p->r; int n;
		if ((p=p->nxt)) {
			n=N_REF; while (n--) wri((*a++)+hshift);
		} else
			while ((x=*a++)) wri(x+hshift);
	}
	wri(0);
}
void write_reloc(
#ifdef OBJ_OUT
				 int directly_to_bin
#else
			#define directly_to_bin 0
#endif
				 ) {
	RTYPE *r=&reloc_tab;
	REF *p=r->r0;
	while (p) {
		unsigned int x,*a=p->r; int n;
		if ((p=p->nxt)) {
			n=N_REF; while (n--) {
				x=*a++;
				directly_to_bin?wrt2(x+hshift+2):wri(x+hshift);
	#ifdef PC
				{ short a=bin[x+2]<<8;
				a+=bin[x+3]+hshift;		/* logically, shouldn't overflow... */
				bin[x+2]=(unsigned char)(a>>8);
				bin[x+3]=(unsigned char)(a);
				}
	#else
				*(long *)(bin+x)+=hshift;
	#endif
			}
		} else
			while ((x=*a++)) {
	#ifdef PC
				{ short a=bin[x+2]<<8;
				a+=bin[x+3]+hshift;		/* logically, shouldn't overflow... */
				bin[x+2]=(unsigned char)(a>>8);
				bin[x+3]=(unsigned char)(a);
				}
	#else
				*(long *)(bin+x)+=hshift;
	#endif
				directly_to_bin?wrt2(x+hshift+2):wri(x+hshift);
			}
	}
	directly_to_bin?wrt2(0):wri(0);
}
#undef directly_to_bin
void write_functab(FUNC *f0) {
	int n=0; FUNC *f=f0;
	while (f)
		n++,f=f->nxt;
	wri(n-1);
	f=f0;
	while (f)
		wri(f->f),write_rt(&f->rt),f=f->nxt;
}
void write_libs() {
	int n=0;
	SYM *sp=libsyms.head;
//	wri(0); return;
	while (sp)
		n++,sp=sp->next;
	wri(n);
	sp=libsyms.head;
	while (sp) {
		memset(hdrp,0,8);
		strcpy((char *)hdrp,sp->name);
		hdrp+=8/sizeof(*hdrp);
		wri(0);
		sp=sp->next;
	}
	sp=libsyms.head;
	while (sp)
		write_functab((FUNC *)sp->value.i),sp=sp->next;
}
unsigned int qlab_src(int n);
void write_export(unsigned int l) {
	int *a=exp_tab->t,x;
	l=(l>>1)-1;
	wri((short)l);
	while (l--) {
		if ((x=*a++))
			x=qlab_src(x)+hshift;
		wri(x);
	}
}

#ifdef EXE_OUT
typedef struct {
	short t;			// type
	unsigned short loc;	// location, offset based on relocated output
} EXE_ITEM;
int
#ifdef PC
#ifdef _MSC_VER
__cdecl
#endif
#else
CALLBACK
#endif
exe_compare(const void *a,const void *b) {
	return ((EXE_ITEM *)a)->loc-((EXE_ITEM *)b)->loc;
}
void *GTPackDo(void *buf,unsigned int *buflen);
#include "out68k_exe.h"
#endif

readonly char pgm_init[0x1A]={0x61,0,0,0,'6','8','k','P'};
readonly char lib_init[0x1A]={0x4E,0x75,0x4E,0x75,'6','8','k','L'};
readonly char pgm_stub[0xA]={0x2F,0x38,0x00,0x34,0x66,0x02,0x50,(char)0x8F,0x4E,0x75};
void wrt2(short x);
void wrt1(char x);
void out_close() {
#ifdef PC
	int calctype_used[]={0,0,0};
	char *calc_symbols[]={"USE_TI89","USE_TI92PLUS","USE_V200"};
	int calc,specific=1,something;
	for (something=0;!something;specific=0)
		for (calc=0;calc<3;calc++)
			if (!specific || search(calc_symbols[calc],-1,&defsyms))
				calctype_used[calc]++,something++;
#endif
	if (bin) {
		/*
		 * 0x1A+(2+10*nLibs+2*(nLibs+nLibCalls+nLibFuncs))+(2+2+2*(nRomFuncs+nRomCalls))
		 * +2+2+(4+nBSS*2)+(2+nExport*2)
		 * +0xA+(nReloc+1)*2
		 *  =
		 * 0x1A+(2+10*nLibs+nLibs(eval(Lib)))+(2+eval(rom))
		 * +2+2+(4+nBSS*2)+(2+nExport*2)
		 * +0xA+eval_rt(reloc_tab)*2
		 *  =
		 * 0x1A+(2+eval_libs())+(2+eval(rom))
		 * +2+2+(4+nBSS*2)+(2+nExport*2)
		 * +0xA+eval_rt(reloc_tab)*2
		 */
		SYM *mainsym;
		unsigned int rom=eval_functab(rom_funcs)-2,lib=eval_export();
#ifdef RAM_CALLS
		unsigned int ram=eval_functab(ram_funcs)-2;
#else
#define ram 0
#endif
#ifdef OBJ_OUT
		int object=!!search("_GENERIC_ARCHIVE",-1,&defsyms);
		if (!object) {
#endif
			int nostub=search("_nostub",-1,&alsyms) || search("_nostub",-1,&gsyms)
				|| search("_nostub",-1,&defsyms) || nostub_mode;
#ifdef EXE_OUT
			int exestub=!!search("EXE_OUT",-1,&defsyms);
			if (exestub) nostub=0;
#else
#define exestub 0
#endif
			debug('+');
			if (nostub) {
				if (ram || rom) fatal("RAM/ROM_CALL in _nostub");
				if (libsyms.head) fatal("Libraries in _nostub");
				if (bss_tab.r) fatal("BSS in _nostub");
				hshift=0;
#ifdef EXE_OUT
			} else if (exestub) {
				if (ram) fatal("RAM_CALL in EXE format");
				if (libsyms.head) fatal("Libraries in exe format");
#endif
			} else {
				hdrp=hdr=malloc((hshift=0x1A-4+(eval_libs())+(rom?rom+4:2)
					+(ram?ram+4:2)+(eval_rt(&reloc_tab)<<1)+(bss_tab.r?4+(eval_rt(&bss_tab)<<1):0)
						+(lib)+(lib?0:0xA))+4);
				if (!hdr)
					fatal("memory");
				memcpy(hdr,lib?lib_init:pgm_init,0x1A);
				hdrp+=0x1A/sizeof(*hdrp);
				write_libs();	// import table
				wri(rom);	// ROM table
				if (rom) write_functab(rom_funcs);
		#ifdef RAM_CALLS
				wri(ram);	// RAM table
				if (ram) write_functab(ram_funcs);
		#else
				wri(0);	// no RAM table
		#endif
		#ifdef OBJ_OUT
				write_reloc(0);	// reloc table
		#else
				write_reloc();	// reloc table
		#endif
				if (bss_tab.r) {
					ovwri(0x14,(char *)hdrp-(char *)hdr);	// BSS table
					wri(lc_bss>>16),wri(lc_bss);
					write_rt(&bss_tab);
				}
				if (lib) {
					ovwri(0x16,(char *)hdrp-(char *)hdr);	// export table
					write_export(lib);
				}
				if (!lib) {
					ovwri(0x02,(char *)hdrp-(char *)hdr-2);	// stub
					memcpy(hdrp,pgm_stub,0xA);
					hdrp+=0xA/sizeof(*hdrp);
				}
		#ifdef PC
				if (hdrp!=hdr+(hshift+4)/sizeof(*hdrp))
					ierr(LINK_MISPL,1);
		#endif
				/* link with file */
				if ((mainsym=search("VERSION",-1,&defsyms))) {

					ovwri(0x10,1<<8);
				}
			}
			mainsym=NULL;
			if (!lib && ((!nostub && !exestub) || !search("NO_MAIN",-1,&defsyms))) {
				if (!(mainsym=search("_main",-1,&alsyms)) && !(mainsym=search("__main",-1,&alsyms)))
					fatal("_main not found");
				if (!nostub && !exestub) {
					ovwri(0x0C,qlab_src(mainsym->value.splab)+hshift);
					if (!!search("NO_SCREEN",-1,&defsyms))
						orwri(0x10,(1<<2));
				}
			}
			scan_unresolved();

			/* finally, write the output to the file */
			if (nostub) {
				if (!mainsym)
					memmove(bin,bin+4,pos-4),pos-=4,hshift=-4;
				else {
			#ifdef PC
					bin[0]=0x60,bin[1]=0x00;
					bin[2]=(unsigned char)((qlab_src(mainsym->value.splab)-2)>>8);
					bin[3]=(unsigned char)((qlab_src(mainsym->value.splab)-2));
			#else
					*(short *)bin=0x6000;
					*(short *)(bin+2)=qlab_src(mainsym->value.splab)-2;
			#endif
					hshift=0;
				}
				put_align2();
				wrt2(0);
			   {
				RTYPE *r=&reloc_tab;
				REF *p=r->r0;
				while (p) {
					unsigned int x,*a=p->r; int n;
					if ((p=p->nxt)) {
						n=N_REF; while (n--) {
				#ifdef PC
							{ short z=bin[(x=*a++)+2+hshift]<<8;
							z+=bin[x+3+hshift]+hshift;
							wrt2(z);
							}
				#else
							wrt2(*(unsigned short *)(bin+(x=*a++)+2+hshift)+hshift);
				#endif
							if (*(unsigned short *)(bin+x+hshift)!=0)
								uerrc("invalid _nostub relocation");
							*(unsigned short *)(bin+x+2+hshift)=0;
							wrt2((unsigned short)(x+hshift));
						}
					} else
						while ((x=*a++)) {
				#ifdef PC
							{ short z=bin[x+2+hshift]<<8;
							z+=bin[x+3+hshift]+hshift;
							wrt2(z);
							}
				#else
							wrt2(*(unsigned short *)(bin+x+2+hshift)+hshift);
				#endif
							if (*(unsigned short *)(bin+x+hshift)!=0)
								uerrc("invalid _nostub relocation");
							*(unsigned short *)(bin+x+2+hshift)=0;
							wrt2((unsigned short)(x+hshift));
						}
				}
			   }
				wrt1((char)0xF3);
				rel_global();
		#ifndef PC
				fwrite(bin,1,pos,output);
		#endif
#ifdef EXE_OUT
			} else if (exestub) {
#ifdef TWIN_COMPILE
				if (twinc_necessary) {
					rel_global();
					goto close_it;	// don't lose time compressing...
				} else {
#endif
			#define EXE_END -1
			#define EXE_PROGRELOC 0
			#define EXE_BSSRELOC 1
			#define EXE_ROMRELOC 2
			#define EXE_ROMRELOC_NOJSR (0x100|EXE_ROMRELOC)
				EXE_ITEM *exe_data=calloc(6000,sizeof(EXE_ITEM));	/* 12 kb */
				EXE_ITEM *exe_ptr=exe_data;
				int exe_num;
				if (!exe_data)
					fatal("Memory");
				memmove(bin,bin+4,pos-4),pos-=4;
			   {
				RTYPE *r=&reloc_tab;
				REF *p=r->r0;
				while (p) {
					unsigned int x,*a=p->r; int n;
					if ((p=p->nxt)) {
						n=N_REF; while (n--) {
							exe_ptr->t=EXE_PROGRELOC;
							exe_ptr->loc=(*a++)-4;
							exe_ptr++;
						}
					} else
						while ((x=*a++)) {
							exe_ptr->t=EXE_PROGRELOC;
							exe_ptr->loc=x-4;
							exe_ptr++;
						}
				}
			   }
			   {
				FUNC *f=rom_funcs;
				while (f) {
					RTYPE *r=&f->rt;
					REF *p=r->r0;
					while (p) {
						unsigned int x,*a=p->r; int n;
						if ((p=p->nxt)) {
							n=N_REF; while (n--) {
								exe_ptr->t=EXE_ROMRELOC;
								exe_ptr->loc=(*a++)-4;
								if (*(long *)(bin+exe_ptr->loc))
									fatal("Invalid ROM_CALL");
								bin[exe_ptr->loc+2]=f->f>>8;
								bin[exe_ptr->loc+3]=f->f&255;
								if (bin[exe_ptr->loc-2]==0x4e
									&& bin[exe_ptr->loc-1]==0xb9)
									exe_ptr->loc-=2;
								else bin[exe_ptr->loc+2]|=0x80,exe_ptr->t=EXE_ROMRELOC_NOJSR;
								exe_ptr++;
							}
						} else
							while ((x=*a++)) {
								exe_ptr->t=EXE_ROMRELOC;
								exe_ptr->loc=x-4;
								if (*(long *)(bin+exe_ptr->loc))
									fatal("Invalid ROM_CALL");
								bin[exe_ptr->loc+2]=f->f>>8;
								bin[exe_ptr->loc+3]=f->f&255;
								if (bin[exe_ptr->loc-2]==0x4e
									&& bin[exe_ptr->loc-1]==0xb9)
									exe_ptr->loc-=2;
								else bin[exe_ptr->loc+2]|=0x80,exe_ptr->t=EXE_ROMRELOC_NOJSR;
								exe_ptr++;
							}
					}
					f=f->nxt;
				}
			   }
				exe_ptr->t=-1; exe_ptr->loc=pos; exe_ptr++;
				exe_num=exe_ptr-exe_data;
				qsort(exe_data,exe_num,sizeof(EXE_ITEM),exe_compare);
			   {
				long relocated_size;
				unsigned short last_pos=0; unsigned char *ptr=bin,*out=bin;
				char *reloc_ptr=(char *)exe_data,*last_esc=NULL;
				int last_esc_num=0;
				int i=exe_num; exe_ptr=exe_data;
				while (i--) {
					unsigned char *dest_ptr=bin+exe_ptr->loc;
					unsigned short delta; short type;
#ifdef PC
#define g16(p) (p+=2,(p[-2]<<8)+p[-1])
#define rd16(p) ((p[0]<<8)+p[1])
#define w16(p,v) (*p++=(v)>>8,*p++=(v)&255,(void)0)
#else
#define g16(p) (*((int *)p)++)
#define rd16(p) (*((int *)p))
#define w16(p,v) (g16(p)=(v))
#endif
					while (ptr<dest_ptr) {
						/*if (ptr==&bin[0xaca])
							bkpt();	// [off_nr$9c] */
						w16(out,rd16(ptr));
#ifndef OLD_LOADER_WITHOUT_BRANCHES
						if (!ptr[1] && (ptr[0]&0xF0)==0x60) {
							//printf(".");
							ptr+=2;
							goto relative_optimize;
						}
#endif
						if ((g16(ptr)&63)==0x3A) {
#ifndef OLD_LOADER_WITHOUT_BRANCHES
						relative_optimize:
#endif
							if (ptr>=dest_ptr) break;
							w16(out,rd16(ptr)+((char *)ptr-(char *)bin));
							ptr+=2;
						}
					}
					delta=(exe_ptr->loc-last_pos)+2;
					last_pos=exe_ptr->loc;
					if ((delta&1))
						uerrc("invalid relocation offset");
					delta>>=1;
					switch ((type=exe_ptr->t)) {	// we *NEED* to save exe_ptr->t as it will be lost
						case EXE_PROGRELOC:
							if (g16(ptr))
								uerrc("invalid _nostub relocation");
							w16(out,rd16(ptr)-4);	// parameters to w16 can't have side-fx...
							ptr+=2;
							last_pos+=4;
							break;
						case EXE_ROMRELOC:
							ptr+=4;
							w16(out,rd16(ptr));	// parameters to w16 can't have side-fx...
							ptr+=2;
							last_pos+=6;
							break;
						case EXE_ROMRELOC_NOJSR:
							ptr+=2;
							w16(out,rd16(ptr));	// parameters to w16 can't have side-fx...
							ptr+=2;
							last_pos+=4;
							break;
					}
					if (delta<128)
						*reloc_ptr++=delta&255;
					else *reloc_ptr++=128|(delta>>8),*reloc_ptr++=delta&255;
					if (!last_esc_num--)
						last_esc=reloc_ptr,*reloc_ptr++=0,last_esc_num=3;
					if (type IS_INVALID) {
						*reloc_ptr++=0;
						break;
					}
					(*last_esc)|=type<<(last_esc_num<<1);
					exe_ptr++;
				}
				rel_global();
				exe_num=reloc_ptr-(char*)exe_data;
				*reloc_ptr=0;							/* (same remark as below...) */
				reloc_ptr=realloc(exe_data,exe_num+1);	/* we don't know if we will need padding... */
				relocated_size=pos;
				pos=out-bin;
				bin=realloc(bin,pos);
#if 0
				{
					unsigned int i;
					for (i=0; i<pos; i++)
						printf("0x%x,\n",(int)bin[i]);
				}
#endif
				bin=GTPackDo(bin,&pos);
				if (((exe_num+pos)&1)) exe_num++;
				{ unsigned short loader_offset=(8-2)+12+pos+exe_num+(bss_tab.r?6:0)+6;
				char header[8];
				char exe_hdr[12]={0,1,0,0,0,0,0,0,0,0,0,0};
				if (loader_offset<=32767) {
					header[0]=0x60,header[1]=0x00;
					header[2]=loader_offset>>8,header[3]=loader_offset&255;
					*(long *)(header+4)=0;
				} else {
			#ifdef PC
					const unsigned char header32[8]=
			#else
					memcpy(header,(char[])
			#endif
						{0x41,0xFA,0x7F,0xF2,0x4E,0xE8,0x80,0x0C}
			#ifdef PC
					; memcpy(header,header32, 8);
			#else
						, 8);
			#endif
					loader_offset+=(0x800C-2);
					header[6]=loader_offset>>8,header[7]=loader_offset&255;
					loader_offset-=(0x800C-2);	// restore it for future use
				}
				bin=realloc(bin,8+12+pos+exe_num+(bss_tab.r?exeloadersize_bss:exeloadersize_nobss));
				if (!bin)
					fatal("Memory");
				memmove(bin+8+12,bin,pos);
				memcpy(bin,header,8);
				exe_hdr[2]=pos>>8,exe_hdr[3]=pos&255;
				exe_hdr[5]=(char)((relocated_size>>16)&255),
					exe_hdr[6]=(char)((relocated_size>>8)&255),
					exe_hdr[7]=(char)(relocated_size&255);
				if (bss_tab.r)
					fatal("BSS in exes are not implemented");
				memcpy(bin+8,exe_hdr,12);
				memcpy(bin+8+12+pos,reloc_ptr,exe_num);
				free(reloc_ptr);
				memcpy(bin+8+12+pos+exe_num,bss_tab.r?exeloader_bss:exeloader_nobss,
					bss_tab.r?exeloadersize_bss:exeloadersize_nobss);
#ifdef PC
				if (verbose) {
					msg2("RAM required during execution   : %5u bytes\n",(unsigned int)relocated_size);
					msg2("Memory required for EXE program : %5u bytes\n",8+12+pos+exe_num);
					msg2("Memory required for EXE loader  : %5u bytes\n",bss_tab.r?exeloadersize_bss:exeloadersize_nobss);
				}
#endif
				pos=8+12+pos+exe_num+(bss_tab.r?exeloadersize_bss:exeloadersize_nobss);
				bin[pos-3]=(loader_offset+22+2)>>8,bin[pos-2]=(loader_offset+22+2)&255;
#ifdef PC
				if (verbose)
					msg2("Total program size              : %5u bytes\n",pos);
#endif
				}
			   }
		#ifndef PC
				fwrite(bin,1,pos,output);
		#endif
#ifdef TWIN_COMPILE
				}
#endif
#endif
			} else {
				rel_global();
				put_align2();
				wrt2(0);
				wrt1((char)0xF3);
		#ifdef PC
				if (!(bin=realloc(bin,bin_size+=hshift)))
					fatal("Memory");
				memmove(bin+hshift+4,bin+4,pos-4);
				memcpy(bin,hdr,hshift+4);
				pos+=hshift;
		#else
				fwrite(hdr,1,hshift+4,output);
				fwrite(bin+4,1,pos-4,output);
		#endif
				free(hdr);
			}
#ifdef OBJ_OUT
		} else {
			unsigned int len;
			int j;
			if (pos&1)
				wrt1(0);
			len=pos-4;

			/* reloc table */
			hshift=-4;
			write_reloc(1);

			/* export table */
			j=N_HASH;
			while (j--) {
				SYM *sp = gsyms.h[j].head;
				while (sp) {
					if ((sp->storage_class == sc_external && sp->used) ||
						(sp->storage_class == sc_global && sp->value.splab)) {
#ifdef PC
						if (!lab_src(sp->value.splab))
							fatal("UNDEF LABEL");	// should this happen ?
#endif
						wrt2(lab_src(sp->value.splab)-2/*fix offset for object format*/);
					}
					sp = sp->next;
				}
			}
			wrt2(0);
			j=N_HASH;
			while (j--) {
				SYM *sp = gsyms.h[j].head;
				while (sp) {
					if ((sp->storage_class == sc_external && sp->used) ||
						(sp->storage_class == sc_global && sp->value.i != -1)) {
						char *s=sp->name;
						do
							wrt1(*s);
						while (*s++);
					}
					sp = sp->next;
				}
			}
			if (pos&1)
				wrt1(0);

			/* import table */
			{
			GTAB *t=glb_tab;
			while (t) {
				REF **r=t->r; int m=N_TAB;
				while (m--) {
					REF *q;
					if ((q=*r++)) {
						while (q) {
							unsigned int p,*a=q->r;
							int n=N_REF;
							while (n--) {
								if (!(p=*a++))
									goto ref_done;
								if (p&1)
									fatal("Negative reloc in object output");
								// warning, this is a 16-bit reloc, but we have to store it as a 32-bit one
						#define offs_convert(p) (p-2/*since 16->32 bit*/-4/*convert bin[] offsets to final object offsets*/)
								if (!offs_convert(p))
									fatal("Word reloc at beginning");
								wrt2(offs_convert(p));
						#undef  offs_convert
							}
							q=q->nxt;
						}
					ref_done:
						wrt2(0);
					}
				}
				t=t->nxt;
			}
			}
			wrt2(0);
			{
			GTAB *t=glb_tab; int l=0x8000;
			while (t) {
				REF **r=t->r; int m=N_TAB;
				while (m--) {
					if (*r++) {
						char *s=unres_name(l);
						do
							wrt1(*s);
						while (*s++);
					}
					l++;
				}
				t=t->nxt;
			}
			}
			if (pos&1)
				wrt1(0);
			wrt2(0);	// leave space for future BSS extension
			hdr=bin; // hack pawa =)
			ovwri(0x0,pos-4);
			ovwri(0x2,len+2);
		#ifndef PC
			wrt1(0);
			wrt1('E');
			wrt1('X');
			wrt1('T');
			wrt1(0);
			wrt1(0xF8);
			fwrite(bin,1,pos,output);
		#endif
		}
#endif
	#ifdef PC
		{
#ifdef OBJ_OUT
			if (!object) {
#endif
				char *extensions[]={".89z",".9xz",".v2z"};
				for (calc=0;calc<3;calc++) {
					if (calctype_used[calc]) {
						unsigned int tifilelen;
						extern char *calcfolder;
						char *tifile=create_ti_file(calc,0x21,calcname,calcfolder,bin,pos,&tifilelen);
						char *buffer; FILE *fp;
						buffer=alloca(strlen(outputname)+strlen(extensions[calc])+1);
						strcpy(buffer,outputname);
						strcat(buffer,extensions[calc]);
						if (!(fp=fopen(buffer,"wb")))
							fatal("Could not open output file.");
						fwrite(tifile,1,tifilelen,fp);
						free(tifile);
						fclose(fp);
					}
				}
#ifdef OBJ_OUT
			} else {
				char buffer[100]; FILE *fp;
				strcpy(buffer,calcname);
				strcat(buffer,".ext");
				if (!(fp=fopen(buffer,"wb")))
					fatal("Could not open output file.");
				fwrite(bin,1,pos,fp);
				fclose(fp);
			}
#endif
		}
	#endif
	#ifdef TWIN_COMPILE
	close_it:
	#endif
		free(bin);
		bin=0;
	}
}

void bin_chk(unsigned int pos) {
	while (bin_size<pos)
		if (!(bin=realloc(bin,bin_size+=BIN_STEP)))
			fatal("Memory");
}

void lab_set(int n,unsigned int v) {
	int i=(n&0x7FFF)>>N_TAB_LOG;
	LTAB *p=local(n)?loc_tab:(LTAB *)glb_tab,*o;
	debugf("lab_set(%x,%x)\n",n,v);
	global_flag+=n&0x8000;
	if (!p) {
		if (local(n))
			loc_tab=p=(LTAB *)xalloc(sizeof(LTAB),_TAB);
		else glb_tab=(GTAB *)(p=(LTAB *)xalloc(sizeof(GTAB),_TAB));
	}
	while (i--)
		if (!(p=(o=p)->nxt))
			o->nxt=p=(LTAB *)xalloc(local(n)?sizeof(LTAB):sizeof(GTAB),_TAB);
	global_flag&=0x7FFF;
#ifndef PC
	n&=N_TAB-1;
	p->t[n]=v;
#else
	i=n&(N_TAB-1);
	p->t[i]=v;
#endif
}

#ifdef PC
/* that's what we use if we want the storage to be persistent, even if we call it from a
 * temp_mem location
 */
void *xallocnt(int s,int m) {
	void *p;
	temp_local++;
	p=xalloc(s,m);
	temp_local--;
	return p;
}
/* xallocnt in global mode */
void *xallocntg(int s,int m) {
	void *p;
	global_flag++;
	p=xallocnt(s,m);
	global_flag--;
	return p;
}
#else
#define xallocnt(s,m) _xallocnt(s)
void *_xallocnt(int s) {
	void *p;
	temp_local++;
	p=xalloc(s,0);
	temp_local--;
	return p;
}
#define xallocntg(s,m) _xallocnt(s)
void *_xallocntg(int s) {
	void *p;
	global_flag++;
	p=xallocnt(s,0);
	global_flag--;
	return p;
}
#endif
unsigned int lab_src(int n) {
	int i=(n&0x7FFF)>>N_TAB_LOG;
	LTAB *p=local(n)?loc_tab:(LTAB *)glb_tab,*o;
	debugfq("lab_src(%x)",n);
	global_flag+=n&0x8000;
	if (!p) {
		if (local(n))
			loc_tab=p=(LTAB *)xallocnt(sizeof(LTAB),_TAB);
		else glb_tab=(GTAB *)(p=(LTAB *)xallocntg(sizeof(GTAB),_TAB));
	}
	while (i--)
		if (!(p=(o=p)->nxt))
			o->nxt=p=(LTAB *)(local(n)?xallocnt(sizeof(LTAB),_TAB):xallocntg(sizeof(GTAB),_TAB));
	global_flag&=0x7FFF;
	n&=N_TAB-1;
	debugf("=%x\n",p->t[n]);
	return p->t[n];
}

RTYPE **xt_find(int n) {
	int i=(n=(int)((short)~n))>>N_TAB_LOG;
	XTAB *p=ext_tab,*o;
	global_flag++;
	if (!p)
		ext_tab=p=(XTAB *)xalloc(sizeof(XTAB),_TAB);
	while (i--)
		if (!(p=(o=p)->nxt))
			o->nxt=p=(XTAB *)xalloc(sizeof(XTAB),_TAB);
	global_flag--;
	n&=N_TAB-1;
	return &p->t[n];
}

int *exp_find(int n) {
	int i=n>>N_TAB_LOG;
	LTAB *p=exp_tab,*o;
	global_flag++;
	if (!p)
		exp_tab=p=(LTAB *)xalloc(sizeof(LTAB),_TAB);
	while (i--)
		if (!(p=(o=p)->nxt))
			o->nxt=p=(LTAB *)xalloc(sizeof(LTAB),_TAB);
	global_flag--;
	n&=N_TAB-1;
	return (int *)&p->t[n];
}

/*void loc_clear() {
	LTAB *p=loc_tab,*o;
	while (p) {
		memset(p->t,0,sizeof(p->t));
		p=p->nxt;
	}
}*/

unsigned int qlab_src(int n) {
	int i=(n&0x7FFF)>>N_TAB_LOG;
	LTAB *p=local(n)?loc_tab:(LTAB *)glb_tab;
	n&=N_TAB-1;
	while (i--) p=p->nxt;
	return p->t[n];
}

REF *ref_add(REF *r,unsigned int v) {
	unsigned int *a;
	debugf("ref_add(%lx,%x)\n",r,v);
	a=r->r;
	while (*a++);
	if (a>(unsigned int *)&r->nxt) {
		global_flag++;
		(r=r->nxt=(REF *)xalloc(sizeof(REF),_REF+REF_ADD))->r[0]=v;
		global_flag--;
	} else a[-1]=v;
	return r;
}

REF **glb_ref(int n) {
	int i=(n&0x7FFF)>>N_TAB_LOG;
	GTAB *p=glb_tab;
	n&=N_TAB-1;
	while (i--)
		p=p->nxt;
	return &p->r[n];
}

void lab_add_ref(int n,unsigned int r) {
	int i=(n&0x7FFF)>>N_TAB_LOG;
	GTAB *p=glb_tab; REF **ro;
	debugf("lab_add_r(%x,%x)\n",n,r);
#ifndef PC
//	asm("0: jbra 0b");
#endif
	if (local(n))
		uerrc2("local label not found: '%s'",unres_name(n));
	global_flag++;
	n&=N_TAB-1;
	while (i--)
		p=p->nxt;
	ro=&p->r[n];
	if (!*ro)
		*ro=(REF *)xalloc(sizeof(REF),_REF+LAB_ADD_REF);
	while ((*ro)->nxt)
		ro=&(*ro)->nxt;
	global_flag--;
	ref_add(*ro,r);
}
void rt_add_ref(RTYPE *rt,unsigned int r) {	// effectively appends reloc to reloc table
	debugf("rt_add_r(%lx,%x)\n",rt,r);
	if (!rt->r) {
		global_flag++;
		rt->r0=rt->r=(REF *)xalloc(sizeof(REF),_REF+RT_ADD_REF);
		global_flag--;
	}
	rt->r=ref_add(rt->r,r);
}
FUNC *func_search(FUNC **f0,unsigned int f) {
	while (*f0)
		if ((*f0)->f==f) break;
		else f0=&(*f0)->nxt;
	if (*f0) return *f0;
	global_flag++;
	*f0=(FUNC *)xalloc(sizeof(FUNC),_RT);
	global_flag--;
	(*f0)->f=f;
	return *f0;
}

typedef struct gv_ret {
	long i;
	int ir,xr,tr;	// Internal/eXternal/Total Relocation count
} GV;
int get_value(struct enode *ep,GV *r,unsigned int rp,int m) {
	int lab; unsigned int p;
	r->ir=0; r->xr=0;
  gv_restart:
	switch (ep->nodetype) {
	  case en_autocon:
	  case en_icon:
#ifndef NOFLOAT
#ifndef PC
	  case en_fcon:
#endif
#endif
		r->i=ep->v.i;
		r->tr=0;
		break;
#ifndef NOFLOAT
#ifdef PC
#ifndef BCDFLT
	  case en_fcon:
		r->i=double2ffp(ep->v.f);
		r->tr=0;
		break;
#endif
#endif
#endif
	  case en_labcon:
	  case en_nacon:
		lab=ep->v.enlab;
		r->tr=1;
		if (global(lab)) {
			r->ir=1;
			if (!(p=lab_src(lab))) {
				if (rp)
					lab_add_ref(lab,(rp+2)+m);
				r->i=0;
				return 1;
			} else
				r->i=p;
		} else {
			if (m) uerrc("negative external reference");
			r->i=0,r->xr=1;
			if (rp) {
				RTYPE *rt=*xt_find(lab);
				if (((long)rt)&0x80000000) {	// DIRTY, but no hidden bug possible (cf xalloc)
					r->i=((long)rt)&0x7FFFFFFF;		// BSS ref
					rt_add_ref(&bss_tab,rp);
				} else rt_add_ref(rt,rp);			// lib/ROM ref
			}
			return 1;
		}
		break;
	  case en_add:
	  case en_sub:
		{
			GV a,b;
			int res=get_value(ep->v.p[0],&a,rp,m);
			res|=get_value(ep->v.p[1],&b,rp,ep->nodetype==en_sub?1-m:m);
			r->xr=a.xr+b.xr;
			r->tr=a.tr+b.tr;
			if (ep->nodetype==en_sub) {
				if (b.xr)
					uerrc("negative external reference");
				r->i=a.i-b.i,r->ir=a.ir-b.ir;
			} else
				r->i=a.i+b.i,r->ir=a.ir+b.ir;
			return res;
		}
		break;
	  case en_uminus:
		m=1-m;
		ep=ep->v.p[0];
		goto gv_restart;
	  default:
		uerrc("invalid expression");
	}
	return 0;
}
int involved_lab(struct enode *ep) {
  gv_restart:
	switch (ep->nodetype) {
	  case en_autocon:
	  case en_icon:
#ifndef NOFLOAT
#ifdef PC
	  case en_fcon:
#endif
#endif
		return -1;
	  case en_labcon:
	  case en_nacon:
		return ep->v.enlab;
	  case en_add:
	  case en_sub:
		return max(involved_lab(ep->v.p[0]),involved_lab(ep->v.p[1]));
	  case en_uminus:
		ep=ep->v.p[0];
		goto gv_restart;
	  default:
		uerrc("invalid expression");
	}
	return -1;
}

xstatic int rel CGLOB, is_long CGLOB;
long get_long(struct enode *ep,unsigned int rp) {
	GV v;
	get_value(ep,&v,rp,0);
	if (v.ir) {
		rt_add_ref(&reloc_tab,rp);
		if (v.ir!=1)
			uerr(ERRA_INVALIDREL);
	}
	rel=v.ir+v.xr;
	return v.i;
}
short get_pcword(struct enode *ep,unsigned int rp) {
	GV v;
	int no_check=get_value(ep,&v,rp-2,0);
	if (v.ir!=1 || v.xr)
		uerr(ERRA_INVALIDREL);
	if (!no_check) {
		v.i -= rp;
		if (v.i>32767L || v.i<-32768L)
#ifdef TWIN_COMPILE
		{
			if (!twinc_necessary) uwarn("twin compilation required");
			twinc_necessary=1;
			*twinc_info++=involved_lab(ep);
			*twinc_info++=rp;
		}
#else
			uerrc("PC-relative mode not allowed here");
#endif
//		v.i += rp;
		return (short)v.i;
	} else
		return (short)v.i-(short)rp;
}
short get_word(struct enode *ep,unsigned int rp,int sign) {
	GV v;
	int no_check=get_value(ep,&v,rp-2,0);
	if (v.ir || v.xr)
		uerr(ERRA_INVALIDREL);
	/* otherwise this would bug with myval&0xFFFF7FFF which is converted to and.w */
	if (!no_check && (v.i>(sign?32767L:65535L) || v.i<(sign?-32768L:-65535L)))
//	if (!no_check && (v.i>(sign?32767L:65535L) || v.i<-32768L))
		uerrc("value can't fit in a word");
	return (short)v.i;
}
long get_offs(struct enode *ep,unsigned int rp) {
	GV v; int l=0;
	get_value(ep,&v,0,0);
	if (v.ir || v.xr)
		l=1;
	if (v.i>32767L || v.i<-32768L)
		l=1;
	if ((is_long=l))
		return get_long(ep,rp);
	else return get_word(ep,rp,1);
}
unsigned char get_byte(struct enode *ep) {
	GV v;
	get_value(ep,&v,0,0);
	if (v.tr)
		uerr(ERRA_INVALIDREL);
	if ((unsigned long)(((unsigned long)v.i)+128)>255)
		uerrc("value can't fit in a byte");
	return (unsigned char)v.i;
}
unsigned short get_quick(struct enode *ep) {
	GV v;
	get_value(ep,&v,0,0);
	if (v.tr)
		uerr(ERRA_INVALIDREL);
	if ((unsigned long)(((unsigned long)v.i)-1)>=8)
		uerrc("value can't fit in a quick constant");
	if (v.i==8)
		v.i=0;
	return (unsigned short)v.i;
}

enum {
	oi_move=1,
	oi_moveq=oi_move+8,
	oi_clr=oi_moveq+1,
	oi_lea=oi_clr+1,
	oi_add=oi_lea+1,
	oi_addi=oi_add+4,
	oi_addq=oi_addi+1,
	oi_sub=oi_addq+1,
	oi_subi=oi_sub+4,
	oi_subq=oi_subi+1,
	oi_muls=oi_subq+1,
	oi_mulu=oi_muls+1,
	oi_divs=oi_mulu+1,
	oi_divu=oi_divs+1,
	oi_and=oi_divu+1,
	oi_andi=oi_and+3,
	oi_or=oi_andi+1,
	oi_ori=oi_or+3,
	oi_eor=oi_ori+1,
	oi_eori=oi_eor+2,
	oi_lsl=oi_eori+1,
	oi_lsr=oi_lsl+3,
	oi_jmp=oi_lsr+3,
	oi_jsr=oi_jmp+1,
	oi_movem=oi_jsr+1,
	oi_rts=oi_movem+3,
	oi_bra=oi_rts+1,
	oi_bsr=oi_bra+1,
	oi_beq=oi_bsr+1,
	oi_bne=oi_beq+1,
	oi_bhs=oi_bne+1,
	oi_bge=oi_bhs+1,
	oi_bhi=oi_bge+1,
	oi_bgt=oi_bhi+1,
	oi_bls=oi_bgt+1,
	oi_ble=oi_bls+1,
	oi_blo=oi_ble+1,
	oi_blt=oi_blo+1,
	oi_tst=oi_blt+1,
	oi_ext=oi_tst+1,
	oi_swap=oi_ext+1,
	oi_neg=oi_swap+1,
	oi_not=oi_neg+1,
	oi_cmp=oi_not+1,
	oi_link=oi_cmp+5,
	oi_unlk=oi_link+1,
	oi_label=oi_unlk+1,
	oi_pea=oi_label+0,
	oi_cmpi=oi_pea+1,
	oi_dbra=oi_cmpi+1,
	oi_asr=oi_dbra+1,
	oi_bset=oi_asr+3,
	oi_bclr=oi_bset+4,
	oi_bchg=oi_bclr+4,
	_oi_asm=oi_bchg+4,
	_oi_adj=_oi_asm+0,
#ifndef ASM
	__oi_end=_oi_adj+0
#endif
#ifdef ASM
	/* ASM-only */
	oi_asl=_oi_adj+0,
	oi_rol=oi_asl+3,
	oi_ror=oi_rol+3,
	oi_roxl=oi_ror+3,
	oi_roxr=oi_roxl+3,
	oi_btst=oi_roxr+3,
	oi_exg=oi_btst+4,
	oi_dc=oi_exg+4,
	oi_ds=oi_dc+1,
	oi_dcb=oi_ds+1,
	oi_bvs=oi_dcb+1,
	oi_bvc=oi_bvs+1,
	oi_bpl=oi_bvc+1,
	oi_bmi=oi_bpl+1,
	oi_trap=oi_bmi+1,
	oi_negx=oi_trap+1,
	oi_addx=oi_negx+1,
	oi_subx=oi_addx+2,
	oi_chk=oi_subx+2,
	oi_even=oi_chk+1,
	oi_dbeq=oi_even+1,
	oi_dbne=oi_dbeq+1,
#ifndef LIGHT_DBXX_AND_SXX
	oi_dbhs=oi_dbne+1,
	oi_dbge=oi_dbhs+1,
	oi_dbhi=oi_dbge+1,
	oi_dbgt=oi_dbhi+1,
	oi_dbls=oi_dbgt+1,
	oi_dble=oi_dbls+1,
	oi_dblo=oi_dble+1,
	oi_dblt=oi_dblo+1,
	oi_dbvs=oi_dblt+1,
	oi_dbvc=oi_dbvs+1,
	oi_dbpl=oi_dbvc+1,
	oi_dbmi=oi_dbpl+1,
	oi_st=oi_dbmi+1,
	oi_sf=oi_st+1,
	oi_seq=oi_sf+1,
	oi_sne=oi_seq+1,
	oi_shs=oi_sne+1,
	oi_sge=oi_shs+1,
	oi_shi=oi_sge+1,
	oi_sgt=oi_shi+1,
	oi_sls=oi_sgt+1,
	oi_sle=oi_sls+1,
	oi_slo=oi_sle+1,
	oi_slt=oi_slo+1,
	oi_svs=oi_slt+1,
	oi_svc=oi_svs+1,
	oi_spl=oi_svc+1,
	oi_smi=oi_spl+1,
	oi_tas=oi_smi+1,
	__oi_end=oi_tas+1
#else
	__oi_end=oi_dbne+1
#endif
#endif
};

readonly unsigned char opi[_op_max+2]={
	oi_move, oi_moveq, oi_clr, oi_lea, oi_add, oi_addi, oi_addq, oi_sub, oi_subi, oi_subq,
	oi_muls, oi_mulu, oi_divs, oi_divu, oi_and, oi_andi, oi_or, oi_ori, oi_eor, oi_eori,
	oi_lsl, oi_lsr, oi_jmp, oi_jsr, oi_movem,
	oi_rts, oi_bra, oi_bsr, oi_beq, oi_bne, oi_bhs, oi_bge, oi_bhi,
	oi_bgt, oi_bls, oi_ble, oi_blo, oi_blt, oi_tst, oi_ext, oi_swap, oi_neg, oi_not, oi_cmp,
	oi_link, oi_unlk, oi_label, oi_pea, oi_cmpi, oi_dbra, oi_asr,
	oi_bset, oi_bclr, oi_bchg, _oi_asm, _oi_adj,
#ifdef ASM
	/* ASM-only */
	oi_asl, oi_rol, oi_ror, oi_roxl, oi_roxr, oi_btst, oi_exg, oi_dc, oi_ds, oi_dcb,
	oi_bvs, oi_bvc, oi_bpl, oi_bmi, oi_trap, oi_negx, oi_addx, oi_subx, oi_chk, oi_even,
	oi_dbeq, oi_dbne,
#ifndef LIGHT_DBXX_AND_SXX
	oi_dbhs, oi_dbge, oi_dbhi, oi_dbgt, oi_dbls, oi_dble, oi_dblo, oi_dblt,/* unsigned, signed */
	oi_dbvs, oi_dbvc, oi_dbpl, oi_dbmi,
	oi_st,  oi_sf,  oi_seq, oi_sne,
	oi_shs, oi_sge, oi_shi, oi_sgt, oi_sls, oi_sle, oi_slo, oi_slt,	/* unsigned, signed */
	oi_svs, oi_svc, oi_spl, oi_smi, oi_tas,
#endif
#endif
	__oi_end
};

typedef struct _opcode {
	char impl,nargs,model,lenf;
	unsigned short bo;
	int a1,a2;
} OPCODE;
enum {
	M_BASIC, M_BASICSWAPPED, M_BRANCH, M_DC, M_ALIGN
};
enum { L_0=1<<0, L_1=1<<1, L_2=1<<2, L_4=1<<4 };
enum {
	A_DREG=0x1,
	A_AREG=0x2,
	A_MEM0=0x4,
	A_MEM=0x84,
	A_PCR=0x8,
	A_IMM=0x10,
	A_MASK1=0x20,
	A_MASK2=0x40,
	A_ADEC=0x80,
	D_RAW2=0x0100,	// |= %00000000 00000000 +WORD
	D_RAW=0x0200,	// |= %00000000 00000000 (+DAT)
	D_LOW=0x0400,	// |= %00000000 dddddddd
	D_EA2=0x0800,	// |= %0000nnnm mm000000 (+DAT)
	D_SZ3=0x3000,	// |= %00000000 0S000000 (argument-independent)
	D_SZ2=0x1000,	// |= %0000000S 00000000 (argument-independent)
	D_SZ=0x2000,	// |= %00000000 ss000000 (argument-independent)
	D_Q=0x4000,		// |= %0000qqq0 00000000
	D_EA=0x8000,	// |= %00000000 00mmmnnn (+DAT)
};
#define A_CCR A_MASK1
#define A_SR A_MASK2
#define A_USP A_MASK1
/* we have to implement A_CCR and A_SR like that to avoid the use
 * of a longword for flags... */
#define A_SRC A_DREG|A_AREG|A_MEM|A_PCR|A_IMM
#define A_SRC2 A_DREG|A_MEM|A_PCR|A_IMM
#define A_DST A_DREG|A_AREG|A_MEM
#define A_DST2 A_DREG|A_MEM

// for each enum(e_op) value :
readonly OPCODE ops[]={
	// op_label
	{0},
	// op_move
	{1,2,M_BASIC,L_2,0x3000,A_SRC|D_EA,A_DST|D_EA2},	// movea is in fact the same instruction
	{1,2,M_BASIC,L_4,0x2000,A_SRC|D_EA,A_DST|D_EA2},
	{1,2,M_BASIC,L_1,0x1000,A_SRC2|D_EA,A_DST2|D_EA2}, // move.b does not accept aregs
	{1,2,M_BASIC,L_2,0x44C0,A_SRC2|D_EA,A_CCR},
	{1,2,M_BASIC,L_2,0x46C0,A_SRC2|D_EA,A_SR},
	{1,2,M_BASIC,L_2,0x40C0,A_SR,A_DST2|D_EA},
	{1,2,M_BASIC,L_4,0x4E60,A_AREG|D_LOW,A_USP},
	{1,2,M_BASIC,L_4,0x4E68,A_USP,A_AREG|D_LOW},
	// op_moveq
	{1,2,M_BASIC,L_0+L_4,0x7000,A_IMM|D_LOW,A_DREG|D_Q},
	// op_clr
	{1,1,M_BASIC,L_1+L_2+L_4,0x4200,A_DST2|D_EA|D_SZ},
	// op_lea
	{1,2,M_BASIC,L_0,0x41C0,A_MEM|A_PCR|D_EA,A_AREG|D_Q},
	// op_add
	{1,2,M_BASIC,L_1+L_2+L_4,0xD000,A_SRC|D_EA|D_SZ,A_DREG|D_Q},
	{1,2,M_BASIC,L_1+L_2+L_4,0xD100,A_DREG|D_Q|D_SZ,A_MEM|D_EA},
	{1,2,M_BASIC,    L_2+L_4,0xD0C0,A_SRC|D_EA|D_SZ2,A_AREG|D_Q},
	{1,2,M_BASIC,L_1+L_2+L_4,0x0600,A_IMM|D_RAW|D_SZ,A_DST2|D_EA},
	// op_addi
	{1,2,M_BASIC,L_1+L_2+L_4,0x0600,A_IMM|D_RAW|D_SZ,A_DST2|D_EA},
	// op_addq
	{1,2,M_BASIC,L_1+L_2+L_4,0x5000,A_IMM|D_Q|D_SZ,A_DST|D_EA},
	// op_sub
	{1,2,M_BASIC,L_1+L_2+L_4,0x9000,A_SRC|D_EA|D_SZ,A_DREG|D_Q},
	{1,2,M_BASIC,L_1+L_2+L_4,0x9100,A_DREG|D_Q|D_SZ,A_MEM|D_EA},
	{1,2,M_BASIC,    L_2+L_4,0x90C0,A_SRC|D_EA|D_SZ2,A_AREG|D_Q},
	{1,2,M_BASIC,L_1+L_2+L_4,0x0400,A_IMM|D_RAW|D_SZ,A_DST2|D_EA},
	// op_subi
	{1,2,M_BASIC,L_1+L_2+L_4,0x0400,A_IMM|D_RAW|D_SZ,A_DST2|D_EA},
	// op_subq
	{1,2,M_BASIC,L_1+L_2+L_4,0x5100,A_IMM|D_Q|D_SZ,A_DST|D_EA},
	// op_muls
	{1,2,M_BASIC,L_2,0xC1C0,A_SRC2|D_EA,A_DREG|D_Q},
	// op_mulu
	{1,2,M_BASIC,L_2,0xC0C0,A_SRC2|D_EA,A_DREG|D_Q},
	// op_divs
	{1,2,M_BASIC,L_2,0x81C0,A_SRC2|D_EA,A_DREG|D_Q},
	// op_divu
	{1,2,M_BASIC,L_2,0x80C0,A_SRC2|D_EA,A_DREG|D_Q},
	// op_and
	{1,2,M_BASIC,L_1+L_2+L_4,0xC000,A_SRC2|D_EA|D_SZ,A_DREG|D_Q},
	{1,2,M_BASIC,L_1+L_2+L_4,0xC100,A_DREG|D_Q|D_SZ,A_MEM|D_EA},
	{1,2,M_BASIC,L_1+L_2+L_4,0x0200,A_IMM|D_RAW|D_SZ,A_DST2|D_EA},
	// op_andi
	{1,2,M_BASIC,L_1+L_2+L_4,0x0200,A_IMM|D_RAW|D_SZ,A_DST2|D_EA},
	// op_or
	{1,2,M_BASIC,L_1+L_2+L_4,0x8000,A_SRC2|D_EA|D_SZ,A_DREG|D_Q},
	{1,2,M_BASIC,L_1+L_2+L_4,0x8100,A_DREG|D_Q|D_SZ,A_MEM|D_EA},
	{1,2,M_BASIC,L_1+L_2+L_4,0x0000,A_IMM|D_RAW|D_SZ,A_DST2|D_EA},
	// op_ori
	{1,2,M_BASIC,L_1+L_2+L_4,0x0000,A_IMM|D_RAW|D_SZ,A_DST2|D_EA},
	// op_eor
	{1,2,M_BASIC,L_1+L_2+L_4,0xB100,A_DREG|D_Q|D_SZ,A_DST2|D_EA},
	{1,2,M_BASIC,L_1+L_2+L_4,0x0A00,A_IMM|D_RAW|D_SZ,A_DST2|D_EA},
	// op_eori
	{1,2,M_BASIC,L_1+L_2+L_4,0x0A00,A_IMM|D_RAW|D_SZ,A_DST2|D_EA},
	// op_lsl
	{1,2,M_BASIC,L_1+L_2+L_4,0xE108,A_IMM|D_Q|D_SZ,A_DREG|D_EA},
	{1,2,M_BASIC,L_1+L_2+L_4,0xE128,A_DREG|D_Q|D_SZ,A_DREG|D_EA},
	{1,1,M_BASIC,L_2,0xE3C0,A_MEM|D_EA},
	// op_lsr
	{1,2,M_BASIC,L_1+L_2+L_4,0xE008,A_IMM|D_Q|D_SZ,A_DREG|D_EA},
	{1,2,M_BASIC,L_1+L_2+L_4,0xE028,A_DREG|D_Q|D_SZ,A_DREG|D_EA},
	{1,1,M_BASIC,L_2,0xE2C0,A_MEM|D_EA},
	// op_jmp
	{1,1,M_BASIC,L_0,0x4EC0,A_MEM|A_PCR|D_EA},
	// op_jsr
	{1,1,M_BASIC,L_0,0x4E80,A_MEM|A_PCR|D_EA},
	// op_movem - contains a little hack : default size is .l, so we separe L_4 from L_2.
	//   Uses M_BASICSWAPPED because when 'uses_link' is set, the link reg needs to be after
	//   the reg mask.
	{1,2,M_BASIC,L_4,0x4880,A_MASK1|D_RAW|D_SZ3,A_MEM|D_EA},	/* LBUG : A_MEM&~POST_I */
	{1,2,M_BASIC,L_2,0x4880,A_MASK1|D_RAW|D_SZ3,A_MEM|D_EA},	/* LBUG : A_MEM&~POST_I */
	{1,2,M_BASICSWAPPED,L_2+L_4,0x4C80,A_MASK2|D_RAW,A_MEM0|D_EA|D_SZ3},
	// op_rts
	{1,0,M_BASIC,L_0,0x4E75},
	// op_bra
	{1,1,M_BRANCH,L_0+L_1+L_2,0x6000},
	// op_bsr
	{1,1,M_BRANCH,L_0+L_1+L_2,0x6100},
	// op_beq
	{1,1,M_BRANCH,L_0+L_1+L_2,0x6700},
	// op_bne
	{1,1,M_BRANCH,L_0+L_1+L_2,0x6600},
	// op_bhs
	{1,1,M_BRANCH,L_0+L_1+L_2,0x6400},
	// op_bge
	{1,1,M_BRANCH,L_0+L_1+L_2,0x6C00},
	// op_bhi
	{1,1,M_BRANCH,L_0+L_1+L_2,0x6200},
	// op_bgt
	{1,1,M_BRANCH,L_0+L_1+L_2,0x6E00},
	// op_bls
	{1,1,M_BRANCH,L_0+L_1+L_2,0x6300},
	// op_ble
	{1,1,M_BRANCH,L_0+L_1+L_2,0x6F00},
	// op_blo
	{1,1,M_BRANCH,L_0+L_1+L_2,0x6500},
	// op_blt
	{1,1,M_BRANCH,L_0+L_1+L_2,0x6D00},
	// op_tst
	{1,1,M_BASIC,L_1+L_2+L_4,0x4A00,A_DST2|D_EA|D_SZ},
	// op_ext
	{1,1,M_BASIC,L_2+L_4,0x4880,A_DREG|D_EA|D_SZ3},
	// op_swap
	{1,1,M_BASIC,L_4,0x4840,A_DREG|D_EA},
	// op_neg
	{1,1,M_BASIC,L_1+L_2+L_4,0x4400,A_DST2|D_EA|D_SZ},
	// op_not
	{1,1,M_BASIC,L_1+L_2+L_4,0x4600,A_DST2|D_EA|D_SZ},
	// op_cmp
	{1,2,M_BASIC,L_1+L_2+L_4,0xB000,A_SRC2|D_EA|D_SZ,A_DREG|D_Q},
	{1,2,M_BASIC,    L_2+L_4,0xB000,A_AREG|D_EA|D_SZ,A_DREG|D_Q},
	{1,2,M_BASIC,    L_2+L_4,0xB0C0,A_SRC|D_EA|D_SZ2,A_AREG|D_Q},
/*	{1,2,M_BASIC,    L_2    ,0xB100,A_SRC2|D_EA|D_SZ,A_AREG|D_Q},
	{1,2,M_BASIC,        L_4,0xB1C0,A_SRC2|D_EA|D_SZ,A_AREG|D_Q},*/
	{1,2,M_BASIC,L_1+L_2+L_4,0x0C00,A_IMM|D_RAW|D_SZ,A_DST2|D_EA},
	{1,2,M_BASIC,L_1+L_2+L_4,0xB108,A_MEM0|D_LOW|D_SZ,A_MEM0|D_Q},	// hum; other modes than A_AINC will generate an internal error (#8451/8452)
	// op_link
	{1,2,M_BASIC,L_2,0x4E50,A_AREG|D_LOW,A_IMM|D_RAW},
	// op_unlk
	{1,1,M_BASIC,L_2,0x4E58,A_AREG|D_LOW},
	// op_pea
	{1,1,M_BASIC,L_0,0x4840,A_MEM|A_PCR|D_EA},
	// op_cmpi
	{1,2,M_BASIC,L_1+L_2+L_4,0x0C00,A_IMM|D_RAW|D_SZ,A_DST2|D_EA},
	// op_dbra
	{1,2,M_BRANCH,L_2,0x51C8,A_DREG|D_LOW},
	// op_asr
	{1,2,M_BASIC,L_1+L_2+L_4,0xE000,A_IMM|D_Q|D_SZ,A_DREG|D_EA},
	{1,2,M_BASIC,L_1+L_2+L_4,0xE020,A_DREG|D_Q|D_SZ,A_DREG|D_EA},
	{1,1,M_BASIC,L_2,0xE0C0,A_MEM|D_EA},
	// op_bset
	{1,2,M_BASIC,L_0+L_1,0x08C0,A_IMM|D_RAW2,A_MEM|D_EA},
	{1,2,M_BASIC,L_0+L_4,0x08C0,A_IMM|D_RAW2,A_DREG|D_EA},
	{1,2,M_BASIC,L_0+L_1,0x01C0,A_DREG|D_Q,A_MEM|D_EA},
	{1,2,M_BASIC,L_0+L_4,0x01C0,A_DREG|D_Q,A_DREG|D_EA},
	// op_bclr
	{1,2,M_BASIC,L_0+L_1,0x0880,A_IMM|D_RAW2,A_MEM|D_EA},
	{1,2,M_BASIC,L_0+L_4,0x0880,A_IMM|D_RAW2,A_DREG|D_EA},
	{1,2,M_BASIC,L_0+L_1,0x0180,A_DREG|D_Q,A_MEM|D_EA},
	{1,2,M_BASIC,L_0+L_4,0x0180,A_DREG|D_Q,A_DREG|D_EA},
	// op_bchg
	{1,2,M_BASIC,L_0+L_1,0x0840,A_IMM|D_RAW2,A_MEM|D_EA},
	{1,2,M_BASIC,L_0+L_4,0x0840,A_IMM|D_RAW2,A_DREG|D_EA},
	{1,2,M_BASIC,L_0+L_1,0x0140,A_DREG|D_Q,A_MEM|D_EA},
	{1,2,M_BASIC,L_0+L_4,0x0140,A_DREG|D_Q,A_DREG|D_EA},
#ifdef ASM
	/* ASM-only */
	// op_asl
	{1,2,M_BASIC,L_1+L_2+L_4,0xE100,A_IMM|D_Q|D_SZ,A_DREG|D_EA},
	{1,2,M_BASIC,L_1+L_2+L_4,0xE120,A_DREG|D_Q|D_SZ,A_DREG|D_EA},
	{1,1,M_BASIC,L_2,0xE1C0,A_MEM|D_EA},
	// op_rol
	{1,2,M_BASIC,L_1+L_2+L_4,0xE118,A_IMM|D_Q|D_SZ,A_DREG|D_EA},
	{1,2,M_BASIC,L_1+L_2+L_4,0xE138,A_DREG|D_Q|D_SZ,A_DREG|D_EA},
	{1,1,M_BASIC,L_2,0xE7C0,A_MEM|D_EA},
	// op_ror
	{1,2,M_BASIC,L_1+L_2+L_4,0xE018,A_IMM|D_Q|D_SZ,A_DREG|D_EA},
	{1,2,M_BASIC,L_1+L_2+L_4,0xE038,A_DREG|D_Q|D_SZ,A_DREG|D_EA},
	{1,1,M_BASIC,L_2,0xE6C0,A_MEM|D_EA},
	// op_roxl
	{1,2,M_BASIC,L_1+L_2+L_4,0xE110,A_IMM|D_Q|D_SZ,A_DREG|D_EA},
	{1,2,M_BASIC,L_1+L_2+L_4,0xE130,A_DREG|D_Q|D_SZ,A_DREG|D_EA},
	{1,1,M_BASIC,L_2,0xE5C0,A_MEM|D_EA},
	// op_roxr
	{1,2,M_BASIC,L_1+L_2+L_4,0xE010,A_IMM|D_Q|D_SZ,A_DREG|D_EA},
	{1,2,M_BASIC,L_1+L_2+L_4,0xE030,A_DREG|D_Q|D_SZ,A_DREG|D_EA},
	{1,1,M_BASIC,L_2,0xE4C0,A_MEM|D_EA},
	// op_btst
	{1,2,M_BASIC,L_0+L_1,0x0800,A_IMM|D_RAW2,A_MEM|D_EA},
	{1,2,M_BASIC,L_0+L_4,0x0800,A_IMM|D_RAW2,A_DREG|D_EA},
	{1,2,M_BASIC,L_0+L_1,0x0100,A_DREG|D_Q,A_MEM|D_EA},
	{1,2,M_BASIC,L_0+L_4,0x0100,A_DREG|D_Q,A_DREG|D_EA},
	// op_exg
	{1,2,M_BASIC,L_0+L_4,0xC140,A_DREG|D_LOW,A_DREG|D_Q},
	{1,2,M_BASIC,L_0+L_4,0xC148,A_AREG|D_LOW,A_AREG|D_Q},
	{1,2,M_BASIC,L_0+L_4,0xC188,A_AREG|D_LOW,A_DREG|D_Q},
	{1,2,M_BASIC,L_0+L_4,0xC188,A_DREG|D_Q,A_AREG|D_LOW},
	// op_dc
	{1,1,M_DC,0,-1},
	// op_ds
	{1,1,M_DC,0,0},
	// op_dcb
	{1,2,M_DC,0,1},
	// op_bvs
	{1,1,M_BRANCH,L_0+L_1+L_2,0x6900},
	// op_bvc
	{1,1,M_BRANCH,L_0+L_1+L_2,0x6800},
	// op_bpl
	{1,1,M_BRANCH,L_0+L_1+L_2,0x6A00},
	// op_bmi
	{1,1,M_BRANCH,L_0+L_1+L_2,0x6B00},
	// op_trap
	{1,1,M_BASIC,L_0,0x4E40,A_IMM|D_LOW},
	// op_negx
	{1,1,M_BASIC,L_1+L_2+L_4,0x4000,A_DST2|D_EA|D_SZ},
	// op_addx
	{1,2,M_BASIC,L_1+L_2+L_4,0xD100,A_DREG|D_LOW|D_SZ,A_DREG|D_Q},
	{1,2,M_BASIC,L_1+L_2+L_4,0xD108,A_ADEC|D_LOW|D_SZ,A_ADEC|D_Q},
	// op_subx
	{1,2,M_BASIC,L_1+L_2+L_4,0x9100,A_DREG|D_LOW|D_SZ,A_DREG|D_Q},
	{1,2,M_BASIC,L_1+L_2+L_4,0x9108,A_ADEC|D_LOW|D_SZ,A_ADEC|D_Q},
	// op_chk
	{1,2,M_BASIC,L_2,0x4180,A_SRC2|D_EA,A_DREG|D_Q},
	// op_even
	{1,0,M_ALIGN,L_0,2},
	// op_dbeq
	{1,2,M_BRANCH,L_2,0x57C8,A_DREG|D_LOW},
	// op_dbne
	{1,2,M_BRANCH,L_2,0x56C8,A_DREG|D_LOW},
#ifndef LIGHT_DBXX_AND_SXX
	// op_dbhs
	{1,2,M_BRANCH,L_2,0x54C8,A_DREG|D_LOW},
	// op_dbge
	{1,2,M_BRANCH,L_2,0x5CC8,A_DREG|D_LOW},
	// op_dbhi
	{1,2,M_BRANCH,L_2,0x52C8,A_DREG|D_LOW},
	// op_dbgt
	{1,2,M_BRANCH,L_2,0x5EC8,A_DREG|D_LOW},
	// op_dbls
	{1,2,M_BRANCH,L_2,0x53C8,A_DREG|D_LOW},
	// op_dble
	{1,2,M_BRANCH,L_2,0x5FC8,A_DREG|D_LOW},
	// op_dblo
	{1,2,M_BRANCH,L_2,0x55C8,A_DREG|D_LOW},
	// op_dblt
	{1,2,M_BRANCH,L_2,0x5DC8,A_DREG|D_LOW},
	// op_dbvs
	{1,2,M_BRANCH,L_2,0x59C8,A_DREG|D_LOW},
	// op_dbvc
	{1,2,M_BRANCH,L_2,0x58C8,A_DREG|D_LOW},
	// op_dbpl
	{1,2,M_BRANCH,L_2,0x5AC8,A_DREG|D_LOW},
	// op_dbmi
	{1,2,M_BRANCH,L_2,0x5BC8,A_DREG|D_LOW},
	// op_st
	{1,1,M_BASIC,L_1,0x50C0,A_DST2|D_EA},
	// op_sf
	{1,1,M_BASIC,L_1,0x51C0,A_DST2|D_EA},
	// op_seq
	{1,1,M_BASIC,L_1,0x57C0,A_DST2|D_EA},
	// op_sne
	{1,1,M_BASIC,L_1,0x56C0,A_DST2|D_EA},
	// op_shs
	{1,1,M_BASIC,L_1,0x54C0,A_DST2|D_EA},
	// op_sge
	{1,1,M_BASIC,L_1,0x5CC0,A_DST2|D_EA},
	// op_shi
	{1,1,M_BASIC,L_1,0x52C0,A_DST2|D_EA},
	// op_sgt
	{1,1,M_BASIC,L_1,0x5EC0,A_DST2|D_EA},
	// op_sls
	{1,1,M_BASIC,L_1,0x53C0,A_DST2|D_EA},
	// op_sle
	{1,1,M_BASIC,L_1,0x5FC0,A_DST2|D_EA},
	// op_slo
	{1,1,M_BASIC,L_1,0x55C0,A_DST2|D_EA},
	// op_slt
	{1,1,M_BASIC,L_1,0x5DC0,A_DST2|D_EA},
	// op_svs
	{1,1,M_BASIC,L_1,0x59C0,A_DST2|D_EA},
	// op_svc
	{1,1,M_BASIC,L_1,0x58C0,A_DST2|D_EA},
	// op_spl
	{1,1,M_BASIC,L_1,0x5AC0,A_DST2|D_EA},
	// op_smi
	{1,1,M_BASIC,L_1,0x5BC0,A_DST2|D_EA},
	// op_tas
	{1,1,M_BASIC,L_1,0x4AC0,A_DST2|D_EA},
#endif
#endif
};

void wrt4(long x) {
	if (odd) fatal("ALIGN");
	bin_chk(pos+=4);
#ifdef PC
	bin[pos-4]=(unsigned char)(x>>24);
	bin[pos-3]=(unsigned char)(x>>16);
	bin[pos-2]=(unsigned char)(x>>8);
	bin[pos-1]=(unsigned char)(x);
#else
	*(long *)(bin+pos-4)=x;
#endif
#ifdef PC
	if ((pos^odd)&1)
		fatal("INT/ODD");
#endif
}
void wrt2(short x) {
	if (odd) fatal("ALIGN");
	bin_chk(pos+=2);
#ifdef PC
	bin[pos-2]=(unsigned char)(x>>8);
	bin[pos-1]=(unsigned char)(x);
#else
	*(short *)(bin+pos-2)=x;
#endif
#ifdef PC
	if ((pos^odd)&1)
		fatal("INT/ODD");
#endif
}
void wrt1(char x) {
	bin_chk(pos+=1);
#ifdef PC
	bin[pos-1]=(unsigned char)(x);
#else
	*(char *)(bin+pos-1)=x;
#endif
	odd=~odd;
#ifdef PC
	if ((pos^odd)&1)
		fatal("INT/ODD");
#endif
}
void fill(int n) {
	if (odd) wrt1(0);
	bin_chk(pos+=n);
	memset(bin+pos-n,0,n);
	odd = -(n&1);
}
void move_pos(long diff) {	/* caution : only works in overwrite mode */
	odd^=-(diff&1);
	pos+=diff;
}
void rewrite(long size) {
	bin_chk(pos+=size);
	memcpy(bin+(pos-size),bin+(pos-2*size),size);
	odd^=-(size&1);
#ifdef PC
	if ((pos^odd)&1)
		fatal("INT/ODD");
#endif
}

readonly int sz_table[]={0,0,0,0,0,2,2,2,0,0,2,2,2,2,4,0,0,0};

readonly struct enode zero={en_icon,0,0,{0},0,0};
readonly struct amode am_null={am_direct,(struct enode *)&zero,0,0,0,0};
#define ip_lab (((struct lbls *)ip)->lab)
void pass1() {
	struct ocode *ip=scope_head;
	OPCODE *op;
	unsigned int vpos=pos;
	while (ip) {
		int opt,sz,n,f;
		struct amode *ap;
		int c=opi[ip->opcode],nm=opi[ip->opcode+1]-c;
		int orig_len=ip->length;
		int argnum_ok=0;
/*		if (ip->opcode==_op_adj) {
			sz=0; opt=0; c=-1;
			goto ok;
		}*/
		err_cur_line=ip->line;
		if (!nm) {
			if (ip->opcode!=op_label)
				fatal("P1: UNIMPL");
			{
			if (!local(ip_lab) && lab_src(ip_lab)) {
				char *name=unres_name(ip_lab);
				while (ip && ip->opcode==op_label) ip=ip->fwd;
				if (ip) err_cur_line=ip->line-1;
				else err_assembly=0;
				uerrc2("label '%s' redefined",name);
			}
			lab_set(ip_lab,vpos);
			ip->opcode=0;
			sz=0;
			goto ok_lab;
			}
		}
		nm--; do {
			opt=0;
			op=(OPCODE *)&ops[c];
			if ((!(n=op->nargs) && ip->oper1) || (n==1 && (!ip->oper1 || ip->oper2))
				|| (n==2 && !ip->oper2))
				goto nxt;
			argnum_ok++;
			ip->length=orig_len;
			switch (op->model) {
			  case M_BASICSWAPPED:	/* caution : assumes that it is the sole M_BASICSWAPPED */
				ap=ip->oper1;		/*      and that it is placed at the very end...        */
				ip->oper1=ip->oper2;
				ip->oper2=ap;
				/* FALLTHROUGH */
			  case M_BASIC:
				if (!((f=op->lenf) & (1<<ip->length))) {
					if (!ip->length) {
						if (!(f & (1<<2))) {
							if (!(f & (1<<4)))
								ip->length=1;
							else ip->length=4;
						} else ip->length=2;
					} else goto nxt;
				}
				sz=2;
				ap=ip->oper1;
				f=op->a1;
				while (n--) {
					int m=ap->mode;
					if (!(f&
						(m<am_indx2?
							(m<am_ainc?
								(m==am_dreg?A_DREG
								:(m==am_areg?A_AREG
								:A_MEM0)
								)
							:
								(m==am_indx?
								A_MEM0
								:(ip->length?(m==am_ainc?A_MEM0:A_ADEC):0)
								)
							)
						:
							(m<am_mask1?
								(m==am_immed?A_IMM:A_MEM0)
								: (m<am_pcrel?(m==am_mask2?A_MASK2:A_MASK1)
									:(m==am_pcrel?A_PCR:
							(m<am_sr?A_MEM0:(m==am_sr?A_SR:(m==am_ccr?A_CCR:A_USP)))))
							)
						)
					))
						goto nxt;
#ifdef TWIN_COMPILE
					{int tclab,tcoff;
					/*if (twinc_prev)
						bkpt();*/
#endif
					if (m==am_direct) {
						if ((f&A_PCR) && ((ap->offset->nodetype==en_labcon
							|| ap->offset->nodetype==en_nacon) &&
#ifndef LARGE_MODEL
								global(ap->offset->v.enlab)
#else
								local(ap->offset->v.enlab)
#endif
#ifdef TWIN_COMPILE
								&& (!twinc_prev ||
									((tclab=twinc_prev[0],tcoff=twinc_prev[1],
									twinc_prev+=2,
									tclab!=ap->offset->v.enlab||abs(tcoff-vpos)>=5000)
									&& (twinc_prev-=2,1)))
#endif
									)) {
							/*if (ap->offset->v.enlab==0xFFFF8032)
								bkpt();*/
							m=am_pcrel;
						} else {
							get_offs(ap->offset,0);
							if (is_long)
								m=am_dirl;
							else m=am_dirw;
						}
						ap = *(ap==ip->oper1 ? &ip->oper1 : &ip->oper2) = copy_addr(ap);
							/* this is because the same operand might be either pcrel or
							  dirl, depending on the instruction/place of the operand */
						ap->mode=m;
					}
#ifdef TWIN_COMPILE
					}
#endif
					if (f&D_RAW2)
						sz+=2;
					else if ((f&D_RAW) && m==am_immed)
						sz+=(ip->length+1)&-2;
					else if (!(f&(D_Q|D_LOW)))
						sz+=(m==am_immed?(ip->length+1)&-2:sz_table[m]);
					ap=ip->oper2;
					f=op->a2;
				}
				break;
			  case M_BRANCH:
				if (n==1) {	// Bcc
					if (ip->oper1->offset->nodetype!=en_labcon
						&& ip->oper1->offset->nodetype!=en_nacon)
//						fatal("BRANCH");
						uerrc("invalid branch");
					n=ip->oper1->offset->v.enlab;
					if (!global(n))
//						fatal("XT BRANCH");
						uerrc("invalid branch");
					if (ip->length==2) opt=0,sz=4;
					else {
						if ((f=lab_src(n))) {
							f-=vpos;
							opt=(f>129 || f<-126);	// all branches are backwards so we needn't
													//  handle branches to next stmt in this pass
						} else opt=1;
						sz=2+(opt<<1);
					}
				} else {	// DBcc
					if ((ip->oper2->offset->nodetype!=en_labcon
						&& ip->oper2->offset->nodetype!=en_nacon)
						|| (ip->oper1->mode!=am_dreg))
//						fatal("DBRANCH");
						uerrc("invalid branch");
					n=ip->oper2->offset->v.enlab;
					if (!global(n))
//						fatal("XT BRANCH");
						uerrc("invalid branch");
					sz=4;
				}
				break;
#ifdef ASM
			  case M_DC:
				f=ip->length-2;
				if (f<=0) f++;	// f=log_2(ip->length)
				if (ip->oper1->mode!=am_direct)
//					fatal("DC");
					uerrc("invalid declaration");
				sz=1;
				if (((short)op->bo)>=0) {
					if (ip->oper1->offset->nodetype!=en_icon)
						uerrc("invalid declaration");
					sz=ip->oper1->offset->v.i;
					if (op->bo && ip->oper2->mode!=am_direct)
						uerrc("invalid declaration");
				}
				sz<<=f;
				if (sz>=128)
					uerrsys("too many elements in 'dc' instruction; use C arrays");
				break;
			  case M_ALIGN:
				sz=0;
				if (vpos&1) {
					sz=1;
					c=oi_dc;
					ip->length=1;
					ip->oper1=(struct amode *)&am_null;
				}
				break;
			  default:
				fatal("P1");
				sz=0;
				break;
#endif
			}
			goto ok;
		nxt:
			c++;
		} while (nm--);
		if (!argnum_ok)
			uerrc2("instruction requires %d arguments",n);
		uerrc("invalid address modes");
		return;
//		fatal("P1: INVALID");
	ok:
#ifdef SIZE_STATS
		if (!ip->opt)	/* not from an asm() statement */
			c_compiler_sz+=sz;	/* note this is approximative, but close to reality */
#endif
		ip->opt=opt;
		ip->sz=sz;
		ip->opcode=c;
	ok_lab:
		ip=ip->fwd;
		vpos+=sz;
	}
}

void pass2() {
	struct ocode *ip=scope_head;
	unsigned int vpos=pos,opos=vpos,s;
	OPCODE *op;
	while (ip) {
		if (ip->opcode>0) {
			s=ip->sz;
			if (ip->opt) {
				op=(OPCODE *)&ops[ip->opcode];
				switch (op->model) {
				  case M_BASIC: case M_BASICSWAPPED:
					break;
				  case M_BRANCH: {	// this is necessarily a Bcc since ip->opt==1
					int n=ip->oper1->offset->v.enlab;
					long f=qlab_src(n);
					if (!f) break;	// global label not found
					if ((unsigned int)f>opos) // we HAVE TO compare with opos (since opos>=vpos)
						f-=opos+2;	// <- here, f is afterwards, so relative to old pos
					else f-=vpos+2;
					/* the case of the next stmt branch :
					 * PASS 1 : the label is unknown, so opt=1 and ip->sz=4
					 * PASS 2 : qlab_src(n)-opos==4, so here, f==2 (rather than f==0) */
					if (f!=2 && f>=-128 && f<=127)
						ip->sz=2;
					else if (f>=-32768 && f<=32767)
						ip->sz=4;
					else err_cur_line=ip->line, uerrc("branch size can't fit in a word");
				   } break;
				}
			}
			opos+=s, vpos+=ip->sz;
		} else if (!ip->opcode)	{// op_label
/*			if (vpos==0x2642)
				printf("rjo");*/
			lab_set(ip_lab,vpos);
		}
/*		else {	// _op_adj
			
		}*/
		ip=ip->fwd;
	}
}

int movem(short x) {
	int n=0;
	while (x) {
		if (x<0) n++;
		x+=x;
	}
	return n;
}

void pass3() {
	struct ocode *ip=scope_head;
#ifdef PC
	unsigned int opos=pos;
#endif
	OPCODE *op;
	while (ip) {
		int n,f;
		struct amode *ap;
		short code;
		if (ip->opcode>0) {
			err_cur_line=ip->line;
			op=(OPCODE *)&ops[ip->opcode];
			n=op->nargs;
			switch (op->model) {
			  case M_BASIC: case M_BASICSWAPPED: {
				int ds=0,ds2=0; int len=ip->length;
#ifdef PC
				long dc=0xdddddddd,dc2=0xdddddddd;
#else
				long dc=dc,dc2=dc2;
#endif
				unsigned int vpos=pos+2;
				code=op->bo;
				ap=ip->oper1;
				f=op->a1;
#ifndef USE_LINK
				if (n)
					if (ap->preg==STACKPTR-8) {
						if (ap->mode==am_ainc) {	// for <op> (a7)+,...
							lc_stk-=len+(len&1);
						} else if (ap->mode==am_adec)
							lc_stk+=len+(len&1);			// for clr  -(a7)
					}
#endif
				while (n--) {
					if ((f&D_SZ3)==D_SZ3) {
						if (len==4) code|=0x40;
					} else if (f&D_SZ)
						code|=(len==2?0x40:(len==4?0x80:0x00));
					else if ((f&D_SZ2) && len==4)
						code|=0x100;
					if (f&D_EA)
						switch (ap->mode) {
						  case am_areg:
						  case am_ind:
						  case am_ainc:
						  case am_adec:
							code+=ap->mode<<3;
						  case am_dreg:
							code+=ap->preg;
							break;
						  case am_indx:
#ifndef USE_LINK
							if (ap->preg==FRAMEPTR-8) {
								if (uses_link)
									ap->preg+=TRUE_FRAMEPTR-FRAMEPTR;
								else {
									ap->preg+=STACKPTR-FRAMEPTR;
									ap->offset->v.i += lc_stk - (ap->offset->v.i>0 ? 4 : 0);
									uses_lc_stk = 1;
								}
							}
#endif
							code+=0x28+ap->preg;
							ds=1; dc=get_word(ap->offset,vpos,1);
							break;
						  case am_pcrel:
							code+=0x3A;
							ds=1; dc=get_pcword(ap->offset,vpos); // rel to vpos, not pos!
							break;
						  case am_dirw:
							code+=0x38;
							ds=1; dc=get_word(ap->offset,vpos,1);
							break;
						  case am_dirl:
							code+=0x39;
							ds=2; dc=get_long(ap->offset,vpos);
							break;
						  case am_immed:
							code+=0x3C;
							ds=(1+len)>>1;
							if (ds==1) dc=get_word(ap->offset,vpos,0);
							else dc=get_long(ap->offset,vpos);
							break;
						  case am_indx2:
							code+=0x30+ap->preg;
							ds=1;
							dc=get_byte(ap->offset)+((short)ap->sreg<<12)+
								(ap->slen==4?0x0800:0x0000);
							break;
						  case am_indx3:
							code+=0x30+ap->preg;
							ds=1;
							dc=get_byte(ap->offset)+((short)ap->sreg<<12)+
								(ap->slen==4?0x8800:0x8000);
							break;
#ifdef PC
						  default:
							ierr(PASS3,1);
#endif
						}
					else if (f&D_EA2)
						switch (ap->mode) {
						  case am_areg:
						  case am_ind:
						  case am_ainc:
						  case am_adec:
							code+=ap->mode<<6;
						  case am_dreg:
							code+=((short)ap->preg)<<9;
							break;
						  case am_indx:
#ifndef USE_LINK
							if (ap->preg==FRAMEPTR-8) {
								if (uses_link)
									ap->preg+=TRUE_FRAMEPTR-FRAMEPTR;
								else {
									ap->preg+=STACKPTR-FRAMEPTR;
									ap->offset->v.i += lc_stk - (ap->offset->v.i>0 ? 4 : 0);
									uses_lc_stk = 1;
								}
							}
#endif
							code+=(0x28<<3)+((short)ap->preg<<9);
							ds=1; dc=get_word(ap->offset,vpos,1);
							break;
/*						  case am_pcrel:
							code+=(0x38<<3)+(0x02<<9);
							ds=1; dc=get_pcword(ap->offset,vpos); // rel to vpos, not pos!
							break;*/
						  case am_dirw:
							code+=0x38<<3;
							ds=1; dc=get_word(ap->offset,vpos,1);
							break;
						  case am_dirl:
							code+=(0x38<<3)+(0x01<<9);
							ds=2; dc=get_long(ap->offset,vpos);
							break;
						  case am_indx2:
							code+=(0x30<<3)+(ap->preg<<9);
							ds=1;
							dc=get_byte(ap->offset)+((short)ap->sreg<<12)+
								(ap->slen==4?0x0800:0x0000);
							break;
						  case am_indx3:
							code+=(0x30<<3)+(ap->preg<<9);
							ds=1;
							dc=get_byte(ap->offset)+((short)ap->sreg<<12)+
								(ap->slen==4?0x8800:0x8000);
							break;
#ifdef PC
						  default:
							ierr(PASS3,2);
#endif
						}
					else if (f&D_Q)
						switch (ap->mode) {
						  case am_areg:
						  case am_dreg:
						  case am_ainc:
						  case am_adec:
							code+=((unsigned short)ap->preg)<<9;
							break;
						  case am_immed:
							code+=get_quick(ap->offset)<<9;
							break;
#ifdef PC
						  default:
							ierr(PASS3,3);
#endif
						}
					else if (f&D_LOW)
						switch (ap->mode) {
						  case am_areg:
						  case am_dreg:
						  case am_adec:
						  case am_ainc:
							code+=ap->preg;
							break;
						  case am_immed:
							code+=get_byte(ap->offset);
							break;
#ifdef PC
						  default:
							ierr(PASS3,4);
#endif
						}
					else if (f&D_RAW)
						switch (ap->mode) {
						  case am_immed:
							if (len==4)
								ds=2,dc=get_long(ap->offset,vpos);
							else
								ds=1,dc=get_word(ap->offset,vpos,0);
							break;
						  case am_mask1:
						  case am_mask2:
							ds=1; dc=ap->offset->v.i;
							break;
#ifdef PC
						  default:
							ierr(PASS3,5);
#endif
						}
					else if (f&D_RAW2)
						ds=1,dc=get_word(ap->offset,vpos,0);
					ap=ip->oper2;
					f=op->a2;
					vpos+=ds<<1;
					if (n) {
						ds2=ds,dc2=dc,ds=0;
#ifndef USE_LINK
						if (ap->preg==STACKPTR-8) {
							if (ap->mode==am_adec) {	// for <op> ...,-(a7)
								if (ip->oper1->mode==am_mask1)
									lc_stk+=movem((short)ip->oper1->offset->v.i)<<(len>>1);
								else lc_stk+=len+(len&1);
							}
							else if (ap->mode==am_ainc && ip->oper1->mode==am_mask2)
								lc_stk-=movem((short)ip->oper1->offset->v.i)<<(len>>1);
						}
#endif
					}
				}
#ifndef USE_LINK
				if (code==0x4FEF				// lea x(a7),a7
					|| (code&0xF13F)==0x500F	// addq.* #x,a7
					|| (code&0xFEFF)==0xDEFC)	// add.* #x,a7
					lc_stk-=ip->oper1->offset->v.i;
				else if ((code&0xF13F)==0x510F	// subq.* #x,a7
					|| (code&0xFEFF)==0x9EFC)	// sub.* #x,a7
					lc_stk+=ip->oper1->offset->v.i;
				else if (((code&0xFFC0)==0x4840 && (code&0x0038/*!swap*/)))	// pea <ea>
					lc_stk+=4;
#endif
				wrt2(code);
				if (--ds2>=0) {
					if (!ds2) wrt2((short)dc2);
					else wrt4(dc2);
				}
				if (--ds>=0) {
					if (!ds) wrt2((short)dc);
					else wrt4(dc);
				}
			   } break;
			  case M_BRANCH: {
				unsigned int vpos=pos+2;
				code=op->bo;
				if (ip->oper2)	// DBcc
					n=ip->oper2->offset->v.enlab, code+=ip->oper1->preg;
				else n=ip->oper1->offset->v.enlab;	// Bcc
				if ((f=qlab_src(n))) {
					if (ip->sz==2)
						wrt2(code|=(f-vpos)&0xFF);
					else
						wrt2(code),wrt2((short)(f-vpos));
				} else
					wrt2(code),wrt2((short)-(short)vpos),lab_add_ref(n,vpos);
			   } break;
			  case M_DC: {
				struct enode *ep=ip->oper1->offset;
				n=1;
				if (((short)op->bo)>=0) {
					n=ip->oper1->offset->v.i;
					if (op->bo)
						ep=ip->oper2->offset;
					else ep=(struct enode *)&zero;
				}
				f=ip->length-2;
				while (n--) {
					if (f<0) {
						GV v;
						get_value(ep,&v,0,0);
						if (v.tr)
							uerr(ERRA_INVALIDREL);
						if (v.i>255 || v.i<-128)
							uerrc("result can't fit in a byte");
						wrt1((char)v.i);
					} else if (!f) wrt2(get_word(ep,pos,0));
					else wrt4(get_long(ep,pos));
				}
			   } break;
			}
#ifdef PC
			opos+=ip->sz;
			if (pos!=opos) {
//				printf("%d",oi_ds);
				ierr(PASS3,6);
			}
#endif
		}
/*		else if (ip->opcode) {	// _op_adj
			
		}*/
		else if (!ip->opcode) {	// op_label
//#ifdef PC
//			if (pos!=qlab_src(ip_lab))
//				 ierr(PASS3,7);
//#endif
			if (pos!=qlab_src(ip_lab))
				uerrc2("label '%s' defined twice",unres_name(ip_lab));
			if (!local(ip_lab)) {
				lab_set(ip_lab,0);	// 'unset' it so there will be no error...
				set_label(ip_lab,pos);
			}
		}
		ip=ip->fwd;
	}
}

void scope_flush(void) {
	err_assembly=1;
	pass1();
	pass2();
	pass3();
	err_assembly=0;
	if (uses_lc_stk && lc_stk) {
		uwarn("stack displacement of %d: compiler bug?",(int)lc_stk);
		iwarn(WARN_LC_STK,1);
	}
	scope_head = 0;
	loc_tab = 0;
}

void scope_init(void) {
	if (scope_head) scope_flush();
	lc_stk = 0;
	uses_lc_stk = 0;
	nextlabel = 1;
	loc_tab = 0;
}

void local_clean(void) {	/* remove local symbols from alsyms -- to be called just before rel_local */
	HTABLE *tab=&alsyms;
	SYM *ttail,**prv;
	struct htab *root;
	int i;
	if (!tab->hash)
		ierr(TABLE_HASH,2);
#ifdef PC
	if (tab->hash!=0x2489)
		ierr(TABLE_HASH,1);
#endif
	i=N_HASH;
	while (i--) {
		prv=&((root=&tab->h[i])->tail);
		while ((ttail=*prv)) {
			if (local(ttail->value.splab)) {
//				*prv = ttail->prev;
				if (ttail->next) ttail->next->prev = ttail->prev;
				if (ttail->prev) ttail->prev->next = ttail->next;
//				ttail->next = prv;
				if (root->tail==ttail) root->tail=ttail->prev;
				if (root->head==ttail) root->head=ttail->next;
/*				if (!root->tail)
					root->head=0;*/
			}
			prv = &(ttail->prev);
		}
	}
}

int label(char *s) {
	int lab; SYM *sp;
/*	if (!strcmp(s,"__L_plane") || !strcmp(s,"__D_plane"))
		bkpt();*/
	if (!(sp=search(s,-1,&alsyms))) {
		if (s[0]!='\\')
			global_flag++;
		sp=(SYM *)xalloc((int)sizeof(SYM),_SYM);
		sp->name=strsave(s);
		if (s[0]=='\\')
			lab_src(sp->value.splab=lab=nxtlabel());
		else if (internal(s))
			lab_src(sp->value.splab=lab=nxtglabel());
		else
			sp->value.splab=lab=extlabel--;
		insert(sp,&alsyms);
		if (s[0]!='\\')
			global_flag--;
	} else
		lab=sp->value.splab;
	debugf("label(%s)=%x\n",s,lab);
#ifdef PC
/*	if (lab==0x806d)
		bkpt();*/
	if (!*s)
		return lab;
#endif
	return lab;
}

void set_label(int lab,unsigned int pos) {
	/*if ((unsigned short)lab==0x8014)
		bkpt();*/
	debugf("set_lab(%x,%x)\n",lab,pos);
	if (lab_src(lab)) uerrc("label redefinition");
	if (global(lab)) {
		lab_set(lab,pos);
		if (!local(lab)) {
			REF **rp,*r;
			unsigned int p,*a; int n;
			r=*(rp=glb_ref(lab));
			while (r) {
				a=r->r;
				n=N_REF;
				while (n--) {
					if (!(p=*a++))
						goto ref_done;
#ifdef PC
					{ int m=p&1; short a,olda;
					p-=m;
					a=(bin[p]<<8)+bin[p+1],olda=a;
					if (m) {
						if ((a-=pos)>0 && olda) fatal("PC RANGE 3");
					} else if ((a+=pos)<0 && olda) fatal("PC RANGE 2");
					bin[p]=(unsigned char)(a>>8);
					bin[p+1]=(unsigned char)(a);
					}
#else
					if (p&1) {
						short *z=(short *)(bin+(p&-2));
						if (*z && ((*z)-=pos)>0)
							fatal("PC RANGE 3");
						else (*z)-=pos;
					} else if ((*(short *)(bin+p)+=pos)<0 && *(short *)(bin+p)!=pos)
						fatal("PC RANGE 2");
#endif
				}
				r=r->nxt;
			}
		  ref_done:
			*rp=0;
		}
	}
	else debugf(" --local\n");
}

void put_label(int lab) {
/*
 * output a compiler generated label.
 */
#ifdef NO_OUT
	return;
#endif
	set_label(lab,pos);
}

void g_strlab(char *s) {
/*
 * generate a named label.
 */
#ifdef NO_OUT
	return;
#endif
	put_label(label(s));
}

void genbyte(int val) {
#ifdef NO_OUT
	return;
#endif
	wrt1((char)val);
}

void genword(int val) {
#ifdef NO_OUT
	return;
#endif
	put_align2();
	wrt2((short)val);
}

typedef struct _pc_bcd_s {
	short exponent;
	unsigned char mantissa[8];
} _pc_bcd;
#ifndef NOFLOAT
void genfloat(double val) {
#ifndef BCDFLT
	put_align2();
	wrt4(double2ffp(val));
#else
	BCD bcd;
	double2bcd(val,&bcd);
	wrt2(bcd.exponent);
#ifdef PC
	{
		int i;
		for (i=0;i<BCDLEN;i++)
			wrt1(bcd.mantissa[i]);
	}
#else
	wrt4(((long *)bcd.mantissa)[0]);
	wrt4(((long *)bcd.mantissa)[1]);
#endif
#endif
}

#ifdef DOUBLE
void gendouble(double val) {
	genfloat(val);
}
#endif
#endif

void genptr(struct enode *node) {
#ifdef NO_OUT
	return;
#endif
	put_align2();
	wrt4(get_long(node,pos));
}

#ifdef PC
extern int forbid_bss;
#endif
void genstorage(struct sym *sp, int align) {
	long size = sp->tp->size;
#ifdef NO_OUT
	return;
#endif
#ifdef PC
#define no_bss (nostub_mode || forbid_bss)
#else
#define no_bss nostub_mode
#endif
	if (align!=1) put_align2();
	size=(size+1)&-2;	// round size
	if (sp->value.splab) {
		if (no_bss) {	/* that's OK in _nostub mode :) */
			if (lab_src(sp->value.splab))
				return;
		} else
			uerrc2("BSS redeclaration of '%s' : use 'extern' for prototyping",sp->name);
	} else if (no_bss) {
			if (sp->storage_class==sc_static)
				sp->value.splab=nxtglabel();
			else
				sp->value.splab=label(sp->name);
		}
	if (no_bss) {
		put_label(sp->value.splab);
		fill(size);
	} else {
		*(long *)xt_find(sp->value.splab=extlabel--)=lc_bss|0x80000000;	// DIRTY, but no hidden
		lc_bss+=size;													//  bug possible
	}																	//  (cf xalloc)
#if 0
	remain = size % AL_DEFAULT;
	if (remain != 0)
		size = size + AL_DEFAULT - remain;
	if (sp->storage_class == sc_static) {
		fprintf(output, "L%ld:" tabs "ds.b %ld\n", sp->value.i, size);
		lc_bss += sp->tp->size;
	 } else
		fprintf(output, "L%d:" tabs "ds.b %ld\n", label(sp->name),size);
#endif
#undef no_bss
}

void dumplits() {
/*
 * dump the string literal pool.
 */
	char		   *cp;
	int 			len;
/*
 * The ACK assembler may produce a .text section of an uneven length.
 * This will eventually bomb the linker when it tries to relocate
 * something in a following (.data) section, which then is misaligned
 * as a whole in memory (perhaps this is just a bug in the linker).
 *
 * To avoid this (it can only happen if the string pool is the last
 * thing dumped to the assembler file) we count the total number of
 * bytes in the string pool and emit a zero filler byte if that
 * number was uneven.
 * This is perhaps an ugly hack, but in virtually all of the cases
 * this filler byte is inserted anyway by the assembler when
 * doing the alignment necessary for the next function body.
 */
//	long			count=0;

	while (strtab != 0) {
		cseg();
		nl();
		put_label((unsigned int) strtab->label);
		cp = strtab->str;
		len = strtab->len;
		//count += (len+1);
		while (len--)
			wrt1(*cp++);
		wrt1(0);
		strtab = strtab->next;
	}
	put_align2();
}


void put_align2(void) {
/* align the following data to 2 */
	if (odd) wrt1(0);
}
/*put_align(align)
	int 			align;
// align the following data
{
	switch (align) {
	  case 1:
		break;
	  case 2:
		if (odd) wrt1(0);
	}
}*/

#ifdef LISTING
void put_external(char *s) {
/* put the definition of an external name in the ouput file */

}
void put_global(char *s) {
/* put the definition of a global name in the output file */

}
#endif

/*cseg()
{
	if (curseg != codeseg) {
		nl();
#ifdef PC
		fputs(tabs ".text\n", output);
#endif
		curseg = codeseg;
	}
}*/
/*dseg()
{
	if (curseg != dataseg) {
		nl();
#ifdef PC
		fputs(tabs ".data\n", output);
#endif
		curseg = dataseg;
	}
}*/

int radix16(char c) {
	if (isdigit(c))
		return c - '0';
/*	if (c >= 'a' && c <= 'z')
		return c - 'a' + 10;*/
	if (c >= 'A' && c <= 'Z')
		return c - 'A' + 10;
	return -1;
}
int hexatoi(char *s) {
	int x=0;
	if (strlen(s)>3) return -1;
	while (*s) {
		int y=radix16(*s++);
		if (y<0) return -1;
		x<<=4; x+=y;
	}
	return x;
}

int internal(char *s) {
	int n=0;
	char c,old=0,*p;
	if (*s == '.')
		return 1;
	p=s;
	while ((c=*p++)) {
		if (n==10) {
			if (((
#ifdef FLINE_RC
				!fline_rc && 
#endif
					!strncmp(s,"_ROM_CALL_",10)))
#ifdef RAM_CALLS
				|| !strncmp(s,"_RAM_CALL_",10)
#endif
				) {
				int f=hexatoi(p-1);
				if (f<0)
					goto cont;
				*xt_find(extlabel)=&(func_search(
#ifdef RAM_CALLS
					s[2]=='O' ? &rom_funcs : &ram_funcs
#else
					&rom_funcs
#endif
					,f))->rt;
				return 0;
			}
		}
		if (n==export_pfx_len) {
			if (!strncmp(s,export_pfx,n)) {
				int f=hexatoi(p);
				if (f<0)
					goto cont3;
				*exp_find(f)=glblabel;
				return 1;
			}
		}
		if (c=='_' && n && old=='_' && *p=='0') {
			SYM *sp;
			int f=hexatoi(p+1);
			if (f<0)
				goto cont2;
			p[-2]=0;
			if (!(sp=search(s,-1,(HTABLE *)&libsyms))) {
				global_flag++;
				sp=(SYM *)xalloc((int)sizeof(SYM),_SYM);
				sp->name=strsave(s);
				insert(sp,(HTABLE *)&libsyms);
				global_flag--;
			}
			*xt_find(extlabel)=&(func_search((FUNC **)&sp->value.i,f))->rt;
			p[-2]='_';
			return 0;
		}
	cont:
	cont2:
	cont3:
		old=c; n++;
	}
	return 1;
}

#ifdef PCH
unsigned char *_end_of(short *p) {	/* works endian-independently */
	while (*p++);
	return (unsigned char *)p;
}
unsigned char *_end_of2(short *p) {	/* works endian-independently */
	while (*p++ || *p);
	return (unsigned char *)(p+1);
}
#define end_of(p) _end_of((short *)(p))
#define end_of2(p) _end_of2((short *)(p))


#ifndef isidch
#ifdef PC
static int isidch(char c) {
	return (c>='0'&&c<='9') || (c>='A'&&c<='Z') || (c>='a'&&c<='z')
		|| c == '_' || c == '$';
}
#else
#define isidch(___c) ({register short __c=(___c); \
(__c>='0'&&__c<='9') || (__c>='A'&&__c<='Z') || (__c>='a'&&__c<='z') || __c=='_' || __c=='$';})
#endif
#endif

int pchsearch(char *id,int mode);
#define lscan(x) pchsearch(x,PCHS_ADD)
#define lexpand(x) (x)
/*#define lscan (void)lexpand
void macro_expansion(char *in,char *inbound);
char lexp_buf[100];
char *lexpand(char *s) {
	char c,*p=lexp_buf;
	strcpy(p,s);
	macro_expansion(p,&lexp_buf[100]);
	if (*p>='0' && *p<='9')
		return s;
	while ((c=*p++))
		if (!isidch(c)) return s;
	return lexp_buf;
}*/

#ifdef PC
#define g16(p) (p+=2,(p[-2]<<8)+p[-1])
#define r16(o) ((ext[o]<<8)+ext[o+1])
#define align(strp) if ((strp-ext)&1) strp++
#else
#define g16(p) (*((int *)p)++)
#define r16(o) (*(int *)(ext+o))
#define align(strp) if (((short)((long)strp))&1) strp++
#endif
void extscan(unsigned char *ext) {
	int codeOff=r16(0);
	unsigned char *offp=ext+codeOff,*strp;
	/* reloc table */
	offp=end_of(offp);
	/* export table */
	strp=end_of(offp);
	while (g16(offp))
		while (*strp++);
	align(strp);
	/* import table */
	offp=strp; strp=end_of2(offp);
	while (*(short *)offp) {
		lscan(strp);
		offp=end_of(offp);
		while (*strp++);
	}
}

#undef off		// for TI-GCC :D
#ifdef PC
#define add_offs(val,offs) do { \
				short a=bin[offs+0]<<8; \
				a+=bin[offs+1]; \
				a+=val; \
				bin[offs+0]=(unsigned char)(a>>8); \
				bin[offs+1]=(unsigned char)(a); \
			} while (0)
#else
#define add_offs(val,offs) (*(short *)(bin+offs)+=val)
#endif

void extload(unsigned char *ext) {
	int codeOff=r16(0); unsigned int wriOff;
	SYM *sp;
	if (odd) wrt1(0);
/*#ifdef PC
	if (codeOff>=0x2000)
		printf("gfio");
#endif*/
	/* In the following case, either the .ext has already been loaded, either
	 * there is a conflict, in which case the error will arise at the end
	 * (what's more, it allows for overriding of the default functions) */
	sp=search(end_of(end_of(ext+codeOff)),-1,&alsyms);
	if (sp && lab_src(sp->value.splab))
		return;
	wriOff=pos-2;			/* since offsets are based on ext::$00, not on ext::$02=code */
	bin_chk(pos+=codeOff-2);
	memcpy(bin+pos-codeOff+2,ext+2,codeOff-2);
  {
	/* reloc table */
	unsigned short off; unsigned char *offp=ext+codeOff;
	while ((off=g16(offp))) {
		rt_add_ref(&reloc_tab,off+wriOff);
		add_offs(wriOff+2,off+wriOff+2);
	}
   {
	/* export table */
	unsigned char *strp=end_of(offp);
	while ((off=g16(offp))) {
		set_label(label(strp),off+wriOff);
		while (*strp++);
	}
	align(strp);
	/* import table */
	offp=strp; strp=end_of2(offp);
	while (*(short *)offp) {
		int n=label(lexpand(strp)); unsigned int p=lab_src(n);
		while ((off=g16(offp))) {
			off+=wriOff;
			if (p) {
#ifdef PC
				unsigned short a=bin[off+2]<<8;
				a+=bin[off+3];
				a+=p;
				bin[off+2]=(unsigned char)(a>>8);
				bin[off+3]=(unsigned char)(a);
#else
				*(short *)(bin+off+2)+=p;
#endif
	/*			if (ip->sz==2)
					wrt2(code|=(f-vpos)&0xFF);
				else wrt2(code),wrt2((short)(f-vpos));*/
			} else
				lab_add_ref(n,off+2);
			rt_add_ref(&reloc_tab,off);
		}
		while (*strp++);
	}
   }
  }
}
#endif

#endif /* MC680X0 */
// vim:ts=4:sw=4
