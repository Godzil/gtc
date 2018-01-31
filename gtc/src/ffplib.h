/*
 * GTools C compiler
 * =================
 * source file :
 * Fast Floating Point routines
 *
 * Copyright 2001-2004 Paul Froissart.
 * Credits to Christoph van Wuellen and Matthew Brandt.
 * All commercial rights reserved.
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 */

#ifndef FFPLIB_H
#define FFPLIB_H
#ifndef str
#define STR(__x) #__x
#define str(__y) STR(__y)
#endif

#define ffpver ffplib__0000
#define ffpadd ffplib__0001
#define ffpsub ffplib__0002
#define ffpcmp ffplib__0003
#define ffpcmp_c ffplib__0004
#define ffpipart ffplib__0005
#define ffputof ffplib__0006
#define ffpltof ffplib__0007
#define ffpftou ffplib__0008
#define ffpftol ffplib__0009
#define ffpmul ffplib__000A
#define ffpmulxp ffplib__000B
#define ffpsqr ffplib__000C
#define ffpdiv ffplib__000D
#define ffpftoa ffplib__000E
#define ffpftobcd ffplib__000F
#define ffplog ffplib__0010
#define ffplog2 ffplib__0011
#define ffpexp ffplib__0012
#define ffpexp2 ffplib__0013
#define ffppwr ffplib__0014
#define ffpsincos ffplib__0015
#define ffpsin ffplib__0016
#define ffpcos ffplib__0017
#define ffptan ffplib__0018
#define sqrt ffplib__0019

double ffpadd(double a,double b);
double ffpsub(double a,double b);
void ffpcmp(double a,double b);
long ffpcmp_c(double a,double b);
double ffpipart(double a);
double ffputof(unsigned long x);
double ffpltof(long x);
unsigned long ffpftou(double a);		// + round
long ffpftol(double a);					// + round / floor
double ffpmul(double a,double b);
double ffpdiv(double a,double b);
void ffpftoa(double a,char *s);
void ffpftobcd(double a,void *d);
double ffplog(double a);
double ffplog2(double a);
double ffpexp(double a);
double ffpexp2(double a);
double ffppwr(double a,double b);
// {d0:double, d1:double} ffpsincos(double a);
double ffpsin(double a);
double ffpcos(double a);
double ffptan(double a);
int sqrt(long x);
#endif
// vim:ts=4:sw=4
