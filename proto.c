#include "proto.h"

ssize_t mc_proto_send_chat (mc_connection *mc, const char *msg)
{
	char send_buf[66];
	
	memset(send_buf, 0x20, 66);
	
	send_buf[0] = 0x0D; /* protocol no. */
	send_buf[1] = 0xFF; /* unused */
	
	strncpy(send_buf+2, msg, strlen(msg));

	return send (mc->mc_server_sock, send_buf, sizeof(send_buf), 0);
}

ssize_t mc_proto_send_pos (mc_connection *mc, short x, short y, short z, char rotx, char roty)
{
	mc->mc_server_data.mc_player_db[256].x = x;
	mc->mc_server_data.mc_player_db[256].y = y;
	mc->mc_server_data.mc_player_db[256].z = z;
	
	char buffer[10];
	buffer[0] = 0x08;
	buffer[1] = 0xFF;

	x = (htons((x) * 32));
	y = (htons((y) * 32));
	z = (htons((z) * 32));

	memcpy(&buffer[2], &x, sizeof(short));
	memcpy(&buffer[4], &y, sizeof(short));
	memcpy(&buffer[6], &z, sizeof(short));
	
	buffer[8] = (int)((float)rotx * (360/256));
	buffer[9] = (int)((float)roty * (360/256));
	
	return send (mc->mc_server_sock, buffer, 10, 0);
}

ssize_t mc_proto_send_block (mc_connection *mc, short x, short y, short z, char mode, char type)
{
	mc->mc_server_data.mc_level_data[offset(mc, x, y, z)] = type;
	
	char buffer[9];
	buffer[0] = 0x05;
	x = htons(x);
	y = htons(y);
	z = htons(z);
	memcpy(buffer+1, &x, sizeof(short));
	memcpy(buffer+3, &y, sizeof(short));
	memcpy(buffer+5, &z, sizeof(short));
	buffer[7] = mode;
	buffer[8] = type; 
	
	send(mc->mc_server_sock, buffer, 9, 0);
	
	buffer[0] = 0x06;
	if (!mode)
		buffer[7] = 0;
	else
		buffer[7] = type;
	
	return send (mc->mc_client_sock, buffer, 8, 0);
}

ssize_t mc_server_authenticate (mc_connection *mc, const char *name) 
{
	char send_buf[131];
	
	memset(send_buf,0x20,130);
	
	send_buf[0] = 0x00; /* protocol no. */
	send_buf[1] = 0x07; /* version */
	
	strncpy(send_buf+2, name, strlen(name));
	strncpy(send_buf+66, mc->mc_server_password, strlen(mc->mc_server_password));
	
	return send (mc->mc_server_sock, send_buf, sizeof(send_buf), 0);
}

int packlen (char code) 
{
	static const int lengths[] = {131, 1, 1, 1028, 7, 9, 8, 74, 10, 7, 5, 4, 2, 66, 65, 2, 0};
	if (code > 0x0f)
		return 0;
	return lengths[code];
}

