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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "misc/types.h"
#include "misc/args.h"
#include "platform/timer.h"
#include "platform/mono.h"
#include "platform/i_net.h"
#include "newmenu.h"
#include "platform/key.h"
#include "gauges.h"
#include "object.h"
#include "misc/error.h"
#include "multi.h"
#include "network.h"
#include "netmisc.h"
#include "laser.h"
#include "gamesave.h"
#include "gamemine.h"
#include "player.h"
#include "gameseq.h"
#include "fireball.h"
#include "game.h"
#include "endlevel.h"
#include "2d/palette.h"
#include "fuelcen.h"
#include "menu.h"
#include "sounds.h"
#include "stringtable.h"
#include "kmatrix.h"
#include "newdemo.h"
#include "multibot.h"
#include "wall.h"
#include "bm.h"
#include "main_shared/effects.h"
#include "physics.h"
#include "platform/platform.h"
#include "misc/rand.h"

#ifdef SHAREWARE
#define PID_REQUEST 					11
#define PID_SYNC						13
#define PID_PDATA						14
#define PID_ADDPLAYER				15
#define PID_DUMP						17
#define PID_ENDLEVEL					18
#define PID_QUIT_JOINING			20
#define PID_OBJECT_DATA				21
#define PID_GAME_LIST				22
#define PID_GAME_INFO				24
#else
#define PID_REQUEST 					25
#define PID_SYNC						27
#define PID_PDATA						28
#define PID_ADDPLAYER				29
#define PID_DUMP						31
#define PID_ENDLEVEL					32
#define PID_QUIT_JOINING			34
#define PID_OBJECT_DATA				35
#define PID_GAME_LIST				36
#define PID_GAME_INFO				37
#endif

#define NETGAME_ANARCHY				0
#define NETGAME_TEAM_ANARCHY		1
#define NETGAME_ROBOT_ANARCHY		2
#define NETGAME_COOPERATIVE		3

#define MAX_ACTIVE_NETGAMES 	4
netgame_info Active_games[MAX_ACTIVE_NETGAMES];
int num_active_games = 0;

int	Network_debug = 0;
int	Network_active = 0;

int  	Network_status = 0;
int 	Network_games_changed = 0;

int 	Network_socket = 0;
int	Network_allow_socket_changes = 0;

uint16_t Current_Port = IPX_DEFAULT_SOCKET;

// For rejoin object syncing

int	Network_rejoined = 0;       // Did WE rejoin this game?
int	Network_new_game = 0;		 // Is this the first level of a new game?
int	Network_send_objects = 0;  // Are we in the process of sending objects to a player?
int 	Network_send_objnum = -1;   // What object are we sending next?
int	Network_player_added = 0;   // Is this a new player or a returning player?
int	Network_send_object_mode = 0; // What type of objects are we sending, static or dynamic?
sequence_packet	Network_player_rejoining; // Who is rejoining now?

fix	LastPacketTime[MAX_PLAYERS]; // For timeouts of idle/crashed players

int 	PacketUrgent = 0;

frame_info 	MySyncPack;
uint8_t 		MySyncPackInitialized = 0;		// Set to 1 if the MySyncPack is zeroed.
uint16_t 		my_segments_checksum = 0;

sequence_packet My_Seq;

extern obj_position Player_init[MAX_PLAYERS];

#define DUMP_CLOSED 0
#define DUMP_FULL 1
#define DUMP_ENDLEVEL 2
#define DUMP_DORK 3
#define DUMP_ABORTED 4
#define DUMP_CONNECTED 5
#define DUMP_LEVEL 6

int network_wait_for_snyc();
//there was a dark, dark time where this language didn't require prototypes at all. 
void network_flush();
void network_send_endlevel_sub(int player_num);
void network_listen();
void network_update_netgame(void);
void network_dump_player(uint8_t* node, int why);
void network_send_objects(void);
void network_send_rejoin_sync(int player_num);
void network_update_netgame(void);
void network_send_game_info(sequence_packet* their);
void network_send_data(uint8_t* ptr, int len, int urgent);
void network_read_sync_packet(netgame_info* sp);
void network_read_pdata_packet(frame_info* pd);
void network_read_object_packet(uint8_t* data);
void network_read_endlevel_packet(uint8_t* data);
void network_send_game_info_request_to(uint8_t* address);

void network_init(void)
{
	// So you want to play a netgame, eh?  Let's a get a few things
	// straight

	int save_pnum = Player_num;

	memset(&Netgame, 0, sizeof(netgame_info));
	memset(&My_Seq, 0, sizeof(sequence_packet));
	My_Seq.type = PID_REQUEST;
	memcpy(My_Seq.player.callsign, Players[Player_num].callsign, CALLSIGN_LEN + 1);
	memcpy(My_Seq.player.node, NetGetLocalAddress(), 4); //NOTE: Anyone reading a sequence packet needs to replace this with a proper address for the current network.
	memcpy(&My_Seq.player.identifier, My_Seq.player.node, sizeof(My_Seq.player.identifier));

	for (Player_num = 0; Player_num < MAX_NUM_NET_PLAYERS; Player_num++)
		init_player_stats_game();

	Player_num = save_pnum;
	multi_new_game();
	Network_new_game = 1;
	Fuelcen_control_center_destroyed = 0;
	network_flush();
}

#define ENDLEVEL_SEND_INTERVAL F1_0*2
#define ENDLEVEL_IDLE_TIME	F1_0*10

void
network_endlevel_poll(int nitems, newmenu_item * menus, int* key, int citem)
{
	// Polling loop for End-of-level menu

	static fix t1 = 0;
	int i = 0;
	int num_ready = 0;
	int num_escaped = 0;
	int goto_secret = 0;

	int previous_state[MAX_NUM_NET_PLAYERS];
	int previous_seconds_left;

	menus = menus;
	citem = citem;
	nitems = nitems;
	key = key;

	// Send our endlevel packet at regular intervals

	if (timer_get_approx_seconds() > t1 + ENDLEVEL_SEND_INTERVAL)
	{
		network_send_endlevel_packet();
		t1 = timer_get_approx_seconds();
	}

	for (i = 0; i < N_players; i++)
		previous_state[i] = Players[i].connected;

	previous_seconds_left = Fuelcen_seconds_left;

	network_listen();

	for (i = 0; i < N_players; i++)
	{
		if (previous_state[i] != Players[i].connected)
		{
			sprintf(menus[i].text, "%s %s", Players[i].callsign, CONNECT_STATES(Players[i].connected));
			menus[i].redraw = 1;
		}
		if (Players[i].connected == 1)
		{
			// Check timeout for idle players
			if (timer_get_approx_seconds() > LastPacketTime[i] + ENDLEVEL_IDLE_TIME)
			{
				mprintf((0, "idle timeout for player %d.\n", i));
				Players[i].connected = 0;
				network_send_endlevel_sub(i);
			}
		}

		if ((Players[i].connected != 1) && (Players[i].connected != 5) && (Players[i].connected != 6))
			num_ready++;
		if (Players[i].connected != 1)
			num_escaped++;
		if (Players[i].connected == 4)
			goto_secret = 1;
	}

	if (num_escaped == N_players) // All players are out of the mine
	{
		Fuelcen_seconds_left = -1;
	}

	if (previous_seconds_left != Fuelcen_seconds_left)
	{
		if (Fuelcen_seconds_left < 0)
		{
			sprintf(menus[N_players].text, TXT_REACTOR_EXPLODED);
			menus[N_players].redraw = 1;
		}
		else
		{
			sprintf(menus[N_players].text, "%s: %d %s  ", TXT_TIME_REMAINING, Fuelcen_seconds_left, TXT_SECONDS);
			menus[N_players].redraw = 1;
		}
	}

	if (num_ready == N_players) // All players have checked in or are disconnected
	{
		if (goto_secret)
			* key = -3;
		else
			*key = -2;
	}
}

void network_endlevel_poll2(int nitems, newmenu_item* menus, int* key, int citem)
{
	// Polling loop for End-of-level menu

	static fix t1 = 0;
	int i = 0;
	int num_ready = 0;
	int goto_secret = 0;

	menus = menus;
	citem = citem;
	nitems = nitems;
	key = key;

	// Send our endlevel packet at regular intervals

	if (timer_get_approx_seconds() > t1 + ENDLEVEL_SEND_INTERVAL)
	{
		network_send_endlevel_packet();
		t1 = timer_get_approx_seconds();
	}

	network_listen();

	for (i = 0; i < N_players; i++)
	{
		if ((Players[i].connected != 1) && (Players[i].connected != 5) && (Players[i].connected != 6))
			num_ready++;
		if (Players[i].connected == 4)
			goto_secret = 1;
	}

	if (num_ready == N_players) // All players have checked in or are disconnected
	{
		if (goto_secret)
			* key = -3;
		else
			*key = -2;
	}
}

int network_endlevel(int* secret)
{
	// Do whatever needs to be done between levels

	newmenu_item m[MAX_NUM_NET_PLAYERS + 1];
	char menu_text[MAX_NUM_NET_PLAYERS + 1][80];
	int i, choice;

	char text[80];

	Function_mode = FMODE_MENU;

	network_flush();

	Network_status = NETSTAT_ENDLEVEL; // We are between levels

	network_listen();

	network_send_endlevel_packet();

newmenu:
	// Setup menu text pointers and zero them
	for (i = 0; i < N_players; i++)
	{
		m[i].type = NM_TYPE_TEXT;
		m[i].text = menu_text[i];
		sprintf(m[i].text, "%s %s", Players[i].callsign, CONNECT_STATES(Players[i].connected));
		LastPacketTime[i] = timer_get_approx_seconds();
	}
	m[N_players].type = NM_TYPE_TEXT;
	m[N_players].text = menu_text[N_players];

	if (Fuelcen_seconds_left < 0)
		sprintf(m[N_players].text, TXT_REACTOR_EXPLODED);
	else
		sprintf(m[N_players].text, "%s: %d %s  ", TXT_TIME_REMAINING, Fuelcen_seconds_left, TXT_SECONDS);

menu:
	sprintf(text, "%s\n%s", TXT_WAITING, TXT_ESC_ABORT);

	choice = newmenu_do3(NULL, text, N_players + 1, m, network_endlevel_poll, 0, "STARS.PCX", 300, 160);

	if (choice == -1) {
		newmenu_item m2[2];

		m2[0].type = m2[1].type = NM_TYPE_MENU;
		m2[0].text = TXT_YES; m2[1].text = TXT_NO;
		choice = newmenu_do1(NULL, TXT_SURE_LEAVE_GAME, 2, m2, network_endlevel_poll2, 1);
		if (choice == 0)
		{
			Players[Player_num].connected = 0;
			network_send_endlevel_packet();
			network_send_endlevel_packet();
			longjmp(LeaveGame, 0);
		}
		if (choice > -2)
			goto newmenu;
	}

	//	kmatrix_view();

	if (choice > -2)
		goto menu;

	if (choice == -3)
		* secret = 1; // If any player went to the secret level, we go to the secret level

	network_send_endlevel_packet();
	network_send_endlevel_packet();
	MySyncPackInitialized = 0;

	network_update_netgame();

	return(0);
}

int can_join_netgame(netgame_info* game)
{
	// Can this player rejoin a netgame in progress?

	int i, num_players;

	if (game->game_status == NETSTAT_STARTING)
		return 1;

	if (game->game_status != NETSTAT_PLAYING)
		return 0;

	// Game is in progress, figure out if this guy can re-join it

	num_players = game->numplayers;

	if (!(game->game_flags & NETGAME_FLAG_CLOSED)) {
		// Look for player that is not connected
		if (game->numplayers < game->max_numplayers)
			return 1;

		for (i = 0; i < num_players; i++) {
			if (game->players[i].connected == 0)
				return 1;
		}
		return 0;
	}

	// Search to see if we were already in this closed netgame in progress


	for (i = 0; i < num_players; i++)
		if ((!_stricmp(Players[Player_num].callsign, game->players[i].callsign)) &&
			//(!memcmp(My_Seq.player.node, game->players[i].node, 4)) &&
			(!memcmp(&My_Seq.player.identifier, &game->players[i].identifier, sizeof(game->players[i].identifier))))
			break;

	if (i != num_players)
		return 1;
	return 0;
}

void
network_disconnect_player(int playernum)
{
	// A player has disconnected from the net game, take whatever steps are
	// necessary 

	if (playernum == Player_num)
	{
		Int3(); // Weird, see Rob
		return;
	}

	Players[playernum].connected = 0;
	Netgame.players[playernum].connected = 0;

	//	create_player_appearance_effect(&Objects[Players[playernum].objnum]);
	multi_make_player_ghost(playernum);

#ifndef SHAREWARE
	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_multi_disconnect(playernum);

	multi_strip_robots(playernum);
#endif
}

