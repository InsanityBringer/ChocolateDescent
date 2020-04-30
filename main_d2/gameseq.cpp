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

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>

#include "misc/rand.h"

//#include "pa_enabl.h"                   //$$POLY_ACC
#include "2d/i_gr.h"
#include "inferno.h"
#include "game.h"
#include "player.h"
#include "platform/key.h"
#include "object.h"
#include "physics.h"
#include "misc/error.h"
#include "platform/joy.h"
#include "platform/mono.h"
#include "iff/iff.h"
#include "2d/pcx.h"
#include "platform/timer.h"
#include "render.h"
#include "laser.h"
#include "screens.h"
#include "textures.h"
#include "slew.h"
#include "gauges.h"
#include "texmap/texmap.h"
#include "3d/3d.h"
#include "effects.h"
#include "2d/effect2d.h"
#include "menu.h"
#include "gameseg.h"
#include "wall.h"
#include "ai.h"
#include "fuelcen.h"
#include "switch.h"
#include "digi.h"
#include "gamesave.h"
#include "scores.h"
#include "2d/ibitblt.h"
#include "mem/mem.h"
#include "2d/palette.h"
#include "morph.h"
#include "lighting.h"
#include "newdemo.h"
#include "titles.h"
#include "collide.h"
#include "weapon.h"
#include "sounds.h"
#include "misc/args.h" //[ISB] WHY DO YOU MAKE BAD DECISIONS
#include "gameseq.h"
#include "gamefont.h"
#include "newmenu.h"
#include "endlevel.h"
#include "network.h"
#include "arcade.h"
#include "playsave.h"
#include "ctype.h"
#include "multi.h"
#include "fireball.h"
#include "kconfig.h"
#include "config.h"
#include "robot.h"
#include "automap.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "modem.h"
#include "text.h"
#include "cfile/cfile.h"
#include "piggy.h"
#include "texmerge.h"
#include "paging.h"
#include "mission.h"
#include "state.h"
#include "songs.h"
#include "netmisc.h"
#include "gamepal.h"
#include "movie.h"
#include "controls.h"
#include "credits.h"

#if defined(POLY_ACC)
#include "poly_acc.h"
#endif
#if defined (TACTILE)
#include "tactile.h"
#endif

#ifdef EDITOR
#include "editor\editor.h"
#endif

// From allender -- you'll find these defines in state.c and cntrlcen.c
// since I couldn't think of a good place to put them and i wanted to
// fix this stuff fast!  Sorry about that...

#ifndef MACINTOSH
#define SECRETB_FILENAME	"secret.sgb"
#define SECRETC_FILENAME	"secret.sgc"
#else
#define SECRETB_FILENAME	":Players:secret.sgb"
#define SECRETC_FILENAME	":Players:secret.sgc"
#endif

//Current_level_num starts at 1 for the first level
//-1,-2,-3 are secret levels
//0 means not a real level loaded
int	Current_level_num = 0, Next_level_num;
char	Current_level_name[LEVEL_NAME_LEN];

#if !defined(SHAREWARE) && !defined(D2_OEM)
int Last_level, Last_secret_level;
#endif

// Global variables describing the player
int 				N_players = 1;						// Number of players ( >1 means a net game, eh?)
int 				Player_num = 0;						// The player number who is on the console.
player                  Players[MAX_PLAYERS + 4];                   // Misc player info
obj_position	Player_init[MAX_PLAYERS];

// Global variables telling what sort of game we have
int MaxNumNetPlayers = -1;
int NumNetPlayerPositions = -1;

extern fix ThisLevelTime;

// Extern from game.c to fix a bug in the cockpit!

extern int last_drawn_cockpit[2];
extern int Last_level_path_created;

//	HUD_clear_messages external, declared in gauges.h
#ifndef _GAUGES_H
extern void HUD_clear_messages(); // From hud.c
#endif

//	Extra prototypes declared for the sake of LINT
void init_player_stats_new_ship(void);
void copy_defaults_to_robot_all(void);

int	Do_appearance_effect = 0;

extern int Rear_view;

int	First_secret_visit = 1;

extern int descent_critical_error;

extern int Last_msg_ycrd;

//--------------------------------------------------------------------
void verify_console_object()
{
	Assert(Player_num > -1);
	Assert(Players[Player_num].objnum > -1);
	ConsoleObject = &Objects[Players[Player_num].objnum];
	Assert(ConsoleObject->type == OBJ_PLAYER);
	Assert(ConsoleObject->id == Player_num);
}

int count_number_of_robots()
{
	int robot_count;
	int i;

	robot_count = 0;
	for (i = 0; i <= Highest_object_index; i++) {
		if (Objects[i].type == OBJ_ROBOT)
			robot_count++;
	}

	return robot_count;
}


int count_number_of_hostages()
{
	int count;
	int i;

	count = 0;
	for (i = 0; i <= Highest_object_index; i++) {
		if (Objects[i].type == OBJ_HOSTAGE)
			count++;
	}

	return count;
}

//added 10/12/95: delete buddy bot if coop game.  Probably doesn't really belong here. -MT
void gameseq_init_network_players()
{
	int i, k, j;

	// Initialize network player start locations and object numbers

	ConsoleObject = &Objects[0];
	k = 0;
	j = 0;
	for (i = 0; i <= Highest_object_index; i++)
	{
		if ((Objects[i].type == OBJ_PLAYER) || (Objects[i].type == OBJ_GHOST) || (Objects[i].type == OBJ_COOP))
		{
			if ((!(Game_mode & GM_MULTI_COOP) && ((Objects[i].type == OBJ_PLAYER) || (Objects[i].type == OBJ_GHOST))) ||
				((Game_mode & GM_MULTI_COOP) && ((j == 0) || (Objects[i].type == OBJ_COOP))))
			{
				// -- mprintf((0, "Created Cooperative multiplayer object\n"));
				Objects[i].type = OBJ_PLAYER;
				// -- mprintf((0, "Player init %d is ship %d.\n", k, j));
				Player_init[k].pos = Objects[i].pos;
				Player_init[k].orient = Objects[i].orient;
				Player_init[k].segnum = Objects[i].segnum;
				Players[k].objnum = i;
				Objects[i].id = k;
				k++;
			}
			else
				obj_delete(i);
			j++;
		}

		if ((Objects[i].type == OBJ_ROBOT) && (Robot_info[Objects[i].id].companion) && (Game_mode & GM_MULTI))
			obj_delete(i);		//kill the buddy in netgames

	}
	NumNetPlayerPositions = k;

#ifndef NDEBUG
	if (((Game_mode & GM_MULTI_COOP) && (NumNetPlayerPositions != 4)) ||
		(!(Game_mode & GM_MULTI_COOP) && (NumNetPlayerPositions != 8)))
	{
		mprintf((1, "--NOT ENOUGH MULTIPLAYER POSITIONS IN THIS MINE!--\n"));
		//Int3(); // Not enough positions!!
	}
#endif
#if defined (D2_OEM)

	if ((Game_mode & GM_MULTI) && Current_mission_num == 0 && Current_level_num == 8)
	{
		for (i = 0; i < N_players; i++)
			if (Players[i].connected && !(NetPlayers.players[i].version_minor & 0xF0))
			{
				nm_messagebox("Warning!", 1, TXT_OK, "This special version of Descent II\nwill disconnect after this level.\nPlease purchase the full version\nto experience all the levels!");
				return;
			}
	}
#endif
}

void gameseq_remove_unused_players()
{
	int i;

	// 'Remove' the unused players
#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
		for (i = 0; i < NumNetPlayerPositions; i++)
		{
			if ((!Players[i].connected) || (i >= N_players))
			{
#ifndef NDEBUG
				//				mprintf((0, "Ghosting player ship %d.\n", i+1));
#endif
				multi_make_player_ghost(i);
			}
		}
	}
	else
#endif
	{		// Note link to above if!!!
#ifndef NDEBUG
// -- mprintf((0, "Removing player objects numbered %d-%d.\n", 1, NumNetPlayerPositions));
#endif
		for (i = 1; i < NumNetPlayerPositions; i++)
		{
			obj_delete(Players[i].objnum);
		}
	}
}

fix StartingShields = INITIAL_SHIELDS;

// Setup player for new game
void init_player_stats_game()
{
	Players[Player_num].score = 0;
	Players[Player_num].last_score = 0;
	Players[Player_num].lives = INITIAL_LIVES;
	Players[Player_num].level = 1;

	Players[Player_num].time_level = 0;
	Players[Player_num].time_total = 0;
	Players[Player_num].hours_level = 0;
	Players[Player_num].hours_total = 0;

	Players[Player_num].energy = INITIAL_ENERGY;
	Players[Player_num].shields = StartingShields;
	Players[Player_num].killer_objnum = -1;

	Players[Player_num].net_killed_total = 0;
	Players[Player_num].net_kills_total = 0;

	Players[Player_num].num_kills_level = 0;
	Players[Player_num].num_kills_total = 0;
	Players[Player_num].num_robots_level = 0;
	Players[Player_num].num_robots_total = 0;
	Players[Player_num].KillGoalCount = 0;

	Players[Player_num].hostages_rescued_total = 0;
	Players[Player_num].hostages_level = 0;
	Players[Player_num].hostages_total = 0;

	Players[Player_num].laser_level = 0;
	Players[Player_num].flags = 0;

	init_player_stats_new_ship();

	First_secret_visit = 1;
}

