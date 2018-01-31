/*
 * GTools C compiler
 * =================
 * source file :
 * misc declarations
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
#define USE_MEMMGT
#include	"define.h"
_FILE(__FILE__)
#include	"c.h"
#include	"expr.h"
#include	"gen.h"
#include	"cglbdec.h"

static struct typ *save_type(struct typ *tp);
static int declbegin(enum(e_sym) st);
static void declenum(struct hstab *table);
static void decl2(void);
static void declstruct(enum(e_bt) ztype);
static void enumbody(struct hstab *table);
static void structbody(struct typ *tp, enum(e_bt) ztype);
long auto_init(long offs,TYP *typ,TYP **tpp,int brace_level,int offmod,int stroff);
void doinit(struct sym *sp, int align);

TYP 		   *head CGLOB, *tail CGLOB;
int 			regptr CGLOB;
XLST_TYPE		reglst[REG_LIST] CGLOBL;
int				regalloc[REG_LIST];
int 			autoptr CGLOB;
XLST_TYPE		autolst[AUTO_LIST] CGLOBL;
int 			global_flag CGLOB,global_strings CGLOB;
int 			asm_flag CGLOB,asm_xflag CGLOB,asm_zflag CGLOB;
char		   *declid CGLOB;
int				store_lst CGLOB;
#ifdef NEW_ATTR
TI_SHORT		func_attr CGLOBL;
#endif
#ifdef MID_DECL
int				middle_decl CGLOB;
#endif

#ifdef LISTING
#define LP 0,
#else
#define LP
#endif

readonly TYP	tp_void = {{0, 0}, 0, LP 1, bt_void, 0, 1};// 1 and not -1 as previously defined
readonly TYP	tp_long = {{0, 0}, 0, LP 4, bt_long, 0, 1};
readonly TYP	tp_ulong = {{0, 0}, 0, LP 4, bt_ulong, 0, 1};
readonly TYP	tp_char = {{0, 0}, 0, LP 1, bt_char, 0, 1};
readonly TYP	tp_uchar = {{0, 0}, 0, LP 1, bt_uchar, 0, 1};
readonly TYP	tp_short = {{0, 0}, 0, LP 2, bt_short, 0, 1};
readonly TYP	tp_ushort = {{0, 0}, 0, LP 2, bt_ushort, 0, 1};
#ifndef NOFLOAT
#ifndef BCDFLT
readonly TYP	tp_float = {{0, 0}, 0, LP 4, bt_float, 0, 1};
#else
readonly TYP	tp_float = {{0, 0}, 0, LP 10, bt_float, 0, 1};
#endif
#ifdef INTEL_386
readonly TYP	tp_double = {{0, 0}, 0, LP 8, bt_double, 0, 1};
#endif
#endif
readonly TYP	tp_constchar = {{0, 0}, LP 0, 1, bt_char, 0, 1, 1};
readonly TYP	tp_string = {{0, 0}, (TYP *)&tp_constchar, LP 4, bt_pointer, 0, 1};
readonly TYP	tp_void_ptr = {{0, 0}, (TYP *)&tp_void, LP 4, bt_pointer, 0, 1};
readonly TYP	tp_econst = {{0, 0}, 0, LP 2, bt_ushort, 1, 1};	// tp_uint + val_flag=1
//readonly TYP	  tp_int, tp_uint;
readonly TYP	tp_func = {{0, 0}, (TYP *)&tp_int, LP 4, bt_func, 1, 1};
//int			  int_bits;

extern int		glblabel;

/* variables for function parameter lists */
int		nparms CGLOB;
char    *names[MAX_PARAMS] CGLOBL;

/* variable for bit fields */
int		bit_offset CGLOB; 	/* the actual offset */
int		bit_width CGLOB;		/* the actual width */
int		bit_next CGLOB;		/* offset for next variable */
int		decl1_level CGLOB;

int stringlit(char *s, int len) {
/*
 * mk_ s a string literal and return its label number.
 * 'len' should not include the terminating zero
 */
	struct slit    *lp;
	char		   *p;
	int 			l, local_global = global_flag;
	global_flag = 0;			/* always allocate from local space. */
	lp=strtab;
	while (lp!=0) {
		if (len==(l=lp->len)) {
			char *a=s,*b=lp->str;
			while (l--)
				if (*a++!=*b++)
					goto stringlit_not_same;
			return lp->label;
		}
	stringlit_not_same:
		lp=lp->next;
	}
	lp = (struct slit *) xalloc((int) sizeof(struct slit), SLIT+STRINGLIT);
	lp->label = global_strings?nxtglabel():nxtlabel();
	p = lp->str = (char *) xalloc((int) len, STR+STRINGLIT);
	lp->len = len;
	while (len--)
		*p++ = *s++;
	lp->next = strtab;
	strtab = lp;
	global_flag = local_global;
	return lp->label;
}
#ifdef USE_DATALIT
int datalit(char *s, int len) {
/*
 * mk_ s a data literal and return its label number.
 */
	struct slit    *lp;
	char		   *p;
	int 			l, local_global = global_flag;
	global_flag = 0;			/* always allocate from local space. */
	lp=datatab;
	while (lp!=0) {
		if (len==(l=lp->len)) {
			char *a=s,*b=lp->str;
			while (l--)
				if (*a++!=*b++)
					goto datalit_not_same;
			return lp->label;
		}
	datalit_not_same:
		lp=lp->next;
	}
	lp = (struct slit *) xalloc((int) sizeof(struct slit), SLIT+STRINGLIT);
	lp->label = global_strings?nxtglabel():nxtlabel();
	p = lp->str = (char *) xalloc((int) len, STR+STRINGLIT);
	lp->len = len;
	while (len--)
		*p++ = *s++;
	lp->next = datatab;
	datatab = lp;
	global_flag = local_global;
	return lp->label;
}
#endif

