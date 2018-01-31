/*
 * GTools C compiler
 * =================
 * source file :
 * symbol extraction routines
 *
 * Copyright 2001-2004 Paul Froissart.
 * Credits to Christoph van Wuellen and Matthew Brandt.
 * All commercial rights reserved.
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 */

#define DECLARE
#include	"define.h"
_FILE(__FILE__)
#include	"version.h"
#include	"c.h"
#include	"expr.h"
#include	"gen.h"
#include	"cglbdec.h"
#ifdef PCH
#include	"pch.h"
#endif
#ifndef NOFLOAT
#include	"ffplib.h"
#define FFP_TEN 0xA0000044
#define FFP_TENINV 0xCCCCCD3D
#endif
#ifdef PC
#ifdef SHORT_INT
#undef int
#endif
#include	<ctype.h>
#include	<stdarg.h>
#ifdef SHORT_INT
#define int short
#endif
#endif

#ifdef PC
unsigned long _w2ul(TI_LONG *x) {
	return ((unsigned long)x->x0<<24)+((unsigned long)x->x1<<16)+
		   ((unsigned long)x->x2<< 8)+((unsigned long)x->x3<< 0);
}
unsigned short _w2us(TI_SHORT *x) {
	return ((unsigned short)x->hi<<8)+x->lo;
}
#endif

FILE		   *input CGLOB;
int 			total_errors CGLOB;
int				err_force_line CGLOB,err_cur_line CGLOB,err_assembly CGLOB;
int 			lastch CGLOB;
enum(e_sym)		lastst CGLOB;
int				lastst_flag CGLOB;
char			lastid[MAX_ID_LEN+1];
int 			got_sym CGLOB;
#ifdef PCH
char			curpch[MAX_ID_LEN+1];
#endif
int				lastcrc CGLOB;
struct sym	   *lastsp CGLOB;
char			laststr[MAX_STRLEN + 1];
int 			lstrlen CGLOB;
unsigned long	ival CGLOB;
#ifndef NOFLOAT
double			rval CGLOB;
#endif

//#define MAX_NUM_ERRS 80
#define MAX_NUM_ERRS 2
int 	 err_num[MAX_NUM_ERRS];
int 	 numerrs;
char	 linein[LINE_LENGTH];
//#define linemax &linein[LINE_LENGTH]
//xstatic int	  act_line;
//xstatic char	  act_file[100] = "<unknown file>";
//xstatic int	  lineno;

#ifdef OLD_MACRO
#define CHAR_POPMAC 1
#define MAX_MAC_NUM 100
TABLE *mac_stk[MAX_MAC_NUM];
TABLE **mac_sp;
#else
#define MAX_MAC_ARGS 20
#endif

char *curname CGLOB;
char			*lptr CGLOB;	/* shared with preproc */
char			*inclname[10];	/* shared with preproc */
FILE			*inclfile[10];	/* shared with preproc */
int 			inclline[10];	/* shared with preproc */
int 			inclifreldepth[10];	/* shared with preproc */
#ifdef GTDEV
int				inclread[10];	/* shared with preproc */
int				inclcoeff[10];	/* shared with preproc */
#endif
int 			incldepth;		/* shared with preproc */
char			*linstack[20];	/* stack for substitutions */
char			chstack[20];	/* place to save lastch */
int 			lstackptr CGLOB;/* substitution stack pointer */
int				preproc_stmt CGLOB;
#ifdef PC
enum(e_sym)		cached_sym CGLOB;	/* cached symbol (for preproc-emulation prefetching) */
#else
enum(e_sym)		_cached_sym CGLOB;
#endif
int				cached_flag CGLOB;
int				cached_lineid CGLOB;
enum(e_sym)		forbid CGLOB;
int				pch_init CGLOB;

extern int getch_comment,getch_pp,getch_in_string,getch_noppdir;

xstatic char *linemin CGLOB,*linemax CGLOB;

int getline(int f);
int getcache(enum(e_sym) f);

#ifndef isalnum
#define isalnum _isalnum
static int isalnum(char c) {
/*
 * This function makes assumptions about the character codes.
 */
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
		(c >= '0' && c <= '9');
}
#endif

//#ifndef isidch
#ifdef PC
static int isidch(char c) {
	return (c>='0'&&c<='9') || (c>='A'&&c<='Z') || (c>='a'&&c<='z')
		|| c == '_' || c == '$';
}
static int isbegidch(char c) {
	return (c>='A'&&c<='Z') || (c>='a'&&c<='z') || c == '_' || c == '$';
}
#else
#define isidch(___c) ({register short __c=(___c); \
(__c>='0'&&__c<='9') || (__c>='A'&&__c<='Z') || (__c>='a'&&__c<='z') || __c=='_' || __c=='$';})
#define isbegidch(___c) ({register short __c=(___c); \
(__c>='A'&&__c<='Z') || (__c>='a'&&__c<='z') || __c=='_' || __c=='$';})
#endif
//#endif

#undef isspace
#ifdef PC
#define isspace _isspace
int isspace(int c) {
	return (c == ' ' || c == '\t' || c == '\n');
}
#else
#define isspace(___c) ({register short __c=(___c); \
__c==' ' || __c=='\t' || __c=='\n';})
#endif

#ifndef isdigit
#define isdigit _isdigit
static int isdigit(char c) {
/*
 * This function makes assumptions about the character codes.
 */
	return (c >= '0' && c <= '9');
}
#endif

#ifdef GTDEV
void progr_initforcurfile();
int progr CGLOB,progr_readtonow CGLOB,progr_coeff CGLOB;
#endif
#define stringify_noexpand(s) #s
#define stringify(s) stringify_noexpand(s)
char * readonly stddefines="__GTC__\000"stringify(GTC_MAJOR)"\0" "__GTC_MINOR__\000"stringify(GTC_MINOR)"\0" "__VERSION__\000\""GTC_VERSION"\"\0"
	"mc68000\000\0" "__embedded__\000\0"
	"__DATE__\000\""__DATE__"\"\0" "__TIME__\000\""__TIME__"\"\0"
	;
void insert_macros(char *p) {
	SYM *sp;
	while (*p) {
		sp=(SYM *)xalloc((int)sizeof(SYM), _SYM);
		sp->name=p;
		while (*p++);
		sp->value.s=p;
		while (*p++);
		insert(sp,&defsyms);
	}
}
#ifdef PC
/* warning: duplicated in cmain.c! */
enum OptionModes { optn_parse=0, optn_global_init, optn_global_init_post, optn_preproc_init };
void option_parse_all(enum(OptionModes) ex_mode);
#endif
extern TI_SHORT func_attr;
#ifdef PCH
void initpch() {
	pchnum = 0;
}
#endif
void initsym() {
#if defined(PC) || (defined(PCH) && defined(MULTIFILE))
	int i;
#endif
#ifdef OLD_MACRO
	lptr = &linein[MAX_MAC_LEN+1];
#else
	lptr = linein+1;
#endif
	*lptr = 0;
	numerrs = 0;
	total_errors = 0;
	err_force_line = -1;
	err_assembly = 0;
	lineno = lineid = 0;
	ifsd = ifskip = ifdepth = ifreldepth = 0;
	ifcount[0] = 0;
	ifval[0] = 0;
#ifdef PCH
#ifdef MULTIFILE
	for (i=pchnum;i--;)
		memset(pchtab[i],0,(short)w2us(pchhead[i]->nID));
#endif
	pch_init = 0;
#endif
#ifdef MID_DECL
	middle_decl = 0;
#endif
#ifdef NEW_ATTR
#ifdef PC
	func_attr.lo = -1;
	func_attr.hi = -1;
#else
	func_attr = -1;
#endif
#endif
#ifdef OLD_MACRO
	mac_sp = &mac_stk[MAX_MAC_NUM];
#endif
#ifdef FLINE_RC
	fline_rc=0;
#endif
#ifdef GTDEV
	progr_initforcurfile();
#endif
	getch_comment=0;
	getch_pp=0;
	getch_in_string=0;
	getch_noppdir=0;
	preproc_stmt=0;
	/*cached_sym2=*/cached_sym=-1;
	forbid=-1;
#ifdef MIN_TEMP_MEM
	min_temp_mem=0;
#endif
	linemin=linein+1;
	linemax=linein+LINE_LENGTH;
	global_strings = 0;
	hashinit(&defsyms);	// this is because the file could *begin* with #define's and/or #if's
	global_flag = 1;	//  (idem)
	temp_local = 0;
	/* pre-defined preprocessor macros */
	insert_macros(stddefines);
#ifdef PC
	option_parse_all(optn_preproc_init);
#endif
}

