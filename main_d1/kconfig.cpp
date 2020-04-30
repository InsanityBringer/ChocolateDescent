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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "misc/error.h"
#include "misc/types.h"
#include "2d/gr.h"
#include "platform/mono.h"
#include "platform/key.h"
#include "2d/palette.h"
#include "game.h"
#include "gamefont.h"
#include "iff/iff.h"
#include "mem/mem.h"
#include "platform/joy.h"
#include "platform/mouse.h"
#include "kconfig.h"
#include "gauges.h"
#include "joydefs.h"
#include "render.h"
#include "arcade.h"
#include "digi.h"
#include "newmenu.h"
#include "endlevel.h"
#include "multi.h"
#include "platform/timer.h"
#include "text.h"
#include "player.h"
#include "menu.h"
#include "args.h"
#include "2d/i_gr.h"

//#define TABLE_CREATION 1

int     sense_function1 = 0;
int	  vfx1_installed = 0;
int     SenseStatus1(void);

// Array used to 'blink' the cursor while waiting for a keypress.
int8_t fades[64] = { 1,1,1,2,2,3,4,4,5,6,8,9,10,12,13,15,16,17,19,20,22,23,24,26,27,28,28,29,30,30,31,31,31,31,31,30,30,29,28,28,27,26,24,23,22,20,19,17,16,15,13,12,10,9,8,6,5,4,4,3,2,2,1,1 };

//char * invert_text[2] = { "N", "Y" };
//char * joybutton_text[28] = { "BTN 1", "BTN 2", "BTN 3", "BTN 4", "", "TRIG", "LEFT", "HAT �", "RIGHT", "", "", "HAT �", "MID", "", "", "HAT ", "", "", "", "HAT �", "TRIG", "LEFT", "RIGHT", "", "UP","DOWN","LEFT", "RIGHT" };
//char * joyaxis_text[4] = { "X1", "Y1", "X2", "Y2" };
//char * mouseaxis_text[2] = { "L/R", "F/B" };
//char * mousebutton_text[3] = { "Left", "Right", "Mid" };

const char* choco_gamepad_text[28] =
{ "0", "1", "2", "3",
"RTRIG", "LTRIG", "LB", "PADL",
"RB", "9", "A", "PADD",
"12", "13", "B", "PADR",
"X", "17", "Y", "PADU",
"20", "21", "22", "23",
"24", "25", "26", "27" };

const char* choco_joybutton_text[28] =
//Basic inputs
{ "BTN 1", "BTN 2", "BTN 3", "BTN 4",
//"Extended" Flightstick inputs, default ATM. 
"BTN 1", "BTN 2", "BTN 3", "HAT �",
"BTN 4", "BTN 5", "BTN 6", "HAT �",
"BTN 7", "BTN 8", "BTN 9", "HAT ",
"BTN 10", "BTN 11", "BTN 12", "HAT �",
//[ISB] can't bind above 20...
"-20-", "-21-", "-22-", "-23-",
"-24-", "-25-", "-26-", "-27-" };

int invert_text[2] = { TNUM_N, TNUM_Y };
int joybutton_text[28] =
{ TNUM_BTN_1, TNUM_BTN_2, TNUM_BTN_3, TNUM_BTN_4,
  -1, TNUM_TRIG, TNUM_LEFT, TNUM_HAT_L,
 TNUM_RIGHT, -1, TNUM_HAT2_D, TNUM_HAT_R,
 TNUM_MID, -1, TNUM_HAT2_R, TNUM_HAT_U,
 TNUM_HAT2_L, -1, TNUM_HAT2_U, TNUM_HAT_D,
 TNUM_TRIG, TNUM_LEFT, TNUM_RIGHT, -1,
  TNUM_UP, TNUM_DOWN, TNUM_LEFT, TNUM_RIGHT };
int joyaxis_text[4] = { TNUM_X1, TNUM_Y1, TNUM_X2, TNUM_Y2 };
int mouseaxis_text[2] = { TNUM_L_R, TNUM_F_B };
int mousebutton_text[3] = { TNUM_LEFT, TNUM_RIGHT, TNUM_MID };

const char* key_text[256] = { \
"","ESC","1","2","3","4","5","6","7","8","9","0","-", 			\
"=","BSPC","TAB","Q","W","E","R","T","Y","U","I","O",				\
"P","[","]","�","LCTRL","A","S","D","F",        \
"G","H","J","K","L",";","'","`",        \
"LSHFT","\\","Z","X","C","V","B","N","M",",",      \
".","/","RSHFT","PAD*","LALT","SPC",      \
"CPSLK","F1","F2","F3","F4","F5","F6","F7","F8","F9",        \
"F10","NMLCK","SCLK","PAD7","PAD8","PAD9","PAD-",   \
"PAD4","PAD5","PAD6","PAD+","PAD1","PAD2","PAD3","PAD0", \
"PAD.","","","","F11","F12","","","","","","","","","",         \
"","","","","","","","","","","","","","","","","","","","",     \
"","","","","","","","","","","","","","","","","","","","",     \
"","","","","","","","","","","","","","","","","","",           \
"PAD�","RCTRL","","","","","","","","","","","","","", \
"","","","","","","","","","","PAD/","","","RALT","",      \
"","","","","","","","","","","","","","HOME","�","PGUP",     \
"","�","","","","END","�","PGDN","INS",       \
"DEL","","","","","","","","","","","","","","","","","",     \
"","","","","","","","","","","","","","","","","","","","",     \
"","","","","","","" };

uint8_t system_keys[] = { KEY_ESC, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12, KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, KEY_MINUS, KEY_EQUAL, KEY_PRINT_SCREEN };

//extern void GameLoop(int, int );

control_info Controls;

uint8_t Config_digi_volume = 16;
uint8_t Config_midi_volume = 16;
uint8_t Config_control_type = 0;
uint8_t Config_channels_reversed = 0;
uint8_t Config_joystick_sensitivity = 8;

fix Cruise_speed = 0;

#define BT_KEY 				0
#define BT_MOUSE_BUTTON 	1
#define BT_MOUSE_AXIS		2
#define BT_JOY_BUTTON 		3
#define BT_JOY_AXIS			4
#define BT_INVERT				5

const char* btype_text[] = { "BT_KEY", "BT_MOUSE_BUTTON", "BT_MOUSE_AXIS", "BT_JOY_BUTTON", "BT_JOY_AXIS", "BT_INVERT" };

#define INFO_Y 28

int Num_items = 23;
kc_item* All_items;

uint8_t kconfig_settings[CONTROL_MAX_TYPES][MAX_CONTROLS];

//----------- WARNING!!!!!!! -------------------------------------------
// THESE NEXT FOUR BLOCKS OF DATA ARE GENERATED BY PRESSING DEL+F12 WHEN
// IN THE KEYBOARD CONFIG SCREEN.  BASICALLY, THAT PROCEDURE MODIFIES THE
// U,D,L,R FIELDS OF THE ARRAYS AND DUMPS THE NEW ARRAYS INTO KCONFIG.COD
//-------------------------------------------------------------------------

uint8_t default_kconfig_settings[CONTROL_MAX_TYPES][MAX_CONTROLS] = {
{0xc8,0x48,0xd0,0x50,0xcb,0x4b,0xcd,0x4d,0x38,0xff,0xff,0x4f,0xff,0x51,0xff,0x4a,0xff,0x4e,0xff,0xff,0x10,0x47,0x12,0x49,0x1d,0x9d,0x39,0xff,0x21,0xff,0x1e,0xff,0x2c,0xff,0x30,0xff,0x13,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xf,0xff,0xff,0xff,0xff,0xff},
{0x0,0x1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff},
{0x5,0xc,0xff,0xff,0xff,0xff,0x7,0xf,0x13,0xb,0xff,0x6,0x8,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff},
{0x0,0x1,0xff,0xff,0x2,0xff,0x7,0xf,0x13,0xb,0xff,0xff,0xff,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x3,0xff,0x3,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff},
{0x3,0x0,0x1,0x2,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff},
{0x0,0x1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x1,0x0,0x0,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff},
{0x0,0x1,0xff,0xff,0x2,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0x0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff},
};

