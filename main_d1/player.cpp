/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license. 
Instead, it is released under the terms of the MIT License.
*/
//[ISB] To be fair though, all it's doing is reading and writing a struct...
#include <stdio.h>

#include "cfile/cfile.h"

#include "player.h"

void P_ReadPlayer(player* plr, FILE* fp)
{
	int i;
	fread(&plr->callsign[0], 1, CALLSIGN_LEN + 1, fp);
	fread(&plr->net_address[0], 1, 4, fp);
	plr->net_port = F_ReadShort(fp);
	plr->connected = F_ReadByte(fp);
	plr->objnum = F_ReadInt(fp);
	plr->n_packets_got = F_ReadInt(fp);
	plr->n_packets_sent = F_ReadInt(fp);

	plr->flags = F_ReadInt(fp);
	plr->energy = F_ReadInt(fp);
	plr->shields = F_ReadInt(fp);
	plr->lives = F_ReadByte(fp);
	plr->level = F_ReadByte(fp);
	plr->laser_level = F_ReadByte(fp);
	plr->starting_level = F_ReadByte(fp);
	plr->killer_objnum = F_ReadShort(fp);
	plr->primary_weapon_flags = F_ReadByte(fp);
	plr->secondary_weapon_flags = F_ReadByte(fp);
	for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
		plr->primary_ammo[i] = F_ReadShort(fp);
	for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
		plr->secondary_ammo[i] = F_ReadShort(fp);

	plr->last_score = F_ReadInt(fp);
	plr->score = F_ReadInt(fp);
	plr->time_level = F_ReadInt(fp);
	plr->time_total = F_ReadInt(fp);

	plr->cloak_time = F_ReadInt(fp);
	plr->invulnerable_time = F_ReadInt(fp);

	plr->net_killed_total = F_ReadShort(fp);
	plr->net_kills_total = F_ReadShort(fp);
	plr->num_kills_level = F_ReadShort(fp);
	plr->num_kills_total = F_ReadShort(fp);
	plr->num_robots_level = F_ReadShort(fp);
	plr->num_robots_total = F_ReadShort(fp);
	plr->hostages_rescued_total = F_ReadShort(fp);
	plr->hostages_total = F_ReadShort(fp);
	plr->hostages_on_board = F_ReadByte(fp);
	plr->hostages_level = F_ReadByte(fp);
	plr->homing_object_dist = F_ReadInt(fp);
	plr->hours_level = F_ReadByte(fp);
	plr->hours_total = F_ReadByte(fp);
}

void P_WritePlayer(player* plr, FILE* fp)
{
	int i;
	fwrite(&plr->callsign[0], 1, CALLSIGN_LEN + 1, fp);
	fwrite(&plr->net_address[0], 1, 4, fp);
	F_WriteShort(fp, plr->net_port);
	F_WriteByte(fp, plr->connected);
	F_WriteInt(fp, plr->objnum);
	F_WriteInt(fp, plr->n_packets_got);
	F_WriteInt(fp, plr->n_packets_sent);

	F_WriteInt(fp, plr->flags);
	F_WriteInt(fp, plr->energy);
	F_WriteInt(fp, plr->shields);
	F_WriteByte(fp, plr->lives);
	F_WriteByte(fp, plr->level);
	F_WriteByte(fp, plr->laser_level);
	F_WriteByte(fp, plr->starting_level);
	F_WriteShort(fp, plr->killer_objnum);
	F_WriteByte(fp, plr->primary_weapon_flags);
	F_WriteByte(fp, plr->secondary_weapon_flags);
	for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
		F_WriteShort(fp, plr->primary_ammo[i]);
	for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
		F_WriteShort(fp, plr->secondary_ammo[i]);

	F_WriteInt(fp, plr->last_score);
	F_WriteInt(fp, plr->score);
	F_WriteInt(fp, plr->time_level);
	F_WriteInt(fp, plr->time_total);

	F_WriteInt(fp, plr->cloak_time);
	F_WriteInt(fp, plr->invulnerable_time);

	F_WriteShort(fp, plr->net_killed_total);
	F_WriteShort(fp, plr->net_kills_total);
	F_WriteShort(fp, plr->num_kills_level);
	F_WriteShort(fp, plr->num_kills_total);
	F_WriteShort(fp, plr->num_robots_level);
	F_WriteShort(fp, plr->num_robots_total);
	F_WriteShort(fp, plr->hostages_rescued_total);
	F_WriteShort(fp, plr->hostages_total);
	F_WriteByte(fp, plr->hostages_on_board);
	F_WriteByte(fp, plr->hostages_level);
	F_WriteInt(fp, plr->homing_object_dist);
	F_WriteByte(fp, plr->hours_level);
	F_WriteByte(fp, plr->hours_total);
}