#if defined(PC) || defined(FLASH_VERSION)
xstatic readonly char    * readonly __err[] = {
	"syntax error",
	"illegal character",
	"illegal floating-point constant",
	"illegal type",
	"undefined identifier '%s'",
	"no field allowed here",
	"'%c' expected",
	"identifier expected",
	"initialization invalid here",
	"incomplete struct/union/enum declaration",
	"illegal initialization",
	"too many initializers",
	"illegal storage class",
	"function body expected",
	"pointer type expected",
	"function type expected",
	"'%s' is not a member of structure",
	"l-value required",
	"error dereferencing a pointer",
	"mismatch between types '%s' and '%s'",
	"expression expected",
	"'while' expected in do-loop",
	"enum value out of range",
	"duplicate case label",
	"undefined label",
	NULL/*"preprocessing error"*/,
	"declared argument missing",
	"illegal field width",
	"illegal constant integer expression",
	"cannot cast from '%s' to '%s'",
	"integer-valued type expected",
	"error casting a constant",
	"illegal redeclaration of '%s'",
	NULL/*"error while scanning parameter list"*/,
	"bad field type, must be unsigned or int",
	"invalid #include statement",
	"cannot open file \"%s\"",
	"wrong #define",
	"#error: %s",
	"duplicate symbol '%s'",
	"constant expression expected",
	"value out of range",
	"too many parameters to function",
	"too few parameters to function",
	"invalid case range",
	"unexpected end of file",
	"%s",
	"internal limit exceeded: %s",
	"invalid relocation in expression",
};
#endif

int _getline(int listflag,char *base) {
	//int rv; //char *p;
#ifdef LISTING
	int i;
#endif
	char *lptr=base;
#ifdef LISTING
	if (lineno > 0 && list_option && !ifskip) {
		fprintf(list, "%s:%6d", lptr, lineno);
		for (i = 0; i < numerrs; i++)
			fprintf(list, " error: %s\n", __err[err_num[i]]);
		numerrs = 0;
	}
#endif
#ifdef GTDEV
	{
	int new_progr=((long)/*progr_readtonow*/(((void *)input->fpos-input->base)-7)*progr_coeff)>>16;
	if (new_progr!=progr)
		progr=new_progr,progr_process(func_sp?func_sp->name:0,curname,new_progr);
	}
#endif
	do {
		++lineno;
	//	  ++act_line;
		if ((int)(&linein[LINE_LENGTH-1]-lptr)<100)
			uerrsys("line is too long");
		if (!fgets(lptr,(int)(&linein[LINE_LENGTH-1]-lptr),input)) {
			if (incldepth > 0) {
				if (ifreldepth) uerrc("unterminated '#if' statement");
				fclose(input);
				input = inclfile[--incldepth];
				curname = inclname[incldepth];
#ifdef GTDEV
				progr_readtonow = inclread[incldepth];
				progr_coeff = inclcoeff[incldepth];
#endif
				lineno = inclline[incldepth];
				ifreldepth = inclifreldepth[incldepth];
	#ifdef OLD_MACRO
				lptr=&linein[MAX_MAC_LEN+1];
	#else
				lptr=linein+1;
	#endif
				*lptr='\n'; lptr[1]=0;
				return 0;
	//			return getline(0);
			} else
				return 1;
		}
#ifdef PC
		if (strlen(lptr)>=2 && lptr[strlen(lptr)-2]=='\r')	// workaround so that Unix version accepts DOS files
			strcpy(&lptr[strlen(lptr)-2],"\n");
#endif
	} while (*(lptr+=strlen(lptr)-2)=='\\' && !ifskip);
#ifdef GTDEV
	progr_readtonow+=strlen(lptr);	// this isn't byte-precise, but OK for a percentage :)
#endif
	/* the following instruction is used so that when reading the character after '#endif'
	 * at the end of a header file (in order to tell 'endif' from 'endif_foo'), the
	 * preprocessor still operates in the current header file, preventing it from
	 * generating an error since a '#if' statement isn't complete */
	/*if (lineno==0x1DC)
		bkpt();*/
	if (lptr[1]!='\n')		/* lptr[2] is '\0' at this point */
		lptr++,lptr[1]='\n',lptr[2]=0;
	preproc_stmt=0;
	return 0;
}

int getline(int listflag) {
	return _getline(listflag,
	#ifdef OLD_MACRO
		lptr=&linein[MAX_MAC_LEN+1]
	#else
		lptr=linein+1
	#endif
		);
/*	p=lptr;
	while (*p && isspace(*p)) p++;
	if (*p == '#') {
		lptr=p;*/
					/*	++lptr;
						lastch = ' ';
						getsym();
						if (lastst == id && !strcmp(lastid, "line"))
								getsym();
						if (lastst != iconst)
								error(ERR_PREPROC);
						act_line = ival-1;
						// scan file name
						i = 0;
						while (isspace(*lptr))
								lptr++;
						if (*lptr == '"')
								lptr++;
						// assume that a filename does not end with double-quotes
						while (*lptr != '\0' && *lptr != '"' && !isspace(*lptr))
								act_file[i++] = *lptr++;
								// DECUS cpp suppresses the name if it has not changed
								// in this case, i is simply zero, and then we keep the 
								// old name
						if (i > 0)
								act_file[i] = '\0';
						return getline();*/
/*		preproc_stmt=1;
		return preprocess();
	}*/
/*	while ((c=*lptr++))
		if (c=='#' && *lptr=='#') {
			char *p=lptr-1;
			char id[MAX_ID_LEN+1],c=*lptr++;
			int i = 0; SYM *sp;
			while (isidch(c=*lptr++)) {
				if (i < MAX_ID_LEN)
					id[i++] = c;
			}
			id[i] = '\0';
			if (mac_sp!=&mac_stk[MAX_MAC_NUM] && (sp = search(lastid, lastcrc, *mac_sp))) {
				char *q=sp->value.s,*e=p+strlen(q);
				//memmove(p,lptr,&linein[MAX_MAC_LEN+1000]-lptr);
				//memmove(e,p,&linein[MAX_MAC_LEN+1000]-e);
				memmove(e,lptr,&linein[MAX_MAC_LEN+1000]-(lptr>e?lptr:e));
				memcpy(p,q,e-p);
				lptr=e;
			}
		}
	lptr=&linein[MAX_MAC_LEN+1];*/
//	return 0;
}

/*
 * getch - basic get character routine.
 */
xstatic int getch_comment CGLOB,getch_pp CGLOB,getch_in_string CGLOB;
int getch_noppdir CGLOB;
xstatic char *getch_p CGLOB;
int getch() {
	if (getch_pp) goto ppdo;
	do {
#ifdef MACRO_PPDIR
		if (lptr!=linein+1 &&  lptr[-1]=='\n' && *lptr) {
			lastch=*lptr;
			goto pptest_continue;	/* we needn't bother about comments since all are removed
									 *  because dodefine() calls getch() to read the macro   */
		}
#endif
		while ((lastch = *(unsigned char *)lptr++) == '\0') {
			if( lstackptr > 0 ) {
				lptr = linstack[--lstackptr];
				lastch = chstack[lstackptr];
				return lastch;
			}
			if (getline(incldepth == 0))
				return lastch = -1;
		pptest_continue:
			if (!getch_comment) {
				getch_p=lptr;
				while (*getch_p && isspace(*getch_p)) getch_p++;
				if (*getch_p == '#' && !getch_noppdir) {
					getch_pp=1;
					lastch = ' ';
					return 0;
				ppdo:
					getch_pp=0;
					lptr=getch_p;
					preproc_stmt=1;
					if (preprocess()) return lastch = -1;
					preproc_stmt=0;
					goto pptest_continue;
				}
			}
		}
		if (lastch=='/' && !getch_in_string) {
			if (*(unsigned char *)lptr=='/' && !getch_comment) {
				while (*(unsigned char *)lptr++)
					if (*(char *)lptr=='\n' && lptr[1])
						uwarn("multi-line comment");
				lptr--;
				return getch();
			} else if (*(unsigned char *)lptr=='*') {
				char last;
				if (getch_comment) {
					uwarn("/" "* in comment block");
					return 0;
				}
				getch_comment=1;
				do {
					last=(char)lastch;
					if (getch()<0) uerrc("unended comment block");
				} while (last!='*' || lastch!='/');
				getch_comment=0;
				return getch();
			}
		}
/*#ifdef ASM
		else if (asm_flag && lastch==';' && !comment) {
			while (*(unsigned char *)lptr++);
			lptr--; return getch();
		}
#endif*/
#ifdef OLD_MACRO
		if (lastch==CHAR_POPMAC) {
			mac_sp++; return getch();
		}
#endif
	} while (ifskip && !getch_comment/* since otherwise we would 'eat' the callers' chars */
		&& !preproc_stmt);
	//if (lastch=='\r') lastch='\n';
#ifdef OLD_MACRO
	if (isspace(lastch) || lastch=='#') {
		char c,*z=lptr-1;
		while (isspace(c=*z++));	// skip spaces
		if (c=='#' && *z=='#') {	// ## ?
			char *p=z-1;
			char id[MAX_ID_LEN+1],c=*z++;
			int i = 0; SYM *sp;
			while (isidch(c=*z++)) {
				if (i < MAX_ID_LEN)
					id[i++] = c;
			}
			id[i] = '\0';
			if (mac_sp!=&mac_stk[MAX_MAC_NUM] && (sp = search(id, -1, (HTABLE *)*mac_sp))) {
				char *q=sp->value.s,*e=p+strlen(q);
				//memmove(p,z,&linein[MAX_MAC_LEN+999]-z);
				//memmove(e,p,&linein[MAX_MAC_LEN+999]-e);
				memmove(e,z,&linein[MAX_MAC_LEN+999]-(z>e?z:e));
				memcpy(p,q,e-p);
				lptr=p;
			} else lptr=p+2;	// ignore ##
			return getch();
		}
	}
#endif
	/*if (lastch=='@')
		bkpt();*/
	return 0;
}

