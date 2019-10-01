/*
*	Horrid attempt at a system-indemendent grpahics implementation.
*	It probably only works with SDL tbh.
*/

#pragma once

#include "misc/types.h"
#include "2d/gr.h"

extern int WindowWidth, WindowHeight;
extern int BestFit;
extern int Fullscreen;

//-----------------------------------------------------------------------------
//	Graphics initalization and shutdown
//-----------------------------------------------------------------------------

//Init the framework
int I_Init();

//Load configuration
int I_ReadChocolateConfig();

//Init graphics library and create a window
int I_InitWindow();

//Close any active windows
void I_ShutdownGraphics();

//Shutdown framework;
void I_Shutdown();

//Display an error message
void I_DisplayError(const char* msg);

//-----------------------------------------------------------------------------
//	Setting graphics modes
//-----------------------------------------------------------------------------

//Check if a mode is okay to use
int I_CheckMode(int mode);

//Set a graphics mode
int I_SetMode(int mode);

//-----------------------------------------------------------------------------
//	Screen palettes
//-----------------------------------------------------------------------------

//Copy a block of palette information into the current palette
void I_WritePalette(int start, int end, uint8_t* data);

//Blank the palette in screen memory
void I_BlankPalette();

//Read the palette back from screen memory.
void I_ReadPalette(uint8_t* dest);

//-----------------------------------------------------------------------------
//	Screen operations
//-----------------------------------------------------------------------------

//I have no idea how this is going to work... attempt to wait on a VBL if possible.
void I_WaitVBL();

//Draws the contents of the currently assigned graphics canvas to the current window and peform buffer swap
//Set sync to wait for v-sync while drawing.
void I_DrawCurrentCanvas(int sync);

//Composition nightmare: Blit given canvas to window buffer, don't trigger redraw. This is needed for paged graphics modes in Descent 1. 
void I_BlitCanvas(grs_canvas *canv);

//More nightmare: Set the "screen canvas"
void I_SetScreenCanvas(grs_canvas* canv);

//-----------------------------------------------------------------------------
//	Control operations
//-----------------------------------------------------------------------------

//Run all pending events. 
void I_DoEvents();

//Put the mouse in relative mode
void I_SetRelative(int state);
