/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License.
*/

#include "platform_filesys.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#if defined(WIN32)
static const char PlatformPathSeparator = '\\';
#else
static const char PlatformPathSeparator = '/';
#endif

// This is not completely ready for primetime
// for Windows.  Drive letters aren't really
// accounted for.
void mkdir_recursive(const char *dir)
{
#if defined(WIN32)
	char tmp[MAX_PATH];
#else
	char tmp[FILENAME_MAX];
#endif
	char *p = NULL;
	size_t len;

	snprintf(tmp, sizeof(tmp), "%s", dir);
	len = strlen(tmp);

	if(tmp[len - 1] == PlatformPathSeparator)
	{
		tmp[len - 1] = 0;
	}

	for(p = tmp + 1; *p; p++)
	{
		if(*p == PlatformPathSeparator)
		{
			*p = 0;
			mkdir(tmp, S_IRWXU);
			*p = PlatformPathSeparator;
		}
	}

	mkdir(tmp, S_IRWXU);
}

const char* get_local_file_path_prefix()
{
#if defined(__APPLE__) && defined(__MACH__)
	if(local_file_path_prefix[0] == 0)
	{
		char chocolate_descent_directory[256];
		sprintf(chocolate_descent_directory, "%s/Library/ChocolateDescent/%s", getenv("HOME"), descent_version_string);
		strcpy(local_file_path_prefix, chocolate_descent_directory);
	}

	return local_file_path_prefix;
#else
	return "";
#endif
}