#ifndef GTDEV
void locate(void) {
	if (err_force_line IS_INVALID) {
#ifdef PC
		msg3("%s:%d: ", curname, lineno);
#else
		char lcl[6+1],*p=lcl,*l=lptr-1; int n=6;
		while (n-- && *l!='\n') *p++=*l++;
		*p=0;
		msg4("%s:%d (%s): ", curname, lineno, lcl);
#endif
	} else
		msg3("%s:%d: ", curname, err_force_line);
}
#endif

#if 0
/*
 * error - print error information
 */
void error(int n) {
	debug('e');
	if (numerrs < MAX_NUM_ERRS)
		err_num[numerrs++] = n;
	++total_errors;
	locate();
#ifdef PC
	msg3("error %d: %s\n", n, __err[n]);
#else
	msg2("error %d\n", n);
#endif
/*
 * Do not proceed if more than MAX_ERROR_COUNT errors were detected.
 * MAX_ERROR_COUNT should be high since each error might be reported
 * more than once
 */
	if (total_errors > MAX_ERROR_COUNT) {
		msg("Too many errors\n");
		_exit(1);
	}
}
#endif

#if 0
void do_warning(char *s, ...) {
#ifndef __GTC__
	va_list args;
	va_start(args,s);
	locate();
	msg("warning: ");
	vmsg(s,args);
	va_end(args);
/*	if (total_errors > MAX_ERROR_COUNT) {
		msg("Too many errors\n");
		_exit(1);
	}*/
#else
	
#endif
}
#endif

#include "error.c"

/*
 * getidstr - get an identifier string, that is either a macro name or an include file.
 *
 * Note that there are no escape characters
 */
void getidstr() {
	register int	i;
	i = 0;
	while (isidch(lastch) || lastch=='\\' || lastch=='/'
#ifdef PC
		|| lastch=='.'
		|| lastch==':'
		|| lastch=='-'
#endif
		) {
		if (lastch=='/') lastch='\\';
//		if (lastch=='\\') getch();
		if (i < MAX_ID_LEN)
			lastid[i++] = lastch;
		getch();
	}
	lastid[i] = '\0';
	lastcrc=crcN(lastid);
}

void getrawid() {
	register int i = 0;
	while (isidch(lastch)) {
		if (i < MAX_ID_LEN)
			lastid[i++] = lastch;
		getch();
	}
	lastid[i] = '\0';
#ifdef PCH
	if (!lastid[1] && lastid[0]=='_')
		memcpy(lastid,curpch,MAX_ID_LEN+1);
#endif
	lastcrc=crcN(lastid);
}

void skipspace() {
	while (isspace(lastch)) getch();
}

