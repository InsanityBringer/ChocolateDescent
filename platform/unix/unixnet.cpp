/*
The code contained in this file is not the property of Parallax Software,
and is not under the terms of the Parallax Software Source license.
Instead, it is released under the terms of the MIT License,
as described in copying.txt.
*/

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include <ifaddrs.h>
#include <net/if.h>

#include "misc/types.h"
#include "misc/byteswap.h"
#include "platform/i_net.h"
#include "platform/mono.h"

uint8_t currentAddress[] = { 0, 0, 0, 0, 0, 0 };
uint8_t serverAddress[] = { 0, 0, 0, 0 };

int netSocket = -1;
sockaddr_in localAddressIn;
sockaddr localAddress;

uint16_t port;

int NetInit(int socket_number, int show_address)
{
	port = socket_number;
	NetChangeRole(0);

	return 0; //We're good
}

static void NetFindLocalAddress(uint8_t *addr)
{
	struct ifaddrs *addrs;

	getifaddrs(&addrs);
	for (struct ifaddrs *cur = addrs; cur; cur = cur->ifa_next)
	{
		if (!(cur->ifa_flags & IFF_UP) || !(cur->ifa_flags & IFF_RUNNING))
			continue;
		if (cur->ifa_flags & IFF_LOOPBACK)
			continue;
		if (cur->ifa_addr && cur->ifa_addr->sa_family == AF_INET)
		{
			memcpy(addr, &((struct sockaddr_in *)cur->ifa_addr)->sin_addr, sizeof(in_addr_t));
			break;
		}
	}

	freeifaddrs(addrs);
}

int NetChangeRole(dbool host)
{
	int err;
	sockaddr_in hack;
	sockaddr self;
	socklen_t selfSize = sizeof(self);

	if (netSocket != -1)
	{
		close(netSocket);
	}

	netSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (netSocket == -1)
	{
		err = errno;
		mprintf((1, "Kernel failed to open insocket with code %d.\n", err));
		return -3;
	}

	//Enable non-blocking mode
	fcntl(netSocket, F_SETFL, fcntl(netSocket, F_GETFL, 0) | O_NONBLOCK);

	//Bind the port
	localAddressIn.sin_family = AF_INET;
	localAddressIn.sin_addr.s_addr = INADDR_ANY;
	localAddressIn.sin_port = htons(port); //Pick a unused port. 

	if (bind(netSocket, (sockaddr*)&localAddressIn, sizeof(localAddressIn)))
	{
		err = errno;
		mprintf((1, "Kernel failed to bind socket with code %d.\n", err));
		close(netSocket);
		netSocket = -1;
		return -3;
	}

	socklen_t hack2 = sizeof(localAddress);
	if (getsockname(netSocket, &localAddress, &hack2))
	{
		err = errno;
		mprintf((1, "Kernel failed to get socket name with code %d.\n", err));
		close(netSocket);
		netSocket = -1;
		return -3;
	}

	//Enable broadcasts
	int broadcast = 1;

	if (setsockopt(netSocket, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast, sizeof(broadcast)))
	{
		err = errno;
		mprintf((1, "Kernel failed to enable sendsocket broadcasts with code %d.\n", err));
		close(netSocket);
		netSocket = -1;
		return -3;
	}

	//Make sure the socket's data length is sufficient.
	unsigned int maxDataLength;
	socklen_t maxDataLengthSize = sizeof(maxDataLength);

	if (getsockopt(netSocket, SOL_SOCKET, SO_SNDBUF, (char*)&maxDataLength, &maxDataLengthSize))
	{
		err = errno;
		mprintf((1, "Kernel failed to read max message size with code %d.\n", err));
		close(netSocket);
		netSocket = -1;
		return -3;
	}

	if (maxDataLength < IPX_MAX_DATA_SIZE)
	{
		mprintf((1, "Kernel packet size is too small. Really? I'm confused.\n"));
		close(netSocket);
		netSocket = -1;
		return -3;
	}
	mprintf((0, "Size of packet is %d\n", maxDataLength));

	NetFindLocalAddress(currentAddress);

	if (getsockname(netSocket, &self, &selfSize))
	{
		mprintf((1, "Can't get socket information\n"));
		close(netSocket);
		netSocket = -1;
		return -3;
	}

	serverAddress[0] = self.sa_data[1];
	serverAddress[1] = self.sa_data[0];
	mprintf((0, "Role change, new port is %d.\n", port));

	return 0;
}

int NetChangeDefaultSocket(uint16_t socket_number)
{
	port = socket_number;
	NetChangeRole(0);
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

uint16_t NetGetCurrentPort()
{
	return port;
}

void NetGetLocalTarget(uint8_t* server, uint8_t* node, uint8_t* local_target)
{
	//no concept of a local target, so eh.
	//TODO: This needs to be refactored into an interface that makes more sense for UDP.
	memcpy(local_target, node, 4);
}

uint8_t packetBuffer[IPX_MAX_DATA_SIZE];
sockaddr_in lastAddr;
socklen_t addrSize = sizeof(lastAddr);

int NetGetPacketData(uint8_t* data)
{
	int err;
	int size;

	size = recvfrom(netSocket, (char*)packetBuffer, IPX_MAX_DATA_SIZE, 0, (sockaddr*)&lastAddr, &addrSize);
	if (size == -1)
	{
		err = errno;
		if (err != EWOULDBLOCK)
			mprintf((0, "Failed to recieve packet, returned error code %d\n", err));
		return 0;
	}

	//optimize? one memcpy every packet sucks, but need to put data in buffer
	memcpy(data, packetBuffer, size);

	return size;
}

void NetGetLastPacketOrigin(uint8_t* addrBuf)
{
	*((in_addr_t*)addrBuf) = lastAddr.sin_addr.s_addr;
}

void NetSendBroadcastPacket(uint8_t* data, int datasize)
{
	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_BROADCAST;
	if (sendto(netSocket, (const char*)data, datasize, 0, (sockaddr*)&addr, sizeof(addr)) == -1)
	{
		mprintf((0, "Failed to broadcast packet, returned error code %d\n", errno));
	}
	printf("sending broadcast\n");
}

//The server, address, and immediate address system isn't a great fit for this UDP nonsense, but what can I do...
//TODO: Refactor Net API, I'll probably have to for internet connections. 
void NetSendPacket(uint8_t* data, int datasize, uint8_t* address, uint8_t* immediate_address)
{
	NetSendInternetworkPacket(data, datasize, immediate_address);
}

void NetSendInternetworkPacket(uint8_t* data, int datasize, uint8_t* address)
{
	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = *((in_addr_t*)address);
	if (sendto(netSocket, (const char*)data, datasize, 0, (sockaddr*)&addr, sizeof(addr)) == -1)
	{
		mprintf((0, "Failed to send packet, returned error code %d\n", errno));
	}
	//printf("sending packet over port %d to %d.%d.%d.%d\n", BS_MakeShort(server), address[0], address[1], address[2], address[3]);
}

void ipx_read_user_file(char* filename)
{
}

void ipx_read_network_file(char* filename)
{
}
