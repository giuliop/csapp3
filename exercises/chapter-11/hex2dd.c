// Take a 32 bit ip address in hex form as input and convert it to decimal
// representation
// example: ./hex2dd 0x8002c2f2
//				  128.2.194.242

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>

#define INPUT_LEN 10		// 10 chars: 0x11223344
#define OUTPUT_LEN 15  // max 15 chars xxx.xxx.xxx.xxx

// convert a hex argument to a dotted decimal string
int main (int argc, char **argv)
{
	if (! (argc > 1) || strlen(argv[1]) != INPUT_LEN)
		return 0;

	struct in_addr ip = { 0 };

	char res[OUTPUT_LEN + 1] = { 0 };		// + 1 for string terminator
	res[OUTPUT_LEN] ='\0';

	char * ip_hex = argv[1]+2;	// +2 to offset 0x at the beginning of argv[1]

	errno = 0;
	ip.s_addr = htonl((uint32_t)strtol(ip_hex, NULL, 16));
	if (errno) {
		perror("malformed address\n");
		exit(-1);
	}
	const char * ok = inet_ntop(AF_INET, &ip, res, OUTPUT_LEN);
	if (! ok) {
		perror("inet_ntop error\n");
		exit(-1);
	}

	printf("%s\n", res);
	return 1;
}




