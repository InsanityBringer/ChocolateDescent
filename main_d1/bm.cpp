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
#include "effects.h"
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

#define CF_ReadFix(a) ((fix)CF_ReadInt(a))

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
		TmapInfo[i].flags = CF_ReadByte(fp);
		TmapInfo[i].lighting = CF_ReadFix(fp);
		TmapInfo[i].damage = CF_ReadFix(fp);
		TmapInfo[i].eclip_num = CF_ReadInt(fp);
	}
}

void write_tmap_info(FILE* fp)
{
	int i;

	for (i = 0; i < MAX_TEXTURES; i++)
	{
		fwrite(&TmapInfo[i].filename[0], 13, 1, fp);
		F_WriteByte(fp, TmapInfo[i].flags);
		F_WriteInt(fp, TmapInfo[i].lighting);
		F_WriteInt(fp, TmapInfo[i].damage);
		F_WriteInt(fp, TmapInfo[i].eclip_num);
	}
}

void read_vclip_info(CFILE* fp)
{
	int i, j;

	for (i = 0; i < VCLIP_MAXNUM; i++) 
	{
		Vclip[i].play_time = CF_ReadFix(fp);
		Vclip[i].num_frames = CF_ReadInt(fp);
		Vclip[i].frame_time = CF_ReadFix(fp);
		Vclip[i].flags = CF_ReadInt(fp);
		Vclip[i].sound_num = CF_ReadShort(fp);
		for (j = 0; j < VCLIP_MAX_FRAMES; j++)
			Vclip[i].frames[j].index = CF_ReadShort(fp);
		Vclip[i].light_value = CF_ReadFix(fp);
	}
}

void write_vclip_info(FILE* fp)
{
	int i, j;

	for (i = 0; i < VCLIP_MAXNUM; i++)
	{
		F_WriteInt(fp, Vclip[i].play_time);
		F_WriteInt(fp, Vclip[i].num_frames);
		F_WriteInt(fp, Vclip[i].frame_time);
		F_WriteInt(fp, Vclip[i].flags);
		F_WriteShort(fp, Vclip[i].sound_num);
		for (j = 0; j < VCLIP_MAX_FRAMES; j++)
			F_WriteShort(fp, Vclip[i].frames[j].index);
		F_WriteInt(fp, Vclip[i].light_value);
	}
}

void read_effect_info(CFILE* fp)
{
	int i, j;

	for (i = 0; i < MAX_EFFECTS; i++)
	{
		Effects[i].vc.play_time = CF_ReadFix(fp);
		Effects[i].vc.num_frames = CF_ReadInt(fp);
		Effects[i].vc.frame_time = CF_ReadFix(fp);
		Effects[i].vc.flags = CF_ReadInt(fp);
		Effects[i].vc.sound_num = CF_ReadShort(fp);
		for (j = 0; j < VCLIP_MAX_FRAMES; j++)
			Effects[i].vc.frames[j].index = CF_ReadShort(fp);
		Effects[i].vc.light_value = CF_ReadFix(fp);
		Effects[i].time_left = CF_ReadFix(fp);
		Effects[i].frame_count = CF_ReadInt(fp);
		Effects[i].changing_wall_texture = CF_ReadShort(fp);
		Effects[i].changing_object_texture = CF_ReadShort(fp);
		Effects[i].flags = CF_ReadInt(fp);
		Effects[i].crit_clip = CF_ReadInt(fp);
		Effects[i].dest_bm_num = CF_ReadInt(fp);
		Effects[i].dest_vclip = CF_ReadInt(fp);
		Effects[i].dest_eclip = CF_ReadInt(fp);
		Effects[i].dest_size = CF_ReadFix(fp);
		Effects[i].sound_num = CF_ReadInt(fp);
		Effects[i].segnum = CF_ReadInt(fp);
		Effects[i].sidenum = CF_ReadInt(fp);
	}
}

