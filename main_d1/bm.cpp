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
#include "misc/types.h"
#include "inferno.h"
#include "2d/gr.h"
#include "bm.h"
#include "mem/mem.h"
#include "cfile/cfile.h"
#include "platform/mono.h"
#include "misc/error.h"
#include "object.h"
#include "vclip.h"
#include "main_shared/effects.h"
#include "polyobj.h"
#include "wall.h"
#include "textures.h"
#include "game.h"
#include "multi.h"
#include "iff/iff.h"
#include "cfile/cfile.h"
#include "hostage.h"
#include "powerup.h"
#include "sounds.h"
#include "piggy.h"
#include "aistruct.h"
#include "robot.h"
#include "weapon.h"
#include "gauges.h"
#include "player.h"
#include "fuelcen.h"
#include "endlevel.h"
#include "cntrlcen.h"

#ifdef EDITOR
#define FIX_STARTS
#endif

uint8_t Sounds[MAX_SOUNDS];
uint8_t AltSounds[MAX_SOUNDS];

int Num_total_object_types;

int8_t	ObjType[MAX_OBJTYPE];
int8_t	ObjId[MAX_OBJTYPE];
fix	ObjStrength[MAX_OBJTYPE];

//for each model, a model number for dying & dead variants, or -1 if none
int Dying_modelnums[MAX_POLYGON_MODELS];
int Dead_modelnums[MAX_POLYGON_MODELS];

//right now there's only one player ship, but we can have another by 
//adding an array and setting the pointer to the active ship.
player_ship only_player_ship, * Player_ship = &only_player_ship;

//----------------- Miscellaneous bitmap pointers ---------------
int					Num_cockpits = 0;
bitmap_index		cockpit_bitmap[N_COCKPIT_BITMAPS];

//---------------- Variables for wall textures ------------------
int 					Num_tmaps;
tmap_info 			TmapInfo[MAX_TEXTURES];

//---------------- Variables for object textures ----------------

int					First_multi_bitmap_num = -1;

bitmap_index		ObjBitmaps[MAX_OBJ_BITMAPS];
uint16_t				ObjBitmapPtrs[MAX_OBJ_BITMAPS];		// These point back into ObjBitmaps, since some are used twice.

//-----------------------------------------------------------------
// Initializes all bitmaps from BITMAPS.TBL file.
int bm_init()
{
	init_polygon_models();
	piggy_init();				// This calls bm_read_all
	piggy_read_sounds();
	return 0;
}

//[ISB] Mac-inspired element readers
void read_tmap_info(CFILE* fp)
{
	int i;

	for (i = 0; i < MAX_TEXTURES; i++) 
	{
		cfread(&TmapInfo[i].filename[0], 13, 1, fp);
		TmapInfo[i].flags = cfile_read_byte(fp);
		TmapInfo[i].lighting = cfile_read_fix(fp);
		TmapInfo[i].damage = cfile_read_fix(fp);
		TmapInfo[i].eclip_num = cfile_read_int(fp);
	}
}

void write_tmap_info(FILE* fp)
{
	int i;

	for (i = 0; i < MAX_TEXTURES; i++)
	{
		fwrite(&TmapInfo[i].filename[0], 13, 1, fp);
		file_write_byte(fp, TmapInfo[i].flags);
		file_write_int(fp, TmapInfo[i].lighting);
		file_write_int(fp, TmapInfo[i].damage);
		file_write_int(fp, TmapInfo[i].eclip_num);
	}
}

void read_vclip_info(CFILE* fp)
{
	int i, j;

	for (i = 0; i < VCLIP_MAXNUM; i++) 
	{
		Vclip[i].play_time = cfile_read_fix(fp);
		Vclip[i].num_frames = cfile_read_int(fp);
		Vclip[i].frame_time = cfile_read_fix(fp);
		Vclip[i].flags = cfile_read_int(fp);
		Vclip[i].sound_num = cfile_read_short(fp);
		for (j = 0; j < VCLIP_MAX_FRAMES; j++)
			Vclip[i].frames[j].index = cfile_read_short(fp);
		Vclip[i].light_value = cfile_read_fix(fp);
	}
}

void write_vclip_info(FILE* fp)
{
	int i, j;

	for (i = 0; i < VCLIP_MAXNUM; i++)
	{
		file_write_int(fp, Vclip[i].play_time);
		file_write_int(fp, Vclip[i].num_frames);
		file_write_int(fp, Vclip[i].frame_time);
		file_write_int(fp, Vclip[i].flags);
		file_write_short(fp, Vclip[i].sound_num);
		for (j = 0; j < VCLIP_MAX_FRAMES; j++)
			file_write_short(fp, Vclip[i].frames[j].index);
		file_write_int(fp, Vclip[i].light_value);
	}
}

