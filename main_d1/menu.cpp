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

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>

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
#include "cfile/cfile.h"
#include "platform/joy.h"
#include "vecmat/vecmat.h"
#include "effects.h"
#include "slew.h"
#include "gamemine.h"
#include "gamesave.h"
#include "2d/palette.h"
#include "args.h"
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

#ifdef EDITOR
#include "editor\editor.h"
extern void init_cockpit(); //[ISB] I really should stuff these somewhere formal
#endif

//hack
#define EZERO 0

//char *menu_difficulty_text[] = { "Trainee", "Rookie", "Fighter", "Hotshot", "Insane" };
//char *menu_detail_text[] = { "Lowest", "Low", "Medium", "High", "Highest", "", "Custom..." };

#define MENU_NEW_GAME            0
#define MENU_GAME      				1 
#define MENU_EDITOR					2
#define MENU_VIEW_SCORES			3
#define MENU_QUIT                4
#define MENU_LOAD_GAME				5
#define MENU_SAVE_GAME				6
#define MENU_DEMO_PLAY				8
#define MENU_LOAD_LEVEL				9
#define MENU_START_NETGAME			10
#define MENU_JOIN_NETGAME			11
#define MENU_CONFIG					13
#define MENU_REJOIN_NETGAME		14
#define MENU_DIFFICULTY				15
#define MENU_START_SERIAL			18
#define MENU_HELP						19
#define MENU_NEW_PLAYER				20
#define MENU_MULTIPLAYER			21
#define MENU_STOP_MODEM				22
#define MENU_SHOW_CREDITS			23
#define MENU_ORDER_INFO				24
#define MENU_PLAY_SONG				25

//ADD_ITEM("Start netgame...", MENU_START_NETGAME, -1 );
//ADD_ITEM("Send net message...", MENU_SEND_NET_MESSAGE, -1 );

#define ADD_ITEM(t,value,key)  do { m[num_options].type=NM_TYPE_MENU; m[num_options].text=t; menu_choice[num_options]=value;num_options++; } while (0)

extern int last_joy_time;		//last time the joystick was used
#ifndef NDEBUG
extern int speedtest_on;
#else
#define speedtest_on 0
#endif

uint8_t do_auto_demo = 1;			// Flag used to enable auto demo starting in main menu.
int Player_default_difficulty; // Last difficulty level chosen by the player
int Auto_leveling_on = 0;
int Menu_draw_copyright = 0;

void autodemo_menu_check(int nitems, newmenu_item* items, int* last_key, int citem)
{
	int curtime;

	nitems = nitems;
	items = items;
	citem = citem;

	//draw copyright message
	if (Menu_draw_copyright) 
	{
		Menu_draw_copyright = 0;
		gr_set_current_canvas(NULL);
		gr_set_curfont(GAME_FONT);
		gr_set_fontcolor(BM_XRGB(6, 6, 6), -1);
		gr_printf(0x8000, grd_curcanv->cv_bitmap.bm_h - GAME_FONT->ft_h - 2, TXT_COPYRIGHT);
	}

	// Don't allow them to hit ESC in the main menu.
	if (*last_key == KEY_ESC)* last_key = 0;

	if (do_auto_demo) 
	{
		curtime = timer_get_approx_seconds();
		//if ( ((keyd_time_when_last_pressed+i2f(20)) < curtime) && ((last_joy_time+i2f(20)) < curtime) && (!speedtest_on)  ) {
		if (((keyd_time_when_last_pressed + i2f(45)) < curtime) && (!speedtest_on)) {
			keyd_time_when_last_pressed = curtime;			// Reset timer so that disk won't thrash if no demos.
			newdemo_start_playback(NULL);		// Randomly pick a file
			if (Newdemo_state == ND_STATE_PLAYBACK) 
			{
				Function_mode = FMODE_GAME;
				*last_key = -2;
			}
		}
	}
}

//static int First_time = 1;
static int main_menu_choice = 0;

