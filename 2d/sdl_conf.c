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

static char* WindowWidthStr = "WindowWidth";
static char* WindowHeightStr = "WindowHeight";
static char* FitModeStr = "FitMode";
static char* FullscreenStr = "Fullscreen";

int I_ReadChocolateConfig()
{
	FILE* infile;
	char line[80], * token, * value, * ptr;

	char* next = NULL;

	WindowWidth = 800;
	WindowHeight = 600;

	errno_t err = fopen_s(&infile, "chocolatedescent.cfg", "rt"); //[ISB] can someone remind me again why I undefined _CRT_SECURE_NO_WARNINGS? I don't know myself
	//[ISB] Can't wait for it to cause severe issues down the line when running on linux.......
	if (infile == NULL) 
	{
		return 1;
	}
	while (!feof(infile)) 
	{
		memset(line, 0, 80);
		fgets(line, 80, infile);
		ptr = &(line[0]);
		while (isspace(*ptr))
			ptr++;
		if (*ptr != '\0') 
		{
			token = strtok_s(ptr, "=", &next);
			value = strtok_s(NULL, "=", &next);
			if (!strcmp(token, WindowWidthStr))
				WindowWidth = strtol(value, NULL, 10);
			else if (!strcmp(token, WindowHeightStr))
				WindowHeight = strtol(value, NULL, 10);
			else if (!strcmp(token, FitModeStr))
				BestFit = strtol(value, NULL, 10);
			else if (!strcmp(token, FullscreenStr))
				Fullscreen = strtol(value, NULL, 10);
		}
	}

	//some basic validation
	if (WindowWidth <= 0) WindowWidth = 800;
	if (WindowHeight <= 0) WindowHeight = 600;

	fclose(infile);

	return 0;
}