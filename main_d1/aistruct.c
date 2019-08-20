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

#include "cfile.h"

#include "ai.h"
#include "aistruct.h"

void P_WriteAILocals(ai_local* info, FILE* fp)
{
	int i;

	F_WriteByte(fp, info->player_awareness_type);
	F_WriteByte(fp, info->retry_count);
	F_WriteByte(fp, info->consecutive_retries);
	F_WriteByte(fp, info->mode);
	F_WriteByte(fp, info->previous_visibility);
	F_WriteByte(fp, info->rapidfire_count);
	F_WriteShort(fp, info->goal_segment);
	F_WriteInt(fp, info->last_see_time);
	F_WriteInt(fp, info->last_attack_time);

	F_WriteInt(fp, info->wait_time);
	F_WriteInt(fp, info->next_fire);
	F_WriteInt(fp, info->player_awareness_time);
	F_WriteInt(fp, info->time_player_seen);
	F_WriteInt(fp, info->time_player_sound_attacked);
	F_WriteInt(fp, info->next_misc_sound_time);
	F_WriteInt(fp, info->time_since_processed);

	for (i = 0; i < MAX_SUBMODELS; i++)
	{
		F_WriteShort(fp, info->goal_angles[i].p);
		F_WriteShort(fp, info->goal_angles[i].b);
		F_WriteShort(fp, info->goal_angles[i].h);
	}
	for (i = 0; i < MAX_SUBMODELS; i++)
	{
		F_WriteShort(fp, info->delta_angles[i].p);
		F_WriteShort(fp, info->delta_angles[i].b);
		F_WriteShort(fp, info->delta_angles[i].h);
	}
	for (i = 0; i < MAX_SUBMODELS; i++)
		F_WriteByte(fp, info->goal_state[i]);
	for (i = 0; i < MAX_SUBMODELS; i++)
		F_WriteByte(fp, info->achieved_state[i]);
}

void P_WriteSegPoint(point_seg* point, FILE* fp)
{
	F_WriteInt(fp, point->segnum);
	F_WriteInt(fp, point->point.x);
	F_WriteInt(fp, point->point.y);
	F_WriteInt(fp, point->point.z);
}

void P_WriteCloakInfo(ai_cloak_info* info, FILE* fp)
{
	F_WriteInt(fp, info->last_time);
	F_WriteInt(fp, info->last_position.x);
	F_WriteInt(fp, info->last_position.y);
	F_WriteInt(fp, info->last_position.z);
}

void P_ReadAILocals(ai_local* info, FILE* fp)
{
	int i;

	info->player_awareness_type = F_ReadByte(fp);
	info->retry_count = F_ReadByte(fp);
	info->consecutive_retries = F_ReadByte(fp);
	info->mode = F_ReadByte(fp);
	info->previous_visibility = F_ReadByte(fp);
	info->rapidfire_count = F_ReadByte(fp);
	info->goal_segment = F_ReadShort(fp);
	info->last_see_time = F_ReadInt(fp);
	info->last_attack_time = F_ReadInt(fp);

	info->wait_time = F_ReadInt(fp);
	info->next_fire = F_ReadInt(fp);
	info->player_awareness_time = F_ReadInt(fp);
	info->time_player_seen = F_ReadInt(fp);
	info->time_player_sound_attacked = F_ReadInt(fp);
	info->next_misc_sound_time = F_ReadInt(fp);
	info->time_since_processed = F_ReadInt(fp);

	for (i = 0; i < MAX_SUBMODELS; i++)
	{
		info->goal_angles[i].p = F_ReadShort(fp);
		info->goal_angles[i].b = F_ReadShort(fp);
		info->goal_angles[i].h = F_ReadShort(fp);
	}
	for (i = 0; i < MAX_SUBMODELS; i++)
	{
		info->delta_angles[i].p = F_ReadShort(fp);
		info->delta_angles[i].b = F_ReadShort(fp);
		info->delta_angles[i].h = F_ReadShort(fp);
	}
	for (i = 0; i < MAX_SUBMODELS; i++)
		info->goal_state[i] = F_ReadByte(fp);
	for (i = 0; i < MAX_SUBMODELS; i++)
		info->achieved_state[i] = F_ReadByte(fp);
}

void P_ReadSegPoint(point_seg* point, FILE* fp)
{
	point->segnum = F_ReadInt(fp);
	point->point.x = F_ReadInt(fp);
	point->point.y = F_ReadInt(fp);
	point->point.z = F_ReadInt(fp);
}

void P_ReadCloakInfo(ai_cloak_info *info, FILE* fp)
{
	info->last_time = F_ReadInt(fp);
	info->last_position.x = F_ReadInt(fp);
	info->last_position.y = F_ReadInt(fp);
	info->last_position.z = F_ReadInt(fp);
}