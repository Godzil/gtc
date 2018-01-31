/*
 * GTools C compiler
 * =================
 * source file :
 * preprocessor
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
#define USE_MEMMGT
#include	"cglbdec.h"
#ifdef PCH
#include	"pch.h"
#endif

extern char *curname;
extern char *lptr;
char *inclname[10];
FILE *inclfile[10];
int inclifreldepth[10];
int incldepth CGLOB;
#ifdef PCH
FILE			*pchfile[20];
char			*pchdata[20] CGLOBL;
char			*pchtab[20];
#ifdef REQ_MGT
char			 pchname[20][15];
char             pchrequired[20];
#endif
int 			pchnum CGLOB;
#endif
int inclline[10];
int lineno CGLOB;
int lineid CGLOB;
int prevlineid CGLOB;
int flags CGLOB;
#ifdef PC
int verbose CGLOB;
#endif
#ifdef PC
int verbosity CGLOB;
#endif
int ifdepth CGLOB;
int ifreldepth CGLOB;
int ifcount[MAX_IF_DEPTH];
int ifval[MAX_IF_DEPTH];
int ifhas[MAX_IF_DEPTH];
int ifskip CGLOB, ifsd CGLOB;

HTABLE defsyms CGLOBL;

extern int getline(int f);

extern TYP *expression();
extern unsigned char id_are_zero;

int doinclude(
#ifdef PCH
		  int pch
#endif
			  );

extern enum(e_sym) forbid;
xstatic int pp_old_forbid CGLOB;
xstatic int pp_old_crc CGLOB;
xstatic char pp_old_id[MAX_ID_LEN+1];

/*static int mem_if=0,mem_def=0;
#define MEM_USE(x,s) mem_##x-=glbmem; s mem_##x+=glbmem;*/

int ppquit() {
	if (pp_old_forbid IS_VALID) {
		forbid=pp_old_forbid;
		if (pp_old_forbid==id) memcpy(lastid,pp_old_id,MAX_ID_LEN+1), lastcrc=pp_old_crc;
	}
#ifdef MACRO_PPDIR
	if (*lptr) lptr=strchr(lptr,'\n')+1;
	if (!*lptr) return getline(incldepth == 0);
	else return 0;
#else
	return getline(incldepth == 0);
#endif
}

#if 0
char *strend(char *s) {
	while (*s++);
	return s-1;
}
#endif

