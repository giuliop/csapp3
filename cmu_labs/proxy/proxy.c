// a simple, threaded web proxy
#include "csapp.h"
#include <string.h>
#include <assert.h>
#include "cache.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define CACHE_HASHCOUNT 100

struct req_hdrs {
	char host[MAXLINE];
	char user_agent[MAXLINE];
	char connection[MAXLINE];
	char proxy_connection[MAXLINE];
	char cache_control[MAXLINE];
	char others[MAXLINE];
};

struct http10request {
	char method[MAXLINE];
	char host[MAXLINE];
	char port[MAXLINE];
	char path[MAXLINE];
	char version[MAXLINE];
	char req_line[MAXLINE];
	char post_arg[MAXLINE];
	struct req_hdrs * hdrs;
};

/* 
 * Cache readers access freely incrementing the cache_readers var until they find
 * a writer in line (cache_writer = 1), then they wait for cache_writer_cond

 * Cache writers notify their presence setting cache_writer to 1 (or wait for
 * cache_writer_cond if already 1) then wait for cache_readers to go to 0
 * (waiting for cache_readers_cond)
 */
static pthread_mutex_t cache_readers_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cache_no_readers_cond = PTHREAD_COND_INITIALIZER;
static long cache_readers = 0;
static pthread_mutex_t cache_writer_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cache_no_writer_cond = PTHREAD_COND_INITIALIZER;
static long cache_writer = 0;
static int cache_enabled = 0;

void set_def_req_field(struct http10request * r) {
	strcpy (r->version, "HTTP/1.0");
	strcpy(r->hdrs->user_agent, "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n");
	strcpy(r->hdrs->connection, "Connection: close\r\n");
	strcpy(r->hdrs->proxy_connection, "Proxy_connection: close\r\n");
	strcpy(r->hdrs->cache_control, "Cache-Control: no-cache, no-store, must-revalidate\r\n");
}

void doit(int fd);
void read_requesthdrs(rio_t * rp, struct req_hdrs * rh);
void read_post_params(rio_t * rio, struct http10request * req);
int send_request(struct http10request * r, int clientfd);
void parse_uri(char * uri, char * host, char * port, char * path);
void clienterror(int fd, char * cause, char *errnum, 
		char * shortmsg, char *longmsg);
void * thread_f (void * x);
void test();

// wrappers to make cache thread-safe
int try_cache(char * resp, long * resp_size, char * key);
void add_to_cache(char * key, char * resp, long read);


int main(int argc, char **argv) 
{
	/*test();*/

	/* Check command line args */
	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}

	// Init the cache
	if (! cache_init(MAX_CACHE_SIZE, CACHE_HASHCOUNT)) {
		fprintf(stderr, "Cannot init cache\n");
		exit(1);
	} else
		cache_enabled = 1;

	// disable SIGPIPE to avoid server terminating on writes to closed sockets
	signal(SIGPIPE, SIG_IGN);

	char hostname[MAXLINE], port[MAXLINE];
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;
	pthread_t tid;
	int connfd;

	int listenfd = Open_listenfd(argv[1]);
	while (1) {
		clientlen = sizeof(clientaddr);
		if ((connfd = accept(listenfd, (SA *)&clientaddr, &clientlen)) < 0) {
			perror("\naccept error");
			continue;
		}
		int s;
		if ((s = getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
					port, MAXLINE, 0)) != 0) { 
			fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));
			continue;
		}
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

	if (close(connfd) == -1) 
		perror("close error");

	return NULL;
}