void init_ammo_and_energy(void)
{
	if (Players[Player_num].energy < INITIAL_ENERGY)
		Players[Player_num].energy = INITIAL_ENERGY;
	if (Players[Player_num].shields < StartingShields)
		Players[Player_num].shields = StartingShields;

	//	for (i=0; i<MAX_PRIMARY_WEAPONS; i++)
	//		if (Players[Player_num].primary_ammo[i] < Default_primary_ammo_level[i])
	//			Players[Player_num].primary_ammo[i] = Default_primary_ammo_level[i];

	//	for (i=0; i<MAX_SECONDARY_WEAPONS; i++)
	//		if (Players[Player_num].secondary_ammo[i] < Default_secondary_ammo_level[i])
	//			Players[Player_num].secondary_ammo[i] = Default_secondary_ammo_level[i];
	if (Players[Player_num].secondary_ammo[0] < 2 + NDL - Difficulty_level)
		Players[Player_num].secondary_ammo[0] = 2 + NDL - Difficulty_level;
}

extern	uint8_t	Last_afterburner_state;

// Setup player for new level (After completion of previous level)
void init_player_stats_level(int secret_flag)
{
	// int	i;

	Players[Player_num].last_score = Players[Player_num].score;

	Players[Player_num].level = Current_level_num;

#ifdef NETWORK
	if (!Network_rejoined)
#endif
	{
		Players[Player_num].time_level = 0;
		Players[Player_num].hours_level = 0;
	}

	Players[Player_num].killer_objnum = -1;

	Players[Player_num].num_kills_level = 0;
	Players[Player_num].num_robots_level = count_number_of_robots();
	Players[Player_num].num_robots_total += Players[Player_num].num_robots_level;

	Players[Player_num].hostages_level = count_number_of_hostages();
	Players[Player_num].hostages_total += Players[Player_num].hostages_level;
	Players[Player_num].hostages_on_board = 0;

	if (!secret_flag)
	{
		init_ammo_and_energy();

		Players[Player_num].flags &= (~KEY_BLUE);
		Players[Player_num].flags &= (~KEY_RED);
		Players[Player_num].flags &= (~KEY_GOLD);

		Players[Player_num].flags &= ~(PLAYER_FLAGS_INVULNERABLE |
			PLAYER_FLAGS_CLOAKED |
			PLAYER_FLAGS_MAP_ALL);

		Players[Player_num].cloak_time = 0;
		Players[Player_num].invulnerable_time = 0;

		if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
			Players[Player_num].flags |= (KEY_BLUE | KEY_RED | KEY_GOLD);
	}

	Player_is_dead = 0; // Added by RH
	Players[Player_num].homing_object_dist = -F1_0; // Added by RH

	Last_laser_fired_time = Next_laser_fire_time = GameTime; // added by RH, solved demo playback bug

	Controls.afterburner_state = 0;
	Last_afterburner_state = 0;

	digi_kill_sound_linked_to_object(Players[Player_num].objnum);

	init_gauges();

#ifdef TACTILE
	if (TactileStick)
		tactile_set_button_jolt();
#endif

	Missile_viewer = NULL;
}

extern	void init_ai_for_ship(void);

// Setup player for a brand-new ship
void init_player_stats_new_ship()
{
	int	i;

	if (Newdemo_state == ND_STATE_RECORDING)
	{
		newdemo_record_laser_level(Players[Player_num].laser_level, 0);
		newdemo_record_player_weapon(0, 0);
		newdemo_record_player_weapon(1, 0);
	}

	Players[Player_num].energy = INITIAL_ENERGY;
	Players[Player_num].shields = StartingShields;
	Players[Player_num].laser_level = 0;
	Players[Player_num].killer_objnum = -1;
	Players[Player_num].hostages_on_board = 0;

	Afterburner_charge = 0;

	for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
	{
		Players[Player_num].primary_ammo[i] = 0;
		Primary_last_was_super[i] = 0;
	}

	for (i = 1; i < MAX_SECONDARY_WEAPONS; i++)
	{
		Players[Player_num].secondary_ammo[i] = 0;
		Secondary_last_was_super[i] = 0;
	}
	Players[Player_num].secondary_ammo[0] = 2 + NDL - Difficulty_level;

	Players[Player_num].primary_weapon_flags = HAS_LASER_FLAG;
	Players[Player_num].secondary_weapon_flags = HAS_CONCUSSION_FLAG;

	Primary_weapon = 0;
	Secondary_weapon = 0;

	Players[Player_num].flags &= ~(PLAYER_FLAGS_QUAD_LASERS |
		PLAYER_FLAGS_AFTERBURNER |
		PLAYER_FLAGS_CLOAKED |
		PLAYER_FLAGS_INVULNERABLE |
		PLAYER_FLAGS_MAP_ALL |
		PLAYER_FLAGS_CONVERTER |
		PLAYER_FLAGS_AMMO_RACK |
		PLAYER_FLAGS_HEADLIGHT |
		PLAYER_FLAGS_HEADLIGHT_ON |
		PLAYER_FLAGS_FLAG);

	Players[Player_num].cloak_time = 0;
	Players[Player_num].invulnerable_time = 0;

	Player_is_dead = 0;		//player no longer dead

	Players[Player_num].homing_object_dist = -F1_0; // Added by RH

	Controls.afterburner_state = 0;
	Last_afterburner_state = 0;

	digi_kill_sound_linked_to_object(Players[Player_num].objnum);

	Missile_viewer = NULL;		///reset missile camera if out there

#ifdef TACTILE
	if (TactileStick)
	{
		tactile_set_button_jolt();
	}
#endif


	init_ai_for_ship();
}

void reset_network_objects()
{
#ifdef NETWORK
	memset(local_to_remote, -1, MAX_OBJECTS * sizeof(short));
	memset(remote_to_local, -1, MAX_NUM_NET_PLAYERS * MAX_OBJECTS * sizeof(short));
	memset(object_owner, -1, MAX_OBJECTS);
#endif
}

extern void init_stuck_objects(void);

#ifdef EDITOR

extern int Slide_segs_computed;

//reset stuff so game is semi-normal when playing from editor
void editor_reset_stuff_on_level()
{
	gameseq_init_network_players();
	init_player_stats_level(0);
	Viewer = ConsoleObject;
	ConsoleObject = Viewer = &Objects[Players[Player_num].objnum];
	ConsoleObject->id = Player_num;
	ConsoleObject->control_type = CT_FLYING;
	ConsoleObject->movement_type = MT_PHYSICS;
	Game_suspended = 0;
	verify_console_object();
	Control_center_destroyed = 0;
	if (Newdemo_state != ND_STATE_PLAYBACK)
		gameseq_remove_unused_players();
	init_cockpit();
	init_robots_for_level();
	init_ai_objects();
	init_morphs();
	init_all_matcens();
	init_player_stats_new_ship();
	init_controlcen_for_level();
	automap_clear_visited();
	init_stuck_objects();
	init_thief_for_level();

	Slide_segs_computed = 0;
}
#endif

void reset_player_object();

//do whatever needs to be done when a player dies in multiplayer

void DoGameOver()
{
	//	nm_messagebox( TXT_GAME_OVER, 1, TXT_OK, "" );

	if (Current_mission_num == 0)
		scores_maybe_add_player(0);

	Function_mode = FMODE_MENU;
	Game_mode = GM_GAME_OVER;
	longjmp(LeaveGame, 0);		// Exit out of game loop

}

extern int do_save_game_menu();

//update various information about the player
void update_player_stats()
{
	Players[Player_num].time_level += FrameTime;	//the never-ending march of time...
	if (Players[Player_num].time_level > i2f(3600))
	{
		Players[Player_num].time_level -= i2f(3600);
		Players[Player_num].hours_level++;
	}

	Players[Player_num].time_total += FrameTime;	//the never-ending march of time...
	if (Players[Player_num].time_total > i2f(3600))
	{
		Players[Player_num].time_total -= i2f(3600);
		Players[Player_num].hours_total++;
	}
}

//hack to not start object when loading level
extern int Dont_start_sound_objects;

//go through this level and start any eclip sounds
void set_sound_sources()
{
	int segnum, sidenum;
	segment* seg;

	digi_init_sounds();		//clear old sounds

	Dont_start_sound_objects = 1;

	for (seg = &Segments[0], segnum = 0; segnum <= Highest_segment_index; seg++, segnum++)
		for (sidenum = 0; sidenum < MAX_SIDES_PER_SEGMENT; sidenum++)
		{
			int tm, ec, sn;

			if (WALL_IS_DOORWAY(seg, sidenum) & WID_RENDER_FLAG)
				if ((((tm = seg->sides[sidenum].tmap_num2) != 0) && ((ec = TmapInfo[tm & 0x3fff].eclip_num) != -1)) || ((ec = TmapInfo[seg->sides[sidenum].tmap_num].eclip_num) != -1))
					if ((sn = Effects[ec].sound_num) != -1)
					{
						vms_vector pnt;
						int csegnum = seg->children[sidenum];

						//check for sound on other side of wall.  Don't add on
						//both walls if sound travels through wall.  If sound
						//does travel through wall, add sound for lower-numbered
						//segment.

						if (IS_CHILD(csegnum) && csegnum < segnum)
						{
							if (WALL_IS_DOORWAY(seg, sidenum) & (WID_FLY_FLAG + WID_RENDPAST_FLAG))
							{
								segment* csegp;
								int csidenum;

								csegp = &Segments[seg->children[sidenum]];
								csidenum = find_connect_side(seg, csegp);

								if (csegp->sides[csidenum].tmap_num2 == seg->sides[sidenum].tmap_num2)
									continue;		//skip this one
							}
						}

						compute_center_point_on_side(&pnt, seg, sidenum);
						digi_link_sound_to_pos(sn, segnum, sidenum, &pnt, 1, F1_0 / 2);
					}
		}

	Dont_start_sound_objects = 0;

}


