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
 * $Source: f:/miner/source/main/rcs/endlevel.h $
 * $Revision: 2.0 $
 * $Author: john $
 * $Date: 1995/02/27 11:31:37 $
 *
 * Header for newfile.c
 *
 */



#ifndef _OUTSIDE_H
#define _OUTSIDE_H

#include "gr.h"
#include "object.h"
#include "vecmat.h"

extern int Endlevel_sequence;

void start_endlevel_sequence();
void render_external_scene(fix eye_offset);
void render_endlevel_frame(fix eye_offset);
void do_endlevel_frame();
void draw_exit_model();
void init_endlevel();
void stop_endlevel_sequence();


extern vms_vector mine_exit_point;
extern int exit_segnum;
extern grs_bitmap* satellite_bitmap, * station_bitmap, * exit_bitmap, * terrain_bitmap;

extern object external_explosion;
extern int ext_expl_playing;

//called for each level to load & setup the exit sequence
void load_endlevel_data(int level_num);

extern int exit_modelnum, destroyed_exit_modelnum;

void generate_starfield();
void draw_stars();
void do_endlevel_flythrough(int n);

#endif