char *litlate(char *s) {
	char *p, *q, m;
	int string=0;
	while (isspace(*s)) s++;
	q = p = (char *)xalloc((int)strlen(s) + 1, STR+LITLATE);
	while ((*p++=m=*s++) && m!='\n')
		if (p-2>=q && p[-2]=='\\') {}
		else if (m=='"') string=~string;
		else if (m=='/' && !string) {
			if (*s=='/') {
				p[-1]=' ', *p++=0; break;
			} else if (*s=='*') {
				p--; if (!isspace(p[-1])) *p++=' ';
				s+=2;	/* since / * / isn't a valid comment */
				while (*s && (*s++!='/' || s[-2]!='*'));
			}
		}
	if (m) *--p=0;
	if (string) return 0;	/* invalid if quotes are unbalanced */
	return q;
}

static long imax(long i, long j) {
	return (i > j) ? i : j;
}

char *strsave(char *s) {
	char *p, *q;
	q = p = (char *)xalloc((int)strlen(s) + 1, STR+STRSAVE);
	while ((*p++ = *s++));
	return q;
}

TYP *copy_type(TYP *s) {
	TYP 		   *tp;
	tp = (TYP *) xalloc((int) sizeof(TYP), _TYP);
	*tp = *s;
	//memcpy(tp, s, sizeof(TYP));
	return tp;
}

TYP *mk_type(enum(e_bt) bt, int siz) {
	TYP 		   *tp;
	tp = (TYP *) xalloc((int) sizeof(TYP), _TYP+MK_TYPE);
	tp->size = siz;
	tp->type = bt;
	tp->st_flag = global_flag;
#ifdef NO_CALLOC
#ifdef LISTING
	tp->sname = 0;
#endif
	tp->btp = 0;
	tp->lst.tail = tp->lst.head = 0; tp->lst.hash = 0;
	tp->val_flag = tp->const_flag = tp->vol_flag = 0;
	tp->bit_width = 0;
#endif
	return tp;
}

TI_SHORT process_attr();
void decl(HTABLE *table) {		/* table is used only for enum members */
//	struct sym	   *sp;

	/*
	 * at top level, 'int' is changed to 'short' or 'long'
	 */

	switch (lastst) {
	  case kw_void:
		head = tail = (TYP *)&tp_void;
		getsym();
		break;
	  case kw_char:
		head = tail = (TYP *)&tp_char;
		getsym();
		break;
	  case kw_short:
		getsym();
		if (lastst == kw_unsigned) {
			getsym();
			head = tail = (TYP *)&tp_ushort;
		} else {
			if (lastst == kw_signed) getsym();
			head = tail = (TYP *)&tp_short;
		}
		if (lastst == kw_int)
			getsym();
		break;
	  case kw_int:
		head = tail = (TYP *)&tp_int;
		getsym();
		break;
	  case kw_long:
		getsym();
		if (lastst == kw_unsigned) {
			getsym();
			head = tail = (TYP *)&tp_ulong;
		} else {
			if (lastst == kw_signed) getsym();
			head = tail = (TYP *)&tp_long;
		}
		if (lastst == kw_int)
			getsym();
		break;
	  case kw_unsigned:
		getsym();
		switch (lastst) {
		  case kw_long:
			getsym();
			head = tail = (TYP *)&tp_ulong;
			if (lastst == kw_int)
				getsym();
			break;
		  case kw_char:
			getsym();
			head = tail = (TYP *)&tp_uchar;
			break;
		  case kw_short:
			getsym();
			head = tail = (TYP *)&tp_ushort;
			if (lastst == kw_int)
				getsym();
			break;
		  case kw_int:
			getsym();
		  default:
			head = tail = (TYP *)&tp_uint;
			break;
		}
		break;
	  case kw_signed:
		getsym();
		switch (lastst) {
		  case kw_long:
			getsym();
			head = tail = (TYP *)&tp_long;
			if (lastst == kw_int)
				getsym();
			break;
		  case kw_char:
			getsym();
			head = tail = (TYP *)&tp_char;
			break;
		  case kw_short:
			getsym();
			head = tail = (TYP *)&tp_short;
			if (lastst == kw_int)
				getsym();
			break;
		  case kw_int:
			getsym();
		  default:
			head = tail = (TYP *)&tp_int;
			break;
		}
		break;
	  case kw_const:
		getsym();
		decl(table);
		head = copy_type(head);
		head->const_flag = 1;
		tail = head;
		break;
	  case kw_volatile:
		getsym();
		decl(table);
		head = copy_type(head);
		head->vol_flag = 1;
		tail = head;
		break;
	  case kw_typeof:
		getsym();
		needpunc(openpa);
		{ struct enode *en;
		/*tmp_use(); -> TODO, but copy_type must be global/local*/
		head = copy_type(expression(&en));
		/*tmp_free();*/ }
		head->const_flag = head->vol_flag = 0;
		tail = head;
		needpunc(closepa);
		break;
	  case id:
		if (lastsp != 0 &&
			lastsp->storage_class == sc_typedef)
			/* type identifier */
		{
			/* BUGGY/FIXME!!! I swapped these two lines, but I can't figure out why
			 * they had to be */
			head = tail = lastsp->tp;
			getsym();
			break;
		}
		/* else fall through */
	  case openpa:
	  case star:
		/* default type is int */
		head = tail = (TYP *)&tp_int;
		break;
#ifndef NOFLOAT
	  case kw_float:
#ifndef DOUBLE
	  case kw_double:
#endif
		head = tail = (TYP *)&tp_float;
		getsym();
		break;
#ifdef DOUBLE
	  case kw_double:
		head = tail = (TYP *)&tp_double;
		getsym();
		break;
#endif
#endif
	  case kw_enum:
		getsym();
		declenum(table);
		break;
	  case kw_struct:
		getsym();
		declstruct(bt_struct);
		break;
	  case kw_union:
		getsym();
		declstruct(bt_union);
		break;
	  case kw_attr:
		func_attr=process_attr();
		decl(table);
		break;
	}
	if (lastst==kw_volatile) {
		head = copy_type(head);
		head->vol_flag = 1;
		tail = head;
		getsym();
	}
}

#ifdef PC
TI_SHORT unset_attr={-1,-1};
TI_SHORT stkparm_attr={0,0};
TI_SHORT regparm_attr={2,1};
#define attr_isset(v) ((v).hi!=(unsigned char)-1)
#else
#define unset_attr -1
#define stkparm_attr 0
#define regparm_attr 0x0201
#define attr_isset(v) ((int)(v) IS_VALID)
#endif