void write_effect_info(FILE* fp)
{
	int i, j;

	for (i = 0; i < MAX_EFFECTS; i++)
	{
		F_WriteInt(fp, Effects[i].vc.play_time);
		F_WriteInt(fp, Effects[i].vc.num_frames);
		F_WriteInt(fp, Effects[i].vc.frame_time);
		F_WriteInt(fp, Effects[i].vc.flags);
		F_WriteShort(fp, Effects[i].vc.sound_num);
		for (j = 0; j < VCLIP_MAX_FRAMES; j++)
			F_WriteShort(fp, Effects[i].vc.frames[j].index);
		F_WriteInt(fp, Effects[i].vc.light_value);
		F_WriteInt(fp, Effects[i].time_left);
		F_WriteInt(fp, Effects[i].frame_count);
		F_WriteShort(fp, Effects[i].changing_wall_texture);
		F_WriteShort(fp, Effects[i].changing_object_texture);
		F_WriteInt(fp, Effects[i].flags);
		F_WriteInt(fp, Effects[i].crit_clip);
		F_WriteInt(fp, Effects[i].dest_bm_num);
		F_WriteInt(fp, Effects[i].dest_vclip);
		F_WriteInt(fp, Effects[i].dest_eclip);
		F_WriteInt(fp, Effects[i].dest_size);
		F_WriteInt(fp, Effects[i].sound_num);
		F_WriteInt(fp, Effects[i].segnum);
		F_WriteInt(fp, Effects[i].sidenum);
	}
}

void read_wallanim_info(CFILE* fp)
{
	int i, j;

	for (i = 0; i < MAX_WALL_ANIMS; i++) 
	{
		WallAnims[i].play_time = CF_ReadFix(fp);;
		WallAnims[i].num_frames = CF_ReadShort(fp);;
		for (j = 0; j < MAX_CLIP_FRAMES; j++)
			WallAnims[i].frames[j] = CF_ReadShort(fp);
		WallAnims[i].open_sound = CF_ReadShort(fp);
		WallAnims[i].close_sound = CF_ReadShort(fp);
		WallAnims[i].flags = CF_ReadShort(fp);
		cfread(&WallAnims[i].filename[0], 13, 1, fp);
		WallAnims[i].pad = CF_ReadByte(fp);
	}
}

void write_wallanim_info(FILE* fp)
{
	int i, j;

	for (i = 0; i < MAX_WALL_ANIMS; i++)
	{
		F_WriteInt(fp, WallAnims[i].play_time);
		F_WriteShort(fp, WallAnims[i].num_frames);
		for (j = 0; j < MAX_CLIP_FRAMES; j++)
			F_WriteShort(fp, WallAnims[i].frames[j]);
		F_WriteShort(fp, WallAnims[i].open_sound);
		F_WriteShort(fp, WallAnims[i].close_sound);
		F_WriteShort(fp, WallAnims[i].flags);
		fwrite(&WallAnims[i].filename[0], 13, 1, fp);
		F_WriteByte(fp, WallAnims[i].pad);
	}
}

