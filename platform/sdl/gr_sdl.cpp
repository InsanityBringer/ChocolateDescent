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
#include <stdlib.h>

#ifdef USE_SDL

#include "SDL.h"
#include "SDL_video.h"
#include "SDL_surface.h"
#include "SDL_pixels.h"
#include "SDL_mouse.h"
#include "SDL_render.h"

#include "2d/gr.h"
#include "platform/platform.h"
#include "misc/error.h"
#include "misc/types.h"

#include "platform/joy.h"
#include "platform/mouse.h"
#include "platform/key.h"
#include "platform/timer.h"

#include "platform/sdl/gl_sdl.h"

#define FITMODE_BEST 1
#define FITMODE_FILTERED 2

#ifdef BUILD_DESCENT2
const char* titleMsg = "Chocolate Descent ][ (" __DATE__ ")";
#else
const char* titleMsg = "Chocolate Descent (" __DATE__ ")";
#endif

int WindowWidth = 1600, WindowHeight = 900;
int CurWindowWidth, CurWindowHeight;
SDL_Window* gameWindow = NULL;
grs_canvas* screenBuffer;

int BestFit = 0;
int Fullscreen = 0;
int SwapInterval = 0;

SDL_Rect screenRectangle, sourceRectangle;
SDL_Surface* softwareSurf = nullptr;

uint32_t localPal[256];

//TODO: temp hack for easy readback, replace with saner code
SDL_Color colors[256];

int refreshDuration = US_70FPS;
bool usingSoftware = false;

int plat_init()
{
	int res;

	res = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_TIMER | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
	if (res)
	{
		Error("Error initalizing SDL: %s\n", SDL_GetError());
		return res;
	}
	//Ensure a library capable of modern functions is available. 
	/*res = SDL_GL_LoadLibrary(NULL);
	if (res)
	{
		Error("I_Init(): Cannot load default OpenGL library: %s\n", SDL_GetError());
		return res;
	}*/
	plat_read_chocolate_cfg();
	return 0;
}

int plat_create_window()
{
	//Attributes like this must be set before windows are created, apparently. 
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	CurWindowWidth = WindowWidth;
	CurWindowHeight = WindowHeight;
	int flags = SDL_WINDOW_HIDDEN;
	if (!NoOpenGL)
		flags |= SDL_WINDOW_OPENGL;
	else
		usingSoftware = true;
	if (Fullscreen)
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_BORDERLESS;
	//SDL is good, create a game window
	gameWindow = SDL_CreateWindow(titleMsg, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WindowWidth, WindowHeight, flags);
	//int result = SDL_CreateWindowAndRenderer(WindowWidth, WindowHeight, flags, &gameWindow, &renderer);
	if (Fullscreen)
		SDL_GetWindowSize(gameWindow, &CurWindowWidth, &CurWindowHeight);

	if (!gameWindow)
	{
		Error("Error creating game window: %s\n", SDL_GetError());
		return 1;
	}
	//where else do i do this...
	I_InitSDLJoysticks();

	if (!NoOpenGL && I_InitGLContext(gameWindow))
	{
		//Failed to initialize OpenGL, try simple surface code instead
		SDL_DestroyWindow(gameWindow);
		usingSoftware = true;

		flags &= ~SDL_WINDOW_OPENGL;
		gameWindow = SDL_CreateWindow(titleMsg, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WindowWidth, WindowHeight, flags);
		if (!gameWindow)
		{
			Error("Error creating game window, after falling back to software: %s\n", SDL_GetError());
			return 1;
		}
	}

	SDL_ShowWindow(gameWindow);

	return 0;
}

void plat_close_window()
{
	if (gameWindow)
	{
		if (!usingSoftware)
			I_ShutdownGL();

		SDL_DestroyWindow(gameWindow);
		gameWindow = NULL;
	}
}

int plat_check_gr_mode(int mode)
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

