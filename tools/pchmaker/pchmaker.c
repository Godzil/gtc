/*
 * GTools C compiler
 * =================
 * source file :
 * Pre-Compiled Header maker code
 */

/* NOTE : this source file is completely awful, but PchMaker was originally
 *  designed to be used only internally */

#include	"define.h"
#include	"pch.h"
#define SHARED	/* we'll generate extra info so shared headers are possible */

FILE *src=0,*pch=0;
char *in=0;
#ifdef SHARED
#include <malloc.h>
char *defname=0;
int n_ignored=0; char **ignored[20];
#endif

void fail(char *s) {
#ifdef PC
	printf(s);
	printf("\n");
#else
	clrscr();
	printf(s);
	ngetchx();
#endif
}

void skipspace() {
	char c;
	while ((c=*in)==' ' || c=='\n' || c=='\t') in++;
}

#ifdef PC
#define N_MAX 4096
#else
#define N_MAX 1024
#endif
char *tab[N_MAX];
int N=0,var_size=0;
int extSz[N_MAX];
int extCumSz[N_MAX];
void *extDat[N_MAX];
int nExt=0;

/*#ifdef PC*/
void wri(char *p,int v) {
	*p++=(char)(v>>8);
	*p=(char)v;
}
void addwri(char *p,int v) {
	(*p++)+=(char)(v>>8);
	(*p)  +=(char)v;
}
int rd(unsigned char *p) {
	return (((int)*p)<<8)+p[1];
}
/*#else
#define wri(p,v) *(short *)(p)=v
#define rd(p) (*(short *)(p))
#endif*/

#ifdef PC
void maj_conv(char *s) {
	char c;
	while ((c=*s++))
		if (c>='A' && c<='Z')
			memmove(s+1,s,strlen(s)+1), *s='-';
}
#endif

char *defPack(char *s);
char *sPack(char *s,char *name,char *defn);
#ifdef PC
int memname,memdefn,meminit,memispk;
#endif
int argslen(char *s) {
	char *p=s;
	while (*p) {	/* skip each argument */
		while (*p++);
	}
	return (int)((long)p)-(int)((long)s);
}
#ifdef PC
#define LOGGING
#endif
extern char *sPk_table;
#ifdef PC
static char __sPk_tab[3000];
#ifdef PC
int sPk_usetab[128]={0};
#endif
#define __sPk_end (&__sPk_tab[3000])
void read_spack_table(int init) {
	FILE *fp=fopen("lex.txt","r");
	char buf[100]; int n; char *p;
	if (!fp) { if (init) printf("Warning : can't open lex file\n"); sPk_table="\0"+1; return; }
	__sPk_tab[0]=0;	/* sPack requires sPk_table[-1] to be 0 */
	sPk_table=__sPk_tab+1;
	sPk_table[0]=0;	/* so that the empty string always comes last :) */
	while (!feof(fp)) {
		p=sPk_table;
		fgets(buf,100,fp);
		n=strlen(buf);
		if (buf[n-1]=='\n') n--, buf[n]=0;
		while (strcmp(p,buf)>0)		/* perform a decreasing insertion sort */
			while (*p++);
		memmove(p+n+1,p,__sPk_end-(p+n+1));
		strcpy(p,buf);
	}
	if (init)
		memset(sPk_usetab,128,0);
	else {
		p=sPk_table; n=0;
		while (*p) {
			if (!sPk_usetab[n])
				memmove(p,p+strlen(p)+1,__sPk_end-p), n++;
			else { while (*p++); n++; }
		}
	}
}
#else
void read_spack_table() {
	sPk_table="\0"+1;
}
#endif
int get_spack_table_size() {
	char *p=sPk_table;
	while (*p)
		while (*p++);
	return (int)(p+1-sPk_table);
}

