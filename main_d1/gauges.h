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

#pragma once

#include "fix/fix.h"
#include "2d/gr.h"
#include "main_shared/piggy.h"

#define MAX_GAUGE_BMS 80	//	increased from 56 to 80 by a very unhappy MK on 10/24/94.

extern bitmap_index Gauges[MAX_GAUGE_BMS];   // Array of all gauge bitmaps.

extern void init_gauge_canvases();
extern void close_gauge_canvases();

//extern void show_score();
//extern void show_score_added();
//extern void add_points_to_score();
//extern void add_bonus_points_to_score();

void render_gauges(void);
void init_gauges(void);
//extern void check_erase_message(void);

// Call to flash a message on the HUD
extern void HUD_render_message_frame();
extern void HUD_init_message(const char* format, ...);
extern void HUD_clear_messages();

#define gauge_message HUD_init_message

extern void draw_hud();		//draw all the HUD stuff

extern void player_dead_message(void);
#ifdef RESTORE_AFTERBURNER
extern void say_afterburner_status(void);
#endif

//fills in the coords of the hostage video window
void get_hostage_window_coords(int* x, int* y, int* w, int* h);

//from testgaug.c
//void gauge_frame(void); //[ISB] not needed

extern void update_laser_weapon_info(void);
extern void play_homing_warning(void);
void add_bonus_points_to_score(int points);
extern void add_points_to_score(int points);

typedef struct {
	uint8_t r, g, b;
} rgb;

extern rgb player_rgb[];
