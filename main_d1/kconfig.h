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

#include "misc/types.h"
#include "fix/fix.h"

typedef struct control_info 
{
	fix	pitch_time;
	fix	vertical_thrust_time;
	fix	heading_time;
	fix	sideways_thrust_time;
	fix	bank_time;
	fix	forward_thrust_time;
#ifdef RESTORE_AFTERBURNER
	fix afterburner_time;
#endif

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

} control_info;

typedef struct kc_item
{
	short id;				// The id of this item
	short x, y;
	short w1;
	short w2;
	short u, d, l, r;
	short text_num1;
	uint8_t type;
	uint8_t value;		// what key,button,etc
} kc_item;

extern control_info Controls;
extern void controls_read_all();
extern void kconfig(int n, char* title);

extern uint8_t Config_digi_volume;
extern uint8_t Config_midi_volume;
extern uint8_t Config_control_type;
extern uint8_t Config_channels_reversed;
extern uint8_t Config_joystick_sensitivity;

#define CONTROL_NONE 0
#define CONTROL_JOYSTICK 1
#define CONTROL_FLIGHTSTICK_PRO 2
#define CONTROL_THRUSTMASTER_FCS 3
#define CONTROL_GRAVIS_GAMEPAD 4
#define CONTROL_MOUSE 5
#define CONTROL_CYBERMAN 6
#define CONTROL_MAX_TYPES 7

#define NUM_KEY_CONTROLS 46
#define NUM_OTHER_CONTROLS 27
#define MAX_CONTROLS 50

extern uint8_t kconfig_settings[CONTROL_MAX_TYPES][MAX_CONTROLS];
extern uint8_t default_kconfig_settings[CONTROL_MAX_TYPES][MAX_CONTROLS];

extern const char* control_text[CONTROL_MAX_TYPES];

extern void kc_set_controls();

// Tries to use vfx1 head tracking.
void kconfig_sense_init();

//set the cruise speed to zero
extern void reset_cruise(void);

extern int kconfig_is_axes_used(int axis);

extern void kconfig_init_external_controls(int intno, int address);

void kc_drawitem(kc_item* item, int is_current);
void kc_change_key(kc_item* item);
void kc_change_joybutton(kc_item* item);
void kc_change_mousebutton(kc_item* item);
void kc_change_joyaxis(kc_item* item);
void kc_change_mouseaxis(kc_item* item);
void kc_change_invert(kc_item* item);