int hdr_read() {
	char c,cont=1;
#ifdef LOGGING
	FILE *logfp=fopen("pchlog.txt","w");
#endif
	var_size=0;
	skipspace();
	if (strncmp(in,"#var ",4)) {
		fail("'#var' expected");
		return 0;
	}
	in+=4;
	while (cont) {
		char *name,*args=NULL,*defn,*init,*block,*ptr;
#ifdef PC
		int extID=-1; int oldinit=0;
#endif
		char **tp,**tp0;
		int A,B,C,D,n/*,*ip*/; unsigned int flags=0;
		skipspace();
		if (!*in) return;
		name=in;
		while ((c=*in)!=' ' && c!='\n' && c!='\t' && c!='(') {
			if (c=='.') *in=' ';
			in++;
		}
		if (c=='(') {
			flags|=PCHID_MACRO;
			*in++=0;
			args=in;
			while ((c=*in)!=')')
				if (c==',') *in++=0;							/* terminate string */
				else if (c=='.') *in++=0, flags|=PCHID_VAMAC;	/* terminate macro */
				else in++;
			*in++=0;											/* terminate macro */
			if ((c=*in)!=' ' && c!='\n' && c!='\t') { fail("space expected"); return 0; }
		}
		*in=0; defn=in; in++;
		skipspace();
		if (*in=='D' && in[1]=='(') {
			int depth=0; char c;
			in+=2; defn=in;
			while ((c=*in++)) {
				if (c=='(') depth++;
				else if (c==')') { if (--depth<0) break; }
			}
			in[-1]=0;
			if (!c) in--;
			if (depth>=0) {
				printf("\n**************\n*** ERROR! ***\n**************\nUnmatched '('\n");
				return 0;
			}
		}
/*		if (*in=='M' && in[1]=='(') {
			int depth=0; char c;
			in+=2; defn=in;
			while ((c=*in++)) {
				if (c=='(') depth++;
				else if (c==')') { if (--depth<0) break; }
			}
			in[-1]=0;
			is_macro=(char)0x80;
		}*/
		defn=defPack(defn);
		if (def_is_packed) flags|=PCHID_PACKED;
#ifdef LOGGING
		fputs(defn,logfp);
		fputc('\n',logfp);
#endif
		skipspace();
		init=in;
		if (!*in) { cont=0; goto write_it; }
		while (*in && (*in!='#' || strncmp(in,"#var ",4))) in++;
		while ((c=*--in)==' ' || c=='\n' || c=='\t');	// remove trailing whitespace
		in++;
		cont=*in;
		*in=0;
		if (in<init) init=in;
		in++;
		skipspace();
		if (!*in) cont=0;
		in+=4;
#ifdef PC
		oldinit=strlen(init);
#endif
		init=sPack(init,name,defn);
write_it:
#ifdef SHARED
		{ int i;
		for (i=0;i<n_ignored;i++) {
			char **p=ignored[i];
			while (*p)
				if (!strcmp(name,*p++))
					goto _continue;
		}
		}
#endif
#ifdef PC
		{ char b[100]; FILE *tfp;
		sprintf(b,"a2ext\\%s.ref",name);
		maj_conv(b);
		tfp=fopen(b,"r");
		if (tfp) {
			void *buf; int sz,i; int *szp=extSz; void **edp=extDat;
			unsigned char sz_buf[2];
			fgets(b+(sizeof("a2ext\\")-1),100,tfp);
			fclose(tfp);
			tfp=fopen(b,"rb");	/* open the .ext file */
			if (!tfp) { printf("Couldn't find .ext file\n"); goto ext_ok; }
			if (fread(sz_buf,1,2,tfp)!=2) { printf("Bad .ext file\n"); goto ext_ok; }
			sz=(sz_buf[0]<<8)+sz_buf[1];
			buf=malloc(sz+1);
			if ((int)fread(buf,1,sz+1,tfp)!=sz) { printf("Bad .ext file\n"); goto ext_ok; }
			fclose(tfp);
			i=nExt; extID=0;
			while (i--) {
				if (*szp++==sz && !memcmp(*edp,buf,sz)) {
					free(buf);
					goto ext_ok;
				}
				edp++; extID++;
			}
			nExt++;
			if (!extID) *extCumSz=0;
			else extCumSz[extID]=extCumSz[extID-1]+szp[-1];
			*szp=sz; *edp=buf;
		  ext_ok:
			(void)0;
		}
		}
#endif
		A=strlen(name);
		if (args) B=argslen(args); else B=-1;
		C=strlen(defn); D=strlen(init);
		n=(A+1)+2*3+(B+1)+(C+1)+(D+1)+1;
		ptr=block=4+(char *)malloc(4+n);
		memset(ptr,0,n);
		wri((char *)((short *)ptr-1),n);
//		wri((char *)((short *)ptr-2),0);
		memcpy(ptr,name,A);
		ptr+=(A+1)+2*3;
		ptr[-2]=flags>>8;
		if (B>0) memcpy(ptr,args,B);
		memcpy(ptr+=(B+1),defn,C);
		memcpy(ptr+=(C+1),init,D);
#ifdef PC
		*(ptr+(D+1))=extID+1;
#else
		*(ptr+(D+1))=0;
#endif
		tp=tab;
		while (strcmp(*tp,block)<0) tp++;
		if (!strcmp(*tp,block)) {
			if (memcmp(*tp,block,n)) {
				printf("Warning:Redeclaration of %s\n",name);
			} else {
				free(block-4);
				continue;
			}
		}
		var_size+=n;
#ifdef PC
		memname+=A+1,memdefn+=C+1,memispk+=D+1;
		meminit+=oldinit+1;
#endif
		tp0=tp;
/*		ip=(int *)*tp;
		((int *)block)[-2]=ip[-2];
		tp--;
		do {
			tp++;
			ip=(int *)*tp;
			ip[-2]+=n;
		} while (**tp!='ÿ');*/
		memmove(tp0+1,tp0,N_MAX*sizeof(char *)+((long)tab)-((long)tp0));
		*tp0=block;
		N++;
		if (N>=N_MAX) { printf("Too many IDs."); return 0; }
	_continue:
		;
	}
#ifdef LOGGING
	fclose(logfp);
#endif
#ifdef SHARED
	{
	FILE *fp=fopen(defname,"w");
	if (fp) {
		int n=N; char **tp=tab;
		while (n--)
			fprintf(fp,"%s\n",*tp++);
		fclose(fp);
	} else {
		printf("Can't open .def file");
		return 0;
	}
	}
#endif
	return 1;
}

