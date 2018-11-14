// Take a 32 bit ip address in decimal representation and converts it 
// in hex form
// representation
// example: ./dd2hex 128.2.194.242
//				  0x8002c2f2

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>

int main (int argc, char **argv)
{
	if (! (argc > 1))
		return 0;

	struct in_addr ip = { 0 };

	int ok = inet_pton(AF_INET, argv[1], &ip);
	if (! ok) {
		perror("Malformed input\n");
		return -1;
	}
	printf("0x%x\n", ntohl(ip.s_addr));
	return 1;
}




