#include <limits.h>
#include <assert.h>

/* Determine whether arguments can be added without overflow */
int uadd_ok(unsigned x, unsigned y) {
	unsigned s = x + y;
	return s >= x;
}

int main() {
	unsigned x = UINT_MAX - 10;
	assert(uadd_ok(x, 10) == 1);
	assert(uadd_ok(x, 11) == 0);
}