void network_new_player(sequence_packet* their)
{
	int objnum;
	int pnum;

	pnum = their->player.connected;

	Assert(pnum >= 0);
	Assert(pnum < MaxNumNetPlayers);

	NetGetLastPacketOrigin(their->player.node);

	objnum = Players[pnum].objnum;

#ifndef SHAREWARE
	if (Newdemo_state == ND_STATE_RECORDING) 
	{
		int new_player;

		if (pnum == N_players)
			new_player = 1;
		else
			new_player = 0;
		newdemo_record_multi_connect(pnum, new_player, their->player.callsign);
	}
#endif

	memcpy(Players[pnum].callsign, their->player.callsign, CALLSIGN_LEN + 1);
	memcpy(Netgame.players[pnum].callsign, their->player.callsign, CALLSIGN_LEN + 1);
	memcpy(Players[pnum].net_address, their->player.node, 4);

	memcpy(Netgame.players[pnum].node, their->player.node, 4);
	memcpy(&Netgame.players[pnum].identifier, &their->player.identifier, sizeof(their->player.identifier));

	NetGetLastPacketOrigin(Netgame.players[pnum].node);

	Players[pnum].n_packets_got = 0;
	Players[pnum].connected = 1;
	Players[pnum].net_kills_total = 0;
	Players[pnum].net_killed_total = 0;
	memset(kill_matrix[pnum], 0, MAX_PLAYERS * sizeof(short));
	Players[pnum].score = 0;
	Players[pnum].flags = 0;

	if (pnum == N_players)
	{
		N_players++;
		Netgame.numplayers = N_players;
	}

	digi_play_sample(SOUND_HUD_MESSAGE, F1_0);

	HUD_init_message("'%s' %s\n", their->player.callsign, TXT_JOINING);

	multi_make_ghost_player(pnum);

#ifndef SHAREWARE
	multi_send_score();
#endif

	//	create_player_appearance_effect(&Objects[objnum]);
}

void network_welcome_player(sequence_packet* their)
{
	// Add a player to a game already in progress
	uint8_t local_address[4];
	int player_num;
	int i;

	NetGetLastPacketOrigin(their->player.node);

	// Don't accept new players if we're ending this level.  Its safe to
	// ignore since they'll request again later

	if ((Endlevel_sequence) || (Fuelcen_control_center_destroyed))
	{
		mprintf((0, "Ignored request from new player to join during endgame.\n"));
		network_dump_player(their->player.node, DUMP_ENDLEVEL);
		return;
	}

	if (Network_send_objects)
	{
		// Ignore silently, we're already responding to someone and we can't
		// do more than one person at a time.  If we don't dump them they will
		// re-request in a few seconds.
		return;
	}

	if (their->player.connected != Current_level_num)
	{
		mprintf((0, "Dumping player due to old level number.\n"));
		network_dump_player(their->player.node, DUMP_LEVEL);
		return;
	}

	player_num = -1;
	memset(&Network_player_rejoining, 0, sizeof(sequence_packet));
	Network_player_added = 0;

	memcpy(local_address, their->player.node, 4);

	for (i = 0; i < N_players; i++)
	{
		if ((!_stricmp(Players[i].callsign, their->player.callsign)) && (!memcmp(Players[i].net_address, local_address, 4)))
		{
			player_num = i;
			break;
		}
	}

	if (player_num == -1)
	{
		// Player is new to this game

		if (!(Netgame.game_flags & NETGAME_FLAG_CLOSED) && (N_players < MaxNumNetPlayers))
		{
			// Add player in an open slot, game not full yet

			player_num = N_players;
			Network_player_added = 1;
		}
		else if (Netgame.game_flags & NETGAME_FLAG_CLOSED)
		{
			// Slots are open but game is closed

			network_dump_player(their->player.node, DUMP_CLOSED);
			return;
		}
		else
		{
			// Slots are full but game is open, see if anyone is
			// disconnected and replace the oldest player with this new one

			int oldest_player = -1;
			fix oldest_time = timer_get_approx_seconds();

			Assert(N_players == MaxNumNetPlayers);

			for (i = 0; i < N_players; i++)
			{
				if ((!Players[i].connected) && (LastPacketTime[i] < oldest_time))
				{
					oldest_time = LastPacketTime[i];
					oldest_player = i;
				}
			}

			if (oldest_player == -1)
			{
				// Everyone is still connected 

				network_dump_player(their->player.node, DUMP_FULL);
				return;
			}
			else
			{
				// Found a slot!

				player_num = oldest_player;
				Network_player_added = 1;
			}
		}
	}
	else
	{
		// Player is reconnecting

		if (Players[player_num].connected)
		{
			mprintf((0, "Extra REQUEST from player ignored.\n"));
			return;
		}

#ifndef SHAREWARE
		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_multi_reconnect(player_num);
#endif

		Network_player_added = 0;

		digi_play_sample(SOUND_HUD_MESSAGE, F1_0);

		HUD_init_message("'%s' %s", Players[player_num].callsign, TXT_REJOIN);
	}

	// Send updated Objects data to the new/returning player

	Network_player_rejoining = *their;
	Network_player_rejoining.player.connected = player_num;
	Network_send_objects = 1;
	Network_send_objnum = -1;

	network_send_objects();
}

int network_objnum_is_past(int objnum)
{
	// determine whether or not a given object number has already been sent
	// to a re-joining player.

	int player_num = Network_player_rejoining.player.connected;
	int obj_mode = !((object_owner[objnum] == -1) || (object_owner[objnum] == player_num));

	if (!Network_send_objects)
		return 0; // We're not sending objects to a new player

	if (obj_mode > Network_send_object_mode)
		return 0;
	else if (obj_mode < Network_send_object_mode)
		return 1;
	else if (objnum < Network_send_objnum)
		return 1;
	else
		return 0;
}

#define OBJ_PACKETS_PER_FRAME 1

#ifndef SHAREWARE
void network_send_door_updates(void)
{
	// Send door status when new player joins

	int i;

	for (i = 0; i < Num_walls; i++)
	{
		if ((Walls[i].type == WALL_DOOR) && ((Walls[i].state == WALL_DOOR_OPENING) || (Walls[i].state == WALL_DOOR_WAITING)))
			multi_send_door_open(Walls[i].segnum, Walls[i].sidenum);
		else if ((Walls[i].type == WALL_BLASTABLE) && (Walls[i].flags & WALL_BLASTED))
			multi_send_door_open(Walls[i].segnum, Walls[i].sidenum);
		else if ((Walls[i].type == WALL_BLASTABLE) && (Walls[i].hps != WALL_HPS))
			multi_send_hostage_door_status(i);
	}

}

void network_process_monitor_vector(int vector)
{
	int i, j;
	int count = 0;
	segment* seg;

	for (i = 0; i <= Highest_segment_index; i++)
	{
		int tm, ec, bm;
		seg = &Segments[i];
		for (j = 0; j < 6; j++)
		{
			if (((tm = seg->sides[j].tmap_num2) != 0) &&
				((ec = TmapInfo[tm & 0x3fff].eclip_num) != -1) &&
				((bm = Effects[ec].dest_bm_num) != -1))
			{
				if (vector & (1 << count))
				{
					seg->sides[j].tmap_num2 = bm | (tm & 0xc000);
					mprintf((0, "Monitor %d blown up.\n", count));
				}
				else
					mprintf((0, "Monitor %d intact.\n", count));
				count++;
				Assert(count < 32);
			}
		}
	}
}

int network_create_monitor_vector(void)
{
	int i, j, k;
	int num_blown_bitmaps = 0;
	int monitor_num = 0;
	int blown_bitmaps[7];
	int vector = 0;
	segment* seg;

	for (i = 0; i < Num_effects; i++)
	{
		if (Effects[i].dest_bm_num > 0) {
			for (j = 0; j < num_blown_bitmaps; j++)
				if (blown_bitmaps[j] == Effects[i].dest_bm_num)
					break;
			if (j == num_blown_bitmaps)
				blown_bitmaps[num_blown_bitmaps++] = Effects[i].dest_bm_num;
		}
	}

	for (i = 0; i < num_blown_bitmaps; i++)
		mprintf((0, "Blown bitmap #%d = %d.\n", i, blown_bitmaps[i]));

	Assert(num_blown_bitmaps <= 7);

	for (i = 0; i <= Highest_segment_index; i++)
	{
		int tm, ec;
		seg = &Segments[i];
		for (j = 0; j < 6; j++)
		{
			if ((tm = seg->sides[j].tmap_num2) != 0)
			{
				if (((ec = TmapInfo[tm & 0x3fff].eclip_num) != -1) &&
					(Effects[ec].dest_bm_num != -1))
				{
					mprintf((0, "Monitor %d intact.\n", monitor_num));
					monitor_num++;
					Assert(monitor_num < 32);
				}
				else
				{
					for (k = 0; k < num_blown_bitmaps; k++)
					{
						if ((tm & 0x3fff) == blown_bitmaps[k])
						{
							mprintf((0, "Monitor %d destroyed.\n", monitor_num));
							vector |= (1 << monitor_num);
							monitor_num++;
							Assert(monitor_num < 32);
							break;
						}
					}
				}
			}
		}
	}
	mprintf((0, "Final monitor vector %x.\n", vector));
	return(vector);
}
#endif

void network_stop_resync(sequence_packet* their)
{
	NetGetLastPacketOrigin(their->player.node);

	if ((!memcmp(&Network_player_rejoining.player.identifier, &their->player.identifier, sizeof(their->player.identifier))) &&
		(!_stricmp(Network_player_rejoining.player.callsign, their->player.callsign)))
	{
		mprintf((0, "Aborting resync for player %s.\n", their->player.callsign));
		Network_send_objects = 0;
		Network_send_objnum = -1;
	}
}

uint8_t object_buffer[IPX_MAX_DATA_SIZE];

void network_send_objects(void)
{
	short remote_objnum;
	int8_t owner;
	int loc, i, h;

	static int obj_count = 0;
	static int frame_num = 0;

	int obj_count_frame = 0;
	int player_num = Network_player_rejoining.player.connected;

	// Send clear objects array trigger and send player num

	Assert(Network_send_objects != 0);
	Assert(player_num >= 0);
	Assert(player_num < MaxNumNetPlayers);

	if (Endlevel_sequence || Fuelcen_control_center_destroyed)
	{
		// Endlevel started before we finished sending the goods, we'll
		// have to stop and try again after the level.

		network_dump_player(Network_player_rejoining.player.node, DUMP_ENDLEVEL);
		Network_send_objects = 0;
		return;
	}

	for (h = 0; h < OBJ_PACKETS_PER_FRAME; h++) // Do more than 1 per frame, try to speed it up without
															  // over-stressing the receiver.
	{
		obj_count_frame = 0;
		memset(object_buffer, 0, IPX_MAX_DATA_SIZE);
		object_buffer[0] = PID_OBJECT_DATA;
		loc = 3;

		if (Network_send_objnum == -1)
		{
			obj_count = 0;
			Network_send_object_mode = 0;
			//			mprintf((0, "Sending object array to player %d.\n", player_num));
			//*(short*)(object_buffer + loc) = -1;	loc += 2;
			netmisc_encode_int16(object_buffer, &loc, -1);
			object_buffer[loc] = player_num; 		loc += 1;
			netmisc_encode_int16(object_buffer, &loc, 0);
			//loc += 2; // Placeholder for remote_objnum, not used here
			Network_send_objnum = 0;
			obj_count_frame = 1;
			frame_num = 0;
		}

		for (i = Network_send_objnum; i <= Highest_object_index; i++)
		{
			if ((Objects[i].type != OBJ_POWERUP) && (Objects[i].type != OBJ_PLAYER) &&
				(Objects[i].type != OBJ_CNTRLCEN) && (Objects[i].type != OBJ_GHOST) &&
				(Objects[i].type != OBJ_ROBOT) && (Objects[i].type != OBJ_HOSTAGE))
				continue;
			if ((Network_send_object_mode == 0) && ((object_owner[i] != -1) && (object_owner[i] != player_num)))
				continue;
			if ((Network_send_object_mode == 1) && ((object_owner[i] == -1) || (object_owner[i] == player_num)))
				continue;

			if (((IPX_MAX_DATA_SIZE - 1) - loc) < (sizeof(object) + 5))
				break; // Not enough room for another object

			obj_count_frame++;
			obj_count++;

			remote_objnum = objnum_local_to_remote((short)i, &owner);
			Assert(owner == object_owner[i]);

			netmisc_encode_int16(object_buffer, &loc, i);
			netmisc_encode_int8(object_buffer, &loc, owner);
			netmisc_encode_int16(object_buffer, &loc, remote_objnum);
			netmisc_encode_object(object_buffer, &loc, &Objects[i]);
			//mprintf((0, "..packing object %d, remote %d\n", i, remote_objnum));
		}

		if (obj_count_frame) // Send any objects we've buffered
		{
			frame_num++;

			Network_send_objnum = i;
			object_buffer[1] = obj_count_frame;
			object_buffer[2] = frame_num;
			//mprintf((0, "Object packet %d contains %d objects.\n", frame_num, obj_count_frame));

			Assert(loc <= IPX_MAX_DATA_SIZE);

			NetSendInternetworkPacket(object_buffer, loc, Network_player_rejoining.player.node);
		}

		if (i > Highest_object_index)
		{
			if (Network_send_object_mode == 0)
			{
				Network_send_objnum = 0;
				Network_send_object_mode = 1; // go to next mode
			}
			else
			{
				Assert(Network_send_object_mode == 1);

				frame_num++;
				// Send count so other side can make sure he got them all
//				mprintf((0, "Sending end marker in packet #%d.\n", frame_num));
				mprintf((0, "Sent %d objects.\n", obj_count));
				object_buffer[0] = PID_OBJECT_DATA;
				object_buffer[1] = 1;
				object_buffer[2] = frame_num;
				loc = 3;
				netmisc_encode_int16(object_buffer, &loc, -2); loc++;
				netmisc_encode_int16(object_buffer, &loc, obj_count);
				NetSendInternetworkPacket(object_buffer, 8, Network_player_rejoining.player.node);

				// Send sync packet which tells the player who he is and to start!
				network_send_rejoin_sync(player_num);

				// Turn off send object mode
				Network_send_objnum = -1;
				Network_send_objects = 0;
				obj_count = 0;
				return;
			} // mode == 1;
		} // i > Highest_object_index
	} // For PACKETS_PER_FRAME
}

