#include <limits.h>
#include <assert.h>

/* Determine whether arguments can be added without overflow */
int tadd_ok(signed x, signed y) {
	signed s = x + y;
	return ! ((s >= 0 && x < 0 && y < 0) || (s <= 0 && x > 0 && y > 0));
}

int main() {
	signed x = INT_MAX - 10;
	assert(tadd_ok(x, 10) == 1);
	assert(tadd_ok(x, 11) == 0);

	signed y = INT_MIN + 10;
	assert(tadd_ok(y, -10) == 1);
	assert(tadd_ok(y, -11) == 0);

	assert(tadd_ok(x, y) == 1);
}
