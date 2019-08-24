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

#pragma once

#ifdef NETWORK

#include "gameseq.h"
#include "multi.h"

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

typedef struct sequence_packet {
	uint8_t					type;
	netplayer_info		player;
} sequence_packet;

#ifdef SHAREWARE
#define NET_XDATA_SIZE 256
#else
#define NET_XDATA_SIZE 454
#endif

#ifdef SHAREWARE
typedef struct frame_info {
	uint8_t				type;						// What type of p
	int				numpackets;
	short				objnum;
	uint8_t				playernum;
	short				obj_segnum;
	vms_vector		obj_pos;
	vms_matrix		obj_orient;
	physics_info	obj_phys_info;
	uint8_t				obj_render_type;
	uint8_t				level_num;
	uint16_t			data_size;		// Size of data appended to the net packet
	uint8_t				data[NET_XDATA_SIZE];		// extra data to be tacked on the end
} frame_info;
#else
typedef struct frame_info {
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
#endif



void network_start_game();
void network_join_game();
void network_rejoin_game();
void network_leave_game();
int network_endlevel(int* secret);
int network_endlevel_poll2(int nitems, struct newmenu_item* menus, int* key, int citem);


int network_level_sync();
void network_send_endlevel_packet();

int network_delete_extra_objects();
int network_find_max_net_players();
int network_objnum_is_past(int objnum);
char* network_get_player_name(int objnum);

void network_disconnect_player(int playernum);


extern int Network_send_objects;
extern int Network_send_objnum;
extern int PacketUrgent;
extern int Network_rejoined;

extern int Network_new_game;
extern int Network_status;

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

#endif