void network_send_rejoin_sync(int player_num)
{
	int i, j;
	int len = 0;
	uint8_t buf[1024];

	Players[player_num].connected = 1; // connect the new guy
	LastPacketTime[player_num] = timer_get_approx_seconds();

	if (Endlevel_sequence || Fuelcen_control_center_destroyed)
	{
		// Endlevel started before we finished sending the goods, we'll
		// have to stop and try again after the level.

		network_dump_player(Network_player_rejoining.player.node, DUMP_ENDLEVEL);
		Network_send_objects = 0;
		return;
	}

	if (Network_player_added)
	{
		Network_player_rejoining.type = PID_ADDPLAYER;
		Network_player_rejoining.player.connected = player_num;
		network_new_player(&Network_player_rejoining);

		len = 0;
		netmisc_encode_sequence_packet(buf, &len, &Network_player_rejoining);

		for (i = 0; i < N_players; i++)
		{
			if ((i != player_num) && (i != Player_num) && (Players[i].connected))
				NetSendPacket(buf, len, Netgame.players[i].node, Players[i].net_address);
		}
	}

	// Send sync packet to the new guy

	network_update_netgame();

	// Fill in the kill list
	for (j = 0; j < MAX_PLAYERS; j++)
	{
		for (i = 0; i < MAX_PLAYERS; i++)
			Netgame.kills[j][i] = kill_matrix[j][i];
		Netgame.killed[j] = Players[j].net_killed_total;
		Netgame.player_kills[j] = Players[j].net_kills_total;
#ifndef SHAREWARE
		Netgame.player_score[j] = Players[j].score;
#endif
	}

#ifndef SHAREWARE
	Netgame.level_time = Players[Player_num].time_level;
	Netgame.monitor_vector = network_create_monitor_vector();
#endif

	mprintf((0, "Sending rejoin sync packet!!!\n"));

	len = 0;
	netmisc_encode_netgameinfo(buf, &len, &Netgame);

	NetSendInternetworkPacket(buf, len, Network_player_rejoining.player.node);
	NetSendInternetworkPacket(buf, len, Network_player_rejoining.player.node); // repeat for safety
	NetSendInternetworkPacket(buf, len, Network_player_rejoining.player.node); // repeat for safety

#ifndef SHAREWARE
	network_send_door_updates();
#endif

	return;
}

char* network_get_player_name(int objnum)
{
	if (objnum < 0) return NULL;
	if (Objects[objnum].type != OBJ_PLAYER) return NULL;
	if (Objects[objnum].id >= MAX_PLAYERS) return NULL;
	if (Objects[objnum].id >= N_players) return NULL;

	return Players[Objects[objnum].id].callsign;
}


void network_add_player(sequence_packet* p)
{
	int i;

	mprintf((0, "Got add player request!\n"));
	//[ISB] The protocol isn't always reliable at specifying an address, and this won't work over the internet either.
	//Always replace what address this player said they have with the address they actually sent from.
	//Port (currently saved in server...) should be trustable. 
	NetGetLastPacketOrigin(p->player.node);

	for (i = 0; i < N_players; i++)
	{
		//[ISB] Can't compare "server" (actually port, refactoring incoming...) anymore, as a client's port can change.
		if (!memcmp(&Netgame.players[i].identifier, &p->player.identifier, sizeof(p->player.identifier)))
			return;		// already got them
	}

	if (N_players >= MAX_PLAYERS)
		return;		// too many of em

	memcpy(Netgame.players[N_players].callsign, p->player.callsign, CALLSIGN_LEN + 1);
	memcpy(Netgame.players[N_players].node, p->player.node, 4);
	memcpy(&Netgame.players[N_players].identifier, &p->player.identifier, sizeof(p->player.identifier));

	Netgame.players[N_players].connected = 1;
	Players[N_players].connected = 1;
	LastPacketTime[N_players] = timer_get_approx_seconds();
	N_players++;
	Netgame.numplayers = N_players;

	// Broadcast updated info

	network_send_game_info(NULL);
}

// One of the players decided not to join the game

void network_remove_player(sequence_packet* p)
{
	int i, pn;

	NetGetLastPacketOrigin(p->player.node);

	pn = -1;
	for (i = 0; i < N_players; i++) 
	{
		if (!memcmp(&Netgame.players[i].identifier, &p->player.identifier, sizeof(p->player.identifier))) 
		{
			pn = i;
			break;
		}
	}

	if (pn < 0) return;

	for (i = pn; i < N_players - 1; i++) 
	{
		memcpy(Netgame.players[i].callsign, Netgame.players[i + 1].callsign, CALLSIGN_LEN + 1);
		memcpy(Netgame.players[i].node, Netgame.players[i + 1].node, 4);
		memcpy(&Netgame.players[i].identifier, &Netgame.players[i + 1].identifier, sizeof(Netgame.players[i].identifier));
	}

	N_players--;
	Netgame.numplayers = N_players;

	// Broadcast new info

	network_send_game_info(NULL);

}

void
network_dump_player(uint8_t* node, int why)
{
	// Inform player that he was not chosen for the netgame
	uint8_t buf[128];
	int len = 0;
	sequence_packet temp;

	temp.type = PID_DUMP;
	memcpy(temp.player.callsign, Players[Player_num].callsign, CALLSIGN_LEN + 1);
	temp.player.connected = why;
	netmisc_encode_sequence_packet(buf, &len, &temp);
	NetSendInternetworkPacket(buf, len, node);
}

void
network_send_game_list_request(void)
{
	// Send a broadcast request for game info
	uint8_t buf[128];
	int len = 0;
	sequence_packet me;

	mprintf((0, "Sending game_list request.\n"));
	memcpy(me.player.callsign, Players[Player_num].callsign, CALLSIGN_LEN + 1);
	memcpy(me.player.node, NetGetLocalAddress(), 4);
	memcpy(&me.player.identifier, &My_Seq.player.identifier, sizeof(My_Seq.player.identifier));
	me.type = PID_GAME_LIST;

	netmisc_encode_sequence_packet(buf, &len, &me);
	NetSendBroadcastPacket((uint8_t*)& me, len);
}

void network_send_game_info_request_to(uint8_t* address)
{
	// Send a broadcast request for game info
	uint8_t buf[128];
	int len = 0;
	sequence_packet me;

	mprintf((0, "Sending game_list request.\n"));
	memcpy(me.player.callsign, Players[Player_num].callsign, CALLSIGN_LEN + 1);
	memcpy(me.player.node, NetGetLocalAddress(), 4);
	memcpy(&me.player.identifier, &My_Seq.player.identifier, sizeof(My_Seq.player.identifier));
	me.type = PID_GAME_LIST;

	netmisc_encode_sequence_packet(buf, &len, &me);
	NetSendInternetworkPacket((uint8_t*)&me, len, address);
}

void
network_update_netgame(void)
{
	// Update the netgame struct with current game variables

	int i, j;

	if (Network_status == NETSTAT_STARTING)
		return;

	Netgame.numplayers = N_players;
	Netgame.game_status = Network_status;
	Netgame.max_numplayers = MaxNumNetPlayers;

	for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	{
		Netgame.players[i].connected = Players[i].connected;
		for (j = 0; j < MAX_NUM_NET_PLAYERS; j++)
			Netgame.kills[i][j] = kill_matrix[i][j];
		Netgame.killed[i] = Players[i].net_killed_total;
		Netgame.player_kills[i] = Players[i].net_kills_total;
#ifndef SHAREWARE
		Netgame.player_score[i] = Players[i].score;
		Netgame.player_flags[i] = (Players[i].flags & (PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY));
#endif
	}
	Netgame.team_kills[0] = team_kills[0];
	Netgame.team_kills[1] = team_kills[1];
	Netgame.levelnum = Current_level_num;
}

void
network_send_endlevel_sub(int player_num)
{
	endlevel_info end;
	int i, len = 0;
	uint8_t buf[1024];

	// Send an endlevel packet for a player
	end.type = PID_ENDLEVEL;
	end.player_num = player_num;
	end.connected = Players[player_num].connected;
	end.kills = Players[player_num].net_kills_total;
	end.killed = Players[player_num].net_killed_total;
	memcpy(end.kill_matrix, kill_matrix[player_num], MAX_PLAYERS * sizeof(short));

	if (Players[player_num].connected == 1) // Still playing
	{
		Assert(Fuelcen_control_center_destroyed);
		end.seconds_left = Fuelcen_seconds_left;
	}

	//	mprintf((0, "Sending endlevel packet.\n"));

	netmisc_encode_endlevel_info(buf, &len, &end);

	for (i = 0; i < N_players; i++)
	{
		if ((i != Player_num) && (i != player_num) && (Players[i].connected))
			NetSendPacket(buf, len, Netgame.players[i].node, Players[i].net_address);
	}
}

void
network_send_endlevel_packet(void)
{
	// Send an updated endlevel status to other hosts

	network_send_endlevel_sub(Player_num);
}

void network_send_game_info(sequence_packet* their)
{
	// Send game info to someone who requested it
	int i, len = 0;
	char old_type, old_status;
	uint8_t buf[1024];

	mprintf((0, "Sending game info.\n"));

	network_update_netgame(); // Update the values in the netgame struct

	old_type = Netgame.type;
	old_status = Netgame.game_status;

	Netgame.type = PID_GAME_INFO;
	if (Endlevel_sequence || Fuelcen_control_center_destroyed)
		Netgame.game_status = NETSTAT_ENDLEVEL;

	netmisc_encode_netgameinfo(buf, &len, &Netgame);

	if (!their)
	{
		//NetSendBroadcastPacket((uint8_t*)& Netgame, sizeof(netgame_info));

		//Can't just broadcast this since all our clients are hypothetically on different ports.
		//A mess, but it'll hopefully work. 
		for (i = 0; i < N_players; i++)
		{
			NetSendInternetworkPacket(buf, len, Netgame.players[i].node);
		}
	}
	else
	{
		NetGetLastPacketOrigin(their->player.node); //Ensure we're sending to the correct place.
		NetSendInternetworkPacket(buf, len, their->player.node);
	}

	Netgame.type = old_type;
	Netgame.game_status = old_status;
}

int network_send_request(void)
{
	// Send a request to join a game 'Netgame'.  Returns 0 if we can join this
	// game, non-zero if there is some problem.
	int i, len = 0;
	uint8_t buf[128];

	Assert(Netgame.numplayers > 0);

	for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
		if (Netgame.players[i].connected)
			break;

	Assert(i < MAX_NUM_NET_PLAYERS);

	mprintf((0, "Sending game enroll request to player %d.  Level = %d\n", i, Netgame.levelnum));

	//	segments_checksum = netmisc_calc_checksum( Segments, sizeof(segment)*(Highest_segment_index+1) ); 	

	My_Seq.type = PID_REQUEST;
	My_Seq.player.connected = Current_level_num;

	netmisc_encode_sequence_packet(buf, &len, &My_Seq);
	NetSendInternetworkPacket(buf, len, Netgame.players[i].node);
	return i;
}