//fix flash_dist=i2f(1);
fix flash_dist = fl2f(.9);

//create flash for player appearance
void create_player_appearance_effect(object* player_obj)
{
	vms_vector pos;
	object* effect_obj;

#ifndef NDEBUG
	{
		int objnum = player_obj - Objects;
		if ((objnum < 0) || (objnum > Highest_object_index))
			Int3(); // See Rob, trying to track down weird network bug
	}
#endif

	if (player_obj == Viewer)
		vm_vec_scale_add(&pos, &player_obj->pos, &player_obj->orient.fvec, fixmul(player_obj->size, flash_dist));
	else
		pos = player_obj->pos;

	effect_obj = object_create_explosion(player_obj->segnum, &pos, player_obj->size, VCLIP_PLAYER_APPEARANCE);

	if (effect_obj)
	{
		effect_obj->orient = player_obj->orient;

		if (Vclip[VCLIP_PLAYER_APPEARANCE].sound_num > -1)
			digi_link_sound_to_object(Vclip[VCLIP_PLAYER_APPEARANCE].sound_num, effect_obj - Objects, 0, F1_0);
	}
}

//
// New Game sequencing functions
//

//pairs of chars describing ranges
char playername_allowed_chars[] = "azAZ09__--";

int MakeNewPlayerFile(int allow_abort)
{
	int x;
	char filename[14];
	newmenu_item m;
	char text[CALLSIGN_LEN + 1] = "";
	FILE* fp;

	strncpy(text, Players[Player_num].callsign, CALLSIGN_LEN);

try_again:
	m.type = NM_TYPE_INPUT; m.text_len = 8; m.text = text;

	Newmenu_allowed_chars = playername_allowed_chars;
	x = newmenu_do(NULL, TXT_ENTER_PILOT_NAME, 1, &m, NULL);
	Newmenu_allowed_chars = NULL;

	if (x < 0)
	{
		if (allow_abort) return 0;
		goto try_again;
	}

	if (text[0] == 0)	//null string
		goto try_again;

	sprintf(filename, "%s.plr", text);

	fp = fopen(filename, "rb");

#ifndef MACINTOSH
	//if the callsign is the name of a tty device, prepend a char
	/*if (fp && isatty(fileno(fp))) //[ISB] I really need to figure out something here tbh
	{
		fclose(fp);
		sprintf(filename,"$%.7s.plr",text);
		fp = fopen(filename,"rb");
	}*/
#endif

	if (fp)
	{
		nm_messagebox(NULL, 1, TXT_OK, "%s '%s' %s", TXT_PLAYER, text, TXT_ALREADY_EXISTS);
		fclose(fp);
		goto try_again;
	}

	if (!new_player_config())
		goto try_again;			// They hit Esc during New player config

	strncpy(Players[Player_num].callsign, text, CALLSIGN_LEN);

	write_player_file();

	return 1;
}

#ifdef WINDOWS
#undef TXT_SELECT_PILOT
#define TXT_SELECT_PILOT "Select pilot\n<Ctrl-D> or Right-click\nto delete"
#endif

//Inputs the player's name, without putting up the background screen
int RegisterPlayer()
{
	int i, j;
	char filename[14];
	int allow_abort_flag = 1;

	if (Players[Player_num].callsign[0] == 0)
	{
		//---------------------------------------------------------------------
		// Set default config options in case there is no config file
		// kc_keyboard, kc_joystick, kc_mouse are statically defined.
		Config_joystick_sensitivity = 8;
		Config_control_type = CONTROL_NONE;
		for (i = 0; i < CONTROL_MAX_TYPES; i++)
			for (j = 0; j < MAX_CONTROLS; j++)
				kconfig_settings[i][j] = default_kconfig_settings[i][j];
		kc_set_controls();
		//----------------------------------------------------------------

		// Read the last player's name from config file, not lastplr.txt
		strncpy(Players[Player_num].callsign, config_last_player, CALLSIGN_LEN);

		if (config_last_player[0] == 0)
			allow_abort_flag = 0;
	}

do_menu_again:
	;

#ifndef MACINTOSH
	if (!newmenu_get_filename(TXT_SELECT_PILOT, "*.plr", filename, allow_abort_flag)) {
		goto do_menu_again; //return 0;		// They hit Esc in file selector
	}
#else
#ifndef APPLE_DEMO
	if (!newmenu_get_filename(TXT_SELECT_PILOT, ".\\Players\\*.plr", filename, allow_abort_flag)) {
		goto do_menu_again;		// They hit Esc in file selector
	}
#else
	newmenu_get_filename("Select Pilot", ".\\Players\\*.plr", filename, 0);		// no abort allowed ever -- and change title of menubox
#endif
#endif

	if (filename[0] == '<')
	{
		// They selected 'create new pilot'
		if (!MakeNewPlayerFile(allow_abort_flag))
			//return 0;		// They hit Esc during enter name stage
			goto do_menu_again;
	}
	else
	{
		strncpy(Players[Player_num].callsign, filename, CALLSIGN_LEN);
	}

	if (read_player_file() != EZERO)
		goto do_menu_again;

	Auto_leveling_on = Default_leveling_on;

	set_display_mode(Default_display_mode);

	WriteConfigFile();		// Update lastplr

	return 1;
}

extern void change_filename_extension(char* dest, const char* src, const char* new_ext);
extern char last_palette_loaded_pig[];

uint8_t* Bitmap_replacement_data = NULL;

typedef struct DiskBitmapHeader
{
	char name[8];
	uint8_t dflags;                   //bits 0-5 anim frame num, bit 6 abm flag
	uint8_t width;                    //low 8 bits here, 4 more bits in wh_extra
	uint8_t height;                   //low 8 bits here, 4 more bits in wh_extra
	uint8_t   wh_extra;               //bits 0-3 width, bits 4-7 height
	uint8_t flags;
	uint8_t avg_color;
	int offset;
} DiskBitmapHeader;

//from piggy code, needed to properly figure out how much memory to allocate
#define BITMAP_HEADER_SIZE 18

void load_bitmap_replacements(char* level_name)
{
	char ifile_name[FILENAME_LEN];
	CFILE* ifile;
	int i;

	//first, free up data allocated for old bitmaps
	if (Bitmap_replacement_data)
	{
		free(Bitmap_replacement_data);
		Bitmap_replacement_data = NULL;
	}

	change_filename_extension(ifile_name, level_name, ".POG");

	ifile = cfopen(ifile_name, "rb");

	if (ifile) {
		int id, version, n_bitmaps;
		int bitmap_data_size;
		uint16_t* indices;

		id = cfile_read_int(ifile);
		version = cfile_read_int(ifile);

		if (id != 'GOPD' || version != 1) {
			cfclose(ifile);
			return;
		}

		n_bitmaps = cfile_read_int(ifile);

		MALLOC(indices, uint16_t, n_bitmaps);

		for (i = 0; i < n_bitmaps; i++)
		{
			indices[i] = cfile_read_short(ifile);
		}

		bitmap_data_size = cfilelength(ifile) - cftell(ifile) - (BITMAP_HEADER_SIZE * n_bitmaps);
		MALLOC(Bitmap_replacement_data, uint8_t, bitmap_data_size);

		for (i = 0; i < n_bitmaps; i++)
		{
			DiskBitmapHeader bmh;
			grs_bitmap temp_bitmap;

			//note the groovy mac-compatible code!
			cfread(bmh.name, 8, 1, ifile);
			bmh.dflags = cfile_read_byte(ifile);
			bmh.width = cfile_read_byte(ifile);
			bmh.height = cfile_read_byte(ifile);
			bmh.wh_extra = cfile_read_byte(ifile);
			bmh.flags = cfile_read_byte(ifile);
			bmh.avg_color = cfile_read_byte(ifile);
			bmh.offset = cfile_read_int(ifile);

			memset(&temp_bitmap, 0, sizeof(grs_bitmap));

			temp_bitmap.bm_w = temp_bitmap.bm_rowsize = bmh.width + ((short)(bmh.wh_extra & 0x0f) << 8);
			temp_bitmap.bm_h = bmh.height + ((short)(bmh.wh_extra & 0xf0) << 4);
			temp_bitmap.avg_color = bmh.avg_color;
			temp_bitmap.bm_data = Bitmap_replacement_data + bmh.offset;

			if (bmh.flags & BM_FLAG_TRANSPARENT) temp_bitmap.bm_flags |= BM_FLAG_TRANSPARENT;
			if (bmh.flags & BM_FLAG_SUPER_TRANSPARENT) temp_bitmap.bm_flags |= BM_FLAG_SUPER_TRANSPARENT;
			if (bmh.flags & BM_FLAG_NO_LIGHTING) temp_bitmap.bm_flags |= BM_FLAG_NO_LIGHTING;
			if (bmh.flags & BM_FLAG_RLE) temp_bitmap.bm_flags |= BM_FLAG_RLE;
			if (bmh.flags & BM_FLAG_RLE_BIG) temp_bitmap.bm_flags |= BM_FLAG_RLE_BIG;

			GameBitmaps[indices[i]] = temp_bitmap;
		}

		cfread(Bitmap_replacement_data, 1, bitmap_data_size, ifile);

		free(indices);

		cfclose(ifile);

		last_palette_loaded_pig[0] = 0;	//force pig re-load

		texmerge_flush();		//for re-merging with new textures
	}
}

