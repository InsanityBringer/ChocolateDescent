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
#include "platform/platform_filesys.h"

char searchStr[13];
DIR *currentDir;

int	FileFindFirst(const char* search_str, FILEFINDSTRUCT* ffstruct)
{
#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	char dir[CHOCOLATE_MAX_FILE_PATH_SIZE];
	char temp_search_path[CHOCOLATE_MAX_FILE_PATH_SIZE];
	char full_search_str[CHOCOLATE_MAX_FILE_PATH_SIZE];
	char *separator_pos;
#else
	char dir[256];
#endif
	const char *search;

	//Make sure the current search buffer is clear
	memset(searchStr, 0, 13);
	memset(dir, 0, 256);

#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	get_full_file_path(dir, "");
	get_platform_localized_interior_path(temp_search_path, search_str);
	memset(full_search_str, 0, CHOCOLATE_MAX_FILE_PATH_SIZE);
	snprintf(full_search_str, CHOCOLATE_MAX_FILE_PATH_SIZE, "%s%s", dir, temp_search_path);
#else
	sprintf(full_search_str, "%s", search_str);
#endif

	//Open the directory for searching
	_splitpath(full_search_str, NULL, dir, NULL, NULL);
	if (strlen(dir) == 0) //godawful hack
	{
#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
		strncpy(dir, full_search_str, CHOCOLATE_MAX_FILE_PATH_SIZE);
		separator_pos = strrchr(dir, PLATFORM_PATH_SEPARATOR);
		if (separator_pos != NULL && separator_pos - dir > 0)
		{
			dir[separator_pos - dir + 1] = '\0';
		}
#else
		dir[0] = '.';
#endif
	}
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
#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	char fname[CHOCOLATE_MAX_FILE_PATH_SIZE];
	char ext[CHOCOLATE_MAX_FILE_PATH_SIZE];
#else
	char fname[256];
	char ext[256];
#endif
	struct dirent *entry;
	struct stat stats;
	if (!currentDir) return 1;
	entry = readdir(currentDir);
	while (entry != NULL)
	{
		//What a mess. ugh
#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
		memset(fname, 0, CHOCOLATE_MAX_FILE_PATH_SIZE);
		memset(ext, 0, CHOCOLATE_MAX_FILE_PATH_SIZE);
#else
		memset(fname, 0, 256);
		memset(ext, 0, 256);
#endif
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
