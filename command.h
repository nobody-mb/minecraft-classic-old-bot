#ifndef MC_COMMAND_H
#define MC_COMMAND_H

#include "main.h"

#define CHECK_ORDER(x1,x2) do {if (x1 > x2) { SWAP(x1, x2); }} while (0)
#define CALC_POS(cur,max) ((MIN(cur+4,max)))

#define MC_ALL			-1
#define MC_NO_REPLACE		0

#define GUARD_MAP	'm'
#define GUARD_PLAYER	'p'
#define GUARD_AREA	'a'

int cuboid (mc_connection *mc, int type, int x1, int y1, int z1, 
	    int x2, int y2, int z2);

int offset(mc_connection *mc, int x, int y, int z);
double dist3d (int x, int y, int z, int xp, int yp, int zp);

void restore_block (mc_connection *mc, int t, int x, int y, int z);
void restore_and_move (mc_connection *mc, int t, int x, int y, int z);

void follow_move (mc_connection *mc, int pid);

int get_pid_by_name (struct mc_server_data data, const char *name);

void *mc_command_run_listener (void *arg);

int mc_client_command_parser (mc_connection *mc, char *buf);

#endif