kc_item kc_keyboard[NUM_KEY_CONTROLS] = {
	{  0, 15, 49, 71, 26, 43,  2, 23,  1,TNUM_PITCH_FORWARD, BT_KEY, 255 },
	{  1, 15, 49,100, 26, 22,  3,  0, 24,TNUM_PITCH_FORWARD, BT_KEY, 255 },
	{  2, 15, 57, 71, 26,  0,  4, 25,  3,TNUM_PITCH_BACKWARD, BT_KEY, 255 },
	{  3, 15, 57,100, 26,  1,  5,  2, 26,TNUM_PITCH_BACKWARD, BT_KEY, 255 },
	{  4, 15, 65, 71, 26,  2,  6, 27,  5,TNUM_TURN_LEFT, BT_KEY, 255 },
	{  5, 15, 65,100, 26,  3,  7,  4, 28,TNUM_TURN_LEFT, BT_KEY, 255 },
	{  6, 15, 73, 71, 26,  4,  8, 29,  7,TNUM_TURN_RIGHT, BT_KEY, 255 },
	{  7, 15, 73,100, 26,  5,  9,  6, 34,TNUM_TURN_RIGHT, BT_KEY, 255 },
	{  8, 15, 85, 71, 26,  6, 10, 35,  9,TNUM_SLIDE_ON, BT_KEY, 255 },
	{  9, 15, 85,100, 26,  7, 11,  8, 36,TNUM_SLIDE_ON, BT_KEY, 255 },
	{ 10, 15, 93, 71, 26,  8, 12, 37, 11,TNUM_SLIDE_LEFT, BT_KEY, 255 },
	{ 11, 15, 93,100, 26,  9, 13, 10, 44,TNUM_SLIDE_LEFT, BT_KEY, 255 },
	{ 12, 15,101, 71, 26, 10, 14, 45, 13,TNUM_SLIDE_RIGHT, BT_KEY, 255 },
	{ 13, 15,101,100, 26, 11, 15, 12, 30,TNUM_SLIDE_RIGHT, BT_KEY, 255 },
	{ 14, 15,109, 71, 26, 12, 16, 31, 15,TNUM_SLIDE_UP, BT_KEY, 255 },
	{ 15, 15,109,100, 26, 13, 17, 14, 32,TNUM_SLIDE_UP, BT_KEY, 255 },
	{ 16, 15,117, 71, 26, 14, 18, 33, 17,TNUM_SLIDE_DOWN, BT_KEY, 255 },
	{ 17, 15,117,100, 26, 15, 19, 16, 38,TNUM_SLIDE_DOWN, BT_KEY, 255 },
	{ 18, 15,129, 71, 26, 16, 20, 39, 19,TNUM_BANK_ON, BT_KEY, 255 },
	{ 19, 15,129,100, 26, 17, 21, 18, 40,TNUM_BANK_ON, BT_KEY, 255 },
	{ 20, 15,137, 71, 26, 18, 22, 41, 21,TNUM_BANK_LEFT, BT_KEY, 255 },
	{ 21, 15,137,100, 26, 19, 23, 20, 42,TNUM_BANK_LEFT, BT_KEY, 255 },
	{ 22, 15,145, 71, 26, 20,  1, 43, 23,TNUM_BANK_RIGHT, BT_KEY, 255 },
	{ 23, 15,145,100, 26, 21, 24, 22,  0,TNUM_BANK_RIGHT, BT_KEY, 255 },
	{ 24,158, 49, 83, 26, 23, 26,  1, 25,TNUM_FIRE_PRIMARY, BT_KEY, 255 },
	{ 25,158, 49,112, 26, 42, 27, 24,  2,TNUM_FIRE_PRIMARY, BT_KEY, 255 },
	{ 26,158, 57, 83, 26, 24, 28,  3, 27,TNUM_FIRE_SECONDARY, BT_KEY, 255 },
	{ 27,158, 57,112, 26, 25, 29, 26,  4,TNUM_FIRE_SECONDARY, BT_KEY, 255 },
	{ 28,158, 65, 83, 26, 26, 34,  5, 29,TNUM_FIRE_FLARE, BT_KEY, 255 },
	{ 29,158, 65,112, 26, 27, 35, 28,  6,TNUM_FIRE_FLARE, BT_KEY, 255 },
	{ 30,158,105, 83, 26, 44, 32, 13, 31,TNUM_ACCELERATE, BT_KEY, 255 },
	{ 31,158,105,112, 26, 45, 33, 30, 14,TNUM_ACCELERATE, BT_KEY, 255 },
	{ 32,158,113, 83, 26, 30, 38, 15, 33,TNUM_REVERSE, BT_KEY, 255 },
	{ 33,158,113,112, 26, 31, 39, 32, 16,TNUM_REVERSE, BT_KEY, 255 },
	{ 34,158, 73, 83, 26, 28, 36,  7, 35,TNUM_DROP_BOMB, BT_KEY, 255 },
	{ 35,158, 73,112, 26, 29, 37, 34,  8,TNUM_DROP_BOMB, BT_KEY, 255 },
	{ 36,158, 85, 83, 26, 34, 44,  9, 37,TNUM_REAR_VIEW, BT_KEY, 255 },
	{ 37,158, 85,112, 26, 35, 45, 36, 10,TNUM_REAR_VIEW, BT_KEY, 255 },
	{ 38,158,125, 83, 26, 32, 40, 17, 39,TNUM_CRUISE_FASTER, BT_KEY, 255 },
	{ 39,158,125,112, 26, 33, 41, 38, 18,TNUM_CRUISE_FASTER, BT_KEY, 255 },
	{ 40,158,133, 83, 26, 38, 42, 19, 41,TNUM_CRUISE_SLOWER, BT_KEY, 255 },
	{ 41,158,133,112, 26, 39, 43, 40, 20,TNUM_CRUISE_SLOWER, BT_KEY, 255 },
	{ 42,158,141, 83, 26, 40, 25, 21, 43,TNUM_CRUISE_OFF, BT_KEY, 255 },
	{ 43,158,141,112, 26, 41,  0, 42, 22,TNUM_CRUISE_OFF, BT_KEY, 255 },
	{ 44,158, 93, 83, 26, 36, 30, 11, 45,TNUM_AUTOMAP, BT_KEY, 255 },
	{ 45,158, 93,112, 26, 37, 31, 44, 12,TNUM_AUTOMAP, BT_KEY, 255 },
};
kc_item kc_joystick[NUM_OTHER_CONTROLS] = {
	{  0, 25, 46, 85, 26, 15,  1, 24,  5,TNUM_FIRE_PRIMARY, BT_JOY_BUTTON, 255 },
	{  1, 25, 54, 85, 26,  0,  4,  5,  6,TNUM_FIRE_SECONDARY, BT_JOY_BUTTON, 255 },
	{  2, 25, 85, 85, 26, 26,  3,  9, 10,TNUM_ACCELERATE, BT_JOY_BUTTON, 255 },
	{  3, 25, 93, 85, 26,  2, 25, 10, 11,TNUM_REVERSE, BT_JOY_BUTTON, 255 },
	{  4, 25, 62, 85, 26,  1, 26,  6,  7,TNUM_FIRE_FLARE, BT_JOY_BUTTON, 255 },
	{  5,180, 46, 59, 26, 23,  6,  0,  1,TNUM_SLIDE_ON, BT_JOY_BUTTON, 255 },
	{  6,180, 54, 59, 26,  5,  7,  1,  4,TNUM_SLIDE_LEFT, BT_JOY_BUTTON, 255 },
	{  7,180, 62, 59, 26,  6,  8,  4, 26,TNUM_SLIDE_RIGHT, BT_JOY_BUTTON, 255 },
	{  8,180, 70, 59, 26,  7,  9, 26,  9,TNUM_SLIDE_UP, BT_JOY_BUTTON, 255 },
	{  9,180, 78, 59, 26,  8, 10,  8,  2,TNUM_SLIDE_DOWN, BT_JOY_BUTTON, 255 },
	{ 10,180, 90, 59, 26,  9, 11,  2,  3,TNUM_BANK_ON, BT_JOY_BUTTON, 255 },
	{ 11,180, 98, 59, 26, 10, 12,  3, 12,TNUM_BANK_LEFT, BT_JOY_BUTTON, 255 },
	{ 12,180,106, 59, 26, 11, 18, 11, 25,TNUM_BANK_RIGHT, BT_JOY_BUTTON, 255 },
	{ 13, 22,146, 51, 26, 24, 15, 25, 14,TNUM_PITCH_UD, BT_JOY_AXIS, 255 },
	{ 14, 22,146, 99,  8, 25, 16, 13, 17,TNUM_PITCH_UD, BT_INVERT, 255 },
	{ 15, 22,154, 51, 26, 13,  0, 18, 16,TNUM_TURN_LR, BT_JOY_AXIS, 255 },
	{ 16, 22,154, 99,  8, 14, 17, 15, 19,TNUM_TURN_LR, BT_INVERT, 255 },
	{ 17,164,146, 58, 26, 16, 19, 14, 18,TNUM_SLIDE_LR, BT_JOY_AXIS, 255 },
	{ 18,164,146,106,  8, 12, 20, 17, 15,TNUM_SLIDE_LR, BT_INVERT, 255 },
	{ 19,164,154, 58, 26, 17, 21, 16, 20,TNUM_SLIDE_UD, BT_JOY_AXIS, 255 },
	{ 20,164,154,106,  8, 18, 22, 19, 21,TNUM_SLIDE_UD, BT_INVERT, 255 },
	{ 21,164,162, 58, 26, 19, 23, 20, 22,TNUM_BANK_LR, BT_JOY_AXIS, 255 },
	{ 22,164,162,106,  8, 20, 24, 21, 23,TNUM_BANK_LR, BT_INVERT, 255 },
	{ 23,164,174, 58, 26, 21,  5, 22, 24,TNUM_THROTTLE, BT_JOY_AXIS, 255 },
	{ 24,164,174,106,  8, 22, 13, 23,  0,TNUM_THROTTLE, BT_INVERT, 255 },
	{ 25, 25,109, 85, 26,  3, 14, 12, 13,TNUM_REAR_VIEW, BT_JOY_BUTTON, 255 },
	{ 26, 25, 70, 85, 26,  4,  2,  7,  8,TNUM_DROP_BOMB, BT_JOY_BUTTON, 255 },
};
kc_item kc_mouse[NUM_OTHER_CONTROLS] = {
	{  0, 25, 46, 85, 26, 12,  1, 24,  5,TNUM_FIRE_PRIMARY, BT_MOUSE_BUTTON, 255 },
	{  1, 25, 54, 85, 26,  0,  4,  5,  6,TNUM_FIRE_SECONDARY, BT_MOUSE_BUTTON, 255 },
	{  2, 25, 85, 85, 26, 26,  3,  9, 10,TNUM_ACCELERATE, BT_MOUSE_BUTTON, 255 },
	{  3, 25, 93, 85, 26,  2, 25, 10, 11,TNUM_REVERSE, BT_MOUSE_BUTTON, 255 },
	{  4, 25, 62, 85, 26,  1, 26,  6,  7,TNUM_FIRE_FLARE, BT_MOUSE_BUTTON, 255 },
	{  5,180, 46, 59, 26, 24,  6,  0,  1,TNUM_SLIDE_ON, BT_MOUSE_BUTTON, 255 },
	{  6,180, 54, 59, 26,  5,  7,  1,  4,TNUM_SLIDE_LEFT, BT_MOUSE_BUTTON, 255 },
	{  7,180, 62, 59, 26,  6,  8,  4, 26,TNUM_SLIDE_RIGHT, BT_MOUSE_BUTTON, 255 },
	{  8,180, 70, 59, 26,  7,  9, 26,  9,TNUM_SLIDE_UP, BT_MOUSE_BUTTON, 255 },
	{  9,180, 78, 59, 26,  8, 10,  8,  2,TNUM_SLIDE_DOWN, BT_MOUSE_BUTTON, 255 },
	{ 10,180, 90, 59, 26,  9, 11,  2,  3,TNUM_BANK_ON, BT_MOUSE_BUTTON, 255 },
	{ 11,180, 98, 59, 26, 10, 12,  3, 12,TNUM_BANK_LEFT, BT_MOUSE_BUTTON, 255 },
	{ 12,180,106, 59, 26, 11,  0, 11, 25,TNUM_BANK_RIGHT, BT_MOUSE_BUTTON, 255 },
	{ 13,103,138, 58, 26, 25, 15, 25, 14,TNUM_PITCH_UD, BT_MOUSE_AXIS, 255 },
	{ 14,103,138,106,  8, 23, 16, 13, 15,TNUM_PITCH_UD, BT_INVERT, 255 },
	{ 15,103,146, 58, 26, 13, 17, 14, 16,TNUM_TURN_LR, BT_MOUSE_AXIS, 255 },
	{ 16,103,146,106,  8, 14, 18, 15, 17,TNUM_TURN_LR, BT_INVERT, 255 },
	{ 17,103,154, 58, 26, 15, 19, 16, 18,TNUM_SLIDE_LR, BT_MOUSE_AXIS, 255 },
	{ 18,103,154,106,  8, 16, 20, 17, 19,TNUM_SLIDE_LR, BT_INVERT, 255 },
	{ 19,103,162, 58, 26, 17, 21, 18, 20,TNUM_SLIDE_UD, BT_MOUSE_AXIS, 255 },
	{ 20,103,162,106,  8, 18, 22, 19, 21,TNUM_SLIDE_UD, BT_INVERT, 255 },
	{ 21,103,170, 58, 26, 19, 23, 20, 22,TNUM_BANK_LR, BT_MOUSE_AXIS, 255 },
	{ 22,103,170,106,  8, 20, 24, 21, 23,TNUM_BANK_LR, BT_INVERT, 255 },
	{ 23,103,182, 58, 26, 21, 14, 22, 24,TNUM_THROTTLE, BT_MOUSE_AXIS, 255 },
	{ 24,103,182,106,  8, 22,  5, 23,  0,TNUM_THROTTLE, BT_INVERT, 255 },
	{ 25, 25,109, 85, 26,  3, 13, 12, 13,TNUM_REAR_VIEW, BT_MOUSE_BUTTON, 255 },
	{ 26, 25, 70, 85, 26,  4,  2,  7,  8,TNUM_DROP_BOMB, BT_MOUSE_BUTTON, 255 },
};