int written=0,pswritten[N_MAX]={0};
#ifdef PC
int dtot=0,dnum=0;
int dhtot=0,dhnum=0;
#endif
int do_write(int a,int b,int real,int psw
#ifdef PC
			 ,int depth
#endif
				 ) {
	if (a<=b) {
		int i=(a+b)/2,sz=rd((char *)((short *)tab[i]-1));
		int x=do_write(a,i-1,0,psw+sz
#ifdef PC
			,0
#endif
				),y=do_write(i+1,b,0,x
#ifdef PC
					,0
#endif
						);
		if (real) {
			char *t=tab[i];
			while (*t++);
			wri((char *)((short *)t+0),x==psw+sz?0:psw+sz);
			wri((char *)((short *)t+1),y==x?0:x);
			addwri((char *)((short *)t+2),i-1);
			fwrite(tab[i],1,sz,pch);
			written+=sz;
//			pswritten=written;
#ifdef PC
			dtot+=depth; dnum++;
			if (x==psw+sz/*) dhtot+=depth, dhnum++;
			if (*/ || y==x) dhtot+=depth, dhnum++;
#endif
			do_write(a,i-1,1,psw+sz
#ifdef PC
				,depth+1
#endif
					),do_write(i+1,b,1,x
#ifdef PC
						,depth+1
#endif
							);
			return 0;
		} else 
/*		if (((int *)tab[i])[-2]!=written)
			printf("Error, written=%d and offset=%d\n",written,((int *)tab[i])[-2]);*/
			return y;
	} else return psw;
}

#ifdef PC
void _fw16(unsigned short x,FILE *fp) {
	fputc((unsigned char)(x>>8),fp);
	fputc((unsigned char)x,fp);
}
#define fw16(x,f) _fw16((unsigned short)(x),f)
#endif

void pch_write() {
	int n,extOff,dicOff;
	int *szp=extSz,*cszp=extCumSz; void **edp=extDat;
	int sPk_size=get_spack_table_size();
	fwrite("PCH\0" "\0\0",PCH_HEAD_SIZE-6,1,pch);
	dicOff=PCH_HEAD_SIZE+var_size;
	extOff=dicOff+sPk_size;
	extOff+=(extOff&1);
	fw16(extOff,pch);
	fw16(dicOff,pch);
	fw16(N-1,pch);
	do_write(1,N-1,1,PCH_HEAD_SIZE
#ifdef PC
		,1
#endif
			);
	fwrite(sPk_table,1,sPk_size,pch);
	extOff+=nExt*2;
	if ((dicOff+sPk_size)&1) fputc(0,pch);
	n=nExt; while (n--) fw16(extOff+(*cszp++),pch);
	n=nExt;
	while (n--)
		fwrite(*edp++,1,*szp++,pch);
#ifndef PC
	fwrite("\0HDR\0\xF8",6,1,pch);
#endif
	printf("Success!\n");
}

