/*
 * GTools C compiler
 * =================
 * source file :
 * optimization
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

static	void fold_const();

void dooper(struct enode **node) {
/*
 * dooper will execute a constant operation in a node and modify the node to
 * be the result of the operation.
 */
	struct enode   *ep = *node;
	enum(e_node) 	type = ep->nodetype;

#define ep0 ep->v.p[0]
#define ep1 ep->v.p[1]
#define epi ep->v.i
#define epf ep->v.f
#define ulong unsigned long

	ep->nodetype = ep0->nodetype;
	if (ep0->nodetype == en_fcon) {
#ifndef NOFLOAT
		switch (type) {
		  case en_uminus:
#ifdef PC
			epf = - ep0->v.f;
#else
			epf = ep0->v.f;
			if (epf) epf^=0x80;
#endif
			break;
		  case en_not:
			ep->nodetype = en_icon;
			epi = (ep0->v.f) ? 0 : 1;
			break;
		  case en_add:
#ifdef PC
			epf = ep0->v.f + ep1->v.f;
#else
			epf = ffpadd(ep0->v.f, ep1->v.f);
#endif
			break;
		  case en_sub:
#ifdef PC
			epf = ep0->v.f - ep1->v.f;
#else
			epf = ffpsub(ep0->v.f, ep1->v.f);
#endif
			break;
		  case en_mul:
#ifdef PC
			epf = ep0->v.f * ep1->v.f;
#else
			epf = ffpmul(ep0->v.f, ep1->v.f);
#endif
			break;
		  case en_div:
			if (ep1->v.f == 0) {
				uwarn("division by zero");
				ep->nodetype = en_div;
			} else {
#ifdef PC
				epf = ep0->v.f / ep1->v.f;
#else
				epf = ffpdiv(ep0->v.f, ep1->v.f);
#endif
			}
			break;
		  case en_eq:
			ep->nodetype = en_icon;
			epi = (ep0->v.f == ep1->v.f) ? 1 : 0;
			break;
		  case en_ne:
			ep->nodetype = en_icon;
			epi = (ep0->v.f != ep1->v.f) ? 1 : 0;
			break;
		  case en_land:
			ep->nodetype = en_icon;
			epi = (ep0->v.f && ep1->v.f) ? 1 : 0;
			break;
		  case en_lor:
			ep->nodetype = en_icon;
			epi = (ep0->v.f || ep1->v.f) ? 1 : 0;
			break;
		  case en_lt:
			ep->nodetype = en_icon;
#ifdef PC
			epi = (ep0->v.f < ep1->v.f) ? 1 : 0;
#else
			epi = (ffpcmp_c(ep0->v.f, ep1->v.f) < 0) ? 1 : 0;
#endif
			break;
		  case en_le:
			ep->nodetype = en_icon;
#ifdef PC
			epi = (ep0->v.f <= ep1->v.f) ? 1 : 0;
#else
			epi = (ffpcmp_c(ep0->v.f, ep1->v.f) <= 0) ? 1 : 0;
#endif
			break;
		  case en_gt:
			ep->nodetype = en_icon;
#ifdef PC
			epi = (ep0->v.f > ep1->v.f) ? 1 : 0;
#else
			epi = (ffpcmp_c(ep0->v.f, ep1->v.f) > 0) ? 1 : 0;
#endif
			break;
		  case en_ge:
			ep->nodetype = en_icon;
#ifdef PC
			epi = (ep0->v.f >= ep1->v.f) ? 1 : 0;
#else
			epi = (ffpcmp_c(ep0->v.f, ep1->v.f) >= 0) ? 1 : 0;
#endif
			break;
		  default:
			ep->nodetype = type;
			iwarn(DOOPER,1);
			break;
		}
#endif /* NOFLOAT */
		return;
	}
	/*
	 * Thus, ep0->nodetype is en_icon
	 * We have to distinguish unsigned long from the other cases
	 *
	 * Since we always store in ep->v.i, it is
	 * ASSUMED THAT (long) (ulong) ep->v.i == ep->v.i always
	 */
	if (ep0->etype == bt_ulong || ep0->etype == bt_pointer) {
		switch (type) {
		  case en_uminus:
			epi = - ep0->v.i;
			break;
		  case en_not:
			epi = ((ulong) ep0->v.i) ? 0 : 1;
			break;
		  case en_compl:
			epi = ~ (ulong) ep0->v.i;
			epi = strip_icon(epi, ep->etype);
			break;
		  case en_add:
			epi = (ulong) ep0->v.i + (ulong) ep1->v.i;
			break;
		  case en_sub:
			epi = (ulong) ep0->v.i - (ulong) ep1->v.i;
			break;
		  case en_mul:
			epi = (ulong) ep0->v.i * (ulong) ep1->v.i;
			break;
		  case en_div:
			if ((ulong) ep1->v.i == 0) {
				uwarn("division by zero");
				ep->nodetype = en_div;
			} else {
				epi = (ulong) ep0->v.i / (ulong) ep1->v.i;
			}
			break;
		  case en_mod:
			if ((ulong) ep1->v.i == 0) {
				uwarn("division by zero");
				ep->nodetype = en_mod;
			} else {
				epi = (ulong) ep0->v.i % (ulong) ep1->v.i;
			}
			break;
		  case en_and:
			epi = (ulong) ep0->v.i & (ulong) ep1->v.i;
			break;
		  case en_or:
			epi = (ulong) ep0->v.i | (ulong) ep1->v.i;
			break;
		  case en_xor:
			epi = (ulong) ep0->v.i ^ (ulong) ep1->v.i;
			break;
		  case en_eq:
			epi = ((ulong) ep0->v.i == (ulong) ep1->v.i) ? 1 : 0;
			break;
		  case en_ne:
			epi = ((ulong) ep0->v.i != (ulong) ep1->v.i) ? 1 : 0;
			break;
		  case en_land:
			epi = ((ulong) ep0->v.i && (ulong) ep1->v.i) ? 1 : 0;
			break;
		  case en_lor:
			epi = ((ulong) ep0->v.i || (ulong) ep1->v.i) ? 1 : 0;
			break;
		  case en_lt:
			epi = ((ulong) ep0->v.i < (ulong) ep1->v.i) ? 1 : 0;
			break;
		  case en_le:
			epi = ((ulong) ep0->v.i <= (ulong) ep1->v.i) ? 1 : 0;
			break;
		  case en_gt:
			epi = ((ulong) ep0->v.i > (ulong) ep1->v.i) ? 1 : 0;
			break;
		  case en_ge:
			epi = ((ulong) ep0->v.i >= (ulong) ep1->v.i) ? 1 : 0;
			break;
		  case en_lsh:
#ifdef MINIMAL_SIZES
			epi = (ulong) ep0->v.i << (char) ep1->v.i;
#else
			epi = (ulong) ep0->v.i << (ulong) ep1->v.i;
#endif
			break;
		  case en_rsh:
#ifdef MINIMAL_SIZES
			epi = (ulong) ep0->v.i >> (char) ep1->v.i;
#else
			epi = (ulong) ep0->v.i >> (ulong) ep1->v.i;
#endif
			break;
		  default:
			ep->nodetype = type;
			iwarn(DOOPER,2);
			break;
		}
	} else {
		switch (type) {
		  case en_uminus:
			epi = - ep0->v.i;
			break;
		  case en_not:
			epi = (ep0->v.i) ? 0 : 1;
			break;
		  case en_compl:
			epi = ~ ep0->v.i;
			epi = strip_icon(epi, type);
			break;
		  case en_add:
			epi = ep0->v.i + ep1->v.i;
			break;
		  case en_sub:
			epi = ep0->v.i - ep1->v.i;
			break;
		  case en_mul:
			epi = ep0->v.i * ep1->v.i;
			break;
		  case en_div:
			if (ep1->v.i == 0) {
				uwarn("division by zero");
				ep->nodetype = en_div;
			} else {
				epi = ep0->v.i / ep1->v.i;
			}
			break;
		  case en_mod:
			if (ep1->v.i == 0) {
				uwarn("division by zero");
				ep->nodetype = en_mod;
			} else {
				epi = ep0->v.i % ep1->v.i;
			}
			break;
		  case en_and:
			epi = ep0->v.i & ep1->v.i;
			break;
		  case en_or:
			epi = ep0->v.i | ep1->v.i;
			break;
		  case en_xor:
			epi = ep0->v.i ^ ep1->v.i;
			break;
		  case en_eq:
			epi = (ep0->v.i == ep1->v.i) ? 1 : 0;
			break;
		  case en_ne:
			epi = (ep0->v.i != ep1->v.i) ? 1 : 0;
			break;
		  case en_land:
			epi = (ep0->v.i && ep1->v.i) ? 1 : 0;
			break;
		  case en_lor:
			epi = (ep0->v.i || ep1->v.i) ? 1 : 0;
			break;
		  case en_lt:
			epi = (ep0->v.i < ep1->v.i) ? 1 : 0;
			break;
		  case en_le:
			epi = (ep0->v.i <= ep1->v.i) ? 1 : 0;
			break;
		  case en_gt:
			epi = (ep0->v.i > ep1->v.i) ? 1 : 0;
			break;
		  case en_ge:
			epi = (ep0->v.i >= ep1->v.i) ? 1 : 0;
			break;
		  case en_lsh:
#ifdef MINIMAL_SIZES
			epi = ep0->v.i << (char)ep1->v.i;
#else
			epi = ep0->v.i << ep1->v.i;
#endif
			break;
		  case en_rsh:
#ifdef MINIMAL_SIZES
			epi = ep0->v.i >> (char)ep1->v.i;
#else
			epi = ep0->v.i >> ep1->v.i;
#endif
			break;
		  default:
			ep->nodetype = type;
			iwarn(DOOPER,3);
			break;
		}
	}
