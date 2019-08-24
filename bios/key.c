#include <conio.h>

#include "key.h"
#include "timer.h"

//[ISB] goddamnit we need a dependency on 2d just for I_DoEvents aaa
#include "i_gr.h"

#ifdef USE_SDL

#include "SDL_events.h"
#include "SDL_keyboard.h"

#define KEY_BUFFER_SIZE 16

//-------- Variable accessed by outside functions ---------
unsigned char 				keyd_buffer_type;		// 0=No buffer, 1=buffer ASCII, 2=buffer scans
unsigned char 				keyd_repeat;
unsigned char 				keyd_editor_mode;
unsigned char 	keyd_last_pressed;
unsigned char 	keyd_last_released;
unsigned char	keyd_pressed[256];
int				keyd_time_when_last_pressed;

typedef struct keyboard {
	unsigned short		keybuffer[KEY_BUFFER_SIZE];
	fix					time_pressed[KEY_BUFFER_SIZE];
	fix					TimeKeyWentDown[256];
	fix					TimeKeyHeldDown[256];
	unsigned int		NumDowns[256];
	unsigned int		NumUps[256];
	unsigned int 		keyhead, keytail;
	unsigned char 		E0Flag;
	unsigned char 		E1Flag;
	int 					in_key_handler;
	//void (__interrupt __far* prev_int_9)(); //[ISB] how times change
} keyboard;

static keyboard key_data;

static unsigned char Installed = 0;

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

