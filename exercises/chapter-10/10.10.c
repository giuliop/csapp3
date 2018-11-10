#include "csapp.h"

int main(int arg, char **argv) {
	int n;
	rio_t rio;
	char buf[MAXLINE];
	char * infile;
	int fd = 0;

	if (argv[1]) {
		infile = argv[1];
		if ((fd = open(infile, O_RDONLY, 0)) == -1) {
			perror("open file error");
			exit(-1);
		}
		dup2(fd, STDIN_FILENO);
	}
	while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0)
		Rio_writen(STDOUT_FILENO, buf , n) ;

	if (fd)
		close(fd);

	return 1;
}
