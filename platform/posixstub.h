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
#define O_BINARY 0
#define _fileno(a) fileno(a)
#define _unlink(a) unlink(a)
#define _mkdir(a) mkdir(a, S_IRWXU | S_IRGRP | S_IROTH)
#define _open(a, b) open(a, b)
#define _close(a) close(a)
#define _read(a, b, c) read(a, b, c)
#define _lseek(a, b, c) lseek(a, b, c)

#define _MAX_PATH 256
#define _MAX_DIR 256

//Renamed string operations
#define _strdup(a) strdup(a)

//[ISB] These replace stricmp and strnicmp for filenames
//Needs to be case sensitive for filenames
#define _strfcmp(a, b) strcmp(a, b)
#define _strnfcmp(a, b, c) strncmp(a, b, c)

//Non-standard file operations
int _filelength(int descriptor);

//Non-standard string functions
int _stricmp(const char* a, const char* b);
int _strnicmp(const char* a, const char* b, int n);
char* _itoa(int num, char* buf, int max);
char* _strrev(char* in);
char* _strlwr(char* in);
char* _strupr(char* in);
void _splitpath(const char *name, char *drive, char *path, char *base, char *ext);

#endif
