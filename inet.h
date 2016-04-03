#ifndef MC_INET_H
#define MC_INET_H

#include "main.h"

int mc_server_connect (mc_connection *mc);
int mc_client_connect (mc_connection *mc);

void mc_disconnect (mc_connection *mc);

int mc_bot_startup (const char *name, const char *ip, in_port_t port, const char *mppass);
int mc_proxy_startup (const char *name, const char *ip, in_port_t port, const char *mppass);

void * mc_server_run_listener (void *arg);
void * mc_client_run_listener (void *arg);


#endif