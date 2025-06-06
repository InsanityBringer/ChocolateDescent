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

#pragma once

#include "misc/types.h"

constexpr int FRACBITS = 16;

typedef int32_t fix;				//16 bits int, 16 bits frac
typedef short fixang;		//angles

//Some handy constants
#define f0_0	0
#define f1_0	0x10000
#define f2_0	0x20000
#define f3_0	0x30000
#define f10_0	0xa0000

#define f0_5 0x8000
#define f0_1 0x199a

#define F0_0	f0_0
#define F1_0	f1_0
#define F2_0	f2_0
#define F3_0	f3_0
#define F10_0	f10_0

#define F0_5 	f0_5
#define F0_1 	f0_1

typedef struct quad 
{
	uint32_t low;
	int32_t high;
} quad;

//Convert an int to a fix
constexpr fix i2f(int i)
{
	return i << FRACBITS;
}

//Get the int part of a fix
constexpr int f2i(fix f)
{
	return f >> FRACBITS;
}

constexpr int f2iu(unsigned int f)
{
	return f >> FRACBITS;
}

//Get the int part of a fix, with rounding
constexpr int f2ir(int f)
{
	return (f + F0_5) >> FRACBITS;
}

//Convert fix to float and float to fix
constexpr float f2fl(fix f)
{
	return f / 65536.0f;
}

constexpr fix fl2f(float f)
{
	return (fix)(f * 65536);
}

//multiply two fixes, return a fix
constexpr fix fixmul(fix a, fix b)
{
	return ((int64_t)a * (int64_t)b) >> FRACBITS;
}

//divide two fixes, return a fix
constexpr fix fixdiv(fix a, fix b)
{
	//[ISB] TODO: Why did I put this here? if it's div0ing something's wrong. The original game doesn't allow it. 
	if (b == 0) return 1;

	return (fix)(((int64_t)a << 16) / b);
}

//multiply two fixes, then divide by a third, return a fix
constexpr fix fixmuldiv(fix a, fix b, fix c)
{
	if (c == 0) return 1;

	return (fix)((int64_t)a * (int64_t)b / c);
}

//multiply two fixes, and add 64-bit product to a quad
void fixmulaccum(int64_t*q, fix a, fix b);

//extract a fix from a quad product
fix fixquadadjust(int64_t q);

//divide a quad by a long
constexpr int32_t fixdivquadlong(int64_t n, uint32_t d)
{
	if (d == 0) return 1;
	return (int32_t)(n / d);
}

//negate a quad
void fixquadnegate(quad* q);

//computes the square root of a long, returning a short
uint16_t long_sqrt(int32_t a);

//computes the square root of a quad, returning a long
uint32_t quad_sqrt(int64_t q);

//computes the square root of a fix, returning a fix
fix fix_sqrt(fix a);

//compute sine and cosine of an angle, filling in the variables
//either of the pointers can be NULL
void fix_sincos(fix a, fix* s, fix* c);		//with interpolation
void fix_fastsincos(fix a, fix* s, fix* c);	//no interpolation

//compute inverse sine & cosine
fixang fix_asin(fix v);
fixang fix_acos(fix v);

//given cos & sin of an angle, return that angle.
//parms need not be normalized, that is, the ratio of the parms cos/sin must
//equal the ratio of the actual cos & sin for the result angle, but the parms 
//need not be the actual cos & sin.  
//NOTE: this is different from the standard C atan2, since it is left-handed.
fixang fix_atan2(fix cos, fix sin);

//for passed value a, returns 1/sqrt(a) 
fix fix_isqrt(fix a);
