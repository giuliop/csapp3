#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <wait.h>

extern char** environ;

int mysystem(char * command) {
	pid_t pid; 
	int status;
	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "Error with fork: %s\n", strerror(errno));
		return -1;
	}
	if (pid == 0) {
		char * args[4] = { "", "-c", command, NULL };
		status = execve("/bin/sh", args, environ);
		if (status) {
			fprintf(stderr, "Error with execve: %s\n", strerror(errno));
			return -1;
		}
		return status;
	}
		// PID > 0: parent process
		pid = waitpid(pid, &status, 0);
	if (pid == -1) {
		fprintf(stderr, "Error with waitpid: %s\n", strerror(errno));
		return -1;
	}
	if (WIFEXITED(status)) {
		int ret = WEXITSTATUS(status);
		return ret;
	} else {
		printf("child %d terminated abnormally\n", pid);
		return -1;
	}
}

int good_exit() {
	exit(10);
}

int main() {
	int st = mysystem("./mysystem_test");
	printf("Should print 10: %d\n", st);
	return 0;
}
