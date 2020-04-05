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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>
#include <dirent.h>
#include "platform/mono.h"
#include "platform/findfile.h"
#include "platform/posixstub.h"

char searchStr[13];
DIR *currentDir;

int	FileFindFirst(const char* search_str, FILEFINDSTRUCT* ffstruct)
{
	char dir[256];
	const char *search;

	//Make sure the current search buffer is clear
	memset(searchStr, 0, 13);
	memset(dir, 0, 256);
	//Open the directory for searching
	_splitpath(search_str, NULL, dir, NULL, NULL);
	if (strlen(dir) == 0) //godawful hack
		dir[0] = '.';
	//mprintf((0, "FindFileFirst: Opening %s\n", dir));
	currentDir = opendir(dir);
	if (!currentDir) return 1;
	//It opened, so get search string
	search = strrchr(search_str, '*');
	strncpy(searchStr, search+2, 12);
	//mprintf((0, "FindFileFirst: Search is %s\n", searchStr));

	return FileFindNext(ffstruct);
}

int	FileFindNext(FILEFINDSTRUCT* ffstruct)
{
	char name[13];
	char fname[256];
	char ext[256];
	struct dirent *entry;
	struct stat stats;
	if (!currentDir) return 1;
	entry = readdir(currentDir);
	while (entry != NULL)
	{
		//What a mess. ugh
		memset(fname, 0, 256);
		memset(ext, 0, 256);
		_splitpath(entry->d_name, NULL, NULL, fname, ext);
		if (strlen(fname) + strlen(ext) + 1 < 13) //Only care about entries that are short enough. This is to prevent problems with Descent's limitation of 8.3 character filenames.
		{
			if (!strncmp(ext, searchStr, 3))
			{
				//mprintf((0, "got %s (%s, %s)\n", entry->d_name, fname, ext));
				stat(entry->d_name, &stats);
				ffstruct->size = static_cast<uint32_t>(stats.st_size);
				strncpy(ffstruct->name, entry->d_name, FF_PATHSIZE);
				return 0;
			}
			
		}
		entry = readdir(currentDir);
	}

	return 1;
}

int	FileFindClose(void)
{
	if (!currentDir) return 0;
	closedir(currentDir);
	return 0;
}
