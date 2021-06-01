/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License.
*/

#pragma once

#include <stddef.h>
#if defined(_WIN32) || defined(_WIN64)
#include <stdlib.h>
#else
#include <limits.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
static const char PLATFORM_PATH_SEPARATOR = '\\';
#else
static const char PLATFORM_PATH_SEPARATOR = '/';
#endif

#if defined(_WIN32) || defined(_WIN64)
#define CHOCOLATE_MAX_FILE_PATH_SIZE _MAX_PATH * 4 //Apparently NT can have longer paths than indicated?
#else
#define CHOCOLATE_MAX_FILE_PATH_SIZE PATH_MAX
#endif

#if defined(BUILD_DESCENT1)
static const char* CHOCOLATE_DESCENT_VERSION_STRING = "Descent 1";
static const unsigned int CHOCOLATE_DESCENT_VERSION = 1;
#elif defined(BUILD_DESCENT2)
static const char* CHOCOLATE_DESCENT_VERSION_STRING = "Descent 2";
static const unsigned int CHOCOLATE_DESCENT_VERSION = 2;
#endif

//-----------------------------------------------------------------------------
//	File system utilities
//-----------------------------------------------------------------------------

//Create directories recursively
void mkdir_recursive(const char* dir);

//Get path to user files for local system
const char* get_local_file_path_prefix();

//Get full path to files using the local file path prefix
void get_full_file_path(char* filename_full_path, const char* filename, const char* additional_path = NULL);

//Get full path to files in an OS-specific temp directory
void get_temp_file_full_path(char* filename_full_path, const char* filename);