void I_SetScreenRect(int w, int h)
{
	//Create the destination rectangle for the game screen
	int bestWidth = CurWindowHeight * 4 / 3;
	if (CurWindowWidth < bestWidth) bestWidth = CurWindowWidth;
	sourceRectangle.x = sourceRectangle.y = 0;
	sourceRectangle.w = w; sourceRectangle.h = h;

	if (!usingSoftware)
	{
		if (BestFit == FITMODE_FILTERED && h <= 400)
		{
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
			w *= 2; h *= 2;
		}
		else
			SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
	}

	if (BestFit == FITMODE_BEST)
	{
		int numWidths = bestWidth / w;
		screenRectangle.w = numWidths * w;
		screenRectangle.h = (screenRectangle.w * 3 / 4);
		screenRectangle.x = (CurWindowWidth - screenRectangle.w) / 2;
		screenRectangle.y = (CurWindowHeight - screenRectangle.h) / 2;
	}
	else
	{
		screenRectangle.w = bestWidth;
		screenRectangle.h = (screenRectangle.w * 3 / 4);
		screenRectangle.x = screenRectangle.y = 0;
		screenRectangle.x = (CurWindowWidth - screenRectangle.w) / 2;
		screenRectangle.y = (CurWindowHeight - screenRectangle.h) / 2;
	}

	if (!usingSoftware)
		GL_SetVideoMode(w, h, &screenRectangle);
	else
	{
		if (softwareSurf)
			SDL_FreeSurface(softwareSurf);
		
		softwareSurf = SDL_CreateRGBSurface(0, w, h, 32, 0, 0, 0, 0);
		if (!softwareSurf)
			Error("Error creating software surface: %s\n", SDL_GetError());
	}
}

