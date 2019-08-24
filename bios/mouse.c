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

#ifdef USE_SDL

//[ISB] barely TBH but I keep their bookkeeping code...

#include "SDL_video.h"
#include "SDL_mouse.h"

#include "mouse.h"
#include "timer.h"
#include "error.h"

#define MOUSE_MAX_BUTTONS	11

typedef struct event_info {
	short x;
	short y;
	short z;
	short pitch;
	short bank;
	short heading;
	ushort button_status;
	ushort device_dependant;
} event_info;

typedef struct mouse_info {
	fix		ctime;
	ubyte		cyberman;
	int		num_buttons;
	ubyte		pressed[MOUSE_MAX_BUTTONS];
	fix		time_went_down[MOUSE_MAX_BUTTONS];
	fix		time_held_down[MOUSE_MAX_BUTTONS];
	uint		num_downs[MOUSE_MAX_BUTTONS];
	uint		num_ups[MOUSE_MAX_BUTTONS];
	event_info* x_info;
	ushort	button_status;
} mouse_info;

typedef struct cyberman_info {
	ubyte device_type;
	ubyte major_version;
	ubyte minor_version;
	ubyte x_descriptor;
	ubyte y_descriptor;
	ubyte z_descriptor;
	ubyte pitch_descriptor;
	ubyte roll_descriptor;
	ubyte yaw_descriptor;
	ubyte reserved;
} cyberman_info;

static mouse_info Mouse;

//variables for relative reads
int lastReadX = 0, lastReadY = 0;

//--------------------------------------------------------
// returns 0 if no mouse
//           else number of buttons
int mouse_init(int enable_cyberman)
{
	//[ISB] man did anyone own a cyberman?

	//SDL gives you five buttons, so you get five buttons. 
	return 5;
}

void mouse_close()
{
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
	//[ISB] er?
}

void mouse_get_pos(int* x, int* y)
{
	SDL_GetMouseState(x, y);
	lastReadX = *x; lastReadY = *y;
}

void mouse_get_delta(int* dx, int* dy)
{
	//hack: still return a delta in case I_SetRelative hasn't been called
	if (SDL_GetRelativeMouseMode())
	{
		SDL_GetRelativeMouseState(dx, dy);
	}
	else
	{
		int ldx, ldy;
		SDL_GetMouseState(&ldx, &ldy);
		*dx = ldx - lastReadX;
		*dy = ldy - lastReadY;
		lastReadX = ldx; lastReadY = ldy;
	}
}

int mouse_get_btns()
{
	return SDL_GetMouseState(NULL, NULL);
}

//[ISB] Okay I'll be fair, this is a parallax level hack if ever I've seen once
extern SDL_Window* gameWindow;
void mouse_set_pos(int x, int y)
{
	if (!gameWindow) return;
	SDL_WarpMouseInWindow(gameWindow, x, y);
}

