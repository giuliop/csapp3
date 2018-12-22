// We start a child processes that sleeps for five seconds and then
// writes to a file signaling timeout. We use 'select' to wait for
// the first event between user input and timeout

#include "csapp.h"

char * tfgets(char *s, int size, FILE * stream) {

	fd_set read_set;
	int stream_fd = fileno(stream);
	struct timeval tv = { .tv_sec = 5,
												.tv_usec = 0 };

	FD_ZERO(&read_set);
	FD_SET(stream_fd, &read_set);

	Select(stream_fd + 1, &read_set, NULL, NULL, &tv);

	if (FD_ISSET(stream_fd, &read_set)) {
		return fgets(s, size, stream);		// parent process
	} else
		return NULL;
}

int main() {
	char buf[MAXLINE];

	if (tfgets(buf, MAXLINE, stdin) == NULL)
		printf("BOOM!\n");
	else
		printf("%s", buf);

	exit(0);
}