void plat_toggle_fullscreen()
{
	if (Fullscreen)
	{
		SDL_SetWindowFullscreen(gameWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
		SDL_GetWindowSize(gameWindow, &CurWindowWidth, &CurWindowHeight);
	}
	else
	{
		SDL_SetWindowFullscreen(gameWindow, 0);
		SDL_SetWindowSize(gameWindow, WindowWidth, WindowHeight);
		CurWindowWidth = WindowWidth; CurWindowHeight = WindowHeight;
		SDL_SetWindowPosition(gameWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	}

	I_SetScreenRect(grd_curscreen->sc_w, grd_curscreen->sc_h);
}

void plat_update_window()
{
	SDL_SetWindowSize(gameWindow, WindowWidth, WindowHeight);
	SDL_SetWindowPosition(gameWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

	plat_toggle_fullscreen();
}

int plat_set_gr_mode(int mode)
{
	int w, h;

	refreshDuration = US_60FPS;
	switch (mode)
	{
	case SM_320x200C:
	case SM_320x200U:
		w = 320; h = 200; refreshDuration = US_70FPS;
		break;
	case SM_320x240U:
		w = 320; h = 240; refreshDuration = US_70FPS; //these need to be checked
		break;
	case SM_360x200U:
		w = 360; h = 200; refreshDuration = US_70FPS;
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
		Error("plat_set_gr_mode: bad mode %d\n", mode);
		return 0;
	}

	//[ISB] this should hopefully fix all instances of the screen flashing white when changing modes
	plat_write_palette(0, 255, gr_palette);
	I_SetScreenRect(w, h);

	return 0;
}

void I_ScaleMouseToWindow(int* x, int* y)
{
	//printf("in: (%d, %d) ", *x, *y);
	*x = (*x * screenRectangle.w / CurWindowWidth);
	*y = (*y * screenRectangle.h / CurWindowHeight);
	if (*x < 0) *x = 0; if (*x >= screenRectangle.w) *x = screenRectangle.w - 1;
	if (*y < 0) *y = 0; if (*y >= screenRectangle.h) *y = screenRectangle.h - 1;
	//printf("out: (%d, %d)\n", *x, *y);
}

void plat_do_events()
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
			if (ev.key.keysym.scancode == SDL_SCANCODE_RETURN && ev.key.state == SDL_PRESSED && ev.key.keysym.mod & KMOD_ALT)
			{
				Fullscreen ^= 1;
				plat_toggle_fullscreen();
			}
			else
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

void plat_set_mouse_relative_mode(int state)
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
		SDL_WarpMouseInWindow(gameWindow, CurWindowWidth / 2, CurWindowHeight / 2);
	}
}

void plat_write_palette(int start, int end, uint8_t* data)
{
	int i;

	//TODO: don't waste time storing in the SDL color array
	for (i = 0; i <= end-start; i++)
	{
		colors[i].r = (Uint8)(data[i * 3 + 0] * 255 / 63);
		colors[i].g = (Uint8)(data[i * 3 + 1] * 255 / 63);
		colors[i].b = (Uint8)(data[i * 3 + 2] * 255 / 63);
		localPal[start+i] = (255 << 24) | (colors[i].r << 16) | (colors[i].g << 8) | (colors[i].b);
	}
	if (!usingSoftware)
		GL_SetPalette(localPal);
}

void plat_blank_palette()
{
	uint8_t pal[768];
	memset(&pal[0], 0, 768 * sizeof(uint8_t));
	plat_write_palette(0, 255, &pal[0]);
}

void plat_read_palette(uint8_t* dest)
{
	int i;
	SDL_Color color;
	for (i = 0; i < 256; i++)
	{
		color = colors[i];
		dest[i * 3 + 0] = (uint8_t)(color.r * 63 / 255);
		dest[i * 3 + 1] = (uint8_t)(color.g * 63 / 255);
		dest[i * 3 + 2] = (uint8_t)(color.b * 63 / 255);
	}
}

void plat_wait_for_vbl()
{
	//Now what is a VBL, anyways?
	//SDL_Delay(1000 / 70);
	I_MarkEnd(refreshDuration);
	I_MarkStart();
}

extern uint8_t* gr_video_memory;
void I_SoftwareBlit()
{
	int x, y;
	int sourcePitch = grd_curscreen->sc_canvas.cv_bitmap.bm_rowsize;
	int destPitch = softwareSurf->pitch;
	uint8_t* source = gr_video_memory;
	if (SDL_LockSurface(softwareSurf))
		Error("Failed to lock software surface for blitting");

	uint8_t* dest = (uint8_t*)softwareSurf->pixels;

	for (y = 0; y < softwareSurf->h; y++)
	{
		for (x = 0; x < softwareSurf->w; x++)
		{
			*(uint32_t*)&dest[x<<2] = localPal[source[x]];
		}
		source += sourcePitch;
		dest += destPitch;
	}

	SDL_UnlockSurface(softwareSurf);

	SDL_Surface* windowSurf = SDL_GetWindowSurface(gameWindow);
	//SDL_BlitSurface(softwareSurf, &sourceRectangle, windowSurf, &sourceRectangle);
	SDL_BlitScaled(softwareSurf, &sourceRectangle, windowSurf, &screenRectangle);
}

void plat_present_canvas(int sync)
{
	if (sync)
	{
		SDL_Delay(1000 / 70);
	}

	if (!usingSoftware)
	{
		GL_DrawPhase1();
		SDL_GL_SwapWindow(gameWindow);
	}
	else
	{
		I_SoftwareBlit();
		SDL_UpdateWindowSurface(gameWindow);
	}
}

void plat_blit_canvas(grs_canvas *canv)
{
	//[ISB] Under the assumption that the screen buffer is always static and valid, memcpy the contents of the canvas into it
	if (canv->cv_bitmap.bm_type == BM_SVGA)
		memcpy(gr_video_memory, canv->cv_bitmap.bm_data, canv->cv_bitmap.bm_w * canv->cv_bitmap.bm_h);
}

void plat_close()
{
	//SDL_GL_UnloadLibrary();
	plat_close_window();
	SDL_Quit();
}

void plat_display_error(const char* msg)
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Game Error", msg, gameWindow);
}

#endif