TI_SHORT process_attr() {
#ifdef DEFAULT_STKPARM
#ifndef NEW_ATTR
	unsigned char rp_dn=0,rp_an=0;
#endif
#define DEFLT_ATTR stkparm_attr
#else
#ifndef NEW_ATTR
	unsigned char rp_dn=2,rp_an=1;
#endif
#define DEFLT_ATTR regparm_attr
#endif
#ifdef NEW_ATTR
	unsigned char rp_dn=-1,rp_an=-1;
#endif
	getsym();
	needpunc(openpa);
	{
		int n=1;
		do {
			if (lastst==openpa) n++;
			else if (lastst==closepa) n--;
			else if (lastst==id) {
#ifdef REGPARM
				if (lastid[0]=='_' && lastid[1]=='_')
					memmove(lastid,lastid+2,MAX_ID_LEN-2),lastid[strlen(lastid)-2]=0;
				if (!strcmp(lastid,"stkparm"))
					rp_dn=0, rp_an=0;
				else if (!strcmp(lastid,"aregparm"))
					rp_dn=3, rp_an=2;
				else if (!strcmp(lastid,"regparm")) {
					getsym();
					if (lastst==openpa) {
						getsym();
						if (lastst!=iconst) error(ERR_SYNTAX);
						else {
							rp_dn = rp_an = (char)ival;
							getsym();
							if (lastst==comma) {
								getsym();
								if (lastst==iconst)
									rp_an = (char)ival, getsym();
							}
						}
						if (rp_dn>CONVENTION_MAX_DATA+1 || rp_an>CONVENTION_MAX_ADDR+1)
							error(ERR_OUTRANGE), rp_dn=0, rp_an=0;
						if (lastst!=closepa) uerr(ERR_PUNCT,')');
					} else { rp_dn=2; rp_an=1; goto nxt_attr; }
				}
#endif
			} else if (lastst!=comma)
				uerr(ERR_PUNCT,')');
			getsym();
		nxt_attr:
			(void)0;
		} while (n>0);
	}
#ifdef PC
	{
	TI_SHORT ret;
	ret.hi=rp_dn;
	ret.lo=rp_an;
	return ret;
	}
#else
	return (rp_dn<<8)+(rp_an);
#endif
}

void decl1(void) {
/* handles *, :, ... */
	TYP 		   *temp1, *temp2, *temp3, *temp4;
	decl1_level++;
	switch (lastst) {
	  case id:
		declid = strsave(lastid);
		getsym();
		if (lastst != colon) {
			decl2();
			break;
		}
		/* FALLTHROUGH */
	  case colon:
		getsym();
		if (decl1_level != 1)
			error(ERR_FIELD);
		if (head->type != tp_int.type && head->type != tp_uint.type)
#if 0
			error(ERR_FTYPE);
#else
			uwarn("field type should be unsigned or int");
#endif
		bit_width = (int)intexpr();
		bit_offset = bit_next;
		if (bit_width < 0 || bit_width > int_bits) {
			error(ERR_WIDTH);
			bit_width = 1;
		}
		if (bit_width == 0 || bit_offset + bit_width > int_bits)
			bit_offset = 0;
		bit_next = bit_offset + bit_width;
		break;
	  case star:
		temp1 = mk_type(bt_pointer, 4);
		temp1->btp = head;
		head = temp1;
		if (tail == NULL)
			tail = head;
		getsym();
		decl1();
		break;
	  case openpa: {
#ifndef NEW_ATTR
		short attr=DEFLT_ATTR;
#endif
		getsym();
		temp1 = head;
		temp2 = tail;
		head = tail = NULL;
		decl1();
#ifndef NEW_ATTR
		if (lastst==kw_attr)
			attr=process_attr();
#endif
		needpunc(closepa);
		temp3 = head;
		temp4 = tail;
		head = temp1;
		tail = temp2;
		decl2();
#ifndef NEW_ATTR
#ifdef REGPARM
		assert(sizeof(short)==sizeof(TI_SHORT));
		if (*(short *)&head->rp_dn==DEFLT_ATTR)
			*(short *)&head->rp_dn=attr;
#endif
#endif
		if (temp4 != NULL) {
			temp4->btp = head;
			if (temp4->type == bt_pointer &&
				temp4->val_flag != 0 && head != NULL)
				temp4->size = (long)((int)head->size)*((int)temp4->size);
			head = temp3;
		}
	   } break;
#ifdef NEW_ATTR
	  case kw_attr:
		func_attr=process_attr();
		decl1();
		break;
#endif
/*	  case kw_attr:
		process_attr();*/
		/* FALL THROUGH */
	  case kw_const:
		getsym();
		head = copy_type(head);
		head->const_flag = 1;
		tail = head;
		decl1();
		break;
	  case kw_volatile:
		getsym();
		head = copy_type(head);
		head->vol_flag = 1;
		tail = head;
		decl1();
		break;
	  default:
		decl2();
		break;
	}
	decl1_level--;
	/*
	 * Any non-bitfield type resets the next offset
	 */
	if (bit_width IS_INVALID)
		bit_next = 0;
}

#ifdef REGPARM
#if 0
short count_dn_an(struct enode *plist,int rp_dn,int rp_an) {
	int true_dn=0,true_an=0;
	int nr=rp_dn+rp_an,np=0,n;
	int list[16],*p;
	struct enode *ep=plist;
	while (ep) np++, ep=ep->v.p[1];
	/* first, push stack params while all the temp registers are free */
	while (np>nr) np--,plist = plist->v.p[1];
	/* store the last params so we can examinate them in the correct order */
	n=np; while (n--) list[n]=plist->v.p[0]->etype-bt_pointer, plist=plist->v.p[1];
	/* now, get the real # of regs */
	p=list;
	n=np; while (n--) {
		if ((!*p++ && rp_an) || !rp_dn)
			true_an++, rp_an--;
		else true_dn++, rp_dn--;
	}
#ifdef PC
	return (true_dn)+(true_an<<8);
#else
	return (true_dn<<8)+(true_an);
#endif
}
#endif
#endif

#ifndef NO_VARARG_FUNC
char variable_arg_name[]="__vararg__";
#define is_variable_arg(sp) ((sp)->name==variable_arg_name)
#define mk_variable_arg(sp) ((sp)->name=variable_arg_name)
#endif