void read_robot_info(CFILE* fp)
{
	int i, j, k;

	for (i = 0; i < MAX_ROBOT_TYPES; i++) 
	{
		Robot_info[i].model_num = CF_ReadInt(fp);
		Robot_info[i].n_guns = CF_ReadInt(fp);
		for (j = 0; j < MAX_GUNS; j++) 
		{
			Robot_info[i].gun_points[j].x = CF_ReadFix(fp);
			Robot_info[i].gun_points[j].y = CF_ReadFix(fp);
			Robot_info[i].gun_points[j].z = CF_ReadFix(fp);
		}
		for (j = 0; j < MAX_GUNS; j++)
			Robot_info[i].gun_submodels[j] = CF_ReadByte(fp);
		Robot_info[i].exp1_vclip_num = CF_ReadShort(fp);
		Robot_info[i].exp1_sound_num = CF_ReadShort(fp);
		Robot_info[i].exp2_vclip_num = CF_ReadShort(fp);
		Robot_info[i].exp2_sound_num = CF_ReadShort(fp);
		Robot_info[i].weapon_type = CF_ReadShort(fp);
		Robot_info[i].contains_id = CF_ReadByte(fp);
		Robot_info[i].contains_count = CF_ReadByte(fp);
		Robot_info[i].contains_prob = CF_ReadByte(fp);
		Robot_info[i].contains_type = CF_ReadByte(fp);
		Robot_info[i].score_value = CF_ReadInt(fp);
		Robot_info[i].lighting = CF_ReadFix(fp);
		Robot_info[i].strength = CF_ReadFix(fp);
		Robot_info[i].mass = CF_ReadFix(fp);
		Robot_info[i].drag = CF_ReadFix(fp);
		for (j = 0; j < NDL; j++)
			Robot_info[i].field_of_view[j] = CF_ReadFix(fp);
		for (j = 0; j < NDL; j++)
			Robot_info[i].firing_wait[j] = CF_ReadFix(fp);
		for (j = 0; j < NDL; j++)
			Robot_info[i].turn_time[j] = CF_ReadFix(fp);
		for (j = 0; j < NDL; j++)
			Robot_info[i].fire_power[j] = CF_ReadFix(fp);
		for (j = 0; j < NDL; j++)
			Robot_info[i].shield[j] = CF_ReadFix(fp);
		for (j = 0; j < NDL; j++)
			Robot_info[i].max_speed[j] = CF_ReadFix(fp);
		for (j = 0; j < NDL; j++)
			Robot_info[i].circle_distance[j] = CF_ReadFix(fp);
		for (j = 0; j < NDL; j++)
			Robot_info[i].rapidfire_count[j] = CF_ReadByte(fp);
		for (j = 0; j < NDL; j++)
			Robot_info[i].evade_speed[j] = CF_ReadByte(fp);
		Robot_info[i].cloak_type = CF_ReadByte(fp);
		Robot_info[i].attack_type = CF_ReadByte(fp);
		Robot_info[i].boss_flag = CF_ReadByte(fp);
		Robot_info[i].see_sound = CF_ReadByte(fp);
		Robot_info[i].attack_sound = CF_ReadByte(fp);
		Robot_info[i].claw_sound = CF_ReadByte(fp);

		for (j = 0; j < MAX_GUNS + 1; j++) 
		{
			for (k = 0; k < N_ANIM_STATES; k++) 
			{
				Robot_info[i].anim_states[j][k].n_joints = CF_ReadShort(fp);
				Robot_info[i].anim_states[j][k].offset = CF_ReadShort(fp);
			}
		}

		Robot_info[i].always_0xabcd = CF_ReadInt(fp);
	}
}

void write_robot_info(FILE* fp)
{
	int i, j, k;

	for (i = 0; i < MAX_ROBOT_TYPES; i++)
	{
		F_WriteInt(fp, Robot_info[i].model_num);
		F_WriteInt(fp, Robot_info[i].n_guns);
		for (j = 0; j < MAX_GUNS; j++)
		{
			F_WriteInt(fp, Robot_info[i].gun_points[j].x);
			F_WriteInt(fp, Robot_info[i].gun_points[j].y);
			F_WriteInt(fp, Robot_info[i].gun_points[j].z);
		}
		for (j = 0; j < MAX_GUNS; j++)
			F_WriteByte(fp, Robot_info[i].gun_submodels[j]);
		F_WriteShort(fp, Robot_info[i].exp1_vclip_num);
		F_WriteShort(fp, Robot_info[i].exp1_sound_num);
		F_WriteShort(fp, Robot_info[i].exp2_vclip_num);
		F_WriteShort(fp, Robot_info[i].exp2_sound_num);
		F_WriteShort(fp, Robot_info[i].weapon_type);
		F_WriteByte(fp, Robot_info[i].contains_id);
		F_WriteByte(fp, Robot_info[i].contains_count);
		F_WriteByte(fp, Robot_info[i].contains_prob);
		F_WriteByte(fp, Robot_info[i].contains_type);
		F_WriteInt(fp, Robot_info[i].score_value);
		F_WriteInt(fp, Robot_info[i].lighting);
		F_WriteInt(fp, Robot_info[i].strength);
		F_WriteInt(fp, Robot_info[i].mass);
		F_WriteInt(fp, Robot_info[i].drag);
		for (j = 0; j < NDL; j++)
			F_WriteInt(fp, Robot_info[i].field_of_view[j]);
		for (j = 0; j < NDL; j++)
			F_WriteInt(fp, Robot_info[i].firing_wait[j]);
		for (j = 0; j < NDL; j++)
			F_WriteInt(fp, Robot_info[i].turn_time[j]);
		for (j = 0; j < NDL; j++)
			F_WriteInt(fp, Robot_info[i].fire_power[j]);
		for (j = 0; j < NDL; j++)
			F_WriteInt(fp, Robot_info[i].shield[j]);
		for (j = 0; j < NDL; j++)
			F_WriteInt(fp, Robot_info[i].max_speed[j]);
		for (j = 0; j < NDL; j++)
			F_WriteInt(fp, Robot_info[i].circle_distance[j]);
		for (j = 0; j < NDL; j++)
			F_WriteByte(fp, Robot_info[i].rapidfire_count[j]);
		for (j = 0; j < NDL; j++)
			F_WriteByte(fp, Robot_info[i].evade_speed[j]);
		F_WriteByte(fp, Robot_info[i].cloak_type);
		F_WriteByte(fp, Robot_info[i].attack_type);
		F_WriteByte(fp, Robot_info[i].boss_flag);
		F_WriteByte(fp, Robot_info[i].see_sound);
		F_WriteByte(fp, Robot_info[i].attack_sound);
		F_WriteByte(fp, Robot_info[i].claw_sound);

		for (j = 0; j < MAX_GUNS + 1; j++)
		{
			for (k = 0; k < N_ANIM_STATES; k++)
			{
				F_WriteShort(fp, Robot_info[i].anim_states[j][k].n_joints);
				F_WriteShort(fp, Robot_info[i].anim_states[j][k].offset);
			}
		}

		F_WriteInt(fp, Robot_info[i].always_0xabcd);
	}
}


