#include <stdio.h>

#define POS_INFINITY 1e309
#define NEG_INFINITY -POS_INFINITY
#define NEG_ZERO (-(0.0))
#define NEG_ZERO2 (1.0/NEG_INFINITY)

int main() {
	printf("pos inf %f \n", POS_INFINITY);
	printf("neg inf %f \n", NEG_INFINITY);
	printf("neg zero %f \n", NEG_ZERO);
	printf("neg zero %f \n", NEG_ZERO2);
}