static long declare(HTABLE *table, enum(e_sc) al, long ilc, enum(e_bt) ztype, int regflag);
static void decl2(void) {
/* Handles __attribute__, [*] (for arrays) and (*) (for functions) */
	TYP 		   *temp1,*temp2;
	long			i;
	switch (lastst) {
	  case openbr:
		getsym();
		if (lastst == closebr)
			i = 0;
		else
			i = intexpr();
		needpunc(closebr);
		if (lastst == openbr)
			decl2();
		temp1 = mk_type(bt_pointer, 0);
		temp1->val_flag = 1;
		temp1->btp = head;
		if (head != 0)
			temp1->size = i * head->size;
		else
			temp1->size = i;
		head = temp1;
		if (tail == NULL)
			tail = head;
		decl2();
		break;
	  case openpa:
		getsym();
		temp1 = mk_type(bt_func, 4);
		temp1->val_flag = 1;
		temp1->btp = head;
		temp1->lst.head=temp1->lst.tail=0; temp1->lst.hash=0;
		head = temp1;
		if (tail == NULL)
			tail = head;
		temp2 = tail;
		if (lastst == kw_void) {
			getcache(-1);
			if (cached_sym == closepa)
				getsym();
		}
		if (lastst == closepa)
			getsym();
		else {
			TABLE *tp;
			struct sym *sp;
			char *func_name;
			long slc;
			int regflag;
			int old_nparms;
			int old_middle_decl=middle_decl,old_store_lst=store_lst;
//			int old_regptr=regptr,old_autoptr=autoptr;
#ifdef NO_PROTO
			if (nparms != 0)	// (void(*)(myparam1...))myfunc() *was* invalid
				error(ERR_PARMS);
#endif
#ifdef REGPARM
#ifdef DEFAULT_STKPARM
			head->rp_dn=0; head->rp_an=0;
#else
			head->rp_dn=2; head->rp_an=1;
#endif
#endif
			/*if (lineid==32)
				bkpt();*/
			tp=(TABLE *)temp1;
			old_nparms=nparms;
			func_name = declid;
			slc=34560;
			store_lst=pch_init?0:global_flag;
			/*
			 * Basically 'store_lst' is always equal to 1, except in two cases:
			 * - declaring a function inside a PCH initialization (pch_init!=0)
			 * - prototyping a function (global_flag==0)
			 * because we don't want the parameters of those functions to mess
			 * with the function we are declaring.
			 *
			 * In all the other cases, we reset 'regptr'/'autoptr' when
			 * parsing the declaration of the function (starting from the
			 * parameter list, of course).
			 */
			if (store_lst) {
				regptr = 0;
				autoptr = 0;
			}
			middle_decl=0;
			while (1) {
				if (nparms >= MAX_PARAMS)
					uerrsys("too many params");
				regflag = 3;
				switch (lastst) {
				case kw_typedef: case kw_static: case kw_auto: case kw_extern:
						error(ERR_ILLCLASS);
						return;
				case kw_register:
						regflag = 4;
						getsym();
						goto do_pdecl;
				case id:
				/*
				 * If it is a typedef identifier, fall through & do the declaration.
				 */
						if (!(sp = lastsp) || sp->storage_class != sc_typedef) {
							names[nparms++] = strsave(lastid);
							getsym();
							break;
						}
				case kw_char: case kw_short: case kw_unsigned: case kw_long:
				case kw_struct: case kw_union: case kw_enum: case kw_void:
				case kw_float: case kw_double: case kw_int: case kw_typeof:
				case kw_signed: case kw_const: case kw_volatile:
do_pdecl:
					slc+=declare((HTABLE *)tp, sc_parms2, slc, bt_struct, regflag);
					if (isaggregate(head))
						func_attr=stkparm_attr;	/* force the function to be stkparm... */
					names[nparms++] = declid;
					break;
				/*case closepa:
					getsym();
					return;	*/
				case dots:
					getsym();
#ifdef NO_VARARG_FUNC
					tp->head=tp->tail=NULL;
#else
					func_attr=stkparm_attr;	/* force the function to be stkparm... */
					{
						SYM *sp=(SYM *)xalloc(sizeof(SYM),_SYM);
						mk_variable_arg(sp);
						append(&sp,(HTABLE *)tp);
						goto args_done;
					}
#endif
					break;
				default:
					error(ERR_SYNTAX);
				}
				if (lastst == comma) getsym();
				else break;
			}
		args_done:
			store_lst=old_store_lst;
			middle_decl=old_middle_decl;
			needpunc(closepa);
			head = temp1;
			tail = temp2;
			if (lastst != begin && (!nparms || !castbegin(lastst)))
				nparms=old_nparms;
			declid=func_name;
		}
#ifdef NEW_ATTR
		*(TI_SHORT *)&head->rp_dn=DEFLT_ATTR;
#endif
		if (lastst==kw_attr)
#ifdef REGPARM
			*(TI_SHORT *)&head->rp_dn=
#endif
				process_attr();
#ifdef NEW_ATTR
		if (attr_isset(func_attr))
			*(TI_SHORT *)&head->rp_dn=func_attr, func_attr=unset_attr;
#endif
/*#ifdef REGPARM
		if (head->rp_dn || head->rp_an)
			*(short *)&(head->rp_dn)=count_dn_an(
#endif*/
		break;
/*		if (lastst == kw_void) {
			getsym();
			if (lastst != closepa)
				error(ERR_PARMS);
		}
		if (lastst == closepa)
			getsym();
		else {
			if (nparms != 0)
				error(ERR_PARMS);
			if (lastst != id) {
				while (lastst != closepa) {
					names[nparms++] = strsave(lastid);
				}
			} else while (lastst == id) {
				if (nparms >= MAX_PARAMS)
					fatal("MAX_PARAMS");
				names[nparms++] = strsave(lastid);
				getsym();
				if (lastst == comma)
					getsym();
			}
			needpunc(closepa);
		}
		break;*/
	}
}

