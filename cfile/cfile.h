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
uint8_t CF_ReadByte(CFILE* fp);
short CF_ReadShort(CFILE* fp);
int CF_ReadInt(CFILE* fp);
//[ISB] hey I made a mistake
#define cfile_read_int(a) CF_ReadInt(a)
#define cfile_read_short(a) CF_ReadShort(a)
#define cfile_read_byte(a) CF_ReadByte(a)
#define cfile_read_fix(a) ((fix)CF_ReadInt(a))

//[ISB] normal file versions of these because why not
uint8_t F_ReadByte(FILE* fp);
short F_ReadShort(FILE* fp);
int F_ReadInt(FILE* fp);
void F_WriteByte(FILE* fp, uint8_t b);
void F_WriteShort(FILE* fp, short s);
void F_WriteInt(FILE* fp, int i);
void CF_GetString(char* buffer, int count, CFILE* fp);

// Allows files to be gotten from an alternate hog file.
// Passing NULL disables this.
int cfile_use_alternate_hogfile(const char* name);

void cfile_read_vector(vms_vector *vec, CFILE* fp);
void cfile_read_angvec(vms_angvec *vec, CFILE* fp);
