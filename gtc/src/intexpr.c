/*
 * GTools C compiler
 * =================
 * source file :
 * integer expression routines
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
#ifndef NOFLOAT
#include	"ffplib.h"
#endif

#ifndef NOFLOAT
double floatexpr() {
/* floating point expression */
	struct enode   *ep;
	struct typ	   *tp;

	tp = exprnc(&ep);
	if (tp == 0) {
		error(ERR_FPCON);
		return (double) 0;
	}

	opt0(&ep);

	if (ep->nodetype == en_icon)
#ifdef PC
		return (double) ep->v.i;
#else
		return ffpltof(ep->v.i);
#endif

	if (ep->nodetype == en_fcon)
		return ep->v.f;

	error (ERR_SYNTAX);
	return (double) 0;
}
#endif /* !NOFLOAT */

long intexpr() {
	struct enode   *ep;
	struct typ	   *tp;
	long val;

	tmp_use();
	tp = exprnc(&ep);
	if (tp == 0) {
		error(ERR_INTEXPR);
		return 0;
	}
	opt0(&ep);

	if (ep->nodetype != en_icon) {
		error(ERR_SYNTAX);
		return 0;
	}
	val=ep->v.i;
	tmp_free();
	return val;
}

#if 0
long intexpr_notemp() {
	struct enode   *ep;
	struct typ	   *tp;

	tp = exprnc(&ep);
	if (tp == 0) {
		error(ERR_INTEXPR);
		return 0;
	}
	opt0(&ep);

	if (ep->nodetype != en_icon) {
		error(ERR_SYNTAX);
		return 0;
	}
	return ep->v.i;
}
#endif
// vim:ts=4:sw=4