int kconfig_is_axes_used(int axis)
{
	int i;
	for (i = 0; i < NUM_OTHER_CONTROLS; i++) 
	{
		if ((kc_joystick[i].type == BT_JOY_AXIS) && (kc_joystick[i].value == axis))
			return 1;
	}
	return 0;
}

#ifdef TABLE_CREATION
int find_item_at(kc_item* items, int nitems, int x, int y)
{
	int i;

	for (i = 0; i < nitems; i++) {
		if (((items[i].x + items[i].w1) == x) && (items[i].y == y))
			return i;
	}
	return -1;
}

int find_next_item_up(kc_item* items, int nitems, int citem)
{
	int x, y, i;

	y = items[citem].y;
	x = items[citem].x + items[citem].w1;

	do {
		y--;
		if (y < 0) {
			y = grd_curcanv->cv_bitmap.bm_h - 1;
			x--;
			if (x < 0) {
				x = grd_curcanv->cv_bitmap.bm_w - 1;
			}
		}
		i = find_item_at(items, nitems, x, y);
	} while (i < 0);

	return i;
}

int find_next_item_down(kc_item* items, int nitems, int citem)
{
	int x, y, i;

	y = items[citem].y;
	x = items[citem].x + items[citem].w1;

	do {
		y++;
		if (y > grd_curcanv->cv_bitmap.bm_h - 1) {
			y = 0;
			x++;
			if (x > grd_curcanv->cv_bitmap.bm_w - 1) {
				x = 0;
			}
		}
		i = find_item_at(items, nitems, x, y);
	} while (i < 0);

	return i;
}

int find_next_item_right(kc_item* items, int nitems, int citem)
{
	int x, y, i;

	y = items[citem].y;
	x = items[citem].x + items[citem].w1;

	do {
		x++;
		if (x > grd_curcanv->cv_bitmap.bm_w - 1) {
			x = 0;
			y++;
			if (y > grd_curcanv->cv_bitmap.bm_h - 1) {
				y = 0;
			}
		}
		i = find_item_at(items, nitems, x, y);
	} while (i < 0);

	return i;
}

int find_next_item_left(kc_item* items, int nitems, int citem)
{
	int x, y, i;

	y = items[citem].y;
	x = items[citem].x + items[citem].w1;

	do {
		x--;
		if (x < 0) {
			x = grd_curcanv->cv_bitmap.bm_w - 1;
			y--;
			if (y < 0) {
				y = grd_curcanv->cv_bitmap.bm_h - 1;
			}
		}
		i = find_item_at(items, nitems, x, y);
	} while (i < 0);

	return i;
}
#endif



void kconfig_sub(kc_item* items, int nitems, char* title)
{
	grs_canvas* save_canvas;
	grs_font* save_font;
	int old_keyd_repeat;

	int i, k, ocitem, citem;
	int time_stopped = 0;


	All_items = items;
	Num_items = nitems;

	if (!((Game_mode & GM_MULTI) && (Function_mode == FMODE_GAME) && (!Endlevel_sequence)))
	{
		time_stopped = 1;
		stop_time();
	}

	save_canvas = grd_curcanv;
	gr_set_current_canvas(NULL);
	save_font = grd_curcanv->cv_font;
	game_flush_inputs();
	old_keyd_repeat = keyd_repeat;
	keyd_repeat = 1;

	//gr_clear_canvas( BM_XRGB(0,0,0) );

	nm_draw_background(0, 0, grd_curcanv->cv_bitmap.bm_w, grd_curcanv->cv_bitmap.bm_h);

	grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_3];

	{
		char* p;
		p = strchr(title, '\n');
		if (p)* p = 32;
		gr_string(0x8000, 8, title);
		if (p)* p = '\n';
	}


	//	if ( items == kc_keyboard )	{
	//		gr_string( 0x8000, 8, "Keyboard" );
	//	} else if ( items == kc_joystick )	{
	//		gr_string( 0x8000, 8, "Joysticks" );
	//	} else if ( items == kc_mouse )	{
	//		gr_string( 0x8000, 8, "Mouse" );
	//	}

	grd_curcanv->cv_font = GAME_FONT;
	gr_set_fontcolor(BM_XRGB(28, 28, 28), -1);

	gr_string(0x8000, 20, TXT_KCONFIG_STRING_1);
	gr_set_fontcolor(BM_XRGB(28, 28, 28), -1);
	if (items == kc_keyboard) 
	{
		gr_set_fontcolor(BM_XRGB(31, 27, 6), -1);
		gr_setcolor(BM_XRGB(31, 27, 6));

		gr_scanline(98, 106, 42);
		gr_scanline(120, 128, 42);
		gr_pixel(98, 43);
		gr_pixel(98, 44);
		gr_pixel(128, 43);
		gr_pixel(128, 44);

		gr_string(109, 40, "OR");

		gr_scanline(253, 261, 42);
		gr_scanline(274, 283, 42);
		gr_pixel(253, 43);
		gr_pixel(253, 44);
		gr_pixel(283, 43);
		gr_pixel(283, 44);

		gr_string(264, 40, "OR");

	} 
	if (items == kc_joystick) 
	{
		gr_set_fontcolor(BM_XRGB(31, 27, 6), -1);
		gr_setcolor(BM_XRGB(31, 27, 6));
		gr_scanline(18, 135, 37);
		gr_scanline(181, 294, 37);
		gr_scanline(18, 144, 119 + 10);
		gr_scanline(174, 294, 119 + 10);
		gr_string(0x8000, 35, TXT_BUTTONS);
		gr_string(0x8000, 117 + 10, TXT_AXES);
		gr_set_fontcolor(BM_XRGB(28, 28, 28), -1);
		gr_string(81, 137, TXT_AXIS);
		gr_string(111, 137, TXT_INVERT);
		gr_string(222, 137, TXT_AXIS);
		gr_string(252, 137, TXT_INVERT);
	}
	else if (items == kc_mouse) 
	{
		gr_set_fontcolor(BM_XRGB(31, 27, 6), -1);
		gr_setcolor(BM_XRGB(31, 27, 6));
		gr_scanline(18, 135, 37);
		gr_scanline(181, 294, 37);
		gr_scanline(18, 144, 119 + 5);
		gr_scanline(174, 294, 119 + 5);
		gr_string(0x8000, 35, TXT_BUTTONS);
		gr_string(0x8000, 117 + 5, TXT_AXES);
		gr_set_fontcolor(BM_XRGB(28, 28, 28), -1);
		gr_string(169, 129, TXT_AXIS);
		gr_string(199, 129, TXT_INVERT);
	}

	for (i = 0; i < nitems; i++) 
	{
		kc_drawitem(&items[i], 0);
	}

	citem = 0;
	kc_drawitem(&items[citem], 1);

	while (1) 
	{
		I_MarkStart();
		I_DoEvents();
		k = key_inkey();
		if (!time_stopped)
		{
#ifdef NETWORK
			if (multi_menu_poll() == -1)
				k = -2;
#endif
		}
		ocitem = citem;
		switch (k) 
		{
		case KEY_BACKSP:
			Int3();
			break;
		case KEY_PRINT_SCREEN:
			save_screen_shot(0);
			break;
		case KEY_CTRLED + KEY_D:
			items[citem].value = 255;
			kc_drawitem(&items[citem], 1);
			break;
		case KEY_CTRLED + KEY_R:
			if (items == kc_keyboard)
			{
				for (i = 0; i < NUM_KEY_CONTROLS; i++) 
				{
					items[i].value = default_kconfig_settings[0][i];
					kc_drawitem(&items[i], 0);
				}
			}
			else {
				for (i = 0; i < NUM_OTHER_CONTROLS; i++) 
				{
					items[i].value = default_kconfig_settings[Config_control_type][i];
					kc_drawitem(&items[i], 0);
				}
			}
			kc_drawitem(&items[citem], 1);
			break;
		case KEY_UP:
		case KEY_PAD8:
#ifdef TABLE_CREATION
			if (items[citem].u == -1) items[citem].u = find_next_item_up(items, nitems, citem);
#endif
			citem = items[citem].u;
			break;
		case KEY_DOWN:
		case KEY_PAD2:
#ifdef TABLE_CREATION
			if (items[citem].d == -1) items[citem].d = find_next_item_down(items, nitems, citem);
#endif
			citem = items[citem].d;
			break;
		case KEY_LEFT:
		case KEY_PAD4:
#ifdef TABLE_CREATION
			if (items[citem].l == -1) items[citem].l = find_next_item_left(items, nitems, citem);
#endif
			citem = items[citem].l;
			break;
		case KEY_RIGHT:
		case KEY_PAD6:
#ifdef TABLE_CREATION
			if (items[citem].r == -1) items[citem].r = find_next_item_right(items, nitems, citem);
#endif
			citem = items[citem].r;
			break;
		case KEY_ENTER:
		case KEY_PADENTER:
			switch (items[citem].type) 
			{
			case BT_KEY:				kc_change_key(&items[citem]); break;
			case BT_MOUSE_BUTTON:	kc_change_mousebutton(&items[citem]); break;
			case BT_MOUSE_AXIS: 		kc_change_mouseaxis(&items[citem]); break;
			case BT_JOY_BUTTON: 		kc_change_joybutton(&items[citem]); break;
			case BT_JOY_AXIS: 		kc_change_joyaxis(&items[citem]); break;
			case BT_INVERT: 			kc_change_invert(&items[citem]); break;
			}
			break;
		case -2:
		case KEY_ESC:
			grd_curcanv->cv_font = save_font;
			gr_set_current_canvas(save_canvas);
			keyd_repeat = old_keyd_repeat;
			game_flush_inputs();
			if (time_stopped)
				start_time();
			return;
#ifdef TABLE_CREATION
		case KEY_DEBUGGED + KEY_F12: {
			FILE* fp;
			for (i = 0; i < NUM_KEY_CONTROLS; i++) {
				kc_keyboard[i].u = find_next_item_up(kc_keyboard, NUM_KEY_CONTROLS, i);
				kc_keyboard[i].d = find_next_item_down(kc_keyboard, NUM_KEY_CONTROLS, i);
				kc_keyboard[i].l = find_next_item_left(kc_keyboard, NUM_KEY_CONTROLS, i);
				kc_keyboard[i].r = find_next_item_right(kc_keyboard, NUM_KEY_CONTROLS, i);
			}
			for (i = 0; i < NUM_OTHER_CONTROLS; i++) {
				kc_joystick[i].u = find_next_item_up(kc_joystick, NUM_OTHER_CONTROLS, i);
				kc_joystick[i].d = find_next_item_down(kc_joystick, NUM_OTHER_CONTROLS, i);
				kc_joystick[i].l = find_next_item_left(kc_joystick, NUM_OTHER_CONTROLS, i);
				kc_joystick[i].r = find_next_item_right(kc_joystick, NUM_OTHER_CONTROLS, i);
			}
			for (i = 0; i < NUM_OTHER_CONTROLS; i++) {
				kc_mouse[i].u = find_next_item_up(kc_mouse, NUM_OTHER_CONTROLS, i);
				kc_mouse[i].d = find_next_item_down(kc_mouse, NUM_OTHER_CONTROLS, i);
				kc_mouse[i].l = find_next_item_left(kc_mouse, NUM_OTHER_CONTROLS, i);
				kc_mouse[i].r = find_next_item_right(kc_mouse, NUM_OTHER_CONTROLS, i);
			}
			fp = fopen("kconfig.cod", "wt");

			fprintf(fp, "ubyte default_kconfig_settings[CONTROL_MAX_TYPES][MAX_CONTROLS] = {\n");
			for (i = 0; i < CONTROL_MAX_TYPES; i++) {
				int j;
				fprintf(fp, "{0x%x", kconfig_settings[i][0]);
				for (j = 1; j < MAX_CONTROLS; j++)
					fprintf(fp, ",0x%x", kconfig_settings[i][j]);
				fprintf(fp, "},\n");
			}
			fprintf(fp, "};\n");

			fprintf(fp, "\nkc_item kc_keyboard[NUM_KEY_CONTROLS] = {\n");
			for (i = 0; i < NUM_KEY_CONTROLS; i++) {
				fprintf(fp, "\t{ %2d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%c%s%c, %s, 255 },\n",
					kc_keyboard[i].id, kc_keyboard[i].x, kc_keyboard[i].y, kc_keyboard[i].w1, kc_keyboard[i].w2,
					kc_keyboard[i].u, kc_keyboard[i].d, kc_keyboard[i].l, kc_keyboard[i].r,
					34, Text_string[kc_keyboard[i].text_num1], 34, btype_text[kc_keyboard[i].type]);
			}
			fprintf(fp, "};");

			fprintf(fp, "\nkc_item kc_joystick[NUM_OTHER_CONTROLS] = {\n");
			for (i = 0; i < NUM_OTHER_CONTROLS; i++) {
				fprintf(fp, "\t{ %2d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%c%s%c, %s, 255 },\n",
					kc_joystick[i].id, kc_joystick[i].x, kc_joystick[i].y, kc_joystick[i].w1, kc_joystick[i].w2,
					kc_joystick[i].u, kc_joystick[i].d, kc_joystick[i].l, kc_joystick[i].r,
					34, Text_string[kc_joystick[i].text_num1], 34, btype_text[kc_joystick[i].type]);
			}
			fprintf(fp, "};");

			fprintf(fp, "\nkc_item kc_mouse[NUM_OTHER_CONTROLS] = {\n");
			for (i = 0; i < NUM_OTHER_CONTROLS; i++) {
				fprintf(fp, "\t{ %2d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%3d,%c%s%c, %s, 255 },\n",
					kc_mouse[i].id, kc_mouse[i].x, kc_mouse[i].y, kc_mouse[i].w1, kc_mouse[i].w2,
					kc_mouse[i].u, kc_mouse[i].d, kc_mouse[i].l, kc_mouse[i].r,
					34, Text_string[kc_mouse[i].text_num1], 34, btype_text[kc_mouse[i].type]);
			}
			fprintf(fp, "};");

			fclose(fp);

		}
									 break;
#endif
		}
		if (ocitem != citem) 
		{
			kc_drawitem(&items[ocitem], 0);
			kc_drawitem(&items[citem], 1);
		}
		I_DrawCurrentCanvas(0);
		I_MarkEnd(US_70FPS);
	}
}


