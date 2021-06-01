/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License.
*/

#pragma once

static char local_file_path_prefix[512] = {0};

#if defined(BUILD_DESCENT1)
static const char* descent_version_string = "Descent 1";
static unsigned int descent_version = 1;
#elif defined(BUILD_DESCENT2)
static const char* descent_version_string = "Descent 2";
static unsigned int descent_version = 2;
#endif

//-----------------------------------------------------------------------------
//	File system utilities
//-----------------------------------------------------------------------------

//Create directories recursively
void mkdir_recursive(const char* dir);

//Get path to user files for local system
const char* get_local_file_path_prefix();

//Get full path to files using the local file path prefix
void get_full_file_path(char* filename_full_path, const char* filename, const char* additional_path);

//Get full path to files in an OS-specific temp directory
void get_temp_file_full_path(char* filename_full_path, const char* filename);