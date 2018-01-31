/*
 * GTools C compiler
 * =================
 * source file :
 * (on-calc) volatilizing a value
 *
 * Copyright 2001-2004 Paul Froissart.
 * Credits to Christoph van Wuellen and Matthew Brandt.
 * All commercial rights reserved.
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 */

#ifndef __IDENTITY
#define __IDENTITY
void *identity(void *x) {
	return x;
}
#endif
// vim:ts=4:sw=4
