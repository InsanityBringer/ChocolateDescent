/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt
*/

/*
*/
#pragma once

#ifdef _WIN32
#include <string.h>
#include <direct.h>
#include <io.h>
#include <fcntl.h>

//[ISB] These replace stricmp and strnicmp for filenames
//Not case sensitive for windows
#define _strfcmp(a, b) _stricmp(a, b)
#define _strnfcmp(a, b, c) _strnicmp(a, b, c)

#else
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

//Renamed file operations
#define _fileno(a) fileno(a)
#define _mkdir(a) _mkdir(a, S_IRWXU | S_IRGRP | S_IROTH)

//Renamed string operations
#define _strdup(a) strdup(a)

//[ISB] These replace stricmp and strnicmp for filenames
//Needs to be case sensitive for filenames
#define _strfcmp(a, b) strcmp(a, b)
#define _strnfcmp(a, b, c) strncmp(a, b, c)

//Non-standard file operations
int _filelength(int descriptor);

//Non-standard string functions
int _stricmp(char* a, char* b);
int _strnicmp(char* a, char* b, int n);
char* _strrev(char* in);
char* _strlwr(char* in);
char* _strupr(char* in);

#endif