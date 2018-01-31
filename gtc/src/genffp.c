/*
 * GTools C compiler
 * =================
 * source file :
 * Fast Floating-Point generator
 *
 * Copyright 2001-2004 Paul Froissart.
 * Credits to Christoph van Wuellen and Matthew Brandt.
 * All commercial rights reserved.
 *
 * This compiler may be redistributed as long there is no
 * commercial interest. The compiler must not be redistributed
 * without its full sources. This notice must stay intact.
 */

/*
 * The following floating-point operations are needed in this module:
 *
 * - comparision with 0.0, 0.5 and 1.0 - division by 2.0 - multiplication with
 * 2.0 (performed as addition here) - subtraction of 1.0
 */

#ifndef NOFLOAT
#ifdef PC
#include	"define.h"
_FILE(__FILE__)
#ifndef BCDFLT
unsigned long double2ffp(double d) {
	unsigned long	mantissa;
	int 			sign = 0, exponent = 64, i;

	if (d < 0.0) {
		sign = 128;
		d = -d;
	}
	while (d < 0.5) {
		d += d;
		--exponent;
		if (exponent == 0)
			return sign;		/* zero fp number */
	}

	while (d >= 1.0) {
		d /= 2.0;
		++exponent;
		if (exponent >= 127)
			return 127 + sign;	/* +/- infinity */
	}

	/* 0.5 <=d <1.0 now: construct the mantissa */

	mantissa = 0;
	for (i = 0; i < 24; i++) {
		/* 24 mantissa bits */
		d += d;
		mantissa = mantissa + mantissa;
		if (d >= 1.0) {
			++mantissa;
			d -= 1.0;
		}
	}

	/* round up, if the next bit would be 1 */
	if (d >= 0.5)
		++mantissa;
	/* check on mantissa overflow */
	if (mantissa > 0xFFFFFF) {
		++exponent;
		/* exponent overflow? */
		if (exponent >= 127)
			return (127 + sign);
		mantissa >>= 1;
	}
	/* put the parts together and return the value */

	return (mantissa << 8) + sign + exponent;
}
#else
void double2bcd(double d,struct bcd *bcd) {
	unsigned char *mantptr=bcd->mantissa;
	int bias = 16384, exponent = 0, i;

	if (d < 0.0) {
		bias += 32768;
		d = -d;
	}
	if (d != 0.0) {
		while (d < 1.0) {
			d *= 10.0;
			--exponent;
			if (exponent < -999) {
				d = 0.0;
				break;
			}
		}
	}

	while (d >= 10.0) {
		d /= 10.0;
		++exponent;
	}

	if (d==0.0) {
		bcd->exponent=16384;
		memset(bcd->mantissa,0,BCDLEN);
		return;
	}

	/* 1.0 <= d < 10.0 now: construct the mantissa */

	d += 5e-16;	/* round up now */
	for (i = 0; i < BCDLEN; i++) {
		unsigned char digit,buffer;
		/* 8 mantissa groups of 2 digits */
		if (d>=5.0) {
			if (d>=7.0) {
				if (d>=8.0) {
					if (d>=9.0)
						digit=9;
					else digit=8;
				} else digit=7;
			} else if (d>=6.0)
				digit=6;
			else digit=5;
		} else {
			if (d>=2.0) {
				if (d>=3.0) {
					if (d>=4.0)
						digit=4;
					else digit=3;
				} else digit=2;
			} else if (d>=1.0)
				digit=1;
			else digit=0;
		}
		d -= digit;
		d *= 10.0;
		buffer = digit<<4;
		if (d>=5.0) {
			if (d>=7.0) {
				if (d>=8.0) {
					if (d>=9.0)
						digit=9;
					else digit=8;
				} else digit=7;
			} else if (d>=6.0)
				digit=6;
			else digit=5;
		} else {
			if (d>=2.0) {
				if (d>=3.0) {
					if (d>=4.0)
						digit=4;
					else digit=3;
				} else digit=2;
			} else if (d>=1.0)
				digit=1;
			else digit=0;
		}
		d -= digit;
		d *= 10.0;
		*mantptr++ = buffer | digit;
	}

	/* put the parts together and return the value */
	bcd->exponent = exponent+bias;

	return;
}

#endif /* !defined(BCDFLT) */
#endif /* defined(PC) */
#endif /* !defined(NOFLOAT) */
// vim:ts=4:sw=4