void network_process_gameinfo(uint8_t* data)
{
	int i, j;
	netgame_info newgame;
	int len = 0;
	uint8_t origin_address[4];
	int connected_to = -1;

	//newgame = (netgame_info*)data;
	netmisc_decode_netgameinfo(data, &len, &newgame);

	Network_games_changed = 1;

	mprintf((0, "Got game data for game %s.\n", newgame.game_name));
	NetGetLastPacketOrigin(origin_address);
	for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	{
		if (!memcmp(origin_address, newgame.players[i].node, 4))
		{
			connected_to = i;
			break;
		}
	}

	if (connected_to == -1)
	{
		memcpy(newgame.players[0].node, origin_address, 4);
		mprintf((0, "bashing player 0 in gameinfo\n"));
	}

	for (i = 0; i < num_active_games; i++)
		if (!_stricmp(Active_games[i].game_name, newgame.game_name) && !memcmp(&Active_games[i].players[0].identifier, &newgame.players[0].identifier, sizeof(newgame.players[0].identifier)))
			break;

	if (i == MAX_ACTIVE_NETGAMES)
	{
		mprintf((0, "Too many netgames.\n"));
		return;
	}
	memcpy(&Active_games[i], &newgame, sizeof(netgame_info));
	if (i == num_active_games)
		num_active_games++;

	if (Active_games[i].numplayers == 0)
	{
		// Delete this game
		for (j = i; j < num_active_games - 1; j++)
			memcpy(&Active_games[j], &Active_games[j + 1], sizeof(netgame_info));
		num_active_games--;
	}
}

void network_process_dump(sequence_packet* their)
{
	// Our request for join was denied.  Tell the user why.

	mprintf((0, "Dumped by player %s, type %d.\n", their->player.callsign, their->player.connected));

	nm_messagebox(NULL, 1, TXT_OK, NET_DUMP_STRINGS(their->player.connected));
	Network_status = NETSTAT_MENU;
}

void network_process_request(sequence_packet* their)
{
	// Player is ready to receieve a sync packet
	int i;

	mprintf((0, "Player %s ready for sync.\n", their->player.callsign));
	NetGetLastPacketOrigin(their->player.node); //ensure correct node

	for (i = 0; i < N_players; i++)
		if (!memcmp(their->player.node, Netgame.players[i].node, 4) && (!_stricmp(their->player.callsign, Netgame.players[i].callsign))) 
		{
			Players[i].connected = 1;
			break;
		}
}

void network_process_packet(uint8_t* data, int length)
{
	//sequence_packet* their = (sequence_packet*)data;
	sequence_packet their;
	int sqlen = 0;
	netmisc_decode_sequence_packet(data, &sqlen, &their);

	//	mprintf( (0, "Got packet of length %d, type %d\n", length, their->type ));

	//	if ( length < sizeof(sequence_packet) ) return;

	length = length;

	switch (their.type) 
	{

	case PID_GAME_INFO:
		mprintf((0, "GOT a PID_GAME_INFO!\n"));
		if (length != sizeof(netgame_info))
			mprintf((0, " Invalid size %d for netgame packet.\n", length));
		if (Network_status == NETSTAT_BROWSING)
			network_process_gameinfo(data);
		break;
	case PID_GAME_LIST:
		// Someone wants a list of games
		mprintf((0, "Got a PID_GAME_LIST!\n"));
		if ((Network_status == NETSTAT_PLAYING) || (Network_status == NETSTAT_STARTING) || (Network_status == NETSTAT_ENDLEVEL))
			if (network_i_am_master())
				network_send_game_info(&their);
		break;
	case PID_ADDPLAYER:
		mprintf((0, "Got NEWPLAYER message from %s.\n", their.player.callsign));
		network_new_player(&their);
		break;
	case PID_REQUEST:
		mprintf((0, "Got REQUEST from '%s'\n", their.player.callsign));
		if (Network_status == NETSTAT_STARTING)
		{
			// Someone wants to join our game!
			network_add_player(&their);
		}
		else if (Network_status == NETSTAT_WAITING)
		{
			// Someone is ready to recieve a sync packet
			network_process_request(&their);
		}
		else if (Network_status == NETSTAT_PLAYING)
		{
			// Someone wants to join a game in progress!
			network_welcome_player(&their);
		}
		break;
	case PID_DUMP:
		if (Network_status == NETSTAT_WAITING)
			network_process_dump(&their);
		break;
	case PID_QUIT_JOINING:
		if (Network_status == NETSTAT_STARTING)
			network_remove_player(&their);
		else if ((Network_status == NETSTAT_PLAYING) && (Network_send_objects))
			network_stop_resync(&their);
		break;
	case PID_SYNC:
		if (Network_status == NETSTAT_WAITING)
		{
			int len = 0;
			netgame_info syncinfo;
			netmisc_decode_netgameinfo(data, &len, &syncinfo);
			network_read_sync_packet(&syncinfo);
		}
		break;
	case PID_PDATA:
		if ((Game_mode & GM_NETWORK) && ((Network_status == NETSTAT_PLAYING) || (Network_status == NETSTAT_ENDLEVEL))) 
		{
			int len = 0;
			frame_info frameinfo;
			netmisc_decode_frame_info(data, &len, &frameinfo);
			network_read_pdata_packet(&frameinfo);
		}
		break;
	case PID_OBJECT_DATA:
		if (Network_status == NETSTAT_WAITING)
			network_read_object_packet(data);
		break;
	case PID_ENDLEVEL:
		if ((Network_status == NETSTAT_ENDLEVEL) || (Network_status == NETSTAT_PLAYING))
			network_read_endlevel_packet(data);
		else
			mprintf((0, "Junked endlevel packet.\n"));
		break;
	default:
		mprintf((0, "Ignoring invalid packet type.\n"));
		Int3(); // Invalid network packet type, see ROB
	}
}

#ifndef NDEBUG
void dump_segments()
{
	FILE* fp;

	fp = fopen("TEST.DMP", "wb");
	fwrite(Segments, sizeof(segment) * (Highest_segment_index + 1), 1, fp);
	fclose(fp);
	mprintf((0, "SS=%d\n", sizeof(segment)));
}
#endif

void
network_read_endlevel_packet(uint8_t* data)
{
	// Special packet for end of level syncing

	int playernum;
	int len = 0;
	endlevel_info end;
	netmisc_decode_endlevel_info(data, &len, &end);

	playernum = end.player_num;

	Assert(playernum != Player_num);
	Assert(playernum < N_players);

	if ((Network_status == NETSTAT_PLAYING) && (end.connected != 0))
		return; // Only accept disconnect packets if we're not out of the level yet

	Players[playernum].connected = end.connected;
	memcpy(&kill_matrix[playernum][0], end.kill_matrix, MAX_PLAYERS * sizeof(short));
	Players[playernum].net_kills_total = end.kills;
	Players[playernum].net_killed_total = end.killed;

	if ((Players[playernum].connected == 1) && (end.seconds_left < Fuelcen_seconds_left))
		Fuelcen_seconds_left = end.seconds_left;

	LastPacketTime[playernum] = timer_get_approx_seconds();

	//	mprintf((0, "Got endlevel packet from player %d.\n", playernum));
}

void
network_pack_objects(void)
{
	// Switching modes, pack the object array

	special_reset_objects();
}

int
network_verify_objects(int remote, int local)
{
	int i;
	int nplayers, got_controlcen = 0;

	if ((remote - local) > 10)
		return(-1);

	//[ISB] Many fan levels don't use reactors, so allow connections otherwise. 
	//Not very chocolately, but it's a bit late...
	//if (Game_mode & GM_MULTI_ROBOTS)
	//	got_controlcen = 1;

	nplayers = 0;

	for (i = 0; i <= Highest_object_index; i++)
	{
		if ((Objects[i].type == OBJ_PLAYER) || (Objects[i].type == OBJ_GHOST))
			nplayers++;
		//if (Objects[i].type == OBJ_CNTRLCEN)
		//	got_controlcen = 1;
	}

	if (/*got_controlcen &&*/ (nplayers >= MaxNumNetPlayers))
		return(0);

	return(1);
}

void network_read_object_packet(uint8_t* data)
{
	// Object from another net player we need to sync with

	short objnum, remote_objnum;
	int8_t obj_owner;
	int segnum, i;
	object* obj;

	static int my_pnum = 0;
	static int mode = 0;
	static int object_count = 0;
	static int frame_num = 0;
	int nobj = data[1];
	int loc = 3;
	int remote_frame_num = data[2];

	frame_num++;

	//	mprintf((0, "Object packet %d (remote #%d) contains %d objects.\n", frame_num, remote_frame_num, nobj));

	for (i = 0; i < nobj; i++)
	{
		//objnum = *(short*)(data + loc);			loc += 2;
		netmisc_decode_int16(data, &loc, &objnum);
		obj_owner = data[loc];						loc += 1;
		//remote_objnum = *(short*)(data + loc);	loc += 2;
		netmisc_decode_int16(data, &loc, &remote_objnum);

		if (objnum == -1)
		{
			// Clear object array
			mprintf((0, "Clearing object array.\n"));

			init_objects();
			Network_rejoined = 1;
			my_pnum = obj_owner;
			change_playernum_to(my_pnum);
			mode = 1;
			object_count = 0;
			frame_num = 1;
		}
		else if (objnum == -2)
		{
			// Special debug checksum marker for entire send
			if (mode == 1)
			{
				network_pack_objects();
				mode = 0;
			}
			mprintf((0, "Objnum -2 found in frame local %d remote %d.\n", frame_num, remote_frame_num));
			mprintf((0, "Got %d objects, expected %d.\n", object_count, remote_objnum));
			if (remote_objnum != object_count) 
			{
				Int3();
			}
			if (network_verify_objects(remote_objnum, object_count))
			{
				// Failed to sync up 
				nm_messagebox(NULL, 1, TXT_OK, TXT_NET_SYNC_FAILED);
				Network_status = NETSTAT_MENU;
				return;
			}
			frame_num = 0;
		}
		else
		{
			if (frame_num != remote_frame_num)
				Int3();

			object_count++;
			if ((obj_owner == my_pnum) || (obj_owner == -1))
			{
				if (mode != 1)
					Int3(); // SEE ROB
				objnum = remote_objnum;
				//if (objnum > Highest_object_index)
				//{
				//	Highest_object_index = objnum;
				//	num_objects = Highest_object_index+1;
				//}
			}
			else
			{
				if (mode == 1)
				{
					network_pack_objects();
					mode = 0;
				}
				objnum = obj_allocate();
			}
			if (objnum != -1) 
			{
				obj = &Objects[objnum];
				if (obj->segnum != -1)
					obj_unlink(objnum);
				Assert(obj->segnum == -1);
				Assert(objnum < MAX_OBJECTS);
				//memcpy(obj, data + loc, sizeof(object));		loc += sizeof(object);
				netmisc_decode_object(data, &loc, obj);
				segnum = obj->segnum;
				obj->next = obj->prev = obj->segnum = -1;
				obj->attached_obj = -1;
				if (segnum > -1)
					obj_link(obj - Objects, segnum);
				if (obj_owner == my_pnum)
					map_objnum_local_to_local(objnum);
				else if (obj_owner != -1)
					map_objnum_local_to_remote(objnum, remote_objnum, obj_owner);
				else
					object_owner[objnum] = -1;
			}
		} // For a standard onbject
	} // For each object in packet
}

void network_sync_poll(int nitems, newmenu_item* menus, int* key, int citem)
{
	// Polling loop waiting for sync packet to start game


	static fix t1 = 0;

	menus = menus;
	citem = citem;
	nitems = nitems;

	network_listen();

	if (Network_status != NETSTAT_WAITING)	// Status changed to playing, exit the menu
		* key = -2;

	if (!Network_rejoined && (timer_get_approx_seconds() > t1 + F1_0 * 2))
	{
		int i;

		// Poll time expired, re-send request

		t1 = timer_get_approx_seconds();

		mprintf((0, "Re-sending join request.\n"));
		i = network_send_request();
		if (i < 0)
			* key = -2;
	}
}

