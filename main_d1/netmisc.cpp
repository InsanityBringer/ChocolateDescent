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

#ifdef NETWORK

#include <string.h>
#include <stdlib.h>

#include "multi.h"
#include "network.h"
#include "misc/types.h"
#include "platform/mono.h"
#include "object.h"
#include "netmisc.h"

// Calculates the checksum of a block of memory.
uint16_t netmisc_calc_checksum(void* vptr, int len)
{
	uint8_t* ptr = (uint8_t*)vptr;
	unsigned int sum1, sum2;

	sum1 = sum2 = 0;

	while (len--) {
		sum1 += *ptr++;
		if (sum1 >= 255) sum1 -= 255;
		sum2 += sum1;
	}
	sum2 %= 255;

	return ((sum1 << 8) + sum2);
}

void netmisc_encode_int8(uint8_t* ptr, int *offset, uint8_t v)
{
	ptr[*offset] = v;
	*offset += 1;
}

void netmisc_encode_int16(uint8_t* ptr, int *offset, short v)
{
	ptr[*offset + 0] = (uint8_t)v & 255;
	ptr[*offset + 1] = (uint8_t)(v >> 8) & 255;
	*offset += 2;
}

void netmisc_encode_int32(uint8_t* ptr, int *offset, int v)
{
	ptr[*offset + 0] = (uint8_t)v & 255;
	ptr[*offset + 1] = (uint8_t)(v >> 8) & 255;
	ptr[*offset + 2] = (uint8_t)(v >> 16) & 255;
	ptr[*offset + 3] = (uint8_t)(v >> 24) & 255;
	*offset += 4;
}

void netmisc_encode_shortpos(uint8_t* ptr, int *offset, shortpos *v)
{
	ptr[*offset + 0] = v->bytemat[0];
	ptr[*offset + 1] = v->bytemat[1];
	ptr[*offset + 2] = v->bytemat[2];
	ptr[*offset + 3] = v->bytemat[3];
	ptr[*offset + 4] = v->bytemat[4];
	ptr[*offset + 5] = v->bytemat[5];
	ptr[*offset + 6] = v->bytemat[6];
	ptr[*offset + 7] = v->bytemat[7];
	ptr[*offset + 8] = v->bytemat[8];
	*offset += 9;
	netmisc_encode_int16(ptr, offset, v->xo);
	netmisc_encode_int16(ptr, offset, v->yo);
	netmisc_encode_int16(ptr, offset, v->zo);
	netmisc_encode_int16(ptr, offset, v->segment);
	netmisc_encode_int16(ptr, offset, v->velx);
	netmisc_encode_int16(ptr, offset, v->vely);
	netmisc_encode_int16(ptr, offset, v->velz);
}

void netmisc_encode_vector(uint8_t* ptr, int* offset, vms_vector* vec)
{
	netmisc_encode_int32(ptr, offset, vec->x);
	netmisc_encode_int32(ptr, offset, vec->y);
	netmisc_encode_int32(ptr, offset, vec->z);
}

void netmisc_encode_matrix(uint8_t* ptr, int* offset, vms_matrix* mat)
{
	netmisc_encode_vector(ptr, offset, &mat->fvec);
	netmisc_encode_vector(ptr, offset, &mat->rvec);
	netmisc_encode_vector(ptr, offset, &mat->uvec);
}

void netmisc_encode_angvec(uint8_t* ptr, int* offset, vms_angvec* vec)
{
	netmisc_encode_int16(ptr, offset, vec->p);
	netmisc_encode_int16(ptr, offset, vec->b);
	netmisc_encode_int16(ptr, offset, vec->h);
}

void netmisc_decode_int8(uint8_t* ptr, int* offset, uint8_t* v)
{
	*v = ptr[*offset];
	*offset += 1;
}

void netmisc_decode_int16(uint8_t* ptr, int* offset, short* v)
{
	*v = (short)(ptr[*offset] | (ptr[*offset + 1] << 8));
	*offset += 2;
}

void netmisc_decode_int32(uint8_t* ptr, int* offset, int* v)
{
	*v = (int)(ptr[*offset] | (ptr[*offset + 1] << 8) | (ptr[*offset + 2] << 16) | (ptr[*offset + 3] << 24));
	*offset += 4;
}

