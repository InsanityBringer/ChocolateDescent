/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License.
*/
/*
*	Code for SDL integration of the GR library
*
*/

#include <string.h>
#include <stdio.h>

#ifdef USE_SDL

#include "SDL.h"
#include "SDL_video.h"
#include "SDL_surface.h"
#include "SDL_pixels.h"
#include "SDL_mouse.h"
#include "SDL_render.h"

#include "2d/gr.h"
#include "2d/i_gr.h"
#include "misc/error.h"
#include "misc/types.h"

#include "platform/joy.h"
#include "platform/mouse.h"
#include "platform/key.h"
#include "platform/timer.h"

#define FITMODE_BEST 1
#define FITMODE_FILTERED 2

int WindowWidth = 1600, WindowHeight = 900;
SDL_Window* gameWindow = NULL;
SDL_Renderer* renderer = NULL;
SDL_Surface* gameSurface = NULL;
SDL_Surface* hackSurface = NULL;
SDL_Texture* gameTexture = NULL;
grs_canvas* screenBuffer;

int BestFit = 0;
int Fullscreen = 0;

SDL_Rect screenRectangle;

uint32_t localPal[256];

int I_Init()
{
	int res;

	res = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_TIMER | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
	if (res)
	{
		Warning("Error initalizing SDL: %s\n", SDL_GetError());
		return res;
	}
	I_ReadChocolateConfig();
	return 0;
}

int I_InitWindow()
{
	//SDL is good, create a game window
	//gameWindow = SDL_CreateWindow("it's a video game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WindowWidth, WindowHeight, /*SDL_WINDOW_INPUT_GRABBED*/0);
	int flags = 0;
	if (Fullscreen)
		flags = SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_BORDERLESS;
	int result = SDL_CreateWindowAndRenderer(WindowWidth, WindowHeight, flags, &gameWindow, &renderer);
	if (Fullscreen)
		SDL_GetWindowSize(gameWindow, &WindowWidth, &WindowHeight);

	if (result)
	{
		Warning("Error creating game window: %s\n", SDL_GetError());
		return 1;
	}
	//where else do i do this...
	I_InitSDLJoysticks();

	return 0;
}

void I_ShutdownGraphics()
{
	if (gameWindow)
		SDL_DestroyWindow(gameWindow);
}

int I_CheckMode(int mode)
{
	//For now, high color modes are rejected (were those ever well supported? or even used?)
	switch (mode)
	{
	case SM_320x200C:
	case SM_320x200U:
	case SM_320x240U:
	case SM_360x200U:
	case SM_360x240U:
	case SM_376x282U:
	case SM_320x400U:
	case SM_320x480U:
	case SM_360x400U:
	case SM_360x480U:
	case SM_360x360U:
	case SM_376x308U:
	case SM_376x564U:
	case SM_640x400V:
	case SM_640x480V:
	case SM_800x600V:
	case SM_1024x768V:
	case 19:
	case 21:
	case SM_1280x1024V: return 0;
	}
	return 11;
}

void I_SetScreenCanvas(grs_canvas* canv)
{
	screenBuffer = canv;
}