extern HTABLE defsyms;
// including #define's
#ifdef OLD_MACRO
int getid(int c) {
	register int	i;
	SYM *sp;
	if (c>=0) {
		lastid[0] = c;
		i = 1;
		while (isidch(lastch)) {
			if (i < MAX_ID_LEN)
				lastid[i++] = lastch;
			getch();
		}
		lastid[i] = '\0';
	}
#ifdef PCH
	if (!lastid[1] && lastid[0]=='_')
		memcpy(lastid,curpch,MAX_ID_LEN+1);
#endif
//search_id:
	lastcrc=crcN(lastid);
#ifdef OLD_MACRO
	if (mac_sp==&mac_stk[MAX_MAC_NUM] || !(sp = search(lastid, lastcrc, (HTABLE *)*mac_sp)))
#endif
		sp = search(lastid, lastcrc, &defsyms);
	if (sp
#ifdef OLD_MACRO
		&& strcmp(sp->value.s,sp->name)
#endif
			) {
		char *p,*q,c; TABLE *tp;
		if ((tp=(TABLE *)sp->tp)!=NULL) {
#ifdef OLD_MACRO
			int d=0,nmac=sp->used,nargs=0;
			char buf[150],*bp=buf;
			SYM *s=tp->head;
			*--mac_sp=tp;
			if (mac_sp<=mac_stk) fatal("GETMAC/stk_overflow");
			skipspace();
			if (lastch!='(') fatal("GETMAC");
			while (1) {
				getch();
				if (lastch=='(') d++;
				else if (lastch==')') { if (--d<0) break; }
				else if (lastch==',' && !d && (sp->storage_class!=sc_vamac || nargs!=nmac-1)) {
					if (nargs<nmac) {
						*bp=0; s->value.s=strsave(bp=buf); nargs++;
						s=s->next; continue;
					} else fatal("GETMAC");
				}
				*bp++=lastch;
				if (bp>=&buf[150]) fatal("GETMAC/too_long");
			}
			if (tp->head) {
				*bp=0; s->value.s=strsave(buf);
				if (++nargs!=nmac) fatal("GETMAC");
			}
#else
			int d=0,nmac=sp->used,nargs=0,concat_mode;
			char ps[500],args[300];
			char *argb=args,*argp=args;
			char c,lastsym[MAX_ID_LEN+1],*psptr,*lptrsav; int lastchsav=lastch;
			SYM *s=tp->head;
			skipspace();
			if (lastch!='(') return 0;
			while (1) {
				getch();
				if (lastch=='(') d++;
				else if (lastch==')') { if (--d<0) break; }
				else if (lastch==',' && !d && (sp->storage_class!=sc_vamac || nargs<nmac-1)) {
					if (nargs<nmac) {
						while (isspace(argp[-1]))	/* remove trailing whitespace */
							argp--;
						*argp++=0; s->value.s=argb; nargs++; argb=argp;
						s=s->next; continue;
					} else fatal("Too many args to macro");
				}
				if (argb!=argp || !isspace(lastch))	/* remove leading whitespace */
					*argp++=lastch;
				if (argp>=&args[350]) fatal("GETMAC/too_long");
			}
			if (tp->head) {	/* i.e. sc_vamac */
				while (isspace(argp[-1]))	/* remove trailing whitespace */
					argp--;
				*argp=0; s->value.s=argb;
				nargs++;				/* this var argument might appear to be "" */
			}
			if (nargs<nmac) fatal("Too few args to macro");
			lptrsav=lptr; lastchsav=lastch;
			if (lptr
			argp=sp->value.s;
			psptr=ps; concat_mode=0;
			while ((c=*argp++)) {
				if (isbegidch(c)) {
					int i=0; SYM *asp; char *str=lastsym;
					while (isidch(c)) {
						if (i<MAX_ID_LEN)
							lastsym[i]=c, i++;
						c=*argp++;
					}
					lastsym[i]=0;
					argp--;
					if ((asp = search(lastsym, -1, (HTABLE *)tp))) {
						str=asp->value.s;
						if (concat_mode && asp==s && sp->storage_class==sc_vamac && !*str
							&& psptr[-1]==',')
								psptr--;
					} else memexg(lastsym,lastid,MAX_ID_LEN+1);
					i=strlen(str);
					memcpy(psptr,str,i); psptr+=i;
					concat_mode=0;
				} else if (c=='#') {
					if (*argp!='#') {	/* single # */
						int i=0; SYM *asp; char *str=lastsym;
						if (!isbegidch(c=*argp++))
							fatal("# id : Syntax");
						while (isidch(c)) {
							if (i<MAX_ID_LEN)
								*str++=c, i++;
							c=*argp++;
						}
						*str=0;
						argp--;
						if (!(asp = search(lastsym, -1, (HTABLE *)tp)))
							fatal("# : arg not found");
						str=asp->value.s;
						*psptr++='"';
						while ((c=*str++)) {
							if (/*c=='\\' ||*/ c=='"') *psptr++='\\';	// ANSI specifies #x with x=\x7F should be "\x7F"...
							*psptr++=c;
						}
						*psptr++='"';
						concat_mode=0;
					} else {			/*  ##  */
						argp++;
						concat_mode=1;
					}
				} else if (!concat_mode || !isspace(c))
					concat_mode=0, *psptr++=c;
			}
			*psptr=0;
			q=ps;
			lptr-=strlen(q);	// se positionner avant le define
								//	(pas très catholique mais rapide)
			if ((p=lptr)<linein) fatal("EXPANDED LINE TOO LONG");
			while ((c=*q++)) *p++=c;
			goto quit;
#endif
		}
		q=sp->value.s;
		lptr-=strlen(q)+1;	// se positionner avant le define
							//	(pas très catholique mais rapide)
#ifdef OLD_MACRO
		if (tp) lptr--;
#endif
		if ((p=lptr)<linein) fatal("EXPANDED LINE TOO LONG");
		while ((c=*q++)) *p++=c;
#ifdef OLD_MACRO
		if (tp) *p++=' ',*p++=CHAR_POPMAC;
#endif
quit:
		getch();
		return 1;
	} else {
		lastst=id;
		/*if (preproc_stmt)*/ return 0;
/*		getcache(id);
		if (cached_sym==glue) {
			char b[MAX_ID_LEN]; int n;
			strcpy(b,lastid);
			n=strlen(b);
			getsym();	// flush cache
			getsym();	// get id
			if (lastst!=id) error(ERR_SYNTAX);
			else {
				memmove(lastid+n,lastid,MAX_ID_LEN-n);
				memcpy(lastid,b,n);
			}
			goto search_id;
		}*/
		return 0;
	}
}

#else

/*
 * expand_do(): replace the first 'len' characters of 'in' with the string 'q'.
 *
 * The resulting line's terminating zero will not exceed 'inbound' (an error is
 * raised otherwise), and the dualstack pointer will be updated if
 * 'need_ds_update' is non-zero.
 *
 * The tricky bit is that when DUAL_STACK is defined, [q,q+strlen(q)] can
 * overlap with [in,inbound[ (although it does not overlap with
 * [in,in+strlen(in)]).
 */
void expand_do(char *q,int len,char *in,char *inbound,int need_ds_update) {
	int newlen=strlen(q),n=newlen-len,r=inbound-in;
	unsigned int oldbytes=strlen(in)+1;
#ifdef DUAL_STACK
	// we don't care about the terminating zero
	char *q_overlapfree = alloca(newlen);
	memcpy(q_overlapfree,q,newlen);
	//printf("expand('%.*s'->'%s', '%s', %d)\n", len,in, q, in, need_ds_update);
#else
	char *q_overlapfree = q;
#endif
	if ((unsigned int)(oldbytes+n)>(unsigned int)r)
		uerrsys("line too long");
	if (n>0)
		memmove(in+n,in,oldbytes);
	else if (n<0)
		memmove(in,in-n,oldbytes+n);
	memcpy(in,q_overlapfree,newlen);
#ifdef DUAL_STACK
	//printf("  -> '%s'\n", in);
	if (need_ds_update)
		ds_update(in+(oldbytes+n));
#endif
}

void macro_expansion(char *in,char *inbound,int need_ds_update);

/*
 * macro_expand(): expand identifier 'id', and depending on the status of 'id':
 * - if 'id' is not a macro, do nothing and return 0
 * - if 'id' is an identifier to be protected from expansion, modify 'oldin'
 *   in-place to prepend an '@@' and return 0
 * - otherwise, concatenate the expansion of 'id' with the contents of 'in',
 *   place the result in 'oldin', and return 1
 *
 * The resulting line's terminating zero will not exceed 'inbound' (an error is
 * raised otherwise), and the dualstack pointer will be updated if
 * 'need_ds_update' is non-zero.
 *
 * The relation between 'in' and 'oldin' should fit one of these scenarios:
 * - in == oldin+strlen(id)  &&  !strncmp(id,oldin,strlen(id))
 * - in == oldin, and 'id' must not be protected from expansion
 */
int macro_expand(char *id,int crc/* may equal -1 */,char *in,char *oldin,char *inbound,int need_ds_update) {
  restart: {
	SYM *sp = search(id, crc, &defsyms);
/*	if ((long)sp==0x7c2040 && asm_flag)
		bkpt();*/
	if (!sp) {
#ifdef MEXP_SUPPORTS_PCH
		int n=pchsearch(id,PCHS_ADD);
		if (n>0)
			goto restart;
#endif
		return 0;
#ifdef PC
	} else if ((sp->used&PPSYM_UNDER_EXPANSION)) {
#else
	} else if ((short)sp->used<0) {
#endif
		expand_do("@@",0,oldin,inbound,need_ds_update);
		return 0;
	} else if (asm_flag && (sp->used&PPSYM_ASM_KEYWORD) && !(asm_xflag&&!asm_zflag)) {
		// for the few ASM keywords (e.g. move, b, l, d7) we don't want to perform expansion, unless:
		// - we are currently parsing an asm operand (asm_xflag!=0)
		// - *and* we are in __c__ mode (asm_zflag==0)
		return 0;
	} else {
		TABLE *tp;
		if ((tp=(TABLE *)sp->tp)!=NULL) {
			int d=0,nmac=(int)(char)sp->used,nargs=0,concat_mode;
			int string=0,number=0,asm_st=0;
#ifdef DUAL_STACK
			void *pop = ds_ensure(800+350);
			char *args = ds_var(char *);
			char *ps;
#else
			char ps[800],args[350];
#endif
			char *argb=args,*argp=args;
			char c,lastsym[MAX_ID_LEN+1],*psptr;
			SYM *s=tp->head;
			while (isspace((c=*in++)));
			if (c!='(') return 0;
			sp->used|=PPSYM_UNDER_EXPANSION;
			while (1) {
				c=*in++;
				if (c=='(') d++;
				else if (c==')') { if (--d<0) break; }
				else if (c=='"') {
					*argp++=c;
					while ((c=*in++) && c!='"') {
						if (argp>=&args[350-3]) uerrsys("macro arguments too long");
						*argp++=c;
						if (c=='\\')	// normally never followed by a 0, since otherwise would be a line continuation
							*argp++=*in++;
					}
					// c is now '"' or 0
				} else if (c==',' && !d && (sp->storage_class!=sc_vamac || nargs<nmac-1)) {
					if (nargs<nmac) {
						while (isspace(argp[-1]))	/* remove trailing whitespace */
							argp--;
						*argp++=0; s->value.s=argb; nargs++; argb=argp;
						s=s->next; continue;
					} else
						uerrc("too many args to macro");
				}
				if (argb!=argp || !isspace(c))	/* remove leading whitespace */
					*argp++=c;
				if (argp>=&args[350]) uerrsys("macro arguments too long");
			}
			if (tp->head) {
				while (isspace(argp[-1]))	/* remove trailing whitespace */
					argp--;
				*argp=0;
				if (!s) uerrc("too many args to macro");
				s->value.s=argb;
				nargs++;				/* if var, this argument might appear to be "" */
			}
			if (nargs<nmac) uerrc("too few args to macro");
#ifdef DUAL_STACK
			ds_update(argp+1);
			ps = ds_var(char *);
#endif
			argp=sp->value.s;
			psptr=ps; concat_mode=0;
#ifdef ASM
			if (asm_flag && !(sp->used&PPSYM_DEFINED_IN_ASM) && !(asm_xflag&&!asm_zflag)) {
				// unless we're already in __c__ mode, transform C defines to
				// '(unsigned long)__c__(normal_value)'
				// (note: we assimilate !asm_xflag to non-__c__ mode, because
				// otherwise getsym caching causes problems)
				//
				// IMPORTANT: this code has a counterpart a few lines below,
				// don't forget to keep it in sync
				strcpy(psptr,"(unsigned long)__c__(");
				psptr+=strlen(psptr);
			}
#endif
			while ((c=*argp++)) {
				if (!isidch(c)) number=0;
				if (c=='\\') { *psptr++=c; c=*argp++; if (!c) break; goto write; }
				else if (c=='"') { string=~string; goto write; }
				else if (c=='{') { if (asm_st<0) asm_st=100,asm_flag++; goto write; }
				else if (c=='}') { if (asm_st>0) asm_st=0,asm_flag--; goto write; }
				else if (c>='0' && c<='9' && !string) { number=1; goto write; }
				else if (isbegidch(c) && !string && !number) {
					int i=0; SYM *asp; char *str=lastsym;
					while (isidch(c)) {
						if (i<MAX_ID_LEN)
							lastsym[i]=c, i++;
						c=*argp++;
					}
					lastsym[i]=0;
					argp--;
					if (lastsym[0]=='a' && lastsym[2]=='m' && !lastsym[3] && lastsym[1]=='s')
						asm_st--;
					if ((asp = search(lastsym, -1, (HTABLE *)tp)))
						str=asp->value.s;
					strcpy(psptr,str);
#ifdef NONSTD_MACRO_CONCAT
					#ifdef DUAL_STACK
					#error fixme
					#endif
					if (asp) macro_expansion(psptr,&ps[800],1);
#endif
#ifdef STD_MACRO_CONCAT
					if (!concat_mode) {
						char *p=argp;
						while (isspace(*p)) p++;
						if (!(*p=='#' && p[1]=='#')) {
					#ifdef DUAL_STACK
							ds_update(psptr+strlen(psptr)+1);
					#endif
							macro_expansion(psptr,&ps[800],1);
						}
					}
#endif
					psptr+=strlen(psptr);
					concat_mode=0;
				} else if (c=='#' && !string) {
					if (*argp!='#') {	/* single # */
						int i=0; SYM *asp; char *str=lastsym;
						char *argpsave=argp;
						while (isspace(*argp)) argp++;
						if (!isbegidch(c=*argp++))
//							uerrc("# : syntax");
							{ c='#'; argp=argpsave; goto write; }
								/* (because of immediate data in ASM macros) */
						while (isidch(c)) {
							if (i<MAX_ID_LEN)
								*str++=c, i++;
							c=*argp++;
						}
						*str=0;
						argp--;
						if (!(asp = search(lastsym, -1, (HTABLE *)tp)))
//							uerrc("# : arg not found");
							{ c='#'; argp=argpsave; goto write; }
								/* (because of immediate data in ASM macros) */
						str=asp->value.s;
						*psptr++='"';
						while ((c=*str++)) {
							if (/*c=='\\' ||*/ c=='"') *psptr++='\\';	// ANSI specifies #x with x=\x7F should be "\x7F"...
							*psptr++=c;
						}
						*psptr++='"';
						concat_mode=0;
					} else {			/*  ##  */
						argp++;
						concat_mode=1;
						while (isspace(psptr[-1])) psptr--;
					}
				} else if (!concat_mode || !isspace(c)) {
				  write:
					concat_mode=0, *psptr++=c;
				}
			}
			if (asm_st>0) asm_flag--;
			if (string) uerrc("unbalanced quote");
#ifdef ASM
			if (asm_flag && !(sp->used&PPSYM_DEFINED_IN_ASM) && !(asm_xflag&&!asm_zflag)) {
				*psptr++=')';
			}
#endif
			*psptr=0;
#ifdef DUAL_STACK
			ds_update(psptr+1);
#endif
#if defined(NONSTD_MACRO_CONCAT_V2) || defined(STD_MACRO_CONCAT)
			macro_expansion(ps,&ps[800],1);
#endif
			expand_do(ps,in-oldin,oldin,inbound,need_ds_update);
#ifdef DUAL_STACK
			/*
			 * Note: this is slightly tricky, as expand_do() may overwrite part
			 * of 'ps', 'args', and stuff like that. But it will not do
			 * anything wrong, because [in,inbound[ was reserved by the caller,
			 * and we are not writing outside of it.
			 *
			 * In fact, the key to this working is that we can postpone any
			 * ds_pop()ing operation, as long as we do not ds_ensure() or
			 * ds_pop() anything else during that time interval, and as long as
			 * we back up the correct dualstack after the ds_pop(). This
			 * wouldn't be true if for example ds_pop() relied on reading some
			 * backup data from the dualstack and this backup data could happen
			 * to be in a previously ds_ensure()d zone.
			 */
			{
				// in fact, this is the caller dualstack only if
				// 'need_ds_update' made 'expand_do' update dualstack,
				// so we recall it only in this case.
				void *caller_dualstack = dualstack;
				ds_pop(pop);
				if (need_ds_update)
					ds_update(caller_dualstack);
			}
#endif
		} else
			sp->used|=PPSYM_UNDER_EXPANSION, expand_do(sp->value.s,in-oldin,oldin,inbound,need_ds_update);
		sp->used&=~PPSYM_UNDER_EXPANSION;
		return 1;
	}
  }
}
/*
 * macro_expansion(): scan the string starting from 'in', expanding any
 * identifiers.
 * Modifies 'in' in-place, but only by calling macro_expand(), which in turn
 * will only modify it through expand_do(). The dualstack size will be updated
 * when 'need_ds_update' is non-zero.
 */
void macro_expansion(char *in,char *inbound,int need_ds_update) {
	char c,*p=in; char newid[MAX_ID_LEN+1]; int string=0,number=0;
	int asm_st=0;
	while ((c=*p++)) {
		if (((c>='0' && c<='9') || c=='@') && !string) number=1;/* so that @@id isn't expanded */
		else if (!string && isbegidch(c)) {
			if (!number) {
				int i=0; char *begin=p-1;
				while (isidch(c)) {
					if (i<MAX_ID_LEN)
						newid[i]=c, i++;
					c=*p++;
				}
				newid[i]=0;
				p--;
				if (newid[0]=='a' && newid[2]=='m' && !newid[3] && newid[1]=='s')
					asm_st--;
				if (macro_expand(newid,-1,p,begin,inbound,need_ds_update)) p=begin;
			}	/* otherwise don't do anything! (0xa with #define xa foo shouldn't do 0foo) */
		} else if (c=='\\') p++;
		else {
			number=0;
			if (c=='"') string=~string;
			else if (c=='{' && asm_st<0) asm_st=100,asm_flag++;
			else if (c=='}' && asm_st>0) asm_st=0,asm_flag--;
		}
	}
	if (asm_st>0) asm_flag--;
	if (string) uerrc("unmatched quote in macro");
}
int getid(int c) {
	register int	i;
	int res;
	if (c>=0) {
		lastid[0] = c;
		i = 1;
		while (isidch(lastch)) {
			if (i < MAX_ID_LEN)
				lastid[i++] = lastch;
			getch();
		}
		lastid[i] = '\0';
	}
#ifdef PCH
	if (!lastid[1] && lastid[0]=='_')
		memcpy(lastid,curpch,MAX_ID_LEN+1);
#endif
	lastcrc=crcN(lastid);
	/*if (!strcmp(lastid,"OSContrastUp"))
		bkpt();*/
	*--lptr=lastch;
//	balance_parenthesis(lptr);
	/*if (!strcmp(lastid,"put_sprite_24"))
		printf("gjioh");*/
	if (!(res=macro_expand(lastid,lastcrc,lptr,lptr,linemax,0)))
		lastst=id;
	getch();
	return res;
}
#endif

/*
 * getsch - get a character in a quoted string.
 *
 * this routine handles all of the escape mechanisms for characters in strings
 * and character constants.
 * flag is 0, if a character constant is being scanned,
 * flag is 1, if a string constant is being scanned
 */
int getsch(int flag) {
	/* flag = "return an in-quote character?" */
	register int	i, j;
/*
 * if we scan a string constant, stop if '"' is seen
 */
	if (flag && lastch == '"') {
		getch();
		if (!preproc_stmt) {
			if (getcache(sconst)) {
				getch();
				return getsch(1);
			}
		}
		return -1;
	}
	if (lastch != '\\') {
		i = lastch;
		getch();
		return i;
	}
	getch();					/* get an escaped character */
	if (isdigit(lastch)) {
		i = 0;
		for (j = 0; j < 3; ++j) {
			if (isdigit(lastch) && lastch <= '7')
				i = 8*i + radix36(lastch);
			else
				break;
			getch();
		}
//		/* signed characters lie in the range -128..127 => shit so disabled */
//		if ((i &= 0377) >= 128) i -= 256;
		return i;
	}
	i = lastch;
	getch();
	switch (i) {
	  case '\n':
		return getsch(flag);
	  case 'b':
		return '\b';
	  case 'f':
		return '\f';
	  case 'n':
		return '\n';
	  case 'r':
		return '\r';
	  case 't':
		return '\t';
	  case 'a':
		return '\a';
	  case 'v':
		return '\v';
	  case 'e':
		return '\x1B';
	  case 'x':
		if ((i=radix36(lastch))<0 || i>=16)
			error(ERR_SYNTAX);
		getch();
		if ((j=radix36(lastch))>=0 && j<16) {
			i = 16*i + j;
			getch();
		}
//		/* signed characters lie in the range -128..127 => shit so disabled */
//		if (i >= 128) i -= 256;
		return i;
	  default:
		uwarn("'%c' isn't an escaped character",i);
		/* FALL THROUGH */
	  case '\\': case '"': case '\'':
		return i;
	}
}

int radix36(char c) {
/*
 * This function makes assumptions about the character codes.
 */
	if (isdigit(c))
		return c - '0';
	if (c >= 'a' && c <= 'z')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'Z')
		return c - 'A' + 10;
	return -1;
}
#ifndef AS	/* otherwise in out68k_as.c */
int hexatoi(char *s) {
	int x=0;
	if (strlen(s)>3) return -1;
	while (*s) {
		int y=radix36(*s++);
		if (y<0) return -1;
		x<<=4; x+=y;
	}
	return x;
}
#endif