void load_robot_replacements(char* level_name);
int read_hamfile();
extern int Robot_replacements_loaded;

//load a level off disk. level numbers start at 1.  Secret levels are -1,-2,-3
void LoadLevel(int level_num, int page_in_textures)
{
	char* level_name;
	player save_player;
	int load_ret;

	save_player = Players[Player_num];

	Assert(level_num <= Last_level && level_num >= Last_secret_level && level_num != 0);

	if (level_num < 0)		//secret level
		level_name = Secret_level_names[-level_num - 1];
	else					//normal level
		level_name = Level_names[level_num - 1];

#ifdef WINDOWS
	dd_gr_set_current_canvas(NULL);
	dd_gr_clear_canvas(BM_XRGB(0, 0, 0));
#else
	gr_set_current_canvas(NULL);
	gr_clear_canvas(BM_XRGB(0, 0, 0));		//so palette switching is less obvious
#endif

	Last_msg_ycrd = -1;		//so we don't restore backgound under msg

//	WIN(LoadCursorWin(MOUSE_WAIT_CURSOR));
//	WIN(ShowCursorW());

#if defined(POLY_ACC)
	gr_palette_load(gr_palette);
	show_boxed_message(TXT_LOADING);
#else
	show_boxed_message(TXT_LOADING);
	gr_palette_load(gr_palette);
	I_DrawCurrentCanvas(0);
	
#endif

	load_ret = load_level(level_name);		//actually load the data from disk!

	if (load_ret)
		Error("Couldn't load level file <%s>, error = %d", level_name, load_ret);

	Current_level_num = level_num;

	//	load_palette_pig(Current_level_palette);		//load just the pig

	load_palette(Current_level_palette, 1, 1);		//don't change screen

#ifdef SHAREWARE
	load_endlevel_data(level_num);
#endif

	if (page_in_textures)
		piggy_load_level_data();

	load_bitmap_replacements(level_name);

	if (Robot_replacements_loaded) {
		read_hamfile();		//load original data
		Robot_replacements_loaded = 0;
	}
	load_robot_replacements(level_name);

#ifdef NETWORK
	my_segments_checksum = netmisc_calc_checksum(Segments, sizeof(segment) * (Highest_segment_index + 1));
#endif

	reset_network_objects();

	Players[Player_num] = save_player;

	set_sound_sources();

	songs_play_level_song(Current_level_num);

	clear_boxed_message();		//remove message before new palette loaded

	gr_palette_load(gr_palette);		//actually load the palette

//	WIN(HideCursorW());
}

//sets up Player_num & ConsoleObject  
void InitPlayerObject()
{
	Assert(Player_num >= 0 && Player_num < MAX_PLAYERS);

	if (Player_num != 0)
	{
		Players[0] = Players[Player_num];
		Player_num = 0;
	}

	Players[Player_num].objnum = 0;

	ConsoleObject = &Objects[Players[Player_num].objnum];

	ConsoleObject->type = OBJ_PLAYER;
	ConsoleObject->id = Player_num;
	ConsoleObject->control_type = CT_FLYING;
	ConsoleObject->movement_type = MT_PHYSICS;
}

extern void game_disable_cheats();
extern void turn_cheats_off();
extern void init_seismic_disturbances(void);
void StartNewLevelSecret(int level_num, int page_in_textures);

//starts a new game on the given level
void StartNewGame(int start_level)
{
	Game_mode = GM_NORMAL;
	Function_mode = FMODE_GAME;

	Next_level_num = 0;

	InitPlayerObject();				//make sure player's object set up

	init_player_stats_game();		//clear all stats

	N_players = 1;
#ifdef NETWORK
	Network_new_game = 0;
#endif

	if (start_level < 0)
		StartNewLevelSecret(start_level, 0);
	else
		StartNewLevel(start_level, 0);

	Players[Player_num].starting_level = start_level;		// Mark where they started

	game_disable_cheats();

	init_seismic_disturbances();
}

//@@//starts a resumed game loaded from disk
//@@void ResumeSavedGame(int start_level)
//@@{
//@@	Game_mode = GM_NORMAL;
//@@	Function_mode = FMODE_GAME;
//@@
//@@	N_players = 1;
//@@	Network_new_game = 0;
//@@
//@@	InitPlayerObject();				//make sure player's object set up
//@@
//@@	StartNewLevel(start_level, 0);
//@@
//@@	game_disable_cheats();
//@@}

#ifndef _NETWORK_H
extern int network_endlevel_poll2(int nitems, newmenu_item* menus, int* key, int citem); // network.c
#endif

extern int N_secret_levels;

#ifdef RELEASE
#define STARS_BACKGROUND (MenuHires?"\x01starsb.pcx":"\x01stars.pcx")
#else
#define STARS_BACKGROUND (MenuHires?"starsb.pcx":"stars.pcx")
#endif

//	-----------------------------------------------------------------------------
//	Does the bonus scoring.
//	Call with dead_flag = 1 if player died, but deserves some portion of bonus (only skill points), anyway.
void DoEndLevelScoreGlitz(int network)
{
	int level_points, skill_points, energy_points, shield_points, hostage_points;
	int	all_hostage_points;
	int	endgame_points;
	char	all_hostage_text[64];
	char	endgame_text[64];
#define N_GLITZITEMS 11
	char				m_str[N_GLITZITEMS][30];
	newmenu_item	m[N_GLITZITEMS + 1];
	int				i, c;
	char				title[128];
	int				is_last_level;
	int				mine_level;

	set_screen_mode(SCREEN_MENU);		//go into menu mode

#ifdef TACTILE
	if (TactileStick)
		ClearForces();
#endif

	mprintf((0, "DoEndLevelScoreGlitz\n"));

	//	Compute level player is on, deal with secret levels (negative numbers)
	mine_level = Players[Player_num].level;
	if (mine_level < 0)
		mine_level *= -(Last_level / N_secret_levels);

	level_points = Players[Player_num].score - Players[Player_num].last_score;

	if (!Cheats_enabled)
	{
		if (Difficulty_level > 1)
		{
			skill_points = level_points * (Difficulty_level) / 4;
			skill_points -= skill_points % 100;
		}
		else
			skill_points = 0;

		shield_points = f2i(Players[Player_num].shields) * 5 * mine_level;
		energy_points = f2i(Players[Player_num].energy) * 2 * mine_level;
		hostage_points = Players[Player_num].hostages_on_board * 500 * (Difficulty_level + 1);

		shield_points -= shield_points % 50;
		energy_points -= energy_points % 50;
	}
	else
	{
		skill_points = 0;
		shield_points = 0;
		energy_points = 0;
		hostage_points = 0;
	}

	all_hostage_text[0] = 0;
	endgame_text[0] = 0;

	if (!Cheats_enabled && (Players[Player_num].hostages_on_board == Players[Player_num].hostages_level))
	{
		all_hostage_points = Players[Player_num].hostages_on_board * 1000 * (Difficulty_level + 1);
		sprintf(all_hostage_text, "%s%i\n", TXT_FULL_RESCUE_BONUS, all_hostage_points);
	}
	else
		all_hostage_points = 0;

	if (!Cheats_enabled && !(Game_mode & GM_MULTI) && (Players[Player_num].lives) && (Current_level_num == Last_level)) //player has finished the game!
	{
		endgame_points = Players[Player_num].lives * 10000;
		sprintf(endgame_text, "%s%i\n", TXT_SHIP_BONUS, endgame_points);
		is_last_level = 1;
	}
	else
		endgame_points = is_last_level = 0;

	mprintf((0, "adding bonus points\n"));
	add_bonus_points_to_score(skill_points + energy_points + shield_points + hostage_points + all_hostage_points + endgame_points);

	c = 0;
	sprintf(m_str[c++], "%s%i", TXT_SHIELD_BONUS, shield_points);		// Return at start to lower menu...
	sprintf(m_str[c++], "%s%i", TXT_ENERGY_BONUS, energy_points);
	sprintf(m_str[c++], "%s%i", TXT_HOSTAGE_BONUS, hostage_points);
	sprintf(m_str[c++], "%s%i", TXT_SKILL_BONUS, skill_points);

	sprintf(m_str[c++], "%s", all_hostage_text);
	if (!(Game_mode & GM_MULTI) && (Players[Player_num].lives) && (Current_level_num == Last_level))
		sprintf(m_str[c++], "%s", endgame_text);

	sprintf(m_str[c++], "%s%i\n", TXT_TOTAL_BONUS, shield_points + energy_points + hostage_points + skill_points + all_hostage_points + endgame_points);
	sprintf(m_str[c++], "%s%i", TXT_TOTAL_SCORE, Players[Player_num].score);

#ifdef WINDOWS
	sprintf(m_str[c++], "");
	sprintf(m_str[c++], "         Done");
#endif

	for (i = 0; i < c; i++) {
		m[i].type = NM_TYPE_TEXT;
		m[i].text = m_str[i];
	}

#ifdef WINDOWS
	m[c - 1].type = NM_TYPE_MENU;
#endif

	// m[c].type = NM_TYPE_MENU;	m[c++].text = "Ok";

	if (Current_level_num < 0)
		sprintf(title, "%s%s %d %s\n %s %s", is_last_level ? "\n\n\n" : "\n", TXT_SECRET_LEVEL, -Current_level_num, TXT_COMPLETE, Current_level_name, TXT_DESTROYED);
	else
		sprintf(title, "%s%s %d %s\n%s %s", is_last_level ? "\n\n\n" : "\n", TXT_LEVEL, Current_level_num, TXT_COMPLETE, Current_level_name, TXT_DESTROYED);

	Assert(c <= N_GLITZITEMS);

	gr_palette_fade_out(gr_palette, 32, 0);

	mprintf((0, "doing menu\n"));

	//PA_DFX (pa_alpha_always()); //[ISB] todo

#ifdef NETWORK
	if (network && (Game_mode & GM_NETWORK))
		newmenu_do2(NULL, title, c, m, (void(*))network_endlevel_poll2, 0, STARS_BACKGROUND);
	else
#endif
		newmenu_do2(NULL, title, c, m, NULL, 0, STARS_BACKGROUND);

	mprintf((0, "done DoEndLevelScoreGlitz\n"));
}