// doit - handle one HTTP request/response transaction
void doit(int fd) 
{
	/* Read request line and headers */
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	rio_t rio;

	rio_readinitb(&rio, fd);
	if (rio_readlineb(&rio, buf, MAXLINE) == -1) {
		perror("rio_readlineb error");
		return;
	}

	printf("%s", buf);
	sscanf(buf, "%s %s %s", method, uri, version);       
	if ((strcasecmp(method, "HEAD")) &&
			(strcasecmp(method, "GET")) &&
			(strcasecmp(method, "POST"))) {                     
		clienterror(fd, method, "501", "Not Implemented",
				"Tiny Proxy does not implement this method");
		return;
	}

	struct req_hdrs rh = { "", "", "", "", "", "" };
	struct http10request req = { "", "", "", "", "", "", "", &rh };
	read_requesthdrs(&rio, &rh);
	strcpy(req.method, method);
	set_def_req_field(&req);

	parse_uri(uri, req.host, req.port, req.path);
	if (rh.host[0] == '\0') {	
		if (req.host[0] == '\0') {
			clienterror(fd, buf, "400", "Bad uri", "Cannot determine valid host");
			return;
		}
		int n = snprintf(rh.host, MAXLINE, "Host: %s:%s\r\n", req.host, req.port);
		if (n >= MAXLINE) {
			clienterror(fd, buf, "400", "Bad host", "Host name is too long");
			return;
		}
	}
	if (req.path[0] == '\0') {	
			clienterror(fd, buf, "400", "Bad uri", "Cannot determine valid path");
			return;
	}
	int n = snprintf(req.req_line, MAXLINE, "%s %s %s\r\n",
			req.method, req.path, req.version);
	if (n >= MAXLINE) {
		clienterror(fd, buf, "400", "Bad request", "Request line is too long");
		return;
	}

	/*printf("\nRequest line: %s", req.req_line);*/
	/*printf("\nRequest headers:\n%s%s%s%s%s\n", rh.host, rh.user_agent,*/
			/*rh.connection, rh.proxy_connection, rh.others);*/

	if (! strcmp(method, "POST"))
			read_post_params(&rio, &req);

	// try the cache before contacting the server
	long keylen = strlen(req.host) + strlen(":") + strlen(req.port)
		+ strlen(req.path) + 1;
	char key[keylen];
	snprintf(key, keylen, "%s%s%s%s", req.host, ":", req.port, req.path);

	char resp[MAX_OBJECT_SIZE] = { 0 };
	long resp_size = 0;
	int cached = try_cache(resp, &resp_size, key);
	if (cached) {
		if (rio_writen(fd, resp, resp_size) == -1)
			perror("rio_writen error");
		return;
	}

	// If not cached send the request to the server
	int clientfd = open_clientfd(req.host, req.port);
	if (clientfd < 0) {
		clienterror(fd, buf, "400", "Bad request", "Cannot contact server");
		return;
	}
	if (! send_request(&req, clientfd)) {
		if (close(clientfd) == -1) 
			perror("close error");
		clienterror(fd, buf, "400", "Bad request", "Cannot contact server");
		return;
	}

// Read response from server, cache if not too large and send to the client
	long read, written;
	rio_readinitb(&rio, clientfd);
	while ((read = rio_readnb(&rio, resp, MAX_OBJECT_SIZE)) > 0) {
		int first_chunck = 1;
		// if we can't write to client we stop writing
		if ((written = rio_writen(fd, resp, read)) < read)
			break;
		// if the response is small enough we cache it
		if (first_chunck && (read < MAX_OBJECT_SIZE)) {
			printf("\nAdding to cache <%s>\nWith value %s\n", key, resp);
			add_to_cache(key, resp, read);
		}
		first_chunck = 0;
	}
	if (read == -1)
		perror("rio_readnb error");
	if (written == -1)
		perror("rio_writen error");
		
	if (close(clientfd) == -1) 
		perror("close error");
}

// send the request to the server, return 1 if success, 0 otherwise
int send_request(struct http10request * r, int clientfd) 
{
	char * lines[] = {r->req_line ,
									  r->hdrs->host ,
										r->hdrs->user_agent ,
										r->hdrs->connection ,
										r->hdrs->proxy_connection ,
										r->hdrs->others ,
										"\r\n" ,
	};
	
	for (int i = 0; i < (sizeof(lines) / sizeof (char *)); ++i) {
		int len = strlen(lines[i]);
		if (rio_writen(clientfd, lines[i], len) < len)
			return 0;
	}

	if (! strcmp (r->method, "POST")) {
		int len = strlen(r->post_arg);
		if (rio_writen(clientfd, r->post_arg, len) < len)
			return 0;
	}

	return 1;
}