void network_start_poll(int nitems, newmenu_item* menus, int* key, int citem)
{
	int i, n, nm;

	key = key;
	citem = citem;

	Assert(Network_status == NETSTAT_STARTING);

	if (!menus[0].value) {
		menus[0].value = 1;
		menus[0].redraw = 1;
	}

	for (i = 1; i < nitems; i++) {
		if ((i >= N_players) && (menus[i].value)) {
			menus[i].value = 0;
			menus[i].redraw = 1;
		}
	}

	nm = 0;
	for (i = 0; i < nitems; i++) {
		if (menus[i].value) {
			nm++;
			if (nm > N_players) {
				menus[i].value = 0;
				menus[i].redraw = 1;
			}
		}
	}

	if (nm > MaxNumNetPlayers) {
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "%s %d %s", TXT_SORRY_ONLY, MaxNumNetPlayers, TXT_NETPLAYERS_IN);
		// Turn off the last player highlighted
		for (i = N_players; i > 0; i--)
			if (menus[i].value == 1)
			{
				menus[i].value = 0;
				menus[i].redraw = 1;
				break;
			}
	}

	if (nitems > MAX_PLAYERS) return;

	n = Netgame.numplayers;
	network_listen();

	if (n < Netgame.numplayers)
	{
		sprintf(menus[N_players - 1].text, "%d. %-16s", N_players, Netgame.players[N_players - 1].callsign);
		menus[N_players - 1].redraw = 1;
		if (N_players <= MaxNumNetPlayers)
		{
			menus[N_players - 1].value = 1;
		}
	}
	else if (n > Netgame.numplayers)
	{
		// One got removed...
		for (i = 0; i < N_players; i++)
		{
			sprintf(menus[i].text, "%d. %-16s", i + 1, Netgame.players[i].callsign);
			if (i < MaxNumNetPlayers)
				menus[i].value = 1;
			else
				menus[i].value = 0;
			menus[i].redraw = 1;
		}
		for (i = N_players; i < n; i++)
		{
			sprintf(menus[i].text, "%d. ", i + 1);		// Clear out the deleted entries...
			menus[i].value = 0;
			menus[i].redraw = 1;
		}
	}
}

int opt_cinvul;
int last_cinvul = 0;
int opt_mode;

void network_game_param_poll(int nitems, newmenu_item* menus, int* key, int citem)
{
#ifdef SHAREWARE
	return;
#else
#ifndef ROCKWELL_CODE
	if (menus[opt_mode + 2].value && !menus[opt_mode + 6].value) {
		menus[opt_mode + 6].value = 1;
		menus[opt_mode + 6].redraw = 1;
	}
	if (menus[opt_mode + 4].value) {
		if (!menus[opt_mode + 7].value) {
			menus[opt_mode + 7].value = 1;
			menus[opt_mode + 7].redraw = 1;
		}
	}
#endif
	if (last_cinvul != menus[opt_cinvul].value) {
		sprintf(menus[opt_cinvul].text, "%s: %d %s", TXT_REACTOR_LIFE, menus[opt_cinvul].value * 5, TXT_MINUTES_ABBREV);
		last_cinvul = menus[opt_cinvul].value;
		menus[opt_cinvul].redraw = 1;
	}

#endif
}

int network_get_game_params(char* game_name, int* mode, int* game_flags, int* level)
{
	int i;
	int opt, opt_name, opt_level, opt_closed, opt_difficulty;
	newmenu_item m[16];
	char name[NETGAME_NAME_LEN + 1];
	char slevel[5];
	char level_text[32];
	char srinvul[32];

	char buf[256];

#ifndef SHAREWARE
	int new_mission_num;
	int anarchy_only;

	new_mission_num = multi_choose_mission(&anarchy_only);

	if (new_mission_num < 0)
		return -1;

	strcpy(Netgame.mission_name, Mission_list[new_mission_num].filename);
	strcpy(Netgame.mission_title, Mission_list[new_mission_num].mission_name);
	Netgame.control_invul_time = control_invul_time;
#endif

	snprintf(buf, 255, "%s\nHosting on port %d", TXT_NETGAME_SETUP, NetGetCurrentPort());
	buf[255] = '\0';

	sprintf(name, "%s%s", Players[Player_num].callsign, TXT_S_GAME);
	sprintf(slevel, "1");

	opt = 0;
	m[opt].type = NM_TYPE_TEXT; m[opt].text = TXT_DESCRIPTION; opt++;

	opt_name = opt;
	m[opt].type = NM_TYPE_INPUT; m[opt].text = name; m[opt].text_len = NETGAME_NAME_LEN; opt++;

	sprintf(level_text, "%s (1-%d)", TXT_LEVEL_, Last_level);
	if (Last_secret_level < -1)
		sprintf(level_text + strlen(level_text) - 1, ", S1-S%d)", -Last_secret_level);
	else if (Last_secret_level == -1)
		sprintf(level_text + strlen(level_text) - 1, ", S1)");

	Assert(strlen(level_text) < 32);

	m[opt].type = NM_TYPE_TEXT; m[opt].text = level_text; opt++;

	opt_level = opt;
	m[opt].type = NM_TYPE_INPUT; m[opt].text = slevel; m[opt].text_len = 4; opt++;

#ifdef ROCKWELL_CODE	
	opt_mode = 0;
#else
	opt_mode = opt;
	m[opt].type = NM_TYPE_TEXT; m[opt].text = TXT_MODE; opt++;
	m[opt].type = NM_TYPE_RADIO; m[opt].text = TXT_ANARCHY; m[opt].value = 1; m[opt].group = 0; opt++;
	m[opt].type = NM_TYPE_RADIO; m[opt].text = TXT_TEAM_ANARCHY; m[opt].value = 0; m[opt].group = 0; opt++;
	m[opt].type = NM_TYPE_RADIO; m[opt].text = TXT_ANARCHY_W_ROBOTS; m[opt].value = 0; m[opt].group = 0; opt++;
	m[opt].type = NM_TYPE_RADIO; m[opt].text = TXT_COOPERATIVE; m[opt].value = 0; m[opt].group = 0; opt++;
#endif
	m[opt].type = NM_TYPE_TEXT; m[opt].text = TXT_OPTIONS; opt++;

	opt_closed = opt;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = TXT_CLOSED_GAME; m[opt].value = 0; opt++;
#ifndef SHAREWARE
	//	m[opt].type = NM_TYPE_CHECK; m[opt].text = TXT_SHOW_IDS; m[opt].value=0; opt++;
	m[opt].type = NM_TYPE_CHECK; m[opt].text = TXT_SHOW_ON_MAP; m[opt].value = 0; opt++;
#endif

	opt_difficulty = opt;
	m[opt].type = NM_TYPE_SLIDER; m[opt].value = Player_default_difficulty; m[opt].text = TXT_DIFFICULTY; m[opt].min_value = 0; m[opt].max_value = (NDL - 1); opt++;

	//	m[opt].type = NM_TYPE_TEXT; m[opt].text = "Reactor Invulnerability (mins)"; opt++;
	//	opt_cinvul = opt;
	//	sprintf( srinvul, "%d", control_invul_time );
	//	m[opt].type = NM_TYPE_INPUT; m[opt].text = srinvul; m[opt].text_len=2; opt++;

	opt_cinvul = opt;
	sprintf(srinvul, "%s: %d %s", TXT_REACTOR_LIFE, 5 * control_invul_time, TXT_MINUTES_ABBREV);
	last_cinvul = control_invul_time;
	m[opt].type = NM_TYPE_SLIDER; m[opt].value = control_invul_time; m[opt].text = srinvul; m[opt].min_value = 0; m[opt].max_value = 15; opt++;

	Assert(opt <= 16);

menu:
	i = newmenu_do1(NULL, buf, opt, m, network_game_param_poll, 1);

	if (i > -1) {
		int j;

		for (j = 0; j < num_active_games; j++)
			if (!_stricmp(Active_games[j].game_name, name))
			{
				nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_DUPLICATE_NAME);
				goto menu;
			}

		strcpy(game_name, name);


		if (!_strnicmp(slevel, "s", 1))
			* level = -atoi(slevel + 1);
		else
			*level = atoi(slevel);

		if ((*level < Last_secret_level) || (*level > Last_level) || (*level == 0))
		{
			nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_LEVEL_OUT_RANGE);
			sprintf(slevel, "1");
			goto menu;
		}
#ifdef ROCKWELL_CODE	
		* mode = NETGAME_COOPERATIVE;
#else
		if (m[opt_mode + 1].value)
			* mode = NETGAME_ANARCHY;
		else if (m[opt_mode + 2].value) {
			*mode = NETGAME_TEAM_ANARCHY;
		}
		else if (anarchy_only) {
			nm_messagebox(NULL, 1, TXT_OK, TXT_ANARCHY_ONLY_MISSION);
			m[opt_mode + 2].value = 0;
			m[opt_mode + 3].value = 0;
			m[opt_mode].value = 1;
			goto menu;
		}
		else if (m[opt_mode + 3].value)
			* mode = NETGAME_ROBOT_ANARCHY;
		else if (m[opt_mode + 4].value)
			* mode = NETGAME_COOPERATIVE;
		else Int3(); // Invalid mode -- see Rob
#endif	// ifdef ROCKWELL

		if (m[opt_closed].value)
			* game_flags |= NETGAME_FLAG_CLOSED;
#ifndef SHAREWARE
		//		if (m[opt_closed+1].value)
		//			*game_flags |= NETGAME_FLAG_SHOW_ID;
		if (m[opt_closed + 1].value)
			* game_flags |= NETGAME_FLAG_SHOW_MAP;
#endif

		Difficulty_level = m[opt_difficulty].value;

		//control_invul_time = atoi( srinvul )*60*F1_0;
		control_invul_time = m[opt_cinvul].value;
		Netgame.control_invul_time = control_invul_time * 5 * F1_0 * 60;
	}
	return i;
}

void
network_set_game_mode(int gamemode)
{
	Show_kill_list = 1;

	if (gamemode == NETGAME_ANARCHY)
		Game_mode = GM_NETWORK;
	else if (gamemode == NETGAME_ROBOT_ANARCHY)
		Game_mode = GM_NETWORK | GM_MULTI_ROBOTS;
	else if (gamemode == NETGAME_COOPERATIVE)
		Game_mode = GM_NETWORK | GM_MULTI_COOP | GM_MULTI_ROBOTS;
	else if (gamemode == NETGAME_TEAM_ANARCHY)
	{
		Game_mode = GM_NETWORK | GM_TEAM;
		Show_kill_list = 2;
	}
	else
		Int3();
	if (Game_mode & GM_MULTI_ROBOTS)
		MaxNumNetPlayers = 4;
	else
		MaxNumNetPlayers = 8;
}

int
network_find_game(void)
{
	// Find out whether or not there is space left on this socket

	fix t1;

	Network_status = NETSTAT_BROWSING;

	num_active_games = 0;

	show_boxed_message(TXT_WAIT);

	network_send_game_list_request();
	t1 = timer_get_approx_seconds() + F1_0 * 2;

	plat_present_canvas(0);

	while (timer_get_approx_seconds() < t1) // Wait 3 seconds for replies
	{
		plat_do_events();
		network_listen();
	}

	clear_boxed_message();

	//	mprintf((0, "%s %d %s\n", TXT_FOUND, num_active_games, TXT_ACTIVE_GAMES));

	if (num_active_games < MAX_ACTIVE_NETGAMES)
		return 0;
	return 1;
}

void network_read_sync_packet(netgame_info* sp)
{
	int i, j;
	int connected_to = -1;
	uint8_t origin_address[4];

	char temp_callsign[CALLSIGN_LEN + 1];

	// This function is now called by all people entering the netgame.

	// mprintf( (0, "%s %d\n", TXT_STARTING_NETGAME, sp->levelnum ));

	if (!network_i_am_master()) //Restricted because the master in charge of sync reads their own packet. 
	{
		//Well, origin player still doesn't know their own IP address, so be sure it's a valid transmit address.
		//TODO: Need to clean this up. 
		NetGetLastPacketOrigin(origin_address);
		for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
		{
			if (!memcmp(origin_address, sp->players[i].node, 4))
			{
				connected_to = i;
				break;
			}
		}

		if (connected_to == -1)
		{
			memcpy(sp->players[0].node, origin_address, 4);
			mprintf((0, "bashing player 0 again\n"));
		}
	}

	if (sp != &Netgame)
		memcpy(&Netgame, sp, sizeof(netgame_info));

	N_players = sp->numplayers;
	Difficulty_level = sp->difficulty;
	Network_status = sp->game_status;

	Assert(Function_mode != FMODE_GAME);

	// New code, 11/27

	mprintf((1, "Netgame.checksum = %d, calculated checksum = %d.\n", Netgame.segments_checksum, my_segments_checksum));

	if (Netgame.segments_checksum != my_segments_checksum)
	{
		Network_status = NETSTAT_MENU;
		nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_NETLEVEL_NMATCH);
#ifdef NDEBUG
		return;
#endif
	}

	// Discover my player number

	memcpy(temp_callsign, Players[Player_num].callsign, CALLSIGN_LEN + 1);

	Player_num = -1;

	for (i = 0; i < MAX_NUM_NET_PLAYERS; i++) 
	{
		Players[i].net_kills_total = 0;
		//		Players[i].net_killed_total = 0;
	}

	for (i = 0; i < N_players; i++) {
		if ((!memcmp(&sp->players[i].identifier, &My_Seq.player.identifier, sizeof(My_Seq.player.identifier))) &&
			(!_stricmp(sp->players[i].callsign, temp_callsign)))
		{
			Assert(Player_num == -1); // Make sure we don't find ourselves twice!  Looking for interplay reported bug
			change_playernum_to(i);
		}
		memcpy(Players[i].callsign, sp->players[i].callsign, CALLSIGN_LEN + 1);
		memcpy(Players[i].net_address, sp->players[i].node, 4);

		Players[i].n_packets_got = 0;				// How many packets we got from them
		Players[i].n_packets_sent = 0;				// How many packets we sent to them
		Players[i].connected = sp->players[i].connected;
		Players[i].net_kills_total += sp->player_kills[i];
#ifndef SHAREWARE
		if ((Network_rejoined) || (i != Player_num))
			Players[i].score = sp->player_score[i];
#endif		
		for (j = 0; j < MAX_NUM_NET_PLAYERS; j++)
		{
			kill_matrix[i][j] = sp->kills[i][j];
		}
	}

	if (!network_i_am_master())
	{
		NetGetLastPacketOrigin(origin_address);
		for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
		{
			if (!memcmp(origin_address, Netgame.players[i].node, 4))
			{
				connected_to = i;
				break;
			}
		}

		if (connected_to == -1)
		{
			memcpy(Netgame.players[0].node, origin_address, 4);
			memcpy(Players[0].net_address, origin_address, 4);
			mprintf((0, "bashing player 0 yet again\n"));
		}
	}

	if (Player_num < 0) 
	{
		Network_status = NETSTAT_MENU;
		mprintf((0, "invalid player number %d?\n", Player_num));
		return;
	}

	if (Network_rejoined)
		for (i = 0; i < N_players; i++)
			Players[i].net_killed_total = sp->killed[i];

