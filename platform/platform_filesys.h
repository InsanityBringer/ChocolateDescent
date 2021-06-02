/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License.
*/

#pragma once

#if defined(__APPLE__) && defined(__MACH__)
#define CHOCOLATE_USE_LOCALIZED_PATHS
#endif

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

#if defined(__APPLE__) && defined(__MACH__)
#define CHOCOLATE_CONFIG_DIR ""
#define CHOCOLATE_SYSTEM_FILE_DIR "Data"
#define CHOCOLATE_PILOT_DIR ""
#define CHOCOLATE_SAVE_DIR ""
#define CHOCOLATE_HISCORE_DIR ""
#define CHOCOLATE_MISSIONS_DIR "Data/Missions"
#define CHOCOLATE_DEMOS_DIR "Data/Demos"
#define CHOCOLATE_SOUNDFONTS_DIR "Data/SoundFonts"
#else
#define CHOCOLATE_CONFIG_DIR ""
#define CHOCOLATE_SYSTEM_FILE_DIR ""
#define CHOCOLATE_PILOT_DIR ""
#define CHOCOLATE_SAVE_DIR ""
#define CHOCOLATE_HISCORE_DIR ""
#define CHOCOLATE_MISSIONS_DIR ""
#define CHOCOLATE_DEMOS_DIR ""
#define CHOCOLATE_SOUNDFONTS_DIR ""
#endif

//-----------------------------------------------------------------------------
//	File system utilities
//-----------------------------------------------------------------------------

//Create directories recursively
void mkdir_recursive(const char* dir);

//Ensure that all of the platform-localized directories exist
void init_all_platform_localized_paths();

//Validate that all required game files are in expected locations and report an error if one isn't
void validate_required_files();

//Get path to user files for local system
const char* get_platform_localized_file_path_prefix();

//Assuming a / is used as a path separator in code, localize it for the platform this is running on
void get_platform_localized_interior_path(char* platform_localized_interior_path, const char* interior_path);

//Localize subpaths within the base directory
void get_platform_localized_path(char* platform_localized_path, const char* subpath);

//Localize file query strings
void get_platform_localized_query_string(char* platform_localized_query_string, const char* subpath, const char* query);

//Get full path to files using the local file path prefix
void get_full_file_path(char* filename_full_path, const char* filename, const char* additional_path = NULL);

//Get full path to files in an OS-specific temp directory
void get_temp_file_full_path(char* filename_full_path, const char* filename);