int alignment(TYP *tp) {
	switch (tp->type) {
	  case bt_uchar:
	  case bt_char:
		return AL_CHAR;
	  case bt_ushort:
	  case bt_short:
		return AL_SHORT;
	  case bt_ulong:
	  case bt_long:
		return AL_LONG;
	  case bt_pointer:
		if (tp->val_flag)
			return alignment(tp->btp);
		else
			return AL_POINTER;
	  case bt_func:
		return AL_FUNC;
	  case bt_float:
		return AL_FLOAT;
#ifdef DOUBLE
	  case bt_double:
		return AL_DOUBLE;
#endif
	  case bt_struct:
	  case bt_union:
		return AL_STRUCT;
	  case bt_void:
		return 1;
	  default:
		ierr(ALIGNMENT,1);
		return 0; // does not happen
	}
}

xstatic long old_nbytes CGLOB;
	/* should be in declare(), but SConvert would put it in the text */
	/*  segment, preventing it from being modified */
static long declare(HTABLE *table, enum(e_sc) al, long ilc, enum(e_bt) ztype, int regflag) {
/*
 * process declarations of the form:
 *
 * <type>		<decl>, <decl>...;
 *
 * leaves the declarations in the symbol table pointed to by table and returns
 * the number of bytes declared. al is the allocation type to assign, ilc is
 * the initial location counter. if al is sc_member then no initialization
 * will be processed. ztype should be bt_struct for normal and in structure
 * declarations and sc_union for in union declarations.
 *
 * regflag values:
 *	   -2:		union  component
 *	   -1:		struct component
 *		0:		static, global, extern
 *		1:		auto
 *		2:		register auto
 *		3:		argument
 *		4:		register argument
 */
	struct sym				 *sp, *sp1;
	TYP 									 *dhead;
	long											nbytes;
	int 											func_flag;
	int 											no_append;
	nbytes = 0;
	decl(table);
	dhead = head;
	for (;;) {
		declid = 0;
		bit_width = -1;
		decl1();
#ifdef MID_DECL
		if (middle_decl && !declid && lastst==closepa)
			return 0;
		/*				if (middle_decl)
						bkpt();*/
#endif
		if (!declid && al == sc_member)
			declid = "__unnamed__";
		if (declid != 0) {		/* otherwise just struct tag
								 * or unnamed argument in func prototype... */
			if (head->size == 0 && head->type == bt_pointer
					&& al == sc_typedef) {
				uwarn("typedef with null size");
			}
#ifdef LISTING
			if (al==sc_typedef)	/* hack to display structs' real names */
				head->sname = declid;
#endif
			func_flag = 0;
			if (head->type == bt_func &&
					(lastst == begin || (nparms > 0 && castbegin(lastst)))) {
				func_flag = 1;
				concat((TABLE *)&lsyms,(TABLE *)head);
			}
			sp = (struct sym *) xalloc((int) sizeof(struct sym), _SYM+_DECLARE);
			sp->name = declid;
			sp->storage_class = (al==sc_parms2) ? sc_auto : al;
#ifdef NO_CALLOC
			sp->used = 0;
#endif

			if (head->type != bt_func && bit_width > 0 && bit_offset > 0) {
				/*
				 * share the storage word with the previously defined field
				 */
				nbytes = old_nbytes - ilc;
			}
			old_nbytes = ilc + nbytes;
			while ((ilc + nbytes) % alignment(head))
				nbytes++;
			//printf("%s: %d\n",sp->name,al);
			if (al==sc_global) {/* don't store value.i (else pb w/ value.splab) */}
			else if (al == sc_static)
#ifdef AS
				sp->value.splab=0/*nxtglabel()*/;//->this will be done in genstorage()
#else
			sp->value.i = nxtglabel();
#endif
			else if (ztype == bt_union)
				sp->value.i = ilc;
			else if (al != sc_auto)
				sp->value.i = ilc + nbytes;
			else
				sp->value.i = -(ilc + nbytes + head->size);

			/*
			 * The following code determines possible candidates for register
			 * variables. Note that 'sp->value.i' could be modified later on,
			 * but only when auto_init()ing an array/struct/union; and these
			 * structures are excluded from here (except for arguments to the
			 * function with type array: they are really pointers).
			 */
			if (store_lst)
				switch (head->type) {
					case bt_pointer:
						if (head->val_flag && regflag < 3)
							break;
						/* FALLTHROUGH */
					case bt_char:
					case bt_uchar:
					case bt_short:
					case bt_ushort:
					case bt_long:
					case bt_ulong:
					case bt_float:
#ifdef DOUBLE
					case bt_double:
#endif
						switch (regflag) {
							case 1:
							case 3:
								if (autoptr < AUTO_LIST)
									autolst[autoptr++] = sp->value.i;
								break;
							case 2:
							case 4:
								if (regptr < REG_LIST)
									reglst[regptr++] = sp->value.i;
								break;
						}
				}

			if (bit_width IS_INVALID) {
				sp->tp = head;
			} else {
				sp->tp = (struct typ *) xalloc((int) sizeof(struct typ),
						_TYP+_DECLARE);
				*(sp->tp) = *head;
				sp->tp->type = bt_bitfield;
				sp->tp->size = tp_int.size;
				sp->tp->bit_width = (char)bit_width;
				sp->tp->bit_offset = (char)bit_offset;
			}
			/*
			 * The following stuff deals with inconsistencies in the
			 * C syntax and semantics, namely the scope of locally
			 * declared functions and its default storage class (extern,
			 * not auto) etc.
			 */
			no_append = 0;
			/*
			 * The flag no_append will be set if some stuff is forwarded to the
			 * global symbol table here and thus should not be repeated in the local
			 * symbol table
			 */

			/*
			 * extern function definitions with function body are global
			 */
			if (func_flag && sp->storage_class == sc_external)
				sp->storage_class = sc_global;
			/*
			 * global function declarations without function body are external
			 */
			if (sp->tp->type == bt_func && sp->storage_class == sc_global
					&& !func_flag) {
				sp->storage_class = sc_external;
			}
			/*
			 * [auto] int test() is not an auto declaration, it should be
			 * external
			 */
			if (sp->tp->type == bt_func && sp->storage_class == sc_auto) {
				/*
				 * althouh the following if-statement is not necessary since
				 * this check is performed in append() anyway, it saves
				 * global storage if the functions has previously been
				 * defined.
				 */
				if ((sp1 = search(sp->name, -1, &gsyms)) == 0) {
					/* put entry in the global symbol table */
					++global_flag;
					sp1 = (struct sym *) xalloc((int) sizeof(struct sym),
							_SYM+_DECLARE);
					sp1->name = strsave(sp->name);
					sp1->storage_class = sc_external;
					sp1->tp = save_type(sp->tp);
#ifdef NO_CALLOC
					sp1->used = 0;
#endif
					append(&sp1, &gsyms);
					--global_flag;
				} else {
					/* already defined in global symbol table */
					if (!eq_type(sp->tp, sp1->tp))
						uerr(ERR_REDECL,sp->name);
				}
				no_append = 1;
				sp = sp1;
			}
			/*
			 * static local function declarations should be put in the
			 * global symbol table to retain the compiler generated
			 * label number
			 */
			if (sp->tp->type == bt_func && sp->storage_class == sc_static
					&& table == &lsyms) {
				if (!(sp1 = search(sp->name, -1, &gsyms))) {
					/* put it into the global symbol table */
					++global_flag;
					sp1 = (struct sym *) xalloc((int) sizeof(struct sym),
							_SYM+_DECLARE);
					sp1->name = strsave(sp->name);
					sp1->storage_class = sc_static;
					sp1->tp = save_type(sp->tp);
					sp1->value.i = sp->value.i;
#ifdef NO_CALLOC
					sp1->used = 0;
#endif
					append(&sp1, &gsyms);
					--global_flag;
				} else {
					if (!eq_type(sp->tp, sp1->tp))
						uerr(ERR_REDECL,sp->name);
				}
				no_append = 1;
				sp = sp1;
			}
			if (ztype == bt_union)
				nbytes = imax(nbytes, sp->tp->size);
			else if (al != sc_external)
				nbytes += sp->tp->size;
			if (!no_append)
				if (al == sc_member)
					insert(sp, table);
				else
					append(&sp, table);
			if (func_flag) {
				/* function body follows */
				int local_nparms = nparms;
				ret_type = sp->tp->btp;
				if (!global_flag)
					error(ERR_SYNTAX);
				nparms = 0;
				funcbody(sp, names, local_nparms);
				return nbytes;
			}
			if ((al == sc_global || al == sc_static) &&
					sp->tp->type != bt_func && sp->used IS_VALID)
				doinit(sp, alignment(head));
			else if (lastst == assign) {
				if (regflag == 1 || regflag == 2) {
					long old=sp->tp->size;
					getsym();
#ifdef MID_DECL_IN_EXPR
#error fix needed here...
#endif
					{struct enode *bak=init_node;
						init_node=NULL;
						old-=auto_init(sp->value.i,sp->tp,&sp->tp,0,0,-1);
						old-=old&1;
						if (old<0) {
							struct enode *en=init_node;
							sp->value.i+=old;
							if (en!=NULL) {
								while (en->nodetype!=en_assign) {
									struct enode *en2=en/*en_void*/->v.p[1]/*en_assign*/
										->v.p[0]/*en_ref*/->v.p[0]/*en_autocon/en_add*/;
									if (en2->nodetype==en_add)
										en2=en2->v.p[0];
									en2->v.i += old;
									en=en->v.p[0];
								}
								en/*en_assign*/->v.p[0]/*en_ref*/
									->v.p[0]/*en_autocon*/->v.i += old;
							}
							nbytes-=old;
						}
						if (bak) {
							if (init_node)
								init_node=mk_node(en_void,bak,init_node);
							else
								init_node=bak;
						}
					}
				} else {
					error(ERR_ILLINIT);
					doinit(sp, alignment(head));
				}
				/* just a small check, it was proposed that I should make it */
				/*if (sp->tp->size == 0 && sp->storage_class != sc_external
				  && sp->storage_class != sc_typedef
				  && !(head->type == bt_pointer && head->val_flag)
				  && regflag < 3) {
				  uwarn("declaration with null size");
				  }*/
			}
		} else if (al==sc_parms2) {		/* in 'if (declid != 0)' ... */
			sp = (struct sym *) xalloc((int) sizeof(struct sym), _SYM+_DECLARE);
			sp->name="";
			sp->tp=head;
			append(&sp,table);
		}
		if (al == sc_parms2
#ifdef MID_DECL
				|| middle_decl
#endif
		   ) return nbytes;
		else if (lastst == semicolon)
			break;
		needpunc(comma);
		if (declbegin(lastst) == 0)
			break;
		head = dhead;
	}
	getsym();
	return nbytes;
}

