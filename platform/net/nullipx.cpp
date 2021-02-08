#include "misc/types.h"
#include "platform/net/nullipx.h"

uint8_t null_address[] = { 0, 0, 0, 0, 0, 0 };

int ipx_init(int socket_number, int show_address)
{
	return -3; //IPX not present
}

int ipx_change_default_socket(uint16_t socket_number)
{
	return 0;
}

uint8_t* ipx_get_my_local_address()
{
	return &null_address[0];
}

uint8_t* ipx_get_my_server_address()
{
	return &null_address[0];
}

void ipx_get_local_target(uint8_t* server, uint8_t* node, uint8_t* local_target)
{
}

int ipx_get_packet_data(uint8_t* data)
{
	return 0;
}

void ipx_send_broadcast_packet_data(uint8_t* data, int datasize)
{
}

void ipx_send_packet_data(uint8_t* data, int datasize, uint8_t* network, uint8_t* address, uint8_t* immediate_address)
{
}

void ipx_send_internetwork_packet_data(uint8_t* data, int datasize, uint8_t* server, uint8_t* address)
{
}

void ipx_read_user_file(char* filename)
{
}

void ipx_read_network_file(char* filename)
{
}