void read_effect_info(CFILE* fp)
{
	int i, j;

	for (i = 0; i < MAX_EFFECTS; i++)
	{
		Effects[i].vc.play_time = cfile_read_fix(fp);
		Effects[i].vc.num_frames = cfile_read_int(fp);
		Effects[i].vc.frame_time = cfile_read_fix(fp);
		Effects[i].vc.flags = cfile_read_int(fp);
		Effects[i].vc.sound_num = cfile_read_short(fp);
		for (j = 0; j < VCLIP_MAX_FRAMES; j++)
			Effects[i].vc.frames[j].index = cfile_read_short(fp);
		Effects[i].vc.light_value = cfile_read_fix(fp);
		Effects[i].time_left = cfile_read_fix(fp);
		Effects[i].frame_count = cfile_read_int(fp);
		Effects[i].changing_wall_texture = cfile_read_short(fp);
		Effects[i].changing_object_texture = cfile_read_short(fp);
		Effects[i].flags = cfile_read_int(fp);
		Effects[i].crit_clip = cfile_read_int(fp);
		Effects[i].dest_bm_num = cfile_read_int(fp);
		Effects[i].dest_vclip = cfile_read_int(fp);
		Effects[i].dest_eclip = cfile_read_int(fp);
		Effects[i].dest_size = cfile_read_fix(fp);
		Effects[i].sound_num = cfile_read_int(fp);
		Effects[i].segnum = cfile_read_int(fp);
		Effects[i].sidenum = cfile_read_int(fp);
	}
}

void write_effect_info(FILE* fp)
{
	int i, j;

	for (i = 0; i < MAX_EFFECTS; i++)
	{
		file_write_int(fp, Effects[i].vc.play_time);
		file_write_int(fp, Effects[i].vc.num_frames);
		file_write_int(fp, Effects[i].vc.frame_time);
		file_write_int(fp, Effects[i].vc.flags);
		file_write_short(fp, Effects[i].vc.sound_num);
		for (j = 0; j < VCLIP_MAX_FRAMES; j++)
			file_write_short(fp, Effects[i].vc.frames[j].index);
		file_write_int(fp, Effects[i].vc.light_value);
		file_write_int(fp, Effects[i].time_left);
		file_write_int(fp, Effects[i].frame_count);
		file_write_short(fp, Effects[i].changing_wall_texture);
		file_write_short(fp, Effects[i].changing_object_texture);
		file_write_int(fp, Effects[i].flags);
		file_write_int(fp, Effects[i].crit_clip);
		file_write_int(fp, Effects[i].dest_bm_num);
		file_write_int(fp, Effects[i].dest_vclip);
		file_write_int(fp, Effects[i].dest_eclip);
		file_write_int(fp, Effects[i].dest_size);
		file_write_int(fp, Effects[i].sound_num);
		file_write_int(fp, Effects[i].segnum);
		file_write_int(fp, Effects[i].sidenum);
	}
}

void read_wallanim_info(CFILE* fp)
{
	int i, j;

	for (i = 0; i < MAX_WALL_ANIMS; i++) 
	{
		WallAnims[i].play_time = cfile_read_fix(fp);;
		WallAnims[i].num_frames = cfile_read_short(fp);;
		for (j = 0; j < MAX_CLIP_FRAMES; j++)
			WallAnims[i].frames[j] = cfile_read_short(fp);
		WallAnims[i].open_sound = cfile_read_short(fp);
		WallAnims[i].close_sound = cfile_read_short(fp);
		WallAnims[i].flags = cfile_read_short(fp);
		cfread(&WallAnims[i].filename[0], 13, 1, fp);
		WallAnims[i].pad = cfile_read_byte(fp);
	}
}

void write_wallanim_info(FILE* fp)
{
	int i, j;

	for (i = 0; i < MAX_WALL_ANIMS; i++)
	{
		file_write_int(fp, WallAnims[i].play_time);
		file_write_short(fp, WallAnims[i].num_frames);
		for (j = 0; j < MAX_CLIP_FRAMES; j++)
			file_write_short(fp, WallAnims[i].frames[j]);
		file_write_short(fp, WallAnims[i].open_sound);
		file_write_short(fp, WallAnims[i].close_sound);
		file_write_short(fp, WallAnims[i].flags);
		fwrite(&WallAnims[i].filename[0], 13, 1, fp);
		file_write_byte(fp, WallAnims[i].pad);
	}
}

