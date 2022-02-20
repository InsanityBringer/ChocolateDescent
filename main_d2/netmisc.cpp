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


#include <stdio.h>
#include <string.h>

#include "inferno.h"
#include "misc/types.h"
#include "platform/mono.h"
#include "multi.h"
#include "network.h"
#include "object.h"
#include "netmisc.h"

// Calculates the checksum of a block of memory.
uint16_t netmisc_calc_checksum( void * vptr, int len )
{
	uint8_t * ptr = (uint8_t *)vptr;
	unsigned int sum1,sum2;

	sum1 = sum2 = 0;

	while(len--)	
	{
		sum1 += *ptr++;
		if (sum1 >= 255 ) sum1 -= 255;
		sum2 += sum1;
	}
	sum2 %= 255;
	
	return ((sum1<<8)+ sum2);
}

void netmisc_encode_int8(uint8_t* ptr, int* offset, uint8_t v)
{
	ptr[*offset] = v;
	*offset += 1;
}

void netmisc_encode_int16(uint8_t* ptr, int* offset, short v)
{
	ptr[*offset + 0] = (uint8_t)v & 255;
	ptr[*offset + 1] = (uint8_t)(v >> 8) & 255;
	*offset += 2;
}

void netmisc_encode_int32(uint8_t* ptr, int* offset, int v)
{
	ptr[*offset + 0] = (uint8_t)v & 255;
	ptr[*offset + 1] = (uint8_t)(v >> 8) & 255;
	ptr[*offset + 2] = (uint8_t)(v >> 16) & 255;
	ptr[*offset + 3] = (uint8_t)(v >> 24) & 255;
	*offset += 4;
}

void netmisc_encode_shortpos(uint8_t* ptr, int* offset, shortpos* v)
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
	v->bytemat[1] = ptr[*offset + 1];
	v->bytemat[2] = ptr[*offset + 2];
	v->bytemat[3] = ptr[*offset + 3];
	v->bytemat[4] = ptr[*offset + 4];
	v->bytemat[5] = ptr[*offset + 5];
	v->bytemat[6] = ptr[*offset + 6];
	v->bytemat[7] = ptr[*offset + 7];
	v->bytemat[8] = ptr[*offset + 8];
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
		netmisc_encode_int8(ptr, offset, objp->ctype.ai_info.cur_path_index);
		netmisc_encode_int8(ptr, offset, objp->ctype.ai_info.dying_sound_playing);
		netmisc_encode_int32(ptr, offset, objp->ctype.ai_info.danger_laser_signature);
		netmisc_encode_int16(ptr, offset, objp->ctype.ai_info.danger_laser_num);
		netmisc_encode_int32(ptr, offset, objp->ctype.ai_info.dying_start_time);
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
		netmisc_encode_int32(ptr, offset, objp->ctype.powerup_info.creation_time);
		netmisc_encode_int32(ptr, offset, objp->ctype.powerup_info.flags);
		extra -= 12;
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
		netmisc_decode_int8(ptr, offset, (uint8_t*)&objp->ctype.ai_info.cur_path_index);
		netmisc_decode_int8(ptr, offset, (uint8_t*)&objp->ctype.ai_info.dying_sound_playing);
		netmisc_decode_int32(ptr, offset, &objp->ctype.ai_info.danger_laser_signature);
		netmisc_decode_int16(ptr, offset, &objp->ctype.ai_info.danger_laser_num);
		netmisc_decode_int32(ptr, offset, &objp->ctype.ai_info.dying_start_time);
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

