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
#include <math.h>
#include <string.h>
#include "platform/platform_filesys.h"
#include "platform/mono.h"
#include "inferno.h"
#include "segment.h"
#include "textures.h"
#include "wall.h"
#include "object.h"
#include "gamemine.h"
#include "misc/error.h"
#include "gameseg.h"
#include "switch.h"
#include "game.h"
#include "newmenu.h"
#include "cfile/cfile.h"		
#include "fuelcen.h"
#include "misc/hash.h"
#include "platform/key.h"
#include "piggy.h"
#include "player.h"
#include "cntrlcen.h"
#include "morph.h"
#include "weapon.h"
#include "render.h"
#include "gameseq.h"
#include "gauges.h"
#include "newdemo.h"
#include "automap.h"
#include "piggy.h"
#include "paging.h"
#include "titles.h"
#include "stringtable.h"
#include "mission.h"
#include "2d/pcx.h"
#include "mem/mem.h"
#include "network.h"
#include "misc/args.h"

#ifndef SHAREWARE

#define STATE_VERSION 7
#define STATE_COMPATIBLE_VERSION 6
// 0 - Put DGSS (Descent Game State Save) id at tof.
// 1 - Added Difficulty level save
// 2 - Added Cheats_enabled flag
// 3 - Added between levels save.
// 4 - Added mission support
// 5 - Mike changed ai and object structure.
// 6 - Added buggin' cheat save
// 7 - Added other cheat saves and game_id.

#define NUM_SAVES 10
#define THUMBNAIL_W 100
#define THUMBNAIL_H 50
#define DESC_LENGTH 20

extern int ai_save_state(FILE * fp);
extern int ai_restore_state(FILE* fp);

extern void multi_initiate_save_game();
extern void multi_initiate_restore_game();

extern int Do_appearance_effect;
extern fix Fusion_next_sound_time;

extern int Laser_rapid_fire, Ugly_robot_cheat, Ugly_robot_texture;
extern int Physics_cheat_flag;
extern int	Lunacy;
extern void do_lunacy_on(void);
extern void do_lunacy_off(void);

int sc_last_item = 0;
grs_bitmap* sc_bmp[NUM_SAVES];

const char *dgss_id = "DGSS";

int state_default_item = 0;

uint32_t state_game_id;

void state_callback(int nitems, newmenu_item* items, int* last_key, int citem)
{
	nitems = nitems;
	last_key = last_key;

	//	if ( sc_last_item != citem )	{
	//		sc_last_item = citem;
	if (citem > 0) 
	{
		if (sc_bmp[citem - 1]) 
		{
			gr_set_current_canvas(NULL);
			gr_bitmap((grd_curcanv->cv_bitmap.bm_w - THUMBNAIL_W) / 2, items[0].y - 5, sc_bmp[citem - 1]);
		}
	}
	//	}	
}

void rpad_string(char* string, int max_chars)
{
	int i, end_found;

	end_found = 0;
	for (i = 0; i < max_chars; i++) 
	{
		if (*string == 0)
			end_found = 1;
		if (end_found)
			* string = ' ';
		string++;
	}
	*string = 0;		// NULL terminate
}

int state_get_save_file(char* fname, char* dsc, int multi)
{
	FILE* fp;
	int i, choice, version;
	newmenu_item m[NUM_SAVES + 1];
#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	char filename[NUM_SAVES][CHOCOLATE_MAX_FILE_PATH_SIZE], temp_filename[CHOCOLATE_MAX_FILE_PATH_SIZE];
#else
	char filename[NUM_SAVES][20];
#endif
	char desc[NUM_SAVES][DESC_LENGTH + 16];
	char id[5];
	int valid = 0;

	for (i = 0; i < NUM_SAVES; i++) 
	{
		sc_bmp[i] = NULL;
#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
		if (!multi)
			snprintf(temp_filename, CHOCOLATE_MAX_FILE_PATH_SIZE, "%s.sg%d", Players[Player_num].callsign, i);
		else
			snprintf(temp_filename, CHOCOLATE_MAX_FILE_PATH_SIZE, "%s.mg%d", Players[Player_num].callsign, i);
		get_full_file_path(filename[i], temp_filename, CHOCOLATE_SAVE_DIR);
#else
		if (!multi)
			sprintf(filename[i], "%s.sg%d", Players[Player_num].callsign, i);
		else
			sprintf(filename[i], "%s.mg%d", Players[Player_num].callsign, i);
#endif
		valid = 0;
		fp = fopen(filename[i], "rb");
		if (fp) 
		{
			//Read id
			fread(id, sizeof(char) * 4, 1, fp);
			if (!memcmp(id, dgss_id, 4)) 
			{
				//Read version
				fread(&version, sizeof(int), 1, fp);
				if (version >= STATE_COMPATIBLE_VERSION) 
				{
					// Read description
					fread(desc[i], sizeof(char) * DESC_LENGTH, 1, fp);
					//rpad_string( desc[i], DESC_LENGTH-1 );
					// Read thumbnail
					//sc_bmp[i] = gr_create_bitmap(THUMBNAIL_W,THUMBNAIL_H );
					//fread( sc_bmp[i]->bm_data, THUMBNAIL_W * THUMBNAIL_H, 1, fp );
					valid = 1;
				}
			}
			fclose(fp);
		}
		if (!valid)
		{
			strcpy(desc[i], TXT_EMPTY);
			//rpad_string( desc[i], DESC_LENGTH-1 );
		}
		m[i].type = NM_TYPE_INPUT_MENU; m[i].text = desc[i]; m[i].text_len = DESC_LENGTH - 1;
	}

	sc_last_item = -1;
	choice = newmenu_do1(NULL, "Save Game", NUM_SAVES, m, NULL, state_default_item);

	for (i = 0; i < NUM_SAVES; i++) 
	{
		if (sc_bmp[i])
			gr_free_bitmap(sc_bmp[i]);
	}

	if (choice > -1) 
	{
		strcpy(fname, filename[choice]);
		strcpy(dsc, desc[choice]);
		state_default_item = choice;
		return choice + 1;
	}
	return 0;
}