void read_robot_info(CFILE* fp)
{
	int i, j, k;

	for (i = 0; i < MAX_ROBOT_TYPES; i++) 
	{
		Robot_info[i].model_num = cfile_read_int(fp);
		Robot_info[i].n_guns = cfile_read_int(fp);
		for (j = 0; j < MAX_GUNS; j++) 
		{
			Robot_info[i].gun_points[j].x = cfile_read_fix(fp);
			Robot_info[i].gun_points[j].y = cfile_read_fix(fp);
			Robot_info[i].gun_points[j].z = cfile_read_fix(fp);
		}
		for (j = 0; j < MAX_GUNS; j++)
			Robot_info[i].gun_submodels[j] = cfile_read_byte(fp);
		Robot_info[i].exp1_vclip_num = cfile_read_short(fp);
		Robot_info[i].exp1_sound_num = cfile_read_short(fp);
		Robot_info[i].exp2_vclip_num = cfile_read_short(fp);
		Robot_info[i].exp2_sound_num = cfile_read_short(fp);
		Robot_info[i].weapon_type = cfile_read_short(fp);
		Robot_info[i].contains_id = cfile_read_byte(fp);
		Robot_info[i].contains_count = cfile_read_byte(fp);
		Robot_info[i].contains_prob = cfile_read_byte(fp);
		Robot_info[i].contains_type = cfile_read_byte(fp);
		Robot_info[i].score_value = cfile_read_int(fp);
		Robot_info[i].lighting = cfile_read_fix(fp);
		Robot_info[i].strength = cfile_read_fix(fp);
		Robot_info[i].mass = cfile_read_fix(fp);
		Robot_info[i].drag = cfile_read_fix(fp);
		for (j = 0; j < NDL; j++)
			Robot_info[i].field_of_view[j] = cfile_read_fix(fp);
		for (j = 0; j < NDL; j++)
			Robot_info[i].firing_wait[j] = cfile_read_fix(fp);
		for (j = 0; j < NDL; j++)
			Robot_info[i].turn_time[j] = cfile_read_fix(fp);
		for (j = 0; j < NDL; j++)
			Robot_info[i].fire_power[j] = cfile_read_fix(fp);
		for (j = 0; j < NDL; j++)
			Robot_info[i].shield[j] = cfile_read_fix(fp);
		for (j = 0; j < NDL; j++)
			Robot_info[i].max_speed[j] = cfile_read_fix(fp);
		for (j = 0; j < NDL; j++)
			Robot_info[i].circle_distance[j] = cfile_read_fix(fp);
		for (j = 0; j < NDL; j++)
			Robot_info[i].rapidfire_count[j] = cfile_read_byte(fp);
		for (j = 0; j < NDL; j++)
			Robot_info[i].evade_speed[j] = cfile_read_byte(fp);
		Robot_info[i].cloak_type = cfile_read_byte(fp);
		Robot_info[i].attack_type = cfile_read_byte(fp);
		Robot_info[i].boss_flag = cfile_read_byte(fp);
		Robot_info[i].see_sound = cfile_read_byte(fp);
		Robot_info[i].attack_sound = cfile_read_byte(fp);
		Robot_info[i].claw_sound = cfile_read_byte(fp);

		for (j = 0; j < MAX_GUNS + 1; j++) 
		{
			for (k = 0; k < N_ANIM_STATES; k++) 
			{
				Robot_info[i].anim_states[j][k].n_joints = cfile_read_short(fp);
				Robot_info[i].anim_states[j][k].offset = cfile_read_short(fp);
			}
		}

		Robot_info[i].always_0xabcd = cfile_read_int(fp);
	}
}