int mc_server_parse_packet (mc_connection *mc, char *buffer)
{
	int i, ii;
	char buf[128];
	memset(buf,0, 128);
	if (buffer[0] == 0x00) {
		for(i = 2, ii = 0; i < 32; ++i, ++ii) {
			if(buffer[i] == '&') {
				if(i <= 32)
					i += 2;
			}
			buf[ii] = buffer[i];
		} buf[32] = 0;
		printf("Connected through MC to server %s\n",buf);
	}
	else if (buffer[0] == 0x02) {
		mc->mc_server_data.mc_current_size = 0;
		mc->mc_server_data.mc_unparsed_data = malloc(1024 * 1024);
	}
	else if (buffer[0] == 0x03) {
		unsigned short size;
		memcpy(&size, &buffer[1], sizeof(short));
		size = htons(size);
		if (size) {
			memcpy(mc->mc_server_data.mc_unparsed_data + 
			       mc->mc_server_data.mc_current_size, 
			       &buffer[3], size);
			
			mc->mc_server_data.mc_current_size += size;
		}
	}
	else if (buffer[0] == 0x04) {
		short tx,ty,tz;
		memcpy(&tx, &buffer[1], sizeof(short));
		memcpy(&ty, &buffer[3], sizeof(short));
		memcpy(&tz, &buffer[5], sizeof(short));
		mc->mc_server_data.mc_level_x = htons(tx);
		mc->mc_server_data.mc_level_y = htons(ty);
		mc->mc_server_data.mc_level_z = htons(tz);
		printf("Level Dimensions: %d %d %d\n", 
		       mc->mc_server_data.mc_level_x,
		       mc->mc_server_data.mc_level_y,
		       mc->mc_server_data.mc_level_z);

		printf("Prepared to decompress %d bytes\n", mc->mc_server_data.mc_current_size);
		
		FILE *fp = fopen("/Users/nobody1/Desktop/out_c.gz", "w");
		fwrite(mc->mc_server_data.mc_unparsed_data, 1, mc->mc_server_data.mc_current_size, fp);
		fclose(fp);
		
		system("gzip -df /Users/nobody1/Desktop/out_c.gz");
		fp = fopen("/Users/nobody1/Desktop/out_c", "r");
		
		int blocks;
		fread(&blocks, sizeof(int), 1, fp);
		blocks = htonl(blocks);
		printf("%d blocks\n", blocks);
		
		mc->mc_server_data.mc_level_data = malloc(blocks + 1);
		fread(mc->mc_server_data.mc_level_data, 1, blocks, fp);
		
		fclose(fp);
		remove("/Users/nobody1/Desktop/out_c");
		
		mc->mc_server_data.mc_level_size = mc->mc_server_data.mc_level_x * 
						mc->mc_server_data.mc_level_y * 
						mc->mc_server_data.mc_level_z;
		
		free(mc->mc_server_data.mc_unparsed_data);
		
	}
	
	else if (buffer[0] == 0x06){
		int x,y,z;
		char t;
		memcpy(&x, &buffer[1], sizeof(short));
		memcpy(&y, &buffer[3], sizeof(short));
		memcpy(&z, &buffer[5], sizeof(short));
		memcpy(&t, &buffer[7], sizeof(char));
		
		x = htons(x);
		y = htons(y);
		z = htons(z);

		if (mc->mc_mode & MC_LOCK_MAP){
			restore_and_move(mc, t, x, y, z);
		} else if (mc->mc_mode & MC_LOCK_AREA) {
			if (x >= mc->mc_server_data.x1 && x <= mc->mc_server_data.x2 &&
			    y >= mc->mc_server_data.y1 && y <= mc->mc_server_data.y2 &&
			    z >= mc->mc_server_data.z1 && z <= mc->mc_server_data.z2) {
				restore_and_move(mc, t, x, y, z);
			}
		} else if (mc->mc_mode & MC_LOCK_PLAYER) {
			int pid = mc->mc_server_data.mc_pid_locked;
			int xp = mc->mc_server_data.mc_player_db[pid].x;
			int yp = mc->mc_server_data.mc_player_db[pid].y;
			int zp = mc->mc_server_data.mc_player_db[pid].z;
			
			if (xp == 1024 && yp == 1024 && zp == 1024) {
				printf("lost connection to player\n");
			} else {
				if (dist3d(x, y, z, xp, yp, zp) < 10.0f) {
					restore_and_move(mc, t, x, y, z);
				}
			}
		} else {
			mc->mc_server_data.mc_level_data[offset(mc, x, y, z)] = t;
		}
	}
	
	else if (buffer[0] == 0x07) {
		for(i = 2, ii = 0; i < 66; ++i, ++ii) {
			if(buffer[i] == '&') {
				i+=2;}
			if(buffer[i] != ' ') {
				buf[ii] = buffer[i];
			}
		} 
		int spid = buffer[1];
		printf("[SERVER] %s joined the game. (%d)\n",buf,spid);
		if (spid == -1 || !strcmp(buf, mc->mc_client_name)) 
			spid = 256;

		mc->mc_server_data.mc_player_db[spid].pid = spid;
		strcpy(mc->mc_server_data.mc_player_db[spid].name, buf);
		short xp, yp, zp;
		memcpy(&xp, &buffer[2], sizeof(short));
		memcpy(&yp, &buffer[4], sizeof(short));
		memcpy(&zp, &buffer[6], sizeof(short));

		mc->mc_server_data.mc_player_db[spid].x = htons(xp)/32;
		mc->mc_server_data.mc_player_db[spid].y = htons(yp)/32;
		mc->mc_server_data.mc_player_db[spid].z = htons(zp)/32;
		
		mc->mc_server_data.mc_player_db[spid].rotx = buffer[8] * (360/256);
		mc->mc_server_data.mc_player_db[spid].roty = buffer[9] * (360/256);
		
	}
	else if (buffer[0] == 0x08) {
		int spid = buffer[1];
		if (spid == -1) 
			spid = 256;

		short xp, yp, zp;
		memcpy(&xp, &buffer[2], sizeof(short));
		memcpy(&yp, &buffer[4], sizeof(short));
		memcpy(&zp, &buffer[6], sizeof(short));
		
		mc->mc_server_data.mc_player_db[spid].x = htons(xp)/32;
		mc->mc_server_data.mc_player_db[spid].y = htons(yp)/32;
		mc->mc_server_data.mc_player_db[spid].z = htons(zp)/32;
		
		mc->mc_server_data.mc_player_db[spid].rotx = buffer[8] * (360/256);
		mc->mc_server_data.mc_player_db[spid].roty = buffer[9] * (360/256);
		
		if (mc->mc_mode & MC_FOLLOW_PLAYER) {
			follow_move(mc, mc->mc_server_data.mc_pid_locked);
		}
	}
	else if (buffer[0] == 0x09) {
		int spid = buffer[1];
		if (spid == -1) 
			spid = 256;
		
		mc->mc_server_data.mc_player_db[spid].x = buffer[2]/32;
		mc->mc_server_data.mc_player_db[spid].y = buffer[3]/32;
		mc->mc_server_data.mc_player_db[spid].z = buffer[4]/32;
		
		mc->mc_server_data.mc_player_db[spid].rotx = buffer[5] * (360/256);
		mc->mc_server_data.mc_player_db[spid].roty = buffer[6] * (360/256);
		
		if (mc->mc_mode & MC_FOLLOW_PLAYER) {
			follow_move(mc, mc->mc_server_data.mc_pid_locked);
		}
	}
	else if (buffer[0] == 0x0A) {
		int dpid = buffer[1];
		
		if (dpid == -1)
			dpid = 256;
		mc->mc_server_data.mc_player_db[dpid].x += buffer[2]/32;
		mc->mc_server_data.mc_player_db[dpid].y += buffer[3]/32;
		mc->mc_server_data.mc_player_db[dpid].z += buffer[4]/32;
		
		if (mc->mc_mode & MC_FOLLOW_PLAYER) {
			follow_move(mc, mc->mc_server_data.mc_pid_locked);
		}
	}
	else if (buffer[0] == 0x0B) {
		int dpid = buffer[1];
		mc->mc_server_data.mc_player_db[dpid].rotx = buffer[2] * (360/256);
		mc->mc_server_data.mc_player_db[dpid].roty = buffer[3] * (360/256);
		
		if (mc->mc_mode & MC_FOLLOW_PLAYER) {
			follow_move(mc, mc->mc_server_data.mc_pid_locked);
		}
		
	}
	else if (buffer[0] == 0x0C) {
		int piddc = buffer[1];
		printf("[SERVER] %s left the game.\n", mc->mc_server_data.mc_player_db[piddc].name);
	}


	else if (buffer[0] == 0x0D) {
		for(i = 2, ii = 0; i < 66; ++i, ++ii) {
			if(buffer[i] == '&') {
				if(i <= 64)
					i += 2;
			}
			buf[ii] = buffer[i];
		} buf[64] = 0;
		
		printf("%s\n",buf);
	}
	else if (buffer[0] == 0x0E) {
		printf("[DISCONNECT] %s\n",buffer);
		return -1;
	}
	return MC_SEND;
}