int state_get_restore_file(char* fname, int multi)
{
	FILE* fp;
	int i, choice, version, nsaves;
	newmenu_item m[NUM_SAVES + 1];
#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	char filename[NUM_SAVES][CHOCOLATE_MAX_FILE_PATH_SIZE], temp_filename[CHOCOLATE_MAX_FILE_PATH_SIZE];
#else
	char filename[NUM_SAVES][20];
#endif
	char desc[NUM_SAVES][DESC_LENGTH + 16];
	char id[5];
	int valid;

	nsaves = 0;
	m[0].type = NM_TYPE_TEXT; m[0].text = (char*)"\n\n\n\n";
	for (i = 0; i < NUM_SAVES; i++) 
	{
		sc_bmp[i] = NULL;
#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
		if (!multi)
			snprintf(temp_filename, CHOCOLATE_MAX_FILE_PATH_SIZE, "%s.sg%d", Players[Player_num].callsign, i);
		else
			snprintf(temp_filename, CHOCOLATE_MAX_FILE_PATH_SIZE, "%s.mg%d", Players[Player_num].callsign, i);
		get_full_file_path(filename[i], temp_filename, CHOCOLATE_SAVE_DIR);
#else
		if (!multi)
			sprintf(filename[i], "%s.sg%d", Players[Player_num].callsign, i);
		else
			sprintf(filename[i], "%s.mg%d", Players[Player_num].callsign, i);
#endif
		valid = 0;
		fp = fopen(filename[i], "rb");
		if (fp) 
		{
			//Read id
			fread(id, sizeof(char) * 4, 1, fp);
			if (!memcmp(id, dgss_id, 4)) 
			{
				//Read version
				fread(&version, sizeof(int), 1, fp);
				if (version >= STATE_COMPATIBLE_VERSION) 
				{
					// Read description
					fread(desc[i], sizeof(char) * DESC_LENGTH, 1, fp);
					//rpad_string( desc[i], DESC_LENGTH-1 );
					m[i + 1].type = NM_TYPE_MENU; m[i + 1].text = desc[i];;
					// Read thumbnail
					sc_bmp[i] = gr_create_bitmap(THUMBNAIL_W, THUMBNAIL_H);
					fread(sc_bmp[i]->bm_data, THUMBNAIL_W * THUMBNAIL_H, 1, fp);
					nsaves++;
					valid = 1;
				}
			}
			fclose(fp);
		}
		if (!valid) 
		{
			strcpy(desc[i], TXT_EMPTY);
			//rpad_string( desc[i], DESC_LENGTH-1 );
			m[i + 1].type = NM_TYPE_TEXT; m[i + 1].text = desc[i];
		}
	}

	if (nsaves < 1) 
	{
		nm_messagebox(NULL, 1, "Ok", "No saved games were found!");
		return 0;
	}

	sc_last_item = -1;
	choice = newmenu_do3(NULL, "Select Game to Restore", NUM_SAVES + 1, m, state_callback, state_default_item + 1, NULL, 190, -1);

	for (i = 0; i < NUM_SAVES; i++) 
	{
		if (sc_bmp[i])
			gr_free_bitmap(sc_bmp[i]);
	}

	if (choice > 0) 
	{
		strcpy(fname, filename[choice - 1]);
		state_default_item = choice - 1;
		return choice;
	}
	return 0;
}