static void test_int(int hex_or_octal) {
/* test on long or unsigned constants */
	if (lastch == 'l' || lastch == 'L') {
		getch();
		lastst = lconst;
		return;
	}
	if (lastch == 'u' || lastch == 'U') {
		/*if (!hex_or_octal)  -> why was it so??? */
			getch();
		lastst = uconst;
		if (lastch=='l' || lastch=='L')
			getch(), lastst = ulconst;
		if (short_option && ival > 65535)
			lastst = ulconst;
		return;
	}
	if ((long)ival<0) {			/* since '-' is treated as unary minus, this only */
		lastst = ulconst;		/*  happens for values that are >0x7FFFFFFF       */
		return;
	}

	/* -32768 thus is stored as (unary minus)(32768l) */
/*
 * Although I think it is not correct, octal or hexadecimal
 * constants in the range 0x8000 ... 0xffff are unsigned in 16-bit mode
 */
	if (short_option) {
		if (hex_or_octal && ival > 32767 && ival < 65536) {
			lastst = uconst;
			return;
		}
		if (ival > 32767) {
			/* -32768 thus is stored as (unary minus)(32768l) */
			lastst = lconst;
			return;
		}
	}
}

/*
 * getbase - get an integer in any base.
 *  b==0 : base 10
 *  b!=0 : base 1<<b (1,3,4)
 */