void read_robot_joints_info(CFILE* fp)
{
	int i;

	for (i = 0; i < MAX_ROBOT_JOINTS; i++)
	{
		Robot_joints[i].jointnum = CF_ReadShort(fp);
		Robot_joints[i].angles.p = CF_ReadShort(fp);
		Robot_joints[i].angles.b = CF_ReadShort(fp);
		Robot_joints[i].angles.h = CF_ReadShort(fp);
	}
}

void write_robot_joints_info(FILE* fp)
{
	int i;

	for (i = 0; i < MAX_ROBOT_JOINTS; i++)
	{
		F_WriteShort(fp, Robot_joints[i].jointnum);
		F_WriteShort(fp, Robot_joints[i].angles.p);
		F_WriteShort(fp, Robot_joints[i].angles.b);
		F_WriteShort(fp, Robot_joints[i].angles.h);
	}
}

void read_weapon_info(CFILE* fp)
{
	int i, j;

	for (i = 0; i < MAX_WEAPON_TYPES; i++)
	{
		Weapon_info[i].render_type = CF_ReadByte(fp);
		Weapon_info[i].model_num = CF_ReadByte(fp);
		Weapon_info[i].model_num_inner = CF_ReadByte(fp);
		Weapon_info[i].persistent = CF_ReadByte(fp);
		Weapon_info[i].flash_vclip = CF_ReadByte(fp);
		Weapon_info[i].flash_sound = CF_ReadShort(fp);
		Weapon_info[i].robot_hit_vclip = CF_ReadByte(fp);
		Weapon_info[i].robot_hit_sound = CF_ReadShort(fp);
		Weapon_info[i].wall_hit_vclip = CF_ReadByte(fp);
		Weapon_info[i].wall_hit_sound = CF_ReadShort(fp);
		Weapon_info[i].fire_count = CF_ReadByte(fp);
		Weapon_info[i].ammo_usage = CF_ReadByte(fp);
		Weapon_info[i].weapon_vclip = CF_ReadByte(fp);
		Weapon_info[i].destroyable = CF_ReadByte(fp);
		Weapon_info[i].matter = CF_ReadByte(fp);
		Weapon_info[i].bounce = CF_ReadByte(fp);
		Weapon_info[i].homing_flag = CF_ReadByte(fp);
		Weapon_info[i].dum1 = CF_ReadByte(fp);
		Weapon_info[i].dum2 = CF_ReadByte(fp);
		Weapon_info[i].dum3 = CF_ReadByte(fp);
		Weapon_info[i].energy_usage = CF_ReadFix(fp);
		Weapon_info[i].fire_wait = CF_ReadFix(fp);
		Weapon_info[i].bitmap.index = CF_ReadShort(fp);	// bitmap_index = short
		Weapon_info[i].blob_size = CF_ReadFix(fp);
		Weapon_info[i].flash_size = CF_ReadFix(fp);
		Weapon_info[i].impact_size = CF_ReadFix(fp);
		for (j = 0; j < NDL; j++)
			Weapon_info[i].strength[j] = CF_ReadFix(fp);
		for (j = 0; j < NDL; j++)
			Weapon_info[i].speed[j] = CF_ReadFix(fp);
		Weapon_info[i].mass = CF_ReadFix(fp);
		Weapon_info[i].drag = CF_ReadFix(fp);
		Weapon_info[i].thrust = CF_ReadFix(fp);

		Weapon_info[i].po_len_to_width_ratio = CF_ReadFix(fp);
		Weapon_info[i].light = CF_ReadFix(fp);
		Weapon_info[i].lifetime = CF_ReadFix(fp);
		Weapon_info[i].damage_radius = CF_ReadFix(fp);
		Weapon_info[i].picture.index = CF_ReadShort(fp);		// bitmap_index is a short
	}
}

