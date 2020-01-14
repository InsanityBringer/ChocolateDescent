/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/

#pragma once
//[ISB] This is needed to ensure rand works consistiently across compilers

//Random max. Must be 0x7fff to work with Descent source.
#define PRAND_MAX 0x7fff

int P_Rand();
void P_SRand(int seed);
