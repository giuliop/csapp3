#include <stdio.h>
#include <string.h>
#include <stdlib.h>

size_t hexCharToNum(char c) {
	switch (c) {
		case '0' : return 0;
		case '1' : return 1;
		case '2' : return 2;
		case '3' : return 3;
		case '4' : return 4;
		case '5' : return 5;
		case '6' : return 6;
		case '7' : return 7;
		case '8' : return 8;
		case '9' : return 9;
		case 'A' : return 10;
		case 'B' : return 11;
		case 'C' : return 12;
		case 'D' : return 13;
		case 'E' : return 14;
		case 'F' : return 15;
	}
	return -1;
}

// Takes a string hex representing a hex number as 0x... and returns a string
// representing its binary representation (to be freed by the caller)
char * strHextoBin (char * hex) {
	char * bin = 0;
	if (hex && (hex[0] == '0') && (hex[1] == 'x')) {
		const char binary[16][5] = { "0000", "0001", "0010", "0011", "0100", "0101",
			"0110", "0111", "1000", "1001", "1010", "1011", "1100", "1101",
			"1110", "1111" };
		hex += 2;
		size_t binLen = strlen(hex) * 4 + 1;
		bin = malloc(binLen * sizeof(char));
		bin[binLen - 1] = 0;
		for (size_t i = 0; *hex; ++hex, i += 4) {
			long n = hexCharToNum(*hex);
			strncpy(bin + i, binary[n], 4);
		}
	}
	return bin;
}

int main() {
	char * a = "0x00359141"; // 3510593
	char * b = "0x4A564504"; // 3510593.0
	printf("%s\n", strHextoBin(a));
	printf("  %s\n", strHextoBin(b));
}

