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
//[ISB] This is adapted from the descent.cfg parser, so above notice is needed

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "platform/platform_filesys.h"
#include "platform/platform.h"
#include "platform/s_midi.h"
#include "platform/mouse.h"

static const char* WindowWidthStr = "WindowWidth";
static const char* WindowHeightStr = "WindowHeight";
static const char* FitModeStr = "FitMode";
static const char* FullscreenStr = "Fullscreen";
static const char* SoundFontPath = "SoundFontPath";
static const char* MouseScalarStr = "MouseScalar";
static const char* SwapIntervalStr = "SwapInterval";
static const char* NoOpenGLStr = "NoOpenGL";
static const char* GenDeviceStr = "PreferredGenMidiDevice";
static const char* MMEDeviceStr = "MMEDevice";

bool NoOpenGL = false;

//[ISB] to be honest, I hate this configuration parser. I should try to create something more flexible at some point.
int plat_read_chocolate_cfg()
{
	FILE* infile;
	char line[512], * token, * value, * ptr;
	char cfgpath[CHOCOLATE_MAX_FILE_PATH_SIZE];

	char* next = NULL;

	WindowWidth = 800;
	WindowHeight = 600;

#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	get_full_file_path(cfgpath, "chocolatedescent.cfg", CHOCOLATE_CONFIG_DIR);
#else
	sprintf(cfgpath, "chocolatedescent.cfg");
#endif

	infile = fopen(cfgpath, "rt");
	if (infile == NULL) 
	{
		//Try creating a default config file
		plat_save_chocolate_cfg();
		return 1;
	}
	while (!feof(infile)) 
	{
		memset(line, 0, 512);
		fgets(line, 512, infile);
		ptr = &(line[0]);
		while (isspace(*ptr))
			ptr++;
		if (*ptr != '\0') 
		{
			token = strtok(ptr, "=");
			value = strtok(NULL, "=");
			if (!strcmp(token, WindowWidthStr))
				WindowWidth = strtol(value, NULL, 10);
			else if (!strcmp(token, WindowHeightStr))
				WindowHeight = strtol(value, NULL, 10);
			else if (!strcmp(token, FitModeStr))
				BestFit = strtol(value, NULL, 10);
			else if (!strcmp(token, FullscreenStr))
				Fullscreen = strtol(value, NULL, 10);
			else if (!strcmp(token, MouseScalarStr))
				MouseScalar = strtof(value, NULL);
			else if (!strcmp(token, SwapIntervalStr))
				SwapInterval = strtol(value, NULL, 10);
			else if (!strcmp(token, SoundFontPath))
			{
				char* p;
#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
				get_full_file_path(&SoundFontFilename[0], value, CHOCOLATE_SOUNDFONTS_DIR);
				SoundFontFilename[CHOCOLATE_MAX_FILE_PATH_SIZE - 1] = '\0';
#else
				strncpy(&SoundFontFilename[0], value, 255);
				SoundFontFilename[_MAX_PATH - 1] = '\0';
#endif
				//[ISB] godawful hack from Descent's config parser, should fix parsing the soundfont path
				p = strchr(SoundFontFilename, '\n');
				if (p)*p = 0;
			}
			else if (!strcmp(token, NoOpenGLStr))
				NoOpenGL = strtol(value, NULL, 10) != 0;
			else if (!strcmp(token, GenDeviceStr))
				PreferredGenDevice = (GenDevices)strtol(value, NULL, 10);
		}
	}

	//some basic validation
	if (WindowWidth <= 320) WindowWidth = 320;
	if (WindowHeight <= 240) WindowHeight = 240;
	if (PreferredMMEDevice < -1) PreferredMMEDevice = -1;

	fclose(infile);

	return 0;
}

void plat_save_chocolate_cfg()
{
	char cfgpath[CHOCOLATE_MAX_FILE_PATH_SIZE];

#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	get_full_file_path(cfgpath, "chocolatedescent.cfg", CHOCOLATE_CONFIG_DIR);
#else
	sprintf(cfgpath, "chocolatedescent.cfg");
#endif

	FILE* infile = fopen(cfgpath, "w");
	if (infile != NULL)
	{
		fprintf(infile, "%s=%d\n", WindowWidthStr, WindowWidth);
		fprintf(infile, "%s=%d\n", WindowHeightStr, WindowHeight);
		fprintf(infile, "%s=%d\n", FitModeStr, BestFit);
		fprintf(infile, "%s=%d\n", FullscreenStr, Fullscreen);
		fprintf(infile, "%s=%f\n", MouseScalarStr, MouseScalar);
		fprintf(infile, "%s=%d\n", SwapIntervalStr, SwapInterval);
		if (SoundFontFilename[0])
			fprintf(infile, "%s=%s\n", SoundFontPath, SoundFontFilename);
		fprintf(infile, "%s=%d\n", NoOpenGLStr, NoOpenGL);
		fprintf(infile, "%s=%d\n", GenDeviceStr, (int)PreferredGenDevice);
		fclose(infile);
	}
}
