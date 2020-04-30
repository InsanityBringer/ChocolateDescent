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

//[ISB] barely TBH but I keep their bookkeeping code...

#include "platform/mouse.h"
#include "platform/timer.h"
#include "misc/error.h"

#define MOUSE_MAX_BUTTONS	11

int Mouse_installed = 0;

float MouseScalar = 1.0f;

typedef struct event_info
{
	short x;
	short y;
	short z;
	short pitch;
	short bank;
	short heading;
	uint16_t button_status;
	uint16_t device_dependant;
} event_info;

typedef struct mouse_info
{
	fix		ctime;
	uint8_t		cyberman;
	int		num_buttons;
	uint8_t		pressed[MOUSE_MAX_BUTTONS];
	fix		time_went_down[MOUSE_MAX_BUTTONS];
	fix		time_held_down[MOUSE_MAX_BUTTONS];
	uint32_t		num_downs[MOUSE_MAX_BUTTONS];
	uint32_t		num_ups[MOUSE_MAX_BUTTONS];
	event_info* x_info;
	uint16_t	button_status;
} mouse_info;

typedef struct cyberman_info
{
	uint8_t device_type;
	uint8_t major_version;
	uint8_t minor_version;
	uint8_t x_descriptor;
	uint8_t y_descriptor;
	uint8_t z_descriptor;
	uint8_t pitch_descriptor;
	uint8_t roll_descriptor;
	uint8_t yaw_descriptor;
	uint8_t reserved;
} cyberman_info;

static mouse_info Mouse;

//--------------------------------------------------------
// returns 0 if no mouse
//           else number of buttons
int mouse_init(int enable_cyberman)
{
	Mouse_installed = 1;
	//[ISB] man did anyone own a cyberman?

	//SDL gives you five buttons, so you get five buttons. 
	return 5;
}

void mouse_close()
{
	Mouse_installed = 0;
	//[ISB] heeh
	//[ISB] I guess I could make these call SDL's init or shutdown functions?
}

int mouse_set_limits(int x1, int y1, int x2, int y2)
{
	//[ISB] this needs to set the bounds for a window. 
	return 0;
}

void mouse_flush()
{
	int i;
	fix CurTime;

	if (!Mouse_installed)
		return;

	//Clear the mouse data
	CurTime = timer_get_fixed_seconds();
	for (i = 0; i < MOUSE_MAX_BUTTONS; i++) 
	{
		Mouse.pressed[i] = 0;
		Mouse.time_went_down[i] = CurTime;
		Mouse.time_held_down[i] = 0;
		Mouse.num_downs[i] = 0;
		Mouse.num_ups[i] = 0;
	}
}

void MousePressed(int button)
{
	Mouse.ctime = timer_get_fixed_seconds();
	if (!Mouse.pressed[button])
	{
		Mouse.pressed[button] = 1;
		Mouse.time_went_down[button] = Mouse.ctime;
	}
	Mouse.num_downs[button]++;
}

void MouseReleased(int button)
{
	Mouse.ctime = timer_get_fixed_seconds();
	if (Mouse.pressed[button])
	{
		Mouse.pressed[button] = 0;
		Mouse.time_held_down[button] += Mouse.ctime - Mouse.time_went_down[button];
	}
	Mouse.num_ups[button]++;
}

// Returns how many times this button has went down since last call.
int mouse_button_down_count(int button)
{
	int count;

	count = Mouse.num_downs[button];
	Mouse.num_downs[button] = 0;

	return count;
}

// Returns 1 if this button is currently down
int mouse_button_state(int button)
{
	int state;

	state = Mouse.pressed[button];

	return state;
}

// Returns how long this button has been down since last call.
fix mouse_button_down_time(int button)
{
	fix time_down, time;

	if (!Mouse.pressed[button]) 
	{
		time_down = Mouse.time_held_down[button];
		Mouse.time_held_down[button] = 0;
	}
	else 
	{
		time = timer_get_fixed_seconds();
		time_down = time - Mouse.time_went_down[button];
		Mouse.time_went_down[button] = time;
	}

	return time_down;
}

void mouse_get_cyberman_pos(int* x, int* y)
{
	//Error("mouse_get_cyberman_pos: STUB How the hell did you get this to call???\n");
}
