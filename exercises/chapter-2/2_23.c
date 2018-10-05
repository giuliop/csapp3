#include <stdio.h>
#include <assert.h>
/*#include <stdlib.h>*/


int fun1 (unsigned word) {
	return (int) ((word << 24) >> 24);
}

int fun2 (unsigned word) {
	return ((int) word << 24) >> 24;
}

// assumptions:
// 32-bit program on two's commplement arithmetic
// right shift of signed values   -> arithmetic
// right shift of unsigned values -> logical

int main() {
	unsigned w;

	w = 0x00000076;
	assert(fun1(w) == 0x00000076);
	assert(fun2(w) == 0x00000076);

	w = 0x87654321;
	assert(fun1(w) == 0x00000021);
	assert(fun2(w) == 0x00000021);

	w = 0x000000C9;
	assert(fun1(w) == 0x000000C9);
	assert(fun2(w) == 0xFFFFFFC9);

	w = 0xEDCBA987;
	assert(fun1(w) == 0x00000087);
	assert(fun2(w) == 0xFFFFFF87);
}