int I_SetMode(int mode)
{
	int w, h;
	SDL_Surface* oldSurf, * oldHackSurf;
	SDL_Texture* oldTex;
	//Retain the old surface to bring over properties
	oldSurf = gameSurface;
	oldHackSurf = hackSurface;
	oldTex = gameTexture;

	switch (mode)
	{
	case SM_320x200C:
	case SM_320x200U:
		w = 320; h = 200;
		break;
	case SM_320x240U:
		w = 320; h = 240;
		break;
	case SM_360x200U:
		w = 360; h = 200;
		break;
	case SM_360x240U:
		w = 360; h = 240;
		break;
	case SM_376x282U:
		w = 376; h = 282;
		break;
	case SM_320x400U:
		w = 320; h = 400;
		break;
	case SM_320x480U:
		w = 320; h = 480;
		break;
	case SM_360x400U:
		w = 360; h = 400;
		break;
	case SM_360x480U:
		w = 360; h = 480;
		break;
	case SM_376x308U:
		w = 376; h = 308;
		break;
	case SM_376x564U:
		w = 376; h = 564;
		break;
	case SM_640x400V:
		w = 640; h = 400;
		break;
	case SM_640x480V:
		w = 640; h = 480;
		break;
	case SM_800x600V:
		w = 800; h = 600;
		break;
	case SM_1024x768V:
		w = 1024; h = 768;
		break;
	case 19:
		w = 320; h = 100;
		break;
	case 21:
		w = 160; h = 100;
		break;
	case SM_1280x1024V:
		w = 1280; h = 1024;
		break;
	default:
		Error("I_SetMode: bad mode %d\n", mode);
		return 0;
	}

	if (BestFit == FITMODE_FILTERED && h <= 400)
	{
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
		w *= 2; h *= 2;
	}
	else
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

	//What horrid screen code tbh
	gameSurface = SDL_CreateRGBSurface(0, w, h, 8, 0, 0, 0, 0);
	if (!gameSurface)
		Error("Error creating surface for mode %d: %s\n", mode, SDL_GetError());

	hackSurface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA8888);
	if (!hackSurface)
		Error("Error creating RGB surface for mode %d: %s\n", mode, SDL_GetError());

	gameTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, w, h);
	if (!gameTexture)
		Error("Error creating renderer texture for mode %d: %s\n", mode, SDL_GetError());

	//[ISB] this should hopefully fix all instances of the screen flashing white when changing modes
	I_WritePalette(0, 255, gr_palette);

	if (oldSurf) 
		SDL_FreeSurface(oldSurf);
	if (oldHackSurf)
		SDL_FreeSurface(oldHackSurf);
	if (oldTex)
		SDL_DestroyTexture(oldTex);

	//Create the destination rectangle for the game screen
	int bestWidth = WindowHeight * 4 / 3;
	if (WindowWidth < bestWidth) bestWidth = WindowWidth;

	if (BestFit == FITMODE_BEST)
	{
		int numWidths = bestWidth / w;
		screenRectangle.w = numWidths * w;
		screenRectangle.h = (screenRectangle.w * 3 / 4);
		screenRectangle.x = (WindowWidth - screenRectangle.w) / 2;
		screenRectangle.y = (WindowHeight - screenRectangle.h) / 2;
	}
	else
	{	
		screenRectangle.w = bestWidth;
		screenRectangle.h = (screenRectangle.w * 3 / 4);
		screenRectangle.x = screenRectangle.y = 0;
		screenRectangle.x = (WindowWidth - screenRectangle.w) / 2;
		screenRectangle.y = (WindowHeight - screenRectangle.h) / 2;
	}

	return 0;
}

void I_ScaleMouseToWindow(int* x, int* y)
{
	//printf("in: (%d, %d) ", *x, *y);
	*x = (*x * screenRectangle.w / WindowWidth);
	*y = (*y * screenRectangle.h / WindowHeight);
	if (*x < 0) *x = 0; if (*x >= screenRectangle.w) *x = screenRectangle.w - 1;
	if (*y < 0) *y = 0; if (*y >= screenRectangle.h) *y = screenRectangle.h - 1;
	//printf("out: (%d, %d)\n", *x, *y);
}

void I_DoEvents()
{
	SDL_Event ev;
	while (SDL_PollEvent(&ev))
	{
		switch (ev.type)
		{
			//Flush input if you click the window, so that you don't abort your game when clicking back in at the ESC menu. heh...
		case SDL_WINDOWEVENT:
		{
			SDL_WindowEvent winEv = ev.window;
			switch (winEv.event)
			{
			case SDL_WINDOWEVENT_FOCUS_GAINED:
				SDL_FlushEvents(SDL_MOUSEBUTTONDOWN, SDL_MOUSEBUTTONUP);
				break;
			}
		}
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			I_MouseHandler(ev.button.button, ev.button.state);
			break;
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			I_KeyHandler(ev.key.keysym.scancode, ev.key.state);
			break;
			//[ISB] kill this. Descent's joystick code expects buttons to report that they're constantly being held down, and these button events only fire when the state changes
/*
		case SDL_CONTROLLERAXISMOTION:
		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP:
			I_ControllerHandler();
			break;
		case SDL_JOYAXISMOTION:
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
		case SDL_JOYHATMOTION:
			I_JoystickHandler();
			break;*/
		}
	}

	I_JoystickHandler();
	I_ControllerHandler();
}