void netmisc_decode_shortpos(uint8_t* ptr, int* offset, shortpos* v)
{
	v->bytemat[0] = ptr[*offset];
	v->bytemat[1] = ptr[*offset+1];
	v->bytemat[2] = ptr[*offset+2];
	v->bytemat[3] = ptr[*offset+3];
	v->bytemat[4] = ptr[*offset+4];
	v->bytemat[5] = ptr[*offset+5];
	v->bytemat[6] = ptr[*offset+6];
	v->bytemat[7] = ptr[*offset+7];
	v->bytemat[8] = ptr[*offset+8];
	*offset += 9;
	netmisc_decode_int16(ptr, offset, &v->xo);
	netmisc_decode_int16(ptr, offset, &v->yo);
	netmisc_decode_int16(ptr, offset, &v->zo);
	netmisc_decode_int16(ptr, offset, &v->segment);
	netmisc_decode_int16(ptr, offset, &v->velx);
	netmisc_decode_int16(ptr, offset, &v->vely);
	netmisc_decode_int16(ptr, offset, &v->velz);
}

void netmisc_decode_vector(uint8_t* ptr, int* offset, vms_vector* vec)
{
	netmisc_decode_int32(ptr, offset, &vec->x);
	netmisc_decode_int32(ptr, offset, &vec->y);
	netmisc_decode_int32(ptr, offset, &vec->z);
}

void netmisc_decode_matrix(uint8_t* ptr, int* offset, vms_matrix* mat)
{
	netmisc_decode_vector(ptr, offset, &mat->fvec);
	netmisc_decode_vector(ptr, offset, &mat->rvec);
	netmisc_decode_vector(ptr, offset, &mat->uvec);
}

void netmisc_decode_angvec(uint8_t* ptr, int* offset, vms_angvec* vec)
{
	netmisc_decode_int16(ptr, offset, &vec->p);
	netmisc_decode_int16(ptr, offset, &vec->b);
	netmisc_decode_int16(ptr, offset, &vec->h);
}

void netmisc_encode_netplayer_info(uint8_t* ptr, int* offset, netplayer_info* info)
{
	netmisc_encode_buffer(ptr, offset, info->callsign, CALLSIGN_LEN + 1);
		netmisc_encode_buffer(ptr, offset, info->node, 4);
	netmisc_encode_int16(ptr, offset, info->socket);
	netmisc_encode_int8(ptr, offset, info->connected);
	netmisc_encode_int32(ptr, offset, info->identifier);
}

void netmisc_encode_netgameinfo(uint8_t* ptr, int* offset, netgame_info* info)
{
	int i, j;
	netmisc_encode_int8(ptr, offset, info->type);
	netmisc_encode_int8(ptr, offset, info->protocol_version);
	netmisc_encode_buffer(ptr, offset, info->game_name, NETGAME_NAME_LEN + 1);
	netmisc_encode_buffer(ptr, offset, info->team_name[0], CALLSIGN_LEN + 1);
	netmisc_encode_buffer(ptr, offset, info->team_name[1], CALLSIGN_LEN + 1);
	netmisc_encode_int8(ptr, offset, info->gamemode);
	netmisc_encode_int8(ptr, offset, info->difficulty);
	netmisc_encode_int8(ptr, offset, info->game_status);
	netmisc_encode_int8(ptr, offset, info->numplayers);
	netmisc_encode_int8(ptr, offset, info->max_numplayers);
	netmisc_encode_int8(ptr, offset, info->game_flags);
	for (i = 0; i < MAX_PLAYERS; i++)
		netmisc_encode_netplayer_info(ptr, offset, &info->players[i]);
	for (i = 0; i < MAX_PLAYERS; i++)
		netmisc_encode_int32(ptr, offset, info->locations[i]);
	for (i = 0; i < MAX_PLAYERS; i++)
		for (j = 0; j < MAX_PLAYERS; j++)
			netmisc_encode_int16(ptr, offset, info->kills[i][j]);

	netmisc_encode_int32(ptr, offset, info->levelnum);
	netmisc_encode_int8(ptr, offset, info->team_vector);
	netmisc_encode_int16(ptr, offset, info->segments_checksum);
	netmisc_encode_int16(ptr, offset, info->team_kills[0]);
	netmisc_encode_int16(ptr, offset, info->team_kills[1]);
	for (i = 0; i < MAX_PLAYERS; i++)
		netmisc_encode_int16(ptr, offset, info->killed[i]);
	for (i = 0; i < MAX_PLAYERS; i++)
		netmisc_encode_int16(ptr, offset, info->player_kills[i]);

	netmisc_encode_int32(ptr, offset, info->level_time);
	netmisc_encode_int32(ptr, offset, info->control_invul_time);
	netmisc_encode_int32(ptr, offset, info->monitor_vector);
	for (i = 0; i < MAX_PLAYERS; i++)
		netmisc_encode_int32(ptr, offset, info->player_score[i]);
	netmisc_encode_buffer(ptr, offset, info->player_flags, MAX_PLAYERS);
	netmisc_encode_buffer(ptr, offset, info->mission_name, 9);
	netmisc_encode_buffer(ptr, offset, info->mission_title, MISSION_NAME_LEN+1);
}

