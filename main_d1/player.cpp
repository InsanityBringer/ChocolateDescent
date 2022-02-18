/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license. 
Instead, it is released under the terms of the MIT License.
*/
//[ISB] To be fair though, all it's doing is reading and writing a struct...
#include <stdio.h>

#include "cfile/cfile.h"

#include "player.h"

void read_player_file(player* plr, FILE* fp)
{
	int i;
	fread(&plr->callsign[0], 1, CALLSIGN_LEN + 1, fp);
	fread(&plr->net_address[0], 1, 4, fp);
	plr->net_port = file_read_short(fp);
	plr->connected = file_read_byte(fp);
	plr->objnum = file_read_int(fp);
	plr->n_packets_got = file_read_int(fp);
	plr->n_packets_sent = file_read_int(fp);

	plr->flags = file_read_int(fp);
	plr->energy = file_read_int(fp);
	plr->shields = file_read_int(fp);
	plr->lives = file_read_byte(fp);
	plr->level = file_read_byte(fp);
	plr->laser_level = file_read_byte(fp);
	plr->starting_level = file_read_byte(fp);
	plr->killer_objnum = file_read_short(fp);
	plr->primary_weapon_flags = file_read_byte(fp);
	plr->secondary_weapon_flags = file_read_byte(fp);
	for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
		plr->primary_ammo[i] = file_read_short(fp);
	for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
		plr->secondary_ammo[i] = file_read_short(fp);

	plr->last_score = file_read_int(fp);
	plr->score = file_read_int(fp);
	plr->time_level = file_read_int(fp);
	plr->time_total = file_read_int(fp);

	plr->cloak_time = file_read_int(fp);
	plr->invulnerable_time = file_read_int(fp);

	plr->net_killed_total = file_read_short(fp);
	plr->net_kills_total = file_read_short(fp);
	plr->num_kills_level = file_read_short(fp);
	plr->num_kills_total = file_read_short(fp);
	plr->num_robots_level = file_read_short(fp);
	plr->num_robots_total = file_read_short(fp);
	plr->hostages_rescued_total = file_read_short(fp);
	plr->hostages_total = file_read_short(fp);
	plr->hostages_on_board = file_read_byte(fp);
	plr->hostages_level = file_read_byte(fp);
	plr->homing_object_dist = file_read_int(fp);
	plr->hours_level = file_read_byte(fp);
	plr->hours_total = file_read_byte(fp);
}

void write_player_file(player* plr, FILE* fp)
{
	int i;
	fwrite(&plr->callsign[0], 1, CALLSIGN_LEN + 1, fp);
	fwrite(&plr->net_address[0], 1, 4, fp);
	file_write_short(fp, plr->net_port);
	file_write_byte(fp, plr->connected);
	file_write_int(fp, plr->objnum);
	file_write_int(fp, plr->n_packets_got);
	file_write_int(fp, plr->n_packets_sent);

	file_write_int(fp, plr->flags);
	file_write_int(fp, plr->energy);
	file_write_int(fp, plr->shields);
	file_write_byte(fp, plr->lives);
	file_write_byte(fp, plr->level);
	file_write_byte(fp, plr->laser_level);
	file_write_byte(fp, plr->starting_level);
	file_write_short(fp, plr->killer_objnum);
	file_write_byte(fp, plr->primary_weapon_flags);
	file_write_byte(fp, plr->secondary_weapon_flags);
	for (i = 0; i < MAX_PRIMARY_WEAPONS; i++)
		file_write_short(fp, plr->primary_ammo[i]);
	for (i = 0; i < MAX_SECONDARY_WEAPONS; i++)
		file_write_short(fp, plr->secondary_ammo[i]);

	file_write_int(fp, plr->last_score);
	file_write_int(fp, plr->score);
	file_write_int(fp, plr->time_level);
	file_write_int(fp, plr->time_total);

	file_write_int(fp, plr->cloak_time);
	file_write_int(fp, plr->invulnerable_time);

	file_write_short(fp, plr->net_killed_total);
	file_write_short(fp, plr->net_kills_total);
	file_write_short(fp, plr->num_kills_level);
	file_write_short(fp, plr->num_kills_total);
	file_write_short(fp, plr->num_robots_level);
	file_write_short(fp, plr->num_robots_total);
	file_write_short(fp, plr->hostages_rescued_total);
	file_write_short(fp, plr->hostages_total);
	file_write_byte(fp, plr->hostages_on_board);
	file_write_byte(fp, plr->hostages_level);
	file_write_int(fp, plr->homing_object_dist);
	file_write_byte(fp, plr->hours_level);
	file_write_byte(fp, plr->hours_total);
}