//	-----------------------------------------------------------------------------
//	Create the main menu.
void create_main_menu(newmenu_item* m, int* menu_choice, int* callers_num_options)
{
	int	num_options;

#ifndef DEMO_ONLY
	num_options = 0;

	//	//	Move down to allow for space to display "Destination Saturn"
	//	if (Saturn) {
	//		int	i;
	//
	//		for (i=0; i<4; i++)
	//			ADD_ITEM("", 0, -1);
	//
	//		if (First_time) {
	//			main_menu_choice = 4;
	//			First_time = 0;
	//		}
	//	}

	ADD_ITEM(TXT_NEW_GAME, MENU_NEW_GAME, KEY_N);

#ifdef SHAREWARE
	if (get_game_list(NULL) > 0)
#endif

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
		//m[num_options].type=NM_TYPE_TEXT;
		//m[num_options++].text=" Debug options:";

		ADD_ITEM((char*)"  Load level...", MENU_LOAD_LEVEL, KEY_N);
#ifdef EDITOR
		ADD_ITEM((char*)"  Editor", MENU_EDITOR, KEY_E);
#endif
	}

	ADD_ITEM((char*)"  Play song", MENU_PLAY_SONG, -1);
#endif

	* callers_num_options = num_options;
}

//returns number of item chosen
int DoMenu()
{
	int menu_choice[25];
	newmenu_item m[25];
	int num_options = 0;

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
		keyd_time_when_last_pressed = timer_get_fixed_seconds();		// .. 20 seconds from now!
		if (main_menu_choice < 0)	main_menu_choice = 0;
		Menu_draw_copyright = 1;
		main_menu_choice = newmenu_do2("", NULL, num_options, m, autodemo_menu_check, main_menu_choice, Menu_pcx_name);
		if (main_menu_choice > -1) do_option(menu_choice[main_menu_choice]);
		create_main_menu(m, menu_choice, &num_options);	//	may have to change, eg, maybe selected pilot and no save games.
	} while (Function_mode == FMODE_MENU);

	//	if (main_menu_choice != -2)
	//		do_auto_demo = 0;		// No more auto demos
	if (Function_mode == FMODE_GAME)
		gr_palette_fade_out(gr_palette, 32, 0);

	return main_menu_choice;
}

extern void show_order_form(void);	// John didn't want this in inferno.h so I just externed it.

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
		if (newmenu_get_filename(TXT_SELECT_DEMO, "*.dem", demo_file, 1)) 
		{
			newdemo_start_playback(demo_file);
		}
	}
	break;
	case MENU_LOAD_GAME:
#ifdef SHAREWARE
		do_load_game_menu();
#else
		state_restore_all(0);
#endif
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
		RegisterPlayer();		//1 == allow escape out of menu
		break;

	case MENU_HELP:
		do_show_help();
		break;