int state_save_old_game(int slotnum, char* sg_name, player* sg_player,
	int sg_difficulty_level, int sg_primary_weapon,
	int sg_secondary_weapon, int sg_next_level_num)
{
	int i;
	int temp_int;
	uint8_t temp_byte;
	char desc[DESC_LENGTH + 1];
#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	char filename[CHOCOLATE_MAX_FILE_PATH_SIZE], temp_buffer[CHOCOLATE_MAX_FILE_PATH_SIZE];
	char* separator_pos;
#else
	char filename[128];
#endif
	grs_canvas* cnv;
	FILE* fp;

#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	snprintf(temp_buffer, CHOCOLATE_MAX_FILE_PATH_SIZE, "%s.sg%d", sg_player->callsign, slotnum);
	get_full_file_path(filename, temp_buffer, CHOCOLATE_SAVE_DIR);
#else
	sprintf(filename, "%s.sg%d", sg_player->callsign, slotnum);
#endif
	fp = fopen(filename, "wb");
	if (!fp) return 0;

	//Save id
	fwrite(dgss_id, sizeof(char) * 4, 1, fp);

	//Save version
	temp_int = STATE_VERSION;
	fwrite(&temp_int, sizeof(int), 1, fp);

	//Save description
	strncpy(desc, sg_name, DESC_LENGTH);
	fwrite(desc, sizeof(char) * DESC_LENGTH, 1, fp);

	// Save the current screen shot...
	cnv = gr_create_canvas(THUMBNAIL_W, THUMBNAIL_H);
	if (cnv) 
	{
		char* pcx_file;
		uint8_t pcx_palette[768];
		grs_bitmap bmp;

		gr_set_current_canvas(cnv);

		gr_clear_canvas(BM_XRGB(0, 0, 0));
		pcx_file = get_briefing_screen(sg_next_level_num);
		if (pcx_file != NULL) 
		{
			bmp.bm_data = NULL;
			if (pcx_read_bitmap(pcx_file, &bmp, BM_LINEAR, pcx_palette) == PCX_ERROR_NONE) 
			{
				grs_point vertbuf[3];
				gr_clear_canvas(255);
				vertbuf[0].x = vertbuf[0].y = -F1_0 * 6;		// -6 pixel rows for ascpect
				vertbuf[1].x = vertbuf[1].y = 0;
				vertbuf[2].x = i2f(THUMBNAIL_W); vertbuf[2].y = i2f(THUMBNAIL_H + 7);	// + 7 pixel rows for ascpect
				scale_bitmap(&bmp, vertbuf);
				gr_remap_bitmap_good(&cnv->cv_bitmap, pcx_palette, -1, -1);
				free(bmp.bm_data);
			}
		}
		fwrite(cnv->cv_bitmap.bm_data, THUMBNAIL_W * THUMBNAIL_H, 1, fp);
		gr_free_canvas(cnv);
	}
	else 
	{
		uint8_t color = 0;
		for (i = 0; i < THUMBNAIL_W * THUMBNAIL_H; i++)
			fwrite(&color, sizeof(uint8_t), 1, fp);
	}

	// Save the Between levels flag...
	temp_int = 1;
	fwrite(&temp_int, sizeof(int), 1, fp);

	// Save the mission info...
#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	separator_pos = strrchr(Mission_list[0].filename, PLATFORM_PATH_SEPARATOR);
	if(separator_pos != NULL)
	{
		strncpy(temp_buffer, separator_pos + 1, CHOCOLATE_MAX_FILE_PATH_SIZE - 1);
		fwrite(temp_buffer, sizeof(char) * 9, 1, fp);
	}
	else
	{
		fwrite(&Mission_list[0], sizeof(char) * 9, 1, fp);
	}
#else
	fwrite(&Mission_list[0], sizeof(char) * 9, 1, fp);
#endif

	//Save level info
	temp_int = sg_player->level;
	fwrite(&temp_int, sizeof(int), 1, fp);
	temp_int = sg_next_level_num;
	fwrite(&temp_int, sizeof(int), 1, fp);

	//Save GameTime
	temp_int = 0;
	fwrite(&temp_int, sizeof(fix), 1, fp);

	//Save player info
	fwrite(sg_player, sizeof(player), 1, fp);

	// Save the current weapon info
	temp_byte = sg_primary_weapon;
	fwrite(&temp_byte, sizeof(int8_t), 1, fp);
	temp_byte = sg_secondary_weapon;
	fwrite(&temp_byte, sizeof(int8_t), 1, fp);

	// Save the difficulty level
	temp_int = sg_difficulty_level;
	fwrite(&temp_int, sizeof(int), 1, fp);

	// Save the Cheats_enabled
	temp_int = 0;
	fwrite(&temp_int, sizeof(int), 1, fp);
	temp_int = 0;		// turbo mode
	fwrite(&temp_int, sizeof(int), 1, fp);

	fwrite(&state_game_id, sizeof(uint32_t), 1, fp);
	fwrite(&Laser_rapid_fire, sizeof(int), 1, fp);
	fwrite(&Ugly_robot_cheat, sizeof(int), 1, fp);
	fwrite(&Ugly_robot_texture, sizeof(int), 1, fp);
	fwrite(&Physics_cheat_flag, sizeof(int), 1, fp);
	fwrite(&Lunacy, sizeof(int), 1, fp);

	fclose(fp);

	return 1;
}

int state_save_all_sub(char* filename, char* desc, int between_levels);