void write_robot_info(FILE* fp)
{
	int i, j, k;

	for (i = 0; i < MAX_ROBOT_TYPES; i++)
	{
		file_write_int(fp, Robot_info[i].model_num);
		file_write_int(fp, Robot_info[i].n_guns);
		for (j = 0; j < MAX_GUNS; j++)
		{
			file_write_int(fp, Robot_info[i].gun_points[j].x);
			file_write_int(fp, Robot_info[i].gun_points[j].y);
			file_write_int(fp, Robot_info[i].gun_points[j].z);
		}
		for (j = 0; j < MAX_GUNS; j++)
			file_write_byte(fp, Robot_info[i].gun_submodels[j]);
		file_write_short(fp, Robot_info[i].exp1_vclip_num);
		file_write_short(fp, Robot_info[i].exp1_sound_num);
		file_write_short(fp, Robot_info[i].exp2_vclip_num);
		file_write_short(fp, Robot_info[i].exp2_sound_num);
		file_write_short(fp, Robot_info[i].weapon_type);
		file_write_byte(fp, Robot_info[i].contains_id);
		file_write_byte(fp, Robot_info[i].contains_count);
		file_write_byte(fp, Robot_info[i].contains_prob);
		file_write_byte(fp, Robot_info[i].contains_type);
		file_write_int(fp, Robot_info[i].score_value);
		file_write_int(fp, Robot_info[i].lighting);
		file_write_int(fp, Robot_info[i].strength);
		file_write_int(fp, Robot_info[i].mass);
		file_write_int(fp, Robot_info[i].drag);
		for (j = 0; j < NDL; j++)
			file_write_int(fp, Robot_info[i].field_of_view[j]);
		for (j = 0; j < NDL; j++)
			file_write_int(fp, Robot_info[i].firing_wait[j]);
		for (j = 0; j < NDL; j++)
			file_write_int(fp, Robot_info[i].turn_time[j]);
		for (j = 0; j < NDL; j++)
			file_write_int(fp, Robot_info[i].fire_power[j]);
		for (j = 0; j < NDL; j++)
			file_write_int(fp, Robot_info[i].shield[j]);
		for (j = 0; j < NDL; j++)
			file_write_int(fp, Robot_info[i].max_speed[j]);
		for (j = 0; j < NDL; j++)
			file_write_int(fp, Robot_info[i].circle_distance[j]);
		for (j = 0; j < NDL; j++)
			file_write_byte(fp, Robot_info[i].rapidfire_count[j]);
		for (j = 0; j < NDL; j++)
			file_write_byte(fp, Robot_info[i].evade_speed[j]);
		file_write_byte(fp, Robot_info[i].cloak_type);
		file_write_byte(fp, Robot_info[i].attack_type);
		file_write_byte(fp, Robot_info[i].boss_flag);
		file_write_byte(fp, Robot_info[i].see_sound);
		file_write_byte(fp, Robot_info[i].attack_sound);
		file_write_byte(fp, Robot_info[i].claw_sound);

		for (j = 0; j < MAX_GUNS + 1; j++)
		{
			for (k = 0; k < N_ANIM_STATES; k++)
			{
				file_write_short(fp, Robot_info[i].anim_states[j][k].n_joints);
				file_write_short(fp, Robot_info[i].anim_states[j][k].offset);
			}
		}

		file_write_int(fp, Robot_info[i].always_0xabcd);
	}
}


void read_robot_joints_info(CFILE* fp)
{
	int i;

	for (i = 0; i < MAX_ROBOT_JOINTS; i++)
	{
		Robot_joints[i].jointnum = cfile_read_short(fp);
		Robot_joints[i].angles.p = cfile_read_short(fp);
		Robot_joints[i].angles.b = cfile_read_short(fp);
		Robot_joints[i].angles.h = cfile_read_short(fp);
	}
}

void write_robot_joints_info(FILE* fp)
{
	int i;

	for (i = 0; i < MAX_ROBOT_JOINTS; i++)
	{
		file_write_short(fp, Robot_joints[i].jointnum);
		file_write_short(fp, Robot_joints[i].angles.p);
		file_write_short(fp, Robot_joints[i].angles.b);
		file_write_short(fp, Robot_joints[i].angles.h);
	}
}

