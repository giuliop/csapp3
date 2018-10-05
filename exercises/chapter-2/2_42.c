#include <stdint.h>
#include <assert.h>
#include <stdio.h>

/* Write a function div16 that returns the valne x/16 for integer argument x.
 * Your function should not use division, modulus, multiplication, any conditionals
 * (if or ?:),any comparison operators (e.g.,<,>, or==), or any loops.
 * You may assume that data type int is 32 bits long and uses a two's-complement
 * representation, and that right shifts are performed arithmetically.
 */

int32_t div16 (int32_t x) {
	int32_t bias = 15 & (x>>31); 
	int32_t res = (x + bias)>>4;
	/*printf("%d / 16 = %d \n", x, res);*/
	return res;
}

int main () {
	assert(div16(6) == 0);
	assert(div16(16) == 1);
	assert(div16(20) == 1);
	assert(div16(1007) == 62);
	assert(div16(-6) == 0);
	assert(div16(-16) == -1);
	assert(div16(-20) == -1);
	assert(div16(-1007) == -62);
}
