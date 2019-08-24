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
 * $Source: f:/miner/source/main/rcs/hostage.h $
 * $Revision: 2.0 $
 * $Author: john $
 * $Date: 1995/02/27 11:27:50 $
 *
 * Header for hostage.c
 *
 */



#ifndef _HOSTAGE_H
#define _HOSTAGE_H

#include "vclip.h"
#include "object.h"

 //#define HOSTAGE_FACES	1		//if defined, hostage faces are in

#define HOSTAGE_SIZE i2f(5)	//3d size of a hostage

#define MAX_HOSTAGE_TYPES		1
#define MAX_HOSTAGES				10		//max per any one level
#define HOSTAGE_MESSAGE_LEN	30
#define HOSTAGE_MAX_GLOBALS	10

// 1 per hostage
typedef struct hostage_data {
	short		objnum;
	int		objsig;
	//uint8_t		type;
	short		vclip_num;
	//short		sound_num;
	char		text[HOSTAGE_MESSAGE_LEN];
} hostage_data;

extern char Hostage_global_message[HOSTAGE_MAX_GLOBALS][HOSTAGE_MESSAGE_LEN];
extern int Hostage_num_globals;

extern int N_hostage_types;

extern int Num_hostages;

extern int Hostage_vclip_num[MAX_HOSTAGE_TYPES];	//for each type of hostage

extern vclip Hostage_face_clip[MAX_HOSTAGES];

extern hostage_data Hostages[MAX_HOSTAGES];

void draw_hostage(object* obj);
void hostage_rescue(int hostage_num);
void hostage_init();

//returns true if something drew
int do_hostage_effects();

void hostage_init_all();
void hostage_compress_all();
int hostage_get_next_slot();
int hostage_is_valid(int hostage_num);
int hostage_object_is_valid(int objnum);
void hostage_init_info(int objnum);

#ifdef HOSTAGE_FACES
int hostage_is_vclip_playing();
void stop_all_hostage_clips();
#else
#define hostage_is_vclip_playing() (0)
#endif


#endif