void write_weapon_info(FILE* fp)
{
	int i, j;

	for (i = 0; i < MAX_WEAPON_TYPES; i++)
	{
		F_WriteByte(fp, Weapon_info[i].render_type);
		F_WriteByte(fp, Weapon_info[i].model_num);
		F_WriteByte(fp, Weapon_info[i].model_num_inner);
		F_WriteByte(fp, Weapon_info[i].persistent);
		F_WriteByte(fp, Weapon_info[i].flash_vclip);
		F_WriteShort(fp, Weapon_info[i].flash_sound);
		F_WriteByte(fp, Weapon_info[i].robot_hit_vclip);
		F_WriteShort(fp, Weapon_info[i].robot_hit_sound);
		F_WriteByte(fp, Weapon_info[i].wall_hit_vclip);
		F_WriteShort(fp, Weapon_info[i].wall_hit_sound);
		F_WriteByte(fp, Weapon_info[i].fire_count);
		F_WriteByte(fp, Weapon_info[i].ammo_usage);
		F_WriteByte(fp, Weapon_info[i].weapon_vclip);
		F_WriteByte(fp, Weapon_info[i].destroyable);
		F_WriteByte(fp, Weapon_info[i].matter);
		F_WriteByte(fp, Weapon_info[i].bounce);
		F_WriteByte(fp, Weapon_info[i].homing_flag);
		F_WriteByte(fp, Weapon_info[i].dum1);
		F_WriteByte(fp, Weapon_info[i].dum2);
		F_WriteByte(fp, Weapon_info[i].dum3);
		F_WriteInt(fp, Weapon_info[i].energy_usage);
		F_WriteInt(fp, Weapon_info[i].fire_wait);
		F_WriteShort(fp, Weapon_info[i].bitmap.index);	// bitmap_index = short
		F_WriteInt(fp, Weapon_info[i].blob_size);
		F_WriteInt(fp, Weapon_info[i].flash_size);
		F_WriteInt(fp, Weapon_info[i].impact_size);
		for (j = 0; j < NDL; j++)
			F_WriteInt(fp, Weapon_info[i].strength[j]);
		for (j = 0; j < NDL; j++)
			F_WriteInt(fp, Weapon_info[i].speed[j]);
		F_WriteInt(fp, Weapon_info[i].mass);
		F_WriteInt(fp, Weapon_info[i].drag);
		F_WriteInt(fp, Weapon_info[i].thrust);

		F_WriteInt(fp, Weapon_info[i].po_len_to_width_ratio);
		F_WriteInt(fp, Weapon_info[i].light);
		F_WriteInt(fp, Weapon_info[i].lifetime);
		F_WriteInt(fp, Weapon_info[i].damage_radius);
		F_WriteShort(fp, Weapon_info[i].picture.index);		// bitmap_index is a short
	}
}

void read_powerup_info(CFILE* fp)
{
	int i;

	for (i = 0; i < MAX_POWERUP_TYPES; i++) 
	{
		Powerup_info[i].vclip_num = CF_ReadInt(fp);
		Powerup_info[i].hit_sound = CF_ReadInt(fp);
		Powerup_info[i].size = CF_ReadFix(fp);
		Powerup_info[i].light = CF_ReadFix(fp);
	}
}