unsigned char ascii_table[128] =
{ 255, 255, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',255,255,
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 255, 255,
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', 39, '`',
  255, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 255,'*',
  255, ' ', 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,255,255,
  255, 255, 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255 };

unsigned char shifted_ascii_table[128] =
{ 255, 255, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',255,255,
  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 255, 255,
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
  255, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 255,255,
  255, ' ', 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,255,255,
  255, 255, 255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
  255,255,255,255,255,255,255,255 };

unsigned char key_to_ascii(int keycode)
{
	int shifted;

	shifted = keycode & KEY_SHIFTED;
	keycode &= 0xFF;

	if (keycode >= 127)
		return 255;

	if (shifted)
		return shifted_ascii_table[keycode];
	else
		return ascii_table[keycode];
}

void key_flush()
{
	int i;
	fix CurTime;

	key_data.keyhead = key_data.keytail = 0;

	//Clear the keyboard buffer
	for (i = 0; i < KEY_BUFFER_SIZE; i++) 
	{
		key_data.keybuffer[i] = 0;
		key_data.time_pressed[i] = 0;
	}

	//Clear the keyboard array

	CurTime = timer_get_fixed_secondsX();

	for (i = 0; i < 256; i++) 
	{
		keyd_pressed[i] = 0;
		key_data.TimeKeyWentDown[i] = CurTime;
		key_data.TimeKeyHeldDown[i] = 0;
		key_data.NumDowns[i] = 0;
		key_data.NumUps[i] = 0;
	}
}

int add_one(int n)
{
	n++;
	if (n >= KEY_BUFFER_SIZE) n = 0;
	return n;
}

// Returns 1 if character waiting... 0 otherwise
int key_checkch()
{
	int is_one_waiting = 0;

	if (key_data.keytail != key_data.keyhead)
		is_one_waiting = 1;
	return is_one_waiting;
}

int key_inkey()
{
	int key = 0;

	if (key_data.keytail != key_data.keyhead) 
	{
		key = key_data.keybuffer[key_data.keyhead];
		key_data.keyhead = add_one(key_data.keyhead);
	}
	return key;
}

int key_inkey_time(fix* time)
{
	int key = 0;

	if (key_data.keytail != key_data.keyhead) 
	{
		key = key_data.keybuffer[key_data.keyhead];
		*time = key_data.time_pressed[key_data.keyhead];
		key_data.keyhead = add_one(key_data.keyhead);
	}

	return key;
}

int key_peekkey()
{
	int key = 0;

	if (key_data.keytail != key_data.keyhead) 
	{
		key = key_data.keybuffer[key_data.keyhead];
	}

	return key;
}

// If not installed, uses BIOS and returns getch();
//	Else returns pending key (or waits for one if none waiting).
int key_getch()
{
	if (!Installed)
		return _getch();

	while (!key_checkch())
		I_DoEvents(); //[ISB] so we can get the freakin key in the first place...

	return key_inkey();
}

unsigned int key_get_shift_status()
{
	unsigned int shift_status = 0;

	if (keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT])
		shift_status |= KEY_SHIFTED;

	if (keyd_pressed[KEY_LALT] || keyd_pressed[KEY_RALT])
		shift_status |= KEY_ALTED;

	if (keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL])
		shift_status |= KEY_CTRLED;

#ifndef NDEBUG
	if (keyd_pressed[KEY_DELETE])
		shift_status |= KEY_DEBUGGED;
#endif

	return shift_status;
}

// Returns the number of seconds this key has been down since last call.
fix key_down_time(int scancode) {
	fix time_down, time;

	if ((scancode < 0) || (scancode > 255)) return 0;

#ifndef NDEBUG
	if (keyd_editor_mode && key_get_shift_status())
		return 0;
#endif

	if (!keyd_pressed[scancode]) 
	{
		time_down = key_data.TimeKeyHeldDown[scancode];
		key_data.TimeKeyHeldDown[scancode] = 0;
	}
	else 
	{
		time = timer_get_fixed_secondsX();
		time_down = time - key_data.TimeKeyWentDown[scancode];
		key_data.TimeKeyWentDown[scancode] = time;
	}

	return time_down;
}

// Returns number of times key has went from up to down since last call.
unsigned int key_down_count(int scancode)
{
	int n;

	if ((scancode < 0) || (scancode > 255)) return 0;

	n = key_data.NumDowns[scancode];
	key_data.NumDowns[scancode] = 0;

	return n;
}


// Returns number of times key has went from down to up since last call.
unsigned int key_up_count(int scancode) {
	int n;

	if ((scancode < 0) || (scancode > 255)) return 0;

	n = key_data.NumUps[scancode];
	key_data.NumUps[scancode] = 0;

	return n;
}

void I_KeyHandler(int sc, dbool down)
{
	int scancode, keycode;
	unsigned char temp;

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
		else
			return;
	}
	else
	{
		scancode = translationTable[sc];
	}

	//Now invoke Descent's processing for the key code
	if (down == SDL_PRESSED)
	{
		// Key going down
		keyd_last_pressed = scancode;
		keyd_time_when_last_pressed = timer_get_fixed_secondsX();
		if (!keyd_pressed[scancode]) 
		{
			// First time down
			key_data.TimeKeyWentDown[scancode] = timer_get_fixed_secondsX();
			keyd_pressed[scancode] = 1;
			key_data.NumDowns[scancode]++;
#ifndef NDEBUG
			if ((keyd_pressed[KEY_LSHIFT]) && (scancode == KEY_BACKSP)) 
			{
				keyd_pressed[KEY_LSHIFT] = 0;
				//Int5(); //the hell is int 5h tbh
				//there's no obvious effect pressing LSHIFT+BACKSPACE in an editor build, so...
			}
#endif
		}
		else if (!keyd_repeat) 
		{
			// Don't buffer repeating key if repeat mode is off
			scancode = 0xAA;
		}

		if (scancode != 0xAA) 
		{
			keycode = scancode;

			if (keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT])
				keycode |= KEY_SHIFTED;

			if (keyd_pressed[KEY_LALT] || keyd_pressed[KEY_RALT])
				keycode |= KEY_ALTED;

			if (keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL])
				keycode |= KEY_CTRLED;

#ifndef NDEBUG
			if (keyd_pressed[KEY_DELETE])
				keycode |= KEY_DEBUGGED;
#endif

			temp = key_data.keytail + 1;
			if (temp >= KEY_BUFFER_SIZE) temp = 0;

			if (temp != key_data.keyhead) {
				key_data.keybuffer[key_data.keytail] = keycode;
				key_data.time_pressed[key_data.keytail] = keyd_time_when_last_pressed;
				key_data.keytail = temp;
			}
		}
	}
	else if (down == SDL_RELEASED)
	{
		// Key going up
		keyd_last_released = scancode;
		keyd_pressed[scancode] = 0;
		key_data.NumUps[scancode]++;
		temp = 0;
		temp |= keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT];
		temp |= keyd_pressed[KEY_LALT] || keyd_pressed[KEY_RALT];
		temp |= keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL];
#ifndef NDEBUG
		temp |= keyd_pressed[KEY_DELETE];
		if (!(keyd_editor_mode && temp))
#endif		// NOTICE LINK TO ABOVE IF!!!!
			key_data.TimeKeyHeldDown[scancode] += timer_get_fixed_secondsX() - key_data.TimeKeyWentDown[scancode];
	}
}

void key_init()
{
	// Initialize queue

	keyd_time_when_last_pressed = timer_get_fixed_seconds();
	keyd_buffer_type = 1;
	keyd_repeat = 1;
	key_data.in_key_handler = 0;
	key_data.E0Flag = 0;
	key_data.E1Flag = 0;

	// Clear the keyboard array
	key_flush();

	if (Installed) return;
	Installed = 1;
	
	//[ISB] no low level crap
}

void key_close()
{
	//[ISB] heh
}

#else

void I_KeyHandler(int sc, dbool down) { }

void key_init() { }
void key_close() { }

void key_flush() { }
int key_checkch() { return 0; }
int key_getch() { return 0; }
int key_inkey() { return 0; }
int key_inkey_time(fix* time) { return 0; }
int key_peekkey() { return 0; }
fix key_down_time(int scancode) { return 0; }
unsigned int key_down_count(int scancode) { return 0; }
unsigned int key_up_count(int scancode) { return 0; }
unsigned char key_to_ascii(int keycode) { return 0; }

unsigned char keyd_buffer_type;
unsigned char keyd_repeat;
unsigned char keyd_editor_mode;
int keyd_time_when_last_pressed;

unsigned char keyd_pressed[256];
unsigned char keyd_last_pressed;
unsigned char keyd_last_released;

#endif