//give the player the opportunity to save his game
void DoEndlevelMenu()
{
	//No between level saves......!!!	state_save_all(1);
}

void InitPlayerPosition(int random_flag);

//	-----------------------------------------------------------------------------------------------------
//called when the player is starting a level (new game or new ship)
void StartSecretLevel()
{
	Assert(!Player_is_dead);

	InitPlayerPosition(0);

	verify_console_object();

	ConsoleObject->control_type = CT_FLYING;
	ConsoleObject->movement_type = MT_PHYSICS;

	// -- WHY? -- disable_matcens();
	clear_transient_objects(0);		//0 means leave proximity bombs

	// create_player_appearance_effect(ConsoleObject);
	Do_appearance_effect = 1;

	ai_reset_all_paths();
	// -- NO? -- reset_time();

	reset_rear_view();
	Auto_fire_fusion_cannon_time = 0;
	Fusion_charge = 0;

	Robot_firing_enabled = 1;
}

extern void set_pos_from_return_segment(void);

//	Returns true if secret level has been destroyed.
int p_secret_level_destroyed(void)
{
	if (First_secret_visit)
	{
		return 0;		//	Never been there, can't have been destroyed.
	}
	else
	{
		FILE* fp;
		if ((fp = fopen(SECRETC_FILENAME, "rb")) != NULL)
		{
			fclose(fp);
			return 0;
		}
		else
		{
			return 1;
		}
	}
}

void load_stars();

//	-----------------------------------------------------------------------------------------------------
void do_secret_message(char* msg)
{
	int	old_fmode;

	load_stars();

#if defined(POLY_ACC)
	pa_save_clut();
	pa_update_clut(gr_palette, 0, 256, 0);
#endif

	old_fmode = Function_mode;
	Function_mode = FMODE_MENU;
	nm_messagebox(NULL, 1, TXT_OK, msg);
	Function_mode = old_fmode;

#if defined(POLY_ACC)
	pa_restore_clut();
#endif

	WIN(DEFINE_SCREEN(NULL));
}

//	-----------------------------------------------------------------------------------------------------
// called when the player is starting a new level for normal game mode and restore state
//	Need to deal with whether this is the first time coming to this level or not.  If not the
//	first time, instead of initializing various things, need to do a game restore for all the
//	robots, powerups, walls, doors, etc.
void StartNewLevelSecret(int level_num, int page_in_textures)
{
	newmenu_item	m[1];
	//int i;

	ThisLevelTime = 0;

	m[0].type = NM_TYPE_TEXT;
	m[0].text = const_cast<char*>(" ");

	last_drawn_cockpit[0] = -1;
	last_drawn_cockpit[1] = -1;

	if (Newdemo_state == ND_STATE_PAUSED)
		Newdemo_state = ND_STATE_RECORDING;

	if (Newdemo_state == ND_STATE_RECORDING)
	{
		newdemo_set_new_level(level_num);
		newdemo_record_start_frame(FrameCount, FrameTime);
	}
	else if (Newdemo_state != ND_STATE_PLAYBACK)
	{

		gr_palette_fade_out(gr_palette, 32, 0);

		set_screen_mode(SCREEN_MENU);		//go into menu mode

		if (First_secret_visit)
		{
			do_secret_message(TXT_SECRET_EXIT);
		}
		else
		{
			FILE* fp;
			if ((fp = fopen(SECRETC_FILENAME, "rb")) != NULL)
			{
				fclose(fp);
				do_secret_message(TXT_SECRET_EXIT);
			}
			else
			{
				char	text_str[128];

				sprintf(text_str, "Secret level already destroyed.\nAdvancing to level %i.", Current_level_num + 1);
				do_secret_message(text_str);
			}
		}
	}

	LoadLevel(level_num, page_in_textures);

	Assert(Current_level_num == level_num);	//make sure level set right

	Assert(Function_mode == FMODE_GAME);

	gameseq_init_network_players(); // Initialize the Players array for 
											  // this level

	HUD_clear_messages();

	automap_clear_visited();

	// --	init_player_stats_level();

	Viewer = &Objects[Players[Player_num].objnum];

	gameseq_remove_unused_players();

	Game_suspended = 0;

	Control_center_destroyed = 0;

	init_cockpit();
	reset_palette_add();

	if (First_secret_visit || (Newdemo_state == ND_STATE_PLAYBACK))
	{
		init_robots_for_level();
		init_ai_objects();
		init_smega_detonates();
		init_morphs();
		init_all_matcens();
		reset_special_effects();
		StartSecretLevel();
	}
	else
	{
		FILE* fp;
		if ((fp = fopen(SECRETC_FILENAME, "rb")) != NULL)
		{
			int	pw_save, sw_save;

			fclose(fp);

			pw_save = Primary_weapon;
			sw_save = Secondary_weapon;
			state_restore_all(1, 1, const_cast<char*>(SECRETC_FILENAME));
			Primary_weapon = pw_save;
			Secondary_weapon = sw_save;
			reset_special_effects();
			StartSecretLevel();
			// -- No: This is only for returning to base level: set_pos_from_return_segment();
		}
		else
		{
			char	text_str[128];

			sprintf(text_str, "Secret level already destroyed.\nAdvancing to level %i.", Current_level_num + 1);
			do_secret_message(text_str);
			return;

			// -- //	If file doesn't exist, it's because reactor was destroyed.
			// -- mprintf((0, "secret.sgc doesn't exist.  Advancing to next level.\n"));
			// -- // -- StartNewLevel(Secret_level_table[-Current_level_num-1]+1, 0);
			// -- StartNewLevel(Secret_level_table[-Current_level_num-1]+1, 0);
			// -- return;
		}
	}

	if (First_secret_visit)
	{
		copy_defaults_to_robot_all();
	}

	turn_cheats_off();

	init_controlcen_for_level();

	//	Say player can use FLASH cheat to mark path to exit.
	Last_level_path_created = -1;

	First_secret_visit = 0;
}

int	Entered_from_level;

void returning_to_level_message(void);
void DoEndGame(void);
void advancing_to_level_message(void);

