#include "csapp.h"
#define TIMEOUT 5

jmp_buf buf;

void sigalarm_handler(int sig) {
	siglongjmp(buf, 1);
}

char * tgets(char * dst, int max, FILE *stream) {
	// install handler for alarm
	if (signal(SIGALRM, sigalarm_handler) == SIG_ERR) {
		unix_error("signal_error");
	}
	// set alarm
	if (!sigsetjmp(buf, 1)) {
		alarm(TIMEOUT);
		return fgets(dst, max, stream);
	} else {
		return NULL;
	}
}

int main() {
	char str[100];
	char * s = tgets(str, 100, stdin);
	if (s)
		printf("\nYou typed: %s\n", s);
	else
		printf("\nTimeout baby!\n");
	return 0;
}
