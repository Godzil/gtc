/*
 * GTools C compiler
 * =================
 * source file :
 * initialisations
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

long 	inittype(TYP *,TYP **);
static long 	initstruct(TYP *,TYP **), initarray(TYP *,TYP **), initunion(TYP *,TYP **);
static int		initchar(), initshort(), initlong(), initpointer();
#ifndef NOFLOAT
static int		initfloat();
#ifdef DOUBLE
static int		initdouble();
#endif
#endif
static struct enode *constexpr();
extern TYP *copy_type(TYP *s);

TYP *copy_type_global(TYP *tp) {
	TYP *tp2;
	temp_local++;
	global_flag++;
	tp2=copy_type(tp);
	global_flag--;
	temp_local--;
	return tp2;
}

void doinit(struct sym *sp, int align) {
	nl();
	if (lastst != assign)
		genstorage(sp, align);
	else {
		struct slit *strtab_old=strtab;
		int glob = global_flag;
		int no_locblk = !locblk;
/*		if (lineid>=0x100)
			bkpt();*/
		strtab = 0;
		global_flag = 0;
		tmp_use();
		temp_local++;
		global_strings++;
		dseg(); 				/* select data segment */
		put_align(align);
#ifndef AS
		if (sp->storage_class == sc_static)
			put_label((unsigned int) sp->value.i);
		else
			g_strlab(sp->name);
#else
#ifdef PC
#define gnu_hook(x,y) ((x)?(x):(y))
#else
#define gnu_hook(x,y) ((x)?:(y))
#endif
		if (sp->storage_class == sc_static) {
			extern int glblabel;
			put_label(gnu_hook(sp->value.splab,sp->value.splab=nxtglabel()));
		} else
			put_label(splbl(sp));
#endif
		getsym();
		(void) inittype(sp->tp,&sp->tp);
/*		if (strtab)
			bkpt();*/
		if (strtab)
			dumplits();
		global_strings--;
		temp_local--;
		tmp_free(); /* just in case, but shouldn't get used because of temp_local */
		if (no_locblk && locblk)
			rel_local();
		global_flag = glob;
		strtab = strtab_old;
	}
}

long inittype(TYP *tp,TYP **tpp) {
	int 			brace_seen = 0;
	long			nbytes;
	if (lastst == begin) {
		brace_seen = 1;
		getsym();
	}
	switch (tp->type) {
	  case bt_char:
	  case bt_uchar:
		nbytes = initchar();
		break;
	  case bt_short:
	  case bt_ushort:
		nbytes = initshort();
		break;
	  case bt_pointer:
		if (tp->val_flag)
			nbytes = initarray(tp,tpp);
		else {
			nbytes = initpointer();
		}
		break;
	  case bt_ulong:
	  case bt_long:
		nbytes = initlong();
		break;
#ifndef NOFLOAT
	  case bt_float:
		nbytes = initfloat();
		break;
#ifdef DOUBLE
	  case bt_double:
		nbytes = initdouble();
		break;
#endif
#endif
	  case bt_struct:
		nbytes = initstruct(tp,tpp);
		break;
	  case bt_union:
		nbytes = initunion(tp,tpp);
		break;
	  default:
		error(ERR_NOINIT);
		nbytes = 0;
	}
	if (brace_seen)
		needpunc(end);
	return nbytes;
}

/*void modf_btp(TYP *new_btp,TYP *tp) {
	tp=copy_type(tp);
	tp->btp=new_btp;
	modf(tp,modfp);
}*/