// read_requesthdrs - read HTTP request headers and fill req_hdrs with
// the request headers
void read_requesthdrs(rio_t *rp, struct req_hdrs * rh) 
{
	char buf[MAXLINE], key[MAXLINE], value[MAXLINE];
	buf[0] = key[0] = value[0] = '\0';

	while(strcmp(buf, "\r\n")) {          
		if (rio_readlineb(rp, buf, MAXLINE) == -1) {
			perror("rio_readlineb error");
			return;
	};
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
			else if (! strcasecmp(key, "Cache-Control:")) 
				strcpy(rh->cache_control, buf);
			else {
				sprintf(rh->others, "%s%s", rh->others, buf);
			}
		}
	}
}

// Read the request body of POST requests and save it in the input req struct
void read_post_params(rio_t * rio, struct http10request * req)
{
	char * i = strstr(req->hdrs->others, "Content-Length");
	if (! i)
		return;
	i += strlen("Content-Length: ");
	char * j = index(i, '\r');
	if (! j)
		return;
	char len[MAXLINE];
	memcpy(len, i, j-i);
	len[j-i] = '\0';
	int content_len = strtol(len, NULL, 10);
	if (rio_readnb(rio, req->post_arg, content_len) == -1)
		perror("rio_readnb error");
}

// Extract the host and path from the input uri and copy it to the
// input pointers; return empty strings if not successful
void parse_uri(char * uri, char * host, char * port, char * path)
{
// supported uri types are the following two, with () indicating
// optional elements:
//		1. scheme://(userinfo@)host(:port)(/(path))
//		2. path
	char * scheme_i = strstr(uri, ":");
	char * host_i = strstr(uri, "://");
	if (! host_i) {
		host[0] = '\0';
		port[0] = '\0';
		if (! scheme_i)
			strncpy(path, uri, MAXLINE);
		else
			strncpy(path, scheme_i + 1, MAXLINE);
		return;
	}

	char * user_i = index(host_i, '@'); 
	host_i = user_i ? user_i + 1 : host_i + 3;

	char * path_i = index(host_i, '/');
	if (path_i) {
		int host_len = path_i - host_i;
		strncpy(host, host_i, host_len);
		*(host + host_len) = '\0';
		strncpy(path, path_i, MAXLINE);
	}
	else {
		strncpy(host, host_i, MAXLINE);
		strcpy(path, "/");
	}

	char * port_i = index(host, ':');
	if (! port_i)
		strcpy(port, "80");
	else {
		strncpy(port, port_i + 1, MAXLINE);
		*port_i = '\0';
	}
}

// thread safe cache lookup
// if successful return 1 and set resp and resp_size
// if unsuccessful return 0
int try_cache(char * resp, long * resp_size, char * key)
{
	if (! cache_enabled) {
		printf("Cache is disabled\n");
		return 0;
	}
	int err;

	// if writer waiting let's give him precedence
	if ((err = pthread_mutex_lock(&cache_writer_mtx)))
		goto out;
	while (cache_writer == 1)
		pthread_cond_wait(&cache_no_writer_cond, &cache_writer_mtx);
	if ((err = pthread_mutex_unlock(&cache_writer_mtx)))
		goto out;

	// notify we are reading
	if ((err = pthread_mutex_lock(&cache_readers_mtx)))
		goto out;
	++cache_readers;
	if ((err = pthread_mutex_unlock(&cache_readers_mtx)))
		goto out;

	struct item * x = cache_lookup(key);
	if (x) {
		*resp_size = x->size;
		memcpy(resp, x->val, x->size);
	}

	// notify we are done reading
	if ((err = pthread_mutex_lock(&cache_readers_mtx)))
		goto out;

	--cache_readers;
	// if no other reader notify that too
	if (! cache_readers)
		pthread_cond_signal(&cache_no_readers_cond);

	if ((err = pthread_mutex_unlock(&cache_readers_mtx)))
		goto out;

	if (x)
		return 1;
	else
		return 0;

out:
	perror("try_cache error");
	printf("Disabling cache now\n");
	cache_enabled = 0;
	return 0;
}

