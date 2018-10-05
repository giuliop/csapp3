#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

unsigned int snooze(unsigned int secs) {
	unsigned int left = sleep(secs);
	printf("\nSlept for %d of %d seconds.\n\n", secs-left, secs);
	return left;
}

void sigint_handler(int sig) {
	return;
}

int main(int argc, char * argv[]) {
	if (signal(SIGINT, sigint_handler) == SIG_ERR) {
		printf("signal error");
		exit(0);
	}
	unsigned int secs = (unsigned int) strtol(argv[1], NULL, 10);
	snooze(secs);
	return 1;
}


