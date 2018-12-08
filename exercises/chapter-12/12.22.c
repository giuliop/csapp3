#include "csapp.h"

int echoline(int connfd, rio_t rio);
void command(void);

int main(int argc, char **argv) 
{
	int listenfd, connfd;
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;
	fd_set read_set, ready_set;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(0);
	}
	listenfd = Open_listenfd(argv[1]); 

	FD_ZERO(&read_set);										/* Clear read set */
	FD_SET(STDIN_FILENO, &read_set);			/* Add stdin to read set */
	FD_SET(listenfd, &read_set);					/* Add listenfd to read set */
	int n = listenfd+1;
	rio_t rio;

	while (1) {
		ready_set = read_set;
		Select(n, &ready_set, NULL, NULL, NULL);
		if (FD_ISSET(STDIN_FILENO, &ready_set))
			command();												/* Read command line from stdin */
		if (FD_ISSET(listenfd, &ready_set)) {
			clientlen = sizeof(struct sockaddr_storage); 
			connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
			Rio_readinitb(&rio, connfd);
			printf("Accepting connection on fd: %d\n", connfd);
			FD_SET(connfd, &read_set);				/* Add connfd to read set */
			n = connfd+1;

		}
		if ((connfd > 0) && (FD_ISSET(connfd, &ready_set))) {
			int n = echoline(connfd, rio);
			if (n <= 0) {
				Close(connfd);
				FD_CLR(connfd, &read_set);
			}
		}
	}
}

void command(void) {
	char buf[MAXLINE];
	if (!Fgets(buf, MAXLINE, stdin))
		exit(0);											/* EOF */
	printf("you said: %s", buf);							/* Process the input command */
}


int echoline(int connfd, rio_t rio) 
{
	size_t n; 
	char buf[MAXLINE]; 

	n = Rio_readlineb(&rio, buf, MAXLINE);
	if (n > 0) {
		printf("server received %d bytes\n", (int)n);
		Rio_writen(connfd, buf, n);
	}
	return n;
}