void kc_drawitem(kc_item* item, int is_current)
{
	int x, w, h, aw;
	char btext[10];

	if (is_current)
		gr_set_fontcolor(BM_XRGB(20, 20, 29), -1);
	else
		gr_set_fontcolor(BM_XRGB(15, 15, 24), -1);
	gr_string(item->x, item->y, Text_string[item->text_num1]);

	if (item->value == 255) 
	{
		sprintf(btext, "");
	}
	else 
	{
		switch (item->type) 
		{
		case BT_KEY:
			strncpy(btext, key_text[item->value], 10); break;
		case BT_MOUSE_BUTTON:
			strncpy(btext, Text_string[mousebutton_text[item->value]], 10); break;
		case BT_MOUSE_AXIS:
			strncpy(btext, Text_string[mouseaxis_text[item->value]], 10); break;
		case BT_JOY_BUTTON:
			if (Config_control_type == CONTROL_FLIGHTSTICK_PRO)
			{
				strncpy(btext, choco_gamepad_text[item->value], 10);
			}
			else
			{
				/*if (joybutton_text[item->value] != -1)
					strncpy(btext, Text_string[joybutton_text[item->value]], 10);
				else
					sprintf(btext, "BTN%d", item->value);*/
				strncpy(btext, choco_joybutton_text[item->value], 10);
			}
			break;
		case BT_JOY_AXIS:
			strncpy(btext, Text_string[joyaxis_text[item->value]], 10); break;
		case BT_INVERT:
			strncpy(btext, Text_string[invert_text[item->value]], 10); break;
		}
	}
	gr_get_string_size(btext, &w, &h, &aw);

	if (is_current)
		gr_setcolor(BM_XRGB(21, 0, 24));
	else
		gr_setcolor(BM_XRGB(16, 0, 19));
	gr_urect(item->w1 + item->x, item->y - 1, item->w1 + item->x + item->w2, item->y + h);

	gr_set_fontcolor(BM_XRGB(28, 28, 28), -1);

	x = item->w1 + item->x + ((item->w2 - w) / 2);

	gr_string(x, item->y, btext);
}


static int looper = 0;

void kc_drawquestion(kc_item* item)
{
	int c, x, w, h, aw;

	gr_get_string_size("?", &w, &h, &aw);

	c = BM_XRGB(21, 0, 24);

	gr_setcolor(gr_fade_table[fades[looper] * 256 + c]);
	looper++;
	if (looper > 63) looper = 0;

	gr_urect(item->w1 + item->x, item->y - 1, item->w1 + item->x + item->w2, item->y + h);

	gr_set_fontcolor(BM_XRGB(28, 28, 28), -1);

	x = item->w1 + item->x + ((item->w2 - w) / 2);

	gr_string(x, item->y, "?");
}

void kc_change_key(kc_item* item)
{
	int i, n, f, k;
	uint8_t keycode;

	gr_set_fontcolor(BM_XRGB(28, 28, 28), -1);

	gr_string(0x8000, INFO_Y, TXT_PRESS_NEW_KEY);

	game_flush_inputs();
	keycode = 255;
	k = 255;
	while ((k != KEY_ESC) && (keycode == 255)) 
	{
		I_DrawCurrentCanvas(0);
		I_DoEvents();
#ifdef NETWORK
		if ((Game_mode & GM_MULTI) && (Function_mode == FMODE_GAME) && (!Endlevel_sequence))
			multi_menu_poll();
#endif
		//		if ( Game_mode & GM_MULTI )
		//			GameLoop( 0, 0 );				// Continue
		k = key_inkey();
		I_Delay(10); //[ISB] TODO find something here
		kc_drawquestion(item);

		for (i = 0; i < 256; i++) 
		{
			if (keyd_pressed[i] && (strlen(key_text[i]) > 0)) 
			{
				f = 0;
				for (n = 0; n < sizeof(system_keys); n++)
					if (system_keys[n] == i)
						f = 1;
				if (!f)
					keycode = i;
			}
		}
	}

	if (k != KEY_ESC) 
	{
		for (i = 0; i < Num_items; i++) 
		{
			n = item - All_items;
			if ((i != n) && (All_items[i].type == BT_KEY) && (All_items[i].value == keycode)) 
			{
				All_items[i].value = 255;
				kc_drawitem(&All_items[i], 0);
			}
		}
		item->value = keycode;
	}
	kc_drawitem(item, 1);
	gr_set_fontcolor(BM_XRGB(28, 28, 28), BM_XRGB(0, 0, 0));

	nm_restore_background(0, INFO_Y, 310, grd_curcanv->cv_font->ft_h);

	game_flush_inputs();

}

void kc_change_joybutton(kc_item* item)
{
	int n, i, k;
	uint8_t code;

	gr_set_fontcolor(BM_XRGB(28, 28, 28), -1);

	gr_string(0x8000, INFO_Y, TXT_PRESS_NEW_JBUTTON);

	game_flush_inputs();
	code = 255;
	k = 255;
	while ((k != KEY_ESC) && (code == 255)) 
	{
		I_DrawCurrentCanvas(0);
		I_DoEvents();
#ifdef NETWORK
		if ((Game_mode & GM_MULTI) && (Function_mode == FMODE_GAME) && (!Endlevel_sequence))
			multi_menu_poll();
#endif
		//		if ( Game_mode & GM_MULTI )
		//			GameLoop( 0, 0 );				// Continue
		k = key_inkey();
		I_Delay(10);

		if (k == KEY_PRINT_SCREEN)
			save_screen_shot(0);

		kc_drawquestion(item);

		if (Config_control_type == CONTROL_THRUSTMASTER_FCS) 
		{
			int axis[4];
			joystick_read_raw_axis(JOY_ALL_AXIS, axis);
			kconfig_read_fcs(axis[3]);
			if (joy_get_button_state(7)) code = 7;
			if (joy_get_button_state(11)) code = 11;
			if (joy_get_button_state(15)) code = 15;
			if (joy_get_button_state(19)) code = 19;
			//for (i = 0; i < 4; i++) 
			for (i = 4; i < 20; i++) //Use Flightstick button assignments
			{
				if (joy_get_button_state(i))
					code = i;
			}
		}
		else if (Config_control_type == CONTROL_FLIGHTSTICK_PRO) 
		{
			for (i = 4; i < 20; i++)
			{
				if (joy_get_button_state(i)) 
				{
					code = i;
					mprintf((0, "JB: %d\n", code));
				}
			}
		}
		else 
		{
			for (i = 0; i < 4; i++)
			{
				if (joy_get_button_state(i))
					code = i;
			}
		}
	}
	if (code != 255) 
	{
		for (i = 0; i < Num_items; i++) 
		{
			n = item - All_items;
			if ((i != n) && (All_items[i].type == BT_JOY_BUTTON) && (All_items[i].value == code)) 
			{
				All_items[i].value = 255;
				kc_drawitem(&All_items[i], 0);
			}
		}
		item->value = code;
	}
	kc_drawitem(item, 1);
	nm_restore_background(0, INFO_Y, 310, grd_curcanv->cv_font->ft_h);
	game_flush_inputs();
}

