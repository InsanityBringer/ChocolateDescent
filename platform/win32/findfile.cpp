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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "platform/findfile.h"

static HANDLE	_FindFileHandle = INVALID_HANDLE_VALUE;

int	FileFindFirst(const char* search_str, FILEFINDSTRUCT* ffstruct)
{
	WIN32_FIND_DATAA find;

	_FindFileHandle = FindFirstFileA((LPSTR)search_str, &find);
	if (_FindFileHandle == INVALID_HANDLE_VALUE) return 1;
	else
	{
		ffstruct->size = find.nFileSizeLow;
		if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			ffstruct->type = FF_TYPE_DIR;
		else
			ffstruct->type = FF_TYPE_FILE;
		//Provide shortnames in case of long names, to avoid problems. 
		if (strlen(find.cAlternateFileName) != 0)
			strncpy(ffstruct->name, find.cAlternateFileName, FF_PATHSIZE);
		else
			strncpy(ffstruct->name, find.cFileName, FF_PATHSIZE);
		return 0;
	}
}

int	FileFindNext(FILEFINDSTRUCT* ffstruct)
{
	WIN32_FIND_DATAA find;

	if (!FindNextFileA(_FindFileHandle, &find)) return 1;
	else 
	{
		//printf("%s, shortname len %d\n", find.cFileName, strlen(find.cAlternateFileName));
		ffstruct->size = find.nFileSizeLow;
		if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			ffstruct->type = FF_TYPE_DIR;
		else
			ffstruct->type = FF_TYPE_FILE;
		//Provide shortnames in case of long names, to avoid problems. 
		if (strlen(find.cAlternateFileName) != 0)
			strncpy(ffstruct->name, find.cAlternateFileName, FF_PATHSIZE);
		else
			strncpy(ffstruct->name, find.cFileName, FF_PATHSIZE);
		return 0;
	}
}

int	FileFindClose(void)
{
	if (!FindClose(_FindFileHandle)) return 1;
	else return 0;
}