#undef ep0
#undef ep1
#undef epi
#undef epf
#undef ulong
}

int pwrof2(long i) {
/*
 * return which power of two i is or -1.
 */
	int 			p;
	long			q;
	q = 2;
	p = 1;
	while (q > 0) {
		if (q == i)
			return p;
		q <<= 1;
		++p;
	}
	return -1;
}

long mod_mask(long i) {
/*
 * make a mod mask for a power of two.
 */
	long			m;
	m = 0;
	while (i--)
		m = (m << 1) | 1;
	return m;
}

void opt0(struct enode **node) {
/*
 * opt0 - delete useless expressions and combine constants.
 *
 * opt0 will delete expressions such as x + 0,
 *	x - 0,
 *	x * 0,
 *	x * 1,
 *	0 / x,
 *	x / 1,
 *	x % (1<<3),
 *	etc from the tree pointed to by node and combine obvious
 * constant operations. It cannot combine name and label constants but will
 * combine icon type nodes.
 */
	struct enode   *ep;

#define ep0 ep->v.p[0]
#define ep1 ep->v.p[1]

	long			val, sc;
	enum(e_node) 	typ;
	ep = *node;
	if (ep == 0)
		return;
	typ = ep->nodetype;
	switch (typ) {
	  case en_ref:
	  case en_fieldref:
	  case en_ainc:
	  case en_adec:
	  case en_deref:
		opt0(&ep0);
		break;
	  case en_uminus:
	  case en_compl:
		opt0(&ep0);
		/*
		 * This operation applied twice is a no-op
		 */
		if (ep0->nodetype == typ) {
			*node = ep0->v.p[0];
			break;
		}
		if (ep0->nodetype == en_icon || ep0->nodetype == en_fcon)
			dooper(node);
		break;
	  case en_not:
		opt0(&ep0);
		if (ep0->nodetype == en_icon || ep0->nodetype == en_fcon)
			dooper(node);
		break;
	  case en_add:
	  case en_sub:
		opt0(&ep0);
		opt0(&ep1);
		/*
		 *	a + (-b)  =  a - b
		 *	a - (-b)  =  a + b
		 *	(-a) + b  =  b - a
		 */
		if (ep1->nodetype == en_uminus) {
			ep1 = ep1->v.p[0];
			ep->nodetype = typ = (typ == en_add) ? en_sub : en_add;
		}
		if (ep0->nodetype == en_uminus && typ == en_add) {
			ep0 = ep0->v.p[0];
			swap_nodes(ep);
			ep->nodetype = typ = en_sub;
		}
		/*
		 * constant expressions
		 */
		if ((ep0->nodetype == en_icon && ep1->nodetype == en_icon) ||
			(ep0->nodetype == en_fcon && ep1->nodetype == en_fcon))   {
			dooper(node);
			break;
		}
		/*infunc("DrawNumberOfClock")
			bkpt();*/
		/* the following would bug, as :
		    short my_array[2]={12,570};
		    int i=2;
		    while (i--) printf("%d",my_array[i]);
		  wouldn't print 570 correctly... */
#ifdef BUGGY_AND_ANYWAY_NOT_COMPAT_WITH_NO_OFFSET_AUTOCON
		if (typ == en_add) {
			if (ep1->nodetype == en_autocon)
				swap_nodes(ep);
		}
#endif
		if (ep0->nodetype == en_icon) {
			if (ep0->v.i == 0) {
				if (typ == en_sub) {
					ep0 = ep1;
					ep->nodetype = typ = en_uminus;
				} else
					*node = ep1;
				break;
#ifdef PREFER_POS_VALUES
			} else if (ep0->nodetype==en_add && ep0->v.i<0) {
				ep0->v.i=-ep0->v.i;
				swap_nodes(ep);
				ep->nodetype=en_sub;
#endif
			}
		} else if (ep1->nodetype == en_icon) {
#ifdef BUGGY_AND_ANYWAY_NOT_COMPAT_WITH_NO_OFFSET_AUTOCON
			if (ep0->nodetype == en_autocon && typ == en_add) {
				ep0->v.i += ep1->v.i;
				*node = ep0;
				break;
			}
#endif
			if (ep1->v.i == 0) {
				*node = ep0;
				break;
#ifdef PREFER_POS_VALUES
			} else if (ep1->v.i<0) {
				ep->nodetype^=1;
				ep1->v.i=-ep1->v.i;
#endif
			}
		}
		break;
	  case en_mul:
		opt0(&ep0);
		opt0(&ep1);
		/*
		 * constant expressions
		 */
		if ((ep0->nodetype == en_icon && ep1->nodetype == en_icon) ||
			(ep0->nodetype == en_fcon && ep1->nodetype == en_fcon))  {
			dooper(node);
			break;
		}
		if (ep0->nodetype == en_icon) {
			val = ep0->v.i;
			if (val == 0) {
				*node = ep0;
				break;
			}
			if (val == 1) {
				*node = ep1;
				break;
			}
			sc = pwrof2(val);
			if (sc IS_VALID) {
				swap_nodes(ep);
				ep1->v.i = sc;
				ep->nodetype = en_lsh;
			}
		} else if (ep1->nodetype == en_icon) {
			val = ep1->v.i;
			if (val == 0) {
				*node = ep1;
				break;
			}
			if (val == 1) {
				*node = ep0;
				break;
			}
			sc = pwrof2(val);
			if (sc IS_VALID) {
				ep1->v.i = sc;
				ep->nodetype = en_lsh;
			}
		}
		break;
	  case en_div:
		opt0(&ep0);
		opt0(&ep1);
		/*
		 * constant expressions
		 */
		if ((ep0->nodetype == en_icon && ep1->nodetype == en_icon) ||
			(ep0->nodetype == en_fcon && ep1->nodetype == en_fcon))  {
			dooper(node);
			break;
		}
		if (ep0->nodetype == en_icon) {
			if (ep0->v.i == 0) {		/* 0/x */
				*node = ep0;
				break;
			}
		} else if (ep1->nodetype == en_icon) {
			val = ep1->v.i;
			if (val == 1) { 	/* x/1 */
				*node = ep0;
				break;
			}
			sc = pwrof2(val);
			if (sc IS_VALID) {
				ep1->v.i = sc;
				ep->nodetype = en_rsh;
			}
		}
		break;
	  case en_mod:
		opt0(&ep0);
		opt0(&ep1);
		/*
		 * constant expressions
		 */
		if ((ep0->nodetype == en_icon && ep1->nodetype == en_icon) ||
			(ep0->nodetype == en_fcon && ep1->nodetype == en_fcon))  {
			dooper(node);
			break;
		}
		if (ep1->nodetype == en_icon) {
			sc = pwrof2(ep1->v.i);
			if (sc IS_VALID) {
				ep1->v.i = mod_mask(sc);
				ep->nodetype = en_and;
			}
		}
		break;
	  case en_land:
	  case en_lor:
#if 0	/* perhaps this isn't *that* useful... and ep0/ep1 are long to access */
		opt0(&ep0);
		opt0(&ep1);
		if ((ep0->nodetype == en_icon && ep1->nodetype == en_icon) ||
			(ep0->nodetype == en_fcon && ep1->nodetype == en_fcon))
			dooper(node);
		else {
			typ = (typ==en_lor);
			if (ep0->nodetype == en_icon) {
				if ((!ep0->v.i)!=typ)	/* absorbing element */
					ep = ep0;
				else	/* "neutral" element */
					//ep = ep1;	-> not quite, should be !!x
					(void)0;
			} else if (ep1->nodetype == en_icon) {
				if ((!ep1->v.i)!=typ)	/* absorbing element */
					ep->nodetype = en_void, ep1->v.i=typ;
				else	/* "neutral" element */
					//ep = ep0;	-> not quite, should be !!x
					(void)0;
			}
		}
		break;
#else
		/* FALL THROUGH */
#endif
	  case en_and:
	  case en_or:
	  case en_xor:
	  case en_eq:
	  case en_ne:
	  case en_lt:
	  case en_le:
	  case en_gt:
	  case en_ge:
		opt0(&ep0);
		opt0(&ep1);
		/*
		 * constant expressions
		 */
		if ((ep0->nodetype == en_icon && ep1->nodetype == en_icon) ||
			(ep0->nodetype == en_fcon && ep1->nodetype == en_fcon))
			dooper(node);
		break;
	  case en_lsh:
	  case en_rsh:
		opt0(&ep0);
		opt0(&ep1);
		if (ep0->nodetype == en_icon && ep1->nodetype == en_icon) {
			dooper(node);
			break;
		}
		/*
		 * optimize a << 0 and a >> 0
		 */
		if (ep1->nodetype == en_icon && ep1->v.i == 0)
			*node = ep0;
		break;
	  case en_cond:
		opt0(&ep0);
		opt0(&ep1);
		if (ep0->nodetype == en_icon) {
			if (ep0->v.i) {
				if (ep1->v.p[0]) *node=ep1->v.p[0];
				else *node=ep0;
			} else
				*node=ep1->v.p[1];
		}
		break;
	  case en_asand:
	  case en_asor:
	  case en_asxor:
	  case en_asadd:
	  case en_assub:
	  case en_asmul:
	  case en_asdiv:
	  case en_asmod:
	  case en_asrsh:
	  case en_aslsh:
	  case en_fcall:
	  case en_void:
	  case en_assign:
		opt0(&ep0);
		opt0(&ep1);
		break;
		/* now handled in expr.c */
	  case en_cast:
		opt0(&ep0);
		if (ep0->nodetype == en_icon) {
#ifndef NOFLOAT
			if (ep->etype != bt_float
#ifdef DOUBLE
				&& ep->etype != bt_double
#endif
					) {
#endif
				ierr(OPT0,1);
/*				ep->nodetype = en_icon;
				ep->v.i = ep0->v.i;*/
#ifndef NOFLOAT
			} else {
#ifdef XCON_DOESNT_SUPPORT_FLOATS
#error Fix me now :)
#endif
				ep->nodetype = en_fcon;
#ifdef PC
				ep->v.f = (double)ep0->v.i;
#else
#ifndef BCDFLT
				ep->v.f = ffpltof(ep0->v.i);
#else
				ep->v.f = (double)ep0->v.i;
#endif
#endif
			}
#endif
		}
		/* perhaps BUGGY */
		else if ((ep0->nodetype == en_labcon || ep0->nodetype == en_nacon)
			&& ep->esize<=ep0->esize) {
			ep0->etype = ep->etype;
			ep0->esize = ep->esize;
			ep=ep0;
			*node=ep;
		}
#ifndef NOFLOAT
		else if (ep0->nodetype == en_fcon && bt_integral(ep->etype)) {
			ep->nodetype = en_icon;
#ifdef PC
			ep->v.i = (long)ep0->v.f;
#else
			ep->v.i = ffpftol(ep0->v.f);
#endif
		}
#endif
		break;
	  /*case en_icon:
	  case en_fcon:
	  case en_labcon:
	  case en_nacon:
	  case en_autocon:
	  case en_tempref:
		break;
	  default:
		uwarn("Didn't optimize nodetype %d. Please send me your source code.",typ);*/
	}

#undef ep0
#undef ep1
}

