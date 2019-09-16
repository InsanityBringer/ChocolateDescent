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

#include "player.h"

extern int ReadConfigFile(void);
extern int WriteConfigFile(void);

extern char config_last_player[CALLSIGN_LEN+1];

extern char config_last_mission[];

extern uint8_t Config_digi_volume;
extern uint8_t Config_midi_volume;
#ifdef MACINTOSH
typedef struct ConfigInfoStruct
{
	uint8_t	mDoNotDisplayOptions;
	uint8_t	mUse11kSounds;
	uint8_t	mDisableSound;
	uint8_t	mDisableMIDIMusic;
	uint8_t	mChangeResolution;
	uint8_t	mDoNotPlayMovies;
	uint8_t	mUserChoseQuit;
	uint8_t	mGameMonitor;
	uint8_t	mAcceleration;				// allow RAVE level acceleration
	uint8_t	mInputSprockets;			// allow use of Input Sprocket devices 
} ConfigInfo;

extern ConfigInfo gConfigInfo;
extern uint8_t Config_master_volume;
#endif
extern uint8_t Config_redbook_volume;
extern uint8_t Config_control_type;
extern uint8_t Config_channels_reversed;
extern uint8_t Config_joystick_sensitivity;

//values for Config_control_type
#define CONTROL_NONE 0
#define CONTROL_JOYSTICK 1
#define CONTROL_FLIGHTSTICK_PRO 2
#define CONTROL_THRUSTMASTER_FCS 3
#define CONTROL_GRAVIS_GAMEPAD 4
#define CONTROL_MOUSE 5
#define CONTROL_CYBERMAN 6
#define CONTROL_WINJOYSTICK 7

#define CONTROL_MAX_TYPES 8