void I_SetRelative(int state)
{
	SDL_bool formerState = SDL_GetRelativeMouseMode();
	SDL_SetRelativeMouseMode((SDL_bool)state);
	if (state && !formerState)
	{
		int bogusX, bogusY;
		SDL_GetRelativeMouseState(&bogusX, &bogusY);
	}
	else if (!state && formerState)
	{
		SDL_WarpMouseInWindow(gameWindow, WindowWidth / 2, WindowHeight / 2);
	}
}

void I_WritePalette(int start, int end, uint8_t* data)
{
	SDL_Palette *pal;
	SDL_Color colors[256];
	int i;
	if (!gameSurface) return;
	pal = gameSurface->format->palette;
	if (!pal) return;

	for (i = 0; i <= end-start; i++)
	{
		colors[i].r = (Uint8)(data[i * 3 + 0] * 255 / 63);
		colors[i].g = (Uint8)(data[i * 3 + 1] * 255 / 63);
		colors[i].b = (Uint8)(data[i * 3 + 2] * 255 / 63);
		localPal[start+i] = 255 | (colors[i].r << 24) | (colors[i].g << 16) | (colors[i].b << 8);
	}
	SDL_SetPaletteColors(pal, &colors[0], start, end-start+1);
}

void I_BlankPalette()
{
	uint8_t pal[768];
	memset(&pal[0], 0, 768 * sizeof(uint8_t));
	I_WritePalette(0, 255, &pal[0]);
}

void I_ReadPalette(uint8_t* dest)
{
	int i;
	SDL_Palette* pal;
	SDL_Color color;
	if (!gameSurface) return;
	pal = gameSurface->format->palette;
	if (!pal) return;
	for (i = 0; i < 256; i++)
	{
		color = pal->colors[i];
		dest[i * 3 + 0] = (uint8_t)(color.r * 63 / 255);
		dest[i * 3 + 1] = (uint8_t)(color.g * 63 / 255);
		dest[i * 3 + 2] = (uint8_t)(color.b * 63 / 255);
	}
}

void I_WaitVBL()
{
	//Now what is a VBL, anyways?
	//SDL_Delay(1000 / 70);
	I_MarkEnd(US_70FPS);
	I_MarkStart();
}

void I_DrawCurrentCanvas(int sync)
{
	if (sync)
	{
		SDL_Delay(1000 / 70);
	}
	SDL_Rect src, dest;

	if (!gameSurface) return;

	src.x = src.y = 0;
	//src.w = grd_curscreen->sc_w; src.h = grd_curscreen->sc_h;
	src.w = gameSurface->w; src.h = gameSurface->h;

	dest.x = dest.y = 0; //dest.w = WindowWidth-1; dest.h = WindowHeight-1;

	uint32_t* texPixels;
	int pitch;
	SDL_LockTexture(gameTexture, NULL, (void**)&texPixels, &pitch);
	uint8_t *pixels = screenBuffer->cv_bitmap.bm_data;
	int iterations = screenBuffer->cv_bitmap.bm_h * screenBuffer->cv_bitmap.bm_w;
	for (int i = 0; i < iterations; i+=8)
	{
		texPixels[i] = localPal[pixels[i]];
		texPixels[i + 1] = localPal[pixels[i + 1]];
		texPixels[i + 2] = localPal[pixels[i + 2]];
		texPixels[i + 3] = localPal[pixels[i + 3]];
		texPixels[i + 4] = localPal[pixels[i + 4]];
		texPixels[i + 5] = localPal[pixels[i + 5]];
		texPixels[i + 6] = localPal[pixels[i + 6]];
		texPixels[i + 7] = localPal[pixels[i + 7]];
		//texPixels++;
		//pixels++;
	}
	SDL_UnlockTexture(gameTexture);

	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, gameTexture, &src, &screenRectangle);
	SDL_RenderPresent(renderer);
}

void I_BlitCanvas(grs_canvas *canv)
{
	//[ISB] Under the assumption that the screen buffer is always static and valid, memcpy the contents of the canvas into it
	if (canv->cv_bitmap.bm_type == BM_SVGA)
		memcpy(screenBuffer->cv_bitmap.bm_data, canv->cv_bitmap.bm_data, canv->cv_bitmap.bm_w * canv->cv_bitmap.bm_h);
}

void I_Shutdown()
{
	SDL_Quit();
}

void I_DisplayError(const char* msg)
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Game Error", msg, gameWindow);
}

#endif
