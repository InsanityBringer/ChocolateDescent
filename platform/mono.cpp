//[ISB]: Stub MDA implemetation for the moment tbh

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "platform/mono.h"

int windowStates[32];

int minit()
{
	fprintf(stderr, "minit: STUB\n");
	return 0;
}

void mclose(int num)
{
	windowStates[num] = 0;
}

void mopen(int n, int row, int col, int width, int height, const char* title)
{
	windowStates[n] = 1;
	fprintf(stderr, "Opened mono window %d: %s\n", n, title);
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
	fprintf(stderr, "_mprintf to window %d:\n", n);
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
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