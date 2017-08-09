#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>


int * initConnection(char* target)
{
	struct sockaddr_in svr_address;
	int * tcp_socket = malloc(sizeof(int));
	*tcp_socket = socket(AF_INET, SOCK_STREAM, 0); 
	fcntl(*tcp_socket, F_SETFL, O_NONBLOCK);
	fd_set fdset;
	struct timeval tv;

	svr_address.sin_family = AF_INET;
	svr_address.sin_port = htons(80);
	svr_address.sin_addr.s_addr = inet_addr(target);
	memset(&(svr_address.sin_zero), '\0', 8);

	connect(*tcp_socket, (struct sockaddr *)&svr_address, sizeof(struct sockaddr));

	FD_ZERO(&fdset);
	FD_SET(*tcp_socket, &fdset);
	tv.tv_sec = 10;
	tv.tv_usec = 0;

	if (select(*tcp_socket + 1, NULL, &fdset, NULL, &tv) == 1)
	{
		int so_error;
		socklen_t len = sizeof(so_error);

		getsockopt(*tcp_socket, SOL_SOCKET, SO_ERROR, &so_error, &len);

		if (!so_error)
		{
			return tcp_socket;
		}
	}
	// return failed connection if this point is reached
	free(tcp_socket);
	return NULL;
}

int main(int argc, char* argv[])
{
	if(argc != 3)
	{
		printf("%s\n", "Incorrect number of arguments. Correct format: ");
		printf("%s\n", "[ip address] [connection count]");
		return 0;
	}
	
	char* target_address = argv[1];
	int num_conns = atoi(argv[2]);

	int* open_conns[num_conns];
	int actv_cnt = 0;
	int i;
	for(i = 0; i < num_conns; i++)
	{
		int* resp = initConnection(target_address);
		if(resp == NULL)
		{
			printf("%s\n", "error connecting");
		}
		else
		{
			printf("%s\n", "connection established");
			open_conns[actv_cnt] = resp;
			++actv_cnt;
		}
	}

	int* active_conns[actv_cnt];
	memcpy(active_conns, open_conns, sizeof(active_conns) );
	
	return 0;
}