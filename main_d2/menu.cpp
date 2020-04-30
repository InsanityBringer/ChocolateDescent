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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifdef WINDOWS
#include "desw.h"
#endif

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>
#include <algorithm>

#include "misc/rand.h"

//#include "pa_enabl.h"                   //$$POLY_ACC
//#include "vga.h"
#include "platform/posixstub.h"
#include "menu.h"
#include "inferno.h"
#include "game.h"
#include "2d/gr.h"
#include "platform/key.h"
#include "iff/iff.h"
#include "mem/mem.h"
#include "misc/error.h"
#include "bm.h"
#include "screens.h"
#include "platform/mono.h"
#include "platform/joy.h"
#include "vecmat/vecmat.h"
#include "effects.h"
#include "slew.h"
#include "gamemine.h"
#include "gamesave.h"
#include "2d/palette.h"
#include "misc/args.h"
#include "newdemo.h"
#include "platform/timer.h"
#include "sounds.h"
#include "gameseq.h"
#include "text.h"
#include "gamefont.h"
#include "newmenu.h"
#include "network.h"
#include "scores.h"
#include "joydefs.h"
#include "modem.h"
#include "playsave.h"
#include "multi.h"
#include "kconfig.h"
#include "titles.h"
#include "credits.h"
#include "texmap/texmap.h"
#include "polyobj.h"
#include "state.h"
#include "mission.h"
#include "songs.h"
#include "config.h"
#include "movie.h"
#include "gamepal.h"
#include "gauges.h"
//#include "rbaudio.h"
#include "powerup.h"

#ifdef EDITOR
#include "editor\editor.h"
#endif

#if defined(POLY_ACC)
#include "poly_acc.h"
#endif

#define MENU_NEW_GAME            0
#define MENU_GAME                               1 
#define MENU_EDITOR                                     2
#define MENU_VIEW_SCORES                        3
#define MENU_QUIT                4
#define MENU_LOAD_GAME                          5
#define MENU_SAVE_GAME                          6
#define MENU_DEMO_PLAY                          8
#define MENU_LOAD_LEVEL                         9
#define MENU_START_IPX_NETGAME                  10
#define MENU_JOIN_IPX_NETGAME                   11
#define MENU_CONFIG                             13
#define MENU_REJOIN_NETGAME                     14
#define MENU_DIFFICULTY                         15
#define MENU_START_SERIAL                       18
#define MENU_HELP                               19
#define MENU_NEW_PLAYER                         20
#define MENU_MULTIPLAYER                        21
#define MENU_STOP_MODEM                         22
#define MENU_SHOW_CREDITS                       23
#define MENU_ORDER_INFO                         24
#define MENU_PLAY_SONG                          25
#define MENU_START_TCP_NETGAME                  26
#define MENU_JOIN_TCP_NETGAME                   27
#define MENU_START_APPLETALK_NETGAME			28
#define MENU_JOIN_APPLETALK_NETGAME				30

//ADD_ITEM("Start netgame...", MENU_START_NETGAME, -1 );
//ADD_ITEM("Send net message...", MENU_SEND_NET_MESSAGE, -1 );

#define ADD_ITEM(t,value,key)  do { m[num_options].type=NM_TYPE_MENU; m[num_options].text=t; menu_choice[num_options]=value;num_options++; } while (0)

extern int last_joy_time;               //last time the joystick was used
#ifndef NDEBUG
extern int Speedtest_on;
#else
#define Speedtest_on 0
#endif

uint8_t do_auto_demo = 1;                 // Flag used to enable auto demo starting in main menu.
int Player_default_difficulty; // Last difficulty level chosen by the player
int Auto_leveling_on = 1;
int Guided_in_big_window = 0;
int Menu_draw_copyright = 0;
int EscortHotKeys = 1;

// Function Prototypes added after LINTING
void do_option(int select);
void do_detail_level_menu_custon(void);
void do_multi_player_menu(void);
void do_detail_level_menu_custom(void);
void do_new_game_menu(void);

extern void ReorderSecondary();
extern void ReorderPrimary();

//returns the number of demo files on the disk
int newdemo_count_demos();
extern uint8_t Version_major, Version_minor;

extern int HoardEquipped(); //[ISB] godawful hack

