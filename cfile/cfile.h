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

#include <stdio.h>
#include "misc/types.h"
#include "fix/fix.h"
#include "vecmat/vecmat.h"

typedef struct CFILE
{
	FILE* file;
	int				size;
	int				lib_offset;
	int				raw_position;
} CFILE;

//Specify the name of the hogfile.  Returns 1 if hogfile found & had files
int cfile_init(const char* hogname);

CFILE* cfopen(const char* filename, const char* mode);
int cfilelength(CFILE* fp);							// Returns actual size of file...
size_t cfread(void* buf, size_t elsize, size_t nelem, CFILE* fp);
void cfclose(CFILE* cfile);
int cfgetc(CFILE* fp);
int cfseek(CFILE* fp, long int offset, int where);
int cftell(CFILE* fp);
char* cfgets(char* buf, size_t n, CFILE* fp);

int cfexist(const char* filename);	// Returns true if file exists on disk (1) or in hog (2).

//[ISB] little endian reading functions
uint8_t cfile_read_byte(CFILE* fp);
short cfile_read_short(CFILE* fp);
int cfile_read_int(CFILE* fp);

#define cfile_read_fix(a) ((fix)cfile_read_int(a))

//[ISB] normal file versions of these because why not
uint8_t file_read_byte(FILE* fp);
short file_read_short(FILE* fp);
int file_read_int(FILE* fp);
void file_write_byte(FILE* fp, uint8_t b);
void file_write_short(FILE* fp, short s);
void file_write_int(FILE* fp, int i);
//Unlike cfgets, this will only end at null terminators, not newlines. 
void cfile_get_string(char* buffer, int count, CFILE* fp);

// Allows files to be gotten from an alternate hog file.
// Passing NULL disables this.
int cfile_use_alternate_hogfile(const char* name);

void cfile_read_vector(vms_vector *vec, CFILE* fp);
void cfile_read_angvec(vms_angvec *vec, CFILE* fp);
