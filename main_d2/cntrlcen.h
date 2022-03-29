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

#include "vecmat/vecmat.h"
#include "object.h"
#include "wall.h"
//#include "switch.h"

#define MAX_CONTROLCEN_GUNS 		8

#define CONTROLCEN_WEAPON_NUM		6

#define	MAX_CONTROLCEN_LINKS		10

typedef struct control_center_triggers
{
	short		num_links;
	short 	seg[MAX_CONTROLCEN_LINKS];
	short		side[MAX_CONTROLCEN_LINKS];
} control_center_triggers;

extern control_center_triggers ControlCenterTriggers;

typedef struct reactor
{
	int	model_num;
	int	n_guns;
	vms_vector gun_points[MAX_CONTROLCEN_GUNS];
	vms_vector gun_dirs[MAX_CONTROLCEN_GUNS];
} reactor;

#define MAX_REACTORS 7

extern int Num_reactors;

extern reactor Reactors[MAX_REACTORS];

//@@extern int	N_controlcen_guns;
extern int	Control_center_been_hit;
extern int	Control_center_player_been_seen;
extern int	Control_center_next_fire_time;
extern int	Control_center_present;
extern int Dead_controlcen_object_num;

//@@extern vms_vector controlcen_gun_points[MAX_CONTROLCEN_GUNS];
//@@extern vms_vector controlcen_gun_dirs[MAX_CONTROLCEN_GUNS];
extern vms_vector Gun_pos[MAX_CONTROLCEN_GUNS];

//do whatever this thing does in a frame
extern void do_controlcen_frame(object *obj);

//	Initialize control center for a level.
//	Call when a new level is started.
extern void init_controlcen_for_level(void);

extern void do_controlcen_destroyed_stuff(object *objp);
extern void do_controlcen_dead_frame(void);

#define DEFAULT_CONTROL_CENTER_EXPLOSION_TIME 30		//	Note: Usually uses Alan_pavlish_reactor_times, but can be overridden in editor.

extern fix Countdown_timer;
extern int Control_center_destroyed,Countdown_seconds_left;
extern int Base_control_center_explosion_time;		//how long to blow up on insane
extern int Reactor_strength;

#include <stdio.h>

void read_reactor_triggers(control_center_triggers* trigger, FILE* fp);
void write_reactor_triggers(control_center_triggers* trigger, FILE* fp);