#ifndef RELEASE

	case MENU_PLAY_SONG: 
	{
		int i;
		char* m[MAX_SONGS];

		for (i = 0; i < MAX_SONGS; i++) 
		{
			m[i] = Songs[i].filename;
		}
		i = newmenu_listbox("Select Song", MAX_SONGS, m, 1, NULL);

		if (i > -1) {
			songs_play_song(i, 0);
		}
	}
						 break;
	case MENU_LOAD_LEVEL: 
	{
		newmenu_item m;
		char text[11] = ""; //[ISB] null terminators are your friend
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


	case MENU_START_NETGAME:
#ifdef NETWORK
		//temp!
		load_mission(0);
		network_start_game();
#endif
		break;
	case MENU_JOIN_NETGAME:
		//temp!
#ifdef NETWORK
		load_mission(0);
		network_join_game();
#endif
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
		credits_show();
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

int	Max_debris_objects, Max_objects_onscreen_detailed;
int	Max_linear_depth_objects;

int8_t	Object_complexity = 2, Object_detail = 2;
int8_t	Wall_detail = 2, Wall_render_depth = 2, Debris_amount = 2, SoundChannels = 2;

int8_t	Render_depths[NUM_DETAIL_LEVELS - 1] = { 6,  9, 12, 15, 20 };
int8_t	Max_perspective_depths[NUM_DETAIL_LEVELS - 1] = { 1,  2,  3,  5,  8 };
int8_t	Max_linear_depths[NUM_DETAIL_LEVELS - 1] = { 3,  5,  7, 10, 17 };
int8_t	Max_linear_depths_objects[NUM_DETAIL_LEVELS - 1] = { 1,  2,  3,  5, 12 };
int8_t	Max_debris_objects_list[NUM_DETAIL_LEVELS - 1] = { 2,  4,  7, 10, 15 };
int8_t	Max_objects_onscreen_detailed_list[NUM_DETAIL_LEVELS - 1] = { 2,  4,  7, 10, 15 };
int8_t	Smts_list[NUM_DETAIL_LEVELS - 1] = { 2,  4,  8, 16, 50 };	//	threshold for models to go to lower detail model, gets multiplied by obj->size
int8_t	Max_sound_channels[NUM_DETAIL_LEVELS - 1] = { 2,  4,  8, 12, 16 };

//	-----------------------------------------------------------------------------
//	Set detail level based stuff.
//	Note: Highest detail level (detail_level == NUM_DETAIL_LEVELS-1) is custom detail level.
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

		//	Set custom menu defaults.
		Object_complexity = detail_level;
		Wall_render_depth = detail_level;
		Object_detail = detail_level;
		Wall_detail = detail_level;
		Debris_amount = detail_level;
		SoundChannels = detail_level;
	}
}

