#ifndef MC_PROTO_H
#define MC_PROTO_H

#include "main.h"

ssize_t mc_server_authenticate (mc_connection *mc, const char *name);

ssize_t mc_proto_send_chat (mc_connection *mc, const char *msg);
ssize_t mc_proto_send_pos (mc_connection *mc, short x, short y, short z, 
			   char rotx, char roty);
ssize_t mc_proto_send_block (mc_connection *mc, short x, short y, short z, 
			     char mode, char type);

int packlen (char code);

int mc_server_parse_packet (mc_connection *mc, char *buffer);
int mc_client_parse_packet (mc_connection *mc, char *buffer);

#endif