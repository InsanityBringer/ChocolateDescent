#include <stdio.h>

#include "platform/mouse.h"
#include "platform/timer.h"
#include "misc/error.h"

#ifdef USE_SDL

#include "SDL_video.h"
#include "SDL_mouse.h"

//variables for relative reads
int lastReadX = 0, lastReadY = 0;

//[ISB] argh but I don't want this a public function ATM
extern void I_ScaleMouseToWindow(int* x, int* y);

void mouse_get_pos(int* x, int* y)
{
	SDL_GetMouseState(x, y);
	I_ScaleMouseToWindow(x, y);
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
	*dx = (int)(*dx * MouseScalar);
	*dy = (int)(*dy * MouseScalar);
}

#define mousebtn(x) (1 << x)

int mouse_get_btns()
{
	int realbuttons = 0, sdlbuttons;
	sdlbuttons = SDL_GetMouseState(NULL, NULL);
	if (sdlbuttons & SDL_BUTTON(1))
		realbuttons |= mousebtn(0);
	if (sdlbuttons & SDL_BUTTON(2))
		realbuttons |= mousebtn(2);
	if (sdlbuttons & SDL_BUTTON(3))
		realbuttons |= mousebtn(1);
	if (sdlbuttons & SDL_BUTTON(4))
		realbuttons |= mousebtn(3);
	if (sdlbuttons & SDL_BUTTON(5))
		realbuttons |= mousebtn(4);

	//printf("realbuttons %d\n", realbuttons);

	return realbuttons;
}

//[ISB] Okay I'll be fair, this is a parallax level hack if ever I've seen once
extern SDL_Window* gameWindow;
void mouse_set_pos(int x, int y)
{
	if (!gameWindow) return;
	SDL_WarpMouseInWindow(gameWindow, x, y);
}

void I_MouseHandler(uint32_t button, dbool down)
{
	if (down && (button == SDL_BUTTON_LEFT)) MousePressed(MBUTTON_LEFT);
	else if (!down && (button == SDL_BUTTON_LEFT)) MouseReleased(MBUTTON_LEFT);
	else if (down && (button == SDL_BUTTON_RIGHT)) MousePressed(MBUTTON_RIGHT);
	else if (!down && (button == SDL_BUTTON_RIGHT)) MouseReleased(MBUTTON_RIGHT);
	else if (down && (button == SDL_BUTTON_MIDDLE)) MousePressed(MBUTTON_MIDDLE);
	else if (!down && (button == SDL_BUTTON_MIDDLE)) MouseReleased(MBUTTON_MIDDLE);
	else if (down && (button == SDL_BUTTON_X1)) MousePressed(MBUTTON_4);
	else if (!down && (button == SDL_BUTTON_X1)) MouseReleased(MBUTTON_4);
	else if (down && (button == SDL_BUTTON_X2)) MousePressed(MBUTTON_5);
	else if (!down && (button == SDL_BUTTON_X2)) MouseReleased(MBUTTON_5);
}

#endif
