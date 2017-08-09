#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int tcp_socket;

int* initConnection(char* target)
{
	struct sockaddr_in svr_address;

	// ERROR CHECKING NECESSARY
	tcp_socket = socket(AF_INET, SOCK_STREAM, 0); 

	svr_address.sin_family = AF_INET;
	svr_address.sin_port = htons(80);
	svr_address.sin_addr.s_addr = inet_addr(target);
	memset(&(svr_address.sin_zero), '\0', 8);

	if(0 > connect(tcp_socket, (struct sockaddr *)&svr_address, sizeof(struct sockaddr)))
	{
		return NULL;
	}
	return &tcp_socket;
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
	int n = 0;
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
			open_conns[n] = resp;
			++n;
		}
	}

	return 0;
}