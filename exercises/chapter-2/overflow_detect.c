#include <stdint.h>
#include <limits.h>
#include <assert.h>
#include <stdio.h>

/* Determine whether unsigned int arguments can be added without overflow */
int uadd_ok(unsigned int x, unsigned int y) {
	unsigned int s = x + y;
	return s >= x;
}

/* Determine whether signed int arguments can be added without overflow */
int tadd_ok(int x, int y) {
	int s = x + y;
	return ! ((s >= 0 && x < 0 && y < 0) || (s <= 0 && x > 0 && y > 0));
}

/* Determine whether signed int arguments can be subtracted without overflow*/
int	tsub_ok(int x, int y) {
	if (y == INT_MIN) {
		return x < 0 ? 1 : 0;
	}
	return tadd_ok(x, -y);
}

/* Determine whether arguments can be multiplied without overflow */
int tmult_ok(int x, int y) {
	int p = x*y;
	/* Either x is zero, or dividing p by x gives y */
	return !x || p/x == y;
}

/* For the case where data type int has 32 bits, devise a version of tmult_ok
	 that uses the 64-bit precision of data type int64_t, without using division. */
int tmult_ok_32bit(int32_t x, int32_t y) {
	int64_t p = (int64_t)x*y;
	return p == (int32_t)p;
}

void test_uadd_ok() {
	unsigned x = UINT_MAX - 10;
	assert(uadd_ok(x, 10));
	assert(! uadd_ok(x, 11));
}

void test_tadd_ok() {
	signed x = INT_MAX - 10;
	assert(tadd_ok(x, 10));
	assert(! tadd_ok(x, 11));

	signed y = INT_MIN + 10;
	assert(tadd_ok(y, -10));
	assert(! tadd_ok(y, -11));
	assert(tadd_ok(x, y));
}

void test_tsub_ok() {
	assert(! tsub_ok(0, INT_MIN));
	assert(tsub_ok(-1, INT_MIN));
	assert(tsub_ok(0, INT_MIN + 1));
}

void test_tmult_ok_32bit() {
	int32_t x = 2147483647; // 1<<31 - 1
	int32_t y = -2147483647 - 1; // 1<<32
	assert(tmult_ok_32bit(x, 1));
	assert(! tmult_ok_32bit(1073741824, 2)); // 1073741824 * 2 = 1<<31
	assert(tmult_ok_32bit(y, 1));
	assert(! tmult_ok_32bit(-1073741825, 2)); // -1073741825 * 2 = -1<<31 - 2
}

int main() {
	test_tadd_ok();
	test_uadd_ok();
	test_tsub_ok();
	test_tmult_ok_32bit();
}