static int declbegin(enum(e_sym) st) {
		return (st == star || st == id || st == openpa ||
				st == openbr || st == colon);
}

static void declenum(HTABLE *table) {
		struct sym				 *sp;
		TYP 									 *tp;
		if (lastst == id) {
				if ((sp = search(lastid, lastcrc, &ltags)) == 0 &&
						(sp = search(lastid, lastcrc, &gtags)) == 0) {
						sp = (struct sym *) xalloc((int) sizeof(struct sym),
							_SYM+DECLENUM);
						if (short_option)
								sp->tp = mk_type(bt_ushort, 2);
						else
								sp->tp = mk_type(bt_ulong, 4);
						sp->storage_class = sc_type;
						sp->name = strsave(lastid);
#ifdef LISTING
						sp->tp->sname = sp->name;
#endif
#ifdef NO_CALLOC
						sp->used = 0;
#endif
						getsym();
						if (lastst != begin)
								error(ERR_INCOMPLETE);
						else {
								if (global_flag)
										append(&sp, &gtags);
								else
										append(&sp, &ltags);
								getsym();
								enumbody(table);
						}
				} else getsym();
				head = sp->tp;
		} else {
				if (short_option)
						tp = mk_type(bt_ushort, 2);
				else
						tp = mk_type(bt_ulong, 4);
				if (lastst != begin)
						error(ERR_INCOMPLETE);
				else {
						getsym();
						enumbody(table);
				}
				head = tp;
		}
}