void write_powerup_info(FILE* fp)
{
	int i;

	for (i = 0; i < MAX_POWERUP_TYPES; i++)
	{
		F_WriteInt(fp, Powerup_info[i].vclip_num);
		F_WriteInt(fp, Powerup_info[i].hit_sound);
		F_WriteInt(fp, Powerup_info[i].size);
		F_WriteInt(fp, Powerup_info[i].light);
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
		Polygon_models[i].n_models = CF_ReadInt(fp);
		Polygon_models[i].model_data_size = CF_ReadInt(fp);
		Polygon_models[i].model_data = (uint8_t*)CF_ReadInt(fp);
		for (j = 0; j < MAX_SUBMODELS; j++)
			Polygon_models[i].submodel_ptrs[j] = CF_ReadInt(fp);
		for (j = 0; j < MAX_SUBMODELS; j++) 
		{
			Polygon_models[i].submodel_offsets[j].x = CF_ReadFix(fp);
			Polygon_models[i].submodel_offsets[j].y = CF_ReadFix(fp);
			Polygon_models[i].submodel_offsets[j].z = CF_ReadFix(fp);
		}
		for (j = 0; j < MAX_SUBMODELS; j++) 
		{
			Polygon_models[i].submodel_norms[j].x = CF_ReadFix(fp);
			Polygon_models[i].submodel_norms[j].y = CF_ReadFix(fp);
			Polygon_models[i].submodel_norms[j].z = CF_ReadFix(fp);
		}
		for (j = 0; j < MAX_SUBMODELS; j++) 
		{
			Polygon_models[i].submodel_pnts[j].x = CF_ReadFix(fp);
			Polygon_models[i].submodel_pnts[j].y = CF_ReadFix(fp);
			Polygon_models[i].submodel_pnts[j].z = CF_ReadFix(fp);
		}
		for (j = 0; j < MAX_SUBMODELS; j++)
			Polygon_models[i].submodel_rads[j] = CF_ReadFix(fp);
		for (j = 0; j < MAX_SUBMODELS; j++)
			Polygon_models[i].submodel_parents[j] = CF_ReadByte(fp);
		for (j = 0; j < MAX_SUBMODELS; j++) 
		{
			Polygon_models[i].submodel_mins[j].x = CF_ReadFix(fp);
			Polygon_models[i].submodel_mins[j].y = CF_ReadFix(fp);
			Polygon_models[i].submodel_mins[j].z = CF_ReadFix(fp);
		}
		for (j = 0; j < MAX_SUBMODELS; j++) 
		{
			Polygon_models[i].submodel_maxs[j].x = CF_ReadFix(fp);
			Polygon_models[i].submodel_maxs[j].y = CF_ReadFix(fp);
			Polygon_models[i].submodel_maxs[j].z = CF_ReadFix(fp);
		}
		Polygon_models[i].mins.x = CF_ReadFix(fp);
		Polygon_models[i].mins.y = CF_ReadFix(fp);
		Polygon_models[i].mins.z = CF_ReadFix(fp);
		Polygon_models[i].maxs.x = CF_ReadFix(fp);
		Polygon_models[i].maxs.y = CF_ReadFix(fp);
		Polygon_models[i].maxs.z = CF_ReadFix(fp);
		Polygon_models[i].rad = CF_ReadFix(fp);
		Polygon_models[i].n_textures = CF_ReadByte(fp);
		Polygon_models[i].first_texture = CF_ReadShort(fp);
		Polygon_models[i].simpler_model = CF_ReadByte(fp);
	}
}