void netmisc_encode_sequence_packet(uint8_t* ptr, int* offset, sequence_packet* info)
{
	netmisc_encode_int8(ptr, offset, info->type);
	netmisc_encode_netplayer_info(ptr, offset, &info->player);
}

void netmisc_encode_frame_info(uint8_t* ptr, int* offset, frame_info* info)
{
	netmisc_encode_int8(ptr, offset, info->type);
	netmisc_encode_buffer(ptr, offset, info->pad, 3);
	netmisc_encode_int32(ptr, offset, info->numpackets);
	netmisc_encode_vector(ptr, offset, &info->obj_pos);
	netmisc_encode_matrix(ptr, offset, &info->obj_orient);
	netmisc_encode_vector(ptr, offset, &info->phys_velocity);
	netmisc_encode_vector(ptr, offset, &info->phys_rotvel);
	netmisc_encode_int16(ptr, offset, info->obj_segnum);
	netmisc_encode_int16(ptr, offset, info->data_size);
	netmisc_encode_int8(ptr, offset, info->playernum);
	netmisc_encode_int8(ptr, offset, info->obj_render_type);
	netmisc_encode_int8(ptr, offset, info->level_num);
	netmisc_encode_buffer(ptr, offset, info->data, NET_XDATA_SIZE);
}

void netmisc_encode_endlevel_info(uint8_t* ptr, int* offset, endlevel_info* info)
{
	int i, j;
	netmisc_encode_int8(ptr, offset, info->type);
	netmisc_encode_int8(ptr, offset, info->player_num);
	netmisc_encode_int8(ptr, offset, info->connected);
	for (i = 0; i < MAX_PLAYERS; i++)
		for (j = 0; j < MAX_PLAYERS; j++)
			netmisc_encode_int16(ptr, offset, info->kill_matrix[i][j]);

	netmisc_encode_int16(ptr, offset, info->kills);
	netmisc_encode_int16(ptr, offset, info->killed);
	netmisc_encode_int8(ptr, offset, info->seconds_left);
}

