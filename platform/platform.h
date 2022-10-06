/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License.
*/

#pragma once

#include "misc/types.h"
#include "2d/gr.h"

extern int WindowWidth, WindowHeight;
extern int BestFit;
extern int Fullscreen;
extern int SwapInterval;

extern bool NoOpenGL;

//-----------------------------------------------------------------------------
//	Graphics initalization and shutdown
//-----------------------------------------------------------------------------

//Init the framework
int plat_init();

//Load configuration
int plat_read_chocolate_cfg();

//Save configuration
void plat_save_chocolate_cfg();

//Init graphics library and create a window
int plat_create_window();

//Close any active windows
void plat_close_window();

//Shutdown framework;
void plat_close();

//Display an error message
void plat_display_error(const char* msg);

//Updates the window, if settings changed in game
void plat_update_window();

//-----------------------------------------------------------------------------
//	Setting graphics modes
//-----------------------------------------------------------------------------

//Check if a mode is okay to use
int plat_check_gr_mode(int mode);

//Set a graphics mode
int plat_set_gr_mode(int mode);

//-----------------------------------------------------------------------------
//	Screen palettes
//-----------------------------------------------------------------------------

//Copy a block of palette information into the current palette
void plat_write_palette(int start, int end, uint8_t* data);

//Blank the palette in screen memory
void plat_blank_palette();

//Read the palette back from screen memory.
void plat_read_palette(uint8_t* dest);

//-----------------------------------------------------------------------------
//	Screen operations
//-----------------------------------------------------------------------------

//I have no idea how this is going to work... attempt to wait on a VBL if possible.
void plat_wait_for_vbl();

//Draws the contents of the currently assigned graphics canvas to the current window and peform buffer swap
//Set sync to wait for v-sync while drawing.
void plat_present_canvas(int sync);

//Composition nightmare: Blit given canvas to window buffer, don't trigger redraw. This is needed for paged graphics modes in Descent 1. 
void plat_blit_canvas(grs_canvas *canv);

//-----------------------------------------------------------------------------
//	Control operations
//-----------------------------------------------------------------------------

//Run all pending events. 
void plat_do_events();

//Put the mouse in relative mode
void plat_set_mouse_relative_mode(int state);