void kc_change_mousebutton(kc_item* item)
{
	int n, i, b, k;
	uint8_t code;

	gr_set_fontcolor(BM_XRGB(28, 28, 28), -1);

	gr_string(0x8000, INFO_Y, TXT_PRESS_NEW_MBUTTON);

	game_flush_inputs();
	code = 255;
	k = 255;
	while ((k != KEY_ESC) && (code == 255)) 
	{
		I_DrawCurrentCanvas(0);
		I_DoEvents();
#ifdef NETWORK
		if ((Game_mode & GM_MULTI) && (Function_mode == FMODE_GAME) && (!Endlevel_sequence))
			multi_menu_poll();
#endif
		//		if ( Game_mode & GM_MULTI )
		//			GameLoop( 0, 0 );				// Continue
		k = key_inkey();
		I_Delay(10);

		if (k == KEY_PRINT_SCREEN)
			save_screen_shot(0);

		kc_drawquestion(item);

		b = mouse_get_btns();
		for (i = 0; i < 3; i++)
		{
			if (b & (1 << i))
				code = i;
		}
	}
	if (code != 255) 
	{
		for (i = 0; i < Num_items; i++) 
		{
			n = item - All_items;
			if ((i != n) && (All_items[i].type == BT_MOUSE_BUTTON) && (All_items[i].value == code))
			{
				All_items[i].value = 255;
				kc_drawitem(&All_items[i], 0);
			}
		}
		item->value = code;
	}
	kc_drawitem(item, 1);
	nm_restore_background(0, INFO_Y, 310, grd_curcanv->cv_font->ft_h);
	game_flush_inputs();

}

void kc_change_joyaxis(kc_item* item)
{
	int axis[4];
	int old_axis[4];
	int n, i, k;
	uint8_t code;

	gr_set_fontcolor(BM_XRGB(28, 28, 28), -1);

	gr_string(0x8000, INFO_Y, TXT_MOVE_NEW_JOY_AXIS);

	game_flush_inputs();
	code = 255;
	k = 255;

	joystick_read_raw_axis(JOY_ALL_AXIS, old_axis);

	while ((k != KEY_ESC) && (code == 255)) 
	{
		I_DrawCurrentCanvas(0);
		I_DoEvents();
#ifdef NETWORK
		if ((Game_mode & GM_MULTI) && (Function_mode == FMODE_GAME) && (!Endlevel_sequence))
			multi_menu_poll();
#endif
		k = key_inkey();
		I_Delay(10);

		if (k == KEY_PRINT_SCREEN)
			save_screen_shot(0);

		kc_drawquestion(item);

		joystick_read_raw_axis(JOY_ALL_AXIS, axis);

		for (i = 0; i < 4; i++)
		{
			if (abs(axis[i] - old_axis[i]) > 20)
			{
				code = i;
			}
			old_axis[i] = axis[i];
		}
	}
	if (code != 255) 
	{
		for (i = 0; i < Num_items; i++) 
		{
			n = item - All_items;
			if ((i != n) && (All_items[i].type == BT_JOY_AXIS) && (All_items[i].value == code))
			{
				All_items[i].value = 255;
				kc_drawitem(&All_items[i], 0);
			}
		}

		item->value = code;
	}
	kc_drawitem(item, 1);
	nm_restore_background(0, INFO_Y, 310, grd_curcanv->cv_font->ft_h);
	game_flush_inputs();
}

void kc_change_mouseaxis(kc_item* item)
{
	int i, n, k;
	uint8_t code;
	int dx, dy;

	gr_set_fontcolor(BM_XRGB(28, 28, 28), -1);

	gr_string(0x8000, INFO_Y, TXT_MOVE_NEW_MSE_AXIS);

	game_flush_inputs();
	code = 255;
	k = 255;

	mouse_get_delta(&dx, &dy);

	while ((k != KEY_ESC) && (code == 255)) 
	{
		I_DrawCurrentCanvas(0);
		I_DoEvents();
#ifdef NETWORK
		if ((Game_mode & GM_MULTI) && (Function_mode == FMODE_GAME) && (!Endlevel_sequence))
			multi_menu_poll();
#endif
		//		if ( Game_mode & GM_MULTI )
		//			GameLoop( 0, 0 );				// Continue
		k = key_inkey();
		I_Delay(10);

		if (k == KEY_PRINT_SCREEN)
			save_screen_shot(0);

		kc_drawquestion(item);

		mouse_get_delta(&dx, &dy);
		if (abs(dx) > 20) code = 0;
		if (abs(dy) > 20)	code = 1;
	}
	if (code != 255) 
	{
		for (i = 0; i < Num_items; i++) 
		{
			n = item - All_items;
			if ((i != n) && (All_items[i].type == BT_MOUSE_AXIS) && (All_items[i].value == code)) 
			{
				All_items[i].value = 255;
				kc_drawitem(&All_items[i], 0);
			}
		}
		item->value = code;
	}
	kc_drawitem(item, 1);
	nm_restore_background(0, INFO_Y, 310, grd_curcanv->cv_font->ft_h);
	game_flush_inputs();
}


void kc_change_invert(kc_item* item)
{
	game_flush_inputs();

	if (item->value)
		item->value = 0;
	else
		item->value = 1;

	kc_drawitem(item, 1);
}

#include "screens.h"

void kconfig(int n, char* title)
{
	int i;
	set_screen_mode(SCREEN_MENU);

	kc_set_controls();

	switch (n) 
	{
	case 0:kconfig_sub(kc_keyboard, NUM_KEY_CONTROLS, title); break;
	case 1:kconfig_sub(kc_joystick, NUM_OTHER_CONTROLS, title); break;
	case 2:kconfig_sub(kc_mouse, NUM_OTHER_CONTROLS, title); break;
	default:
		Int3();
		return;
	}

	reset_cockpit();		//force cockpit redraw next time

	// Update save values...

	for (i = 0; i < NUM_KEY_CONTROLS; i++)
		kconfig_settings[0][i] = kc_keyboard[i].value;

	if ((Config_control_type > 0) && (Config_control_type < 5)) 
	{
		for (i = 0; i < NUM_OTHER_CONTROLS; i++)
			kconfig_settings[Config_control_type][i] = kc_joystick[i].value;
	}
	else if (Config_control_type > 4) 
	{
		for (i = 0; i < NUM_OTHER_CONTROLS; i++)
			kconfig_settings[Config_control_type][i] = kc_mouse[i].value;
	}

}


void kconfig_read_fcs(int raw_axis)
{
	int raw_button, button, axis_min[4], axis_center[4], axis_max[4];

	if (Config_control_type != CONTROL_THRUSTMASTER_FCS) return;

	joy_get_cal_vals(axis_min, axis_center, axis_max);

	if (axis_max[3] > 1)
		raw_button = (raw_axis * 100) / axis_max[3];
	else
		raw_button = 0;

	if (raw_button > 88)
		button = 0;
	else if (raw_button > 63)
		button = 7;
	else if (raw_button > 39)
		button = 11;
	else if (raw_button > 15)
		button = 15;
	else
		button = 19;

	kconfig_set_fcs_button(19, button);
	kconfig_set_fcs_button(15, button);
	kconfig_set_fcs_button(11, button);
	kconfig_set_fcs_button(7, button);
}


void kconfig_set_fcs_button(int btn, int button)
{
	int state, time_down, upcount, downcount;
	state = time_down = upcount = downcount = 0;

	if (joy_get_button_state(btn)) 
	{
		if (btn == button)
		{
			state = 1;
			time_down = FrameTime;
		}
		else 
		{
			upcount = 1;
		}
	}
	else 
	{
		if (btn == button) 
		{
			state = 1;
			time_down = FrameTime;
			downcount = 1;
		}
		else 
		{
			upcount = 1;
		}
	}
	joy_set_btn_values(btn, state, time_down, downcount, upcount);
}

fix Last_angles_p = 0;
fix Last_angles_b = 0;
fix Last_angles_h = 0;
uint8_t Last_angles_read = 0;

extern int			VR_sensitivity;

int VR_sense_range[3] = { 25, 50, 75 };

void read_head_tracker()
{
	//Warning("read_head_tracker: STUB\n");
}

#define	PH_SCALE	8
#define	JOYSTICK_READ_TIME	(F1_0/10)		//	Read joystick at 10 Hz.
fix	LastReadTime = 0;

fix	joy_axis[4];

uint8_t 			kc_use_external_control = 0;
uint8_t				kc_enable_external_control = 1;
uint8_t 			kc_external_intno = 0;
control_info* kc_external_control = NULL;
uint8_t* kc_external_name = NULL;
uint8_t				kc_external_version = 0;

void kconfig_init_external_controls(int intno, int address)
{
	Warning("kconfig_init_external_controls: STUB\n");
}

void kconfig_read_external_controls()
{
	//Warning("kconfig_read_external_controls: STUB\n");
}

