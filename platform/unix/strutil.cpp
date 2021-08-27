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
/*
 * $Source: Smoke:miner:source:misc::RCS:strutil.c $
 * $Revision: 1.3 $
 * $Author: allender $
 * $Date: 1995/07/17 10:44:11 $
 *
 * string manipulation utility code
 * 
 * $Log: strutil.c $
 * Revision 1.3  1995/07/17  10:44:11  allender
 * fixed strrev
 *
 * Revision 1.2  1995/05/04  20:10:59  allender
 * added some string routines
 *
 * Revision 1.1  1995/03/30  15:03:55  allender
 * Initial revision
 *
*/
 
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "mem/mem.h"

// string compare without regard to case

int _stricmp(const char *s1, const char *s2)
{
	while( *s1 && *s2 )	
	{
		if (tolower(*s1) != tolower(*s2))	return 1;
		s1++;
		s2++;
	}
	if (*s1 || *s2) return 1;
	return 0;
}

int _strnicmp(const char *s1, const char *s2, int n)
{
	while(*s1 && *s2 && n)	
	{
		if (tolower(*s1) != tolower(*s2))	return 1;
		s1++;
		s2++;
		n--;
	}
	return 0;
}

void _strlwr(char *s1)
{
	while(*s1)
	{
		*s1 = tolower(*s1);
		s1++;
	}
}

void _strupr(char *s1)
{
	while(*s1)
	{
		*s1 = toupper(*s1);
		s1++;
	}
}

void _strrev(char *s1)
{
	int i,l;
	char *s2;
	
	s2 = (char *)malloc(strlen(s1) + 1);
	strcpy(s2, s1);
	l = strlen(s2);
	for (i = 0; i < l; i++)
		s1[l-1-i] = s2[i];
	free(s2);
}

char* _itoa(int num, char* buf, int max)
{
	snprintf(buf, max, "%d", num);
	return buf;
}

void _splitpath(const char *name, char *drive, char *path, char *base, char *ext)
{
	char *s, *p;

	//[ISB] oops, splitpath is destructive, lovely...
	//Since the incoming char* is likely const, copy it into a new buffer. 
	char* buf = (char*)malloc(sizeof(char) * (strlen(name) + 1));
	strcpy(buf, name);

	p = &buf[0];
	s = strchr(p, ':');
	if ( s != NULL ) 
	{
		if (drive) 
		{
			*s = '\0';
			strcpy(drive, p);
			*s = ':';
		}
		p = s+1;
		if (!p)
		{
			free(buf);
			return;
		}
	} else if (drive)
		*drive = '\0';
	
	s = strrchr(p, '\\');
	if ( s != NULL) 
	{
		if (path) 
		{
			char c;
			
			c = *(s+1);
			*(s+1) = '\0';
			strcpy(path, p);
			*(s+1) = c;
		}
		p = s+1;
		if (!p)
		{
			free(buf);
			return;
		}
	} 
	else if (path)
		*path = '\0';

	s = strchr(p, '.');
	if ( s != NULL) 
	{
		if (base) 
		{
			*s = '\0';
			strcpy(base, p);
			*s = '.';
		}
		p = s+1;
		if (!p)
		{
			free(buf);
			return;
		}
	} 
	else if (base)
		*base = '\0';
		
	if (ext)
		strcpy(ext, p);	

	free(buf);	
}
