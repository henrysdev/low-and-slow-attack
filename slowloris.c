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

#define DEFAULT_PORT 80
#define TIMEOUT 10
#define ALIVE_LOOP_DELAY 4
#define MAX_PACKET_SIZE 512

// creates a new connection and returns a pointer to the socket that
// has been created
int * initConnection(char* target)
{
	// declare and initialize socket
	struct sockaddr_in svr_address;
	int32_t * tcp_socket = malloc(sizeof(int));
	*tcp_socket = socket(AF_INET, SOCK_STREAM, 0); 

	// file control and time declarations to allow for connection timeout
	fcntl(*tcp_socket, F_SETFL, O_NONBLOCK);
	fd_set fdset;
	struct timeval tv;

	// assign socket attribute appropriate values
	svr_address.sin_family = AF_INET;
	svr_address.sin_port = htons(DEFAULT_PORT);
	svr_address.sin_addr.s_addr = inet_addr(target);
	memset(&(svr_address.sin_zero), '\0', 8);

	// attempt connection to remote machine
	connect(*tcp_socket, (struct sockaddr *)&svr_address, sizeof(struct sockaddr));

	// timeout condition
	FD_ZERO(&fdset);
	FD_SET(*tcp_socket, &fdset);
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;

	// attempt connection before timeout
	if (select(*tcp_socket + 1, NULL, &fdset, NULL, &tv) == 1)
	{
		int32_t so_error;
		socklen_t len = sizeof(so_error);

		getsockopt(*tcp_socket, SOL_SOCKET, SO_ERROR, &so_error, &len);

		if (!so_error)
		{
			// return pointer to successfully connected socket if no error occurred
			return tcp_socket;
		}
	}
	// return failed connection if this point is reached
	free(tcp_socket);
	return NULL;
}

// formats a standard HTTP request given a series of parameters 
char * formHttpRequest(char* url_path, char* host_address, char* request_body, uint32_t cont_len_hdr)
{
	char * http_request = malloc(sizeof(char) * MAX_PACKET_SIZE);
	snprintf(http_request, MAX_PACKET_SIZE, 
		"GET %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"Content-type: application/x-www-form-urlencoded\r\n"
		"Content-length: %d\r\n\r\n"
		"%s\r\n", url_path, host_address, cont_len_hdr, request_body);
	return http_request;
}

int main(int argc, char* argv[])
{
	// check for correct number of arguments
	if(argc != 3)
	{
		printf("%s\n", "Incorrect number of arguments. Correct format: ");
		printf("%s\n", "[ip address] [connection count]");
		return 0;
	}
	
	// parse arguments
	char* target_address = argv[1];
	int num_conns = atoi(argv[2]);

	// create array to hold sockets
	int32_t* open_conns[num_conns];
	int32_t actv_cnt = 0;
	int32_t i;

	// try to establish connection for every socket
	for(i = 0; i < num_conns; i++)
	{
		int32_t* resp = initConnection(target_address);
		if(resp == NULL)
		{
			printf("%s\n", "error connecting");
		}
		else
		{
			printf("%s%d\n", "connection established ",actv_cnt);
			open_conns[actv_cnt] = resp;
			++actv_cnt;
		}
	}

	// copy over connections that succeeded to new array
	int32_t* active_conns[actv_cnt];
	memcpy(active_conns, open_conns, sizeof(active_conns) );
	
	// keep alive loop
	for(;;)
	{
		// send keep alive headers
		for(i = 0; i < actv_cnt; i++)
		{
			char* url_path = "/projects";
			char* request_body = "abc123";
			uint32_t cont_len_hdr = strlen(request_body);

			// formatted http request string to be sent
			char* http_request = formHttpRequest(url_path, target_address, request_body, cont_len_hdr);
			
			int32_t resp;
			resp = write(*active_conns[i], http_request, strlen(request_body) + 1);

			if(resp < 0)
			{
				printf("%s\n", "failed to send HTTP request");
			}
			else
			{
				printf("%s\n", "sent stay alive packet successfully");
			}
			free(http_request);
		}
		// delay before sending next wave of packets
		sleep(ALIVE_LOOP_DELAY);
	}

	return 0;
}