void read_weapon_info(CFILE* fp)
{
	int i, j;

	for (i = 0; i < MAX_WEAPON_TYPES; i++)
	{
		Weapon_info[i].render_type = cfile_read_byte(fp);
		Weapon_info[i].model_num = cfile_read_byte(fp);
		Weapon_info[i].model_num_inner = cfile_read_byte(fp);
		Weapon_info[i].persistent = cfile_read_byte(fp);
		Weapon_info[i].flash_vclip = cfile_read_byte(fp);
		Weapon_info[i].flash_sound = cfile_read_short(fp);
		Weapon_info[i].robot_hit_vclip = cfile_read_byte(fp);
		Weapon_info[i].robot_hit_sound = cfile_read_short(fp);
		Weapon_info[i].wall_hit_vclip = cfile_read_byte(fp);
		Weapon_info[i].wall_hit_sound = cfile_read_short(fp);
		Weapon_info[i].fire_count = cfile_read_byte(fp);
		Weapon_info[i].ammo_usage = cfile_read_byte(fp);
		Weapon_info[i].weapon_vclip = cfile_read_byte(fp);
		Weapon_info[i].destroyable = cfile_read_byte(fp);
		Weapon_info[i].matter = cfile_read_byte(fp);
		Weapon_info[i].bounce = cfile_read_byte(fp);
		Weapon_info[i].homing_flag = cfile_read_byte(fp);
		Weapon_info[i].dum1 = cfile_read_byte(fp);
		Weapon_info[i].dum2 = cfile_read_byte(fp);
		Weapon_info[i].dum3 = cfile_read_byte(fp);
		Weapon_info[i].energy_usage = cfile_read_fix(fp);
		Weapon_info[i].fire_wait = cfile_read_fix(fp);
		Weapon_info[i].bitmap.index = cfile_read_short(fp);	// bitmap_index = short
		Weapon_info[i].blob_size = cfile_read_fix(fp);
		Weapon_info[i].flash_size = cfile_read_fix(fp);
		Weapon_info[i].impact_size = cfile_read_fix(fp);
		for (j = 0; j < NDL; j++)
			Weapon_info[i].strength[j] = cfile_read_fix(fp);
		for (j = 0; j < NDL; j++)
			Weapon_info[i].speed[j] = cfile_read_fix(fp);
		Weapon_info[i].mass = cfile_read_fix(fp);
		Weapon_info[i].drag = cfile_read_fix(fp);
		Weapon_info[i].thrust = cfile_read_fix(fp);

		Weapon_info[i].po_len_to_width_ratio = cfile_read_fix(fp);
		Weapon_info[i].light = cfile_read_fix(fp);
		Weapon_info[i].lifetime = cfile_read_fix(fp);
		Weapon_info[i].damage_radius = cfile_read_fix(fp);
		Weapon_info[i].picture.index = cfile_read_short(fp);		// bitmap_index is a short
	}
}

void write_weapon_info(FILE* fp)
{
	int i, j;

	for (i = 0; i < MAX_WEAPON_TYPES; i++)
	{
		file_write_byte(fp, Weapon_info[i].render_type);
		file_write_byte(fp, Weapon_info[i].model_num);
		file_write_byte(fp, Weapon_info[i].model_num_inner);
		file_write_byte(fp, Weapon_info[i].persistent);
		file_write_byte(fp, Weapon_info[i].flash_vclip);
		file_write_short(fp, Weapon_info[i].flash_sound);
		file_write_byte(fp, Weapon_info[i].robot_hit_vclip);
		file_write_short(fp, Weapon_info[i].robot_hit_sound);
		file_write_byte(fp, Weapon_info[i].wall_hit_vclip);
		file_write_short(fp, Weapon_info[i].wall_hit_sound);
		file_write_byte(fp, Weapon_info[i].fire_count);
		file_write_byte(fp, Weapon_info[i].ammo_usage);
		file_write_byte(fp, Weapon_info[i].weapon_vclip);
		file_write_byte(fp, Weapon_info[i].destroyable);
		file_write_byte(fp, Weapon_info[i].matter);
		file_write_byte(fp, Weapon_info[i].bounce);
		file_write_byte(fp, Weapon_info[i].homing_flag);
		file_write_byte(fp, Weapon_info[i].dum1);
		file_write_byte(fp, Weapon_info[i].dum2);
		file_write_byte(fp, Weapon_info[i].dum3);
		file_write_int(fp, Weapon_info[i].energy_usage);
		file_write_int(fp, Weapon_info[i].fire_wait);
		file_write_short(fp, Weapon_info[i].bitmap.index);	// bitmap_index = short
		file_write_int(fp, Weapon_info[i].blob_size);
		file_write_int(fp, Weapon_info[i].flash_size);
		file_write_int(fp, Weapon_info[i].impact_size);
		for (j = 0; j < NDL; j++)
			file_write_int(fp, Weapon_info[i].strength[j]);
		for (j = 0; j < NDL; j++)
			file_write_int(fp, Weapon_info[i].speed[j]);
		file_write_int(fp, Weapon_info[i].mass);
		file_write_int(fp, Weapon_info[i].drag);
		file_write_int(fp, Weapon_info[i].thrust);

		file_write_int(fp, Weapon_info[i].po_len_to_width_ratio);
		file_write_int(fp, Weapon_info[i].light);
		file_write_int(fp, Weapon_info[i].lifetime);
		file_write_int(fp, Weapon_info[i].damage_radius);
		file_write_short(fp, Weapon_info[i].picture.index);		// bitmap_index is a short
	}
}