int preprocess() {
	if ((pp_old_forbid=forbid)==id) memcpy(pp_old_id,lastid,MAX_ID_LEN+1), pp_old_crc=lastcrc,
		forbid=-1;
	++lptr; // skip '#'
	lastch = ' ';
	getch();
	getsym();				/* get first word on line */
	if(lastst != id && lastst != kw_if && lastst != kw_else) {
		uerrc("preprocessor command expected");
		return ppquit();
	}
/*	if (!strcmp(curname,"compat.h") && lineno>=23)
		bkpt();*/
/*	if (!strcmp(curname,"messages.c") && lineno>=0)
		bkpt();*/
	if (lastst == kw_if /*!strcmp(lastid,"if")*/ || !strcmp(lastid,"elif")) {
		struct enode *ep; TYP *tp; enum(e_bt) typ;
		int v=lastst==kw_if;
		if (!v) ifskip=ifhas[ifdepth];
		if (ifskip) return doif(-1,v);
#ifndef MACRO_PPDIR
		strcpy(lptr," ] ");	// so that getsym() in the new expression will
		getsym();			//  not read any new line
#else
	{
		char *q=strchr(lptr,'\n');
		memmove(q+3,q,strlen(q)+1);
		memcpy(q," ] ",3);
		getsym();
	}
#endif
		tmp_use();
		id_are_zero++;
		tp=expression(&ep);
		id_are_zero--;
		if (!tp || lastst!=closebr) error(ERR_EXPREXPECT);
		else if ((typ=tp->type)==bt_void || typ==bt_pointer) error(ERR_INTEGER);
		else {
			opt0(&ep);
			if (ep->nodetype!=en_icon) error(ERR_CONSTEXPECT);
			else { int w=doif(ep->v.i!=0,v); tmp_free(); return w; }
		}
		tmp_free();
		return ppquit();
	} else if (!strcmp(lastid,"ifdef") || !strcmp(lastid,"ifndef")) {
		int v=lastid[2]!='d';
		if (ifskip) return doif(-1,1);
		skipspace();
		if ((lastch>='A'&&lastch<='Z') || (lastch>='a'&&lastch<='z')
			|| lastch=='_' || lastch=='$') {
			getidstr();
			return doif(v ^ !!search(lastid,lastcrc,&defsyms),1);
		} else error(ERR_IDEXPECT);
		return ppquit();
	} else if (lastst == kw_else/*!strcmp(lastid,"else")*/) {
		if (!ifreldepth) uerrc("'#else' unexpected");
		else if (!ifskip || ifdepth<=ifsd) {
			if (--ifcount[ifdepth]!=0) uerrc("'#else' unexpected");
			else ifskip = ifval[ifdepth];
		}
		return ppquit();
	} else if (!strcmp(lastid,"endif")) {
		if (!ifreldepth) uerrc("'#endif' unexpected");
		else {
			ifdepth--, ifreldepth--;
			if (ifdepth<ifsd) ifskip=0;
			if (!ifskip) ifskip = ifcount[ifdepth] - ifval[ifdepth];
		}
		return ppquit();
	} else if (ifskip)
		return ppquit();	// ignore all other preproc directives
	else if (!strcmp(lastid,"include"))
#ifdef PCH
		return doinclude(0);
#else
		return doinclude();
#endif
#ifdef PCH
	else if (!strcmp(lastid,"header") || !strcmp(lastid,"headeropt"))
		return doinclude(lastid[sizeof("header")-1]?-1:1);
#endif
	else if (!strcmp(lastid,"define"))
		return dodefine(1);
#ifdef MACRO_PPDIR
	else if (!strcmp(lastid,"macro"))
		return dodefine(-1);
#endif
	else if (!strcmp(lastid,"undef"))
		return dodefine(0);
	else if (!strcmp(lastid,"pragma")) {
		getsym();
		if (lastst==id && !strcmp(lastid,"lang")) {
			int not_flag=0;
			getsym();
			needpunc(openpa);
			while (lastst==id || lastst==not) {
				if (lastst==not) not_flag^=1;
				else {
					if (!strcmp(lastid,"ansi")) {
						if (not_flag) uwarn("'!' ignored"); flags=flags_ansi;
					} else if (!strcmp(lastid,"gtc")) {
						if (not_flag) uwarn("'!' ignored"); flags=flags_fullgtc;
					} else {
						int opt;
						if (!strcmp(lastid,"mid_decl")) opt=X_MID_DECL;
						else if (!strcmp(lastid,"comp_string")) opt=X_COMP_STRING;
						else if (!strcmp(lastid,"lang_ext")) opt=X_LANG_EXT;
						else { uwarn("invalid '#pragma'"); break; }
						if (not_flag) flags &= ~opt;
						else flags |= opt;
					}
					not_flag = 0;
				}
				getsym();
			}
			if (lastst!=closepa) uerr(ERR_PUNCT,')');
/*		} else if (!strcmp(lastid,"poison")) {
			while (1) {
				getsym();
				if (*/
		} else if (lastst==id && !strcmp(lastid,"opt")) {
			getsym();
			needpunc(openpa);
			while (lastst==id) {
#ifdef SPEED_OPT
				if (!strcmp(lastid,"speed"))
					speed_opt_value = 1;
				else if (!strcmp(lastid,"normal"))
					speed_opt_value = 0;
				else if (!strcmp(lastid,"size"))
					speed_opt_value = -1;
				else if (!strcmp(lastid,"uberspeed"))
					speed_opt_value = 3;
				else
					uwarn("invalid '#pragma'");
#endif
				getsym();
			}
			if (lastst!=closepa) uerr(ERR_PUNCT,')');
		} else uwarn("unknown '#pragma' type");
		return ppquit();
	} else if (!strcmp(lastid,"warning")) {
		uwarn("#warning: %.*s",strlen(lptr)-1,lptr); return ppquit();
	} else if (!strcmp(lastid,"error")) {
		uerr(ERR_CUSTOM,lptr); return ppquit();
	} else {
		uerrc("preprocessor command expected");
		return ppquit();
	}
}