void *cuboid_thread (void *arg)
{
	mc_connection *mc = (mc_connection *)arg;
	struct mc_server_data data = mc->mc_server_data;
	
	cuboid(mc, data.type, data.x1, data.y1, data.z1, data.x2, data.y2, data.z2);
	
	return NULL;
}

int mc_client_parse_packet (mc_connection *mc, char *buffer)
{
	int i, ii;
	int x, y, z;
	char buf[129];
	
	if (buffer[0] == 0x00) {
		memset(buffer+2, 0x20, 129);
		memcpy(buffer+2, mc->mc_client_name, strlen(mc->mc_client_name));
		memcpy(buffer+66, mc->mc_server_password, strlen(mc->mc_server_password));
	} else if (buffer[0] == 0x05) {
		memcpy(&x, &buffer[1], sizeof(short));
		memcpy(&y, &buffer[3], sizeof(short));
		memcpy(&z, &buffer[5], sizeof(short));
		mc->mc_server_data.mc_level_data[offset(mc, htons(x), htons(y), htons(z))] = buffer[8];
		if (mc->mc_mode & MC_WAITING_FOR_1) {
			mc->mc_server_data.x1 = htons(x);
			mc->mc_server_data.y1 = htons(y);
			mc->mc_server_data.z1 = htons(z);
			
			printf("First point: %d %d %d\n", 
			       mc->mc_server_data.x1, mc->mc_server_data.y1, mc->mc_server_data.z1);
			
			restore_block(mc, buffer[8], mc->mc_server_data.x1, mc->mc_server_data.y1, mc->mc_server_data.z1);
			
			mc->mc_mode &= ~MC_WAITING_FOR_1;
			mc->mc_mode |= MC_WAITING_FOR_2;
			
			return MC_CONTINUE;
		} else if (mc->mc_mode & MC_WAITING_FOR_2) {
			mc->mc_server_data.x2 = htons(x);
			mc->mc_server_data.y2 = htons(y);
			mc->mc_server_data.z2 = htons(z);
			
			printf("Second point: %d %d %d\n", 
			       mc->mc_server_data.x2, mc->mc_server_data.y2, mc->mc_server_data.z2);
			
			restore_block(mc, buffer[8], mc->mc_server_data.x2, mc->mc_server_data.y2, mc->mc_server_data.z2);
			
			if (mc->mc_mode & MC_USE_TYPE) {
				mc->mc_server_data.type = buffer[8];
				printf("Using type %d\n", buffer[8]);
				mc->mc_mode &= ~MC_USE_TYPE;
			}
			mc->mc_mode &= ~MC_WAITING_FOR_2;
			
			pthread_t thd;
			pthread_create(&thd, NULL, cuboid_thread, mc);
			pthread_join(thd, NULL);

			return MC_CONTINUE;
		}
	} else if (buffer[0] == 0x08) {
		if (mc->mc_mode & MC_HIDE_POSITION) {
			return MC_CONTINUE;
		}
	}
	else if (buffer[0] == 0x0D) {
		for(i = 2, ii = 0; i < 66; ++i, ++ii) {
			if(buffer[i] == '&') {
				if(i <= 64)
					i += 2;
			}
			buf[ii] = buffer[i];
		} buf[64] = 0;

		if (buf[0] == '\'') {
			mc_client_command_parser(mc, buf);
			return MC_CONTINUE;
		}
	}
	return MC_SEND;
}