#include <unistd.h>
#include <sys/stat.h>
#include "csapp.h"


int main (int argc, char **argv) {
	struct stat stat;
	char *type, *readok;

	errno = 0;
	int fd = (int)strtol(argv[1], NULL, 10);
	if (errno) {
		perror("strtol error\n");
		exit(-1);
	}

	Fstat(fd, &stat);
	if (S_ISREG(stat.st_mode))
		type = "regular";
	else if (S_ISDIR(stat.st_mode))
		type = "directory";
	else
		type = "other";
	if (stat.st_mode & S_IRUSR)
		readok = "yes";
	else readok = "no";

	printf("type: %s, read: %s\n", type, readok);
	exit(0);
}