static long xfold(struct enode *node) {
/*
 * xfold will remove constant nodes and return the values to the calling
 * routines.
 */
	long			i;
	if (node == 0)
		return 0;
	switch (node->nodetype) {
	  case en_icon:
		i = node->v.i;
		node->v.i = 0;
		return i;
	  case en_add:
		return xfold(node->v.p[0]) + xfold(node->v.p[1]);
	  case en_sub:
		return xfold(node->v.p[0]) - xfold(node->v.p[1]);
	  case en_mul:
		if (node->v.p[0]->nodetype == en_icon)
			return xfold(node->v.p[1]) * node->v.p[0]->v.i;
		else if (node->v.p[1]->nodetype == en_icon)
			return xfold(node->v.p[0]) * node->v.p[1]->v.i;
		else {
			fold_const(&node->v.p[0]);
			fold_const(&node->v.p[1]);
			return 0;
		}
		/*
		 * CVW: This seems wrong to me... case en_lsh: if(
		 * node->v.p[0]->nodetype == en_icon ) return xfold(node->v.p[1]) <<
		 * node->v.p[0]->v.i; else if( node->v.p[1]->nodetype == en_icon )
		 * return xfold(node->v.p[0]) << node->v.p[1]->v.i; else return 0;
		 */
	  case en_uminus:
		return -xfold(node->v.p[0]);
	  case en_lsh:
	  case en_rsh:
	  case en_div:
	  case en_mod:
	  case en_asadd:
	  case en_assub:
	  case en_asmul:
	  case en_asdiv:
	  case en_asmod:
	  case en_and:
	  case en_land:
	  case en_or:
	  case en_lor:
	  case en_xor:
	  case en_asand:
	  case en_asor:
	  case en_asxor:
	  case en_void:
	  case en_fcall:
	  case en_assign:
	  case en_eq:
	  case en_ne:
	  case en_lt:
	  case en_le:
	  case en_gt:
	  case en_ge:
		fold_const(&node->v.p[0]);
		fold_const(&node->v.p[1]);
		return 0;
	  case en_ref:
	  case en_fieldref:
	  case en_compl:
	  case en_not:
	  case en_deref:
		fold_const(&node->v.p[0]);
		return 0;
		/*
		 * This is not stricly legal: (long)(x+10) * 4l might not be the same
		 * as (long)(x) * 4l + 40l but it is the same as long as no overflows
		 * occur
		 */
	  case en_cast:
#ifdef DONTDEF
		return xfold(node->v.p[0]);
#endif
		/*
		 * Well, sometimes I prefer purity to efficiency
		 * It is a matter of taste how you decide here....
		 */
		fold_const(&node->v.p[0]);
		return 0;
	}
	return 0;
}