void netmisc_encode_object(uint8_t* ptr, int* offset, object* objp)
{
	int i;
	int extra = 64;
	netmisc_encode_int32(ptr, offset, objp->signature);
	netmisc_encode_int8(ptr, offset, objp->type);
	netmisc_encode_int8(ptr, offset, objp->id);
	netmisc_encode_int16(ptr, offset, objp->next);
	netmisc_encode_int16(ptr, offset, objp->prev);
	netmisc_encode_int8(ptr, offset, objp->control_type);
	netmisc_encode_int8(ptr, offset, objp->movement_type);
	netmisc_encode_int8(ptr, offset, objp->render_type);
	netmisc_encode_int8(ptr, offset, objp->flags);

	netmisc_encode_int16(ptr, offset, objp->segnum);
	netmisc_encode_int16(ptr, offset, objp->attached_obj);

	netmisc_encode_vector(ptr, offset, &objp->pos);
	netmisc_encode_matrix(ptr, offset, &objp->orient);

	netmisc_encode_int32(ptr, offset, objp->size);
	netmisc_encode_int32(ptr, offset, objp->shields);

	netmisc_encode_vector(ptr, offset, &objp->last_pos);

	netmisc_encode_int8(ptr, offset, objp->contains_type);
	netmisc_encode_int8(ptr, offset, objp->contains_id);
	netmisc_encode_int8(ptr, offset, objp->contains_count);
	netmisc_encode_int8(ptr, offset, objp->matcen_creator);

	netmisc_encode_int32(ptr, offset, objp->lifeleft);

	switch (objp->movement_type)
	{
	case MT_PHYSICS:
		netmisc_encode_vector(ptr, offset, &objp->mtype.phys_info.velocity);
		netmisc_encode_vector(ptr, offset, &objp->mtype.phys_info.thrust);
		netmisc_encode_int32(ptr, offset, objp->mtype.phys_info.mass);
		netmisc_encode_int32(ptr, offset, objp->mtype.phys_info.drag);
		netmisc_encode_int32(ptr, offset, objp->mtype.phys_info.brakes);
		netmisc_encode_vector(ptr, offset, &objp->mtype.phys_info.rotvel);
		netmisc_encode_vector(ptr, offset, &objp->mtype.phys_info.rotthrust);
		netmisc_encode_int16(ptr, offset, objp->mtype.phys_info.turnroll);
		netmisc_encode_int16(ptr, offset, objp->mtype.phys_info.flags);
		extra = 0;
		break;
	case MT_SPINNING:
		netmisc_encode_vector(ptr, offset, &objp->mtype.spin_rate);
		extra -= 12;
		break;
	}
	*offset += extra;
	extra = 30;
	switch (objp->control_type)
	{
	case CT_AI:
		netmisc_encode_int8(ptr, offset, objp->ctype.ai_info.behavior);
		for (i = 0; i < MAX_AI_FLAGS; i++)
			netmisc_encode_int8(ptr, offset, objp->ctype.ai_info.flags[i]);
		
		netmisc_encode_int16(ptr, offset, objp->ctype.ai_info.hide_segment);
		netmisc_encode_int16(ptr, offset, objp->ctype.ai_info.hide_index);
		netmisc_encode_int16(ptr, offset, objp->ctype.ai_info.path_length);
		netmisc_encode_int16(ptr, offset, objp->ctype.ai_info.cur_path_index);
		netmisc_encode_int16(ptr, offset, objp->ctype.ai_info.follow_path_start_seg);
		netmisc_encode_int16(ptr, offset, objp->ctype.ai_info.follow_path_end_seg);
		netmisc_encode_int32(ptr, offset, objp->ctype.ai_info.danger_laser_signature);
		netmisc_encode_int16(ptr, offset, objp->ctype.ai_info.danger_laser_num);
		extra = 0;
		break;
	case CT_EXPLOSION:
	case CT_DEBRIS:
		netmisc_encode_int32(ptr, offset, objp->ctype.expl_info.spawn_time);
		netmisc_encode_int32(ptr, offset, objp->ctype.expl_info.delete_time);
		netmisc_encode_int16(ptr, offset, objp->ctype.expl_info.delete_objnum);
		netmisc_encode_int16(ptr, offset, objp->ctype.expl_info.attach_parent);
		netmisc_encode_int16(ptr, offset, objp->ctype.expl_info.next_attach);
		netmisc_encode_int16(ptr, offset, objp->ctype.expl_info.prev_attach);
		extra -= 16;
		break;
	case CT_WEAPON:
		netmisc_encode_int16(ptr, offset, objp->ctype.laser_info.parent_type);
		netmisc_encode_int16(ptr, offset, objp->ctype.laser_info.parent_num);
		netmisc_encode_int32(ptr, offset, objp->ctype.laser_info.parent_signature);
		netmisc_encode_int32(ptr, offset, objp->ctype.laser_info.creation_time);
		netmisc_encode_int16(ptr, offset, objp->ctype.laser_info.last_hitobj);
		netmisc_encode_int16(ptr, offset, objp->ctype.laser_info.track_goal);
		netmisc_encode_int32(ptr, offset, objp->ctype.laser_info.multiplier);
		extra -= 20;
		break;
	case CT_LIGHT:
		netmisc_encode_int32(ptr, offset, objp->ctype.light_info.intensity);
		extra -= 4;
		break;
	case CT_POWERUP:
		netmisc_encode_int32(ptr, offset, objp->ctype.powerup_info.count);
		extra -= 4;
		break;
	}
	*offset += extra;
	extra = 76;
	switch (objp->render_type)
	{
	case RT_MORPH:
	case RT_POLYOBJ:
		netmisc_encode_int32(ptr, offset, objp->rtype.pobj_info.model_num);
		for (i = 0; i < MAX_SUBMODELS; i++)
			netmisc_encode_angvec(ptr, offset, &objp->rtype.pobj_info.anim_angles[i]);
		netmisc_encode_int32(ptr, offset, objp->rtype.pobj_info.subobj_flags);
		netmisc_encode_int32(ptr, offset, objp->rtype.pobj_info.tmap_override);
		netmisc_encode_int32(ptr, offset, objp->rtype.pobj_info.alt_textures);
		extra = 0;
		break;
	case RT_WEAPON_VCLIP:
	case RT_HOSTAGE:
	case RT_POWERUP:
	case RT_FIREBALL:
		netmisc_encode_int32(ptr, offset, objp->rtype.vclip_info.vclip_num);
		netmisc_encode_int32(ptr, offset, objp->rtype.vclip_info.frametime);
		netmisc_encode_int8(ptr, offset, objp->rtype.vclip_info.framenum);
		extra -= 9;
		break;
	}
	*offset += extra;
}