// ---------------------------------------------------------------------------------------------------------------
//	Called from switch.c when player is on a secret level and hits exit to return to base level.
void ExitSecretLevel(void)
{
	FILE* fp;

	if (Newdemo_state == ND_STATE_PLAYBACK)
		return;

	if (!Control_center_destroyed)
	{
		state_save_all(0, 2, const_cast<char*>(SECRETC_FILENAME));
	}

	if ((fp = fopen(SECRETB_FILENAME, "rb")) != NULL)
	{
		int	pw_save, sw_save;

		returning_to_level_message();
		fclose(fp);
		pw_save = Primary_weapon;
		sw_save = Secondary_weapon;
		state_restore_all(1, 1, const_cast<char*>(SECRETB_FILENAME));
		Primary_weapon = pw_save;
		Secondary_weapon = sw_save;
	}
	else
	{
		// File doesn't exist, so can't return to base level.  Advance to next one.
		if (Entered_from_level == Last_level)
			DoEndGame();
		else
		{
			advancing_to_level_message();
			StartNewLevel(Entered_from_level + 1, 0);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------
//	Set invulnerable_time and cloak_time in player struct to preserve amount of time left to
//	be invulnerable or cloaked.
void do_cloak_invul_secret_stuff(fix old_gametime)
{
	if (Players[Player_num].flags & PLAYER_FLAGS_INVULNERABLE)
	{
		fix	time_used;

		time_used = old_gametime - Players[Player_num].invulnerable_time;
		Players[Player_num].invulnerable_time = GameTime - time_used;
	}

	if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED)
	{
		fix	time_used;

		time_used = old_gametime - Players[Player_num].cloak_time;
		Players[Player_num].cloak_time = GameTime - time_used;
	}
}

// ---------------------------------------------------------------------------------------------------------------
//	Called from switch.c when player passes through secret exit.  That means he was on a non-secret level and he
//	is passing to the secret level.
//	Do a savegame.
void EnterSecretLevel(void)
{
	fix	old_gametime;
	int	i;

	Assert(!(Game_mode & GM_MULTI));

	Entered_from_level = Current_level_num;

	if (Control_center_destroyed)
		DoEndLevelScoreGlitz(0);

	if (Newdemo_state != ND_STATE_PLAYBACK)
		state_save_all(0, 1, NULL);	//	Not between levels (ie, save all), IS a secret level, NO filename override

	//	Find secret level number to go to, stuff in Next_level_num.
	for (i = 0; i < -Last_secret_level; i++)
		if (Secret_level_table[i] == Current_level_num)
		{
			Next_level_num = -(i + 1);
			break;
		}
		else if (Secret_level_table[i] > Current_level_num) //	Allows multiple exits in same group.
		{
			Next_level_num = -i;
			break;
		}

	if (!(i < -Last_secret_level))		//didn't find level, so must be last
		Next_level_num = Last_secret_level;

	old_gametime = GameTime;

	StartNewLevelSecret(Next_level_num, 1);

	// do_cloak_invul_stuff();
}

void AdvanceLevel(int secret_flag);

//called when the player has finished a level
void PlayerFinishedLevel(int secret_flag)
{
	Assert(!secret_flag);

	//credit the player for hostages
	Players[Player_num].hostages_rescued_total += Players[Player_num].hostages_on_board;

	if (Game_mode & GM_NETWORK)
		Players[Player_num].connected = 2; // Finished but did not die

	last_drawn_cockpit[0] = -1;
	last_drawn_cockpit[1] = -1;

	AdvanceLevel(secret_flag);				//now go on to the next one (if one)
}

#if defined(D2_OEM) || defined(COMPILATION)
#define MOVIE_REQUIRED 0
#else
#define MOVIE_REQUIRED 1
#endif

#ifdef D2_OEM
#define ENDMOVIE "endo"
#else
#define ENDMOVIE "end"
#endif

void show_order_form();
extern void com_hangup(void);

//called when the player has finished the last level
void DoEndGame(void)
{
	mprintf((0, "DoEndGame\n"));

	Function_mode = FMODE_MENU;
	if ((Newdemo_state == ND_STATE_RECORDING) || (Newdemo_state == ND_STATE_PAUSED))
		newdemo_stop_recording();

	set_screen_mode(SCREEN_MENU);

	WINDOS(
		dd_gr_set_current_canvas(NULL),
		gr_set_current_canvas(NULL)
	);

	key_flush();

	if (Current_mission_num == 0 && !(Game_mode & GM_MULTI))		//only built-in mission, & not multi
	{
		int played = MOVIE_NOT_PLAYED;	//default is not played

#ifdef SHAREWARE
		songs_play_song(SONG_ENDGAME, 0);
		mprintf((0, "doing briefing\n"));
		do_briefing_screens("ending2.tex", 1);
		mprintf((0, "briefing done\n"));
#else
		init_subtitles(ENDMOVIE ".tex");	//ingore errors
		played = PlayMovie(ENDMOVIE, MOVIE_REQUIRED);
		close_subtitles();
#ifdef D2_OEM
		if (!played) {
			songs_play_song(SONG_TITLE, 0);
			do_briefing_screens("end2oem.tex", 1);
		}
#endif
#endif	
	}
	else if (!(Game_mode & GM_MULTI)) //not multi
	{
		char tname[FILENAME_LEN];
		sprintf(tname, "%s.tex", Current_mission_filename);
		do_briefing_screens(tname, Last_level + 1);		//level past last is endgame breifing

		//try doing special credits
		sprintf(tname, "%s.ctb", Current_mission_filename);
		credits_show(tname);
	}

	key_flush();

#ifdef SHAREWARE
	show_order_form();
#endif

#ifdef NETWORK
	if (Game_mode & GM_MULTI)
		multi_endlevel_score();
	else
#endif
		DoEndLevelScoreGlitz(0);

	if (Current_mission_num == 0 && !((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP)))
	{
		WINDOS(
			dd_gr_set_current_canvas(NULL),
			gr_set_current_canvas(NULL)
		);
		WINDOS(
			dd_gr_clear_canvas(BM_XRGB(0, 0, 0)),
			gr_clear_canvas(BM_XRGB(0, 0, 0))
		);
		gr_palette_clear();
		load_palette(DEFAULT_PALETTE, 0, 1);
		scores_maybe_add_player(0);
	}

	Function_mode = FMODE_MENU;

	if ((Game_mode & GM_SERIAL) || (Game_mode & GM_MODEM))
		Game_mode |= GM_GAME_OVER;		//preserve modem setting so go back into modem menu
	else
		Game_mode = GM_GAME_OVER;


	longjmp(LeaveGame, 0);		// Exit out of game loop
}

//from which level each do you get to each secret level 
int Secret_level_table[MAX_SECRET_LEVELS_PER_MISSION];

//called to go to the next level (if there is one)
//if secret_flag is true, advance to secret level, else next normal one
//	Return true if game over.
void AdvanceLevel(int secret_flag)
{
	int result;

	mprintf((0, "AdvanceLevel\n"));

	Assert(!secret_flag);

	if (Current_level_num != Last_level)
	{
#ifdef NETWORK
		if (Game_mode & GM_MULTI)
			multi_endlevel_score();
		else
#endif
			DoEndLevelScoreGlitz(0);		//give bonuses
	}

	Control_center_destroyed = 0;

#ifdef EDITOR
	if (Current_level_num == 0)
		return;		//not a real level
#endif

#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
		result = multi_endlevel(&secret_flag); // Wait for other players to reach this point
		if (result) // failed to sync
			if (Current_level_num == Last_level)		//player has finished the game!
				longjmp(LeaveGame, 0);		// Exit out of game loop
			else
				return;
	}
#endif

	if (Current_level_num == Last_level) //player has finished the game!
	{
		mprintf((0, "Finished last level!\n"));
		DoEndGame();
	}
	else
	{
		Next_level_num = Current_level_num + 1;		//assume go to next normal level

		if (!(Game_mode & GM_MULTI))
			DoEndlevelMenu(); // Let use save their game

		StartNewLevel(Next_level_num, 0);
	}
}

#ifdef MACINTOSH		// horrible hack of a routine to load just the palette from the stars.pcx file

extern char last_palette_loaded[];

void load_stars_palette()
{
	int pcx_error;
	uint8_t pal[256 * 3];

	pcx_error = pcx_read_bitmap_palette(STARS_BACKGROUND, pal);
	Assert(pcx_error == PCX_ERROR_NONE);

	//@@gr_remap_bitmap_good( bmp, pal, -1, -1 );


	{	//remap stuff. this code is kindof a hack

		//now, before we bring up the menu, we need to 
		//do some stuff to make sure the palette is ok.  First, we need to
		//get our current palette into the 2d's array, so the remapping will
		//work.  Second, we need to remap the fonts.  Third, we need to fill 
		//in part of the fade tables so the darkening of the menu edges works

		gr_copy_palette(gr_palette, pal, sizeof(gr_palette));
		remap_fonts_and_menus(1);

	}

	strcpy(last_palette_loaded, "");		//force palette load next time
}
#endif

void nm_draw_background1(const char* filename);

void load_stars()
{
	WIN(DEFINE_SCREEN(STARS_BACKGROUND));
	nm_draw_background1(STARS_BACKGROUND);
}


void died_in_mine_message(void)
{
	// Tell the player he died in the mine, explain why
	int old_fmode;

	if (Game_mode & GM_MULTI)
		return;

	gr_palette_fade_out(gr_palette, 32, 0);

	set_screen_mode(SCREEN_MENU);		//go into menu mode

	WINDOS(
		dd_gr_set_current_canvas(NULL),
		gr_set_current_canvas(NULL)
	);

	load_stars();

#if defined(POLY_ACC)
	pa_save_clut();
	pa_update_clut(gr_palette, 0, 256, 0);
#endif

	old_fmode = Function_mode;
	Function_mode = FMODE_MENU;
	nm_messagebox(NULL, 1, TXT_OK, TXT_DIED_IN_MINE);
	Function_mode = old_fmode;

#if defined(POLY_ACC)
	pa_restore_clut();
#endif

	WIN(DEFINE_SCREEN(NULL));
}

//	Called when player dies on secret level.
void returning_to_level_message(void)
{
	char	msg[128];

	int old_fmode;

	if (Game_mode & GM_MULTI)
		return;

	gr_palette_fade_out(gr_palette, 32, 0);

	set_screen_mode(SCREEN_MENU);		//go into menu mode

	gr_set_current_canvas(NULL);

	load_stars();

#if defined(POLY_ACC)
	pa_save_clut();
	pa_update_clut(gr_palette, 0, 256, 0);
#endif

	old_fmode = Function_mode;
	Function_mode = FMODE_MENU;
	sprintf(msg, "Returning to level %i", Entered_from_level);
	nm_messagebox(NULL, 1, TXT_OK, msg);
	Function_mode = old_fmode;

#if defined(POLY_ACC)
	pa_restore_clut();
#endif

	WIN(DEFINE_SCREEN(NULL));
}

//	Called when player dies on secret level.
void advancing_to_level_message(void)
{
	char	msg[128];

	int old_fmode;

	//	Only supposed to come here from a secret level.
	Assert(Current_level_num < 0);

	if (Game_mode & GM_MULTI)
		return;

	gr_palette_fade_out(gr_palette, 32, 0);

	set_screen_mode(SCREEN_MENU);		//go into menu mode

	gr_set_current_canvas(NULL);

	load_stars();

#if defined(POLY_ACC)
	pa_save_clut();
	pa_update_clut(gr_palette, 0, 256, 0);
#endif

	old_fmode = Function_mode;
	Function_mode = FMODE_MENU;
	sprintf(msg, "Base level destroyed.\nAdvancing to level %i", Entered_from_level + 1);
	nm_messagebox(NULL, 1, TXT_OK, msg);
	Function_mode = old_fmode;

#if defined(POLY_ACC)
	pa_restore_clut();
#endif

	WIN(DEFINE_SCREEN(NULL));
}

void digi_stop_digi_sounds();

void DoPlayerDead()
{
	reset_palette_add();

	gr_palette_load(gr_palette);

	//	digi_pause_digi_sounds();		//kill any continuing sounds (eg. forcefield hum)
	digi_stop_digi_sounds();		//kill any continuing sounds (eg. forcefield hum)

	dead_player_end();		//terminate death sequence (if playing)

#ifdef EDITOR
	if (Game_mode == GM_EDITOR) {			//test mine, not real level
		object* playerobj = &Objects[Players[Player_num].objnum];
		//nm_messagebox( "You're Dead!", 1, "Continue", "Not a real game, though." );
		load_level("gamesave.lvl");
		init_player_stats_new_ship();
		playerobj->flags &= ~OF_SHOULD_BE_DEAD;
		StartLevel(0);
		return;
	}
#endif

#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
		multi_do_death(Players[Player_num].objnum);
	}
	else
#endif
	{				//Note link to above else!
		Players[Player_num].lives--;
		if (Players[Player_num].lives == 0)
		{
			DoGameOver();
			return;
		}
	}

	if (Control_center_destroyed)
	{
		//clear out stuff so no bonus
		Players[Player_num].hostages_on_board = 0;
		Players[Player_num].energy = 0;
		Players[Player_num].shields = 0;
		Players[Player_num].connected = 3;

		died_in_mine_message(); // Give them some indication of what happened

		if (Current_level_num < 0)
		{
			FILE* fp;

			if ((fp = fopen(SECRETB_FILENAME, "rb")) != NULL)
			{
				fclose(fp);
				returning_to_level_message();
				state_restore_all(1, 2, const_cast<char*>(SECRETB_FILENAME));			//	2 means you died
				set_pos_from_return_segment();
				Players[Player_num].lives--;						//	re-lose the life, Players[Player_num].lives got written over in restore.
			}
			else
			{
				advancing_to_level_message();
				StartNewLevel(Entered_from_level + 1, 0);
				init_player_stats_new_ship();	//	New, MK, 05/29/96!, fix bug with dying in secret level, advance to next level, keep powerups!
			}
		}
		else
		{
			AdvanceLevel(0);			//if finished, go on to next level

			init_player_stats_new_ship();
			last_drawn_cockpit[0] = -1;
			last_drawn_cockpit[1] = -1;
		}

	}
	else if (Current_level_num < 0)
	{
		FILE* fp;
		if ((fp = fopen(SECRETB_FILENAME, "rb")) != NULL)
		{
			fclose(fp);
			returning_to_level_message();
			if (!Control_center_destroyed)
				state_save_all(0, 2, const_cast<char*>(SECRETC_FILENAME));
			state_restore_all(1, 2, const_cast<char*>(SECRETB_FILENAME));
			set_pos_from_return_segment();
			Players[Player_num].lives--;						//	re-lose the life, Players[Player_num].lives got written over in restore.
		}
		else
		{
			died_in_mine_message(); // Give them some indication of what happened
			advancing_to_level_message();
			StartNewLevel(Entered_from_level + 1, 0);
			init_player_stats_new_ship();
		}
	}
	else
	{
		init_player_stats_new_ship();
		StartLevel(1);
	}

	digi_sync_sounds();
}

