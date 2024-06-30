
#include "platform/key.h"
#include "platform/timer.h"

#ifdef USE_SDL

#include "SDL_events.h"
#include "SDL_keyboard.h"

//holy crap this sucks
int translationTable[] =
{ -1, -1, -1, -1, KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I,
KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T,
KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5,
KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, KEY_ENTER, KEY_ESC, KEY_BACKSP, KEY_TAB,
KEY_SPACEBAR, KEY_MINUS, KEY_EQUAL, KEY_LBRACKET, KEY_RBRACKET, KEY_SLASH,
-1 /*what the fuck is NONUSHASH?*/, KEY_SEMICOL, KEY_RAPOSTRO, KEY_LAPOSTRO,
KEY_COMMA, KEY_PERIOD, KEY_DIVIDE, KEY_CAPSLOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4,
KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12, KEY_PRINT_SCREEN, 
KEY_SCROLLOCK, KEY_PAUSE, KEY_INSERT, KEY_HOME, KEY_PAGEUP, KEY_DELETE, KEY_END, KEY_PAGEDOWN, KEY_RIGHT,
KEY_LEFT, KEY_DOWN, KEY_UP, KEY_NUMLOCK, KEY_PADDIVIDE, KEY_PADMULTIPLY, KEY_PADMINUS, KEY_PADPLUS,
KEY_PADENTER, KEY_PAD1, KEY_PAD2, KEY_PAD3, KEY_PAD4, KEY_PAD5, KEY_PAD6, KEY_PAD7,
KEY_PAD8, KEY_PAD9, KEY_PAD0, KEY_PADPERIOD, -1, -1, -1, KEY_EQUAL, -1, -1, -1, -1, -1
-1, -1, -1, -1, -1 ,-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

void I_KeyHandler(int sc, dbool down)
{
	int scancode;

	//First of all, translate the SDL key scancode into a Descent key scancode. Holy crap this code sucks
	if (sc >= 128) //another goddamned hack to avoid having to expand the annoying table
	{
		if (sc == SDL_SCANCODE_LCTRL)
			scancode = KEY_LCTRL;
		else if (sc == SDL_SCANCODE_RCTRL)
			scancode = KEY_RCTRL;
		else if (sc == SDL_SCANCODE_LALT)
			scancode = KEY_LALT;
		else if (sc == SDL_SCANCODE_RALT)
			scancode = KEY_RALT;
		else if (sc == SDL_SCANCODE_LSHIFT)
			scancode = KEY_LSHIFT;
		else if (sc == SDL_SCANCODE_RSHIFT)
			scancode = KEY_RSHIFT;
		else if (sc == SDL_SCANCODE_LGUI)
			scancode = KEY_LCMD;
		else if (sc == SDL_SCANCODE_RGUI)
			scancode = KEY_RCMD;
		else
			return;
	}
	else
	{
		scancode = translationTable[sc];
	}

	if (down == SDL_PRESSED)
		KeyPressed(scancode);
	else if (down == SDL_RELEASED)
		KeyReleased(scancode);
}

#endif
