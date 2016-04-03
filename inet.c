#include "inet.h"

int mc_server_connect (mc_connection *mc)
{
	struct sockaddr_in sin;
	int ret;
	
	mc->mc_server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (inet_pton(AF_INET, (char *)mc->mc_server_address, &sin.sin_addr) < 0) {
		mc_disconnect(mc);
		return -1;
	}
	sin.sin_port = htons(mc->mc_server_port);
	sin.sin_family = AF_INET;

	ret = connect(mc->mc_server_sock, (struct sockaddr *)&sin, sizeof(struct sockaddr_in));
	if (ret < 0) {
		mc_disconnect(mc);
		return -1;
	}
	return 0;
}

int mc_client_connect (mc_connection *mc)
{
	int e = 1, t;
	struct sockaddr_in sin; 
	
	mc->mc_client_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	setsockopt(mc->mc_client_sock,SOL_SOCKET,SO_REUSEADDR,(const char*)&e,sizeof(int));
	
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(mc->mc_client_port);
	sin.sin_family = AF_INET;
	
	bind (mc->mc_client_sock, (struct sockaddr *)&sin, sizeof(sin));
	listen (mc->mc_client_sock, 1);
	
	t = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	t = -1;
	while (t == -1) 
		t = accept(mc->mc_client_sock, NULL, NULL);
	
	mc->mc_client_sock = t;
	
	return 0; 
}

int mc_bot_startup (const char *name, const char *ip, in_port_t port, const char *mppass)
{
	mc_connection *mc = malloc(sizeof(mc_connection));

	mc->mc_mode = MC_CLIENT;
	mc->mc_server_address = strdup(ip);
	mc->mc_client_name = strdup(name);
	mc->mc_server_password = strdup(mppass);
	mc->mc_server_port = port;
	
	if (mc_server_connect(mc) < 0) 
		return -1;
	printf("Connected to server\n");

	mc_server_authenticate(mc, name);
	printf("Authenticated\n");

	pthread_create(&mc->mc_server_thread, NULL, mc_server_run_listener, mc);
	pthread_create(&mc->mc_cmd_thread, NULL, mc_command_run_listener, mc);
	
	pthread_join(mc->mc_server_thread, NULL);
	pthread_join(mc->mc_cmd_thread, NULL);
	
	return 0;

}

int mc_proxy_startup (const char *name, const char *ip, in_port_t port, const char *mppass)
{
	mc_connection *mc = malloc(sizeof(mc_connection));

	mc->mc_mode = MC_TUNNEL;
	mc->mc_server_address = strdup(ip);
	mc->mc_client_name = strdup(name);
	mc->mc_server_password = strdup(mppass);
	mc->mc_server_port = port;
	
	mc->mc_client_port = 33344;
	
	if (mc_server_connect(mc) < 0) 
		return -1;
	printf("Connected to server\n");

	if (mc_client_connect(mc) < 0)
		return -1;
	printf("Connected to client\n");

	pthread_create(&mc->mc_server_thread, NULL, mc_server_run_listener, mc);
	pthread_create(&mc->mc_client_thread, NULL, mc_client_run_listener, mc);
	
	pthread_join(mc->mc_server_thread, NULL);
	pthread_join(mc->mc_client_thread, NULL);
	
	return 0;
}

void mc_disconnect (mc_connection *mc)
{
	close(mc->mc_server_sock);
	close(mc->mc_client_sock);
	
	free((char *)mc->mc_server_address);
	free((char *)mc->mc_server_password);
	
	free(mc);
}

char *mc_get_packet(int stc) {
	char *buffer;
	int i;
	ssize_t a;
	buffer = malloc(1);
	a = recv(stc, buffer, 1, 0);
	if (a < 0)
		return NULL;
	i = packlen(*buffer);
	if (i > 1) {
		buffer = realloc(buffer, i);
		while (a != i-1){
			a = recv(stc, buffer+1, i-1, MSG_PEEK);
		}
		a = recv(stc, buffer+1, i-1, 0);
	} 
	return buffer;
}

void * mc_server_run_listener (void *arg)
{
	mc_connection *mc = (mc_connection *)arg;
	
	while(1) {
		char *pack_buf = mc_get_packet(mc->mc_server_sock);
		if (pack_buf == NULL)
			break;
		if (mc_server_parse_packet(mc, pack_buf) == MC_SEND && mc->mc_mode & MC_TUNNEL)
			send(mc->mc_client_sock,pack_buf,packlen(pack_buf[0]),0);
		free(pack_buf); 
	}

error:
	printf("Server connection terminated, quitting...\n");
	pthread_cancel(mc->mc_client_thread);
	pthread_cancel(mc->mc_server_thread);
	return NULL;
}

void * mc_client_run_listener (void *arg)
{
	mc_connection *mc = (mc_connection *)arg;
	
	while(1) {
		char *pack_buf = mc_get_packet(mc->mc_client_sock);
		if (pack_buf == NULL)
			break;
		if (mc_client_parse_packet(mc, pack_buf) == MC_SEND && mc->mc_mode & MC_TUNNEL)
			send(mc->mc_server_sock,pack_buf,packlen(pack_buf[0]),0);
		free(pack_buf); 
	}
	
error:
	printf("Client connection terminated, quitting...\n");
	pthread_cancel(mc->mc_client_thread);
	pthread_cancel(mc->mc_server_thread);
	return NULL;
}