#ifdef PC
extern char *incfolders[10];
extern int incfoldernum;
#endif

#ifdef PC
#ifdef _WIN32
#define DIR_SEPARATOR_STRING "\\"
#else
#ifdef UNIXOID
#define DIR_SEPARATOR_STRING "/"
#else
#error Please define a directory separator for your platform.
#endif
#endif
#endif

#ifdef PC
void verbose_print_searchdirs() {
	int n=incfoldernum;
	while (n--)
		fprintf(stderr,"search directory: '%s'\n",incfolders[n]);
}
void verbose_print_include(char *filename) {
	int i;
	for (i=0; i<incldepth; i++)
		fprintf(stderr,"  ");
	fprintf(stderr,"include file '%s'\n",filename);
}
#endif
FILE *xfopen(char *f,char *m,int sys) {
	FILE *fp;
#ifdef PC
	char b[300];
	int n=incfoldernum;
#define xfopen_try(z) do { if ((fp=fopen(z,m))) { if (verbosity&VERBOSITY_PRINT_INCLUDES) verbose_print_include(z); return fp; } } while (0)
#else
	char b[50];
#define xfopen_try(z) do { if ((fp=fopen(z,m))) return fp; } while (0)
#endif
	if (!sys) xfopen_try(f);
#ifdef PC
	while (n--) {
		sprintf(b,"%s"DIR_SEPARATOR_STRING"%s",incfolders[n],f);
		xfopen_try(b);
	}
#else
	sprintf(b,"gtchdr\\%s",f);
	xfopen_try(b);
	sprintf(b,"gtc\\%s",f);
	xfopen_try(b);
#endif
	xfopen_try(f);
	return NULL;
}