static void getbase(int b) {
	/*
	 * rval is computed simultaneously - this handles floating point
	 * constants whose integer part is greater than INT_MAX correctly, e. g.
	 * 10000000000000000000000.0
	 */
	register unsigned long i=0;
	register int	j;
#ifndef NOFLOAT
	register double r = 0;
#endif

	if (!b) {
		while (isdigit(lastch)) {
			j = radix36(lastch);
			i = 8 * i + 2 * i + j;
#ifndef NOFLOAT
#ifdef PC
			r = 10.0 * r + (double) j;
#else
			r = ffpadd(ffpmul(FFP_TEN, r), ffputof(j));
#endif
#endif
			getch();
		}
	} else {
		while (isalnum(lastch)) {
			if (!((j = radix36(lastch)) >> b)) {
				i = (i << b) + j;
				getch();
			} else
				break;
		}
	}
	ival = i;
#ifndef NOFLOAT
	rval = r;
#endif
	lastst = iconst;
}

#ifndef NOFLOAT
/*
 * getfrac - get fraction part of a floating number.
 */
static void getfrac() {
	double frmul;
#ifdef PC
	frmul = 0.1;
#else
	frmul = FFP_TENINV;
#endif
	while (isdigit(lastch)) {
#ifdef PC
		rval += frmul * radix36(lastch);
#else
		rval = ffpadd(rval, ffpmul(frmul, ffputof(radix36(lastch))));
#endif
		getch();
#ifdef PC
		frmul *= 0.1;
#else
		frmul = ffpmul(frmul, FFP_TENINV);
#endif
	}
}

/*
 * getexp - get exponent part of floating number.
 *
 * this algorithm is primative but usefull.  Floating exponents are limited to
 * +/-255 but most hardware won't support more anyway.
 */
static void getexp() {
	double			expo, exmul;
	expo = rval;
	if (lastch == '-') {
#ifdef PC
		exmul = 0.1;
#else
		exmul = FFP_TENINV;
#endif
		getch();
	} else
#ifdef PC
		exmul = 10.0;
#else
		exmul = FFP_TEN;
#endif
	getbase(0);
	lastst = rconst;
	while (ival--)
#ifdef PC
		expo *= exmul;
#else
		expo = ffpmul(expo, exmul);
#endif
	rval = expo;
}
#endif

/*
 * getnum - get a number from input.
 *
 * getnum handles all of the numeric input. it accepts decimal, octal,
 * binary, hexadecimal, and floating point numbers.
 */
static void getnum() {
		if (lastch == '0') {
				getch();
				if (lastch == 'x' || lastch == 'X') {
						getch();
						getbase(4);
						test_int(1);
						return;
				}
				if (lastch == 'b' || lastch == 'B') {
						getch();
						getbase(1);
						test_int(1);
						return;
				}
				if (lastch != '.' && lastst != 'e' && lastst != 'E') {
						getbase(3);
						test_int(1);
						return;
				}
		}
		getbase(0);
#ifndef NOFLOAT
		if (lastch == '.') {
				/* rval already set */
				getch();
				getfrac();				/* add the fractional part */
				lastst = rconst;
		}
		if (lastch == 'e' || lastch == 'E') {
				getch();
				getexp();				/* get the exponent */
		}
#endif
		/*
		* look for l and u qualifiers
		*/
		if (lastst == iconst)
				test_int(0);
	/*
				* look for (but ignore) f and l qualifiers
		*/
		if (lastst == rconst)
				if (lastch == 'f' || lastch == 'F' || lastch == 'l' ||
						lastch == 'L')
						getch();
}

#ifdef PCH
#ifndef PC
#include "sUnpack.c"
#endif
char sUnpackBuf[500];
int pchload(char *name,char *data,unsigned int flags,unsigned char *tabp, unsigned char *p0,TI_SHORT *extTab) {
	struct sym *sp = 0;
	int r=0;
	char na[MAX_ID_LEN+1];
	char oldpch[MAX_ID_LEN+1];
#ifdef MIN_TEMP_MEM
	int old_min_temp_mem=min_temp_mem;
	min_temp_mem=temp_mem;
#endif
	(*tabp)++;
/*	if (!strcmp(name,"compare_t"))
		bkpt();*/
	if (*data || (flags&PCHID_MACRO)) {		// is there a #define ?
		int n=0;
#ifndef LOWMEM
		global_flag++;
#endif
		sp = (SYM *)xalloc((int)sizeof(SYM), _SYM+DODEFINE);
/*		if ((long)sp==0x789388)
			bkpt();*/
		sp->name = strsave(name);
		sp->storage_class = sc_define;
#ifdef NO_CALLOC
		sp->tp=NULL;
#endif
		if (flags&PCHID_MACRO) {
			TABLE *tp;
			sp->tp=(TYP *)(tp=(TABLE *)xalloc((int)sizeof(TABLE), _TABLE+DODEFINE));
	#ifdef NO_CALLOC
			tp->hash=0;
			tp->head=tp->tail=NULL;
	#endif
			if (*data) {
				while (*data) {
					SYM *arg;
					n++;
					arg=(SYM *)xalloc((int)sizeof(SYM), _SYM+DODEFINE);
					arg->name=data;
	#ifdef NO_CALLOC
					arg->tp=NULL;
	#endif
					insert(arg,(HTABLE *)tp);
					while (*data++);
				}
				if (flags&PCHID_VAMAC) sp->storage_class = sc_vamac;
			}
			data++;
		}
		if (flags&PCHID_PACKED)
			sp->value.s = strsave(sUnpack(data,sUnpackBuf,p0+w2us(((PCH_HEAD *)p0)->dic_off)));
		else sp->value.s = data;
		insert(sp,&defsyms);
		sp->used=n;
#ifndef LOWMEM
		global_flag--;
#endif
		r=1;
	}
	while (*data++);
	if (*data) {
		TYP *head0=head,*tail0=tail;
		char *declid0=declid;
		int nparms0=nparms;
		char *names0[MAX_PARAMS];
		int bit_offset0=bit_offset, bit_width0=bit_width, bit_next0=bit_next;
		int decl1_level0=decl1_level,pch_init0=pch_init;
		char lptr_save[LINE_LENGTH]; int l=strlen(data); // TODO: use DUAL_STACK for saving lptr
		struct enode *old_init_node=init_node;
		int old_middle_decl=middle_decl;
		int lcrc=lastcrc;
#ifdef MEXP_SUPPORTS_PCH
		char *old_lptr=lptr;
		char *old_linemin=linemin,*old_linemax=linemax;
#endif
		memcpy(names0,names,MAX_PARAMS*sizeof(char *));
		memcpy(na,lastid,MAX_ID_LEN+1);
		memcpy(oldpch,curpch,MAX_ID_LEN+1);
		memcpy(curpch,name,MAX_ID_LEN+1);
#ifdef MEXP_SUPPORTS_PCH
		lptr=lptr_save+1;
		linemin=lptr_save,linemax=lptr_save+LINE_LENGTH;
#else
		memcpy(lptr_save,lptr-1,LINE_LENGTH);
#endif
		pch_init=1;
		lastch=' ';
		head=tail=0;
		init_node=0;
		middle_decl=0;
		decl1_level=0;
#ifndef MEXP_SUPPORTS_PCH
#ifdef OLD_MACRO
		lptr=linein+MAX_MAC_LEN+1;
#else
		lptr=linein+1;
#endif
#endif
		sUnpack(data,lptr,p0+w2us(((PCH_HEAD *)p0)->dic_off));
		l=strlen(lptr);
/*		memcpy(lptr,data,l);*/
		memcpy(lptr+l," alloca  ",10);	/* the spaces are the end are there because we don't */
		global_flag++;					/*  want to read a new line (otherwise bugs)         */
		getsym();
		while (lastst != kw_alloca) {
			dodecl(sc_global);
			if (lastst != kw_alloca)
				getsym();
		}
		global_flag--;
#ifdef MEXP_SUPPORTS_PCH
		lptr=old_lptr-1;	/* so that it re-reads the last char */
		linemin=old_linemin,linemax=old_linemax;
#else
#ifdef OLD_MACRO
		lptr=linein+MAX_MAC_LEN+1;
#else
		lptr=linein+1;
#endif
		memcpy(lptr,lptr_save,LINE_LENGTH);	/* so that it re-reads the last char */
#endif
		getch();
		lastst = id;
		memcpy(lastid,na,MAX_ID_LEN+1);
		memcpy(curpch,oldpch,MAX_ID_LEN+1);
		init_node=old_init_node;
		middle_decl=old_middle_decl;
		head=head0; tail=tail0;
		pch_init=pch_init0;
		declid=declid0;
		nparms=nparms0;
		bit_offset=bit_offset0; bit_width=bit_width0; bit_next=bit_next0;
		decl1_level=decl1_level0;
		lastcrc=lcrc;
		memcpy(names,names0,MAX_PARAMS*sizeof(char *));
	}
#ifdef AS							/* .ext management is enabled only in AS mode */
	while (*data++);
	if (*data) {
		unsigned char *ext=p0+w2us(extTab[*data-1]);
		extscan(ext);				/* load .ext files used by current ext */
		extload(ext);				/* now write it :) */
	}
#endif
#ifndef MEXP_SUPPORTS_PCH
	if (r) getid(-1);
#endif
#ifndef MEXP_SUPPORTS_PCH
	(*tabp)--;
#endif
#ifdef MIN_TEMP_MEM
	min_temp_mem=old_min_temp_mem;
#endif
	return r;
}

