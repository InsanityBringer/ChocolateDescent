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

#pragma once

//---------------------------------------------------------------
// Initializes all IPX internals. 
// If socket_number==0, then opens next available socket.
// Returns:	0  if successful.
//				-1 if socket already open.
//				-2	if socket table full.
//				-3 if IPX not installed.
//				-4 if couldn't allocate low dos memory
//				-5 if error with getting internetwork address
extern int NetInit(int socket_number, int show_address);

extern int NetChangeDefaultSocket(uint16_t socket_number);

//Changes role. Something broadcasting must be host, while something
//trying to recieve must not be a host. This is a mess, but it should work.
extern int NetChangeRole(dbool host);

// Returns a pointer to 6-byte address
extern uint8_t* NetGetLocalAddress();
// Returns a pointer to 4-byte server
extern uint8_t* NetGetServerAddress();

// Determines the local address equivalent of an internetwork address.
void NetGetLocalTarget(uint8_t* server, uint8_t* node, uint8_t* local_target);

// If any packets waiting to be read in, this fills data in with the packet data and returns
// the number of bytes read.  Else returns 0 if no packets waiting.
extern int NetGetPacketData(uint8_t* data);

//After reading a packet from NetGetPacketData, this can be used to get the origin address from it.
//Call this instead of letting the protocol specify return addresses, because that system won't
//get you on the internet. 
extern void NetGetLastPacketOrigin(uint8_t* addrBuf);

// Sends a broadcast packet to everyone on this socket.
extern void NetSendBroadcastPacket(uint8_t* data, int datasize);

// Sends a packet to a certain address
extern void NetSendPacket(uint8_t* data, int datasize, uint8_t* port, uint8_t* address, uint8_t* immediate_address);
extern void NetSendInternetworkPacket(uint8_t* data, int datasize, uint8_t* port, uint8_t* address);

//[ISB] changed to fit aligned size of network information structure. God, this is going to be an adenture...
#define IPX_MAX_DATA_SIZE (546)

extern void ipx_read_user_file(char* filename);
extern void ipx_read_network_file(char* filename);