#ifdef GTDEV
void progr_initforcurfile() {
#if 0
	long pos=ftell(input);
	fseek(input,0,SEEK_END);
	progr_coeff=(100l<<16)/(int)ftell(input);
	fseek(input,pos,SEEK_SET);
#else
	progr_coeff=(100l<<16)/peek_w(input->base);
#endif
	progr_readtonow=0;
}
#endif
int doinclude(
#ifdef PCH
		  int pch
#endif
			  ) {
	int rv; char c;
	strcat(lptr,"  ");
	skipspace();
	if ((c=lastch-'"') && c!='<'-'"')
		goto incl_err;
	getch();
	if (!((lastch>='A'&&lastch<='Z') || (lastch>='a'&&lastch<='z')
		|| lastch=='_' || lastch=='$' || lastch=='\\'))
		goto incl_err;
	getidstr();				/* get file to include, without extension */
#ifdef PCH
	if (!pch) {
#endif
		inclline[incldepth] = lineno;
		inclifreldepth[incldepth] = ifreldepth;
#ifdef GTDEV
		inclread[incldepth] = progr_readtonow;
		inclcoeff[incldepth] = progr_coeff;
#endif
		inclname[incldepth] = curname;
		inclfile[incldepth++] = input;	/* push current input file */
	restart_h:
		input = xfopen(lastid,"r",c);
		if (!input) {
			char *tigcclib_aliases = "all\0alloc\0args\0asmtypes\0assert\0bascmd\0basfunc\0basop\0cert\0compat\0ctype\0default\0dialogs\0dll\0error\0estack\0events\0files\0flash\0float\0gdraw\0graph\0graphing\0gray\0homescr\0intr\0kbd\0limits\0link\0math\0mem\0menus\0nostub\0peekpoke\0printf\0romsymb\0rsa\0setjmp\0sprites\0statline\0stdarg\0stddef\0stdio\0stdlib\0string\0system\0textedit\0timath\0unknown\0values\0vat\0version\0wingraph\0";
			while (*tigcclib_aliases)
				if (
			#ifdef PC
						!strncmp(lastid,tigcclib_aliases,strlen(tigcclib_aliases)) && !strcmp(lastid+strlen(tigcclib_aliases),".h")
			#else
						!strcmp(lastid,tigcclib_aliases)
			#endif
				   ) {
			#ifdef PC
					strcpy(lastid,"tigcclib.h");
			#else
					strcpy(lastid,"tigcclib");
			#endif
					goto restart_h;
				} else {
					while (*tigcclib_aliases++);
				}
//			input = inclfile[--incldepth];
			uerr(ERR_CANTOPEN,lastid);
//			_exit(18);
		}
		ifreldepth = 0;
#ifdef GTDEV
		progr_initforcurfile();
#endif
		global_flag++;
		curname = strsave(lastid);
		global_flag--;
#ifdef PCH
	} else {
		char b[30];
#ifdef REQ_MGT
		int i=pchnum;
		while (i--)
			if (!strcmp(lastid,pchname[i]))
				goto done;
#endif
#ifdef PC
		sprintf(b,"%s.pch",lastid);
#else
		sprintf(b,"zheader\\%s",lastid);
#endif
		if (!(pchfile[pchnum] = xfopen(b, "rb", c))) {
			if (pch<0)
				goto done;
			uerr(ERR_CANTOPEN,b);
//			_exit(18);
		}
#ifdef REQ_MGT
		strcpy(pchname[pchnum],lastid);
		pchrequired[pchnum]=0;
#endif
#ifdef PC
		pchdata[pchnum] = malloc(150000);
		fread(pchdata[pchnum],1,150000,pchfile[pchnum]);
		if (w2ul(pchhead[pchnum]->magic)!=
			((long)'P'<<24)+((long)'C'<<16)+((long)'H'<<8)
				|| !(pchtab[pchnum] = malloc(w2us(pchhead[pchnum]->nID)))) {
			fclose(pchfile[pchnum]);
			goto incl_err;
		}
#define w2s(x) ((short)w2us(x))
#else
		if (w2ul(((PCH_HEAD *)(pchdata[pchnum] = *(char **)(pchfile[pchnum])))->magic)!=
			((long)'P'<<24)+((long)'C'<<16)+((long)'H'<<8)
				|| !(pchtab[pchnum] = malloc((short)pchhead[pchnum]->nID))) {
			fclose(pchfile[pchnum]);
			goto incl_err;
		}
#define w2s(x) ((short)(x))
#endif
		memset(pchtab[pchnum],0,w2s(pchhead[pchnum]->nID));
		pchnum++;
	}
#endif
done:
	if (lastch=='.') getch(),getidstr();
	c=c?'>':'"';
	if (lastch!=c)
		uerr(ERR_PUNCT,c);
	rv = getline(incldepth == 1);
#ifdef PCH
	if (!pch)
#endif
		lineno = 1; 	   /* dont list include files */
	return rv;
incl_err:
	error(ERR_INCLFILE);
	return ppquit();
}