#ifndef SHAREWARE
	if (Network_rejoined)
	{
		network_process_monitor_vector(sp->monitor_vector);
		Players[Player_num].time_level = sp->level_time;
	}
#endif

	team_kills[0] = sp->team_kills[0];
	team_kills[1] = sp->team_kills[1];

	Players[Player_num].connected = 1;
	Netgame.players[Player_num].connected = 1;

	if (!Network_rejoined)
		for (i = 0; i < MaxNumNetPlayers; i++) 
		{
			Objects[Players[i].objnum].pos = Player_init[Netgame.locations[i]].pos;
			Objects[Players[i].objnum].orient = Player_init[Netgame.locations[i]].orient;
			obj_relink(Players[i].objnum, Player_init[Netgame.locations[i]].segnum);
		}

	Objects[Players[Player_num].objnum].type = OBJ_PLAYER;

	Network_status = NETSTAT_PLAYING;
	Function_mode = FMODE_GAME;
	multi_sort_kill_list();

}

void network_send_sync(void)
{
	int i, j, np;
	uint8_t buf[1024];
	int len = 0;

	// Randomize their starting locations...

	P_SRand(I_GetTicks());
	for (i = 0; i < MaxNumNetPlayers; i++)
	{
		if (Players[i].connected)
			Players[i].connected = 1; // Get rid of endlevel connect statuses
		if (Game_mode & GM_MULTI_COOP)
			Netgame.locations[i] = i;
		else {
			do
			{
				np = P_Rand() % MaxNumNetPlayers;
				for (j = 0; j < i; j++)
				{
					if (Netgame.locations[j] == np)
					{
						np = -1;
						break;
					}
				}
			} while (np < 0);
			// np is a location that is not used anywhere else..
			Netgame.locations[i] = np;
			//			mprintf((0, "Player %d starting in location %d\n" ,i ,np ));
		}
	}

	// Push current data into the sync packet

	network_update_netgame();
	Netgame.game_status = NETSTAT_PLAYING;
	Netgame.type = PID_SYNC;
	Netgame.segments_checksum = my_segments_checksum;

	netmisc_encode_netgameinfo(buf, &len, &Netgame);

	for (i = 0; i < N_players; i++) 
	{
		if ((!Players[i].connected) || (i == Player_num))
			continue;

		// Send several times, extras will be ignored
		NetSendInternetworkPacket(buf, len, Netgame.players[i].node);
		NetSendInternetworkPacket(buf, len, Netgame.players[i].node);
		NetSendInternetworkPacket(buf, len, Netgame.players[i].node);
	}
	network_read_sync_packet(&Netgame); // Read it myself, as if I had sent it
}

int
network_select_teams(void)
{
#ifndef SHAREWARE
	newmenu_item m[MAX_PLAYERS + 4];
	int choice, opt, opt_team_b;
	uint8_t team_vector = 0;
	char team_names[2][CALLSIGN_LEN + 1];
	int i;
	int pnums[MAX_PLAYERS + 2];

	// One-time initialization

	for (i = N_players / 2; i < N_players; i++) // Put first half of players on team A
	{
		team_vector |= (1 << i);
	}

	sprintf(team_names[0], "%s", TXT_BLUE);
	sprintf(team_names[1], "%s", TXT_RED);

	// Here comes da menu
menu:
	m[0].type = NM_TYPE_INPUT; m[0].text = team_names[0]; m[0].text_len = CALLSIGN_LEN;

	opt = 1;
	for (i = 0; i < N_players; i++)
	{
		if (!(team_vector & (1 << i)))
		{
			m[opt].type = NM_TYPE_MENU; m[opt].text = Netgame.players[i].callsign; pnums[opt] = i; opt++;
		}
	}
	opt_team_b = opt;
	m[opt].type = NM_TYPE_INPUT; m[opt].text = team_names[1]; m[opt].text_len = CALLSIGN_LEN; opt++;
	for (i = 0; i < N_players; i++)
	{
		if (team_vector & (1 << i))
		{
			m[opt].type = NM_TYPE_MENU; m[opt].text = Netgame.players[i].callsign; pnums[opt] = i; opt++;
		}
	}
	m[opt].type = NM_TYPE_TEXT; m[opt].text = ""; opt++;
	m[opt].type = NM_TYPE_MENU; m[opt].text = TXT_ACCEPT; opt++;

	Assert(opt <= MAX_PLAYERS + 4);

	choice = newmenu_do(NULL, TXT_TEAM_SELECTION, opt, m, NULL);

	if (choice == opt - 1)
	{
		if ((opt - 2 - opt_team_b < 2) || (opt_team_b == 1))
		{
			nm_messagebox(NULL, 1, TXT_OK, TXT_TEAM_MUST_ONE);
			goto menu;
		}

		Netgame.team_vector = team_vector;
		strcpy(Netgame.team_name[0], team_names[0]);
		strcpy(Netgame.team_name[1], team_names[1]);
		return 1;
	}

	else if ((choice > 0) && (choice < opt_team_b)) {
		team_vector |= (1 << pnums[choice]);
	}
	else if ((choice > opt_team_b) && (choice < opt - 2)) {
		team_vector &= ~(1 << pnums[choice]);
	}
	else if (choice == -1)
		return 0;
	goto menu;
#else
	return 0;
#endif
}

int
network_select_players(void)
{
	int i, j;
	newmenu_item m[MAX_PLAYERS];
	char text[MAX_PLAYERS][25];
	char title[50];
	int save_nplayers;

	network_add_player(&My_Seq);

	for (i = 0; i < MAX_PLAYERS; i++) {
		sprintf(text[i], "%d.  %-16s", i + 1, "");
		m[i].type = NM_TYPE_CHECK; m[i].text = text[i]; m[i].value = 0;
	}

	m[0].value = 1;				// Assume server will play...

	sprintf(text[0], "%d. %-16s", 1, Players[Player_num].callsign);
	sprintf(title, "%s %d %s", TXT_TEAM_SELECT, MaxNumNetPlayers, TXT_TEAM_PRESS_ENTER);

GetPlayersAgain:
	j = newmenu_do1(NULL, title, MAX_PLAYERS, m, network_start_poll, 1);

	save_nplayers = N_players;

	if (j < 0)
	{
		// Aborted!					 
		// Dump all players and go back to menu mode

	abort:
		for (i = 1; i < N_players; i++)
			network_dump_player(Netgame.players[i].node, DUMP_ABORTED);

		Netgame.numplayers = 0;
		network_send_game_info(0); // Tell everyone we're bailing

		Network_status = NETSTAT_MENU;
		return(0);
	}

	// Count number of players chosen

	N_players = 0;
	for (i = 0; i < save_nplayers; i++)
	{
		if (m[i].value)
			N_players++;
	}

	if (N_players > MaxNumNetPlayers) {
		nm_messagebox(TXT_ERROR, 1, TXT_OK, "%s %d %s", TXT_SORRY_ONLY, MaxNumNetPlayers, TXT_NETPLAYERS_IN);
		N_players = save_nplayers;
		goto GetPlayersAgain;
	}

#ifdef NDEBUG
	if (N_players < 2) {
		nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_TEAM_ATLEAST_TWO);
		N_players = save_nplayers;
		goto GetPlayersAgain;
	}
#endif

#ifdef NDEBUG
	if ((Netgame.gamemode == NETGAME_TEAM_ANARCHY) && (N_players < 3)) {
		nm_messagebox(TXT_ERROR, 1, TXT_OK, TXT_TEAM_ATLEAST_THREE);
		N_players = save_nplayers;
		goto GetPlayersAgain;
	}
#endif

	// Remove players that aren't marked.
	N_players = 0;
	for (i = 0; i < save_nplayers; i++) 
	{
		if (m[i].value)
		{
			if (i > N_players)
			{
				memcpy(Netgame.players[N_players].callsign, Netgame.players[i].callsign, CALLSIGN_LEN + 1);
				memcpy(Netgame.players[N_players].node, Netgame.players[i].node, 4);
				memcpy(&Netgame.players[N_players].identifier, &Netgame.players[i].identifier, sizeof(Netgame.players[i].identifier));
			}
			Players[N_players].connected = 1;
			N_players++;
		}
		else
		{
			network_dump_player(Netgame.players[i].node, DUMP_DORK);
		}
	}

	for (i = N_players; i < MAX_NUM_NET_PLAYERS; i++) 
	{
		memset(Netgame.players[i].callsign, 0, CALLSIGN_LEN + 1);
		memset(Netgame.players[i].node, 0, 4);
		memset(&Netgame.players[i].identifier, 0, sizeof(Netgame.players[i].identifier));
	}

	if (Netgame.gamemode == NETGAME_TEAM_ANARCHY)
		if (!network_select_teams())
			goto abort;

	return(1);
}

void
network_start_game(void)
{
	int i;
	char game_name[NETGAME_NAME_LEN + 1];
	int chosen_game_mode, game_flags, level;

	Assert(sizeof(frame_info) < IPX_MAX_DATA_SIZE);

	mprintf((0, "Using frame_info len %d, max %d.\n", sizeof(frame_info), IPX_MAX_DATA_SIZE));

	if (!Network_active)
	{
		nm_messagebox(NULL, 1, TXT_OK, TXT_IPX_NOT_FOUND);
		return;
	}

	network_init();
	change_playernum_to(0);

	if (network_find_game())
	{
		nm_messagebox(NULL, 1, TXT_OK, TXT_NET_FULL);
		return;
	}

	game_flags = 0;
	i = network_get_game_params(game_name, &chosen_game_mode, &game_flags, &level);

	if (i < 0) return;

	N_players = 0;

	// LoadLevel(level); Old, no longer used.

	Netgame.difficulty = Difficulty_level;
	Netgame.gamemode = chosen_game_mode;
	Netgame.game_status = NETSTAT_STARTING;
	Netgame.numplayers = 0;
	Netgame.max_numplayers = MaxNumNetPlayers;
	Netgame.levelnum = level;
	Netgame.game_flags = game_flags;
	Netgame.protocol_version = MULTI_PROTO_VERSION;

	strcpy(Netgame.game_name, game_name);

	Network_status = NETSTAT_STARTING;

	network_set_game_mode(Netgame.gamemode);

	if (network_select_players())
	{
		StartNewLevel(Netgame.levelnum);
	}
	else
		Game_mode = GM_GAME_OVER;
}

void restart_net_searching(newmenu_item* m)
{
	int i;
	N_players = 0;
	num_active_games = 0;

	memset(Active_games, 0, sizeof(netgame_info) * MAX_ACTIVE_NETGAMES);

	for (i = 0; i < MAX_ACTIVE_NETGAMES; i++) {
		sprintf(m[(2 * i) + 1].text, "%d.                                       ", i + 1);
		sprintf(m[(2 * i) + 2].text, " \n");
		m[(2 * i) + 1].redraw = 1;
		m[(2 * i) + 2].redraw = 1;
	}
	Network_games_changed = 1;
}

