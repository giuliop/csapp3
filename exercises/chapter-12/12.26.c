#include "csapp.h"

/* 
 * The struct hostent has the following members
 * char *h_name
 *		 This is the “official” name of the host.
 *
 * char **h_aliases
 *		 Alternative names, in as a null-terminated vector of strings.
 *
 * int h_addrtype
 *		 The address type; in practice, always either AF_INET or AF_INET6
 *
 * int h_length
 *		 This is the length, in bytes, of each address.
 *
 * char **h_addr_list
 *		 Vector of addresses for the host, terminated by a null pointer.
 *
 */

sem_t mutex;

void init() {
	Sem_init(&mutex, 0, 1);
}

// Allocate and return pointer to deep copied hostend, or NULL if error
struct hostent * hostent_deepcopy(struct hostent * from) {

	struct hostent * to = malloc(sizeof(struct hostent));

	// copy char * h_name
	to->h_name = strdup(from->h_name);
	if (! to->h_name)
		return NULL;

	// copy char ** h_aliases
	int h_aliases_len = 0;
	while (from->h_aliases[h_aliases_len])
		++h_aliases_len;

	to->h_aliases = malloc((h_aliases_len + 1) * sizeof(char *));
	to->h_aliases[h_aliases_len] = '\0';

	for(int i = 0; from->h_aliases[i]; ++i) {
		to->h_aliases[i] = strdup(from->h_aliases[i]);
		if (! to->h_aliases[i])
			return NULL;
	}

	// copy int h_addrtype
	to->h_addrtype = from->h_addrtype;

	// copy int h_length
	to->h_length = from->h_length;

	// copy char ** h_addr_list
	int h_addr_list_len = 0;
	while (from->h_addr_list[h_addr_list_len])
		++h_addr_list_len;

	to->h_addr_list = malloc((h_addr_list_len + 1) * sizeof(char *));
	to->h_addr_list[h_addr_list_len] = '\0';

	for(int i = 0; i < h_addr_list_len; ++i) {
		to->h_addr_list[i] = strdup(from->h_addr_list[i]);
		if (! to->h_addr_list[i])
			return NULL;
	}

	return to;
}

// free the hostent and its members
void free_hostent(struct hostent * h) {
	free(h->h_name);
	for(int i = 0; h->h_aliases[i]; ++i) {
		free(h->h_aliases[i]);
	}
	free(h->h_aliases);
	for(int i = 0; h->h_addr_list[i]; ++i) {
		free(h->h_addr_list[i]);
	}
	free(h->h_addr_list);
	free(h);
}

// thread safe version of gethostbyname implemented with lock and copy
struct hostent * gethostbyname_ts(const char * hostname) {

	struct hostent * h;

	P(&mutex);
	h = gethostbyname(hostname);
	h = hostent_deepcopy(h);
	V(&mutex);

	return h;
}

// print an hostent struct
void print_hostent(char * hostname, struct hostent * h) {
	char buf[MAXBUF];
	printf("\n-----\n"
			"Looking up hostname %s\n\n"
			"Official name: %s\n"
			"Address(es): ",
			hostname, h->h_name);
	char * padding = "";
	for (int i = 0; h->h_addr_list[i]; ++i) {
		const char * ok = inet_ntop(AF_INET, h->h_addr_list[i], buf, MAXBUF);
		if (ok)
			printf("%s%s\n", padding, buf);
		padding = "             ";
	}
	printf("\n");
}

// f is the thread function: gethostbyname and print its address(es)
void * f(void * v) {
	char * hostname = (char *)v;
	struct hostent * h = gethostbyname_ts(hostname);
	if (! h) {
		printf("Error looking up %s\n", hostname);
		exit(0);
	}
	print_hostent(hostname, h);
	free_hostent(h);
	return NULL;
}

int main() {
	init();
	pthread_t tid;
	int n = 5;
	const char * domains[] = {
		"google.com" ,
		"twitter.com" ,
		"yahoo.com" ,
		"apple.com" ,
		"giuliopizzini.com"
	};

	for (int i = 0; i < n; ++i) {
		Pthread_create(&tid, NULL, f, (void*)(domains[i]));
	}

	int ret = 1;
	pthread_exit(&ret);
}