void read_powerup_info(CFILE* fp)
{
	int i;

	for (i = 0; i < MAX_POWERUP_TYPES; i++) 
	{
		Powerup_info[i].vclip_num = cfile_read_int(fp);
		Powerup_info[i].hit_sound = cfile_read_int(fp);
		Powerup_info[i].size = cfile_read_fix(fp);
		Powerup_info[i].light = cfile_read_fix(fp);
	}
}

void write_powerup_info(FILE* fp)
{
	int i;

	for (i = 0; i < MAX_POWERUP_TYPES; i++)
	{
		file_write_int(fp, Powerup_info[i].vclip_num);
		file_write_int(fp, Powerup_info[i].hit_sound);
		file_write_int(fp, Powerup_info[i].size);
		file_write_int(fp, Powerup_info[i].light);
	}
}

void read_polygon_models(CFILE* fp)
{
	int i, j;

	if (N_polygon_models > MAX_POLYGON_MODELS)
	{
		Error("Too many polymodels in piggy file!");
		return;
	}

	//[ISB] however it's supposed to only fill out so many...
	for (i = 0; i < N_polygon_models; i++) 
	{
		Polygon_models[i].n_models = cfile_read_int(fp);
		Polygon_models[i].model_data_size = cfile_read_int(fp);
		Polygon_models[i].model_data = (uint8_t*)cfile_read_int(fp);
		for (j = 0; j < MAX_SUBMODELS; j++)
			Polygon_models[i].submodel_ptrs[j] = cfile_read_int(fp);
		for (j = 0; j < MAX_SUBMODELS; j++) 
		{
			Polygon_models[i].submodel_offsets[j].x = cfile_read_fix(fp);
			Polygon_models[i].submodel_offsets[j].y = cfile_read_fix(fp);
			Polygon_models[i].submodel_offsets[j].z = cfile_read_fix(fp);
		}
		for (j = 0; j < MAX_SUBMODELS; j++) 
		{
			Polygon_models[i].submodel_norms[j].x = cfile_read_fix(fp);
			Polygon_models[i].submodel_norms[j].y = cfile_read_fix(fp);
			Polygon_models[i].submodel_norms[j].z = cfile_read_fix(fp);
		}
		for (j = 0; j < MAX_SUBMODELS; j++) 
		{
			Polygon_models[i].submodel_pnts[j].x = cfile_read_fix(fp);
			Polygon_models[i].submodel_pnts[j].y = cfile_read_fix(fp);
			Polygon_models[i].submodel_pnts[j].z = cfile_read_fix(fp);
		}
		for (j = 0; j < MAX_SUBMODELS; j++)
			Polygon_models[i].submodel_rads[j] = cfile_read_fix(fp);
		for (j = 0; j < MAX_SUBMODELS; j++)
			Polygon_models[i].submodel_parents[j] = cfile_read_byte(fp);
		for (j = 0; j < MAX_SUBMODELS; j++) 
		{
			Polygon_models[i].submodel_mins[j].x = cfile_read_fix(fp);
			Polygon_models[i].submodel_mins[j].y = cfile_read_fix(fp);
			Polygon_models[i].submodel_mins[j].z = cfile_read_fix(fp);
		}
		for (j = 0; j < MAX_SUBMODELS; j++) 
		{
			Polygon_models[i].submodel_maxs[j].x = cfile_read_fix(fp);
			Polygon_models[i].submodel_maxs[j].y = cfile_read_fix(fp);
			Polygon_models[i].submodel_maxs[j].z = cfile_read_fix(fp);
		}
		Polygon_models[i].mins.x = cfile_read_fix(fp);
		Polygon_models[i].mins.y = cfile_read_fix(fp);
		Polygon_models[i].mins.z = cfile_read_fix(fp);
		Polygon_models[i].maxs.x = cfile_read_fix(fp);
		Polygon_models[i].maxs.y = cfile_read_fix(fp);
		Polygon_models[i].maxs.z = cfile_read_fix(fp);
		Polygon_models[i].rad = cfile_read_fix(fp);
		Polygon_models[i].n_textures = cfile_read_byte(fp);
		Polygon_models[i].first_texture = cfile_read_short(fp);
		Polygon_models[i].simpler_model = cfile_read_byte(fp);
	}
}

