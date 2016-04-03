#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <curl/curl.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <zlib.h>
#include <math.h>

#include "command.h"
#include "inet.h"
#include "proto.h"

#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))
#define SWAP(x,y) ((x)^=(y),(y)^=(x),(x)^=(y))

#define MAX_PLAYERS		(257)

#define MC_ERROR		0x01
#define MC_SEND			0x02
#define MC_CONTINUE		0x04


#define MC_CLIENT		0x001
#define MC_TUNNEL		0x002

#define MC_LOCK_MAP		0x004
#define MC_LOCK_AREA		0x008
#define MC_LOCK_PLAYER		0x010
#define MC_FOLLOW_PLAYER	0x020	
#define MC_HIDE_POSITION	0x040
#define MC_WAITING_FOR_1	0x080
#define MC_WAITING_FOR_2	0x100
#define MC_USE_TYPE		0x200


#ifndef MC_PLAYER_S
#define MC_PLAYER_S
struct mc_player {
	int16_t x;
	int16_t y;
	int16_t z;
	int32_t pid;
	int16_t rotx;
	int16_t roty;
	char name[32];
};
#endif

#ifndef MC_DATA_S
#define MC_DATA_S
struct mc_server_data {
	struct mc_player mc_player_db[MAX_PLAYERS + 1];
	
	uint32_t mc_level_x;
	uint32_t mc_level_y;
	uint32_t mc_level_z;
	
	unsigned char *mc_unparsed_data;
	uint32_t mc_current_size;
	
	unsigned char *mc_level_data;
	uint32_t mc_level_size;
	
	int mc_pid_locked;
	
	int x1, y1, z1, x2, y2, z2, type;
};
#endif

#ifndef MC_CONNECTION_S
#define MC_CONNECTION_S
struct mc_server_connection {
	int mc_server_sock;
	int mc_client_sock;
	
	const char *mc_server_address;
	in_port_t mc_server_port;
	const char *mc_server_password;
	
	const char *mc_client_name;
	in_port_t mc_client_port;
	
	pthread_t mc_server_thread;
	pthread_t mc_client_thread;
	pthread_t mc_cmd_thread;
	
	struct mc_server_data mc_server_data;
	
	int mc_mode;
};

typedef struct mc_server_connection mc_connection;
#endif