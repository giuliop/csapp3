// A simple, iterative HTTP/1.0 Web server that uses I/O
// multiplexing to implement concurrency

#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp, char * method, long * content_len);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, char * method);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, char * method);
void clienterror(int fd, char *cause, char *errnum, 
		char *shortmsg, char *longmsg);

struct pool { // Represents a pool of connected descriptors
	int maxfd;										// Largest descriptor in read_set
	fd_set read_set;							// Set of all active descriptors
	fd_set ready_set;							// Subset of descriptors ready for reading
	int nready;										// Number of ready descriptors from select
	int maxi;											// Highwater index into client array
	int clientfd[FD_SETSIZE];			// Set of active descriptors
};

void init_pool(int listenfd, struct pool * p);
void add_client(int connfd, struct pool * p);
void check_clients(struct pool * p);

// signal handerl for SIGCHLD
void sigchld_handler(int sig) {
	Wait(NULL); /* Parent waits for and reaps child */ 
}

int main(int argc, char **argv) 
{
	int listenfd, connfd;
	char hostname[MAXLINE], port[MAXLINE];
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;
	static struct pool pool;

	/* Check command line args */
	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}

	if (signal(SIGCHLD, sigchld_handler) == SIG_ERR)
		unix_error("signal error");

	listenfd = Open_listenfd(argv[1]);
	init_pool(listenfd, &pool);

	while (1) {
		pool.ready_set = pool.read_set;
		pool.nready = Select(pool.maxfd + 1, &pool.ready_set, NULL, NULL, NULL);

		// If new client connected add it to the pool
		if (FD_ISSET(listenfd, &pool.ready_set)) {
			clientlen = sizeof(clientaddr);
			connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); 
			Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
					port, MAXLINE, 0);
			printf("Accepted connection from (%s, %s)\n", hostname, port);
			add_client(connfd, &pool);
		}

		// Serve the connected clients
		check_clients(&pool);
	}
}

void init_pool(int listenfd, struct pool * p) {
	p->maxfd = listenfd;
	FD_ZERO(&p->read_set);
	FD_SET(listenfd, &p->read_set);
	for (int i = 0; i < FD_SETSIZE; ++i)
		p->clientfd[i] = -1;
	p-> maxi = -1;
}

void add_client(int connfd, struct pool * p) {
	printf("Adding client %d\n", connfd);
	int i;
	--(p->nready);

	for (i = 0; i < FD_SETSIZE; ++i) {
		if (p->clientfd[i] < 0) {

			p->clientfd[i] = connfd;
			FD_SET(connfd, &p->read_set);

			if (p->maxi < i)
				p->maxi = i;

			if (p->maxfd < connfd)
				p->maxfd = connfd;

		break;
		}
	}
	if (i == FD_SETSIZE)
		app_error("add_client error: too many clients");
	printf("Client %d added\n", connfd);
}

void check_clients(struct pool * p) {
	int connfd, i;
	for (i = 0; (i <= p->maxi) && (p->nready > 0); ++i) {
		connfd = p->clientfd[i];
		if ((connfd > 0) && FD_ISSET(connfd, &p->ready_set)) {
			doit(connfd);                                             
			Close(connfd);                                            
			--(p->nready);
			FD_CLR(connfd, &p->read_set);
			p->clientfd[i] = -1;
		}
	}
}

// doit - handle one HTTP request/response transaction
void doit(int fd) 
{
	int is_static;
	struct stat sbuf;
	long content_len = 0;
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	char filename[MAXLINE], cgiargs[MAXLINE];
	rio_t rio;

	/* Read request line and headers */
	Rio_readinitb(&rio, fd);
	if (!Rio_readlineb(&rio, buf, MAXLINE))  
		return;
	printf("%s", buf);
	sscanf(buf, "%s %s %s", method, uri, version);       
	if ((strcasecmp(method, "HEAD")) &&
			(strcasecmp(method, "GET")) &&
			(strcasecmp(method, "POST"))) {                     
		clienterror(fd, method, "501", "Not Implemented",
				"Tiny does not implement this method");
		return;
	}
	read_requesthdrs(&rio, method, &content_len);

	/* Parse URI from HEAD/GET request */
	is_static = parse_uri(uri, filename, cgiargs);       

	/* if POST is dynamic and we need to parse args from request body*/
	if (! strcasecmp(method, "POST")) {
			is_static = 0;
			Rio_readnb(&rio, cgiargs, content_len);
	}

	printf("\n%s\n", filename);
	if (stat(filename, &sbuf) < 0) {                     
		// we try to append .html to filename
		if (strlen(filename) + 5 < MAXLINE) {
			strcat(filename, ".html");
			if (stat(filename, &sbuf) < 0) {                     
				clienterror(fd, filename, "404", "Not found",
						"Tiny couldn't find this file");
				return;
			}                                                    
		}
	}

	if (is_static) { /* Serve static content */          
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) { 
			clienterror(fd, filename, "403", "Forbidden",
					"Tiny couldn't read the file");
			return;
		}
		serve_static(fd, filename, sbuf.st_size, method);
	}
	else { /* Serve dynamic content */
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) { 
			clienterror(fd, filename, "403", "Forbidden",
					"Tiny couldn't run the CGI program");
			return;
		}
		serve_dynamic(fd, filename, cgiargs, method);            
	}
}

