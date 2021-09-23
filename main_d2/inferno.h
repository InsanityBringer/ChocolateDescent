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

#pragma once

#include "settings.h"	//include personal settings

#include "misc/types.h"

//	MACRO for single line #ifdef WINDOWS #else DOS
#ifdef WINDOWS_FAKE_NOTACTUALLYWINDOWS
#define WINDOS(x,y) x
#define WIN(x) x
#else
#define WINDOS(x,y) y
#define WIN(x)
#endif

#ifdef MACINTOSH
#define MAC(x) x
#else
#define MAC(x)
#endif


/**
 **	Constants
 **/

//the maximum length of a filename
#define FILENAME_LEN 13

//for Function_mode variable
#define FMODE_EXIT		0		//leaving the program
#define FMODE_MENU		1		//Using the menu
#define FMODE_GAME		2		//running the game
#define FMODE_EDITOR		3		//running the editor

//This constant doesn't really belong here, but it is because of horrible
//circular dependencies involving object.h, aistruct.h, polyobj.h, & robot.h
#define MAX_SUBMODELS 10			//how many animating sub-objects per model

/**
 **	Global variables
 **/

extern int Function_mode;			//in game or editor?
extern int Screen_mode;				//editor screen or game screen?

//The version number of the game
extern uint8_t Version_major,Version_minor;

//[ISB] Logic version support:
//Logic version affects anything that happens once you're in a game.
//Things like what format of the level will be used are for the data version,
//which is set by the data you loaded. In contrast, for the full game, it
//should be possible to override logic versions to emulate other versions.

//When using the demo data, this will be forced to SHAREWARE.
//Otherwise, it will default to FULL_1_2.

enum class LogicVer
{
	SHAREWARE = 0, //Emulate shareware logic. The full game
	FULL_1_0, //Emulate initial release. Unused in practice. Mostly distinguishes shareware.
	FULL_1_1, //Emulate 1.1 patch. Unused in practice.
	FULL_1_2 //Emulate 1.2 patch. The default
};

//Data version support:
//Data version affects anything related to the game data itself.
//It is set based on what game data is present, and will control how that data
//is loaded, and what features are available.
//There's only two options here, Demo and Full, as all full releases use the same formats.
//This may eventually include Destination Quartzon since while that uses mostly full data,
//the logic is much different. 

enum class DataVer
{
	DEMO = 0,
	FULL
};

extern LogicVer CurrentLogicVersion;
extern DataVer CurrentDataVersion;

#ifdef MACINTOSH
extern ubyte Version_fix;
#endif

int D_DescentMain(int argc, const char** argv);