void netmisc_decode_netplayer_info(uint8_t* ptr, int* offset, netplayer_info* info)
{
	netmisc_decode_buffer(ptr, offset, info->callsign, CALLSIGN_LEN + 1);
	netmisc_decode_buffer(ptr, offset, info->node, 4);
	netmisc_decode_int16(ptr, offset, (short*)&info->socket);
	netmisc_decode_int8(ptr, offset, (uint8_t*)&info->connected);
	netmisc_decode_int32(ptr, offset, (int*)&info->identifier);
}

void netmisc_decode_netgameinfo(uint8_t* ptr, int* offset, netgame_info* info)
{
	int i, j;
	netmisc_decode_int8(ptr, offset, &info->type);
	netmisc_decode_int8(ptr, offset, &info->protocol_version);
	netmisc_decode_buffer(ptr, offset, info->game_name, NETGAME_NAME_LEN + 1);
	netmisc_decode_buffer(ptr, offset, info->team_name[0], CALLSIGN_LEN + 1);
	netmisc_decode_buffer(ptr, offset, info->team_name[1], CALLSIGN_LEN + 1);
	netmisc_decode_int8(ptr, offset, &info->gamemode);
	netmisc_decode_int8(ptr, offset, &info->difficulty);
	netmisc_decode_int8(ptr, offset, &info->game_status);
	netmisc_decode_int8(ptr, offset, &info->numplayers);
	netmisc_decode_int8(ptr, offset, &info->max_numplayers);
	netmisc_decode_int8(ptr, offset, &info->game_flags);
	for (i = 0; i < MAX_PLAYERS; i++)
		netmisc_decode_netplayer_info(ptr, offset, &info->players[i]);
	for (i = 0; i < MAX_PLAYERS; i++)
		netmisc_decode_int32(ptr, offset, &info->locations[i]);
	for (i = 0; i < MAX_PLAYERS; i++)
		for (j = 0; j < MAX_PLAYERS; j++)
			netmisc_decode_int16(ptr, offset, &info->kills[i][j]);

	netmisc_decode_int32(ptr, offset, &info->levelnum);
	netmisc_decode_int8(ptr, offset, &info->team_vector);
	netmisc_decode_int16(ptr, offset, (short*)&info->segments_checksum);
	netmisc_decode_int16(ptr, offset, &info->team_kills[0]);
	netmisc_decode_int16(ptr, offset, &info->team_kills[1]);
	for (i = 0; i < MAX_PLAYERS; i++)
		netmisc_decode_int16(ptr, offset, &info->killed[i]);
	for (i = 0; i < MAX_PLAYERS; i++)
		netmisc_decode_int16(ptr, offset, &info->player_kills[i]);

	netmisc_decode_int32(ptr, offset, &info->level_time);
	netmisc_decode_int32(ptr, offset, &info->control_invul_time);
	netmisc_decode_int32(ptr, offset, &info->monitor_vector);
	for (i = 0; i < MAX_PLAYERS; i++)
		netmisc_decode_int32(ptr, offset, &info->player_score[i]);
	netmisc_decode_buffer(ptr, offset, info->player_flags, MAX_PLAYERS);
	netmisc_decode_buffer(ptr, offset, info->mission_name, 9);
	netmisc_decode_buffer(ptr, offset, info->mission_title, MISSION_NAME_LEN + 1);
}

void netmisc_decode_sequence_packet(uint8_t* ptr, int* offset, sequence_packet* info)
{
	netmisc_decode_int8(ptr, offset, &info->type);
	netmisc_decode_netplayer_info(ptr, offset, &info->player);
}

