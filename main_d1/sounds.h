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
/*
 * $Source: f:/miner/source/main/rcs/sounds.h $
 * $Revision: 2.0 $
 * $Author: john $
 * $Date: 1995/02/27 11:27:32 $
 *
 * Numbering system for the sounds.
 *
 */

#ifndef _SOUNDS_H
#define _SOUNDS_H

#include "vecmat/vecmat.h"
#include "digi.h"

 //------------------- List of sound effects --------------------

#define SOUND_LASER_FIRED								10

#define SOUND_WEAPON_HIT_DOOR							27
#define SOUND_WEAPON_HIT_BLASTABLE					11
#define SOUND_BADASS_EXPLOSION						11  	// need something different for this if possible

#define SOUND_ROBOT_HIT_PLAYER						17

#define SOUND_ROBOT_HIT									20
#define SOUND_ROBOT_DESTROYED							21
#define SOUND_VOLATILE_WALL_HIT						21

#define SOUND_LASER_HIT_CLUTTER						30
#define SOUND_CONTROL_CENTER_HIT						30
#define SOUND_EXPLODING_WALL							31		//one long sound
#define SOUND_CONTROL_CENTER_DESTROYED				31

#define SOUND_CONTROL_CENTER_WARNING_SIREN		32
#define SOUND_MINE_BLEW_UP								33

#define SOUND_FUSION_WARMUP							34

#define SOUND_REFUEL_STATION_GIVING_FUEL			62

#define SOUND_PLAYER_HIT_WALL							70
#define SOUND_PLAYER_GOT_HIT							71

#define SOUND_HOSTAGE_RESCUED							91

#define SOUND_COUNTDOWN_0_SECS						100	//countdown 100..114
#define SOUND_COUNTDOWN_13_SECS						113
#define SOUND_COUNTDOWN_29_SECS						114

#define SOUND_HUD_MESSAGE								117
#define SOUND_HUD_KILL									118

#define SOUND_HOMING_WARNING							122			//	Warning beep: You are being tracked by a missile! Borrowed from old repair center sounds.

#define SOUND_VOLATILE_WALL_HISS						151	//	need a hiss sound here.

#define SOUND_GOOD_SELECTION_PRIMARY				153
#define SOUND_BAD_SELECTION							156

#define SOUND_GOOD_SELECTION_SECONDARY				154		//	Adam: New sound number here! MK, 01/30/95
#define SOUND_ALREADY_SELECTED						155		//	Adam: New sound number here! MK, 01/30/95

#define SOUND_CLOAK_OFF									161	//sound when cloak goes away
#define SOUND_INVULNERABILITY_OFF					163	//sound when invulnerability goes away

#define	SOUND_BOSS_SHARE_SEE							183
#define	SOUND_BOSS_SHARE_ATTACK						184
#define	SOUND_BOSS_SHARE_DIE							185

#define	SOUND_NASTY_ROBOT_HIT_1						190	//	ding.raw	; tearing metal 1
#define	SOUND_NASTY_ROBOT_HIT_2						191	//	ding.raw	; tearing metal 2

#define	ROBOT_SEE_SOUND_DEFAULT						170
#define	ROBOT_ATTACK_SOUND_DEFAULT					171
#define	ROBOT_CLAW_SOUND_DEFAULT					190

#define 	SOUND_BIG_ENDLEVEL_EXPLOSION				SOUND_EXPLODING_WALL
#define 	SOUND_TUNNEL_EXPLOSION						SOUND_EXPLODING_WALL

#define	SOUND_DROP_BOMB								26

#define  SOUND_CHEATER									200

//--------------------------------------------------------------
#define MAX_SOUNDS 	250

//I think it would be nice to have a scrape sound... 
//#define SOUND_PLAYER_SCRAPE_WALL						72

extern uint8_t Sounds[MAX_SOUNDS];
extern uint8_t AltSounds[MAX_SOUNDS];

#endif
