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

#include "config.h"
#include "gamestat.h"

typedef struct control_info 
{
	fix	pitch_time;						
	fix	vertical_thrust_time;
	fix	heading_time;
	fix	sideways_thrust_time;
	fix	bank_time;
	fix	forward_thrust_time;
		
	uint8_t	rear_view_down_count;	
	uint8_t	rear_view_down_state;	
	
	uint8_t	fire_primary_down_count;
	uint8_t	fire_primary_state;
	uint8_t	fire_secondary_state;
	uint8_t	fire_secondary_down_count;
	uint8_t	fire_flare_down_count;

	uint8_t	drop_bomb_down_count;	

	uint8_t	automap_down_count;
	uint8_t	automap_state;
  
	uint8_t	afterburner_state;
	uint8_t cycle_primary_count;
	uint8_t cycle_secondary_count;
	uint8_t headlight_count;	
} control_info;

typedef struct ext_control_info 
{
	fix	pitch_time;						
	fix	vertical_thrust_time;
	fix	heading_time;
	fix	sideways_thrust_time;
	fix	bank_time;
	fix	forward_thrust_time;
		
	uint8_t	rear_view_down_count;	
	uint8_t	rear_view_down_state;	
	
	uint8_t	fire_primary_down_count;
	uint8_t	fire_primary_state;
	uint8_t	fire_secondary_state;
	uint8_t	fire_secondary_down_count;
	uint8_t	fire_flare_down_count;

	uint8_t	drop_bomb_down_count;	

	uint8_t	automap_down_count;
	uint8_t	automap_state; 
} ext_control_info;

typedef struct advanced_ext_control_info
{
	fix	pitch_time;						
	fix	vertical_thrust_time;
	fix	heading_time;
	fix	sideways_thrust_time;
	fix	bank_time;
	fix	forward_thrust_time;
		
	uint8_t	rear_view_down_count;	
	uint8_t	rear_view_down_state;	
	
	uint8_t	fire_primary_down_count;
	uint8_t	fire_primary_state;
	uint8_t	fire_secondary_state;
	uint8_t	fire_secondary_down_count;
	uint8_t	fire_flare_down_count;

	uint8_t	drop_bomb_down_count;	

	uint8_t	automap_down_count;
	uint8_t	automap_state;

// everything below this line is for version >=1.0

 vms_angvec heading;	  
 char oem_message[64]; 

// everything below this line is for version >=2.0

 vms_vector ship_pos;
 vms_matrix ship_orient;

// everything below this line is for version >=3.0
    
 uint8_t cycle_primary_count; 
 uint8_t cycle_secondary_count;  
 uint8_t afterburner_state;
 uint8_t headlight_count;

// everything below this line is for version >=4.0

 int primary_weapon_flags;
 int secondary_weapon_flags;
 uint8_t current_primary_weapon;
 uint8_t current_secondary_weapon;

 
 vms_vector force_vector;
 vms_matrix force_matrix;
 int joltinfo[3];
 int x_vibrate_info[2];
 int y_vibrate_info[2];

 int x_vibrate_clear;
 int y_vibrate_clear;

 uint8_t game_status;

 uint8_t headlight_state;
 uint8_t current_guidebot_command;

 uint8_t keyboard[128]; // scan code array, not ascii

 uint8_t Reactor_blown;
 
} advanced_ext_control_info;

extern uint8_t ExtGameStatus;
extern control_info Controls;
extern void controls_read_all();
extern void kconfig(int n, char * title );

#define NUM_KEY_CONTROLS 57
#define NUM_OTHER_CONTROLS 31
#define MAX_CONTROLS 60         //there are actually 48, so this leaves room for more   

extern uint8_t kconfig_settings[CONTROL_MAX_TYPES][MAX_CONTROLS];
extern uint8_t default_kconfig_settings[CONTROL_MAX_TYPES][MAX_CONTROLS];

extern char *control_text[CONTROL_MAX_TYPES];

extern void kc_set_controls();

// Tries to use vfx1 head tracking.
void kconfig_sense_init();

//set the cruise speed to zero
extern void reset_cruise(void);

extern int kconfig_is_axes_used(int axis);

extern void kconfig_init_external_controls(int intno, int address);