// insert req in the cache, thread-safe
void add_to_cache(char * key, char * resp, long read)
{
	if (! cache_enabled) {
		printf("Cache is disabled\n");
		return;
	}
	int err;

	// notify we are wating to write
	if ((err = pthread_mutex_lock(&cache_writer_mtx)))
		goto out;
	while (cache_writer == 1)
		pthread_cond_wait(&cache_no_writer_cond, &cache_writer_mtx);

	++cache_writer;
	if ((err = pthread_mutex_unlock(&cache_writer_mtx)))
		goto out;

	// wait for all current readers to get out and then write
	if ((err = pthread_mutex_lock(&cache_readers_mtx)))
		goto out;

	while (cache_readers > 0)
		pthread_cond_wait(&cache_no_readers_cond, &cache_readers_mtx);

	cache_insert(key, resp, read);

	if ((err = pthread_mutex_unlock(&cache_readers_mtx)))
		goto out;

	// Signal we are done writing
	if ((err = pthread_mutex_lock(&cache_writer_mtx)))
		goto out;
	--cache_writer;
	if ((err = pthread_mutex_unlock(&cache_writer_mtx)))
		goto out;

	pthread_cond_signal(&cache_no_writer_cond);

	return;

out:
	perror("add_to_cache error");
	printf("Disabling cache now\n");
	cache_enabled = 0;
}

/* clienterror - returns an error message to the client */
void clienterror(int fd, char *cause, char *errnum, 
		char *shortmsg, char *longmsg) 
{
	char buf[MAXBUF], body[MAXBUF];

	/* Build the HTTP response body */
	sprintf(body, "<html><title>Tiny Error</title>");
	sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
	sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
	sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
	sprintf(body, "%s<hr><em>Tiny Proxy</em>\r\n", body);

	/* Build  the HTTP response header */
	sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
	sprintf(buf, "Content-type: text/html\r\n");
	sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
	
	// Print the response
	int len = strlen(buf);
	if (rio_writen(fd, buf, len) < len)
		return;
	rio_writen(fd, body, strlen(body));
}


void test_parse_uri()
{
	char * uris[] = { "http://giulio@www.abc.xyz.com"				,
									 "http://www.abc.xyz.com"								,
									 "http://www.abc.xyz.com:30"						,
									 "http://www.abc.xyz.com/path/path"			,
									 "http://www.abc.xyz.com:30/path/path"	,
									 "www.abc.xyz.com:30/path/path"					,
									 "path@/path"														,
	};
	char * hosts[] = {"www.abc.xyz.com"	,
									  "www.abc.xyz.com"	,
										"www.abc.xyz.com"	,
										"www.abc.xyz.com"	,
										"www.abc.xyz.com"	,
										""										,
										""										,
	};
	char * ports[] = {"80"	,
									  "80"	,
										"30"	,
										"80"	,
										"30"	,
										""		,
										""		,
	};
	char * paths[] = { "/"							,
									   "/"							,
										 "/"							,
										 "/path/path"			,
										 "/path/path"			,
										 "30/path/path"		,
										 "path@/path"			,
	};
	for (int i = 0; i < 7; ++i) {
		char host[MAXBUF];
		char path[MAXBUF];
		char port[MAXLINE];
		parse_uri(uris[i], host, port, path);
		/*printf("Test %d -> testing uri %40s\n"*/
				/*"          wanting host%40s\n"*/
				/*"          getting host%40s\n"*/
				/*"          wanting port%40s\n"*/
				/*"          getting port%40s\n"*/
				/*"          wanting path%40s\n"*/
				/*"          getting path%40s\n\n",*/
				/*i+1, uris[i], hosts[i], host, ports[i], port, paths[i], path);*/
		assert(! strcmp(hosts[i], host));
		assert(! strcmp(ports[i], port));
		assert(! strcmp(paths[i], path));
	}
}

void test()
{
	test_parse_uri();
}