static void fold_const(struct enode **node) {
/*
 * reorganize an expression for optimal constant grouping.
 */
	struct enode   *ep;
	long			i;
	ep = *node;
	if (ep == 0)
		return;
	if (ep->nodetype == en_add) {
		if (ep->v.p[0]->nodetype == en_icon) {
			ep->v.p[0]->v.i += xfold(ep->v.p[1]);
			return;
		} else if (ep->v.p[1]->nodetype == en_icon) {
			ep->v.p[1]->v.i += xfold(ep->v.p[0]);
			return;
		}
	} else if (ep->nodetype == en_sub) {
		if (ep->v.p[0]->nodetype == en_icon) {
			ep->v.p[0]->v.i -= xfold(ep->v.p[1]);
			return;
		} else if (ep->v.p[1]->nodetype == en_icon) {
			ep->v.p[1]->v.i -= xfold(ep->v.p[0]);
			return;
		}
	}
	i = xfold(ep);
	if (i != 0) {
/*
 * strip_icon is in fact harmless here since this value is
 * just added to *node
 * consider in 16-bit mode:
 *
 * int day, year;
 * day = 365 * (year - 1970);
 *
 * and look at the code, which is transformed to
 *
 * day = 365*year + 1846;
 *
 * which works if the multiplication returns the lower 16 bits of
 * the result correctly.
 */
		i = strip_icon(i, (*node)->etype);
		ep = mk_icon(i);
		ep->etype = (*node)->etype;
		ep->esize = (*node)->esize;
		ep = mk_node(en_add, *node, ep);
		ep->etype = (*node)->etype;
		ep->esize = (*node)->esize;
		*node = ep;
	}
}

void opt4(struct enode **node) {
/*
 * apply all constant optimizations.
 */
	/* opt0(node); */
	fold_const(node);
	opt0(node);
}
// vim:ts=4:sw=4