int state_save_all(int between_levels)
{
	char filename[128], desc[DESC_LENGTH + 1];

	if (Game_mode & GM_MULTI) 
	{
#ifdef MULTI_SAVE
		if (FindArg("-multisave"))
			multi_initiate_save_game();
		else
#endif  
			HUD_init_message("Can't save in a multiplayer game!");
		return 0;
	}

	mprintf((0, "CL=%d, NL=%d\n", Current_level_num, Next_level_num));

	stop_time();

	if (!state_get_save_file(filename, desc, 0)) 
	{
		start_time();
		return 0;
	}

	return state_save_all_sub(filename, desc, between_levels);
}

int state_save_all_sub(char* filename, char* desc, int between_levels)
{
	int i, j;
	FILE* fp;
	grs_canvas* cnv;
#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	char temp_buffer[CHOCOLATE_MAX_FILE_PATH_SIZE];
	char* separator_pos;
#endif

	if (Game_mode & GM_MULTI) 
	{
#ifdef MULTI_SAVE
		if (!FindArg("-multisave"))
#endif  
			return 0;
	}

	fp = fopen(filename, "wb");
	if (!fp) 
	{
		start_time();
		return 0;
	}

	//Save id
	fwrite(dgss_id, sizeof(char) * 4, 1, fp);

	//Save version
	i = STATE_VERSION;
	//fwrite(&i, sizeof(int), 1, fp);
	F_WriteInt(fp, i);

	//Save description
	fwrite(desc, sizeof(char) * DESC_LENGTH, 1, fp);

	// Save the current screen shot...
	cnv = gr_create_canvas(THUMBNAIL_W, THUMBNAIL_H);
	if (cnv)
	{
		gr_set_current_canvas(cnv);
		if (between_levels) 
		{
			char* pcx_file;
			uint8_t pcx_palette[768];
			grs_bitmap bmp;

			gr_clear_canvas(BM_XRGB(0, 0, 0));
			pcx_file = get_briefing_screen(Next_level_num);
			if (pcx_file != NULL)
			{
				bmp.bm_data = NULL;
				if (pcx_read_bitmap(pcx_file, &bmp, BM_LINEAR, pcx_palette) == PCX_ERROR_NONE) 
				{
					grs_point vertbuf[3];
					gr_clear_canvas(255);
					vertbuf[0].x = vertbuf[0].y = -F1_0 * 6;		// -6 pixel rows for ascpect
					vertbuf[1].x = vertbuf[1].y = 0;
					vertbuf[2].x = i2f(THUMBNAIL_W); vertbuf[2].y = i2f(THUMBNAIL_H + 7);	// + 7 pixel rows for ascpect
					scale_bitmap(&bmp, vertbuf);
					gr_remap_bitmap_good(&cnv->cv_bitmap, pcx_palette, -1, -1);
					free(bmp.bm_data);
				}
			}
		}
		else 
		{
			render_frame(0);
		}
		fwrite(cnv->cv_bitmap.bm_data, THUMBNAIL_W * THUMBNAIL_H, 1, fp);
		gr_free_canvas(cnv);
	}
	else 
	{
		uint8_t color = 0;
		for (i = 0; i < THUMBNAIL_W * THUMBNAIL_H; i++)
			fwrite(&color, sizeof(uint8_t), 1, fp);
	}

	// Save the Between levels flag...
	//fwrite(&between_levels, sizeof(int), 1, fp);
	F_WriteInt(fp, between_levels);

	// Save the mission info...
#if defined(CHOCOLATE_USE_LOCALIZED_PATHS)
	separator_pos = strrchr(Mission_list[Current_mission_num].filename, PLATFORM_PATH_SEPARATOR);
	if(separator_pos != NULL)
	{
		strncpy(temp_buffer, separator_pos + 1, CHOCOLATE_MAX_FILE_PATH_SIZE - 1);
		fwrite(temp_buffer, sizeof(char) * 9, 1, fp);
	}
	else
	{
		fwrite(&Mission_list[Current_mission_num], sizeof(char) * 9, 1, fp);
	}
#else
	fwrite(&Mission_list[Current_mission_num], sizeof(char) * 9, 1, fp);
#endif

	//Save level info
	//fwrite(&Current_level_num, sizeof(int), 1, fp);
	//fwrite(&Next_level_num, sizeof(int), 1, fp);
	F_WriteInt(fp, Current_level_num);
	F_WriteInt(fp, Next_level_num);

	//Save GameTime
	//fwrite(&GameTime, sizeof(fix), 1, fp);
	F_WriteInt(fp, GameTime);

	//Save player info
	//fwrite(&Players[Player_num], sizeof(player), 1, fp);
	P_WritePlayer(&Players[Player_num], fp);

	// Save the current weapon info
	//fwrite(&Primary_weapon, sizeof(int8_t), 1, fp);
	//fwrite(&Secondary_weapon, sizeof(int8_t), 1, fp);
	F_WriteByte(fp, Primary_weapon);
	F_WriteByte(fp, Secondary_weapon);

	// Save the difficulty level
	//fwrite(&Difficulty_level, sizeof(int), 1, fp);
	F_WriteInt(fp, Difficulty_level);

	// Save the Cheats_enabled
	//fwrite(&Cheats_enabled, sizeof(int), 1, fp);
	//fwrite(&Game_turbo_mode, sizeof(int), 1, fp);
	F_WriteInt(fp, Cheats_enabled);
	F_WriteInt(fp, Game_turbo_mode);

	if (!between_levels) 
	{
		//Finish all morph objects
		for (i = 0; i <= Highest_object_index; i++) 
		{
			if ((Objects[i].type != OBJ_NONE) && (Objects[i].render_type == RT_MORPH)) 
			{
				morph_data* md;
				md = find_morph_data(&Objects[i]);
				if (md) 
				{
					md->obj->control_type = md->morph_save_control_type;
					md->obj->movement_type = md->morph_save_movement_type;
					md->obj->render_type = RT_POLYOBJ;
					md->obj->mtype.phys_info = md->morph_save_phys_info;
					md->obj = NULL;
				}
				else //maybe loaded half-morphed from disk
				{
					Objects[i].flags |= OF_SHOULD_BE_DEAD;
					Objects[i].render_type = RT_POLYOBJ;
					Objects[i].control_type = CT_NONE;
					Objects[i].movement_type = MT_NONE;
				}
			}
		}

		//Save object info
		i = Highest_object_index + 1;
		//fwrite(&i, sizeof(int), 1, fp);
		//fwrite(Objects, sizeof(object) * i, 1, fp);
		F_WriteInt(fp, i);
		for (i = 0; i < Highest_object_index + 1; i++)
		{
			P_WriteObject(&Objects[i], fp);
		}

		//Save wall info
		//i = Num_walls;
		//fwrite(&i, sizeof(int), 1, fp);
		//fwrite(Walls, sizeof(wall) * i, 1, fp);
		F_WriteInt(fp, Num_walls);
		for (i = 0; i < Num_walls; i++)
		{
			P_WriteWall(&Walls[i], fp);
		}

		//Save door info
		//i = Num_open_doors;
		//fwrite(&i, sizeof(int), 1, fp);
		//fwrite(ActiveDoors, sizeof(active_door) * i, 1, fp);
		F_WriteInt(fp, Num_open_doors);
		for (i = 0; i < Num_open_doors; i++)
		{
			P_WriteActiveDoor(&ActiveDoors[i], fp);
		}

		//Save trigger info
		//fwrite(&Num_triggers, sizeof(int), 1, fp);
		//fwrite(Triggers, sizeof(trigger) * Num_triggers, 1, fp);
		F_WriteInt(fp, Num_triggers);
		for (i = 0; i < Num_triggers; i++)
		{
			P_WriteTrigger(&Triggers[i], fp);
		}

		//Save tmap info
		for (i = 0; i <= Highest_segment_index; i++) 
		{
			for (j = 0; j < 6; j++) 
			{
				F_WriteShort(fp, Segments[i].sides[j].wall_num);
				F_WriteShort(fp, Segments[i].sides[j].tmap_num);
				F_WriteShort(fp, Segments[i].sides[j].tmap_num2);
			}
		}

		// Save the fuelcen info
		//fwrite(&Fuelcen_control_center_destroyed, sizeof(int), 1, fp);
		//fwrite(&Fuelcen_seconds_left, sizeof(int), 1, fp);
		F_WriteInt(fp, Fuelcen_control_center_destroyed);
		F_WriteInt(fp, Fuelcen_seconds_left);
		//fwrite(&Num_robot_centers, sizeof(int), 1, fp);
		F_WriteInt(fp, Num_robot_centers);
		//fwrite(RobotCenters, sizeof(matcen_info) * Num_robot_centers, 1, fp);
		for (i = 0; i < Num_robot_centers; i++)
		{
			P_WriteMatcen(&RobotCenters[i], fp);
		}
		//fwrite(&ControlCenterTriggers, sizeof(control_center_triggers), 1, fp);
		P_WriteReactorTrigger(&ControlCenterTriggers, fp);
		//fwrite(&Num_fuelcenters, sizeof(int), 1, fp);
		//fwrite(Station, sizeof(FuelCenter) * Num_fuelcenters, 1, fp);
		F_WriteInt(fp, Num_fuelcenters);
		for (int i = 0; i < Num_fuelcenters; i++)
		{
			P_WriteFuelCenter(&Station[i], fp);
		}

		// Save the control cen info
		//fwrite(&Control_center_been_hit, sizeof(int), 1, fp);
		//fwrite(&Control_center_player_been_seen, sizeof(int), 1, fp);
		//fwrite(&Control_center_next_fire_time, sizeof(int), 1, fp);
		//fwrite(&Control_center_present, sizeof(int), 1, fp);
		//fwrite(&Dead_controlcen_object_num, sizeof(int), 1, fp);
		F_WriteInt(fp, Control_center_been_hit);
		F_WriteInt(fp, Control_center_player_been_seen);
		F_WriteInt(fp, Control_center_next_fire_time);
		F_WriteInt(fp, Control_center_present);
		F_WriteInt(fp, Dead_controlcen_object_num);

		// Save the AI state
		ai_save_state(fp);

		// Save the automap visited info
		fwrite(Automap_visited, sizeof(uint8_t) * MAX_SEGMENTS, 1, fp);
	}
	//fwrite(&state_game_id, sizeof(uint32_t), 1, fp);
	//fwrite(&Laser_rapid_fire, sizeof(int), 1, fp);
	//fwrite(&Ugly_robot_cheat, sizeof(int), 1, fp);
	//fwrite(&Ugly_robot_texture, sizeof(int), 1, fp);
	//fwrite(&Physics_cheat_flag, sizeof(int), 1, fp);
	//fwrite(&Lunacy, sizeof(int), 1, fp);
	F_WriteInt(fp, state_game_id);
	F_WriteInt(fp, Laser_rapid_fire);
	F_WriteInt(fp, Ugly_robot_cheat);
	F_WriteInt(fp, Ugly_robot_texture);
	F_WriteInt(fp, Physics_cheat_flag);
	F_WriteInt(fp, Lunacy);

	fclose(fp);

	start_time();

	return 1;
}