/* read_requesthdrs - read HTTP request headers, if method is post
	 then fills content_len with the request content_length*/
void read_requesthdrs(rio_t *rp, char * method, long * content_len) 
{
	char buf[MAXLINE], last[MAXLINE];

	Rio_readlineb(rp, buf, MAXLINE);
	printf("%s", buf);
	while(strcmp(buf, "\r\n")) {          
		strncpy(last, buf, MAXLINE);
		Rio_readlineb(rp, buf, MAXLINE);
		printf("%s", buf);
	}

	if (! strcmp(method, "POST")) {
		char * p = index(last, ' ');
		* content_len = strtol(p+1, NULL, 10);
	}

	return;
}

/* parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static */
int parse_uri(char *uri, char *filename, char *cgiargs) 
{
	char *ptr;

	if (!strstr(uri, "cgi-bin")) {  /* Static content */ 
		strcpy(cgiargs, "");                             
		strcpy(filename, ".");                           
		strcat(filename, uri);                           
		if (uri[strlen(uri)-1] == '/')                   
			strcat(filename, "index.html");               
		return 1;
	}
	else {  /* Dynamic content */                        
		ptr = index(uri, '?');                           
		if (ptr) {
			strcpy(cgiargs, ptr+1);
			*ptr = '\0';
		}
		else 
			strcpy(cgiargs, "");                         
		strcpy(filename, ".");                           
		strcat(filename, uri);                           
		return 0;
	}
}

/* serve_static - copy a file back to the client */
void serve_static(int fd, char *filename, int filesize, char *method) 
{
	int srcfd;
	char *srcp, filetype[MAXLINE], buf[MAXBUF];

	/* Send response headers to client */
	get_filetype(filename, filetype);       
	sprintf(buf, "HTTP/1.0 200 OK\r\n");    
	sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
	sprintf(buf, "%sConnection: close\r\n", buf);
	sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
	sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
	Rio_writen(fd, buf, strlen(buf));       
	printf("Response headers:\n");
	printf("%s", buf);

	if (! strcasecmp(method, "HEAD"))
		return;

	/* Send response body to client */
	srcfd = Open(filename, O_RDONLY, 0);    
	srcp = malloc(filesize);
	Rio_readn(srcfd, srcp, filesize);
	Close(srcfd);                           
	Rio_writen(fd, srcp, filesize);         
}

/* get_filetype - derive file type from file name */
void get_filetype(char *filename, char *filetype) 
{
	if (strstr(filename, ".html"))
		strcpy(filetype, "text/html");
	else if (strstr(filename, ".css"))
		strcpy(filetype, "text/css");
	else if (strstr(filename, ".gif"))
		strcpy(filetype, "image/gif");
	else if (strstr(filename, ".png"))
		strcpy(filetype, "image/png");
	else if (strstr(filename, ".jpg"))
		strcpy(filetype, "image/jpeg");
	else if (strstr(filename, ".mpg"))
		strcpy(filetype, "video/mpeg");
	else
		strcpy(filetype, "text/plain");
}  

/* serve_dynamic - run a CGI program on behalf of the client */
void serve_dynamic(int fd, char *filename, char *cgiargs, char * method) 
{
	char buf[MAXLINE], *emptylist[] = { NULL };

	/* Return first part of HTTP response */
	sprintf(buf, "HTTP/1.0 200 OK\r\n"); 
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Server: Tiny Web Server\r\n");
	Rio_writen(fd, buf, strlen(buf));

	if (Fork() == 0) { /* Child */ 
		/* Real server would set all CGI vars here */
		setenv("QUERY_STRING", cgiargs, 1); 
		setenv("METHOD", method, 1); 
		Dup2(fd, STDOUT_FILENO);         /* Redirect stdout to client */ 
		Execve(filename, emptylist, environ); /* Run CGI program */ 
	}
}

/* clienterror - returns an error message to the client */
void clienterror(int fd, char *cause, char *errnum, 
		char *shortmsg, char *longmsg) 
{
	char buf[MAXLINE], body[MAXBUF];

	/* Build the HTTP response body */
	sprintf(body, "<html><title>Tiny Error</title>");
	sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
	sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
	sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
	sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

	/* Print the HTTP response */
	sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-type: text/html\r\n");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
	Rio_writen(fd, buf, strlen(buf));
	Rio_writen(fd, body, strlen(body));
}
