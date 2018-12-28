// a simple, threaded web proxy
#include "csapp.h"
#include <string.h>
#include <assert.h>

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

struct req_hdrs {
	char host[MAXLINE];
	char user_agent[MAXLINE];
	char connection[MAXLINE];
	char proxy_connection[MAXLINE];
	char others[MAXLINE];
};

void set_def_req_hdrs(struct req_hdrs * rh) {
	strcpy(rh->user_agent, "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n");
	strcpy(rh->connection, "Connection: close\r\n");
	strcpy(rh->proxy_connection, "Proxy_connection: close\r\n");
}

void doit(int fd);
void read_requesthdrs(rio_t * rp, struct req_hdrs * rh);
void serve(int fd, char *filename, int filesize, char * method);
void get_filetype(char *filename, char *filetype);
void get_host(char * uri, char * dest);
void clienterror(int fd, char * cause, char *errnum, 
		char * shortmsg, char *longmsg);
void * thread_f (void * x);
void test_get_host();

int main(int argc, char **argv) 
{
	test_get_host();

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

	/*if (signal(SIGCHLD, sigchld_handler) == SIG_ERR)*/
		/*unix_error("signal error");*/

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

// doit - handle one HTTP request/response transaction
void doit(int fd) 
{
	/* Read request line and headers */
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	rio_t rio;

	Rio_readinitb(&rio, fd);
	if (!Rio_readlineb(&rio, buf, MAXLINE))  
		return;

	printf("%s", buf);
	sscanf(buf, "%s %s %s", method, uri, version);       
	if ((strcasecmp(method, "HEAD")) &&
			(strcasecmp(method, "GET")) &&
			(strcasecmp(method, "POST"))) {                     
		clienterror(fd, method, "501", "Not Implemented",
				"Tiny Proxy does not implement this method");
		return;
	}

	/*long content_len = 0;*/
	/*read_requesthdrs(&rio, method, &content_len);*/
	struct req_hdrs rh = {"", "", "", "", ""};
	read_requesthdrs(&rio, &rh);
	set_def_req_hdrs(&rh);

	if (rh.host[0] == '\0') {	
		char h[MAXLINE];
		get_host(uri, h);
		if (h[0] == '\0') {
			clienterror(fd, buf, "400", "Bad uri", "The uri is not valid");
			return;
		}
		strcpy(rh.host, "Host: ");
		strcat(h, "\r\n");
		strcat(rh.host, h);
	}
	printf("\nmethod: %s\nuri: %s \nversion: %s\n", method, uri, version);
	printf("\n%s%s%s%s%s\n", rh.host, rh.user_agent, rh.connection,
	rh.proxy_connection, rh.others);

	/*request(...);*/
	/*serve(...);*/
}

// read_requesthdrs - read HTTP request headers and fill req_hdrs with
// the request headers
void read_requesthdrs(rio_t *rp, struct req_hdrs * rh) 
{
	char buf[MAXLINE], key[MAXLINE], value[MAXLINE];

	while(strcmp(buf, "\r\n")) {          
		Rio_readlineb(rp, buf, MAXLINE);
		char * i = index(buf, ':');
		int key_len = i ? i - buf : 0;
		// now let's parse headers discarding malformed ones
		if (key_len > 0) {
			strncpy(key, buf, key_len + 1);
			key[key_len+1] = '\0';
			strncpy(value, i+1, MAXLINE);
			if (! strcasecmp(key, "Host:"))
				strcpy(rh->host, buf);
			else if (! strcasecmp(key, "User-Agent:")) 
				strcpy(rh->user_agent, buf);
			else if (! strcasecmp(key, "Connection:")) 
				strcpy(rh->connection, buf);
			else if (! strcasecmp(key, "Proxy-Connection:")) 
				strcpy(rh->proxy_connection, buf);
			else {
				sprintf(rh->others, "%s%s", rh->others, buf);
			}
		}
	}
}

// Extract the host from the input uri and copy it to dest
// Return empty string if not successful
void get_host(char * uri, char * dest)
{
// supported uri types are the following two, with () indicating
// optional elements:
//		1. scheme://(userinfo@)host(:port)(/(path))
//		2. path
// we only consider success if url is of type 1 and we can extract a host
	char * i = strstr(uri, "://");
	if ( (! i) || (i[3] == '\0') ) {
		dest[0] = '\0';
		return;
	}

	char * user = index(i, '@'); 
	char * host = user ? user + 1 : i + 3;

	char * path = index(host, '/');
	if (path) {
		int host_len = path - host;
		strncpy(dest, host, host_len);
		*(dest + host_len) = '\0';
	}
	else
		strcpy(dest, host);

	char * port = index(host, ':');
	if (! port) {
		strcpy(dest + strlen(dest), ":80");
	}
}


/* serve- copy response back to the client */
void serve(int fd, char *filename, int filesize, char *method) 
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
	sprintf(body, "%s<hr><em>Tiny Proxy</em>\r\n", body);

	/* Print the HTTP response */
	sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-type: text/html\r\n");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
	Rio_writen(fd, buf, strlen(buf));
	Rio_writen(fd, body, strlen(body));
}


void test_get_host()
{
	char * in[] = { " http://giulio@www.abc.xyz.com"				,
									 "http://www.abc.xyz.com"								,
									 "http://www.abc.xyz.com:30"						,
									 "http://www.abc.xyz.com/path/path"			,
									 "http://www.abc.xyz.com:30/path/path"	,
									 "www.abc.xyz.com:30/path/path"					,
	};
	char * out[] = {  "www.abc.xyz.com:80"	,
									  "www.abc.xyz.com:80"	,
										"www.abc.xyz.com:30"	,
										"www.abc.xyz.com:80"	,
										"www.abc.xyz.com:30"	,
										""										,
	};
	for (int i = 0; i < 6; ++i) {
		char dest[MAXBUF];
		get_host(in[i], dest);
		/*printf("Test %d -> testing %40s\n"*/
					 /*"          wanting %40s\n"*/
					 /*"          getting %40s\n\n", i, in[i], out[i], dest);*/
		assert(! strcmp(out[i], dest));
	}
}

