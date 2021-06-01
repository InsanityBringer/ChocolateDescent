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

static char local_file_path_prefix[CHOCOLATE_MAX_FILE_PATH_SIZE] = {0};

// This is not completely ready for primetime
// for Windows.  Drive letters aren't really
// accounted for.
void mkdir_recursive(const char *dir)
{
	char tmp[CHOCOLATE_MAX_FILE_PATH_SIZE];
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
		char chocolate_descent_directory[CHOCOLATE_MAX_FILE_PATH_SIZE];
		memset(chocolate_descent_directory, 0, CHOCOLATE_MAX_FILE_PATH_SIZE);
		snprintf(chocolate_descent_directory, CHOCOLATE_MAX_FILE_PATH_SIZE, "%s/Library/Application Support/Chocolate Descent/%s", getenv("HOME"), CHOCOLATE_DESCENT_VERSION_STRING);
		strcpy(local_file_path_prefix, chocolate_descent_directory);
	}

	return local_file_path_prefix;
#else
	return ".";
#endif
}

void get_platform_localized_interior_path(char* platform_localized_interior_path, const char* interior_path)
{
	if (PLATFORM_PATH_SEPARATOR == '/')
	{
		strncpy(platform_localized_interior_path, interior_path, CHOCOLATE_MAX_FILE_PATH_SIZE - 1);
		return;
	}

	char temp_buf[CHOCOLATE_MAX_FILE_PATH_SIZE];
	strncpy(temp_buf, interior_path, CHOCOLATE_MAX_FILE_PATH_SIZE - 1);

	char* separator_pos = strchr(temp_buf, '/');
	while (separator_pos)
	{
		*separator_pos = PLATFORM_PATH_SEPARATOR;
		separator_pos = strchr(separator_pos, '/');
	}

	strncpy(platform_localized_interior_path, temp_buf, CHOCOLATE_MAX_FILE_PATH_SIZE - 1);
}

void get_full_file_path(char* filename_full_path, const char* filename, const char* additional_path)
{
	char temp_buf[CHOCOLATE_MAX_FILE_PATH_SIZE], platform_localized_interior_path[CHOCOLATE_MAX_FILE_PATH_SIZE];
	const char* separator_pos;
	memset(temp_buf, 0, CHOCOLATE_MAX_FILE_PATH_SIZE);
	separator_pos = strrchr(filename, PLATFORM_PATH_SEPARATOR);
	if (separator_pos == NULL)
	{
		if (additional_path == NULL || strlen(additional_path) == 0)
		{
			snprintf(filename_full_path, CHOCOLATE_MAX_FILE_PATH_SIZE, "%s%c%s", get_local_file_path_prefix(), PLATFORM_PATH_SEPARATOR, filename);
			return;
		}
		else
		{
			get_platform_localized_interior_path(platform_localized_interior_path, additional_path);
			snprintf(filename_full_path, CHOCOLATE_MAX_FILE_PATH_SIZE, "%s%c%s%c%s", get_local_file_path_prefix(), PLATFORM_PATH_SEPARATOR, platform_localized_interior_path, PLATFORM_PATH_SEPARATOR, filename);
			return;
		}
	}
	else
	{
		strncpy(temp_buf, separator_pos + 1, CHOCOLATE_MAX_FILE_PATH_SIZE - 1);

		if (additional_path == NULL || strlen(additional_path) == 0)
		{
			snprintf(filename_full_path, CHOCOLATE_MAX_FILE_PATH_SIZE, "%s%c%s", get_local_file_path_prefix(), PLATFORM_PATH_SEPARATOR, temp_buf);
			return;
		}
		else
		{
			get_platform_localized_interior_path(platform_localized_interior_path, additional_path);
			snprintf(filename_full_path, CHOCOLATE_MAX_FILE_PATH_SIZE, "%s%c%s%c%s", get_local_file_path_prefix(), PLATFORM_PATH_SEPARATOR, platform_localized_interior_path, PLATFORM_PATH_SEPARATOR, temp_buf);
			return;
		}
	}

	snprintf(filename_full_path, CHOCOLATE_MAX_FILE_PATH_SIZE, ".%c%s", PLATFORM_PATH_SEPARATOR, filename);
	return;
}

void get_temp_file_full_path(char* filename_full_path, const char* filename)
{
#if defined(__APPLE__) && defined(__MACH__)
	char temp_buf[CHOCOLATE_MAX_FILE_PATH_SIZE];
	const char* separator_pos;
	memset(temp_buf, 0, CHOCOLATE_MAX_FILE_PATH_SIZE);
	separator_pos = strrchr(filename, PLATFORM_PATH_SEPARATOR);
	if (separator_pos == NULL)
	{
		snprintf(filename_full_path, CHOCOLATE_MAX_FILE_PATH_SIZE, "%s%s", getenv("TMPDIR"), filename);
		return;
	}
	else
	{
		strncpy(temp_buf, separator_pos + 1, CHOCOLATE_MAX_FILE_PATH_SIZE - 1);
		snprintf(filename_full_path, CHOCOLATE_MAX_FILE_PATH_SIZE, "%s%s", getenv("TMPDIR"), temp_buf);
		return;
	}
#endif

	snprintf(filename_full_path, CHOCOLATE_MAX_FILE_PATH_SIZE, ".%c%s", PLATFORM_PATH_SEPARATOR, filename);
	return;
}