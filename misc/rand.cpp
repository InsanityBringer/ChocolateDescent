/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/
#include "rand.h"

static unsigned int randNext = 1;

//[ISB] Compiler independent rand function that replicates Watcom C's rand
//Numbers taken from C standard doc example implementation. These numbers are used in watcom c and glibc...
//I hope it isn't a problem using them...
int P_Rand()
{
	randNext = randNext * 1103515245 + 12345;
	return (unsigned int)(randNext/65536) & PRAND_MAX;
}

void P_SRand(int seed)
{
	randNext = seed;
}