void write_polygon_models(FILE* fp)
{
	int i, j;

	Assert(N_polygon_models < MAX_POLYGON_MODELS); //avoid trashing memory I guess

	//[ISB] however it's supposed to only fill out so many...
	for (i = 0; i < N_polygon_models; i++)
	{
		file_write_int(fp, Polygon_models[i].n_models);
		file_write_int(fp, Polygon_models[i].model_data_size);
		file_write_int(fp, (uintptr_t)Polygon_models[i].model_data);
		for (j = 0; j < MAX_SUBMODELS; j++)
			file_write_int(fp, Polygon_models[i].submodel_ptrs[j]);
		for (j = 0; j < MAX_SUBMODELS; j++)
		{
			file_write_int(fp, Polygon_models[i].submodel_offsets[j].x);
			file_write_int(fp, Polygon_models[i].submodel_offsets[j].y);
			file_write_int(fp, Polygon_models[i].submodel_offsets[j].z);
		}
		for (j = 0; j < MAX_SUBMODELS; j++)
		{
			file_write_int(fp, Polygon_models[i].submodel_norms[j].x);
			file_write_int(fp, Polygon_models[i].submodel_norms[j].y);
			file_write_int(fp, Polygon_models[i].submodel_norms[j].z);
		}
		for (j = 0; j < MAX_SUBMODELS; j++)
		{
			file_write_int(fp, Polygon_models[i].submodel_pnts[j].x);
			file_write_int(fp, Polygon_models[i].submodel_pnts[j].y);
			file_write_int(fp, Polygon_models[i].submodel_pnts[j].z);
		}
		for (j = 0; j < MAX_SUBMODELS; j++)
			file_write_int(fp, Polygon_models[i].submodel_rads[j]);
		for (j = 0; j < MAX_SUBMODELS; j++)
			file_write_byte(fp, Polygon_models[i].submodel_parents[j]);
		for (j = 0; j < MAX_SUBMODELS; j++)
		{
			file_write_int(fp, Polygon_models[i].submodel_mins[j].x);
			file_write_int(fp, Polygon_models[i].submodel_mins[j].y);
			file_write_int(fp, Polygon_models[i].submodel_mins[j].z);
		}
		for (j = 0; j < MAX_SUBMODELS; j++)
		{
			file_write_int(fp, Polygon_models[i].submodel_maxs[j].x);
			file_write_int(fp, Polygon_models[i].submodel_maxs[j].y);
			file_write_int(fp, Polygon_models[i].submodel_maxs[j].z);
		}
		file_write_int(fp, Polygon_models[i].mins.x);
		file_write_int(fp, Polygon_models[i].mins.y);
		file_write_int(fp, Polygon_models[i].mins.z);
		file_write_int(fp, Polygon_models[i].maxs.x);
		file_write_int(fp, Polygon_models[i].maxs.y);
		file_write_int(fp, Polygon_models[i].maxs.z);
		file_write_int(fp, Polygon_models[i].rad);
		file_write_byte(fp, Polygon_models[i].n_textures);
		file_write_short(fp, Polygon_models[i].first_texture);
		file_write_byte(fp, Polygon_models[i].simpler_model);
	}
}

void read_player_ship(CFILE* fp)
{
	int i;
	only_player_ship.model_num = cfile_read_int(fp);
	only_player_ship.expl_vclip_num = cfile_read_int(fp);
	only_player_ship.mass = cfile_read_fix(fp);
	only_player_ship.drag = cfile_read_fix(fp);
	only_player_ship.max_thrust = cfile_read_fix(fp);
	only_player_ship.reverse_thrust = cfile_read_fix(fp);
	only_player_ship.brakes = cfile_read_fix(fp);
	only_player_ship.wiggle = cfile_read_fix(fp);
	only_player_ship.max_rotthrust = cfile_read_fix(fp);
	for (i = 0; i < N_PLAYER_GUNS; i++)
	{
		only_player_ship.gun_points[i].x = cfile_read_fix(fp);
		only_player_ship.gun_points[i].y = cfile_read_fix(fp);
		only_player_ship.gun_points[i].z = cfile_read_fix(fp);
	}
}

void write_player_ship(FILE* fp)
{
	int i;
	file_write_int(fp, only_player_ship.model_num);
	file_write_int(fp, only_player_ship.expl_vclip_num);
	file_write_int(fp, only_player_ship.mass);
	file_write_int(fp, only_player_ship.drag);
	file_write_int(fp, only_player_ship.max_thrust);
	file_write_int(fp, only_player_ship.reverse_thrust);
	file_write_int(fp, only_player_ship.brakes);
	file_write_int(fp, only_player_ship.wiggle);
	file_write_int(fp, only_player_ship.max_rotthrust);
	for (i = 0; i < N_PLAYER_GUNS; i++)
	{
		file_write_int(fp, only_player_ship.gun_points[i].x);
		file_write_int(fp, only_player_ship.gun_points[i].y);
		file_write_int(fp, only_player_ship.gun_points[i].z);
	}
}