void netmisc_decode_frame_info(uint8_t* ptr, int* offset, frame_info* info)
{
	netmisc_decode_int8(ptr, offset, &info->type);
	netmisc_decode_buffer(ptr, offset, &info->pad, 3);
	netmisc_decode_int32(ptr, offset, &info->numpackets);
	netmisc_decode_vector(ptr, offset, &info->obj_pos);
	netmisc_decode_matrix(ptr, offset, &info->obj_orient);
	netmisc_decode_vector(ptr, offset, &info->phys_velocity);
	netmisc_decode_vector(ptr, offset, &info->phys_rotvel);
	netmisc_decode_int16(ptr, offset, &info->obj_segnum);
	netmisc_decode_int16(ptr, offset, (short*)&info->data_size);
	netmisc_decode_int8(ptr, offset, &info->playernum);
	netmisc_decode_int8(ptr, offset, &info->obj_render_type);
	netmisc_decode_int8(ptr, offset, &info->level_num);
	netmisc_decode_buffer(ptr, offset, &info->data, NET_XDATA_SIZE);
}

void netmisc_decode_endlevel_info(uint8_t* ptr, int* offset, endlevel_info* info)
{
	int i, j;
	netmisc_decode_int8(ptr, offset, &info->type);
	netmisc_decode_int8(ptr, offset, &info->player_num);
	netmisc_decode_int8(ptr, offset, (uint8_t*)&info->connected);
	for (i = 0; i < MAX_PLAYERS; i++)
		for (j = 0; j < MAX_PLAYERS; j++)
			netmisc_decode_int16(ptr, offset, &info->kill_matrix[i][j]);

	netmisc_decode_int16(ptr, offset, &info->kills);
	netmisc_decode_int16(ptr, offset, &info->killed);
	netmisc_decode_int8(ptr, offset, &info->seconds_left);
}

