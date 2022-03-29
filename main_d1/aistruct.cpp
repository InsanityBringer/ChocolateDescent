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
//[ISB] This file contains functions to safely read and write the various 
//AI structures to disk, and not much more. 

#include <stdio.h>

#include "cfile/cfile.h"

#include "ai.h"
#include "aistruct.h"

void P_WriteAILocals(ai_local* info, FILE* fp)
{
	int i;

	file_write_byte(fp, info->player_awareness_type);
	file_write_byte(fp, info->retry_count);
	file_write_byte(fp, info->consecutive_retries);
	file_write_byte(fp, info->mode);
	file_write_byte(fp, info->previous_visibility);
	file_write_byte(fp, info->rapidfire_count);
	file_write_short(fp, info->goal_segment);
	file_write_int(fp, info->last_see_time);
	file_write_int(fp, info->last_attack_time);

	file_write_int(fp, info->wait_time);
	file_write_int(fp, info->next_fire);
	file_write_int(fp, info->player_awareness_time);
	file_write_int(fp, info->time_player_seen);
	file_write_int(fp, info->time_player_sound_attacked);
	file_write_int(fp, info->next_misc_sound_time);
	file_write_int(fp, info->time_since_processed);

	for (i = 0; i < MAX_SUBMODELS; i++)
	{
		file_write_short(fp, info->goal_angles[i].p);
		file_write_short(fp, info->goal_angles[i].b);
		file_write_short(fp, info->goal_angles[i].h);
	}
	for (i = 0; i < MAX_SUBMODELS; i++)
	{
		file_write_short(fp, info->delta_angles[i].p);
		file_write_short(fp, info->delta_angles[i].b);
		file_write_short(fp, info->delta_angles[i].h);
	}
	for (i = 0; i < MAX_SUBMODELS; i++)
		file_write_byte(fp, info->goal_state[i]);
	for (i = 0; i < MAX_SUBMODELS; i++)
		file_write_byte(fp, info->achieved_state[i]);
}

void P_WriteSegPoint(point_seg* point, FILE* fp)
{
	file_write_int(fp, point->segnum);
	file_write_int(fp, point->point.x);
	file_write_int(fp, point->point.y);
	file_write_int(fp, point->point.z);
}

void P_WriteCloakInfo(ai_cloak_info* info, FILE* fp)
{
	file_write_int(fp, info->last_time);
	file_write_int(fp, info->last_position.x);
	file_write_int(fp, info->last_position.y);
	file_write_int(fp, info->last_position.z);
}

void P_ReadAILocals(ai_local* info, FILE* fp)
{
	int i;

	info->player_awareness_type = file_read_byte(fp);
	info->retry_count = file_read_byte(fp);
	info->consecutive_retries = file_read_byte(fp);
	info->mode = file_read_byte(fp);
	info->previous_visibility = file_read_byte(fp);
	info->rapidfire_count = file_read_byte(fp);
	info->goal_segment = file_read_short(fp);
	info->last_see_time = file_read_int(fp);
	info->last_attack_time = file_read_int(fp);

	info->wait_time = file_read_int(fp);
	info->next_fire = file_read_int(fp);
	info->player_awareness_time = file_read_int(fp);
	info->time_player_seen = file_read_int(fp);
	info->time_player_sound_attacked = file_read_int(fp);
	info->next_misc_sound_time = file_read_int(fp);
	info->time_since_processed = file_read_int(fp);

	for (i = 0; i < MAX_SUBMODELS; i++)
	{
		info->goal_angles[i].p = file_read_short(fp);
		info->goal_angles[i].b = file_read_short(fp);
		info->goal_angles[i].h = file_read_short(fp);
	}
	for (i = 0; i < MAX_SUBMODELS; i++)
	{
		info->delta_angles[i].p = file_read_short(fp);
		info->delta_angles[i].b = file_read_short(fp);
		info->delta_angles[i].h = file_read_short(fp);
	}
	for (i = 0; i < MAX_SUBMODELS; i++)
		info->goal_state[i] = file_read_byte(fp);
	for (i = 0; i < MAX_SUBMODELS; i++)
		info->achieved_state[i] = file_read_byte(fp);
}

void P_ReadSegPoint(point_seg* point, FILE* fp)
{
	point->segnum = file_read_int(fp);
	point->point.x = file_read_int(fp);
	point->point.y = file_read_int(fp);
	point->point.z = file_read_int(fp);
}

void P_ReadCloakInfo(ai_cloak_info *info, FILE* fp)
{
	info->last_time = file_read_int(fp);
	info->last_position.x = file_read_int(fp);
	info->last_position.y = file_read_int(fp);
	info->last_position.z = file_read_int(fp);
}