void network_join_poll(int nitems, newmenu_item* menus, int* key, int citem)
{
	// Polling loop for Join Game menu
	static fix t1 = 0;
	int i, osocket;

	menus = menus;
	citem = citem;
	nitems = nitems;
	key = key;

	if (Network_allow_socket_changes) {
		osocket = Network_socket;

		if (*key == KEY_PAGEUP) { Network_socket--; *key = 0; }
		if (*key == KEY_PAGEDOWN) { Network_socket++; *key = 0; }

		if (Network_socket + IPX_DEFAULT_SOCKET > 0x8000)
			Network_socket = 0x8000 - IPX_DEFAULT_SOCKET;

		if (Network_socket + IPX_DEFAULT_SOCKET < 0)
			Network_socket = IPX_DEFAULT_SOCKET;

		if (Network_socket != osocket) {
			sprintf(menus[0].text, "%s %+d", TXT_CURRENT_IPX_SOCKET, Network_socket);
			menus[0].redraw = 1;
			mprintf((0, "Changing to socket %d\n", Network_socket));
			network_listen();
			NetChangeDefaultSocket(IPX_DEFAULT_SOCKET + Network_socket);
			restart_net_searching(menus);
			network_send_game_list_request();
			return;
		}
	}

	if (timer_get_approx_seconds() > t1 + F1_0 * 4)
	{
		t1 = timer_get_approx_seconds();
		network_send_game_list_request();
	}

	network_listen();

	if (!Network_games_changed)
		return;

	Network_games_changed = 0;

	// Copy the active games data into the menu options
	for (i = 0; i < num_active_games; i++)
	{
		int game_status = Active_games[i].game_status;
		int j, nplayers = 0;
		char levelname[4];

		for (j = 0; j < Active_games[i].numplayers; j++)
			if (Active_games[i].players[j].connected)
				nplayers++;

		if (Active_games[i].levelnum < 0)
			sprintf(levelname, "S%d", -Active_games[i].levelnum);
		else
			sprintf(levelname, "%d", Active_games[i].levelnum);

		sprintf(menus[(2 * i) + 1].text, "%d. %s (%s)", i + 1, Active_games[i].game_name, MODE_NAMES(Active_games[i].gamemode));

		if (game_status == NETSTAT_STARTING)
		{
			sprintf(menus[(2 * i) + 2].text, "%s%s  %s%d\n", TXT_NET_FORMING, levelname, TXT_NET_PLAYERS, nplayers);
		}
		else if (game_status == NETSTAT_PLAYING)
		{
			if (can_join_netgame(&Active_games[i]))
				sprintf(menus[(2 * i) + 2].text, "%s%s  %s%d\n", TXT_NET_JOIN, levelname, TXT_NET_PLAYERS, nplayers);
			else
				sprintf(menus[(2 * i) + 2].text, "%s\n", TXT_NET_CLOSED);
		}
		else
			sprintf(menus[(2 * i) + 2].text, "%s\n", TXT_NET_BETWEEN);

		if (strlen(Active_games[i].mission_name) > 0)
			sprintf(menus[(2 * i) + 2].text + strlen(menus[(2 * i) + 2].text), "%s%s", TXT_MISSION, Active_games[i].mission_title);

		Assert(strlen(menus[(2 * i) + 2].text) < 70);
		menus[(2 * i) + 1].redraw = 1;
		menus[(2 * i) + 2].redraw = 1;
	}

	for (i = num_active_games; i < MAX_ACTIVE_NETGAMES; i++)
	{
		sprintf(menus[(2 * i) + 1].text, "%d.                                       ", i + 1);
		sprintf(menus[(2 * i) + 2].text, " \n");
		menus[(2 * i) + 1].redraw = 1;
		menus[(2 * i) + 2].redraw = 1;
	}
}

int network_wait_for_sync()
{
	char text[60];
	newmenu_item m[2];
	int i, choice;

	Network_status = NETSTAT_WAITING;

	m[0].type = NM_TYPE_TEXT; m[0].text = text;
	m[1].type = NM_TYPE_TEXT; m[1].text = TXT_NET_LEAVE;

	i = network_send_request();

	if (i < 0)
		return(-1);

	sprintf(m[0].text, "%s\n'%s' %s", TXT_NET_WAITING, Netgame.players[i].callsign, TXT_NET_TO_ENTER);

menu:
	choice = newmenu_do(NULL, TXT_WAIT, 2, m, network_sync_poll);

	if (choice > -1)
		goto menu;

	if (Network_status != NETSTAT_PLAYING)
	{
		sequence_packet me;
		uint8_t buf[128];
		int len = 0;

		//		if (Network_status == NETSTAT_ENDLEVEL)
		//		{
		//		 	network_send_endlevel_packet(0);
		//			longjmp(LeaveGame, 0);
		//		}		

		mprintf((0, "Aborting join.\n"));
		me.type = PID_QUIT_JOINING;
		memcpy(me.player.callsign, Players[Player_num].callsign, CALLSIGN_LEN + 1);
		memcpy(me.player.node, NetGetLocalAddress(), 4);
		memcpy(&me.player.identifier, &My_Seq.player.identifier, sizeof(My_Seq.player.identifier));

		netmisc_encode_sequence_packet(buf, &len, &me);
		NetSendInternetworkPacket(buf, len, Netgame.players[0].node);
		N_players = 0;
		Function_mode = FMODE_MENU;
		Game_mode = GM_GAME_OVER;
		return(-1);	// they cancelled		
	}
	return(0);
}

void
network_request_poll(int nitems, newmenu_item* menus, int* key, int citem)
{
	// Polling loop for waiting-for-requests menu

	int i = 0;
	int num_ready = 0;

	menus = menus;
	citem = citem;
	nitems = nitems;
	key = key;

	// Send our endlevel packet at regular intervals

//	if (timer_get_approx_seconds() > t1+ENDLEVEL_SEND_INTERVAL)
//	{
//		network_send_endlevel_packet();
//		t1 = timer_get_approx_seconds();
//	}

	network_listen();

	for (i = 0; i < N_players; i++)
	{
		if ((Players[i].connected == 1) || (Players[i].connected == 0))
			num_ready++;
	}

	if (num_ready == N_players) // All players have checked in or are disconnected
	{
		*key = -2;
	}
}

void
network_wait_for_requests(void)
{
	// Wait for other players to load the level before we send the sync
	int choice, i;
	newmenu_item m[1];

	Network_status = NETSTAT_WAITING;

	m[0].type = NM_TYPE_TEXT; m[0].text = TXT_NET_LEAVE;

	mprintf((0, "Entered wait_for_requests : N_players = %d.\n", N_players));

	for (choice = 0; choice < N_players; choice++)
		mprintf((0, "Players[%d].connected = %d.\n", choice, Players[choice].connected));

	Network_status = NETSTAT_WAITING;
	network_flush();

	Players[Player_num].connected = 1;

menu:
	choice = newmenu_do(NULL, TXT_WAIT, 1, m, network_request_poll);

	if (choice == -1)
	{
		// User aborted
		choice = nm_messagebox(NULL, 3, TXT_YES, TXT_NO, TXT_START_NOWAIT, TXT_QUITTING_NOW);
		if (choice == 2)
			return;
		if (choice != 0)
			goto menu;

		// User confirmed abort

		for (i = 0; i < N_players; i++)
			if ((Players[i].connected != 0) && (i != Player_num))
				network_dump_player(Netgame.players[i].node, DUMP_ABORTED);

		longjmp(LeaveGame, 0);
	}
	else if (choice != -2)
		goto menu;
}

int
network_level_sync(void)
{
	// Do required syncing between (before) levels

	int result;

	mprintf((0, "Player %d entering network_level_sync.\n", Player_num));

	MySyncPackInitialized = 0;

	//	my_segments_checksum = netmisc_calc_checksum(Segments, sizeof(segment)*(Highest_segment_index+1));

	network_flush(); // Flush any old packets

	if (N_players == 0)
		result = network_wait_for_sync();
	else if (network_i_am_master())
	{
		network_wait_for_requests();
		network_send_sync();
		result = 0;
	}
	else
		result = network_wait_for_sync();

	if (result)
	{
		Players[Player_num].connected = 0;
		network_send_endlevel_packet();
		longjmp(LeaveGame, 0);
	}
	return(0);
}

void network_join_game()
{
	int choice, i;
	char menu_text[(MAX_ACTIVE_NETGAMES * 2) + 1][70];

	newmenu_item m[((MAX_ACTIVE_NETGAMES) * 2) + 1];

	if (!Network_active)
	{
		nm_messagebox(NULL, 1, TXT_OK, TXT_IPX_NOT_FOUND);
		return;
	}

	network_init();

	N_players = 0;

	setjmp(LeaveGame);

	Network_status = NETSTAT_BROWSING; // We are looking at a game menu

	network_listen();  // Throw out old info

	network_send_game_list_request(); // broadcast a request for lists

	num_active_games = 0;

	memset(m, 0, sizeof(newmenu_item) * (MAX_ACTIVE_NETGAMES * 2));
	memset(Active_games, 0, sizeof(netgame_info) * MAX_ACTIVE_NETGAMES);

	m[0].text = menu_text[0];
	m[0].type = NM_TYPE_TEXT;
	if (Network_allow_socket_changes)
		sprintf(m[0].text, "Current IPX Socket is default%+d", Network_socket);
	else
		sprintf(m[0].text, "");

	for (i = 0; i < MAX_ACTIVE_NETGAMES; i++) {
		m[2 * i + 1].text = menu_text[2 * i + 1];
		m[2 * i + 2].text = menu_text[2 * i + 2];
		m[2 * i + 1].type = NM_TYPE_MENU;
		m[2 * i + 2].type = NM_TYPE_TEXT;
		sprintf(m[(2 * i) + 1].text, "%d.                                       ", i + 1);
		sprintf(m[(2 * i) + 2].text, " \n");
		m[(2 * i) + 1].redraw = 1;
		m[(2 * i) + 2].redraw = 1;
	}

	Network_games_changed = 1;
remenu:
	choice = newmenu_do1(NULL, TXT_NET_SEARCHING, (MAX_ACTIVE_NETGAMES) * 2 + 1, m, network_join_poll, 0);

	if (choice == -1) {
		Network_status = NETSTAT_MENU;
		return;	// they cancelled		
	}
	choice--;
	choice /= 2;

	if (choice >= num_active_games)
	{
		nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_INVALID_CHOICE);
		goto remenu;
	}

	// Choice has been made and looks legit
	if (Active_games[choice].game_status == NETSTAT_ENDLEVEL)
	{
		nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_NET_GAME_BETWEEN2);
		goto remenu;
	}

	if (Active_games[choice].protocol_version != MULTI_PROTO_VERSION)
	{
		nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_VERSION_MISMATCH);
		goto remenu;
	}

#ifndef SHAREWARE
	{
		// Check for valid mission name
		mprintf((0, "Loading mission:%s.\n", Active_games[choice].mission_name));
		if (!load_mission_by_name(Active_games[choice].mission_name))
		{
			nm_messagebox(NULL, 1, TXT_OK, TXT_MISSION_NOT_FOUND);
			goto remenu;
		}
	}
#endif

	if (!can_join_netgame(&Active_games[choice]))
	{
		if (Active_games[choice].numplayers == Active_games[choice].max_numplayers)
			nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_GAME_FULL);
		else
			nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_IN_PROGRESS);
		goto remenu;
	}

	// Choice is valid, prepare to join in

	memcpy(&Netgame, &Active_games[choice], sizeof(netgame_info));
	Difficulty_level = Netgame.difficulty;
	MaxNumNetPlayers = Netgame.max_numplayers;
	change_playernum_to(1);

	network_set_game_mode(Netgame.gamemode);

	StartNewLevel(Netgame.levelnum);

	return;		// look ma, we're in a game!!!
}

void network_join_game_at(uint8_t* address)
{
	int i;
	char menu_text[(MAX_ACTIVE_NETGAMES * 2) + 1][70];
	fix start_time;

	newmenu_item m[((MAX_ACTIVE_NETGAMES) * 2) + 1];

	if (!Network_active)
	{
		nm_messagebox(NULL, 1, TXT_OK, TXT_IPX_NOT_FOUND);
		return;
	}

	network_init();

	N_players = 0;

	Network_status = NETSTAT_BROWSING; // We are looking at a game menu

	network_listen();  // Throw out old info

	num_active_games = 0;

	memset(m, 0, sizeof(newmenu_item) * (MAX_ACTIVE_NETGAMES * 2));
	memset(Active_games, 0, sizeof(netgame_info) * MAX_ACTIVE_NETGAMES);

	network_send_game_info_request_to(address);
	start_time = timer_get_fixed_seconds();

	show_boxed_message(TXT_WAIT);
	plat_present_canvas(0);

	if (setjmp(LeaveGame)) //aborting game
		return;

	Network_games_changed = 1;

	for(;;)
	{
		plat_do_events();
		//listen for requests
		network_listen();

		//Look if a game has been made available yet
		if (num_active_games > 0)
			break;

		if (timer_get_fixed_seconds() > start_time + F1_0 * 5)
		{
			nm_messagebox(TXT_SORRY, 1, TXT_OK, "No game found within 5 seconds.");
			return;
		}
	}

	// Choice has been made and looks legit
	if (Active_games[0].game_status == NETSTAT_ENDLEVEL)
	{
		nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_NET_GAME_BETWEEN2);
		return;
		//goto remenu;
	}

	if (Active_games[0].protocol_version != MULTI_PROTO_VERSION)
	{
		nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_VERSION_MISMATCH);
		return;
		//goto remenu;
	}

