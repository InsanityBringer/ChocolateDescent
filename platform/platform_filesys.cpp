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

#if defined(_WIN32)
static const char PLATFORM_PATH_SEPARATOR = '\\';
#else
static const char PLATFORM_PATH_SEPARATOR = '/';
#endif

// This is not completely ready for primetime
// for Windows.  Drive letters aren't really
// accounted for.
void mkdir_recursive(const char *dir)
{
#if defined(_WIN32)
	char tmp[MAX_PATH];
#else
	char tmp[FILENAME_MAX];
#endif
	char *p = NULL;
	size_t len;

	snprintf(tmp, sizeof(tmp), "%s", dir);
	len = strlen(tmp);

	if(tmp[len - 1] == PLATFORM_PATH_SEPARATOR)
	{
		tmp[len - 1] = 0;
	}

	for(p = tmp + 1; *p; p++)
	{
		if(*p == PLATFORM_PATH_SEPARATOR)
		{
			*p = 0;
			mkdir(tmp, S_IRWXU);
			*p = PLATFORM_PATH_SEPARATOR;
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
		sprintf(chocolate_descent_directory, "%s/Library/Application Support/Chocolate Descent/%s", getenv("HOME"), descent_version_string);
		strcpy(local_file_path_prefix, chocolate_descent_directory);
	}

	return local_file_path_prefix;
#else
	return ".";
#endif
}

void get_full_file_path(char* filename_full_path, const char* filename, const char* additional_path)
{
	char temp_buf[256];
	const char* separator_pos;
	separator_pos = strrchr(filename, PLATFORM_PATH_SEPARATOR);
	if (separator_pos == NULL)
	{
		if (additional_path == NULL || strlen(additional_path) == 0)
		{
			sprintf(filename_full_path, "%s%c%s", get_local_file_path_prefix(), PLATFORM_PATH_SEPARATOR, filename);
			return;
		}
		else
		{
			sprintf(filename_full_path, "%s%c%s%c%s", get_local_file_path_prefix(), PLATFORM_PATH_SEPARATOR, additional_path, PlatformPathSeparator, filename);
			return;
		}
	}
	else
	{
		strncpy(temp_buf, separator_pos + 1, 255);

		if (additional_path == NULL || strlen(additional_path) == 0)
		{
			sprintf(filename_full_path, "%s%c%s", get_local_file_path_prefix(), PLATFORM_PATH_SEPARATOR, temp_buf);
			return;
		}
		else
		{
			sprintf(filename_full_path, "%s%c%s%c%s", get_local_file_path_prefix(), PLATFORM_PATH_SEPARATOR, additional_path, PlatformPathSeparator, temp_buf);
			return;
		}
	}

	sprintf(filename_full_path, ".%c%s", PLATFORM_PATH_SEPARATOR, filename);
	return;
}

void get_temp_file_full_path(char* filename_full_path, const char* filename)
{
#if defined(__APPLE__) && defined(__MACH__)
	char temp_buf[256];
	const char* separator_pos;
	separator_pos = strrchr(filename, PLATFORM_PATH_SEPARATOR);
	if (separator_pos == NULL)
	{
		sprintf(filename_full_path, "%s%s", getenv("TMPDIR"), filename);
		return;
	}
	else
	{
		strncpy(temp_buf, separator_pos + 1, 255);
		sprintf(filename_full_path, "%s%s", getenv("TMPDIR"), temp_buf);
		return;
	}
#endif

	sprintf(filename_full_path, ".%c%s", PLATFORM_PATH_SEPARATOR, filename);
	return;
}