//	-----------------------------------------------------------------------------
void do_detail_level_menu(void)
{
	int s;
	newmenu_item m[7];

	m[0].type = NM_TYPE_MENU; m[0].text = MENU_DETAIL_TEXT(0);
	m[1].type = NM_TYPE_MENU; m[1].text = MENU_DETAIL_TEXT(1);
	m[2].type = NM_TYPE_MENU; m[2].text = MENU_DETAIL_TEXT(2);
	m[3].type = NM_TYPE_MENU; m[3].text = MENU_DETAIL_TEXT(3);
	m[4].type = NM_TYPE_MENU; m[4].text = MENU_DETAIL_TEXT(4);
	m[5].type = NM_TYPE_TEXT; m[5].text = (char*)"";
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

//	-----------------------------------------------------------------------------
void do_detail_level_menu_custom_menuset(int nitems, newmenu_item* items, int* last_key, int citem)
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

//	-----------------------------------------------------------------------------
void do_detail_level_menu_custom(void)
{
	int	s = 0;
	newmenu_item m[7];

	do 
	{
		m[0].type = NM_TYPE_SLIDER;
		m[0].text = TXT_OBJ_COMPLEXITY;
		m[0].value = Object_complexity;
		m[0].min_value = 0;
		m[0].max_value = NDL - 1;

		m[1].type = NM_TYPE_SLIDER;
		m[1].text = TXT_OBJ_DETAIL;
		m[1].value = Object_detail;
		m[1].min_value = 0;
		m[1].max_value = NDL - 1;

		m[2].type = NM_TYPE_SLIDER;
		m[2].text = TXT_WALL_DETAIL;
		m[2].value = Wall_detail;
		m[2].min_value = 0;
		m[2].max_value = NDL - 1;

		m[3].type = NM_TYPE_SLIDER;
		m[3].text = TXT_WALL_RENDER_DEPTH;
		m[3].value = Wall_render_depth;
		m[3].min_value = 0;
		m[3].max_value = NDL - 1;

		m[4].type = NM_TYPE_SLIDER;
		m[4].text = TXT_DEBRIS_AMOUNT;
		m[4].value = Debris_amount;
		m[4].min_value = 0;
		m[4].max_value = NDL - 1;

		m[5].type = NM_TYPE_SLIDER;
		m[5].text = TXT_SOUND_CHANNELS;
		m[5].value = SoundChannels;
		m[5].min_value = 0;
		m[5].max_value = NDL - 1;

		m[6].type = NM_TYPE_TEXT;
		m[6].text = TXT_LO_HI;

		s = newmenu_do1(NULL, TXT_DETAIL_CUSTOM, 7, m, do_detail_level_menu_custom_menuset, s);
	} while (s > -1);

	set_custom_detail_vars();
}

void do_new_game_menu()
{
	int n_missions, new_level_num, player_highest_level;

#ifndef SHAREWARE
	n_missions = build_mission_list(0);

	if (n_missions > 1)
	{
		int new_mission_num, i, default_mission;
		char* m[MAX_MISSIONS];

		default_mission = 0;
		for (i = 0; i < n_missions; i++) 
		{
			m[i] = Mission_list[i].mission_name;
			if (!_strfcmp(m[i], config_last_mission))
				default_mission = i;
		}

		new_mission_num = newmenu_listbox1("New Game\n\nSelect mission", n_missions, m, 1, default_mission, NULL);

		if (new_mission_num == -1)
			return;		//abort!

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
		newmenu_item m[2];
		char info_text[80];
		char num_text[10];
		int choice;

	try_again:
		sprintf(info_text, "%s %d", TXT_START_ANY_LEVEL, player_highest_level);

		m[0].type = NM_TYPE_TEXT; m[0].text = info_text;
		m[1].type = NM_TYPE_INPUT; m[1].text_len = 10; m[1].text = num_text;

		strcpy(num_text, "1");

		choice = newmenu_do(NULL, TXT_SELECT_START_LEV, 2, &m[0], NULL);

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

#ifdef PSX_BUILD_TOOLS
	{
		int i;
		for (i = Last_secret_level; i <= Last_level; i++) {
			if (i != 0)
				StartNewGame(i);
		}
	}
#endif

	StartNewGame(new_level_num);

}

void do_load_game_menu()
{
	newmenu_item m[N_SAVE_SLOTS];
	char* saved_text[N_SAVE_SLOTS];
	int i, choice;

	get_game_list(saved_text);

	for (i = 0; i < N_SAVE_SLOTS; i++) 
	{

		if (saved_text[i][0]) {
			m[i].type = NM_TYPE_MENU;
			m[i].text = saved_text[i];
		}
		else {
			m[i].type = NM_TYPE_TEXT;
			m[i].text = TXT_EMPTY;
		}
	}

	choice = newmenu_do(NULL, TXT_LOAD_GAME, N_SAVE_SLOTS, m, NULL);

	if (choice != -1) 
	{
		int ret;

		if ((ret = load_player_game(choice)) == EZERO)
			ResumeSavedGame(Players[Player_num].level);
		else 
		{
			newmenu_item m1[3];

			m1[0].type = NM_TYPE_TEXT;  m1[0].text = strerror(ret);
			m1[1].type = NM_TYPE_TEXT;  m1[1].text = (char*)"";
			m1[2].type = NM_TYPE_TEXT;  m1[2].text = TXT_ENTER_TO_CONT;

			newmenu_do(NULL, TXT_ERR_LOADING_GAME, 3, m1, NULL);

		}
	}
}

void do_save_game_menu()
{
	newmenu_item m[N_SAVE_SLOTS];
	char* saved_text_ptrs[N_SAVE_SLOTS];
	char menu_text[N_SAVE_SLOTS][GAME_NAME_LEN + 1];		//+1 for terminating zero
	int i, choice;

	get_game_list(saved_text_ptrs);

	for (i = 0; i < N_SAVE_SLOTS; i++) 
	{

		strcpy(menu_text[i], saved_text_ptrs[i]);

		m[i].type = NM_TYPE_INPUT_MENU;
		m[i].text_len = GAME_NAME_LEN;
		m[i].text = menu_text[i];

		if (!menu_text[i][0])
			strcpy(menu_text[i], TXT_EMPTY);

	}

	choice = newmenu_do(NULL, TXT_SAVE_GAME_SLOTS, N_SAVE_SLOTS, m, NULL);

	if (choice != -1) 
	{
		int ret;

		if ((ret = save_player_game(choice, m[choice].text)) != EZERO)
			nm_messagebox(NULL, 1, TXT_CONTINUE, "%s\n%s\n\n", TXT_SAVE_ERROR, strerror(ret));
	}

}

extern void GameLoop(int, int);

void joydef_menuset(int nitems, newmenu_item* items, int* last_key, int citem)
{
	nitems = nitems;
	*last_key = *last_key;

	if (citem == 4) 
	{
		gr_palette_set_gamma(items[4].value);
	}

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
}

//this change was made in DESCENT.TEX, but since we're not including that
//file in the v1.1 update, we're making the change in the code here also
#ifdef SHAREWARE
#undef	TXT_JOYS_SENSITIVITY
#define	TXT_JOYS_SENSITIVITY "Joystick/Mouse\nSensitivity"
#endif

void do_options_menu()
{
	newmenu_item m[13];
	int i = 0;

	do 
	{
		m[0].type = NM_TYPE_SLIDER; m[0].text = TXT_FX_VOLUME; m[0].value = Config_digi_volume; m[0].min_value = 0; m[0].max_value = 8;
		m[1].type = NM_TYPE_SLIDER; m[1].text = TXT_MUSIC_VOLUME; m[1].value = Config_midi_volume; m[1].min_value = 0; m[1].max_value = 8;
		m[2].type = NM_TYPE_CHECK; m[2].text = TXT_REVERSE_STEREO; m[2].value = Config_channels_reversed;
		m[3].type = NM_TYPE_TEXT; m[3].text = (char*)"";
		m[4].type = NM_TYPE_SLIDER; m[4].text = TXT_BRIGHTNESS; m[4].value = gr_palette_get_gamma(); m[4].min_value = 0; m[4].max_value = 8;
		m[5].type = NM_TYPE_TEXT; m[5].text = (char*)"";
		m[6].type = NM_TYPE_MENU; m[6].text = TXT_CONTROLS_;
		m[7].type = NM_TYPE_MENU; m[7].text = TXT_DETAIL_LEVELS;
		m[8].type = NM_TYPE_MENU; m[8].text = TXT_CAL_JOYSTICK;
		m[9].type = NM_TYPE_TEXT; m[9].text = (char*)"";
		m[10].type = NM_TYPE_SLIDER; m[10].text = TXT_JOYS_SENSITIVITY; m[10].value = Config_joystick_sensitivity; m[10].min_value = 0; m[10].max_value = 8;
		m[11].type = NM_TYPE_TEXT; m[11].text = (char*)"";
		m[12].type = NM_TYPE_CHECK; m[12].text = (char*)"Ship auto-leveling"; m[12].value = Auto_leveling_on;

		i = newmenu_do1(NULL, TXT_OPTIONS, 13, m, joydef_menuset, i);

		switch (i) 
		{
		case 6: joydefs_config(); 			break;
		case 7: do_detail_level_menu();	break;
		case 8: joydefs_calibrate();		break;
		}

		Config_channels_reversed = m[2].value;
		Config_joystick_sensitivity = m[10].value;
		Auto_leveling_on = m[12].value;
	} while (i > -1);

	if (Config_midi_volume < 1)
	{
		digi_play_midi_song(NULL, NULL, NULL, 0);
	}

	write_player_file();
}

void do_multi_player_menu()
{
	int menu_choice[3];
	newmenu_item m[3];
	int choice = 0, num_options = 0;
	int old_game_mode;

	do 
	{
		old_game_mode = Game_mode;
		num_options = 0;

		ADD_ITEM(TXT_START_NET_GAME, MENU_START_NETGAME, -1);
		ADD_ITEM(TXT_JOIN_NET_GAME, MENU_JOIN_NETGAME, -1);
		ADD_ITEM(TXT_MODEM_GAME, MENU_START_SERIAL, -1);

		choice = newmenu_do1(NULL, TXT_MULTIPLAYER, num_options, m, NULL, choice);

		if (choice > -1)
			do_option(menu_choice[choice]);

		if (old_game_mode != Game_mode)
			break;		// leave menu

	} while (choice > -1);
}