extern int BigWindowSwitch;
void filter_objects_from_level();

//called when the player is starting a new level for normal game mode and restore state
//	secret_flag set if came from a secret level
void StartNewLevelSub(int level_num, int page_in_textures, int secret_flag)
{
	if (!(Game_mode & GM_MULTI))
	{
		last_drawn_cockpit[0] = -1;
		last_drawn_cockpit[1] = -1;
	}
	BigWindowSwitch = 0;

	if (Newdemo_state == ND_STATE_PAUSED)
		Newdemo_state = ND_STATE_RECORDING;

	if (Newdemo_state == ND_STATE_RECORDING)
	{
		newdemo_set_new_level(level_num);
		newdemo_record_start_frame(FrameCount, FrameTime);
	}

	if (Game_mode & GM_MULTI)
		Function_mode = FMODE_MENU; // Cheap fix to prevent problems with errror dialogs in loadlevel.

	LoadLevel(level_num, page_in_textures);

	Assert(Current_level_num == level_num);	//make sure level set right

	gameseq_init_network_players(); // Initialize the Players array for 
											  // this level
	Viewer = &Objects[Players[Player_num].objnum];

	Assert(N_players <= NumNetPlayerPositions);
	//If this assert fails, there's not enough start positions

#ifdef NETWORK
	if (Game_mode & GM_NETWORK)
	{
		if (network_level_sync()) // After calling this, Player_num is set
			return;
	}
	if ((Game_mode & GM_SERIAL) || (Game_mode & GM_MODEM))
	{
		if (com_level_sync())
			return;
	}
#endif

	Assert(Function_mode == FMODE_GAME);

#ifndef NDEBUG
	//-- mprintf((0, "Player_num = %d, N_players = %d.\n", Player_num, N_players)); // DEBUG
#endif
	HUD_clear_messages();
	automap_clear_visited();

#ifdef NETWORK
	if (Network_new_game == 1)
	{
		Network_new_game = 0;
		init_player_stats_new_ship();
	}
#endif
	init_player_stats_level(secret_flag);

#ifdef NETWORK
	if ((Game_mode & GM_MULTI_COOP) && Network_rejoined)
	{
		int i;
		for (i = 0; i < N_players; i++)
			Players[i].flags |= Netgame.player_flags[i];
	}

	if (Game_mode & GM_MULTI)
	{
		multi_prep_level(); // Removes robots from level if necessary
	}
#endif

	gameseq_remove_unused_players();

	Game_suspended = 0;

	Control_center_destroyed = 0;

	set_screen_mode(SCREEN_GAME);
	init_cockpit();
	init_robots_for_level();
	init_ai_objects();
	init_smega_detonates();
	init_morphs();
	init_all_matcens();
	reset_palette_add();
	init_thief_for_level();
	init_stuck_objects();
	game_flush_inputs();		// clear out the keyboard
	if (!(Game_mode & GM_MULTI))
		filter_objects_from_level();

	turn_cheats_off();

	if (!(Game_mode & GM_MULTI) && !Cheats_enabled)
		set_highest_level(Current_level_num);
	else
		read_player_file();		//get window sizes

	reset_special_effects();

#ifdef NETWORK
	if (Network_rejoined == 1)
	{
#ifndef NDEBUG
		mprintf((0, "Restarting - joining in random location.\n"));
#endif
		Network_rejoined = 0;
		StartLevel(1);
	}
	else
#endif
		StartLevel(0);		// Note link to above if!

	copy_defaults_to_robot_all();
	init_controlcen_for_level();

	//	Say player can use FLASH cheat to mark path to exit.
	Last_level_path_created = -1;
}

#ifdef NETWORK
extern void bash_to_shield(int, const char*);
#else
void bash_to_shield(int i, const char* s)
{
	int type = Objects[i].id;

	mprintf((0, "Bashing %s object #%i to shield.\n", s, i));

	Objects[i].id = POW_SHIELD_BOOST;
	Objects[i].rtype.vclip_info.vclip_num = Powerup_info[Objects[i].id].vclip_num;
	Objects[i].rtype.vclip_info.frametime = Vclip[Objects[i].rtype.vclip_info.vclip_num].frame_time;
}
#endif

void filter_objects_from_level()
{
	int i;

	mprintf((0, "Highest object index=%d\n", Highest_object_index));

	for (i = 0; i <= Highest_object_index; i++)
	{
		if (Objects[i].type == OBJ_POWERUP)
			if (Objects[i].id == POW_FLAG_RED || Objects[i].id == POW_FLAG_BLUE)
				bash_to_shield(i, "Flag!!!!");
	}
}

struct {
	int	level_num;
	char	movie_name[FILENAME_LEN];
} intro_movie[] = { { 1,"pla"},
							{ 5,"plb"},
							{ 9,"plc"},
							{13,"pld"},
							{17,"ple"},
							{21,"plf"},
							{24,"plg"} };