char sym0[9]={0};

#ifdef PC
int main(int argc,char **argv) {
	char *srcname,*pchname; char pchbuf[200],defbuf[200];
	int quiet=0;
	int error = 0;
#ifdef SHARED
	while (argc>=2 && argv[1][0]=='-') {
		if (argv[1][1]=='i') {
		FILE *fp=fopen(argv[1]+2,"r");
		char lbuf[200],*lptr; int l_n=0;
		char *lnbuf[2000],**lnptr;
		if (!fp) {
			fail("Can't open .def file");
			return 254;
		}
		while (!feof(fp)) {
			fgets(lbuf,190,fp);
			lbuf[strlen(lbuf)-1]=0;	// strip \n
			if (!*lbuf) continue;
			lptr=alloca(strlen(lbuf)+1);
			strcpy(lptr,lbuf);
			lnbuf[l_n++]=lptr;
		}
		lnbuf[l_n++]=0;
		lnptr=alloca(l_n*sizeof(char*));
		memcpy(lnptr,lnbuf,l_n*sizeof(char*));
		ignored[n_ignored++]=lnptr;
		fclose(fp);
		argv++; argc--;
		} else if (argv[1][1]=='q') {
			quiet=1;
			argv++; argc--;
		} else break;
	}
#endif
	if (argc!=2 && argc!=3) {
		fail("Syntax : pchmaker [-i<pch1> ... -i<pchN>] [-q] <in> [<out>]\n");
		return 255;
	}
	srcname=argv[1];
	if (strlen(srcname)>=200-4-1) {
		fail("File name is too long\n");
		return 254;
	}
	if (argc==2) {
		char *dot_ptr = strrchr(srcname,'.');
		int dot = dot_ptr ? dot_ptr-srcname : strlen(srcname);
		strcpy(pchname=pchbuf,srcname), strcpy(pchname+dot,".pch");
#ifdef SHARED
		strcpy(defname=defbuf,srcname), strcpy(defname+dot,".def");
#endif
	} else pchname=argv[2];
#else
#include <estack.h>
void _main(void) {
	char *srcname,pchname[30];
	int error = 0;
	srcname=top_estack;
	if (*srcname!=STR_TAG) {
		fail("Usage: pchmaker(\"infile\")");
		return;
	}
	srcname--;
	while (*--srcname);
	srcname++;
	sprintf(pchname,"zheader\\%s",srcname);
#endif
	src=fopen(srcname,"r");
	pch=fopen(pchname,"wb");
	if (!src || !pch) {
		fail("Couldn't open file");
		if (src) fclose(src);
		return;
	}
#ifndef PC
	in=*(char **)src;
#else
	in=malloc(100000);
	in[fread(in,1,100000,src)]=0;
#endif
	/*pswritten=*/written=4;
	N=1;
	memset(sym0,0,9);
	sym0[3]=5;
	tab[0]=4+sym0;
	tab[1]=4+"\0\6\0\0ÿ";
	read_spack_table(1);
	if (hdr_read()) {
#ifdef PC
#define clr(x) mem##x=0
	clr(name),clr(defn),clr(ispk),clr(init);
	fseek(src,0,SEEK_SET);
	in=malloc(100000);
	in[fread(in,1,100000,src)]=0;
	written=4;
	N=1;
	memset(sym0,0,9);
	sym0[3]=5;
	tab[0]=4+sym0;
	tab[1]=4+"\0\6\0\0ÿ";
	read_spack_table(0);
	hdr_read();
#endif
#ifdef PC
		dnum=dtot=0;
		dhnum=dhtot=0;
#endif
		pch_write();
#ifdef PC
		if (!quiet) {
#define disp(x) printf("Memory used by '" #x "' : %5d bytes\n", mem##x)
		    printf("Header contains %d identifiers\n",N-1);
		    printf("Average tree depth : %f\n",((float)dtot)/dnum);		/* depth if ID is in PCH */
		    printf("Average leaf depth : %f\n",((float)dhtot)/dhnum);	/* depth if ID not found */
		    printf("\n");
		    disp(name),disp(defn),disp(ispk),disp(init);
		    printf("\nHeader has %d extension%s\n",nExt,nExt==1?"":"s");
		    printf("Memory used by extensions : %5d bytes\n", extCumSz[nExt-1]+extSz[nExt-1]);
		}
#endif
	} else
	    error = 1;
	fclose(pch);
	fclose(src);
	return error;
}
