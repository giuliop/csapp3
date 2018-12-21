// We start a child processes that sleeps for five seconds and then
// terminates triggering a signal handler that times out tfgets
// with a non local jump

#include "csapp.h"

sigjmp_buf jmpbuf;

void handler(int sig) {
	siglongjmp(jmpbuf, 1);
}

void timer() {
	if (fork() == 0) {	// child process
		sleep(5);
		exit(0);
	}
}

char * tfgets(char *s, int size, FILE * stream) {

	if (! sigsetjmp(jmpbuf, 1))
		Signal(SIGCHLD, handler);
	else
		return NULL;

	timer();

	fgets(s, size, stream);
	Signal(SIGCHLD, SIG_DFL);
	return s;
}

int main() {
	char buf[MAXLINE];

	if (tfgets(buf, MAXLINE, stdin) == NULL)
		printf("BOOM!\n");
	else
		printf("%s", buf);

	exit(0);
}