static void enumbody(HTABLE *table) {
	long			evalue;
	struct sym	   *sp;
	evalue = 0;
	while (lastst == id) {
		sp = (struct sym *) xalloc((int) sizeof(struct sym), _SYM+ENUMBODY);
		sp->value.i = evalue++;
		sp->name = strsave(lastid);
		sp->storage_class = sc_const;
		sp->tp = (TYP *)&tp_econst;
		append(&sp, table);
		getsym();
		if (lastst == assign) {
			getsym();
			evalue = sp->value.i = intexpr();
			evalue++;
		}
		if (short_option && (sp->value.i < -32768 || sp->value.i > /*32767*/65535))
			error(ERR_ENUMVAL);
		if (lastst == comma)
			getsym();
		else if (lastst != end)
			break;
	}
	needpunc(end);
}

static void declstruct(enum(e_bt) ztype) {
/*
 * declare a structure or union type. ztype should be either bt_struct or
 * bt_union.
 *
 * References to structures/unions not yet declared are allowed now. The
 * declaration has to be done before the structure is used the first time
 */
	struct sym	   *sp;
	TYP 		   *tp;
	if (lastst == id) {
		if ((sp = search(lastid, lastcrc, &ltags)) == 0 &&
			(sp = search(lastid, lastcrc, &gtags)) == 0) {
			sp = (struct sym *) xalloc((int) sizeof(struct sym), _SYM+DECLSTRUCT);
			sp->name = strsave(lastid);
			sp->tp = mk_type(ztype, 0);
			sp->storage_class = sc_type;
#ifdef LISTING
			sp->tp->sname = sp->name;
#endif
			if (global_flag)
				append(&sp, &gtags);
			else
				append(&sp, &ltags);
		}
		if (sp->tp->lst.head != 0)
			getsym();
		else {
			getsym();
			/* allow, e.g. struct x *p; before x is defined */
			if (lastst == begin) {
				getsym();
				structbody(sp->tp, ztype);
			}
		}
		head = sp->tp;
	} else {
		tp = mk_type(ztype, 0);
		if (lastst != begin)
			error(ERR_INCOMPLETE);
		else {
			getsym();
			structbody(tp, ztype);
		}
		head = tp;
	}
}

static void structbody(TYP *tp, enum(e_bt) ztype) {
	long			slc;
	slc = 0;
	bit_next = 0;
	while (lastst != end) {
		if (ztype == bt_struct)
			slc += declare((HTABLE *)&(tp->lst), sc_member, slc, ztype, -1);
		else
			slc = imax(slc, declare((HTABLE *)&tp->lst, sc_member, 0l, ztype, -2));
	}
	/*
	 * consider alignment of structs in arrays...
	 */
#if AL_DEFAULT==2
	slc+=(slc&1);
#else
	if (slc % AL_DEFAULT != 0)
		slc += AL_DEFAULT - (slc % AL_DEFAULT);
#endif
	tp->size = slc;
	getsym();
	bit_next = 0;
}

extern char linein[LINE_LENGTH];
extern char *lptr;
#ifdef VCG
extern int vcg_lvl;
#endif
static int nologo=0;
void compile(void) {
/*
 * main compiler routine. this routine parses all of the declarations using
 * declare which will call funcbody as functions are encountered.
 */
#ifdef VERBOSE
		time_t			ltime;
#endif

	debugcls();
#ifdef PC
	if (!verbose)
		nologo=1;
#endif
	if (!nologo)
#ifdef PC
		msg(
			"        ---  GTC C compiler  ---        \n\n"
			" © 2001-2004 by Paul Froissart\n\n"),nologo++;
#else
#ifndef GTDEV
		msg("GTC compiler - © 2001-2004\n by Paul Froissart\n"),nologo++;
#endif
#endif

#ifdef VERBOSE
	times(&tms_buf);
	ltime = tms_buf.tms_utime;
#endif

	strtab = 0;
	lc_bss = 0;
	debug('1');
	hashinit(&gsyms);
	hashinit(&gtags);
	hashinit(&lsyms);
	hashinit(&ltags);
	hashinit(&labsyms);
	debug('6');
	nextlabel = 1;
	store_lst = 1;
	asm_flag = 0;
	asm_xflag = 0;
	asm_zflag = 0;
	nparms = 0;
	func_sp = NULL;
#ifdef DUAL_STACK
	{
		// create a 16-byte 'leak' (which will be freed in rel_global(), though)
		// to avoid perpetual malloc()/free() of the dualstack
		ds_ensure(16);
		{
		char *ptr = ds_var(char *);
		ds_update(ptr+16);
		}
	}
#endif
#ifdef VCG
	vcg_lvl=VCG_MAX;
#endif
	getch();
	getsym();
	debug('7');
	while (lastst != eof) {
		extern int got_sym;
		got_sym = 0;
		dodecl(sc_global);
#if 0
		if (lastst != eof)		/* what the heck is this??? am I doing a mistake    */
			getsym();			/*  when I disable it? how come I find it only now? */
#elif 1	/* UPDATE : the former seems to be a (missed) fix for the infinite loop *
		 * when a particular kind of syntax error occurred. This is now fixed. */
		if (!got_sym)
			uerr(ERR_SYNTAX);
#endif
	}
#ifdef VERBOSE
	times(&tms_buf);
	decl_time += tms_buf.tms_utime - ltime;
#endif
	debug('8');
	dumplits();
	debug('9');
}

