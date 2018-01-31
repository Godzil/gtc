/*
 * GTools C compiler
 * =================
 * source file :
 * keyword searching
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

xstatic readonly struct kwblk {
	char		   *word;
	enum(e_sym)		stype;
#ifndef PC
	int pad;
#endif
}				keywords[] = {
	{
		"__asm__", kw_asm
	}, {
		"__attribute__", kw_attr
	}, {
		"__builtin_constant_p", kwb_constant_p
	}, {
		"__c__", kw_c
	}, {
		"__count__", kw_count
	}, {
		"__eval__", kw_eval
	}, {
		"__loop__", kw_loop
	}, {
		"__softcast__", kw_softcast
	}, {
		"__until__", kw_until
	}, {
		"alloca", kw_alloca
	}, {
		"asm", kw_asm
	}, {
		"auto", kw_auto
	}, {
		"break", kw_break
	}, {
		"case", kw_case
	}, {
		"char", kw_char
	}, {
		"const", kw_const
	}, {
		"continue", kw_continue
	}, {
		"default", kw_default
	}, {
		"defined", kw_defined
	}, {
		"do", kw_do
	}, {
		"double", kw_double
	}, {
		"else", kw_else
	}, {
		"enum", kw_enum
	}, {
		"extern", kw_extern
	}, {
		"float", kw_float
	}, {
		"for", kw_for
	}, {
		"goto", kw_goto
	}, {
		"if", kw_if
	}, {
		"incbin", kw_incbin
	}, {
		"int", kw_int
	}, {
		"long", kw_long
	}, {
		"loop", kw_loop
	}, {
		"register", kw_register
	}, {
		"return", kw_return
	}, {
		"short", kw_short
	}, {
		"signed", kw_signed
	}, {
		"sizeof", kw_sizeof
	}, {
		"static", kw_static
	}, {
		"struct", kw_struct
	}, {
		"switch", kw_switch
	}, {
		"typedef", kw_typedef
	}, {
		"typeof", kw_typeof
	}, {
		"union", kw_union
	}, {
		"unsigned", kw_unsigned
	}, {
		"until", kw_until
	}, {
		"void", kw_void
	}, {
		"volatile", kw_volatile
	}, {
		"while", kw_while
/*	}, {
		0, 0*/
	}
};

#define kw_N 48

/*
 * Dichotomic search allows a max of 6 comparisons instead of 50...
 */
void searchkw(void) {
	char *s1,*s2,c;
	const struct kwblk *kwbp;
	int a=0,b=kw_N-1,m;
#ifdef PC
	if (sizeof(keywords)/sizeof(struct kwblk)!=kw_N)
		fatal("BUILD ERROR: INVALID KEYWORDS #");
#endif
	while (a<=b) {
		m=(a+b)>>1;
		kwbp=&keywords[m];
		s1=lastid;
		s2=kwbp->word;
		do {
			if (!(c=*s2++)) {
				if (!*s1)
					lastst = kwbp->stype;
				if (is_lang_ext(lastst) && lastid[0]!='_' && !(flags&X_LANG_EXT))
					lastst = id;
				if (*s1!='u')	// the only case when a kw is the beginning of another one
					return;		// is 'do' vs 'double'
			}
		} while (*s1++==c);
		if (s1[-1]<c)	// il faut aller plus haut
			b=m-1;
		else a=m+1;
	}
}
/*searchkw() // non-dichotomic version
{
	char *s1,*s2,c;
	struct kwblk *kwbp;
	kwbp = keywords;
	while (kwbp->word != 0) {
		s1 = lastid;
		s2 = kwbp->word;
		kwbp++;
		do {
			if (!(c=*s2++)) {
				if (!*s1++)
					lastst = (--kwbp)->stype;
				return;
			}
		} while (*s1++==c);
	}
}*/
// vim:ts=4:sw=4