#ifndef SHAREWARE
	{
		// Check for valid mission name
		mprintf((0, "Loading mission:%s.\n", Active_games[0].mission_name));
		if (!load_mission_by_name(Active_games[0].mission_name))
		{
			nm_messagebox(NULL, 1, TXT_OK, TXT_MISSION_NOT_FOUND);
			return;
			//goto remenu;
		}
	}
#endif

	if (!can_join_netgame(&Active_games[0]))
	{
		if (Active_games[0].numplayers == Active_games[0].max_numplayers)
			nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_GAME_FULL);
		else
			nm_messagebox(TXT_SORRY, 1, TXT_OK, TXT_IN_PROGRESS);
		return;
	}

	// Choice is valid, prepare to join in

	memcpy(&Netgame, &Active_games[0], sizeof(netgame_info));
	Difficulty_level = Netgame.difficulty;
	MaxNumNetPlayers = Netgame.max_numplayers;
	change_playernum_to(1);

	network_set_game_mode(Netgame.gamemode);

	StartNewLevel(Netgame.levelnum);

	return;		// look ma, we're in a game!!!
}

void network_leave_game()
{
	network_do_frame(1, 1);

	if ((network_i_am_master()) && (Network_status == NETSTAT_STARTING))
	{
		Netgame.numplayers = 0;
		network_send_game_info(0);
	}

	Players[Player_num].connected = 0;
	network_send_endlevel_packet();
	change_playernum_to(0);
	Game_mode = GM_GAME_OVER;
	network_flush();
}

void network_flush()
{
	uint8_t packet[IPX_MAX_DATA_SIZE];

	if (!Network_active)
		return;

	while (NetGetPacketData(packet) > 0)
		;
}

void network_listen()
{
	int size;
	uint8_t packet[IPX_MAX_DATA_SIZE];

	if (!Network_active) return;

	if (!(Game_mode & GM_NETWORK) && (Function_mode == FMODE_GAME))
		mprintf((0, "Calling network_listen() when not in net game.\n"));

	size = NetGetPacketData(packet);
	while (size > 0) {
		network_process_packet(packet, size);
		size = NetGetPacketData(packet);
	}
}

void network_send_data(uint8_t* ptr, int len, int urgent)
{
	char check;

	if (Endlevel_sequence)
		return;

	if (!MySyncPackInitialized) {
		MySyncPackInitialized = 1;
		memset(&MySyncPack, 0, sizeof(frame_info));
	}

	if (urgent)
		PacketUrgent = 1;

	if ((MySyncPack.data_size + len) > NET_XDATA_SIZE) {
		check = ptr[0];
		network_do_frame(1, 0);
		if (MySyncPack.data_size != 0) {
			mprintf((0, "%d bytes were added to data by network_do_frame!\n", MySyncPack.data_size));
			Int3();
		}
		//		Int3();		// Trying to send too much!
		//		return;
		mprintf((0, "Packet overflow, sending additional packet, type %d len %d.\n", ptr[0], len));
		Assert(check == ptr[0]);
	}

	Assert(MySyncPack.data_size + len <= NET_XDATA_SIZE);

	memcpy(&MySyncPack.data[MySyncPack.data_size], ptr, len);
	MySyncPack.data_size += len;
}

void network_timeout_player(int playernum)
{
	// Remove a player from the game if we haven't heard from them in 
	// a long time.
	int i, n = 0;

	Assert(playernum < N_players);
	Assert(playernum > -1);

	network_disconnect_player(playernum);
	create_player_appearance_effect(&Objects[Players[playernum].objnum]);

	digi_play_sample(SOUND_HUD_MESSAGE, F1_0);

	HUD_init_message("%s %s", Players[playernum].callsign, TXT_DISCONNECTING);
	for (i = 0; i < N_players; i++)
		if (Players[i].connected)
			n++;

	if (n == 1)
	{
		nm_messagebox(NULL, 1, TXT_OK, TXT_YOU_ARE_ONLY);
	}
}

fix last_send_time = 0;
fix last_timeout_check = 0;

void network_do_frame(int force, int listen)
{
	int i;
	uint8_t buf[1024];
	int len = 0;

	if (!(Game_mode & GM_NETWORK)) return;

	if ((Network_status != NETSTAT_PLAYING) || (Endlevel_sequence)) // Don't send postion during escape sequence...
		goto listen;

	last_send_time += FrameTime;
	last_timeout_check += FrameTime;

	// Send out packet 10 times per second maximum... unless they fire, then send more often...
	if ((last_send_time > F1_0 / 10) || (Network_laser_fired) || force || PacketUrgent) {
		if (Players[Player_num].connected) {
			int objnum = Players[Player_num].objnum;
			PacketUrgent = 0;

			if (listen) {
				multi_send_robot_frame(0);
				multi_send_fire();		// Do firing if needed..
			}

			//			mprintf((0, "Send packet, %f secs, %d bytes.\n", f2fl(last_send_time), MySyncPack.data_size));

			last_send_time = 0;

			MySyncPack.type = PID_PDATA;
			MySyncPack.playernum = Player_num;
#ifdef SHAREWARE
			MySyncPack.objnum = Players[Player_num].objnum;
#endif
			MySyncPack.obj_segnum = Objects[objnum].segnum;
			MySyncPack.obj_pos = Objects[objnum].pos;
			MySyncPack.obj_orient = Objects[objnum].orient;
#ifdef SHAREWARE
			MySyncPack.obj_phys_info = Objects[objnum].mtype.phys_info;
#else
			MySyncPack.phys_velocity = Objects[objnum].mtype.phys_info.velocity;
			MySyncPack.phys_rotvel = Objects[objnum].mtype.phys_info.rotvel;
#endif
			MySyncPack.obj_render_type = Objects[objnum].render_type;
			MySyncPack.level_num = Current_level_num;

			for (i = 0; i < N_players; i++)
			{
				if ((Players[i].connected) && (i != Player_num))
				{
					MySyncPack.numpackets = Players[i].n_packets_sent++;
					len = 0;
					netmisc_encode_frame_info(buf, &len, &MySyncPack);
					//					mprintf((0, "sync pack is %d bytes long.\n", sizeof(frame_info)));
					NetSendPacket(buf, len - NET_XDATA_SIZE + MySyncPack.data_size, Netgame.players[i].node, Players[i].net_address);
					//NetSendPacket((uint8_t*)& MySyncPack, sizeof(frame_info) - NET_XDATA_SIZE + MySyncPack.data_size, Netgame.players[i].node, Players[i].net_address);
				}
			}
			MySyncPack.data_size = 0;		// Start data over at 0 length.
			if (Fuelcen_control_center_destroyed)
				network_send_endlevel_packet();
			//mprintf( (0, "Packet has %d bytes appended (TS=%d)\n", MySyncPack.data_size, sizeof(frame_info)-NET_XDATA_SIZE+MySyncPack.data_size ));
		}
	}

	if (!listen)
		return;

	if ((last_timeout_check > F1_0) && !(Fuelcen_control_center_destroyed))
	{
		fix approx_time = timer_get_approx_seconds();
		// Check for player timeouts
		for (i = 0; i < N_players; i++)
		{
			if ((i != Player_num) && (Players[i].connected == 1))
			{
				if ((LastPacketTime[i] == 0) || (LastPacketTime[i] > approx_time))
				{
					LastPacketTime[i] = approx_time;
					continue;
				}
				if ((approx_time - LastPacketTime[i]) > (5 * F1_0))
					network_timeout_player(i);
			}
		}
		last_timeout_check = 0;
	}

listen:
	if (!listen)
	{
		MySyncPack.data_size = 0;
		return;
	}
	network_listen();

	if (Network_send_objects)
		network_send_objects();
}

int missed_packets = 0;

void network_consistency_error(void)
{
	static int count = 0;

	if (count++ < 10)
		return;

	Function_mode = FMODE_MENU;
	nm_messagebox(NULL, 1, TXT_OK, TXT_CONSISTENCY_ERROR);
	Function_mode = FMODE_GAME;
	count = 0;
	multi_quit_game = 1;
	multi_leave_menu = 1;
	multi_reset_stuff();
}

void network_read_pdata_packet(frame_info* pd)
{
	int TheirPlayernum = pd->playernum;
#ifdef SHAREWARE
	int TheirObjnum = pd->objnum;
#else
	int TheirObjnum = Players[pd->playernum].objnum;
#endif
	object* TheirObj = NULL;

	if (TheirPlayernum < 0)
	{
		Int3(); // This packet is bogus!!
		return;
	}
	if (!multi_quit_game && (TheirPlayernum >= N_players)) 
	{
		Int3(); // We missed an important packet!
		network_consistency_error();
		return;
	}
	if (Endlevel_sequence || (Network_status == NETSTAT_ENDLEVEL)) 
	{
		int old_Endlevel_sequence = Endlevel_sequence;
		Endlevel_sequence = 1;
		if (pd->data_size > 0) {
			// pass pd->data to some parser function....
			multi_process_bigdata(pd->data, pd->data_size);
		}
		Endlevel_sequence = old_Endlevel_sequence;
		return;
	}
	//	mprintf((0, "Gametime = %d, Frametime = %d.\n", GameTime, FrameTime));

	if ((int8_t)pd->level_num != Current_level_num)
	{
		mprintf((0, "Got frame packet from player %d wrong level %d!\n", pd->playernum, pd->level_num));
		return;
	}

	TheirObj = &Objects[TheirObjnum];

	//------------- Keep track of missed packets -----------------
	Players[TheirPlayernum].n_packets_got++;
	LastPacketTime[TheirPlayernum] = timer_get_approx_seconds();

	if (pd->numpackets != Players[TheirPlayernum].n_packets_got) {
		missed_packets += pd->numpackets - Players[TheirPlayernum].n_packets_got;

		if (missed_packets > 0)
			mprintf((0, "Missed %d packets from player #%d (%d total)\n", pd->numpackets - Players[TheirPlayernum].n_packets_got, TheirPlayernum, missed_packets));
		else
			mprintf((0, "Got %d late packets from player #%d (%d total)\n", Players[TheirPlayernum].n_packets_got - pd->numpackets, TheirPlayernum, missed_packets));

		Players[TheirPlayernum].n_packets_got = pd->numpackets;
	}

	//------------ Read the player's ship's object info ----------------------
	TheirObj->pos = pd->obj_pos;
	TheirObj->orient = pd->obj_orient;
#ifdef SHAREWARE
	TheirObj->mtype.phys_info = pd->obj_mtype.phys_info;
#else
	TheirObj->mtype.phys_info.velocity = pd->phys_velocity;
	TheirObj->mtype.phys_info.rotvel = pd->phys_rotvel;
#endif

	if ((TheirObj->render_type != pd->obj_render_type) && (pd->obj_render_type == RT_POLYOBJ))
		multi_make_ghost_player(TheirPlayernum);

	obj_relink(TheirObjnum, pd->obj_segnum);

	if (TheirObj->movement_type == MT_PHYSICS)
		set_thrust_from_velocity(TheirObj);

	//------------ Welcome them back if reconnecting --------------
	if (!Players[TheirPlayernum].connected) 
	{
		Players[TheirPlayernum].connected = 1;

#ifndef SHAREWARE
		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_multi_reconnect(TheirPlayernum);
#endif

		multi_make_ghost_player(TheirPlayernum);

		create_player_appearance_effect(&Objects[TheirObjnum]);

		digi_play_sample(SOUND_HUD_MESSAGE, F1_0);
		HUD_init_message("'%s' %s", Players[TheirPlayernum].callsign, TXT_REJOIN);

#ifndef SHAREWARE
		multi_send_score();
#endif
	}

	//------------ Parse the extra data at the end ---------------

	if (pd->data_size > 0) 
	{
		// pass pd->data to some parser function....
		multi_process_bigdata(pd->data, pd->data_size);
	}
	//	mprintf( (0, "Got packet with %d bytes on it!\n", pd->data_size ));

}

#endif