int state_restore_all_sub(char* filename, int multi);

int state_restore_all(int in_game)
{
	char filename[128];

	if (Game_mode & GM_MULTI) 
	{
#ifdef MULTI_SAVE
		if (FindArg("-multisave"))
			multi_initiate_restore_game();
		else
#endif
			HUD_init_message("Can't restore in a multiplayer game!");
		return 0;
	}

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_stop_recording();

	if (Newdemo_state != ND_STATE_NORMAL)
		return 0;

	stop_time();
	if (!state_get_restore_file(filename, 0)) 
	{
		start_time();
		return 0;
	}

	if (in_game) 
	{
		int choice;
		choice = nm_messagebox(NULL, 2, "Yes", "No", "Restore Game?");
		if (choice != 0) 
		{
			start_time();
			return 0;
		}
	}

	start_time();

	return state_restore_all_sub(filename, 0);
}

int state_restore_all_sub(char* filename, int multi)
{
	int ObjectStartLocation;
	int BogusSaturnShit = 0;
	int version, i, j, segnum;
	object* obj;
	FILE* fp;
	int current_level, next_level;
	int between_levels;
	char mission[16];
	char desc[DESC_LENGTH + 1];
	char id[5];
	char org_callsign[CALLSIGN_LEN + 16];

	if (Game_mode & GM_MULTI) 
	{
#ifdef MULTI_SAVE
		if (!FindArg("-multisave"))
#endif
			return 0;
	}

	fp = fopen(filename, "rb");
	if (!fp) return 0;

	//Read id
	fread(id, sizeof(char) * 4, 1, fp);
	if (memcmp(id, dgss_id, 4)) 
	{
		fclose(fp);
		return 0;
	}

	//Read version
	//fread(&version, sizeof(int), 1, fp);
	version = F_ReadInt(fp);
	if (version < STATE_COMPATIBLE_VERSION) 
	{
		fclose(fp);
		return 0;
	}

	// Read description
	fread(desc, sizeof(char) * DESC_LENGTH, 1, fp);

	// Skip the current screen shot...
	fseek(fp, THUMBNAIL_W * THUMBNAIL_H, SEEK_CUR);

	// Read the Between levels flag...
	//fread(&between_levels, sizeof(int), 1, fp);
	between_levels = F_ReadInt(fp);

	// Read the mission info...
	fread(mission, sizeof(char) * 9, 1, fp);

	if (!load_mission_by_name(mission)) 
	{
		nm_messagebox(NULL, 1, "Ok", "Error!\nUnable to load mission\n'%s'\n", mission);
		fclose(fp);
		return 0;
	}

	//Read level info
	//fread(&current_level, sizeof(int), 1, fp);
	//fread(&next_level, sizeof(int), 1, fp);
	current_level = F_ReadInt(fp);
	next_level = F_ReadInt(fp);

	//Restore GameTime
	//fread(&GameTime, sizeof(fix), 1, fp);
	GameTime = F_ReadInt(fp);

	// Start new game....
	if (!multi) 
	{
		Game_mode = GM_NORMAL;
		Function_mode = FMODE_GAME;
#ifdef NETWORK
		change_playernum_to(0);
#endif
		strcpy(org_callsign, Players[0].callsign);
		N_players = 1;
		InitPlayerObject();				//make sure player's object set up
		init_player_stats_game();		//clear all stats
	}
	else 
	{
		strcpy(org_callsign, Players[Player_num].callsign);
	}
	//Read player info

	if (between_levels) 
	{
		int saved_offset;
		//fread(&Players[Player_num], sizeof(player), 1, fp);
		P_ReadPlayer(&Players[Player_num], fp);
		saved_offset = ftell(fp);
		fclose(fp);
		do_briefing_screens(next_level);
		fp = fopen(filename, "rb");
		fseek(fp, saved_offset, SEEK_SET);
		StartNewLevelSub(next_level, 0);
	}
	else 
	{
		StartNewLevelSub(current_level, 0);
		//fread(&Players[Player_num], sizeof(player), 1, fp);
		P_ReadPlayer(&Players[Player_num], fp);
	}
	strcpy(Players[Player_num].callsign, org_callsign);

	// Set the right level
	if (between_levels)
		Players[Player_num].level = next_level;

	// Restore the weapon states
	//fread(&Primary_weapon, sizeof(int8_t), 1, fp);
	//fread(&Secondary_weapon, sizeof(int8_t), 1, fp);
	Primary_weapon = F_ReadByte(fp);
	Secondary_weapon = F_ReadByte(fp);

	select_weapon(Primary_weapon, 0, 0, 0);
	select_weapon(Secondary_weapon, 1, 0, 0);

	// Restore the difficulty level
	//fread(&Difficulty_level, sizeof(int), 1, fp);
	Difficulty_level = F_ReadInt(fp);

	// Restore the cheats enabled flag
	//fread(&Cheats_enabled, sizeof(int), 1, fp);
	//fread(&Game_turbo_mode, sizeof(int), 1, fp);
	Cheats_enabled = F_ReadInt(fp);
	Game_turbo_mode = F_ReadInt(fp);

	if (!between_levels) 
	{
		Do_appearance_effect = 0;			// Don't do this for middle o' game stuff.

		ObjectStartLocation = ftell(fp);
	RetryObjectLoading:
		//Clear out all the objects from the lvl file
		for (segnum = 0; segnum <= Highest_segment_index; segnum++)
			Segments[segnum].objects = -1;
		reset_objects(1);

		//Read objects, and pop 'em into their respective segments.
		//fread(&i, sizeof(int), 1, fp);
		//Highest_object_index = i - 1;
		Highest_object_index = F_ReadInt(fp) - 1;
		if (!BogusSaturnShit)
		{
			int objnum;
			//fread(Objects, sizeof(object) * i, 1, fp);
			for (objnum = 0; objnum <= Highest_object_index; objnum++)
				P_ReadObject(&Objects[objnum], fp);
		}
		else 
		{
			uint8_t tmp_object[sizeof(object)];
			memset(&tmp_object[0], 0, sizeof(object));
			for (i = 0; i <= Highest_object_index; i++) 
			{
				//fread(tmp_object, sizeof(object) - 3, 1, fp);
				P_ReadObject(&Objects[i], fp);
				fseek(fp, -3, SEEK_CUR);
				// Insert 3 bytes after the read in obj->rtype.pobj_info.alt_textures field.
				memcpy(&Objects[i], tmp_object, sizeof(object) - 3);
				Objects[i].rtype.pobj_info.alt_textures = -1;
			}
		}

		Object_next_signature = 0;
		for (i = 0; i <= Highest_object_index; i++) 
		{
			obj = &Objects[i];
			obj->rtype.pobj_info.alt_textures = -1;
			segnum = obj->segnum;
			obj->next = obj->prev = obj->segnum = -1;
			if (obj->type != OBJ_NONE) 
			{
				// Check for a bogus Saturn version!!!!
				if (!BogusSaturnShit) 
				{
					if ((segnum < 0) || (segnum > Highest_segment_index)) 
					{
						BogusSaturnShit = 1;
						mprintf((1, "READING BOGUS SATURN VERSION OBJECTS!!! (Object:%d)\n", i));
						fseek(fp, ObjectStartLocation, SEEK_SET);
						goto RetryObjectLoading;
					}
				}
				obj_link(i, segnum);
				if (obj->signature > Object_next_signature)
					Object_next_signature = obj->signature;
			}
		}
		special_reset_objects();
		Object_next_signature++;

		//Restore wall info
		//fread(&i, sizeof(int), 1, fp);
		Num_walls = F_ReadInt(fp);
		// Check for a bogus Saturn version!!!!
		if (!BogusSaturnShit) 
		{
			if ((Num_walls < 0) || (Num_walls > MAX_WALLS)) 
			{
				BogusSaturnShit = 1;
				mprintf((1, "READING BOGUS SATURN VERSION OBJECTS!!! (Num_walls)\n"));
				fseek(fp, ObjectStartLocation, SEEK_SET);
				goto RetryObjectLoading;
			}
		}

		//fread(Walls, sizeof(wall) * Num_walls, 1, fp);
		for (i = 0; i < Num_walls; i++)
		{
			P_ReadWall(&Walls[i], fp);
		}
		// Check for a bogus Saturn version!!!!
		if (!BogusSaturnShit) 
		{
			for (i = 0; i < Num_walls; i++) 
			{
				if ((Walls[i].segnum < 0) || (Walls[i].segnum > Highest_segment_index) || (Walls[i].sidenum < -1) || (Walls[i].sidenum > 5)) 
				{
					BogusSaturnShit = 1;
					mprintf((1, "READING BOGUS SATURN VERSION OBJECTS!!! (Wall %d)\n", i));
					fseek(fp, ObjectStartLocation, SEEK_SET);
					goto RetryObjectLoading;
				}
			}
		}

		//Restore door info
		//fread(&i, sizeof(int), 1, fp);
		Num_open_doors = F_ReadInt(fp);
		//fread(ActiveDoors, sizeof(active_door) * Num_open_doors, 1, fp);
		for (i = 0; i < Num_open_doors; i++)
		{
			P_ReadActiveDoor(&ActiveDoors[i], fp);
		}

		//Restore trigger info
		//fread(&Num_triggers, sizeof(int), 1, fp);
		Num_triggers = F_ReadInt(fp);
		Assert(Num_triggers < MAX_TRIGGERS);
		//fread(Triggers, sizeof(trigger) * Num_triggers, 1, fp);
		for (i = 0; i < Num_triggers; i++)
		{
			P_ReadTrigger(&Triggers[i], fp);
		}

		//Restore tmap info
		for (i = 0; i <= Highest_segment_index; i++) 
		{
			for (j = 0; j < 6; j++) 
			{
				//fread(&Segments[i].sides[j].wall_num, sizeof(short), 1, fp);
				//fread(&Segments[i].sides[j].tmap_num, sizeof(short), 1, fp);
				//fread(&Segments[i].sides[j].tmap_num2, sizeof(short), 1, fp);
				Segments[i].sides[j].wall_num = F_ReadShort(fp);
				Segments[i].sides[j].tmap_num = F_ReadShort(fp);
				Segments[i].sides[j].tmap_num2 = F_ReadShort(fp);
			}
		}

		//Restore the fuelcen info
		//fread(&Fuelcen_control_center_destroyed, sizeof(int), 1, fp);
		//fread(&Fuelcen_seconds_left, sizeof(int), 1, fp);
		Fuelcen_control_center_destroyed = F_ReadInt(fp);
		Fuelcen_seconds_left = F_ReadInt(fp);
		//fread(&Num_robot_centers, sizeof(int), 1, fp);
		Num_robot_centers = F_ReadInt(fp);
		Assert(Num_robot_centers < MAX_ROBOT_CENTERS);
		//fread(RobotCenters, sizeof(matcen_info) * Num_robot_centers, 1, fp);
		for (i = 0; i < Num_robot_centers; i++)
		{
			P_ReadMatcen(&RobotCenters[i], fp);
		}

		//fread(&ControlCenterTriggers, sizeof(control_center_triggers), 1, fp);
		P_ReadReactorTrigger(&ControlCenterTriggers, fp);
		//fread(&Num_fuelcenters, sizeof(int), 1, fp);
		Num_fuelcenters = F_ReadInt(fp);
		Assert(Num_fuelcenters < MAX_NUM_FUELCENS);
		//fread(Station, sizeof(FuelCenter) * Num_fuelcenters, 1, fp);
		for (i = 0; i < Num_fuelcenters; i++)
		{
			P_ReadFuelCenter(&Station[i], fp);
		}

		// Restore the control cen info
		//fread(&Control_center_been_hit, sizeof(int), 1, fp);
		//fread(&Control_center_player_been_seen, sizeof(int), 1, fp);
		//fread(&Control_center_next_fire_time, sizeof(int), 1, fp);
		//fread(&Control_center_present, sizeof(int), 1, fp);
		//fread(&Dead_controlcen_object_num, sizeof(int), 1, fp);
		Control_center_been_hit = F_ReadInt(fp);
		Control_center_player_been_seen = F_ReadInt(fp);
		Control_center_next_fire_time = F_ReadInt(fp);
		Control_center_present = F_ReadInt(fp);
		Dead_controlcen_object_num = F_ReadInt(fp);

		// Restore the AI state
		ai_restore_state(fp);

		// Restore the automap visited info
		fread(Automap_visited, sizeof(uint8_t) * MAX_SEGMENTS, 1, fp);

		//	Restore hacked up weapon system stuff.
		Fusion_next_sound_time = GameTime;
		Auto_fire_fusion_cannon_time = 0;
		Next_laser_fire_time = GameTime;
		Next_missile_fire_time = GameTime;
		Last_laser_fired_time = GameTime;

	}
	state_game_id = 0;

	if (version >= 7) 
	{
		int tmp_Lunacy;
		//fread(&state_game_id, sizeof(uint32_t), 1, fp);
		//fread(&Laser_rapid_fire, sizeof(int), 1, fp);
		//fread(&Ugly_robot_cheat, sizeof(int), 1, fp);
		//fread(&Ugly_robot_texture, sizeof(int), 1, fp);
		//fread(&Physics_cheat_flag, sizeof(int), 1, fp);
		//fread(&tmp_Lunacy, sizeof(int), 1, fp);
		state_game_id = F_ReadInt(fp);
		Laser_rapid_fire = F_ReadInt(fp);
		Ugly_robot_cheat = F_ReadInt(fp);
		Ugly_robot_texture = F_ReadInt(fp);
		Physics_cheat_flag = F_ReadInt(fp);
		tmp_Lunacy = F_ReadInt(fp);
		if (tmp_Lunacy)
			do_lunacy_on();
	}

	fclose(fp);

	// Load in bitmaps, etc..
	piggy_load_level_data();

	return 1;
}
#endif
