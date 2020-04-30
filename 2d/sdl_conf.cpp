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

#include "2d/i_gr.h"
#include "platform/s_midi.h"
#include "platform/mouse.h"

static const char* WindowWidthStr = "WindowWidth";
static const char* WindowHeightStr = "WindowHeight";
static const char* FitModeStr = "FitMode";
static const char* FullscreenStr = "Fullscreen";
static const char* SoundFontPath = "SoundFontPath";
static const char* MouseScalarStr = "MouseScalar";

//[ISB] to be honest, I hate this configuration parser. I should try to create something more flexible at some point.
int I_ReadChocolateConfig()
{
	FILE* infile;
	char line[512], * token, * value, * ptr;

	char* next = NULL;

	WindowWidth = 800;
	WindowHeight = 600;

	//errno_t err = fopen_s(&infile, "chocolatedescent.cfg", "rt"); //[ISB] can someone remind me again why I undefined _CRT_SECURE_NO_WARNINGS? I don't know myself
	infile = fopen("chocolatedescent.cfg", "rt");
	if (infile == NULL) 
	{
		//Try creating a default config file
		//err = fopen_s(&infile, "chocolatedescent.cfg", "w");
		infile = fopen("chocolatedescent.cfg", "w");
		if (infile != NULL)
		{
			fprintf(infile, "%s=%d\n", WindowWidthStr, WindowWidth);
			fprintf(infile, "%s=%d\n", WindowHeightStr, WindowHeight);
			fprintf(infile, "%s=%d\n", FitModeStr, BestFit);
			fprintf(infile, "%s=%d\n", FullscreenStr, Fullscreen);
			if (SoundFontFilename[0])
				fprintf(infile, "%s=%s", SoundFontPath, SoundFontFilename);
			fclose(infile);
		}
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
			else if (!strcmp(token, SoundFontPath))
			{
				char* p;
				memset(&SoundFontFilename[0], 0, 256);
				strncpy(&SoundFontFilename[0], value, 255);
				//[ISB] godawful hack from Descent's config parser, should fix parsing the soundfont path
				p = strchr(SoundFontFilename, '\n');
				if (p)* p = 0;
			}
		}
	}

	//some basic validation
	if (WindowWidth <= 320) WindowWidth = 320;
	if (WindowHeight <= 240) WindowHeight = 240;

	fclose(infile);

	return 0;
}
