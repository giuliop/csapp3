#include <stdlib.h>
#include <stdio.h>

int main(int argc, char * argv[], char * envp[]) {
	printf("Command-line arguments:\n");
	for (int i = 0; argv[i]; ++i) {
		printf("    argv[%2d]: %s\n", i, argv[i]);
	}
	printf("\n");
	printf("Environment variables:\n");
	for (int i = 0; envp[i]; ++i) {
		printf("    envp[%2d]: %s\n", i, envp[i]);
	}
	return 1;
}