// ------------------------------------------------------------------------
void autodemo_menu_check(int nitems, newmenu_item* items, int* last_key, int citem)
{
	int curtime;

	nitems = nitems;
	items = items;
	citem = citem;

	//draw copyright message
	if (Menu_draw_copyright)
	{
		int w, h, aw;

		Menu_draw_copyright = 0;
		WINDOS(dd_gr_set_current_canvas(NULL),
			gr_set_current_canvas(NULL));
		gr_set_curfont(GAME_FONT);
		gr_set_fontcolor(BM_XRGB(6, 6, 6), -1);

		gr_get_string_size("V2.2", &w, &h, &aw);

		WIN(DDGRLOCK(dd_grd_curcanv));
		gr_printf(0x8000, grd_curcanv->cv_bitmap.bm_h - GAME_FONT->ft_h - 2, TXT_COPYRIGHT);
		gr_printf(grd_curcanv->cv_bitmap.bm_w - w - 2, grd_curcanv->cv_bitmap.bm_h - GAME_FONT->ft_h - 2, "V%d.%d", Version_major, Version_minor);

#ifdef SANTA		//say this is hoard version
		if (HoardEquipped())
		{
			gr_set_curfont(MEDIUM2_FONT);
			gr_printf(MenuHires ? 495 : 00, MenuHires ? 88 : 44, "Vertigo");
		}
#endif

		WIN(DDGRUNLOCK(dd_grd_curcanv));
	}

	// Don't allow them to hit ESC in the main menu.
	if (*last_key == KEY_ESC)* last_key = 0;

	if (do_auto_demo)
	{
		curtime = timer_get_approx_seconds();
		//if ( ((keyd_time_when_last_pressed+i2f(20)) < curtime) && ((last_joy_time+i2f(20)) < curtime) && (!Speedtest_on)  ) {
#ifndef MACINTOSH		// for now only!!!!
		if (((keyd_time_when_last_pressed + i2f(25)) < curtime) && (!Speedtest_on))
		{
#else
		if ((keyd_time_when_last_pressed + i2f(40)) < curtime)
		{
#endif
			int n_demos;
			n_demos = newdemo_count_demos();
		try_again:;
			if ((P_Rand() % (n_demos + 1)) == 0)
			{
#ifndef SHAREWARE
#ifdef WINDOWS
				mouse_set_mode(1);				//re-enable centering mode
				HideCursorW();
#endif
				PlayMovie("intro.mve", 0);
				songs_play_song(SONG_TITLE, 1);
				*last_key = -3; //exit menu to force redraw even if not going to game mode. -3 tells menu system not to restore
				set_screen_mode(SCREEN_MENU);
#ifdef WINDOWS
				mouse_set_mode(0);				//disenable centering mode
				ShowCursorW();
#endif
#endif // end of ifndef shareware
			}
			else
			{
				WIN(HideCursorW());
				keyd_time_when_last_pressed = curtime;                  // Reset timer so that disk won't thrash if no demos.
				newdemo_start_playback(NULL);           // Randomly pick a file
				if (Newdemo_state == ND_STATE_PLAYBACK)
				{
					Function_mode = FMODE_GAME;
					*last_key = -3; //exit menu to get into game mode. -3 tells menu system not to restore
				}
				else
					goto try_again;	//keep trying until we get a demo that works
			}
		}
	}
}

//static int First_time = 1;
static int main_menu_choice = 0;

//      -----------------------------------------------------------------------------
//      Create the main menu.
void create_main_menu(newmenu_item * m, int* menu_choice, int* callers_num_options)
{
	int     num_options;

#ifndef DEMO_ONLY
	num_options = 0;

	ADD_ITEM(TXT_NEW_GAME, MENU_NEW_GAME, KEY_N);

	ADD_ITEM(TXT_LOAD_GAME, MENU_LOAD_GAME, KEY_L);

	ADD_ITEM(TXT_MULTIPLAYER_, MENU_MULTIPLAYER, -1);

	ADD_ITEM(TXT_OPTIONS_, MENU_CONFIG, -1);
	ADD_ITEM(TXT_CHANGE_PILOTS, MENU_NEW_PLAYER, unused);
	ADD_ITEM(TXT_VIEW_DEMO, MENU_DEMO_PLAY, 0);
	ADD_ITEM(TXT_VIEW_SCORES, MENU_VIEW_SCORES, KEY_V);
#ifdef SHAREWARE
	ADD_ITEM(TXT_ORDERING_INFO, MENU_ORDER_INFO, -1);
#endif
	ADD_ITEM(TXT_CREDITS, MENU_SHOW_CREDITS, -1);
#endif
	ADD_ITEM(TXT_QUIT, MENU_QUIT, KEY_Q);

#ifndef RELEASE
	if (!(Game_mode & GM_MULTI))
	{
		ADD_ITEM(const_cast<char*>("  Load level..."), MENU_LOAD_LEVEL, KEY_N);
#ifdef EDITOR
		ADD_ITEM(const_cast<char*>("  Editor"), MENU_EDITOR, KEY_E);
#endif
	}
	//ADD_ITEM(const_cast<char*>("  Play song"), MENU_PLAY_SONG, -1 );
#endif

	* callers_num_options = num_options;
}

//returns number of item chosen
int DoMenu()
{
	int menu_choice[25];
	newmenu_item m[25];
	int num_options = 0;

	load_palette(MENU_PALETTE, 0, 1);		//get correct palette

	if (Players[Player_num].callsign[0] == 0)
	{
		RegisterPlayer();
		return 0;
	}

	if ((Game_mode & GM_SERIAL) || (Game_mode & GM_MODEM))
	{
		do_option(MENU_START_SERIAL);
		return 0;
	}

	create_main_menu(m, menu_choice, &num_options);

	do
	{
		keyd_time_when_last_pressed = timer_get_fixed_seconds();                // .. 20 seconds from now!
		if (main_menu_choice < 0)      main_menu_choice = 0;
		Menu_draw_copyright = 1;
		main_menu_choice = newmenu_do2("", NULL, num_options, m, autodemo_menu_check, main_menu_choice, Menu_pcx_name);
		if (main_menu_choice > -1) do_option(menu_choice[main_menu_choice]);
		create_main_menu(m, menu_choice, &num_options); //      may have to change, eg, maybe selected pilot and no save games.
	} while (Function_mode == FMODE_MENU);

	//      if (main_menu_choice != -2)
	//              do_auto_demo = 0;               // No more auto demos
	if (Function_mode == FMODE_GAME)
		gr_palette_fade_out(gr_palette, 32, 0);

	return main_menu_choice;
}

extern void show_order_form(void);      // John didn't want this in inferno.h so I just externed it.

#ifdef WINDOWS
#undef TXT_SELECT_DEMO
#define TXT_SELECT_DEMO "Select Demo\n<Ctrl-D> or Right-click\nto delete"
#endif

//returns flag, true means quit menu
void do_option(int select)
{
	switch (select)
	{
	case MENU_NEW_GAME:
		do_new_game_menu();
		break;
	case MENU_GAME:
		break;
	case MENU_DEMO_PLAY:
	{
		char demo_file[16];
		if (newmenu_get_filename(TXT_SELECT_DEMO, ".\\demos\\*.dem", demo_file, 1))
		{
			newdemo_start_playback(demo_file);
		}
	}
	break;
	case MENU_LOAD_GAME:
		state_restore_all(0, 0, NULL);
		break;
#ifdef EDITOR
	case MENU_EDITOR:
		Function_mode = FMODE_EDITOR;
		init_cockpit();
		break;
#endif
	case MENU_VIEW_SCORES:
		gr_palette_fade_out(gr_palette, 32, 0);
		scores_view(-1);
		break;
#ifdef SHAREWARE
	case MENU_ORDER_INFO:
		show_order_form();
		break;
#endif
	case MENU_QUIT:
#ifdef EDITOR
		if (!SafetyCheck()) break;
#endif
		gr_palette_fade_out(gr_palette, 32, 0);
		Function_mode = FMODE_EXIT;
		break;
	case MENU_NEW_PLAYER:
		RegisterPlayer();               //1 == allow escape out of menu
		break;

	case MENU_HELP:
		do_show_help();
		break;

#ifndef RELEASE

	case MENU_PLAY_SONG:
	{
		int i;
		char* m[MAX_NUM_SONGS];

		for (i = 0; i < Num_songs; i++)
		{
			m[i] = Songs[i].filename;
		}
		i = newmenu_listbox("Select Song", Num_songs, m, 1, NULL);

		if (i > -1)
		{
			songs_play_song(i, 0);
		}
	}
	break;
	case MENU_LOAD_LEVEL:
	{
		newmenu_item m;
		char text[11] = "";
		int new_level_num;

		m.type = NM_TYPE_INPUT; m.text_len = 10; m.text = text;

		newmenu_do(NULL, "Enter level to load", 1, &m, NULL);

		new_level_num = atoi(m.text);

		if (new_level_num != 0 && new_level_num >= Last_secret_level && new_level_num <= Last_level)
		{
			gr_palette_fade_out(gr_palette, 32, 0);
			StartNewGame(new_level_num);
		}

		break;
	}
#endif


	case MENU_START_IPX_NETGAME:
#ifdef NETWORK
		load_mission(0);
#ifdef MACINTOSH
		Network_game_type = IPX_GAME;
#endif
		//			WIN(ipx_create_read_thread());
		network_start_game();
#endif
		break;

	case MENU_JOIN_IPX_NETGAME:
#ifdef NETWORK
		load_mission(0);
#ifdef MACINTOSH
		Network_game_type = IPX_GAME;
#endif
		//			WIN(ipx_create_read_thread());
		network_join_game();
#endif
		break;

	case MENU_START_TCP_NETGAME:
	case MENU_JOIN_TCP_NETGAME:
		nm_messagebox(TXT_SORRY, 1, TXT_OK, "Not available in shareware version!");
		// DoNewIPAddress();
		break;

	case MENU_START_SERIAL:
#ifdef NETWORK
		com_main_menu();
#endif
		break;
	case MENU_MULTIPLAYER:
		do_multi_player_menu();
		break;
	case MENU_CONFIG:
		do_options_menu();
		break;
	case MENU_SHOW_CREDITS:
		gr_palette_fade_out(gr_palette, 32, 0);
		songs_stop_all();
		credits_show(NULL);
		break;
	default:
		Error("Unknown option %d in do_option", select);
		break;
	}
}

int do_difficulty_menu()
{
	int s;
	newmenu_item m[5];

	m[0].type = NM_TYPE_MENU; m[0].text = MENU_DIFFICULTY_TEXT(0);
	m[1].type = NM_TYPE_MENU; m[1].text = MENU_DIFFICULTY_TEXT(1);
	m[2].type = NM_TYPE_MENU; m[2].text = MENU_DIFFICULTY_TEXT(2);
	m[3].type = NM_TYPE_MENU; m[3].text = MENU_DIFFICULTY_TEXT(3);
	m[4].type = NM_TYPE_MENU; m[4].text = MENU_DIFFICULTY_TEXT(4);

	s = newmenu_do1(NULL, TXT_DIFFICULTY_LEVEL, NDL, m, NULL, Difficulty_level);

	if (s > -1)
	{
		if (s != Difficulty_level)
		{
			Player_default_difficulty = s;
			write_player_file();
		}
		Difficulty_level = s;
		mprintf((0, "%s %s %i\n", TXT_DIFFICULTY_LEVEL, TXT_SET_TO, Difficulty_level));
		return 1;
	}
	return 0;
}

int     Max_debris_objects, Max_objects_onscreen_detailed;
int     Max_linear_depth_objects;

int8_t    Object_complexity = 2, Object_detail = 2;
int8_t    Wall_detail = 2, Wall_render_depth = 2, Debris_amount = 2, SoundChannels = 2;

int8_t    Render_depths[NUM_DETAIL_LEVELS - 1] = { 6,  9, 12, 15, 50 };
int8_t    Max_perspective_depths[NUM_DETAIL_LEVELS - 1] = { 1,  2,  3,  5,  8 };
int8_t    Max_linear_depths[NUM_DETAIL_LEVELS - 1] = { 3,  5,  7, 10, 50 };
int8_t    Max_linear_depths_objects[NUM_DETAIL_LEVELS - 1] = { 1,  2,  3,  7, 20 };
int8_t    Max_debris_objects_list[NUM_DETAIL_LEVELS - 1] = { 2,  4,  7, 10, 15 };
int8_t    Max_objects_onscreen_detailed_list[NUM_DETAIL_LEVELS - 1] = { 2,  4,  7, 10, 15 };
int8_t    Smts_list[NUM_DETAIL_LEVELS - 1] = { 2,  4,  8, 16, 50 };   //      threshold for models to go to lower detail model, gets multiplied by obj->size
int8_t    Max_sound_channels[NUM_DETAIL_LEVELS - 1] = { 2,  4,  8, 12, 16 };

//      -----------------------------------------------------------------------------
//      Set detail level based stuff.
//      Note: Highest detail level (detail_level == NUM_DETAIL_LEVELS-1) is custom detail level.
void set_detail_level_parameters(int detail_level)
{
	Assert((detail_level >= 0) && (detail_level < NUM_DETAIL_LEVELS));

	if (detail_level < NUM_DETAIL_LEVELS - 1)
	{
		Render_depth = Render_depths[detail_level];
		Max_perspective_depth = Max_perspective_depths[detail_level];
		Max_linear_depth = Max_linear_depths[detail_level];
		Max_linear_depth_objects = Max_linear_depths_objects[detail_level];

		Max_debris_objects = Max_debris_objects_list[detail_level];
		Max_objects_onscreen_detailed = Max_objects_onscreen_detailed_list[detail_level];

		Simple_model_threshhold_scale = Smts_list[detail_level];

		digi_set_max_channels(Max_sound_channels[detail_level]);

		//      Set custom menu defaults.
		Object_complexity = detail_level;
		Wall_render_depth = detail_level;
		Object_detail = detail_level;
		Wall_detail = detail_level;
		Debris_amount = detail_level;
		SoundChannels = detail_level;

#if defined(POLY_ACC)

#ifdef MACINTOSH
		if (detail_level < 2)
		{
			pa_set_filtering(0);
		}
		else if (detail_level < 4)
		{
			pa_set_filtering(1);
		}
		else if (detail_level == 4)
		{
			pa_set_filtering(2);
		}
#else
		pa_filter_mode = detail_level;
#endif
#endif

	}
}

//      -----------------------------------------------------------------------------
void do_detail_level_menu(void)
{
	int s;
	newmenu_item m[7];

	m[0].type = NM_TYPE_MENU; m[0].text = MENU_DETAIL_TEXT(0);
	m[1].type = NM_TYPE_MENU; m[1].text = MENU_DETAIL_TEXT(1);
	m[2].type = NM_TYPE_MENU; m[2].text = MENU_DETAIL_TEXT(2);
	m[3].type = NM_TYPE_MENU; m[3].text = MENU_DETAIL_TEXT(3);
	m[4].type = NM_TYPE_MENU; m[4].text = MENU_DETAIL_TEXT(4);
	m[5].type = NM_TYPE_TEXT; m[5].text = const_cast<char*>("");
	m[6].type = NM_TYPE_MENU; m[6].text = MENU_DETAIL_TEXT(5);

	s = newmenu_do1(NULL, TXT_DETAIL_LEVEL, NDL + 2, m, NULL, Detail_level);

	if (s > -1)
	{
		switch (s)
		{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
			Detail_level = s;
			mprintf((0, "Detail level set to %i\n", Detail_level));
			set_detail_level_parameters(Detail_level);
			break;
		case 6:
			Detail_level = 5;
			do_detail_level_menu_custom();
			break;
		}
	}
}

//      -----------------------------------------------------------------------------
void do_detail_level_menu_custom_menuset(int nitems, newmenu_item * items, int* last_key, int citem)
{
	nitems = nitems;
	*last_key = *last_key;
	citem = citem;

	Object_complexity = items[0].value;
	Object_detail = items[1].value;
	Wall_detail = items[2].value;
	Wall_render_depth = items[3].value;
	Debris_amount = items[4].value;
	SoundChannels = items[5].value;
#if defined(POLY_ACC)
	pa_filter_mode = items[6].value;
#endif

}

void set_custom_detail_vars(void)
{
	Render_depth = Render_depths[Wall_render_depth];

	Max_perspective_depth = Max_perspective_depths[Wall_detail];
	Max_linear_depth = Max_linear_depths[Wall_detail];

	Max_debris_objects = Max_debris_objects_list[Debris_amount];

	Max_objects_onscreen_detailed = Max_objects_onscreen_detailed_list[Object_complexity];
	Simple_model_threshhold_scale = Smts_list[Object_complexity];
	Max_linear_depth_objects = Max_linear_depths_objects[Object_detail];

	digi_set_max_channels(Max_sound_channels[SoundChannels]);
}

#define	DL_MAX	10

//      -----------------------------------------------------------------------------

void do_detail_level_menu_custom(void)
{
	int	count;
	int	s = 0;
	newmenu_item m[DL_MAX];
#if defined(POLY_ACC)
	int filtering_id;
#endif

	do
	{
		count = 0;
		m[count].type = NM_TYPE_SLIDER;
		m[count].text = TXT_OBJ_COMPLEXITY;
		m[count].value = Object_complexity;
		m[count].min_value = 0;
		m[count++].max_value = NDL - 1;

		m[count].type = NM_TYPE_SLIDER;
		m[count].text = TXT_OBJ_DETAIL;
		m[count].value = Object_detail;
		m[count].min_value = 0;
		m[count++].max_value = NDL - 1;

		m[count].type = NM_TYPE_SLIDER;
		m[count].text = TXT_WALL_DETAIL;
		m[count].value = Wall_detail;
		m[count].min_value = 0;
		m[count++].max_value = NDL - 1;

		m[count].type = NM_TYPE_SLIDER;
		m[count].text = TXT_WALL_RENDER_DEPTH;
		m[count].value = Wall_render_depth;
		m[count].min_value = 0;
		m[count++].max_value = NDL - 1;

		m[count].type = NM_TYPE_SLIDER;
		m[count].text = TXT_DEBRIS_AMOUNT;
		m[count].value = Debris_amount;
		m[count].min_value = 0;
		m[count++].max_value = NDL - 1;

		m[count].type = NM_TYPE_SLIDER;
		m[count].text = TXT_SOUND_CHANNELS;
		m[count].value = SoundChannels;
		m[count].min_value = 0;
		m[count++].max_value = NDL - 1;

#if defined(POLY_ACC)
		MAC(if (PAEnabled) {
			)
			filtering_id = count;
			m[count].type = NM_TYPE_SLIDER;
			m[count].text = "FILTERING";
			m[count].value = pa_filter_mode;
			m[count].min_value = 0;
#ifdef MACINTOSH
			m[count++].max_value = 2;
#else
			m[count++].max_value = NDL - 1;
#endif
			MAC(
		})
#endif

				m[count].type = NM_TYPE_TEXT;
			m[count++].text = TXT_LO_HI;

			Assert(count < DL_MAX);

			s = newmenu_do1(NULL, TXT_DETAIL_CUSTOM, count, m, do_detail_level_menu_custom_menuset, s);
	} while (s > -1);

	set_custom_detail_vars();

#if defined(MACINTOSH) && defined(POLY_ACC)
	if (PAEnabled)
		pa_set_filtering(m[filtering_id].value);
#endif
}

#ifndef MACINTOSH
int Default_display_mode = 0;
int Current_display_mode = 0;
#else
int Default_display_mode = 1;
int Current_display_mode = 1;
#endif

extern int MenuHiresAvailable;

typedef struct
{
	int	VGA_mode;
	short	w, h;
	short	render_method;
	short	flags;
} dmi;

dmi display_mode_info[7] =
{
		{SM_320x200C,	 320,	200, VR_NONE, VRF_ALLOW_COCKPIT + VRF_COMPATIBLE_MENUS},
		{SM_640x480V,	 640, 480, VR_NONE, VRF_COMPATIBLE_MENUS + VRF_ALLOW_COCKPIT},
		{SM_320x400U,	 320, 400, VR_NONE, VRF_USE_PAGING},
		{SM_640x400V,	 640, 400, VR_NONE, VRF_COMPATIBLE_MENUS},
		{SM_800x600V,	 800, 600, VR_NONE, VRF_COMPATIBLE_MENUS},
		{SM_1024x768V,	1024,	768, VR_NONE, VRF_COMPATIBLE_MENUS},
		{SM_1280x1024V,1280,1024, VR_NONE, VRF_COMPATIBLE_MENUS},
};

WIN(extern int DD_Emulation);


void set_display_mode(int mode)
{
	dmi* dmi;

	if ((Current_display_mode == -1) || (VR_render_mode != VR_NONE))	//special VR mode
		return;								//...don't change

	if (mode >= 5 && !FindArg("-superhires"))
		mode = 4;

	if (!MenuHiresAvailable && (mode != 2))
		mode = 0;

	if (gr_check_mode(display_mode_info[mode].VGA_mode) != 0)		//can't do mode //[ISB]vga_check_mode
		mode = 0;

	Current_display_mode = mode;

	dmi = &display_mode_info[mode];

	if (Current_display_mode != -1)
	{
		game_init_render_buffers(dmi->VGA_mode, dmi->w, dmi->h, dmi->render_method, dmi->flags);
		Default_display_mode = Current_display_mode;
	}

	Screen_mode = -1;		//force screen reset
}

void do_screen_res_menu()
{
#define N_SCREENRES_ITEMS 9
	newmenu_item m[N_SCREENRES_ITEMS];
	int citem;
	int i;
	int n_items;
	int result;

	if ((Current_display_mode == -1) || (VR_render_mode != VR_NONE)) {				//special VR mode
		nm_messagebox(TXT_SORRY, 1, TXT_OK,
			"You may not change screen\n"
			"resolution when VR modes enabled.");
		return;
	}

	m[0].type = NM_TYPE_TEXT;	 m[0].value = 0;    			  m[0].text = const_cast<char*>("Modes w/ Cockpit:");

	m[1].type = NM_TYPE_RADIO; m[1].value = 0; m[1].group = 0; m[1].text = const_cast<char*>(" 320x200");

	m[2].type = NM_TYPE_RADIO; m[2].value = 0; m[2].group = 0; m[2].text = const_cast<char*>(" 640x480");
	m[3].type = NM_TYPE_TEXT;	 m[3].value = 0;   				  m[3].text = const_cast<char*>("Modes w/o Cockpit:");
	m[4].type = NM_TYPE_RADIO; m[4].value = 0; m[4].group = 0; m[4].text = const_cast<char*>(" 320x400");
	m[5].type = NM_TYPE_RADIO; m[5].value = 0; m[5].group = 0; m[5].text = const_cast<char*>(" 640x480");
	m[6].type = NM_TYPE_RADIO; m[6].value = 0; m[6].group = 0; m[6].text = const_cast<char*>(" 800x600");
	n_items = 7;
	if (FindArg("-superhires"))
	{
		m[7].type = NM_TYPE_RADIO; m[7].value = 0; m[7].group = 0; m[7].text = const_cast<char*>(" 1024x768");
		m[8].type = NM_TYPE_RADIO; m[8].value = 0; m[8].group = 0; m[8].text = const_cast<char*>(" 1280x1024");
		n_items += 2;
	}

	citem = Current_display_mode + 1;

	if (Current_display_mode >= 2)
		citem++;

	if (citem >= n_items)
		citem = n_items - 1;

	m[citem].value = 1;

	newmenu_do1(NULL, "Select screen mode", n_items, m, NULL, citem);

	for (i = 0; i < n_items; i++)
		if (m[i].value)
			break;

#ifndef WINDOWS 								// if i >= 4 keep it that way since we skip 320x400
	if (i >= 4)
		i--;
#endif

	i--;

	if (((i != 0) && (i != 2) && !MenuHiresAvailable) || gr_check_mode(display_mode_info[i].VGA_mode)) {
		nm_messagebox(TXT_SORRY, 1, TXT_OK,
			"Cannot set requested\n"
			"mode on this video card.");
		return;
	}
#ifdef SHAREWARE
	if (i != 0)
		nm_messagebox(TXT_SORRY, 1, TXT_OK,
			"High resolution modes are\n"
			"only available in the\n"
			"Commercial version of Descent 2.");
	return;
#else
	if (i != Current_display_mode)
		set_display_mode(i);
#endif
}

void do_new_game_menu()
{
	int new_level_num, player_highest_level;

#ifndef SHAREWARE
	int n_missions;

	n_missions = build_mission_list(0);

	if (n_missions > 1)
	{
		int new_mission_num, i, default_mission;
		char* m[MAX_MISSIONS];

		default_mission = 0;
		for (i = 0; i < n_missions; i++)
		{
			m[i] = Mission_list[i].mission_name;
			if (!_stricmp(m[i], config_last_mission))
				default_mission = i;
		}

		new_mission_num = newmenu_listbox1("New Game\n\nSelect mission", n_missions, m, 1, default_mission, NULL);

		if (new_mission_num == -1)
			return;         //abort!

		strcpy(config_last_mission, m[new_mission_num]);

		if (!load_mission(new_mission_num))
		{
			nm_messagebox(NULL, 1, TXT_OK, "Error in Mission file");
			return;
		}
	}
#endif

	new_level_num = 1;

	player_highest_level = get_highest_level();

	if (player_highest_level > Last_level)
		player_highest_level = Last_level;

	if (player_highest_level > 1)
	{
		newmenu_item m[4];
		char info_text[80];
		char num_text[10];
		int choice;
		int n_items;

	try_again:
		sprintf(info_text, "%s %d", TXT_START_ANY_LEVEL, player_highest_level);

		m[0].type = NM_TYPE_TEXT; m[0].text = info_text;
		m[1].type = NM_TYPE_INPUT; m[1].text_len = 10; m[1].text = num_text;
		n_items = 2;

		strcpy(num_text, "1");

		choice = newmenu_do(NULL, TXT_SELECT_START_LEV, n_items, m, NULL);

		if (choice == -1 || m[1].text[0] == 0)
			return;

		new_level_num = atoi(m[1].text);

		if (!(new_level_num > 0 && new_level_num <= player_highest_level))
		{
			m[0].text = TXT_ENTER_TO_CONT;
			nm_messagebox(NULL, 1, TXT_OK, TXT_INVALID_LEVEL);
			goto try_again;
		}
	}

	Difficulty_level = Player_default_difficulty;

	if (!do_difficulty_menu())
		return;

	gr_palette_fade_out(gr_palette, 32, 0);
	StartNewGame(new_level_num);
}

extern void GameLoop(int, int);

extern int Redbook_enabled;

void do_sound_menu();
void do_toggles_menu();

void options_menuset(int nitems, newmenu_item * items, int* last_key, int citem)
{
	if (citem == 5)
	{
		gr_palette_set_gamma(items[5].value);
	}

	nitems++;		//kill warning
	last_key++;		//kill warning
}

void do_options_menu()
{
	newmenu_item m[12];
	int i = 0;

	do {
		m[0].type = NM_TYPE_MENU;   m[0].text = const_cast<char*>("Sound effects & music...");
		m[1].type = NM_TYPE_TEXT;   m[1].text = const_cast<char*>("");
		m[2].type = NM_TYPE_MENU;   m[2].text = TXT_CONTROLS_;
		m[3].type = NM_TYPE_MENU;   m[3].text = TXT_CAL_JOYSTICK;
		m[4].type = NM_TYPE_TEXT;   m[4].text = const_cast<char*>("");

#if defined(POLY_ACC)
		m[5].type = NM_TYPE_TEXT;   m[5].text = const_cast<char*>("");
#else
		m[5].type = NM_TYPE_SLIDER; m[5].text = TXT_BRIGHTNESS; m[5].value = gr_palette_get_gamma(); m[5].min_value = 0; m[5].max_value = 8;
#endif


#ifdef PA_3DFX_VOODOO
		m[6].type = NM_TYPE_TEXT;   m[6].text = const_cast<char*>("");
#else
		m[6].type = NM_TYPE_MENU;   m[6].text = TXT_DETAIL_LEVELS;
#endif

#if defined(POLY_ACC)
		m[7].type = NM_TYPE_TEXT;   m[7].text = const_cast<char*>("");
#else
		m[7].type = NM_TYPE_MENU;   m[7].text = const_cast<char*>("Screen resolution...");
#endif
		m[8].type = NM_TYPE_TEXT;   m[8].text = const_cast<char*>("");
		m[9].type = NM_TYPE_MENU;   m[9].text = const_cast<char*>("Primary autoselect ordering...");
		m[10].type = NM_TYPE_MENU;   m[10].text = const_cast<char*>("Secondary autoselect ordering...");
		m[11].type = NM_TYPE_MENU;   m[11].text = const_cast<char*>("Toggles...");

		i = newmenu_do1(NULL, TXT_OPTIONS, sizeof(m) / sizeof(*m), m, options_menuset, i);

		switch (i)
		{
		case  0: do_sound_menu();			break;
		case  2: joydefs_config();			break;
		case  3: joydefs_calibrate();		break;
		case  6: do_detail_level_menu(); 	break;
		case  7: do_screen_res_menu();		break;
		case  9: ReorderPrimary();			break;
		case 10: ReorderSecondary();		break;
		case 11: do_toggles_menu();			break;
		}

	} while (i > -1);

	write_player_file();
}

extern int Redbook_playing;
void set_redbook_volume(int volume);

//WIN(extern int RBCDROM_State);
//WIN(static BOOL windigi_driver_off=FALSE);

void sound_menuset(int nitems, newmenu_item * items, int* last_key, int citem)
{
	nitems = nitems;
	*last_key = *last_key;

	if (Config_digi_volume != items[0].value)
	{
		Config_digi_volume = items[0].value;

		digi_set_digi_volume((Config_digi_volume * 32768) / 8);
		digi_play_sample_once(SOUND_DROP_BOMB, F1_0);
	}

	if (Config_midi_volume != items[1].value)
	{
		Config_midi_volume = items[1].value;
		digi_set_midi_volume((Config_midi_volume * 128) / 8);
	}

	// don't enable redbook for a non-apple demo version of the shareware demo
#if !defined(SHAREWARE) || ( defined(SHAREWARE) && defined(APPLE_DEMO) )

	if (Config_redbook_volume != items[2].value)
	{
		Config_redbook_volume = items[2].value;
		set_redbook_volume(Config_redbook_volume);
	}

	if (items[4].value != (Redbook_playing != 0))
	{

		if (items[4].value && FindArg("-noredbook"))
		{
			nm_messagebox(TXT_SORRY, 1, TXT_OK, "Redbook audio has been disabled\non the command line");
			items[4].value = 0;
			items[4].redraw = 1;
		}
		else
		{
			Redbook_enabled = items[4].value;

			mprintf((1, "Redbook_enabled = %d\n", Redbook_enabled));

			if (Function_mode == FMODE_MENU)
				songs_play_song(SONG_TITLE, 1);
			else if (Function_mode == FMODE_GAME)
				songs_play_level_song(Current_level_num);
			else
				Int3();

			if (items[4].value && !Redbook_playing)
			{
				nm_messagebox(TXT_SORRY, 1, TXT_OK, "Cannot start CD Music.  Insert\nyour Descent II CD and try again");
				items[4].value = 0;
				items[4].redraw = 1;
			}

			items[1].type = (Redbook_playing ? NM_TYPE_TEXT : NM_TYPE_SLIDER);
			items[1].redraw = 1;
			items[2].type = (Redbook_playing ? NM_TYPE_SLIDER : NM_TYPE_TEXT);
			items[2].redraw = 1;
		}
	}

#endif
	citem++;		//kill warning
}

void do_sound_menu()
{
	newmenu_item m[6];
	int i = 0;

	do
	{
		m[0].type = NM_TYPE_SLIDER; m[0].text = TXT_FX_VOLUME; m[0].value = Config_digi_volume; m[0].min_value = 0; m[0].max_value = 8;
		m[1].type = (Redbook_playing ? NM_TYPE_TEXT : NM_TYPE_SLIDER); m[1].text = const_cast<char*>("MIDI music volume"); m[1].value = Config_midi_volume; m[1].min_value = 0; m[1].max_value = 8;

#ifdef SHAREWARE
		m[2].type = NM_TYPE_TEXT; m[2].text = const_cast<char*>("");
		m[3].type = NM_TYPE_TEXT; m[3].text = const_cast<char*>("");
		m[4].type = NM_TYPE_TEXT; m[4].text = const_cast<char*>("");

#else		// ifdef SHAREWARE
		m[2].type = (Redbook_playing ? NM_TYPE_SLIDER : NM_TYPE_TEXT); m[2].text = const_cast<char*>("CD music volume"); m[2].value = Config_redbook_volume; m[2].min_value = 0; m[2].max_value = 8;

		m[3].type = NM_TYPE_TEXT; m[3].text = const_cast<char*>("");

		m[4].type = NM_TYPE_CHECK;  m[4].text = const_cast<char*>("CD Music (Redbook) enabled"); m[4].value = (Redbook_playing != 0);
#endif

		m[5].type = NM_TYPE_CHECK;  m[5].text = TXT_REVERSE_STEREO; m[5].value = Config_channels_reversed;

		i = newmenu_do1(NULL, "Sound Effects & Music", sizeof(m) / sizeof(*m), m, sound_menuset, i);

		Redbook_enabled = m[4].value;
		Config_channels_reversed = m[5].value;

	} while (i > -1);

	if (Config_midi_volume < 1)
	{
		digi_play_midi_song(NULL, NULL, NULL, 0);
	}

}


extern int Automap_always_hires;

#define ADD_CHECK(n,txt,v)  do { m[n].type=NM_TYPE_CHECK; m[n].text=txt; m[n].value=v;} while (0)

void do_toggles_menu()
{
#if defined(POLY_ACC)
#define N_TOGGLE_ITEMS 6        // get rid of automap hi-res.
#else
#define N_TOGGLE_ITEMS 7
#endif

	newmenu_item m[N_TOGGLE_ITEMS];
	int i = 0;

	do
	{
		ADD_CHECK(0, const_cast<char*>("Ship auto-leveling"), Auto_leveling_on);
		ADD_CHECK(1, const_cast<char*>("Show reticle"), Reticle_on);
		ADD_CHECK(2, const_cast<char*>("Missile view"), Missile_view_enabled);
		ADD_CHECK(3, const_cast<char*>("Headlight on when picked up"), Headlight_active_default);
		ADD_CHECK(4, const_cast<char*>("Show guided missile in main display"), Guided_in_big_window);
		ADD_CHECK(5, const_cast<char*>("Escort robot hot keys"), EscortHotKeys);
#if !defined(POLY_ACC)
		ADD_CHECK(6, const_cast<char*>("Always show HighRes Automap"), std::min(MenuHiresAvailable, Automap_always_hires));
#endif
		//when adding more options, change N_TOGGLE_ITEMS above
		i = newmenu_do1(NULL, "Toggles", N_TOGGLE_ITEMS, m, NULL, i);

		Auto_leveling_on = m[0].value;
		Reticle_on = m[1].value;
		Missile_view_enabled = m[2].value;
		Headlight_active_default = m[3].value;
		Guided_in_big_window = m[4].value;
		EscortHotKeys = m[5].value;


#if !defined(POLY_ACC)
		if (MenuHiresAvailable)
			Automap_always_hires = m[6].value;
		else if (m[6].value)
			nm_messagebox(TXT_SORRY, 1, "OK", "High Resolution modes are\nnot available on this video card");
#endif

	} while (i > -1);
}

void do_multi_player_menu()
{
	int menu_choice[5];
	newmenu_item m[5];
	int choice = 0, num_options = 0;
	int old_game_mode;

	do
	{
		//		WIN(ipx_destroy_read_thread());

		old_game_mode = Game_mode;
		num_options = 0;

		ADD_ITEM(TXT_START_IPX_NET_GAME, MENU_START_IPX_NETGAME, -1);
		ADD_ITEM(TXT_JOIN_IPX_NET_GAME, MENU_JOIN_IPX_NETGAME, -1);
		//  ADD_ITEM(TXT_START_TCP_NET_GAME, MENU_START_TCP_NETGAME, -1 );
		//  ADD_ITEM(TXT_JOIN_TCP_NET_GAME, MENU_JOIN_TCP_NETGAME, -1 );

		ADD_ITEM(TXT_MODEM_GAME, MENU_START_SERIAL, -1);

		choice = newmenu_do1(NULL, TXT_MULTIPLAYER, num_options, m, NULL, choice);

		if (choice > -1)
			do_option(menu_choice[choice]);

		if (old_game_mode != Game_mode)
			break;          // leave menu

	} while (choice > -1);

}

void DoNewIPAddress()
{
	newmenu_item m[4];
	char IPText[30];
	int choice;

	m[0].type = NM_TYPE_TEXT; m[0].text = const_cast<char*>("Enter an address or hostname:");
	m[1].type = NM_TYPE_INPUT; m[1].text_len = 50; m[1].text = IPText;
	IPText[0] = 0;

	choice = newmenu_do(NULL, "Join a TCPIP game", 2, m, NULL);

	if (choice == -1 || m[1].text[0] == 0)
		return;

	nm_messagebox(TXT_SORRY, 1, TXT_OK, "That address is not valid!");
}