char *litlate(char *s);
extern int getch_noppdir;
int dodefine(int mode) {	// 1: #define, -1 : #macro, 0 : #undef
	SYM *sp,*ds;
	int n=0,flags=0;
/*	getsym();*/
	skipspace();
	if ((lastch>='A'&&lastch<='Z') || (lastch>='a'&&lastch<='z')
		|| lastch=='_' || lastch=='$') {
		getidstr();				/* get past #define */
	} else {
		error(ERR_DEFINE);
		return ppquit();
	}
	if (!mode) {
		if (!symremove(lastid,&defsyms))	/* if we didn't unload anything, let's */
			pchsearch(lastid,PCHS_UNLOAD);	/*  try and unload the symbol */
		return ppquit();
	}
#ifdef AS
	if (!strcmp(lastid,"NOSTUB"))
		nostub_mode=1;
#endif
#ifdef ASM
	if (asm_isreserved())
		flags|=PPSYM_ASM_KEYWORD;
	if (asm_flag)
		flags|=PPSYM_DEFINED_IN_ASM;
#endif
	++global_flag;			/* always do #define as globals */
	sp = (SYM *)xalloc((int)sizeof(SYM), _SYM+DODEFINE);
	sp->name = strsave(lastid);
	sp->storage_class = sc_define;
	if (lastch=='(') {
		TABLE *tp;
/*		if (lineid==0x1A)
			bkpt();*/
		sp->tp=(TYP *)(tp=(TABLE *)xalloc((int)sizeof(TABLE), _TABLE+DODEFINE));
#ifdef NO_CALLOC
		tp->hash=0;
		tp->head=tp->tail=NULL;
#endif
		getsym();
		getsym();
		if (lastst!=closepa) {
			while (1) {
				SYM *arg;
				n++;
				if (lastst!=id) error(ERR_IDEXPECT);
				else {
					arg=(SYM *)xalloc((int)sizeof(SYM), _SYM+DODEFINE);
					arg->name=strsave(lastid);
#ifdef NO_CALLOC
					arg->tp=NULL;
#endif
					insert(arg,(HTABLE *)tp);
				}
				getsym();
				if (lastst!=comma) break;
				getsym();
			}
			if (lastst==dots) { sp->storage_class = sc_vamac; getsym(); }
			if (lastst!=closepa) needpunc(closepa);
		}
		if (!isspace(lastch))	// undo last getch()...
			lptr--;
#ifdef NO_CALLOC
	} else {
		sp->tp=NULL;
#endif
	}
#ifdef MACRO_PPDIR
	if (mode<0) {
		char *s=(char *)alloca(500),*p=s;
		int n=499;
		getch_noppdir++;
		do {
			if (getch()<0) error(ERR_DEFINE);
			*p++=(/*lastch=='\n'?'\r':*/lastch);
		} while (n-- && (lastch!='#' || *lptr!='e' || strncmp(lptr+1,"ndm",3)));
		getch_noppdir--;
		if (n<=0) error(ERR_DEFINE);
		p-=2; // remove '\n' '#'
		*p++=0;
		n = p-s;
		p = (char *)xalloc(n, STR);
		memcpy(p,s,n);
		sp->value.s=p;
	} else
#endif
	if (strlen(lptr)>1500)
		uerrsys("definition too long (1500 characters max)");
	if (!(sp->value.s = litlate(lptr)))
		uerrc("unbalanced quotes in #define");
#ifdef FLINE_RC
	if (!strcmp(sp->name,"USE_FLINE_ROM_CALLS"))
		fline_rc=1;
#endif
	if ((ds=search(sp->name,-1,&defsyms))) {
		if (strcmp(sp->value.s,ds->value.s))
			uwarn("redefinition of macro '%s'",sp->name);
		symremove(sp->name,&defsyms);
	}
	insert(sp,&defsyms);
	sp->used=n+flags;
	--global_flag;
	return ppquit();
}

// x = result of the test (1 : true, 0 : false, -1 : ignore all branches)
// c = is it a #if ? (rather than a #elif)
int doif(int x,int c) {
	if ((ifreldepth+=c,ifdepth+=c)>MAX_IF_DEPTH) uerrc("too many imbricated '#if's");
	else if (!ifreldepth) uerrc("'#elif' unexpected");
	else if (!ifskip || ifdepth<=ifsd) {
		ifsd=ifdepth;
		if (c)
			ifhas[ifdepth] = x;
		else
			ifhas[ifdepth] |= x;
		ifskip = (ifval[ifdepth] = x) - (ifcount[ifdepth] = 1);
	}
	if (x<0) {
		ifhas[ifdepth] = x;
		ifskip = x;
	}
	return ppquit();
}
// vim:ts=4:sw=4