void write_polygon_models(FILE* fp)
{
	int i, j;

	Assert(N_polygon_models < MAX_POLYGON_MODELS); //avoid trashing memory I guess

	//[ISB] however it's supposed to only fill out so many...
	for (i = 0; i < N_polygon_models; i++)
	{
		F_WriteInt(fp, Polygon_models[i].n_models);
		F_WriteInt(fp, Polygon_models[i].model_data_size);
		F_WriteInt(fp, (uintptr_t)Polygon_models[i].model_data);
		for (j = 0; j < MAX_SUBMODELS; j++)
			F_WriteInt(fp, Polygon_models[i].submodel_ptrs[j]);
		for (j = 0; j < MAX_SUBMODELS; j++)
		{
			F_WriteInt(fp, Polygon_models[i].submodel_offsets[j].x);
			F_WriteInt(fp, Polygon_models[i].submodel_offsets[j].y);
			F_WriteInt(fp, Polygon_models[i].submodel_offsets[j].z);
		}
		for (j = 0; j < MAX_SUBMODELS; j++)
		{
			F_WriteInt(fp, Polygon_models[i].submodel_norms[j].x);
			F_WriteInt(fp, Polygon_models[i].submodel_norms[j].y);
			F_WriteInt(fp, Polygon_models[i].submodel_norms[j].z);
		}
		for (j = 0; j < MAX_SUBMODELS; j++)
		{
			F_WriteInt(fp, Polygon_models[i].submodel_pnts[j].x);
			F_WriteInt(fp, Polygon_models[i].submodel_pnts[j].y);
			F_WriteInt(fp, Polygon_models[i].submodel_pnts[j].z);
		}
		for (j = 0; j < MAX_SUBMODELS; j++)
			F_WriteInt(fp, Polygon_models[i].submodel_rads[j]);
		for (j = 0; j < MAX_SUBMODELS; j++)
			F_WriteByte(fp, Polygon_models[i].submodel_parents[j]);
		for (j = 0; j < MAX_SUBMODELS; j++)
		{
			F_WriteInt(fp, Polygon_models[i].submodel_mins[j].x);
			F_WriteInt(fp, Polygon_models[i].submodel_mins[j].y);
			F_WriteInt(fp, Polygon_models[i].submodel_mins[j].z);
		}
		for (j = 0; j < MAX_SUBMODELS; j++)
		{
			F_WriteInt(fp, Polygon_models[i].submodel_maxs[j].x);
			F_WriteInt(fp, Polygon_models[i].submodel_maxs[j].y);
			F_WriteInt(fp, Polygon_models[i].submodel_maxs[j].z);
		}
		F_WriteInt(fp, Polygon_models[i].mins.x);
		F_WriteInt(fp, Polygon_models[i].mins.y);
		F_WriteInt(fp, Polygon_models[i].mins.z);
		F_WriteInt(fp, Polygon_models[i].maxs.x);
		F_WriteInt(fp, Polygon_models[i].maxs.y);
		F_WriteInt(fp, Polygon_models[i].maxs.z);
		F_WriteInt(fp, Polygon_models[i].rad);
		F_WriteByte(fp, Polygon_models[i].n_textures);
		F_WriteShort(fp, Polygon_models[i].first_texture);
		F_WriteByte(fp, Polygon_models[i].simpler_model);
	}
}

void read_player_ship(CFILE* fp)
{
	int i;
	only_player_ship.model_num = CF_ReadInt(fp);
	only_player_ship.expl_vclip_num = CF_ReadInt(fp);
	only_player_ship.mass = CF_ReadFix(fp);
	only_player_ship.drag = CF_ReadFix(fp);
	only_player_ship.max_thrust = CF_ReadFix(fp);
	only_player_ship.reverse_thrust = CF_ReadFix(fp);
	only_player_ship.brakes = CF_ReadFix(fp);
	only_player_ship.wiggle = CF_ReadFix(fp);
	only_player_ship.max_rotthrust = CF_ReadFix(fp);
	for (i = 0; i < N_PLAYER_GUNS; i++)
	{
		only_player_ship.gun_points[i].x = CF_ReadFix(fp);
		only_player_ship.gun_points[i].y = CF_ReadFix(fp);
		only_player_ship.gun_points[i].z = CF_ReadFix(fp);
	}
}

void write_player_ship(FILE* fp)
{
	int i;
	F_WriteInt(fp, only_player_ship.model_num);
	F_WriteInt(fp, only_player_ship.expl_vclip_num);
	F_WriteInt(fp, only_player_ship.mass);
	F_WriteInt(fp, only_player_ship.drag);
	F_WriteInt(fp, only_player_ship.max_thrust);
	F_WriteInt(fp, only_player_ship.reverse_thrust);
	F_WriteInt(fp, only_player_ship.brakes);
	F_WriteInt(fp, only_player_ship.wiggle);
	F_WriteInt(fp, only_player_ship.max_rotthrust);
	for (i = 0; i < N_PLAYER_GUNS; i++)
	{
		F_WriteInt(fp, only_player_ship.gun_points[i].x);
		F_WriteInt(fp, only_player_ship.gun_points[i].y);
		F_WriteInt(fp, only_player_ship.gun_points[i].z);
	}
}


