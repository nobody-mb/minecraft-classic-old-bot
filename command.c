#include "command.h"

int offset (mc_connection *mc, int x, int y, int z) {
	return y * (mc->mc_server_data.mc_level_x*mc->mc_server_data.mc_level_z) + 
		z * mc->mc_server_data.mc_level_z + x;
}

struct cuboid_data {
	int x1, y1, z1;
	int x2, y2, z2;
	
	int build_distance;
	int type;
};

double dist3d (int x, int y, int z, int xp, int yp, int zp)
{
	int dx = abs(x - xp);
	int dy = abs(y - yp);
	int dz = abs(z - zp);
	
	return sqrt((dx*dx) + (dy*dy) + (dz*dz));
}

#define CAN_SET(t)	((t >= 8 && t <= 11) || t == 0)
#define CAN_DELETE(t)	((t < 7 || t > 11) && t != 0)


void restore_block (mc_connection *mc, int t, int x, int y, int z)
{
/*	int old = mc->mc_server_data.mc_level_data[offset(mc, x, y, z)];
	
	if (old == 2)
		old = 3;
	if (old == 43)
		old = 44;
	    
	if (CAN_SET(new) && old != 0) {
		printf("restoring %d (replaced by %d)\n", old, new);
		mc_proto_send_block(mc, x, y, z, 1, old);
	}
	
	if (CAN_DELETE(new) && old == 0)  {
		printf("deleting %d\n", new);
		mc_proto_send_block(mc, x, y, z, 0, 1);
	}
	
	mc->mc_server_data.mc_level_data[offset(mc, x, y, z)] = old;*/
	int type = mc->mc_server_data.mc_level_data[offset(mc, x, y, z)];
	if (t) {
		if (t != 7 && t != 8 && t != 9 && !(t == 3 && type == 2)) {
			if (type != 0) {
				if (type == 2) {
					type = 3;
				} else if (type == 43) {
					type = 44;
				} 
				printf("restoring replaced block\n");
				mc_proto_send_block(mc, x, y, z, 1, type);
				t = type;
			} else {
				printf("deleting\n");
				mc_proto_send_block(mc, x, y, z, 0, 1);
				t = 0;
			}
		}
	} else {
		if (type == 2) {
			type = 3;
		} else if (type == 43) {
			type = 44;
			mc_proto_send_block(mc, x, y + 1, z, 1, 44);
		} 
		if (type != 0) {
			printf("restoring %d\n", type);
			mc_proto_send_block(mc, x, y, z, 1, type);
			t = type;
		}
	}
	
	mc->mc_server_data.mc_level_data[offset(mc, x, y, z)] = t;

}

void restore_and_move (mc_connection *mc, int t, int x, int y, int z)
{
	int xp = mc->mc_server_data.mc_player_db[256].x;
	int yp = mc->mc_server_data.mc_player_db[256].y;
	int zp = mc->mc_server_data.mc_player_db[256].z;
	
	if (dist3d(x, y, z, xp, yp, zp) < 6) {
		restore_block(mc, t, x, y, z);
	} else {
		mc_proto_send_pos(mc, x, y, z, 130, 130);
		mc_proto_send_pos(mc, x, y, z, 130, 130);
		restore_block(mc, t, x, y, z);
	}
}  

int get_block_at_coords (mc_connection *mc, int x, int y, int z)
{
	return mc->mc_server_data.mc_level_data[offset(mc, x, y, z)];
}

int cuboid_check_square (mc_connection *mc, int type, 
			 int bx, int by, int bz, int x2, int y2, int z2)
{
	for (int x = bx; x <= (MIN(bx+8, x2));x++) {
		for (int y = by; y <= (MIN(by+8, y2));y++) {
			for (int z = bz; z <= (MIN(bz+8, z2));z++) {
				int block = get_block_at_coords(mc, x, y, z);
				if (block != type)
					return 1;
			}
		}
	}
	
	return 0;
}



int cuboid (mc_connection *mc, int type, int x1, int y1, int z1, int x2, int y2, int z2) 
{
	int xp, yp, zp;
	CHECK_ORDER(x1, x2);
	CHECK_ORDER(y1, y2);
	CHECK_ORDER(z1, z2);
	
	printf("%d %d %d %d %d %d | %d\n",x1,y1,z1,x2,y2,z2,type);

	for (int bx = x1; bx <= x2+1; bx+=9) {
		for (int by = y1; by <= y2+1; by+=9) {
			for (int bz = z1; bz <= z2+1; bz+=9) {
				if (!cuboid_check_square(mc, type, bx, by, bz, x2, y2, z2))
					continue;
				xp = CALC_POS(bx, x2);
				yp = CALC_POS(by, y2);
				zp = CALC_POS(bz, z2);
				mc_proto_send_pos(mc, xp, yp, zp, 130, 130);
				mc_proto_send_pos(mc, xp, yp, zp, 130, 130);
				for (int x = bx; x <= (MIN(bx+8, x2));x++) {
					for (int y = by; y <= (MIN(by+8, y2));y++) {
						for (int z = bz; z <= (MIN(bz+8, z2));z++) {
							int block = get_block_at_coords(mc, x, y, z);
							if (type == 0 && block != 0) {
								mc_proto_send_block(mc,x,y,z,0,1);
								usleep(1000);
							} else if (type != 0 && block != 0) {
								mc_proto_send_block(mc,x,y,z,0,1);
								mc_proto_send_block(mc,x,y,z,1,type);
								usleep(1000);
							} else if (type != 0 && block == 0) {
								mc_proto_send_block(mc,x,y,z,1,type);
								usleep(1000);
							}
						}
					}
				}
				usleep(10000);
			}
		}
	}
	 // la 17 33 121 47 24 126
	return 0;
}

