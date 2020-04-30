/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "misc/error.h"
#include "fix/fix.h"

extern uint8_t guess_table[];
extern short sincos_table[];
extern uint16_t asin_table[];
extern uint16_t acos_table[];
extern fix isqrt_guess_table[];

//negate a quad
void fixquadnegate(quad* q)
{
	q->low = 0 - q->low;
	q->high = 0 - q->high - (q->low != 0);
}

//multiply two ints & add 64-bit result to 64-bit sum
void fixmulaccum(int64_t*q, fix a, fix b)
{
	int64_t sum;

	sum = (int64_t)a * (int64_t)b;
	*q += sum;
}

//extract a fix from a quad product

//parabolicus's version
fix fixquadadjust(int64_t q)
{
	fix v = (fix)(q >> 16);
	int32_t vh = q >> 48;
	int signb = vh < 0;
	int signv = v < 0;
	if (signb != signv)
	{
		v = (fix)0x7FFFFFFF;
		if (signb) v = -v;
	}
	return v;
}

fix fixmul(fix a, fix b)
{
	/*	"imul	edx"				\
	"shrd	eax,edx,16";*/
	//[ISB] heh
	int64_t mul;
	mul = (int64_t)a * (int64_t)b;
	return (fix)(mul >> 16);
}

//divide a quad by a fix, returning a fix
int32_t fixdivquadlong(int64_t n, uint32_t d)
{
	if (d == 0) return 1;
	return (int32_t)(n / d);
}

//divide a quad by a fix, returning a fix
uint32_t ufixdivquadlong(uint32_t nl, uint32_t nh, uint32_t d)
{
	uint64_t num = ((( uint64_t)nh << 32) + (uint64_t)nl);
	return (uint32_t)(num / d);
}

fix fixdiv(fix a, fix b)
{
	//return fixdivquadlong(a << 16, a >> 16, b);
	//	return (fix)FixDiv((Fixed)a,(Fixed)b);
	//[ISB] horrible hack that probably won't work
	if (b == 0) return 1;
	uint32_t ia = (uint32_t)a >> 16; if (a < 0) ia |= 0xFFFF0000;
	uint32_t ib = a << 16;
	int64_t ic = ((int64_t)ia << 32) + (int64_t)ib;

	return (fix)(ic / b);
}

//multiply two fixes, then divide by a third, return a fix
fix fixmuldiv(fix a, fix b, fix c)
{
	if (c == 0) return 1;

	int64_t mul;
	mul = (int64_t)a * (int64_t)b;
	return (fix)(mul / c);
}

fixang fix_atan2(fix cos, fix sin)
{
	int64_t q;
	fix m, t;

	//Assert(!(cos==0 && sin==0));

	//find smaller of two

	q = 0;

	fixmulaccum(&q, sin, sin);
	fixmulaccum(&q, cos, cos);

	m = quad_sqrt(q);

	if (m == 0)
		return 0;

	if (labs(sin) < labs(cos)) //sin is smaller, use arcsin
	{
		t = fix_asin(fixdiv(sin, m));
		if (cos < 0)
			t = 0x8000 - t;
		return (fixang)t; //[ISB] cast, fixes C4244
	}
	else 
	{
		t = fix_acos(fixdiv(cos, m));
		if (sin < 0)
			t = -t;
		return (fixang)t; //[ISB] cast
	}

}

//computes the square root of a quad, returning a long 
uint32_t quad_sqrt(int64_t q)
{
	uint32_t cnt, r, old_r, t;
	int32_t high = (q >> 32) & 0xFFFFFFFF;
	quad tq;
	//[ISB] fixes c4700 error
	memset((void*)&tq, 0, sizeof(quad));

	if (q < 0)
		return 0;

	//if (high == 0 && low >= 0)
	if (q < 0x7FFFFFFF)
		return long_sqrt((int32_t)q);

	if (high & 0xff000000)
		cnt = 12 + 16;
	else if (high & 0xff0000)
		cnt = 8 + 16;
	else if (high & 0xff00)
		cnt = 4 + 16;
	else
		cnt = 0 + 16;

	r = guess_table[(high >> cnt) & 0xff] << cnt;

	//quad loop usually executed 4 times

	r = (fixdivquadlong(q, r) + r) / 2;
	r = (fixdivquadlong(q, r) + r) / 2;
	r = (fixdivquadlong(q, r) + r) / 2;

	do 
	{
		old_r = r;
		t = fixdivquadlong(q, r);

		if (t == r)	//got it!
			return r;

		r = t / 2 + r / 2; //t / 2 + r / 2 + (t & r & 1) Quoth Parabolicus: this would make it exact to (r + t) / 2;

		/*iterLimit++;
		if (iterLimit > 500)
		{
			Warning("quad_sqrt: SERIOUS INFINITE LOOP PROBLEM DETECTED\nYELL AT ISB IF YOU SEE THIS\n");
			break;
		}*/

	} while (!(r == t || r == old_r));

	t = fixdivquadlong(q, r);
	//tq.low = tq.high;
	int64_t tq2;
	fixmulaccum(&tq2, r, t);
	int32_t high2 = (tq2 >> 32) & 0xFFFFFFFF;
	uint32_t low2 = (uint32_t)(tq2 & 0xFFFFFFFFF);
	uint32_t low = (uint32_t)(q & 0xFFFFFFFFF);
	if (low2 != low || high2 != high)
		r++;

	return r;
}