void bm_read_all(CFILE* fp)
{
	int i;// , j, k;

	//  bitmap_index is a short

	NumTextures = CF_ReadInt(fp);
	for (i = 0; i < MAX_TEXTURES; i++)
		Textures[i].index = CF_ReadShort(fp);

	read_tmap_info(fp);

	cfread(Sounds, sizeof(uint8_t), MAX_SOUNDS, fp);
	cfread(AltSounds, sizeof(uint8_t), MAX_SOUNDS, fp);

	Num_vclips = CF_ReadInt(fp);
	read_vclip_info(fp);

	Num_effects = CF_ReadInt(fp);
	read_effect_info(fp);

	Num_wall_anims = CF_ReadInt(fp);
	read_wallanim_info(fp);

	N_robot_types = CF_ReadInt(fp);
	read_robot_info(fp);

	N_robot_joints = CF_ReadInt(fp);
	read_robot_joints_info(fp);

	N_weapon_types = CF_ReadInt(fp);
	read_weapon_info(fp);

	N_powerup_types = CF_ReadInt(fp);
	read_powerup_info(fp);

	N_polygon_models = CF_ReadInt(fp);
	read_polygon_models(fp);

	for (i = 0; i < N_polygon_models; i++) 
	{
		Polygon_models[i].model_data = (uint8_t*)malloc(Polygon_models[i].model_data_size);
		Assert(Polygon_models[i].model_data != NULL);
		cfread(Polygon_models[i].model_data, sizeof(uint8_t), Polygon_models[i].model_data_size, fp);
	}

	for (i = 0; i < MAX_GAUGE_BMS; i++)
		Gauges[i].index = CF_ReadShort(fp);

	for (i = 0; i < MAX_POLYGON_MODELS; i++)
		Dying_modelnums[i] = CF_ReadInt(fp);
	for (i = 0; i < MAX_POLYGON_MODELS; i++)
		Dead_modelnums[i] = CF_ReadInt(fp);

	for (i = 0; i < MAX_OBJ_BITMAPS; i++)
		ObjBitmaps[i].index = CF_ReadShort(fp);
	for (i = 0; i < MAX_OBJ_BITMAPS; i++)
		ObjBitmapPtrs[i] = CF_ReadShort(fp);

	read_player_ship(fp);

	Num_cockpits = CF_ReadInt(fp);
	for (i = 0; i < N_COCKPIT_BITMAPS; i++)
		cockpit_bitmap[i].index = CF_ReadShort(fp);
	//[ISB] there's a story behind this, isn't there... who was drunk...
	cfread(Sounds, sizeof(uint8_t), MAX_SOUNDS, fp);
	cfread(AltSounds, sizeof(uint8_t), MAX_SOUNDS, fp);

	Num_total_object_types = CF_ReadInt(fp);
	cfread(ObjType, sizeof(int8_t), MAX_OBJTYPE, fp);
	cfread(ObjId, sizeof(int8_t), MAX_OBJTYPE, fp);
	for (i = 0; i < MAX_OBJTYPE; i++)
		ObjStrength[i] = CF_ReadFix(fp);

	First_multi_bitmap_num = CF_ReadInt(fp);
	N_controlcen_guns = CF_ReadInt(fp);

	for (i = 0; i < MAX_CONTROLCEN_GUNS; i++) 
	{
		controlcen_gun_points[i].x = CF_ReadFix(fp);
		controlcen_gun_points[i].y = CF_ReadFix(fp);
		controlcen_gun_points[i].z = CF_ReadFix(fp);
	}
	for (i = 0; i < MAX_CONTROLCEN_GUNS; i++) 
	{
		controlcen_gun_dirs[i].x = CF_ReadFix(fp);
		controlcen_gun_dirs[i].y = CF_ReadFix(fp);
		controlcen_gun_dirs[i].z = CF_ReadFix(fp);
	}

	exit_modelnum = CF_ReadInt(fp);
	destroyed_exit_modelnum = CF_ReadInt(fp);

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