int pchsearch(char *id,int mode) {	/* returns 0 if not PCH, 1 if PCH/def, -1 if PCH/init */
	int n=pchnum;
	/*if (!strcmp(id,"LINK_ME"))
		bkpt();*/
/*	if (!strcmp(id,"unpack"))
		bkpt();*/
	while (n--) {
		unsigned char *p0=pchdata[n],*tab=pchtab[n],*tabp,*p,c,*q;
		TI_SHORT *extTab=(TI_SHORT *)(p0+w2us(pchhead[n]->ext_off));
		int z=PCH_HEAD_SIZE;
		do {
			p=p0+z; q=id;
			while ((c=*p++) && c==*q++);
			if (!c && !*q) {
				int idnum=(p[4]<<8)+p[5];
				tabp=&tab[idnum&PCHID_MASK];
				if (*tabp)			/* really quit, because it would be a mess   */
					return 0;		/*  if we scanned other files for this ID ;) */
				if (mode>0) {		/* mode==PCHS_ADD */
#ifdef REQ_MGT
					pchrequired[n]=1;
#endif
					return pchload(id,p+6,idnum,tabp,p0,extTab)?1:-1;
				} else			/* mode==PCHS_UNLOAD */
					(*tabp)++;		/* forbid the loading of this symbol */
			}
			if (c) while (*p++);
			if (c<q[-1]) p+=2;
			z=(*p++)<<8; z+=*p;
		} while (z);
	}
	return 0;
}
#endif

/*
 * getsym - get next symbol from input stream.
 *
 * getsym is the basic lexical analyzer.
 * It builds basic tokens out of the characters on the input stream
 * and sets the following global variables:
 *
 * lastch:		A look behind buffer.
 * lastst:		type of last symbol read.
 * laststr: 	last string constant read.
 * lastid:		last identifier read.
 * ival:		last integer constant read.
 * rval:		last real constant read.
 *
 * getsym should be called for all your input needs...
 */

void getsym() {
		register int	i, j;
		got_sym = 1;
		prevlineid = lineid;
		if ((int)cached_sym IS_VALID) {
			lastst=cached_sym; lineid=cached_lineid; cached_sym=-1;
			return;
		}
restart:						/* we come back here after comments */
		skipspace();
		lineid = lineno;
		if (lastch IS_INVALID)
				lastst = eof;
		else if (isdigit(lastch))
				getnum();
		else if (isidch(lastch)) {
				if (forbid==(lastst=id)) return;
				i=lastch; getch();
				if (getid(i)) goto restart;
		} else
				switch (lastch) {
				case '+':
						getch();
						if (lastch == '+') {
								getch();
								lastst = autoinc;
						} else if (lastch == '=') {
								getch();
								lastst = asplus;
						} else
								lastst = plus;
						break;
				case '-':
						getch();
						if (lastch == '-') {
								getch();
								lastst = autodec;
						} else if (lastch == '=') {
								getch();
								lastst = asminus;
						} else if (lastch == '>') {
								getch();
								lastst = pointsto;
						} else
								lastst = minus;
						break;
				case '*':
						getch();
						if (lastch == '=') {
								getch();
								lastst = astimes;
						} else
								lastst = star;
						break;
				case '/':
						getch();
						if (lastch == '=') {
								getch();
								lastst = asdivide;
						/*} else if (lastch == '*') {
								getch();
								for (;;) {
										if (lastch == '*') {
												getch();
												if (lastch == '/') {
														getch();
														goto restart;
												}
										} else getch();
								}
						} else if (lastch == '/') {
								getch();
								for (;;) {
										if (lastch == '\n') {
												getch();
												goto restart;
										} else getch();
								}*/
						} else
								lastst = divide;
						break;
				case '^':
						getch();
						if (lastch == '=') {
								getch();
								lastst = asuparrow;
						} else
								lastst = uparrow;
						break;
				case ';':
						getch();
						lastst = semicolon;
						break;
				case ':':
						getch();
						lastst = colon;
						break;
				case '=':
						getch();
						if (lastch == '=') {
								getch();
								lastst = eq;
						} else
								lastst = assign;
						break;
				case '>':
						getch();
						if (lastch == '=') {
								getch();
								lastst = geq;
						} else if (lastch == '>') {
								getch();
								if (lastch == '=') {
										getch();
										lastst = asrshift;
								} else
										lastst = rshift;
						} else
								lastst = gt;
						break;
				case '<':
						getch();
						if (lastch == '=') {
								getch();
								lastst = leq;
						} else if (lastch == '<') {
								getch();
								if (lastch == '=') {
										getch();
										lastst = aslshift;
								} else
										lastst = lshift;
						} else
								lastst = lt;
						break;
				case '\'':
						getch();
						ival = getsch(0);		/* get a string char */
						if (lastch != '\'')
								error(ERR_SYNTAX);
						else
								getch();
						lastst = iconst;
						break;
				case '\"':
					if (forbid!=sconst) {
						getch_in_string=1;
						getch();
						for (i = 0; ; ++i) {
								if ((j = getsch(1)) IS_INVALID)
										break;
								if (i < MAX_STRLEN)
										laststr[i] = j;
						}
						getch_in_string=0;
						/*
			 * Attention: laststr may contain zeroes!
			 */
						if (i > MAX_STRLEN) {
								i = MAX_STRLEN;
								uwarn("string constant too long");
						}
						lstrlen = i;
						laststr[i] = 0;
					}
					lastst = sconst;
					break;
				case '!':
						getch();
						if (lastch == '=') {
								getch();
								lastst = neq;
						} else
								lastst = not;
						break;
				case '%':
						getch();
						if (lastch == '=') {
								getch();
								lastst = asmodop;
						} else
								lastst = modop;
						break;
				case '~':
						getch();
						lastst = compl;
						break;
				case '.':
						getch();
						if (isdigit(lastch)) {
#ifndef NOFLOAT
							rval = 0;
							getfrac();
#endif
							lastst = rconst;
							if (lastch=='e' || lastch=='E') {
								getch();
#ifndef NOFLOAT
								getexp();
#endif
							}
						} else if (lastch=='.') {
							getch();
							if (lastch!='.') error(ERR_SYNTAX);
							getch();
							lastst = dots;
						} else
							lastst = dot;
						break;
				case ',':
						getch();
						lastst = comma;
						break;
				case '&':
						getch();
						if (lastch == '&') {
								lastst = land;
								getch();
						} else if (lastch == '=') {
								lastst = asand;
								getch();
						} else
								lastst = and;
						break;
				case '|':
						getch();
						if (lastch == '|') {
								lastst = lor;
								getch();
						} else if (lastch == '=') {
								lastst = asor;
								getch();
						} else
								lastst = or;
						break;
				case '(':
						getch();
#ifdef ASM
#ifdef OLD_AMODE_INPUT
						if (asm_xflag
							&& ((lastch=='p' && *lptr=='c')
								|| (lastch=='s' && *lptr=='p')
								|| (lastch=='a' && *lptr>='0' && *lptr<='7'))
							&& (lptr[1]==')' || lptr[1]==','))
								lastst = kw_offs_end;
						else
#else
						if (asm_xflag
							&& ((lastch=='p' && *lptr=='c')
								|| (lastch=='s' && *lptr=='p')
								|| (lastch=='a' && *lptr>='0' && *lptr<='7'))
							&& (lptr[1]==')' || lptr[1]==','))
								lastst_flag=1;
						else	lastst_flag=0;
#endif
#endif
							lastst = openpa;
						break;
				case ')':
						getch();
						lastst = closepa;
						break;
				case '[':
						getch();
						lastst = openbr;
						break;
				case ']':
						getch();
						lastst = closebr;
						break;
				case '{':
						getch();
						lastst = begin;
						break;
				case '}':
						getch();
						lastst = end;
						break;
				case '?':
						getch();
						lastst = hook;
						break;
				case '#':
						getch();
						// TODO: rewrite the following (disabled because not ANSI-compliant)
						/*getsym();
						if (lastst==id) {
							SYM *sp;
							if (mac_sp!=&mac_stk[MAX_MAC_NUM] &&
								(sp = search(lastid, lastcrc, (HTABLE *)*mac_sp))) {
								char c,*q=sp->value.s; char buf[MAX_STRLEN],*bp; int i;
								lastst=sconst;
								bp=buf;
								*bp++='"';
								i=MAX_STRLEN-1-2;
								do {
									if (!(c=*q++)) break;
									if (c=='"') { *bp++='\\'; *bp++='"'; }
									else *bp++=c;
								} while (i--);
								*bp++='"';
								lptr-=bp-buf+1;
								memcpy(lptr,buf,bp-buf);
								getch();
							} else error(ERR_UNDEFINED);
						} else if (lastst==lconst||lastst==iconst||lastst==uconst) {
								char buf[MAX_STRLEN],*bp;
								lastst=sconst;
								bp=buf;
								*bp++='"';
								sprintf(bp,"%ld",ival);
								bp+=strlen(bp);
								*bp++='"';
								lptr-=bp-buf+1;
								memcpy(lptr,buf,bp-buf);
								getch();
						} else error(ERR_IDEXPECT);
						getsym();*/
#ifdef ASM
						if (asm_flag) {
							lastst = sharp;
							break;
						} else {
#endif
							error(ERR_ILLCHAR);
							goto restart;	/* get a real token */
#ifdef ASM
						}
#endif
/*						if (lastst==iconst || lastst==lconst || lastst==uconst) {
							lastst=sconst;
							if (forbid!=lastst) sprintf(laststr,"%ld",ival);
							else {
								char b[20]; int n;
								sprintf(b,"%ld",ival);
								lptr-=1+(n=strlen(b));
								strncpy(lptr,b,n);
								lptr[n]='"';
							}
						} else error(ERR_INTEGER);*/
						break;
#ifdef ASM
				case '\\':
						if (asm_flag) {
							if (forbid==(lastst=id)) return;
							getch();
							if (getid((int)'\\')) goto restart;
						} else {
							getch();
							error(ERR_ILLCHAR);
							goto restart;	/* get a real token */
						}
						break;
#endif
				case '@':
						getch();
						if (lastch=='@') {
							getch();
							if (isbegidch(lastch)) {
								if (forbid==(lastst=id)) return;
								getrawid();
								break;
							}
						}
						/* otherwise fall through */
				default:
						getch();
						error(ERR_ILLCHAR);
						goto restart;	/* get a real token */
		}
		if (lastst == id) {
			searchkw();
//#ifdef OLD_MACRO
			if (lastst == kw_eval) {
				struct enode *ep;
				getsym();
				needpunc(openpa);
				if (!expression(&ep)) error(ERR_EXPREXPECT);
				if (lastst!=closepa) needpunc(closepa);
				opt0(&ep);
				if (ep->nodetype==en_icon) {
					lastst=lconst;
					ival=ep->v.i;
#ifndef NOFLOAT
				} else if (ep->nodetype==en_fcon) {
					lastst=rconst;
					rval=ep->v.f;
#endif
				} else error(ERR_CONSTEXPECT);
			}
#ifdef ASM
			else if (asm_flag && lastst == id) {
				if (!lastid[2]) {
					if (lastid[1]>='0' && lastid[1]<='7') {
						lastreg=lastid[1]-'0';	// we don't care if we set lastreg even if
						if (lastid[0]=='d')		//  lastst is an id
							lastst=kw_dreg;		// (for example 'z6' will set lastreg to 6)
						else if (lastid[0]=='a')
							lastst=kw_areg;
					} else if (lastid[1]=='p' && lastid[0]=='s')
						lastst=kw_areg,lastreg=7;
				}
				asm_searchkw();
			}
#endif
#if defined(PCH) && !defined(MEXP_SUPPORTS_PCH)
			if (lastst==id && !(lastsp = search(lastid, lastcrc, &lsyms))
				 && !(lastsp = search(lastid, lastcrc, &gsyms))) {
#if 0
				int n=pchnum;
				while (n--) {
					unsigned char *p0=pchdata[n],*tab=pchtab[n],*tabp,*p,c,*q;
					TI_SHORT *extTab=(TI_SHORT *)(p0+w2s(pchhead[n]->ext_off));
					int z=PCH_HEAD_SIZE;
					do {
						p=p0+z; q=lastid;
						while ((c=*p++) && c==*q++);
						if (!c && !*q) {
							tabp=&tab[(p[4]<<8)+p[5]];
							if (*tabp)			/* really quit, because it would be a mess   */
								goto pch_done;	/*  if we scanned other files for this ID ;) */
							if (pchload(p+6,tabp,p0,extTab)) goto restart;
							else {
								lastsp = gsearch(lastid, lastcrc);
								goto pch_done;
							}
						}
						if (c) while (*p++);
						if (c<q[-1]) p+=2;
						z=(*p++)<<8; z+=*p;
					} while (z);
				}
			  pch_done:
				(void)0;
#else
				int n=pchsearch(lastid,PCHS_ADD);
				if (n>0) goto restart;
				else if (n<0) lastsp = gsearch(lastid, lastcrc);
#endif
			}
#else
			if (lastst==id && !(lastsp = search(lastid, lastcrc, &lsyms)))
				 lastsp = search(lastid, lastcrc, &gsyms);
#endif
		}
/*		if (global_flag)
		if (lastst==id && !strcmp(lastid,"pos"))
			bkpt();*/
/*	if (lineid==0x51)
		bkpt();*/
}

