#include "csapp.h"
#define TIMEOUT 5

volatile sig_atomic_t flag;

void sigalarm_handler(int sig) {
	sio_puts("Timeout baby!\n");
	_exit(0);
}

char * tgets(char * dst, int max, FILE *stream) {
	// install handler for alarm
	if (signal(SIGALRM, sigalarm_handler) == SIG_ERR) {
		unix_error("signal_error");
	}
	// set alarm
	alarm(TIMEOUT);
	return fgets(dst, max, stream);
}

int main() {
	char str[100];
	pid_t pid = Fork();
	if (pid == 0) {
		tgets(str, 100, stdin);
		printf("You typed: %s\n", str);
	}
	wait(NULL);
	return 0;
}