void netmisc_decode_object(uint8_t* ptr, int* offset, object* objp)
{
	int i;
	int extra = 64;
	netmisc_decode_int32(ptr, offset, &objp->signature);
	netmisc_decode_int8(ptr, offset, &objp->type);
	netmisc_decode_int8(ptr, offset, &objp->id);
	netmisc_decode_int16(ptr, offset, &objp->next);
	netmisc_decode_int16(ptr, offset, &objp->prev);
	netmisc_decode_int8(ptr, offset, &objp->control_type);
	netmisc_decode_int8(ptr, offset, &objp->movement_type);
	netmisc_decode_int8(ptr, offset, &objp->render_type);
	netmisc_decode_int8(ptr, offset, &objp->flags);

	netmisc_decode_int16(ptr, offset, &objp->segnum);
	netmisc_decode_int16(ptr, offset, &objp->attached_obj);

	netmisc_decode_vector(ptr, offset, &objp->pos);
	netmisc_decode_matrix(ptr, offset, &objp->orient);

	netmisc_decode_int32(ptr, offset, &objp->size);
	netmisc_decode_int32(ptr, offset, &objp->shields);

	netmisc_decode_vector(ptr, offset, &objp->last_pos);

	netmisc_decode_int8(ptr, offset, (uint8_t*)&objp->contains_type);
	netmisc_decode_int8(ptr, offset, (uint8_t*)&objp->contains_id);
	netmisc_decode_int8(ptr, offset, (uint8_t*)&objp->contains_count);
	netmisc_decode_int8(ptr, offset, (uint8_t*)&objp->matcen_creator);

	netmisc_decode_int32(ptr, offset, &objp->lifeleft);

	switch (objp->movement_type)
	{
	case MT_PHYSICS:
		netmisc_decode_vector(ptr, offset, &objp->mtype.phys_info.velocity);
		netmisc_decode_vector(ptr, offset, &objp->mtype.phys_info.thrust);
		netmisc_decode_int32(ptr, offset, &objp->mtype.phys_info.mass);
		netmisc_decode_int32(ptr, offset, &objp->mtype.phys_info.drag);
		netmisc_decode_int32(ptr, offset, &objp->mtype.phys_info.brakes);
		netmisc_decode_vector(ptr, offset, &objp->mtype.phys_info.rotvel);
		netmisc_decode_vector(ptr, offset, &objp->mtype.phys_info.rotthrust);
		netmisc_decode_int16(ptr, offset, &objp->mtype.phys_info.turnroll);
		netmisc_decode_int16(ptr, offset, (short*)&objp->mtype.phys_info.flags);
		extra = 0;
		break;
	case MT_SPINNING:
		netmisc_decode_vector(ptr, offset, &objp->mtype.spin_rate);
		extra -= 12;
		break;
	}
	*offset += extra;
	extra = 30;
	switch (objp->control_type)
	{
	case CT_AI:
		netmisc_decode_int8(ptr, offset, &objp->ctype.ai_info.behavior);
		for (i = 0; i < MAX_AI_FLAGS; i++)
			netmisc_decode_int8(ptr, offset, (uint8_t*)&objp->ctype.ai_info.flags[i]);

		netmisc_decode_int16(ptr, offset, &objp->ctype.ai_info.hide_segment);
		netmisc_decode_int16(ptr, offset, &objp->ctype.ai_info.hide_index);
		netmisc_decode_int16(ptr, offset, &objp->ctype.ai_info.path_length);
		netmisc_decode_int16(ptr, offset, &objp->ctype.ai_info.cur_path_index);
		netmisc_decode_int16(ptr, offset, &objp->ctype.ai_info.follow_path_start_seg);
		netmisc_decode_int16(ptr, offset, &objp->ctype.ai_info.follow_path_end_seg);
		netmisc_decode_int32(ptr, offset, &objp->ctype.ai_info.danger_laser_signature);
		netmisc_decode_int16(ptr, offset, &objp->ctype.ai_info.danger_laser_num);
		extra = 0;
		break;
	case CT_EXPLOSION:
	case CT_DEBRIS:
		netmisc_decode_int32(ptr, offset, &objp->ctype.expl_info.spawn_time);
		netmisc_decode_int32(ptr, offset, &objp->ctype.expl_info.delete_time);
		netmisc_decode_int16(ptr, offset, &objp->ctype.expl_info.delete_objnum);
		netmisc_decode_int16(ptr, offset, &objp->ctype.expl_info.attach_parent);
		netmisc_decode_int16(ptr, offset, &objp->ctype.expl_info.next_attach);
		netmisc_decode_int16(ptr, offset, &objp->ctype.expl_info.prev_attach);
		extra -= 16;
		break;
	case CT_WEAPON:
		netmisc_decode_int16(ptr, offset, &objp->ctype.laser_info.parent_type);
		netmisc_decode_int16(ptr, offset, &objp->ctype.laser_info.parent_num);
		netmisc_decode_int32(ptr, offset, &objp->ctype.laser_info.parent_signature);
		netmisc_decode_int32(ptr, offset, &objp->ctype.laser_info.creation_time);
		netmisc_decode_int16(ptr, offset, &objp->ctype.laser_info.last_hitobj);
		netmisc_decode_int16(ptr, offset, &objp->ctype.laser_info.track_goal);
		netmisc_decode_int32(ptr, offset, &objp->ctype.laser_info.multiplier);
		extra -= 20;
		break;
	case CT_LIGHT:
		netmisc_decode_int32(ptr, offset, &objp->ctype.light_info.intensity);
		extra -= 4;
		break;
	case CT_POWERUP:
		netmisc_decode_int32(ptr, offset, &objp->ctype.powerup_info.count);
		extra -= 4;
		break;
	}
	*offset += extra;
	extra = 76;
	switch (objp->render_type)
	{
	case RT_MORPH:
	case RT_POLYOBJ:
		netmisc_decode_int32(ptr, offset, &objp->rtype.pobj_info.model_num);
		for (i = 0; i < MAX_SUBMODELS; i++)
			netmisc_decode_angvec(ptr, offset, &objp->rtype.pobj_info.anim_angles[i]);
		netmisc_decode_int32(ptr, offset, &objp->rtype.pobj_info.subobj_flags);
		netmisc_decode_int32(ptr, offset, &objp->rtype.pobj_info.tmap_override);
		netmisc_decode_int32(ptr, offset, &objp->rtype.pobj_info.alt_textures);
		extra = 0;
		break;
	case RT_WEAPON_VCLIP:
	case RT_HOSTAGE:
	case RT_POWERUP:
	case RT_FIREBALL:
		netmisc_decode_int32(ptr, offset, &objp->rtype.vclip_info.vclip_num);
		netmisc_decode_int32(ptr, offset, &objp->rtype.vclip_info.frametime);
		netmisc_decode_int8(ptr, offset, (uint8_t*)&objp->rtype.vclip_info.framenum);
		extra -= 9;
		break;
	}
	*offset += extra;
}

#endif