void controls_read_all()
{
	int i;
	int slide_on, bank_on;
	int dx, dy;
	int idx, idy;
	fix ctime;
	fix mouse_axis[2];
	int raw_joy_axis[4];
	int mouse_buttons;
	fix k0, k1, k2, k3, kp;
	fix k4, k5, k6, k7, kh;
	uint8_t channel_masks;
	int use_mouse, use_joystick;
	int speed_factor = 1;

	if (Game_turbo_mode)
		speed_factor = 2;

	if (Arcade_mode) {
		arcade_read_controls();
		return;
	}

	{
		fix temp = Controls.heading_time;
		fix temp1 = Controls.pitch_time;
		memset(&Controls, 0, sizeof(control_info));
		Controls.heading_time = temp;
		Controls.pitch_time = temp1;
	}
	slide_on = 0;
	bank_on = 0;

	ctime = timer_get_fixed_seconds();

	//---------  Read Joystick -----------
	if ((LastReadTime + JOYSTICK_READ_TIME > ctime) && (Config_control_type != CONTROL_THRUSTMASTER_FCS)) {
		if ((ctime < 0) && (LastReadTime > 0))
			LastReadTime = ctime;
		use_joystick = 1;
	}
	else if ((Config_control_type > 0) && (Config_control_type < 5)) {
		LastReadTime = ctime;
		channel_masks = joystick_read_raw_axis(JOY_ALL_AXIS, raw_joy_axis);

		for (i = 0; i < 4; i++) {
			if (channel_masks & (1 << i)) {
				int joy_null_value = 10;

				if ((i == 3) && (Config_control_type == CONTROL_THRUSTMASTER_FCS)) {
					kconfig_read_fcs(raw_joy_axis[i]);
				}
				else {
					raw_joy_axis[i] = joy_get_scaled_reading(raw_joy_axis[i], i);

					if (kc_joystick[23].value == i)		// If this is the throttle
						joy_null_value = 20;				// Then use a larger dead-zone

					if (raw_joy_axis[i] > joy_null_value)
						raw_joy_axis[i] = ((raw_joy_axis[i] - joy_null_value) * 128) / (128 - joy_null_value);
					else if (raw_joy_axis[i] < -joy_null_value)
						raw_joy_axis[i] = ((raw_joy_axis[i] + joy_null_value) * 128) / (128 - joy_null_value);
					else
						raw_joy_axis[i] = 0;
					joy_axis[i] = (raw_joy_axis[i] * FrameTime) / 128;
				}
			}
			else {
				joy_axis[i] = 0;
			}
		}
		use_joystick = 1;
	}
	else {
		for (i = 0; i < 4; i++)
			joy_axis[i] = 0;
		use_joystick = 0;
	}

	if (Config_control_type == 5) {
		//---------  Read Mouse -----------
		mouse_get_delta(&dx, &dy);
		mouse_axis[0] = (dx * FrameTime) / 35;
		mouse_axis[1] = (dy * FrameTime) / 25;
		mouse_buttons = mouse_get_btns();
		//mprintf(( 0, "Mouse %d,%d b:%d, 0x%x\n", mouse_axis[0], mouse_axis[1], mouse_buttons, FrameTime ));
		use_mouse = 1;
	}
	else if (Config_control_type == 6) {
		//---------  Read Cyberman -----------
		mouse_get_cyberman_pos(&idx, &idy);
		mouse_axis[0] = (idx * FrameTime) / 128;
		mouse_axis[1] = (idy * FrameTime) / 128;
		mouse_buttons = mouse_get_btns();
		use_mouse = 1;
	}
	else {
		mouse_axis[0] = 0;
		mouse_axis[1] = 0;
		mouse_buttons = 0;
		use_mouse = 0;
	}


	//------------- Read slide_on -------------

		// From keyboard...
	if (kc_keyboard[8].value < 255) slide_on |= keyd_pressed[kc_keyboard[8].value];
	if (kc_keyboard[9].value < 255) slide_on |= keyd_pressed[kc_keyboard[9].value];
	// From joystick...
	if ((use_joystick) && (kc_joystick[5].value < 255)) slide_on |= joy_get_button_state(kc_joystick[5].value);
	// From mouse...
	if ((use_mouse) && (kc_mouse[5].value < 255)) slide_on |= mouse_buttons & (1 << kc_mouse[5].value);

	//------------- Read bank_on ---------------

		// From keyboard...
	if (kc_keyboard[18].value < 255) bank_on |= keyd_pressed[kc_keyboard[18].value];
	if (kc_keyboard[19].value < 255) bank_on |= keyd_pressed[kc_keyboard[19].value];
	// From joystick...
	if ((use_joystick) && (kc_joystick[10].value < 255)) bank_on |= joy_get_button_state(kc_joystick[10].value);
	// From mouse...
	if ((use_mouse) && (kc_mouse[10].value < 255)) bank_on |= mouse_buttons & (1 << kc_mouse[10].value);

	//------------ Read pitch_time -----------
	if (!slide_on) {
		// mprintf((0, "pitch: %7.3f %7.3f: %7.3f\n", f2fl(k4), f2fl(k6), f2fl(Controls.heading_time)));
		kp = 0;
		k0 = speed_factor * key_down_time(kc_keyboard[0].value) / 2;	// Divide by two since we want pitch to go slower
		k1 = speed_factor * key_down_time(kc_keyboard[1].value) / 2;
		k2 = speed_factor * key_down_time(kc_keyboard[2].value) / 2;
		k3 = speed_factor * key_down_time(kc_keyboard[3].value) / 2;

		// From keyboard...
		if (kc_keyboard[0].value < 255) kp += k0 / PH_SCALE;
		if (kc_keyboard[1].value < 255) kp += k1 / PH_SCALE;
		if (kc_keyboard[2].value < 255) kp -= k2 / PH_SCALE;
		if (kc_keyboard[3].value < 255) kp -= k3 / PH_SCALE;

		// From Cyberman...
		if ((use_mouse) && (Config_control_type == CONTROL_CYBERMAN)) {
			kp += mouse_button_down_time(MBUTTON_PITCH_FORWARD) / (PH_SCALE * 2);
			kp -= mouse_button_down_time(MBUTTON_PITCH_BACKWARD) / (PH_SCALE * 2);
		}

		if (kp == 0)
			Controls.pitch_time = 0;
		else if (kp > 0) {
			if (Controls.pitch_time < 0)
				Controls.pitch_time = 0;
		}
		else // kp < 0
			if (Controls.pitch_time > 0)
				Controls.pitch_time = 0;
		Controls.pitch_time += kp;

		// From joystick...
		if ((use_joystick) && (kc_joystick[13].value < 255)) {
			if (!kc_joystick[14].value)		// If not inverted...
				Controls.pitch_time -= (joy_axis[kc_joystick[13].value] * Config_joystick_sensitivity) / 8;
			else
				Controls.pitch_time += (joy_axis[kc_joystick[13].value] * Config_joystick_sensitivity) / 8;
		}

		// From mouse...
		//mprintf(( 0, "UM: %d, PV: %d\n", use_mouse, kc_mouse[13].value ));
		if ((use_mouse) && (kc_mouse[13].value < 255)) {
			if (!kc_mouse[14].value)		// If not inverted...
				Controls.pitch_time -= (mouse_axis[kc_mouse[13].value] * Config_joystick_sensitivity) / 8;
			else
				Controls.pitch_time += (mouse_axis[kc_mouse[13].value] * Config_joystick_sensitivity) / 8;
		}
	}
	else {
		Controls.pitch_time = 0;
	}


	//----------- Read vertical_thrust_time -----------------

	if (slide_on) {
		k0 = speed_factor * key_down_time(kc_keyboard[0].value);
		k1 = speed_factor * key_down_time(kc_keyboard[1].value);
		k2 = speed_factor * key_down_time(kc_keyboard[2].value);
		k3 = speed_factor * key_down_time(kc_keyboard[3].value);

		// From keyboard...
		if (kc_keyboard[0].value < 255) Controls.vertical_thrust_time += k0;
		if (kc_keyboard[1].value < 255) Controls.vertical_thrust_time += k1;
		if (kc_keyboard[2].value < 255) Controls.vertical_thrust_time -= k2;
		if (kc_keyboard[3].value < 255) Controls.vertical_thrust_time -= k3;

		// From Cyberman...
		if ((use_mouse) && (Config_control_type == CONTROL_CYBERMAN)) {
			Controls.vertical_thrust_time -= mouse_button_down_time(MBUTTON_PITCH_FORWARD);
			Controls.vertical_thrust_time += mouse_button_down_time(MBUTTON_PITCH_BACKWARD);
		}

		// From joystick...
		if ((use_joystick) && (kc_joystick[13].value < 255)) {
			if (!kc_joystick[14].value)		// If not inverted...
				Controls.vertical_thrust_time += joy_axis[kc_joystick[13].value];
			else
				Controls.vertical_thrust_time -= joy_axis[kc_joystick[13].value];
		}

		// From mouse...
		if ((use_mouse) && (kc_mouse[13].value < 255)) {
			if (!kc_mouse[14].value)		// If not inverted...
				Controls.vertical_thrust_time -= mouse_axis[kc_mouse[13].value];
			else
				Controls.vertical_thrust_time += mouse_axis[kc_mouse[13].value];
		}
	}

	// From keyboard...
	if (kc_keyboard[14].value < 255) Controls.vertical_thrust_time += speed_factor * key_down_time(kc_keyboard[14].value);
	if (kc_keyboard[15].value < 255) Controls.vertical_thrust_time += speed_factor * key_down_time(kc_keyboard[15].value);
	if (kc_keyboard[16].value < 255) Controls.vertical_thrust_time -= speed_factor * key_down_time(kc_keyboard[16].value);
	if (kc_keyboard[17].value < 255) Controls.vertical_thrust_time -= speed_factor * key_down_time(kc_keyboard[17].value);

	// From joystick...
	if ((use_joystick) && (kc_joystick[19].value < 255)) {
		if (!kc_joystick[20].value)		// If not inverted...
			Controls.vertical_thrust_time += joy_axis[kc_joystick[19].value];
		else
			Controls.vertical_thrust_time -= joy_axis[kc_joystick[19].value];
	}

	// From joystick buttons
	if ((use_joystick) && (kc_joystick[8].value < 255)) Controls.vertical_thrust_time += joy_get_button_down_time(kc_joystick[8].value);
	if ((use_joystick) && (kc_joystick[9].value < 255)) Controls.vertical_thrust_time -= joy_get_button_down_time(kc_joystick[9].value);

	// From mouse buttons
	if ((use_mouse) && (kc_mouse[8].value < 255)) Controls.vertical_thrust_time += mouse_button_down_time(kc_mouse[8].value);
	if ((use_mouse) && (kc_mouse[9].value < 255)) Controls.vertical_thrust_time -= mouse_button_down_time(kc_mouse[9].value);

	// From mouse...
	if ((use_mouse) && (kc_mouse[19].value < 255)) {
		if (!kc_mouse[20].value)		// If not inverted...
			Controls.vertical_thrust_time += mouse_axis[kc_mouse[19].value];
		else
			Controls.vertical_thrust_time -= mouse_axis[kc_mouse[19].value];
	}

	// From Cyberman...
	if ((use_mouse) && (Config_control_type == CONTROL_CYBERMAN)) {
		Controls.vertical_thrust_time += mouse_button_down_time(MBUTTON_Z_UP) / 2;
		Controls.vertical_thrust_time -= mouse_button_down_time(MBUTTON_Z_DOWN) / 2;
	}

	//---------- Read heading_time -----------

	if (!slide_on && !bank_on) {
		//mprintf((0, "heading: %7.3f %7.3f: %7.3f\n", f2fl(k4), f2fl(k6), f2fl(Controls.heading_time)));
		kh = 0;
		k4 = speed_factor * key_down_time(kc_keyboard[4].value);
		k5 = speed_factor * key_down_time(kc_keyboard[5].value);
		k6 = speed_factor * key_down_time(kc_keyboard[6].value);
		k7 = speed_factor * key_down_time(kc_keyboard[7].value);

		// From keyboard...
		if (kc_keyboard[4].value < 255) kh -= k4 / PH_SCALE;
		if (kc_keyboard[5].value < 255) kh -= k5 / PH_SCALE;
		if (kc_keyboard[6].value < 255) kh += k6 / PH_SCALE;
		if (kc_keyboard[7].value < 255) kh += k7 / PH_SCALE;

		// From Cyberman...
		if ((use_mouse) && (Config_control_type == CONTROL_CYBERMAN)) {
			kh -= mouse_button_down_time(MBUTTON_HEAD_LEFT) / PH_SCALE;
			kh += mouse_button_down_time(MBUTTON_HEAD_RIGHT) / PH_SCALE;
		}

		if (kh == 0)
			Controls.heading_time = 0;
		else if (kh > 0) {
			if (Controls.heading_time < 0)
				Controls.heading_time = 0;
		}
		else // kh < 0
			if (Controls.heading_time > 0)
				Controls.heading_time = 0;
		Controls.heading_time += kh;

		// From joystick...
		if ((use_joystick) && (kc_joystick[15].value < 255)) {
			if (!kc_joystick[16].value)		// If not inverted...
				Controls.heading_time += (joy_axis[kc_joystick[15].value] * Config_joystick_sensitivity) / 8;
			else
				Controls.heading_time -= (joy_axis[kc_joystick[15].value] * Config_joystick_sensitivity) / 8;
		}

		// From mouse...
		if ((use_mouse) && (kc_mouse[15].value < 255)) {
			if (!kc_mouse[16].value)		// If not inverted...
				Controls.heading_time += (mouse_axis[kc_mouse[15].value] * Config_joystick_sensitivity) / 8;
			else
				Controls.heading_time -= (mouse_axis[kc_mouse[15].value] * Config_joystick_sensitivity) / 8;
		}
	}
	else {
		Controls.heading_time = 0;
	}

	//----------- Read sideways_thrust_time -----------------

	if (slide_on) {
		k0 = speed_factor * key_down_time(kc_keyboard[4].value);
		k1 = speed_factor * key_down_time(kc_keyboard[5].value);
		k2 = speed_factor * key_down_time(kc_keyboard[6].value);
		k3 = speed_factor * key_down_time(kc_keyboard[7].value);

		// From keyboard...
		if (kc_keyboard[4].value < 255) Controls.sideways_thrust_time -= k0;
		if (kc_keyboard[5].value < 255) Controls.sideways_thrust_time -= k1;
		if (kc_keyboard[6].value < 255) Controls.sideways_thrust_time += k2;
		if (kc_keyboard[7].value < 255) Controls.sideways_thrust_time += k3;

		// From joystick...
		if ((use_joystick) && (kc_joystick[15].value < 255)) {
			if (!kc_joystick[16].value)		// If not inverted...
				Controls.sideways_thrust_time += joy_axis[kc_joystick[15].value];
			else
				Controls.sideways_thrust_time -= joy_axis[kc_joystick[15].value];
		}

		// From cyberman
		if ((use_mouse) && (Config_control_type == CONTROL_CYBERMAN)) {
			Controls.sideways_thrust_time -= mouse_button_down_time(MBUTTON_HEAD_LEFT);
			Controls.sideways_thrust_time += mouse_button_down_time(MBUTTON_HEAD_RIGHT);
		}

		// From mouse...
		if ((use_mouse) && (kc_mouse[15].value < 255)) {
			if (!kc_mouse[16].value)		// If not inverted...
				Controls.sideways_thrust_time += mouse_axis[kc_mouse[15].value];
			else
				Controls.sideways_thrust_time -= mouse_axis[kc_mouse[15].value];
		}
	}

	// From keyboard...
	if (kc_keyboard[10].value < 255) Controls.sideways_thrust_time -= speed_factor * key_down_time(kc_keyboard[10].value);
	if (kc_keyboard[11].value < 255) Controls.sideways_thrust_time -= speed_factor * key_down_time(kc_keyboard[11].value);
	if (kc_keyboard[12].value < 255) Controls.sideways_thrust_time += speed_factor * key_down_time(kc_keyboard[12].value);
	if (kc_keyboard[13].value < 255) Controls.sideways_thrust_time += speed_factor * key_down_time(kc_keyboard[13].value);

	// From joystick...
	if ((use_joystick) && (kc_joystick[17].value < 255)) {
		if (!kc_joystick[18].value)		// If not inverted...
			Controls.sideways_thrust_time -= joy_axis[kc_joystick[17].value];
		else
			Controls.sideways_thrust_time += joy_axis[kc_joystick[17].value];
	}

	// From joystick buttons
	if ((use_joystick) && (kc_joystick[6].value < 255)) Controls.sideways_thrust_time -= joy_get_button_down_time(kc_joystick[6].value);
	if ((use_joystick) && (kc_joystick[7].value < 255)) Controls.sideways_thrust_time += joy_get_button_down_time(kc_joystick[7].value);

	// From mouse buttons
	if ((use_mouse) && (kc_mouse[6].value < 255)) Controls.sideways_thrust_time -= mouse_button_down_time(kc_mouse[6].value);
	if ((use_mouse) && (kc_mouse[7].value < 255)) Controls.sideways_thrust_time += mouse_button_down_time(kc_mouse[7].value);

	// From mouse...
	if ((use_mouse) && (kc_mouse[17].value < 255)) {
		if (!kc_mouse[18].value)		// If not inverted...
			Controls.sideways_thrust_time += mouse_axis[kc_mouse[17].value];
		else
			Controls.sideways_thrust_time -= mouse_axis[kc_mouse[17].value];
	}

	//----------- Read bank_time -----------------

	if (bank_on) {
		k0 = speed_factor * key_down_time(kc_keyboard[4].value);
		k1 = speed_factor * key_down_time(kc_keyboard[5].value);
		k2 = speed_factor * key_down_time(kc_keyboard[6].value);
		k3 = speed_factor * key_down_time(kc_keyboard[7].value);

		// From keyboard...
		if (kc_keyboard[4].value < 255) Controls.bank_time += k0;
		if (kc_keyboard[5].value < 255) Controls.bank_time += k1;
		if (kc_keyboard[6].value < 255) Controls.bank_time -= k2;
		if (kc_keyboard[7].value < 255) Controls.bank_time -= k3;

		// From Cyberman...
		if ((use_mouse) && (Config_control_type == CONTROL_CYBERMAN)) {
			Controls.bank_time -= mouse_button_down_time(MBUTTON_HEAD_LEFT);
			Controls.bank_time += mouse_button_down_time(MBUTTON_HEAD_RIGHT);
		}

		// From joystick...
		if ((use_joystick) && (kc_joystick[15].value < 255)) {
			if (!kc_joystick[16].value)		// If not inverted...
				Controls.bank_time -= (joy_axis[kc_joystick[15].value] * Config_joystick_sensitivity) / 8;
			else
				Controls.bank_time += (joy_axis[kc_joystick[15].value] * Config_joystick_sensitivity) / 8;
		}

		// From mouse...
		if ((use_mouse) && (kc_mouse[15].value < 255)) {
			if (!kc_mouse[16].value)		// If not inverted...
				Controls.bank_time += (mouse_axis[kc_mouse[15].value] * Config_joystick_sensitivity) / 8;
			else
				Controls.bank_time -= (mouse_axis[kc_mouse[15].value] * Config_joystick_sensitivity) / 8;
		}
	}

	// From keyboard...
	if (kc_keyboard[20].value < 255) Controls.bank_time += speed_factor * key_down_time(kc_keyboard[20].value);
	if (kc_keyboard[21].value < 255) Controls.bank_time += speed_factor * key_down_time(kc_keyboard[21].value);
	if (kc_keyboard[22].value < 255) Controls.bank_time -= speed_factor * key_down_time(kc_keyboard[22].value);
	if (kc_keyboard[23].value < 255) Controls.bank_time -= speed_factor * key_down_time(kc_keyboard[23].value);

	// From joystick...
	if ((use_joystick) && (kc_joystick[21].value < 255)) {
		if (!kc_joystick[22].value)		// If not inverted...
			Controls.bank_time -= joy_axis[kc_joystick[21].value];
		else
			Controls.bank_time += joy_axis[kc_joystick[21].value];
	}

	// From joystick buttons
	if ((use_joystick) && (kc_joystick[11].value < 255)) Controls.bank_time += joy_get_button_down_time(kc_joystick[11].value);
	if ((use_joystick) && (kc_joystick[12].value < 255)) Controls.bank_time -= joy_get_button_down_time(kc_joystick[12].value);

	// From mouse buttons
	if ((use_mouse) && (kc_mouse[11].value < 255)) Controls.bank_time += mouse_button_down_time(kc_mouse[11].value);
	if ((use_mouse) && (kc_mouse[12].value < 255)) Controls.bank_time -= mouse_button_down_time(kc_mouse[12].value);

	// From mouse...
	if ((use_mouse) && (kc_mouse[21].value < 255)) {
		if (!kc_mouse[22].value)		// If not inverted...
			Controls.bank_time += mouse_axis[kc_mouse[21].value];
		else
			Controls.bank_time -= mouse_axis[kc_mouse[21].value];
	}

	// From Cyberman
	if ((use_mouse) && (Config_control_type == CONTROL_CYBERMAN)) {
		Controls.bank_time += mouse_button_down_time(MBUTTON_BANK_LEFT);
		Controls.bank_time -= mouse_button_down_time(MBUTTON_BANK_RIGHT);
	}

	//----------- Read forward_thrust_time -------------

		// From keyboard...
	if (kc_keyboard[30].value < 255) Controls.forward_thrust_time += speed_factor * key_down_time(kc_keyboard[30].value);
	if (kc_keyboard[31].value < 255) Controls.forward_thrust_time += speed_factor * key_down_time(kc_keyboard[31].value);
	if (kc_keyboard[32].value < 255) Controls.forward_thrust_time -= speed_factor * key_down_time(kc_keyboard[32].value);
	if (kc_keyboard[33].value < 255) Controls.forward_thrust_time -= speed_factor * key_down_time(kc_keyboard[33].value);

	// From joystick...
	if ((use_joystick) && (kc_joystick[23].value < 255)) {
		if (!kc_joystick[24].value)		// If not inverted...
			Controls.forward_thrust_time -= joy_axis[kc_joystick[23].value];
		else
			Controls.forward_thrust_time += joy_axis[kc_joystick[23].value];
	}

	// From joystick buttons
	if ((use_joystick) && (kc_joystick[2].value < 255)) Controls.forward_thrust_time += joy_get_button_down_time(kc_joystick[2].value);
	if ((use_joystick) && (kc_joystick[3].value < 255)) Controls.forward_thrust_time -= joy_get_button_down_time(kc_joystick[3].value);

	// From mouse...
	if ((use_mouse) && (kc_mouse[23].value < 255)) {
		if (!kc_mouse[24].value)		// If not inverted...
			Controls.forward_thrust_time -= mouse_axis[kc_mouse[23].value];
		else
			Controls.forward_thrust_time += mouse_axis[kc_mouse[23].value];
	}

	// From mouse buttons
	if ((use_mouse) && (kc_mouse[2].value < 255)) Controls.forward_thrust_time += mouse_button_down_time(kc_mouse[2].value);
	if ((use_mouse) && (kc_mouse[3].value < 255)) Controls.forward_thrust_time -= mouse_button_down_time(kc_mouse[3].value);

	//----------- Read fire_primary_down_count
	if (kc_keyboard[24].value < 255) Controls.fire_primary_down_count += key_down_count(kc_keyboard[24].value);
	if (kc_keyboard[25].value < 255) Controls.fire_primary_down_count += key_down_count(kc_keyboard[25].value);
	if ((use_joystick) && (kc_joystick[0].value < 255)) Controls.fire_primary_down_count += joy_get_button_down_cnt(kc_joystick[0].value);
	if ((use_mouse) && (kc_mouse[0].value < 255)) Controls.fire_primary_down_count += mouse_button_down_count(kc_mouse[0].value);

	//----------- Read fire_primary_state
	if (kc_keyboard[24].value < 255) Controls.fire_primary_state |= keyd_pressed[kc_keyboard[24].value];
	if (kc_keyboard[25].value < 255) Controls.fire_primary_state |= keyd_pressed[kc_keyboard[25].value];
	if ((use_joystick) && (kc_joystick[0].value < 255)) Controls.fire_primary_state |= joy_get_button_state(kc_joystick[0].value);
	if ((use_mouse) && (kc_mouse[0].value < 255)) Controls.fire_primary_state |= mouse_button_state(kc_mouse[0].value);

	//----------- Read fire_secondary_down_count
	if (kc_keyboard[26].value < 255) Controls.fire_secondary_down_count += key_down_count(kc_keyboard[26].value);
	if (kc_keyboard[27].value < 255) Controls.fire_secondary_down_count += key_down_count(kc_keyboard[27].value);
	if ((use_joystick) && (kc_joystick[1].value < 255)) Controls.fire_secondary_down_count += joy_get_button_down_cnt(kc_joystick[1].value);
	if ((use_mouse) && (kc_mouse[1].value < 255)) Controls.fire_secondary_down_count += mouse_button_down_count(kc_mouse[1].value);

	//----------- Read fire_secondary_state
	if (kc_keyboard[26].value < 255) Controls.fire_secondary_state |= keyd_pressed[kc_keyboard[26].value];
	if (kc_keyboard[27].value < 255) Controls.fire_secondary_state |= keyd_pressed[kc_keyboard[27].value];
	if ((use_joystick) && (kc_joystick[1].value < 255)) Controls.fire_secondary_state |= joy_get_button_state(kc_joystick[1].value);
	if ((use_mouse) && (kc_mouse[1].value < 255)) Controls.fire_secondary_state |= mouse_button_state(kc_mouse[1].value);

	//----------- Read fire_flare_down_count
	if (kc_keyboard[28].value < 255) Controls.fire_flare_down_count += key_down_count(kc_keyboard[28].value);
	if (kc_keyboard[29].value < 255) Controls.fire_flare_down_count += key_down_count(kc_keyboard[29].value);
	if ((use_joystick) && (kc_joystick[4].value < 255)) Controls.fire_flare_down_count += joy_get_button_down_cnt(kc_joystick[4].value);
	if ((use_mouse) && (kc_mouse[4].value < 255)) Controls.fire_flare_down_count += mouse_button_down_count(kc_mouse[4].value);

	//----------- Read drop_bomb_down_count
	if (kc_keyboard[34].value < 255) Controls.drop_bomb_down_count += key_down_count(kc_keyboard[34].value);
	if (kc_keyboard[35].value < 255) Controls.drop_bomb_down_count += key_down_count(kc_keyboard[35].value);
	if ((use_joystick) && (kc_joystick[26].value < 255)) Controls.drop_bomb_down_count += joy_get_button_down_cnt(kc_joystick[26].value);
	if ((use_mouse) && (kc_mouse[26].value < 255)) Controls.drop_bomb_down_count += mouse_button_down_count(kc_mouse[26].value);

	//----------- Read rear_view_down_count
	if (kc_keyboard[36].value < 255) Controls.rear_view_down_count += key_down_count(kc_keyboard[36].value);
	if (kc_keyboard[37].value < 255) Controls.rear_view_down_count += key_down_count(kc_keyboard[37].value);
	if ((use_joystick) && (kc_joystick[25].value < 255)) Controls.rear_view_down_count += joy_get_button_down_cnt(kc_joystick[25].value);
	if ((use_mouse) && (kc_mouse[25].value < 255)) Controls.rear_view_down_count += mouse_button_down_count(kc_mouse[25].value);

	//----------- Read rear_view_down_state
	if (kc_keyboard[36].value < 255) Controls.rear_view_down_state |= keyd_pressed[kc_keyboard[36].value];
	if (kc_keyboard[37].value < 255) Controls.rear_view_down_state |= keyd_pressed[kc_keyboard[37].value];
	if ((use_joystick) && (kc_joystick[25].value < 255)) Controls.rear_view_down_state |= joy_get_button_state(kc_joystick[25].value);
	if ((use_mouse) && (kc_mouse[25].value < 255)) Controls.rear_view_down_state |= mouse_button_state(kc_mouse[25].value);

	//----------- Read automap_down_count
	if (kc_keyboard[44].value < 255) Controls.automap_down_count += key_down_count(kc_keyboard[44].value);
	if (kc_keyboard[45].value < 255) Controls.automap_down_count += key_down_count(kc_keyboard[45].value);

	//----------- Read automap_state
	if (kc_keyboard[44].value < 255) Controls.automap_state |= keyd_pressed[kc_keyboard[44].value];
	if (kc_keyboard[45].value < 255) Controls.automap_state |= keyd_pressed[kc_keyboard[45].value];

	//----------- Read stupid-cruise-control-type of throttle.
	{
		if (kc_keyboard[38].value < 255) Cruise_speed += fixdiv(speed_factor * key_down_time(kc_keyboard[38].value) * 5, FrameTime);
		if (kc_keyboard[39].value < 255) Cruise_speed += fixdiv(speed_factor * key_down_time(kc_keyboard[39].value) * 5, FrameTime);
		if (kc_keyboard[40].value < 255) Cruise_speed -= fixdiv(speed_factor * key_down_time(kc_keyboard[40].value) * 5, FrameTime);
		if (kc_keyboard[41].value < 255) Cruise_speed -= fixdiv(speed_factor * key_down_time(kc_keyboard[41].value) * 5, FrameTime);
		if ((kc_keyboard[42].value < 255) && (key_down_count(kc_keyboard[42].value)))
			Cruise_speed = 0;
		if ((kc_keyboard[43].value < 255) && (key_down_count(kc_keyboard[43].value)))
			Cruise_speed = 0;

		if (Cruise_speed > i2f(100)) Cruise_speed = i2f(100);
		if (Cruise_speed < 0) Cruise_speed = 0;

		if (Controls.forward_thrust_time == 0)
			Controls.forward_thrust_time = fixmul(Cruise_speed, FrameTime) / 100;
	}

	read_head_tracker();

	// Read external controls
	if (kc_use_external_control)
		kconfig_read_external_controls();

	//----------- Clamp values between -FrameTime and FrameTime
	if (FrameTime > F1_0)
		mprintf((1, "Bogus frame time of %.2f seconds\n", f2fl(FrameTime)));

	if (Controls.pitch_time > FrameTime / 2) Controls.pitch_time = FrameTime / 2;
	if (Controls.vertical_thrust_time > FrameTime) Controls.vertical_thrust_time = FrameTime;
	if (Controls.heading_time > FrameTime) Controls.heading_time = FrameTime;
	if (Controls.sideways_thrust_time > FrameTime) Controls.sideways_thrust_time = FrameTime;
	if (Controls.bank_time > FrameTime) Controls.bank_time = FrameTime;
	if (Controls.forward_thrust_time > FrameTime) Controls.forward_thrust_time = FrameTime;
#ifdef RESTORE_AFTERBURNER
	if (Controls.afterburner_time > FrameTime ) Controls.afterburner_time = FrameTime;
#endif

	if (Controls.pitch_time < -FrameTime / 2) Controls.pitch_time = -FrameTime / 2;
	if (Controls.vertical_thrust_time < -FrameTime) Controls.vertical_thrust_time = -FrameTime;
	if (Controls.heading_time < -FrameTime) Controls.heading_time = -FrameTime;
	if (Controls.sideways_thrust_time < -FrameTime) Controls.sideways_thrust_time = -FrameTime;
	if (Controls.bank_time < -FrameTime) Controls.bank_time = -FrameTime;
	if (Controls.forward_thrust_time < -FrameTime) Controls.forward_thrust_time = -FrameTime;
#ifdef RESTORE_AFTERBURNER
	if (Controls.afterburner_time < -FrameTime ) Controls.afterburner_time = -FrameTime;
#endif


	//--------- Don't do anything if in debug mode
#ifndef NDEBUG
	if (keyd_pressed[KEY_DELETE]) {
		memset(&Controls, 0, sizeof(control_info));
	}
#endif
}