int get_pid_by_name (struct mc_server_data data, const char *name)
{
	int i;
	for (i = 0; i < MAX_PLAYERS; i++) {
		if (!strcasecmp(data.mc_player_db[i].name, name)) {
			return i;
		}
	}
	return -1;
}

const char *get_name_by_pid (struct mc_server_data *data, int pid)
{
	return data->mc_player_db[pid].name;
}


int guard (mc_connection *mc, char *buffer) 
{
	int mode, pid;
	struct mc_server_data *data;
	
	if (strlen(buffer) < 2)
		return -1;
	
	mode = buffer[1];
	data = &mc->mc_server_data;
	
	if (mode == GUARD_MAP) {
		if (!(mc->mc_mode & MC_LOCK_MAP)) {
			mc->mc_mode |= MC_LOCK_MAP;
			printf("Locked map\n");
			if (mc->mc_mode & MC_LOCK_AREA) {
				mc->mc_mode &= ~MC_LOCK_AREA;
				printf("Unlocked area\n");
			}
			if (mc->mc_mode & (MC_LOCK_PLAYER|MC_FOLLOW_PLAYER)) {
				mc->mc_mode &= ~(MC_LOCK_PLAYER|MC_FOLLOW_PLAYER);
				printf("Unlocked player %s\n", 
				       get_name_by_pid(data, data->mc_pid_locked));
			}
		} else {
			mc->mc_mode &= ~MC_LOCK_MAP;
			printf("Unlocked map\n");
		}
	} else if (mode == GUARD_PLAYER) {
		if (!(mc->mc_mode & (MC_LOCK_PLAYER|MC_FOLLOW_PLAYER))) {
			if (strlen(buffer) < 4)
				return -1;
			pid = get_pid_by_name(mc->mc_server_data, buffer + 3);
			mc->mc_server_data.mc_pid_locked = pid;
			if (pid < 0)
				return -1;
			printf("Locked player %s\n", buffer + 3);
			mc->mc_mode |= (MC_LOCK_PLAYER|MC_FOLLOW_PLAYER);
			if (mc->mc_mode & MC_LOCK_AREA) {
				mc->mc_mode &= ~MC_LOCK_AREA;
				printf("Unlocked area\n");
			}
			if (mc->mc_mode & MC_LOCK_MAP) {
				mc->mc_mode &= ~MC_LOCK_MAP;
				printf("Unlocked map\n");
			}
		} else {
			mc->mc_mode &= ~(MC_LOCK_PLAYER|MC_FOLLOW_PLAYER);
			printf("Unlocked player %s\n", 
			        get_name_by_pid(data, data->mc_pid_locked));
		}
	} else if (mode == GUARD_AREA) {
		if (!(mc->mc_mode & MC_LOCK_AREA)) {
			if (sscanf(&buffer[3], "%d %d %d %d %d %d", 
				   &data->x1, &data->y1, &data->z1, 
				   &data->x2, &data->y2, &data->z2) != 6) {
				printf("Invalid coordinates\n");
				return -1;
			}
			CHECK_ORDER(data->x1, data->x2);
			CHECK_ORDER(data->y1, data->y2);
			CHECK_ORDER(data->z1, data->z2);
			
			printf("Locked area (%d, %d, %d) -> (%d, %d, %d)\n", 
			       data->x1, data->y1, data->z1, 
			       data->x2, data->y2, data->z2);
			
			mc->mc_mode |= MC_LOCK_AREA;
			if (mc->mc_mode & MC_LOCK_MAP) {
				mc->mc_mode &= ~MC_LOCK_MAP;
				printf("Unlocked map\n");
			}
			if (mc->mc_mode & (MC_LOCK_PLAYER|MC_FOLLOW_PLAYER)) {
				mc->mc_mode &= ~(MC_LOCK_PLAYER|MC_FOLLOW_PLAYER);
				printf("Unlocked player %s\n", 
				        get_name_by_pid(data, data->mc_pid_locked));

			}
		} else {
			mc->mc_mode &= ~MC_LOCK_AREA;
			printf("Unlocked area\n");
		}
	} else {
		return -1;
	}
	return 0;
}

