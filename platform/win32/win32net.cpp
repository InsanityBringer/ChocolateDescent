/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

#include "misc/types.h"
#include "misc/byteswap.h"
#include "platform/i_net.h"
#include "platform/mono.h"

uint8_t currentAddress[] = { 0, 0, 0, 0, 0, 0 };
uint8_t serverAddress[] = { 0, 0, 0, 0 };

SOCKET netSocket = INVALID_SOCKET;
WSADATA socketData;
CSADDR_INFO localAddressInfo;
sockaddr_in localAddressIn;
sockaddr localAddress;
hostent* localHost;

uint16_t port;

int NetInit(int socket_number, int show_address)
{
	int err = WSAStartup(MAKEWORD(2, 0), &socketData);
	if (err)
	{
		mprintf((1, "Winsock startup failed with code %d.\n", err));
		return -3;
	}

	port = socket_number;
	NetChangeRole(0); //start as a client

	return 0; //We're good
}

int NetChangeRole(dbool host)
{
	int err;
	sockaddr_in hack;
	sockaddr self;
	int selfSize = sizeof(self);

	if (netSocket != INVALID_SOCKET)
	{
		closesocket(netSocket);
	}

	netSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (netSocket == INVALID_SOCKET)
	{
		err = WSAGetLastError();
		mprintf((1, "WinSock failed to open insocket with code %d.\n", err));
		return -3;
	}

	//Enable broadcasts
	u_long ugh = 1;

	ioctlsocket(netSocket, FIONBIO, &ugh);

	localHost = gethostbyname("");

	//Bind the port
	localAddressIn.sin_family = AF_INET;
	//localAddressIn.sin_addr.s_addr = inet_addr(inet_ntoa(*(struct in_addr*)*localHost->h_addr_list));
	localAddressIn.sin_addr.s_addr = INADDR_ANY;
	if (host)
		localAddressIn.sin_port = htons(port);
	else
		localAddressIn.sin_port = 0; //Pick a unused port. 

	if (bind(netSocket, (SOCKADDR*)&localAddressIn, sizeof(localAddressIn)))
	{
		err = WSAGetLastError();
		mprintf((1, "WinSock failed to bind socket with code %d.\n", err));
		closesocket(netSocket);
		netSocket = INVALID_SOCKET;
		return -3;
	}

	int hack2 = sizeof(localAddress);
	if (getsockname(netSocket, &localAddress, &hack2))
	{
		err = WSAGetLastError();
		mprintf((1, "WinSock failed to get socket name with code %d.\n", err));
		closesocket(netSocket);
		netSocket = INVALID_SOCKET;
		return -3;
	}

	//Enable broadcasts
	BOOL broadcast = 1;

	if (setsockopt(netSocket, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast, sizeof(broadcast)))
	{
		err = WSAGetLastError();
		mprintf((1, "WinSock failed to enable sendsocket broadcasts with code %d.\n", err));
		closesocket(netSocket);
		netSocket = INVALID_SOCKET;
		return -3;
	}

	//Make sure the socket's data length is sufficient.
	unsigned int maxDataLength;
	int maxDataLengthSize = sizeof(maxDataLength);

	if (getsockopt(netSocket, SOL_SOCKET, SO_MAX_MSG_SIZE, (char*)&maxDataLength, &maxDataLengthSize))
	{
		err = WSAGetLastError();
		mprintf((1, "WinSock failed to read max message size with code %d.\n", err));
		closesocket(netSocket);
		netSocket = INVALID_SOCKET;
		return -3;
	}

	if (maxDataLength < IPX_MAX_DATA_SIZE)
	{
		mprintf((1, "WinSock packet size is too small. Really? I'm confused.\n"));
		closesocket(netSocket);
		netSocket = INVALID_SOCKET;
		return -3;
	}
	mprintf((0, "Size of packet is %d\n", maxDataLength));

	//TODO: Not sure if this is an implementation detail or if it's safe to do this
	memcpy(currentAddress, *localHost->h_addr_list, 4);

	if (getsockname(netSocket, &self, &selfSize))
	{
		mprintf((1, "Can't get socket information\n"));
		closesocket(netSocket);
		netSocket = INVALID_SOCKET;
		return -3;
	}

	serverAddress[0] = self.sa_data[1];
	serverAddress[1] = self.sa_data[0];
	mprintf((0, "Role change, new port is %d.\n", BS_MakeShort(serverAddress)));

	return 0;
}

int NetChangeDefaultSocket(uint16_t socket_number)
{
	return 0;
}

uint8_t* NetGetLocalAddress()
{
	mprintf((0, "Getting IP address, it is %d.%d.%d.%d\n", currentAddress[0], currentAddress[1], currentAddress[2], currentAddress[3]));
	return &currentAddress[0];
}

uint8_t* NetGetServerAddress()
{
	return &serverAddress[0];
}

void NetGetLocalTarget(uint8_t* server, uint8_t* node, uint8_t* local_target)
{
	//no concept of a local target, so eh.
	memcpy(local_target, node, 4);
}

uint8_t packetBuffer[IPX_MAX_DATA_SIZE];
int NetGetPacketData(uint8_t* data)
{
	int err;
	int size;
	sockaddr_in addr = {};
	int addrSize = sizeof(addr);

	size = recvfrom(netSocket, (char*)packetBuffer, IPX_MAX_DATA_SIZE, 0, (sockaddr*)&addr, &addrSize);
	if (size == -1)
	{
		err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
			mprintf((0, "Failed to recieve packet, returned error code %d\n", err));
		return 0;
	}
	
	printf("there's data yo\n");
	//optimize? one memcpy every packet sucks, but need to put data in buffer
	memcpy(data, packetBuffer, size);

	return size;
}

void NetSendBroadcastPacket(uint8_t* data, int datasize)
{
	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_BROADCAST;
	if (sendto(netSocket, (const char*)data, datasize, 0, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		mprintf((0, "Failed to broadcast packet, returned error code %d\n", WSAGetLastError()));
	}
	printf("sending broadcast\n");
}

//The server, address, and immediate address system isn't a great fit for this UDP nonsense, but what can I do...
//TODO: Refactor Net API, I'll probably have to for internet connections. 
void NetSendPacket(uint8_t* data, int datasize, uint8_t* network, uint8_t* address, uint8_t* immediate_address)
{
	NetSendInternetworkPacket(data, datasize, network, address);
}

void NetSendInternetworkPacket(uint8_t* data, int datasize, uint8_t* server, uint8_t* address)
{
	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(BS_MakeShort(server));
	addr.sin_addr.s_addr = *((long*)address);
	if (sendto(netSocket, (const char*)data, datasize, 0, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		mprintf((0, "Failed to send packet, returned error code %d\n", WSAGetLastError()));
	}
	printf("sending packet over port %d to %d.%d.%d.%d\n", BS_MakeShort(server), address[0], address[1], address[2], address[3]);
}

void ipx_read_user_file(char* filename)
{
}

void ipx_read_network_file(char* filename)
{
}