#define NUM_INTRO_MOVIES (sizeof(intro_movie) / sizeof(*intro_movie))

extern int MenuHiresAvailable;
extern int robot_movies;	//0 means none, 1 means lowres, 2 means hires
extern int intro_played;	//true if big intro movie played

void ShowLevelIntro(int level_num)
{
	//if shareware, show a briefing?

	if (!(Game_mode & GM_MULTI)) {
		int i;
		uint8_t save_pal[sizeof(gr_palette)];

		memcpy(save_pal, gr_palette, sizeof(gr_palette));

#if defined(D2_OEM) || defined(COMPILATION)
		if (level_num == 1 && !intro_played)
			do_briefing_screens("brief2o.tex", 1);
#endif

		if (Current_mission_num == 0)
		{
			int movie = 0;
#ifdef SHAREWARE
			if (level_num == 1)
			{
				do_briefing_screens("brief2.tex", 1);
			}
#else
			for (i = 0; i < NUM_INTRO_MOVIES; i++)
			{
				if (intro_movie[i].level_num == level_num)
				{
					PlayMovie(intro_movie[i].movie_name, MOVIE_REQUIRED);
					movie = 1;
					break;
				}
			}

#ifdef WINDOWS
			if (!movie) {					//must go before briefing
				dd_gr_init_screen();
				Screen_mode = -1;
			}
#endif

			if (robot_movies)
			{
				int hires_save = MenuHiresAvailable;

				if (robot_movies == 1)		//lowres only
				{
					MenuHiresAvailable = 0;		//pretend we can't do highres

					if (hires_save != MenuHiresAvailable)
						Screen_mode = -1;		//force reset

				}
				do_briefing_screens("robot.tex", level_num);
				MenuHiresAvailable = hires_save;
			}

#endif
		}
		else {	//not the built-in mission.  check for add-on briefing
			char tname[FILENAME_LEN];
			sprintf(tname, "%s.tex", Current_mission_filename);
			do_briefing_screens(tname, level_num);
		}


		memcpy(gr_palette, save_pal, sizeof(gr_palette));
	}
}

//	---------------------------------------------------------------------------
//	If starting a level which appears in the Secret_level_table, then set First_secret_visit.
//	Reason: On this level, if player goes to a secret level, he will be going to a different
//	secret level than he's ever been to before.
//	Sets the global First_secret_visit if necessary.  Otherwise leaves it unchanged.
void maybe_set_first_secret_visit(int level_num)
{
	int	i;

	for (i = 0; i < N_secret_levels; i++) {
		if (Secret_level_table[i] == level_num) {
			First_secret_visit = 1;
			mprintf((0, "Bashing First_secret_visit to 1 because entering level %i.\n", level_num));
		}
	}
}

//called when the player is starting a new level for normal game model
//	secret_flag if came from a secret level
void StartNewLevel(int level_num, int secret_flag)
{
	ThisLevelTime = 0;

	if ((level_num > 0) && (!secret_flag)) 
	{
		maybe_set_first_secret_visit(level_num);
	}

	ShowLevelIntro(level_num);
	WIN(DEFINE_SCREEN(NULL));		// ALT-TAB: no restore of background.
	StartNewLevelSub(level_num, 1, secret_flag);
}

//initialize the player object position & orientation (at start of game, or new ship)
void InitPlayerPosition(int random_flag)
{
	int NewPlayer;

	if (!((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))) // If not deathmatch
		NewPlayer = Player_num;

#ifdef NETWORK //[ISB] I need to check this
	else if (random_flag == 1)
	{
		int i, closest = -1, trys = 0;
		fix closest_dist = 0x7ffffff, dist;

		P_SRand(clock());

#ifndef NDEBUG
		if (NumNetPlayerPositions != MAX_NUM_NET_PLAYERS)
		{
			mprintf((1, "WARNING: There are only %d start positions!\n"));
			//Int3();
		}
#endif

		do
		{
			if (trys > 0)
			{
				mprintf((0, "Can't start in location %d because its too close to player %d\n", NewPlayer, closest));
			}
			trys++;

			NewPlayer = P_Rand() % NumNetPlayerPositions;

			closest = -1;
			closest_dist = 0x7fffffff;

			for (i = 0; i < N_players; i++)
			{
				if ((i != Player_num) && (Objects[Players[i].objnum].type == OBJ_PLAYER))
				{
					dist = find_connected_distance(&Objects[Players[i].objnum].pos, Objects[Players[i].objnum].segnum, &Player_init[NewPlayer].pos, Player_init[NewPlayer].segnum, 10, WID_FLY_FLAG);	//	Used to be 5, search up to 10 segments
					// -- mprintf((0, "Distance from start location %d to player %d is %f.\n", NewPlayer, i, f2fl(dist)));
					if ((dist < closest_dist) && (dist >= 0)) {
						closest_dist = dist;
						closest = i;
					}
				}
			}

			// -- mprintf((0, "Closest from pos %d is %f to plr %d.\n", NewPlayer, f2fl(closest_dist), closest));
		} while ((closest_dist < i2f(15 * 20)) && (trys < MAX_NUM_NET_PLAYERS * 2));
	}
#endif
	else
	{
		mprintf((0, "Starting position is not being changed.\n"));
		goto done; // If deathmatch and not random, positions were already determined by sync packet
	}
	Assert(NewPlayer >= 0);
	Assert(NewPlayer < NumNetPlayerPositions);

	ConsoleObject->pos = Player_init[NewPlayer].pos;
	ConsoleObject->orient = Player_init[NewPlayer].orient;
	// -- mprintf((0, "Pos set to %8x %8x %8x\n", ConsoleObject->pos.x, ConsoleObject->pos.y, ConsoleObject->pos.z));

		// -- mprintf((0, "Re-starting in location %d of %d.\n", NewPlayer+1, NumNetPlayerPositions));

	obj_relink(ConsoleObject - Objects, Player_init[NewPlayer].segnum);

done:
	reset_player_object();
	reset_cruise();
}

//	-----------------------------------------------------------------------------------------------------
//	Initialize default parameters for one robot, copying from Robot_info to *objp.
//	What about setting size!?  Where does that come from?
void copy_defaults_to_robot(object* objp)
{
	robot_info* robptr;
	int			objid;

	Assert(objp->type == OBJ_ROBOT);
	objid = objp->id;
	Assert(objid < N_robot_types);

	robptr = &Robot_info[objid];

	//	Boost shield for Thief and Buddy based on level.
	objp->shields = robptr->strength;

	if ((robptr->thief) || (robptr->companion))
	{
		objp->shields = (objp->shields * (abs(Current_level_num) + 7)) / 8;

		if (robptr->companion)
		{
			//	Now, scale guide-bot hits by skill level
			switch (Difficulty_level)
			{
			case 0:	objp->shields = i2f(20000);	break;		//	Trainee, basically unkillable
			case 1:	objp->shields *= 3;				break;		//	Rookie, pretty dang hard
			case 2:	objp->shields *= 2;				break;		//	Hotshot, a bit tough
			default:	break;
			}
		}
	}
	else if (robptr->boss_flag)	//	MK, 01/16/95, make boss shields lower on lower diff levels.
		objp->shields = objp->shields / (NDL + 3) * (Difficulty_level + 4);

	//	Additional wimpification of bosses at Trainee
	if ((robptr->boss_flag) && (Difficulty_level == 0))
		objp->shields /= 2;
}

//	-----------------------------------------------------------------------------------------------------
//	Copy all values from the robot info structure to all instances of robots.
//	This allows us to change bitmaps.tbl and have these changes manifested in existing robots.
//	This function should be called at level load time.
void copy_defaults_to_robot_all()
{
	int	i;

	for (i = 0; i <= Highest_object_index; i++)
		if (Objects[i].type == OBJ_ROBOT)
			copy_defaults_to_robot(&Objects[i]);

}

extern void clear_stuck_objects(void);

//	-----------------------------------------------------------------------------------------------------
//called when the player is starting a level (new game or new ship)
void StartLevel(int random_flag)
{
	Assert(!Player_is_dead);

	InitPlayerPosition(random_flag);

	verify_console_object();

	ConsoleObject->control_type = CT_FLYING;
	ConsoleObject->movement_type = MT_PHYSICS;

	disable_matcens();

	clear_transient_objects(0);		//0 means leave proximity bombs

	// create_player_appearance_effect(ConsoleObject);
	Do_appearance_effect = 1;

#ifdef NETWORK
	if (Game_mode & GM_MULTI)
	{
		if (Game_mode & GM_MULTI_COOP)
			multi_send_score();
		multi_send_position(Players[Player_num].objnum);
		multi_send_reappear();
	}

	if (Game_mode & GM_NETWORK)
		network_do_frame(1, 1);
#endif

	ai_reset_all_paths();
	ai_init_boss_for_ship();
	clear_stuck_objects();

#ifdef EDITOR
	//	Note, this is only done if editor builtin.  Calling this from here
	//	will cause it to be called after the player dies, resetting the
	//	hits for the buddy and thief.  This is ok, since it will work ok
	//	in a shipped version.
	init_ai_objects();
#endif

	reset_time();

	reset_rear_view();
	Auto_fire_fusion_cannon_time = 0;
	Fusion_charge = 0;

	Robot_firing_enabled = 1;
}
