// Take a 32 bit ip address in hex form as input and convert it to decimal
// representation
// example: ./hex2dd 0x8002c2f2
//				  128.2.194.242

#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>

#define INPUT_LEN 10  // 10 chars: 0x11223344

// convert a hex argument to a dotted decimal string
int main (int argc, char **argv)
{
	if (! (argc > 1) || strlen(argv[1]) != INPUT_LEN)
		return 0;

	int res[4] = { 0 };		// to hold the resulting xxx.xxx.xxx.xxx
	char n[3] = { 0 };			// to hold a single input byte to convert from the input
	n[2] ='\0';

	char * byte = argv[1]+2;	// +2 to offset 0x at the beginning of argv[1]
	for (int i = 0; i < 4; i++, byte+=2) {
		strncpy(n, byte, 2);
		errno = 0;
		res[i] = strtol(n, NULL, 16);
		if (errno) {
			perror("malformed address\n");
			exit(-1);
		}
	}

	printf("%d.%d.%d.%d\n", res[0], res[1], res[2], res[3]);

	return 1;
}




