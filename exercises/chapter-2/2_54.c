#include <assert.h>
#include <stdio.h>
#include <float.h>

int B(int x) {
	return x == (int)(float)x;
}

int C(double d) {
	return d == (double)(float)d;
}

int H(float f, double d) {
	return (f+d)-f == d;
}

int main() {
	assert(B((1<<25)));
	assert(!B((1<<25)+1));
	assert(B((1<<24)-1));
	assert(!B((1<<25)-1));

	// fail precision
  assert(C((1<<24)-1.0));
  assert(!C((1<<25)-1.0));
	// fail magnitude
  assert(C(0x1.0p127));
  assert(!C(0x1.0p128));

	float f = 1.0e30;
	double d = 1.0;
	assert(!H(f,d));
}
