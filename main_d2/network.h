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

#ifdef NETWORK

#ifndef _NETWORK_H
#define _NETWORK_H

#include "gameseq.h"
#include "multi.h"

// The default socket to use.

#ifdef SHAREWARE
#define IPX_DEFAULT_SOCKET 0x5110		
#else
#define IPX_DEFAULT_SOCKET 0x5130		
#endif

#define NETSTAT_MENU					0
#define NETSTAT_PLAYING				1
#define NETSTAT_BROWSING			2
#define NETSTAT_WAITING				3
#define NETSTAT_STARTING			4
#define NETSTAT_ENDLEVEL			5

#define CONNECT_DISCONNECTED		0
#define CONNECT_PLAYING				1
#define CONNECT_WAITING				2
#define CONNECT_DIED_IN_MINE		3
#define CONNECT_FOUND_SECRET		4
#define CONNECT_ESCAPE_TUNNEL		5
#define CONNECT_END_MENU			6

// defines and other things for appletalk/ipx games on mac

typedef struct sequence_packet
{
	uint8_t					type;
	int 					Security;
	netplayer_info		player;
} sequence_packet;

#define NET_XDATA_SIZE 454


// frame info is aligned -- 01/18/96 -- MWA
// if you change this structure -- be sure to keep
// alignment:
//		bytes on byte boundries
//		shorts on even byte boundries
//		ints on even byte boundries

typedef struct frame_info
{
	uint8_t				type;						// What type of packet
	uint8_t				pad[3];					// Pad out length of frame_info packet
	int				numpackets;
	vms_vector		obj_pos;
	vms_matrix		obj_orient;
	vms_vector		phys_velocity;
	vms_vector		phys_rotvel;
	short				obj_segnum;
	uint16_t			data_size;		// Size of data appended to the net packet
	uint8_t				playernum;
	uint8_t				obj_render_type;
	uint8_t				level_num;
	uint8_t				data[NET_XDATA_SIZE];		// extra data to be tacked on the end
} frame_info;

// short_frame_info is not aligned -- 01/18/96 -- MWA
// won't align because of shortpos.  Shortpos needs
// to stay in current form.

typedef struct short_frame_info
{
	uint8_t				type;						// What type of packet
	uint8_t				pad[3];					// Pad out length of frame_info packet
	int				numpackets;
	shortpos			thepos;
	uint16_t			data_size;		// Size of data appended to the net packet
	uint8_t				playernum;
	uint8_t				obj_render_type;
	uint8_t				level_num;
	uint8_t				data[NET_XDATA_SIZE];		// extra data to be tacked on the end
} short_frame_info;

typedef struct endlevel_info
{
	uint8_t                                   type;
	uint8_t                                   player_num;
	int8_t                                    connected;
	uint8_t                                   seconds_left;
	short                                   kill_matrix[MAX_PLAYERS][MAX_PLAYERS];
	short                                   kills;
	short                                   killed;
} endlevel_info;

void network_start_game();
void network_join_game();
void network_rejoin_game();
void network_leave_game();
int network_endlevel(int* secret);
void network_endlevel_poll2(int nitems, struct newmenu_item* menus, int* key, int citem);


int network_level_sync();
void network_send_endlevel_packet();

int network_delete_extra_objects();
int network_find_max_net_players();
int network_objnum_is_past(int objnum);
char* network_get_player_name(int objnum);
void network_send_naked_packet(char* buf, short len, int who);

int HoardEquipped();

void network_disconnect_player(int playernum);

extern int NetGameType;
extern int Network_send_objects;
extern int Network_send_objnum;
extern int PacketUrgent;
extern int Network_rejoined;

extern int Network_new_game;
extern int Network_status;

extern uint16_t Current_Port;

extern fix LastPacketTime[MAX_PLAYERS];

extern uint16_t my_segments_checksum;
// By putting an up-to-20-char-message into Network_message and 
// setting Network_message_reciever to the player num you want to
// send it to (100 for broadcast) the next frame the player will
// get your message.

// Call once at the beginning of a frame
void network_do_frame(int force, int listen);

// Tacks data of length 'len' onto the end of the next
// packet that we're transmitting.
void network_send_data(uint8_t* ptr, int len, int urgent);

//Connects to a game at a specific address with the current port.  
void network_join_game_at(uint8_t* address);

#endif
#endif