extern unsigned int pos;
static long initarray(TYP *tp,TYP **tpp) {
	long			nbytes;
	char		   *p;
	int 			len;
#ifdef AS
	unsigned int max_pos=pos;
#endif
//	tmp_use();
	nbytes = 0;
	if (lastst == sconst && (tp->btp->type == bt_char ||
							 tp->btp->type == bt_uchar)) {
		len = lstrlen;
		nbytes = len;
		p = laststr;
		while (len--)
			genbyte(*p++);
		if (!tp->size)	/* if tp->size!=0, then the padding stuff takes care of it */
			genbyte(0), nbytes++;
		while (nbytes < tp->size)
			genbyte(0), nbytes++;
		getsym();				/* skip sconst */
	} else if (lastst == kw_incbin) {
		FILE *fp; int size;
#ifndef PC
		unsigned char type;
#endif
/*		getsym();
		if (lastst!=sconst)
			error(ERR_SYNTAX);*/
		skipspace();
		if (lastch!='"')
			error(ERR_SYNTAX);
		getch();
		getidstr();
		if (lastch!='"')
			error(ERR_SYNTAX);
		getch();
		getsym();
		fp=fopen(lastid,"rb");
		if (!fp)
			uerr(ERR_CANTOPEN,lastid);
#ifdef PC
		p=malloc(100000);
		size=fread(p,1,100000,fp);
#else
		p=*(char **)fp;
		size=*(unsigned short *)p;
		type=p[size+1];
		if (type==0xF3 || type==0xE0) size--;
		else if (type==0xF8) { size--; do size--; while (p[size+1]); }
#endif
		if (tp->size && size>tp->size) size=tp->size;
		nbytes=size;
		while (size--) genbyte(*p++);
		while (nbytes<tp->size) {
			genbyte(0);
			nbytes++;
		}
	} else if (lastst == end)
		goto pad;
	else {
		int i,additional,size;
		for (i=0;;) {
			additional=0;
			if (lastst == openbr) {
				getsym(); {
				int j=intexpr();
				long diff=tp->btp->size*(j-i);
				if (lastst == dots) {
					getsym();
					additional=intexpr()-j;
				}
				if (lastst != closebr) error(ERR_SYNTAX);
				else {
					getsym();
					if (lastst == assign) getsym();
					if (i<j) {
						nbytes += diff;
						while (diff--)
							genbyte(0);
						i=j;
					} else if (j<i) {
	#ifdef AS
						move_pos(diff); nbytes+=diff; i=j;
	#else
	//					uwarn("brackets not supported in non-AS mode");
	#endif
					}
				}
			}}
			if (additional>=0) {
				nbytes += (size=inittype(tp->btp,NULL)); i++;
#ifdef AS
				while (additional--) rewrite(size), nbytes+=size, i++;
#endif
			}
	#ifdef AS
			if (pos>max_pos) max_pos=pos;
	#endif
			if (lastst == comma)
				getsym();
			if (lastst == end || lastst == semicolon) {
			  pad:
	#ifdef AS
				if (pos<max_pos) move_pos(max_pos-pos), nbytes+=max_pos-pos;
	#endif
				while (nbytes < tp->size) {
					genbyte(0);
					nbytes++;
				}
				break;
			}
			if (tp->size > 0 && nbytes >= tp->size)
				break;
		}
	}
#if 0
	if (tp->size == 0)
		tp->size = nbytes;
	if (nbytes > tp->size)
		error(ERR_INITSIZE);
#else
	if (tp->size && nbytes > tp->size)
		error(ERR_INITSIZE);
	if (tp->size != nbytes && tpp)
		/* fix the symbol's size, unless tpp=0 (item of a struct/union or array) */
		tp = copy_type_global(tp),
		*tpp = tp,
		tp->size = nbytes;	// we need this for the 'sizeof' operator...
#endif
//	tmp_free();
	return nbytes;
}

static long initunion(TYP *tp,TYP **tpp) {
	struct sym	   *sp;
	long			nbytes;
	
	int brace_seen = 0;
	if (lastst == begin) {
		brace_seen = 1;
		getsym();
	}

	sp = tp->lst.head;
/*
 * Initialize the first branch
 */
	if (sp == 0)
		return 0;
	nbytes = inittype(sp->tp,NULL);
	while (nbytes < tp->size) {
		genbyte(0);
		nbytes++;
	}

	if (tp->size != nbytes && tpp)
		tp = copy_type_global(tp),
		*tpp = tp,
		tp->size = nbytes;	// we need this for the 'sizeof' operator...

	if (brace_seen)
		needpunc(end);
}

static long initstruct(TYP *tp,TYP **tpp) {
	struct sym	   *sp;
	long			nbytes;
	nbytes = 0;
	sp = tp->lst.head;			/* start at top of symbol table */
	if (lastst != end)
		while (sp != 0) {
			while (nbytes < sp->value.i) {	/* align properly */
				nbytes++;
				genbyte(0);
			}
			nbytes += inittype(sp->tp,NULL);
			if (lastst == comma)
				getsym();
			if (lastst == end || lastst == semicolon)
				break;
			sp = sp->next;
		}
	while (nbytes < tp->size) {
		genbyte(0);
		nbytes++;
	}
	if (tp->size != nbytes && tpp)
		tp = copy_type_global(tp),
		*tpp = tp,
		tp->size = nbytes;	// we need this for the 'sizeof' operator...

	return tp->size;
}

static int initchar() {
	genbyte((int) intexpr());
	return 1;
}

static int initshort() {
	genword((int) intexpr());
	return 2;
}

static int initlong() {
/*
 * We allow longs to be initialized with pointers now.
 * Thus, we call constexpr() instead of intexpr.
 */
#if 0
/*
 * This is more strict
 */
	genlong(intexpr());
	return 4;
#endif
	genptr(constexpr());
	return 4;
}

#ifndef NOFLOAT
static int initfloat() {
#ifndef NOFLOAT
	double			floatexpr();
#ifdef PC
	genfloat(floatexpr());
#else
#ifndef BCDFLT
	genptr(floatexpr());
#else
	genfloat(floatexpr());
#endif
#endif
#endif
#ifdef NOFLOAT
	genptr(0l);
#endif
	return 4;
}

#ifdef DOUBLE
static int initdouble() {
#ifdef NOFLOAT
	int i;
	for (i=0; i< tp_double.size; i++)
		genbyte(0);
#endif
#ifndef NOFLOAT
	double floatexpr();
	gendouble(floatexpr());
#endif
	return tp_double.size;
}
#endif
#endif

static int initpointer() {
	genptr(constexpr());
	return 4;
}

static struct enode *constexpr() {
	struct enode *ep;
	struct typ	 *tp;
/*	if (lineid==0x1e0)
		bkpt();*/
	tp=exprnc(&ep);
	if (tp == 0) {
		error(ERR_EXPREXPECT);
		return 0;
	}
	opt0(&ep);
	if (!tst_const(ep)) {
		error(ERR_CONSTEXPECT);
		return 0;
	}
	return ep;
}
// vim:ts=4:sw=4