#ifdef VCG
extern struct ocode *vcg_peep_head[];
#endif
void dodecl(enum(e_sc) defclass) {
		long slc = 0;
		struct sym *sp;
		int regflag;
		for (;;) {
				regflag = 1;
				switch (lastst) {
				case kw_register:
						regflag = 2;
						getsym();
						if (defclass != sc_auto && defclass != sc_parms)
								error(ERR_ILLCLASS);
						goto do_decl;
				case kw_auto:
						getsym();
						if (defclass != sc_auto)
								error(ERR_ILLCLASS);
						goto do_decl;
				case kw_asm:
						if (!global_flag) /* more secure than 'if (defclass != sc_global)' ? */
							return;			// so that while (x) { asm(...); ... }
						getsym();			// works (as in-line asm)
#ifdef ASM
						if (lastst != begin) {
#endif
	#ifndef AS
							if (lastst != openpa)
	#endif
								error(ERR_SYNTAX);
	#ifndef AS
							else {
								getsym();
								if (lastst != sconst)
									error(ERR_SYNTAX);
								put_code(_op_asm,0,(struct amode *)strsave(laststr),NIL_AMODE);
								getsym();
								needpunc(closepa);
								needpunc(semicolon);
							}
	#endif
#ifdef ASM
						} else {
							int glob = global_flag;
					#ifdef AS
							struct ocode *sh = scope_head;
							int nextlab = nextlabel;
					#endif
							global_flag = 0;
#ifdef PCH
							push_local_mem_ctx();	/* because global_flag=0 is possible inside
													 *  a function */
#endif
					#ifdef AS
							put_align2();
							scope_head = 0;
							scope_init();
					#endif
							asm_flag++;
							asm_head = 0;
							asm_getsym();
							while (getasm_main());
							asm_flag--;
							needpunc(end);
							needpunc(semicolon);
							if (strtab)
								dumplits();
					#ifdef AS
							scope_head = asm_head;
							scope_flush();
							nextlabel = nextlab;
							scope_head = sh;
							local_clean();
					#else
						#ifdef VCG
							vcg_peep_head[vcg_lvl] = asm_head;
						#else
							peep_head = asm_head;
						#endif
							flush_peep();
					#endif
#ifdef PCH
							pop_local_mem_ctx();
#else
							rel_local();			/* release local symbols */
#endif
							global_flag = glob;
						}
#endif
						break;
				case id:
				/*
				 * If it is a typedef identifier, do the declaration.
				 */
						if ((sp = lastsp) != 0 && sp->storage_class == sc_typedef)
								goto do_decl;
						/* else fall through */
						/*
						 * If defclass == sc_global (we are outside any function), almost
						 * anything can start a declaration, look, e.g. mined:
						 * (*escfunc(c))() ,,almost anything'' is not exact: id (no
						 * typedef id), star, or openpa.
						 */
				case openpa:
				case star:
						if (defclass == sc_global)
								goto do_decl;
						return;
						/* else fall through to declare  */
				case kw_char:
				case kw_short:
				case kw_unsigned:
				case kw_long:
				case kw_struct:
				case kw_union:
				case kw_enum:
				case kw_void:
				case kw_float:
				case kw_double:
				case kw_int:
				case kw_typeof:
				case kw_attr:
				case kw_signed: case kw_const: case kw_volatile:
do_decl:
						if (defclass == sc_global)
								(void) declare(&gsyms, sc_global, 0l, bt_struct, 0);
						else if (defclass == sc_auto)
								lc_auto +=
								declare(&lsyms, sc_auto, lc_auto, bt_struct, regflag);
						else			/* defclass == sc_parms (parameter decl.) */
								slc +=
								declare(&lsyms, sc_auto, slc, bt_struct, regflag + 2);
						break;
				case kw_static:
						getsym();
						if (defclass == sc_member)
								error(ERR_ILLCLASS);
						if (defclass == sc_auto)
								(void) declare(&lsyms, sc_static, 0l, bt_struct, 0);
						else
								(void) declare(&gsyms, sc_static, 0l, bt_struct, 0);
						break;
				case kw_typedef:
						getsym();
						if (defclass == sc_member)
								error(ERR_ILLCLASS);
						if (defclass == sc_auto)
								(void) declare(&lsyms, sc_typedef, 0l, bt_struct, 0);
						else
								(void) declare(&gsyms, sc_typedef, 0l, bt_struct, 0);
						break;
				case kw_extern:
						getsym();
						if (defclass == sc_member)
								error(ERR_ILLCLASS);
						if (defclass == sc_auto)
								(void) declare(&lsyms, sc_external, 0l, bt_struct, 0);
						else
								(void) declare(&gsyms, sc_external, 0l, bt_struct, 0);
						break;
				default:
						return;
				}
		}
}

int eq_type(TYP *tp1, TYP *tp2) {
/*
 * This is used to tell valid from invalid redeclarations
 */
	if (tp1 == 0 || tp2 == 0)
		return 0;

	if (tp1 == tp2)
		return 1;

	if (tp1->type != tp2->type)
		return 0;

	if (tp1->type == bt_pointer)
		return eq_type(tp1->btp, tp2->btp);
	if (tp1->type == bt_func) {
		SYM *sp=tp1->lst.head,*sp2=tp2->lst.head;
		if (!sp || !sp2)	// one is only a prototype : OK
			return eq_type(tp1->btp, tp2->btp);
		while (sp && sp2) {
			/*
			 * in vararg funcs we can have sp->tp==sp2->tp==NULL, but
			 * eq_type(NULL,NULL) is 0 hence the first check
			 */
			if (sp->tp!=sp2->tp && !eq_type(sp->tp,sp2->tp))
				return 0;
			sp=sp->next; sp2=sp2->next;
		}
		if (sp || sp2) return 0;	// != # of args
		return eq_type(tp1->btp, tp2->btp);
	}
	if (tp1->type == bt_struct || tp1->type == bt_union)
		return (tp1->lst.head == tp2->lst.head);

	return 1;
}

int indir_num(TYP *tp) {
	int n=0;
	while (tp->type == bt_pointer || tp->type == bt_func) {
		n++;
		tp=tp->btp;
	}
	return n;
}

static TYP *save_type(TYP *tp) {
	TYP 		   *ret_tp;
/* the type structure referenced by tp may be in local tables.
   Copy it to global tables and return a pointer to it.
 */
	if (tp->st_flag)
		return tp;
	++global_flag;
	ret_tp = (TYP *) xalloc((int) sizeof(TYP), _TYP+SV_TYPE);

/* copy TYP structure */
	*ret_tp = *tp;
	ret_tp->st_flag = 1;

	if (tp->type == bt_func || tp->type == bt_pointer)
		ret_tp->btp = save_type(tp->btp);

	if (tp->type == bt_struct || tp->type == bt_union) {
		/*
		 * It consumes very much memory to duplicate the symbol table, so we
		 * don't do it. I think we needn't do it if it is in local tables, so
		 * this warning is perhaps meaningless.
		 */
//		uwarn("didn't copy local struct/union in save_type");
		ret_tp->lst.tail = ret_tp->lst.head = 0; ret_tp->lst.hash = 0;
	}
	--global_flag;
	return ret_tp;
}
// vim:ts=4:sw=4
