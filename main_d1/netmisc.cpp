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