//Adaption of Parallax's mouse handler
void I_MouseHandler(uint button, dbool down)
{
	Mouse.ctime = timer_get_fixed_secondsX();

	if (down && (button == SDL_BUTTON_LEFT)) // left button pressed
	{
		if (!Mouse.pressed[MB_LEFT]) {
			Mouse.pressed[MB_LEFT] = 1;
			Mouse.time_went_down[MB_LEFT] = Mouse.ctime;
		}
		Mouse.num_downs[MB_LEFT]++;
	}
	else if (!down && (button == SDL_BUTTON_LEFT)) // left button released
	{
		if (Mouse.pressed[MB_LEFT]) {
			Mouse.pressed[MB_LEFT] = 0;
			Mouse.time_held_down[MB_LEFT] += Mouse.ctime - Mouse.time_went_down[MB_LEFT];
		}
		Mouse.num_ups[MB_LEFT]++;
	}

	if (down && (button == SDL_BUTTON_RIGHT)) // right button pressed
	{
		if (!Mouse.pressed[MB_RIGHT]) {
			Mouse.pressed[MB_RIGHT] = 1;
			Mouse.time_went_down[MB_RIGHT] = Mouse.ctime;
		}
		Mouse.num_downs[MB_RIGHT]++;
	}
	else if (!down && (button == SDL_BUTTON_RIGHT)) // right button released
	{
		if (Mouse.pressed[MB_RIGHT]) {
			Mouse.pressed[MB_RIGHT] = 0;
			Mouse.time_held_down[MB_RIGHT] += Mouse.ctime - Mouse.time_went_down[MB_RIGHT];
		}
		Mouse.num_ups[MB_RIGHT]++;
	}

	if (down && (button == SDL_BUTTON_MIDDLE)) // middle button pressed
	{
		if (!Mouse.pressed[MB_MIDDLE]) {
			Mouse.pressed[MB_MIDDLE] = 1;
			Mouse.time_went_down[MB_MIDDLE] = Mouse.ctime;
		}
		Mouse.num_downs[MB_MIDDLE]++;
	}
	else if (!down && (button == SDL_BUTTON_MIDDLE)) // middle button released
	{
		if (Mouse.pressed[MB_MIDDLE]) {
			Mouse.pressed[MB_MIDDLE] = 0;
			Mouse.time_held_down[MB_MIDDLE] += Mouse.ctime - Mouse.time_went_down[MB_MIDDLE];
		}
		Mouse.num_ups[MB_MIDDLE]++;
	}

	if (down && (button == SDL_BUTTON_X1)) // button 4 pressed
	{
		if (!Mouse.pressed[MB_4]) {
			Mouse.pressed[MB_4] = 1;
			Mouse.time_went_down[MB_4] = Mouse.ctime;
		}
		Mouse.num_downs[MB_4]++;
	}
	else if (!down && (button == SDL_BUTTON_X1)) // button 4 released
	{
		if (Mouse.pressed[MB_4]) {
			Mouse.pressed[MB_4] = 0;
			Mouse.time_held_down[MB_4] += Mouse.ctime - Mouse.time_went_down[MB_4];
		}
		Mouse.num_ups[MB_4]++;
	}

	if (down && (button == SDL_BUTTON_X2)) // button 5 pressed
	{
		if (!Mouse.pressed[MB_5]) {
			Mouse.pressed[MB_5] = 1;
			Mouse.time_went_down[MB_5] = Mouse.ctime;
		}
		Mouse.num_downs[MB_5]++;
	}
	else if (!down && (button == SDL_BUTTON_X2)) // button 5 released
	{
		if (Mouse.pressed[MB_5]) {
			Mouse.pressed[MB_5] = 0;
			Mouse.time_held_down[MB_5] += Mouse.ctime - Mouse.time_went_down[MB_5];
		}
		Mouse.num_ups[MB_5]++;
	}
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
		time = timer_get_fixed_secondsX();
		time_down = time - Mouse.time_went_down[button];
		Mouse.time_went_down[button] = time;
	}

	return time_down;
}

void mouse_get_cyberman_pos(int* x, int* y)
{
	Error("mouse_get_cyberman_pos: STUB How the hell did you get this to call???\n");
}

#else

#include "mouse.h"
#include "timer.h"
#include "error.h"

int mouse_init(int enable_cyberman) { return 0; }
int mouse_set_limits(int x1, int y1, int x2, int y2) { return 0; }
void mouse_flush() { }	// clears all mice events...
void mouse_close() { }
void mouse_get_pos(int* x, int* y) { }
void mouse_get_delta(int* dx, int* dy) { }
int mouse_get_btns() { return 0; }
void mouse_set_pos(int x, int y) { }
void mouse_get_cyberman_pos(int* x, int* y) { }
fix mouse_button_down_time(int button) { return 0; }
int mouse_button_down_count(int button) { return 0; }
int mouse_button_state(int button) { return 0; }

void I_MouseHandler(uint button, dbool down) { }

#endif