int getcache(enum(e_sym) f) {
	enum(e_sym) my_st=lastst;
	int my_flag=lastst_flag;
	int my_line=lineid,my_prev=prevlineid;
	if (cached_sym!=-1) return (cached_sym==f);
	cached_sym=-2;	// tell getcache not to put anything into cache
	forbid=f;
	getsym();
	cached_sym=lastst;
	cached_flag=lastst_flag;
	cached_lineid=my_line;
	lastst=my_st;
	lastst_flag=my_flag;
	lineid=my_line;
	prevlineid=my_prev;
	forbid=-1;
	if (cached_sym==f) cached_sym=-1;
	return (cached_sym==-1);
/*	enum(e_sym) my_st=lastst,cached=cached_sym;
	if ((int)cached_sym2 IS_VALID) return (cached_sym2==f);
	cached_sym=-1;	// prevent getsym from caching :)
	forbid=f;
	getsym();
	forbid=-1;
	if ((int)cached IS_VALID) {
		if ((int)cached_sym2 IS_VALID) fatal("CACHE");
		cached_sym2=lastst;
		cached_sym=cached;
		lastst=my_st;
		if (cached_sym2==f) cached_sym2=-1;
		return (cached_sym2==f);
	} else {
		cached_sym=lastst;
		lastst=my_st;
		if (cached_sym==f) cached_sym=-1;
		return (cached_sym==f);
	}*/
}

void needpunc(enum(e_sym) p) {
	if (lastst == p)
		getsym();
	else
		uerr(ERR_PUNCT,
			p==semicolon?';':
			 (p==begin?'{':
			  (p==end?'}':
			   (p==openpa?'(':
			    (p==closepa?')':
			     (p==hook?'?':
			      (p==comma?',':
					  (p==closebr?']':' '))))))));
}

extern unsigned char sizeof_flag;
extern unsigned char id_are_zero;
void do_compile() {
	/* parser initialization */
	sizeof_flag=0; id_are_zero=0;
	flags=flags_basegtc;
#ifdef SPEED_OPT
	speed_opt_value = default_speed_opt_value;
#endif
	/* lexical analyzer / preprocessor initialization */
	initsym();
	/* compilation itself */
	compile();
}
#ifdef PCH
void closepch() {
#ifdef REQ_MGT
	{
		FILE *req_file=NULL;
		int i,needs_req=0;
		for (i=pchnum;i--;)
			if (pchrequired[i] && strcmp(pchname[i],"stdhead"))
				needs_req++;
		if (needs_req) {
			char buf[sizeof("[Req_v1]\n")+1];
			char temp[5000];
			*temp=0;
			req_file=fopen(proj_file,"r");
			if (req_file) {
				while (!feof(req_file)) {
					fgets(buf,sizeof("[Req_v1]\n"),req_file);
					strcat(temp,buf);
					if (!strcmp(buf,"[Req_v1]\n")) {
						needs_req=0;
						break;
					}
				}
				fclose(req_file);
			}
			req_file=fopen(proj_file,"w");
			if (!req_file) fatal("Could not open project file.");
			fputs(temp,req_file);
			if (needs_req)
				fputs("[Req_v1]\n",req_file);
			needs_req=1;
		}
#endif
	/* close PCH files */
	while (pchnum) {
		pchnum--;
#ifdef REQ_MGT
		if (pchrequired[pchnum] && needs_req)
			fprintf(req_file,"%s\n",pchname[pchnum]);
#endif
		fclose(pchfile[pchnum]);
	}
#ifdef REQ_MGT
		if (needs_req)
			fclose(req_file);
	}
#endif
}
#endif
// vim:ts=4:sw=4
