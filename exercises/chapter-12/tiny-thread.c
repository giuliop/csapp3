// A simple, iterative HTTP/1.0 Web server that uses threads to
// implement concurrency

#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp, char * method, long * content_len);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, char * method);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, char * method);
void clienterror(int fd, char *cause, char *errnum, 
		char *shortmsg, char *longmsg);

void * thread_f (void * x);

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
	pthread_t tid;

	/* Check command line args */
	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}

	if (signal(SIGCHLD, sigchld_handler) == SIG_ERR)
		unix_error("signal error");

	listenfd = Open_listenfd(argv[1]);
	while (1) {
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); 
		Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
				port, MAXLINE, 0);
		printf("Accepted connection from (%s, %s)\n", hostname, port);

		int * thread_connfd = malloc(sizeof(int));
		*thread_connfd = connfd;
		pthread_create(&tid, NULL, thread_f, (void *)thread_connfd); 
	}
}

void * thread_f (void * x) {
	pthread_detach(pthread_self());
	int connfd = *((int *)x);
	free(x);
	doit(connfd);                                             
	Close(connfd);                                             
	return NULL;
}

/*
 * doit - handle one HTTP request/response transaction
 */
/* $begin doit */
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