void follow_move (mc_connection *mc, int pid)
{
	int xp = mc->mc_server_data.mc_player_db[pid].x;
	int yp = mc->mc_server_data.mc_player_db[pid].y;
	int zp = mc->mc_server_data.mc_player_db[pid].z;
	int rxp = mc->mc_server_data.mc_player_db[pid].rotx;
	int ryp = mc->mc_server_data.mc_player_db[pid].roty;
	
	if (xp == 1024 && yp == 1024 && zp == 1024) {
		printf("lost connection to player\n");
		mc_proto_send_pos(mc, 64, 1001, 64, 0, 0);
		return;
	} 
	
	printf("moved to %d %d %d | %d %d\n", xp, yp, zp, rxp, ryp);
	
	mc_proto_send_pos(mc, xp, yp, zp, rxp, ryp);
	

}

void *mc_command_run_listener (void *arg)
{
	mc_connection *mc = (mc_connection *)arg;
	char buffer[1024];
	int x1, y1, z1, x2, y2, z2, type, pid;
	
	while (1) {
		fgets(buffer, sizeof(buffer), stdin);
		buffer[strlen(buffer) - 1] = '\0';
		switch (buffer[0]) {
			case 'k':
				pid = get_pid_by_name(mc->mc_server_data, buffer + 2);
				if (pid >= 0) {
					x1 = mc->mc_server_data.mc_player_db[pid].x;
					y1 = mc->mc_server_data.mc_player_db[pid].y;
					z1 = mc->mc_server_data.mc_player_db[pid].z;
					printf("Player %s at %d, %d, %d\n", buffer + 2, x1, y1, z1);
				} else {
					printf("Could not find player %s\n", buffer + 2);
				}
				break;
			case 's':
				mc_proto_send_chat(mc, &buffer[2]);
				break;
			case 'l':
				if (guard(mc, buffer) < 0)
					printf("Invalid arguments\n");
				break;
			case 'c':
				if (sscanf(&buffer[2], "%d %d %d %d %d %d %d", 
					   &type, &x1, &y1, &z1, &x2, &y2, &z2) != 7) {
					printf("Invalid coordinates\n");
					break;
				}
				cuboid(mc, type, x1, y1, z1, x2, y2, z2);
				break;
			case 'z':;
				int times;
				if (sscanf(&buffer[2], "%d %d %d %d", 
					   &times, &x1, &y1, &z1) != 4) {
					printf("Invalid coordinates\n");
					break;
				}
				int i;
				for (i = 0; i < times; i++) {
					mc_proto_send_pos(mc, x1, y1, z1, arc4random() % 256, arc4random() % 256);
					usleep(10000);
				}
				break;
			case 'f':
				if (!(mc->mc_mode & MC_FOLLOW_PLAYER)) {
					pid = get_pid_by_name(mc->mc_server_data, buffer + 2);
					if (pid >= 0) {
						mc->mc_server_data.mc_pid_locked = pid;
						printf("Following player %s\n", buffer + 2);
						mc->mc_mode |= MC_FOLLOW_PLAYER;
					} else {
						printf("Could not find player %s\n", buffer + 2);
					}
				} else {
					mc->mc_mode &= ~MC_FOLLOW_PLAYER;
					printf("No longer following\n");
				}
				break;
			default:
				printf("Invalid option '%c'\n", buffer[0]);
		}
	}
	
	return NULL;
}

int mc_client_command_parser (mc_connection *mc, char *buf)
{
	int i;
	int x1, y1, z1;
	int pid, type;
	
	if (!strncmp(buf + 1, "tp", 2)) {
		for (i = 4; i < 66; i++)
			if (buf[i] == ' ')
				buf[i] = 0;
		pid = get_pid_by_name(mc->mc_server_data, buf + 4);
		if (pid >= 0) {
			x1 = mc->mc_server_data.mc_player_db[pid].x;
			y1 = mc->mc_server_data.mc_player_db[pid].y;
			z1 = mc->mc_server_data.mc_player_db[pid].z;
			printf("Player %s at %d, %d, %d\n", buf + 4, x1, y1, z1);
			mc_proto_send_pos(mc, x1, y1, z1, 0, 0);
		} else {
			printf("Could not find player \"%s\"\n", buf + 4);
		}
	} else if (buf[1] == 'z') {
		if (strlen(buf) < 3 || sscanf(&buf[3], "%d", &type) != 1) 
			mc->mc_mode |= MC_USE_TYPE;
		else
			mc->mc_server_data.type = type;
		printf("Place two blocks to define the cuboid\n");
		mc->mc_mode |= MC_WAITING_FOR_1;
	} else if (buf[1] == 'n') {
		if (!(mc->mc_mode & MC_HIDE_POSITION)) {
			mc->mc_mode |= MC_HIDE_POSITION;
			printf("No longer sending position updates\n");
		} else {
			mc->mc_mode &= ~MC_HIDE_POSITION;
			printf("Sending position updates\n");
		}
	} else {
		printf("Invalid command %s\n", buf + 1);
	}

	return 0;
}