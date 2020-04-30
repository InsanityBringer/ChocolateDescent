/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/
//[ISB]: Stub MDA implemetation for the moment tbh
#ifndef NDEBUG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <algorithm>

#include "platform/mono.h"

#define NUMWINDOWS 2

int windowStates[NUMWINDOWS];
char windowNames[NUMWINDOWS][256];

int lastWindow = -1;

int minit()
{
	//fprintf(stderr, "minit: STUB\n");
	return 0;
}

void mclose(int num)
{
	windowStates[num] = 0;
}

void mopen(int n, int row, int col, int width, int height, const char* title)
{
	if (n < NUMWINDOWS)
	{
		windowStates[n] = 1;
		fprintf(stderr, "Opened mono window %d: %s\n", n, title);
		//is this sufficient error checking?
		strncpy(windowNames[n], title, std::min(strlen(title), (size_t)255));
	}
}

void mclear(int n)
{
	fprintf(stderr, "mclear: STUB\n");
}

void _mprintf(int n, const char* format, ...)
{
	//[ISB] console IO on Windows is the slowest thing on planet earth, so actually check if open
	if (!windowStates[n]) return;
	va_list args;
	if (lastWindow != n)
	{
		fprintf(stderr, "%s:\n_______________________________\n", windowNames[n]);
	}
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	lastWindow = n;
}

void _mprintf_at(int n, int row, int col, const char* format, ...)
{
	fprintf(stderr, "_mprintf_at: STUB\n");
}

void mputc(int n, char c)
{
	fprintf(stderr, "mputc: STUB\n");
}

void mputc_at(int n, int row, int col, char c)
{
	fprintf(stderr, "mputc_at: STUB\n");
}

void msetcursor(int row, int col)
{
	//this never even worked so why stub it
}

void mrefresh(short n)
{
	fprintf(stderr, "mrefresh: STUB\n");
}

#endif