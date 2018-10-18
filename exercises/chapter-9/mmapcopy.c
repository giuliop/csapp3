#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>

int mmapcopy(int fd) {
	struct stat buf;
	if (fstat(fd, &buf) == -1) {
		perror("Error with fstat");
		return -1;
	}
	size_t size = buf.st_size;
	void * p = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (p == MAP_FAILED) {
		perror("Error with mmap");
		return -1;
	}
	int written = fwrite(p, size, 1, stdout);
	if (munmap(p, size) == -1) {
		perror("Error with munmap");
		return -1;
	}
	if (written != 1) {
		perror("Error with fwrite");
		return -1;
	}
}

int main(int argc, char * argv[]) {
	if (argc == 1)
		return 1;
	FILE * fp = fopen(argv[1], "r");
	if (! fp) {
		perror("Error with fopen");
		return -1;
	}
	int fd = fileno(fp);
	if (fd == -1) {
		perror("Error with fileno");
		return -1;
	}
	int ret = mmapcopy(fd);
	if (fclose(fp)) {
		perror("Error with fclose");
		return -1;
	}
	return ret;
}