void bm_read_all(CFILE* fp)
{
	int i;// , j, k;

	//  bitmap_index is a short

	NumTextures = cfile_read_int(fp);
	for (i = 0; i < MAX_TEXTURES; i++)
		Textures[i].index = cfile_read_short(fp);

	read_tmap_info(fp);

	cfread(Sounds, sizeof(uint8_t), MAX_SOUNDS, fp);
	cfread(AltSounds, sizeof(uint8_t), MAX_SOUNDS, fp);

	Num_vclips = cfile_read_int(fp);
	read_vclip_info(fp);

	Num_effects = cfile_read_int(fp);
	read_effect_info(fp);

	Num_wall_anims = cfile_read_int(fp);
	read_wallanim_info(fp);

	N_robot_types = cfile_read_int(fp);
	read_robot_info(fp);

	N_robot_joints = cfile_read_int(fp);
	read_robot_joints_info(fp);

	N_weapon_types = cfile_read_int(fp);
	read_weapon_info(fp);

	N_powerup_types = cfile_read_int(fp);
	read_powerup_info(fp);

	N_polygon_models = cfile_read_int(fp);
	read_polygon_models(fp);

	for (i = 0; i < N_polygon_models; i++) 
	{
		Polygon_models[i].model_data = (uint8_t*)malloc(Polygon_models[i].model_data_size);
		Assert(Polygon_models[i].model_data != NULL);
		cfread(Polygon_models[i].model_data, sizeof(uint8_t), Polygon_models[i].model_data_size, fp);
	}

	for (i = 0; i < MAX_GAUGE_BMS; i++)
		Gauges[i].index = cfile_read_short(fp);

	for (i = 0; i < MAX_POLYGON_MODELS; i++)
		Dying_modelnums[i] = cfile_read_int(fp);
	for (i = 0; i < MAX_POLYGON_MODELS; i++)
		Dead_modelnums[i] = cfile_read_int(fp);

	for (i = 0; i < MAX_OBJ_BITMAPS; i++)
		ObjBitmaps[i].index = cfile_read_short(fp);
	for (i = 0; i < MAX_OBJ_BITMAPS; i++)
		ObjBitmapPtrs[i] = cfile_read_short(fp);

	read_player_ship(fp);

	Num_cockpits = cfile_read_int(fp);
	for (i = 0; i < N_COCKPIT_BITMAPS; i++)
		cockpit_bitmap[i].index = cfile_read_short(fp);
	//[ISB] there's a story behind this, isn't there... who was drunk...
	cfread(Sounds, sizeof(uint8_t), MAX_SOUNDS, fp);
	cfread(AltSounds, sizeof(uint8_t), MAX_SOUNDS, fp);

	Num_total_object_types = cfile_read_int(fp);
	cfread(ObjType, sizeof(int8_t), MAX_OBJTYPE, fp);
	cfread(ObjId, sizeof(int8_t), MAX_OBJTYPE, fp);
	for (i = 0; i < MAX_OBJTYPE; i++)
		ObjStrength[i] = cfile_read_fix(fp);

	First_multi_bitmap_num = cfile_read_int(fp);
	N_controlcen_guns = cfile_read_int(fp);

	for (i = 0; i < MAX_CONTROLCEN_GUNS; i++) 
	{
		controlcen_gun_points[i].x = cfile_read_fix(fp);
		controlcen_gun_points[i].y = cfile_read_fix(fp);
		controlcen_gun_points[i].z = cfile_read_fix(fp);
	}
	for (i = 0; i < MAX_CONTROLCEN_GUNS; i++) 
	{
		controlcen_gun_dirs[i].x = cfile_read_fix(fp);
		controlcen_gun_dirs[i].y = cfile_read_fix(fp);
		controlcen_gun_dirs[i].z = cfile_read_fix(fp);
	}

	exit_modelnum = cfile_read_int(fp);
	destroyed_exit_modelnum = cfile_read_int(fp);

#ifdef FIX_STARS
	init_endlevel();
#endif

	//[ISB] I'm normally not huge about changing game logic, but this isn't user facing most of the time, so lets do it anyways
	//This code will make the editor work better when run without parsing bitmaps.tbl. 
#ifdef EDITOR
	N_hostage_types++;
	Hostage_vclip_num[0] = 33;
	//Construct TMAP list
	for (i = 0; i < NumTextures; i++)
	{
		if (i >= WallAnims[0].frames[0]) //Don't include doors
		{
			break;
		}
		TmapList[i] = i;
		Num_tmaps++;
	}
#endif
}
