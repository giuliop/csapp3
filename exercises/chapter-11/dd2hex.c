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

#define RES_LEN 10  // 10 chars: 0x11223344

int main (int argc, char **argv)
{
	if (! (argc > 1))
		return 0;

	int res[4] = { 0 };	// to hold the 4 bytes of the hex representation
	char n[4] = { 0 };	// to hold a single byte decimal representation from the input
	n[4] ='\0';

	char * byte = argv[1];
	char * next_dot;
	for (int i = 0; i < 4; i++, byte = next_dot + 1) {
		if (i<3)
			next_dot = strchr(byte, (int)'.');
		else
			next_dot += 4;	// at most 4, if less strncpy will stop at \0 terminator anyway
		strncpy(n, byte, next_dot - byte);
		n[next_dot - byte] = '\0';
		errno = 0;
		res[i] = (int)strtol(n, NULL, 10);
		if (errno) {
			perror("malformed address\n");
			exit(-1);
		}
	}

	printf("0x%02x%02x%02x%02x\n", res[0], res[1], res[2], res[3]);

	return 1;
}




