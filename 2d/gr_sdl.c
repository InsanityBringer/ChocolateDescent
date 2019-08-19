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

#include "sdl.h"
#include "sdl_video.h"
#include "sdl_surface.h"
#include "sdl_pixels.h"
#include "sdl_mouse.h"
#include "sdl_render.h"

#include "gr.h"
#include "i_gr.h"
#include "error.h"
#include "types.h"

#include "mouse.h"
#include "key.h"

int WindowWidth = 1600, WindowHeight = 900;
SDL_Window* gameWindow = NULL;
SDL_Renderer* renderer = NULL;
SDL_Surface* gameSurface = NULL;
SDL_Surface* hackSurface = NULL;
SDL_Texture* gameTexture = NULL;
grs_canvas* screenBuffer;

SDL_bool bestFit = SDL_FALSE;

SDL_Rect screenRectangle;

int I_Init()
{
	int res;

	res = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_TIMER);
	if (res)
	{
		Warning("Error initalizing SDL: %s\n", SDL_GetError());
		return res;
	}
	return 0;
}

int I_InitWindow()
{
	//SDL is good, create a game window
	//gameWindow = SDL_CreateWindow("it's a video game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WindowWidth, WindowHeight, /*SDL_WINDOW_INPUT_GRABBED*/0);
	int result = SDL_CreateWindowAndRenderer(WindowWidth, WindowHeight, 0, &gameWindow, &renderer);

	if (result)
	{
		Warning("Error creating game window: %s\n", SDL_GetError());
		return 1;
	}

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
	case 19: return 0;
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
	default:
		Error("I_SetMode: bad mode %d\n", mode);
		return 0;
	}

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

	if (oldSurf) 
		SDL_FreeSurface(oldSurf);
	if (oldHackSurf)
		SDL_FreeSurface(oldHackSurf);
	if (oldTex)
		SDL_DestroyTexture(oldTex);

	//Create the destination rectangle for the game screen
	int bestWidth = WindowHeight * 4 / 3;
	if (WindowWidth < bestWidth) bestWidth = WindowWidth;

	if (bestFit)
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
				printf("Hi you clicked my window. Isn't that great?\n");
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
		}
	}

}

void I_SetRelative(int state)
{
	if (state == 0 && SDL_GetRelativeMouseMode() == SDL_TRUE)
	{
		SDL_WarpMouseInWindow(gameWindow, WindowWidth / 2, WindowHeight / 2);
	}
	SDL_SetRelativeMouseMode(state);
}

void I_WritePalette(int start, int end, ubyte* data)
{
	SDL_Palette *pal;
	SDL_Color colors[256];
	int i;
	if (!gameSurface) return;
	pal = gameSurface->format->palette;
	if (!pal) return;

	for (i = start; i <= end; i++)
	{
		colors[i].r = (Uint8)(data[i * 3 + 0] * 255 / 63);
		colors[i].g = (Uint8)(data[i * 3 + 1] * 255 / 63);
		colors[i].b = (Uint8)(data[i * 3 + 2] * 255 / 63);
	}
	SDL_SetPaletteColors(pal, &colors[0], start, end-start+1);
}

void I_BlankPalette()
{
	ubyte pal[768];
	memset(&pal[0], 0, 768 * sizeof(ubyte));
	I_WritePalette(0, 255, &pal[0]);
}

void I_ReadPalette(ubyte* dest)
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
		dest[i * 3 + 0] = (ubyte)(color.r * 63 / 255);
		dest[i * 3 + 1] = (ubyte)(color.g * 63 / 255);
		dest[i * 3 + 2] = (ubyte)(color.b * 63 / 255);
	}
}

void I_WaitVBL()
{
	//Now what is a VBL, anyways?
}

void I_DrawCurrentCanvas(int sync)
{
	if (sync)
	{
		SDL_Delay(1000 / 70);
	}
	//Uint8* pixels;
	SDL_Surface* winSurface;
	SDL_Rect src, dest;

	if (!gameSurface) return;

	unsigned char* pixels = (unsigned char*)gameSurface->pixels;

	SDL_LockSurface(gameSurface);
	memcpy(pixels, screenBuffer->cv_bitmap.bm_data, screenBuffer->cv_bitmap.bm_w * screenBuffer->cv_bitmap.bm_h); //[ISB] alternate attempt at this nonsense
	SDL_UnlockSurface(gameSurface);

	src.x = src.y = 0;
	src.w = grd_curscreen->sc_w; src.h = grd_curscreen->sc_h;

	dest.x = dest.y = 0; //dest.w = WindowWidth-1; dest.h = WindowHeight-1;

	//draw to window
	if (SDL_BlitSurface(gameSurface, &src, hackSurface, &dest))
	{
		Warning("Cannot blit subscreen: %s\n", SDL_GetError());
	}

	//I hate this
	unsigned char* texPixels;
	int pitch;
	SDL_LockTexture(gameTexture, NULL, &texPixels, &pitch);
	SDL_LockSurface(hackSurface);
	pixels = (unsigned char*)hackSurface->pixels;
	for (int i = 0; i < screenBuffer->cv_bitmap.bm_h; i++)
	{
		memcpy(texPixels, pixels, grd_curscreen->sc_w * 4);
		texPixels += pitch;
		pixels += grd_curscreen->sc_w * 4;
	}
	SDL_UnlockTexture(gameTexture);
	SDL_UnlockSurface(hackSurface);

	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, gameTexture, &src, &screenRectangle);
	SDL_RenderPresent(renderer);

	/*
	dest.w = WindowWidth; dest.h = WindowHeight;
	if (SDL_BlitScaled(hackSurface, &src, winSurface, &dest))
	{
		Warning("Cannot blit screen: %s\n", SDL_GetError());
	}
	SDL_UpdateWindowSurface(gameWindow);*/
}

void I_BlitCurrentCanvas()
{
}

void I_BlitCanvas(grs_canvas *canv)
{
}

void I_Shutdown()
{
	SDL_Quit();
}