void reset_cruise(void)
{
	Cruise_speed = 0;
}


void kc_set_controls()
{
	int i;

	for (i = 0; i < NUM_KEY_CONTROLS; i++)
		kc_keyboard[i].value = kconfig_settings[0][i];

	if ((Config_control_type > 0) && (Config_control_type < 5)) {
		for (i = 0; i < NUM_OTHER_CONTROLS; i++) {
			kc_joystick[i].value = kconfig_settings[Config_control_type][i];
			if (kc_joystick[i].type == BT_INVERT) {
				if (kc_joystick[i].value != 1)
					kc_joystick[i].value = 0;
				kconfig_settings[Config_control_type][i] = kc_joystick[i].value;
			}
		}
	}
	else if (Config_control_type > 4) {
		for (i = 0; i < NUM_OTHER_CONTROLS; i++) {
			kc_mouse[i].value = kconfig_settings[Config_control_type][i];
			if (kc_mouse[i].type == BT_INVERT) {
				if (kc_mouse[i].value != 1)
					kc_mouse[i].value = 0;
				kconfig_settings[Config_control_type][i] = kc_mouse[i].value;
			}
		}
	}
}


int SenseStatus1(void)
{
	Warning("SenseStatus1: STUB\n");

	return(0);
}

int SenseGetData(int function, int cls, fix* yaw, fix* pitch, fix* roll, int* buttons)
{
	Warning("SenseGetData: STUB\n");
	return 0;
}

void kconfig_center_headset()
{
	Warning("kconfig_center_headset: STUB\n");
}

int SenseSetZero(int function, int cls)
{
	Warning("SenseSetZero: STUB\n");
	return 0;
}

void kconfig_sense_init()
{
	Warning("kconfig_sense_init: STUB\n");
}