//computes the square root of a long, returning a short
uint16_t long_sqrt(int32_t a)
{
	int cnt, r, old_r, t;

	if (a <= 0)
		return 0;

	if (a & 0xff000000)
		cnt = 12;
	else if (a & 0xff0000)
		cnt = 8;
	else if (a & 0xff00)
		cnt = 4;
	else
		cnt = 0;

	r = guess_table[(a >> cnt) & 0xff] << cnt;

	//the loop nearly always executes 3 times, so we'll unroll it 2 times and
	//not do any checking until after the third time.  By my calcutations, the
	//loop is executed 2 times in 99.97% of cases, 3 times in 93.65% of cases, 
	//four times in 16.18% of cases, and five times in 0.44% of cases.  It never
	//executes more than five times.  By timing, I determined that is is faster
	//to always execute three times and not check for termination the first two
	//times through.  This means that in 93.65% of cases, we save 6 cmp/jcc pairs,
	//and in 6.35% of cases we do an extra divide.  In real life, these numbers
	//might not be the same.

	r = ((a / r) + r) / 2;
	r = ((a / r) + r) / 2;

	do {

		old_r = r;
		t = a / r;

		if (t == r)	//got it!
			return r;

		r = (t + r) / 2;

	} while (!(r == t || r == old_r));

	if (a % r)
		r++;

	return r;
}

//computes the square root of a fix, returning a fix
fix fix_sqrt(fix a)
{
	return ((fix)long_sqrt(a)) << 8;
}


//compute sine and cosine of an angle, filling in the variables
//either of the pointers can be NULL
//with interpolation
void fix_sincos(fix a, fix* s, fix* c)
{
	int i, f;
	fix ss, cc;

	i = (a >> 8) & 0xff;
	f = a & 0xff;

	ss = sincos_table[i];
	*s = (ss + (((sincos_table[i + 1] - ss) * f) >> 8)) << 2;

	cc = sincos_table[i + 64];
	*c = (cc + (((sincos_table[i + 64 + 1] - cc) * f) >> 8)) << 2;

}

//compute sine and cosine of an angle, filling in the variables
//either of the pointers can be NULL
//no interpolation
void fix_fastsincos(fix a, fix* s, fix* c)
{
	int i;

	i = (a >> 8) & 0xff;

	if (s)
	{
		*s = sincos_table[i] << 2;
	}
	if (c)
	{
		*c = sincos_table[i + 64] << 2;
	}
}

//compute inverse sine
fixang fix_asin(fix v)
{
	fix vv;
	int i, f, aa;

	vv = labs(v);

	if (vv >= f1_0)		//check for out of range
		return 0x4000;

	i = (vv >> 8) & 0xff;
	f = vv & 0xff;

	aa = asin_table[i];
	aa = aa + (((asin_table[i + 1] - aa) * f) >> 8);

	if (v < 0)
		aa = -aa;

	return aa;
}

//compute inverse cosine
fixang fix_acos(fix v)
{
	fix vv;
	int i, f, aa;

	vv = labs(v);

	if (vv >= f1_0)		//check for out of range
		return 0;

	i = (vv >> 8) & 0xff;
	f = vv & 0xff;

	aa = acos_table[i];
	aa = aa + (((acos_table[i + 1] - aa) * f) >> 8);

	if (v < 0)
		aa = 0x8000 - aa;

	return aa;
}

#define TABLE_SIZE 1024

//for passed value a, returns 1/sqrt(a) 
fix fix_isqrt(fix a)
{
	int i, b = a;
	int cnt = 0;
	int r;

	if (a == 0) return 0;

	while (b >= TABLE_SIZE) 
	{
		b >>= 1;
		cnt++;
	}

	//printf( "Count = %d (%d>>%d)\n", cnt, b, (cnt+1)/2 );
	r = isqrt_guess_table[b] >> ((cnt + 1) / 2);

	//printf( "Initial r = %d\n", r );

	for (i = 0; i < 3; i++) 
	{
		int old_r = r;
		r = fixmul(((3 * 65536) - fixmul(fixmul(r, r), a)), r) / 2;
		//printf( "r %d  = %d\n", i, r );
		if (old_r >= r) return (r + old_r) / 2;
	}

	return r;
}

