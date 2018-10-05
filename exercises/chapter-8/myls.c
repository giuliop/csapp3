#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[], char *envp[]) {
	char prog[] = "/bin/ls";
	if (execve(prog, argv, envp))
		fprintf(stderr, "Error with execve: %s\n", strerror(errno));
	exit(-1);
}
