/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function. 
     The max operator count is checked by dlc. Note that '=' is not 
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
/* 
 * bitAnd - x&y using only ~ and | 
 *   Example: bitAnd(6, 5) = 4
 *   Legal ops: ~ |
 *   Max ops: 8
 *   Rating: 1
 */
int bitAnd(int x, int y) {
	/* We do x AND y as NOT ((NOT x) OR (NOT y)) */
	x = ~x;
	y = ~y;
  return ~(x|y);
}
/* 
 * getByte - Extract byte n from word x
 *   Bytes numbered from 0 (LSB) to 3 (MSB)
 *   Examples: getByte(0x12345678,1) = 0x56
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 2
 */
int getByte(int x, int n) {
	/* We make n as n*8 to have the bits to right shift x to put the wanted byte
	 * as first byte that we then extract with AND and the mask 0xff */
	n += n;
	n += n;
	n += n;
	x >>= n;
	return (x & 0xff);
}
/* 
 * logicalShift - shift x to the right by n, using a logical shift
 *   Can assume that 0 <= n <= 31
 *   Examples: logicalShift(0x87654321,4) = 0x08765432
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3 
 */
int logicalShift(int x, int n) {
	/* We extract the sign bit, replace it with 0, right shift the number as as logical
	 * shift since the sign bit is zero, create a mask as 0..0x0..0 where x is the
	 * sign bit so we can reset it to the original value with an OR */
	int mask = (x>>31) & 1;	// sign bit of x
	mask <<= (31+(~n+1)); // put sign bit in position after right shift
	x &= ~(1<<31); // set sign bit to zero
	x >>= n; // perform shift as if logical
	return x|mask;
}
/*
 * bitCount - returns count of number of 1's in word
 *   Examples: bitCount(5) = 2, bitCount(7) = 3
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 40
 *   Rating: 4
 */
int bitCount(int x) {
	/* Implementation of Hamming weigth algorithm. We first sum every two
	 * bits together by adding x to its right shift by one each masked by 010101..
	 * so that we clear every other bit, and so on until we sum all bits
	 */
	int mask = 0x55;									//													  01010101
	mask = mask + (mask<<8);					//									 01010101 01010101
	mask = mask + (mask<<16);					// 01010101 01010101 01010101 01010101 
	x = (x & mask) + ((x>>1) & mask);
	mask = 0x33;											//														00110011
	mask = mask + (mask<<8);					//									 00110011 00110011
	mask = mask + (mask<<16);					// 00110011 00110011 00110011 00110011 
	x = (x & mask) + ((x>>2) & mask);
	mask = 0x0f;											//														00001111
	mask = mask + (mask<<8);					//									 00001111 00001111
	mask = mask + (mask<<16);					// 00001111 00001111 00001111 00001111  
	x = (x & mask) + ((x>>4) & mask);
	x += x>>8;
	x += x>>16;
	mask = 0xff;
  return x & mask;
}
/* 
 * bang - Compute !x without using !
 *   Examples: bang(3) = 0, bang(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int bang(int x) {
	/* The idea is to OR all bits among themselves, and negate the resulting bit.
	 * We of it by right shifting x by half each time (16 then 8, 4, 2, 1) and ORing
	 * it with itself. Then we negate and return the leasts significant bit
	 */
	int x1 = x>>16;
	x |= x1;
	x1 = x>>8;
	x |= x1;
	x1 = x>>4;
	x |= x1;
	x1 = x>>2;
	x |= x1;
	x1 = x>>1;
	x |= x1;
	return (~x) & 1;
}
/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {
  return 1<<31;
}
/* 
 * fitsBits - return 1 if x can be represented as an 
 *  n-bit, two's complement integer.
 *   1 <= n <= 32
 *   Examples: fitsBits(5,3) = 0, fitsBits(-4,3) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
int fitsBits(int x, int n) {
	/* Positive numbers can be represented if have no ones after the n-1 least
	 * signtificative bits; negative ones if no zeroes.
	 * By right shiftinf x by n-1 and adding the sign bit we get all 0s if
	 * the number is representable, so we just return its negation
	 */
	x >>= n + (~1+1);
	x += (x>>31) & 1;
  return !x;
}
/* 
 * divpwr2 - Compute x/(2^n), for 0 <= n <= 30
 *  Round toward zero
 *   Examples: divpwr2(15,1) = 7, divpwr2(-33,4) = -2
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
int divpwr2(int x, int n) {
	/* For positive numbers right shifting already rounds towards zero
	 * For negative ones we need to add 1 if the number is not a multiple
	 * of 2^n that is if it has 1s in the least significative n bits
	 */
	int mask = ~(~0 << n);			// n 1s
	int round = !!(x & mask);
	x >>= n;
	x += (x>>31) & round;
	return x;
}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
	/* In two complement -x = ~x+1 */
  return (~x+1);
}
/* 
 * isPositive - return 1 if x > 0, return 0 otherwise 
 *   Example: isPositive(-1) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 8
 *   Rating: 3
 */
int isPositive(int x) {
	/* x is positive if its sign bit is zero and it is not zero */
	int sign = (x>>31) & 1;
  return !sign & !!x;		// & !!x to account for the case x = 0
}
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
	/* x is less or equal in one of these cases:
	 * 1. x is neg and y is pos
	 * 2. x and y are equal
	 * 3. x and y have same sign and their difference is negative
	 */
	int delta = x + (~y+1); // could overlfow if x and y different sign
	int x_neg = (x>>31) & 1;
	int y_neg = (y>>31) & 1;
	int delta_neg = (delta>>31) & 1;
	return (x_neg & !y_neg) | !delta | ((!(x_neg^y_neg)) & delta_neg); 
}
/*
 * ilog2 - return floor(log base 2 of x), where x > 0
 *   Example: ilog2(16) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 90
 *   Rating: 4
 */
int ilog2(int x) {
	/* The answer is 31 - the number of leading 0 of x (how many times we can right
	 * shift x, or divide it by 2). To compute it we propagate the most significant
	 * 1 to the right by ORing x with x right shifted by 1, 2, 4...
	 * Then we can count 1s with the Hamming distance algo and subtract 1
	 * which is the same as 31 - leading 0s
	*/
	int mask = 0x55;

	x = x | x>>1;
	x = x | x>>2;
	x = x | x>>4;
	x = x | x>>8;
	x = x | x>>16;
	
	mask = mask + (mask<<8);
	mask = mask + (mask<<16);
	x = (x & mask) + ((x>>1) & mask);
	mask = 0x33;
	mask = mask + (mask<<8);
	mask = mask + (mask<<16);
	x = (x & mask) + ((x>>2) & mask);
	mask = 0x0f;
	mask = mask + (mask<<8);
	mask = mask + (mask<<16);
	x = (x & mask) + ((x>>4) & mask);
	x += x>>8;
	x += x>>16;
	mask = 0xff;
  x &= mask;

  return x + (~1+1);
}
/* 
 * float_neg - Return bit-level equivalent of expression -f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representations of
 *   single-precision floating point values.
 *   When argument is NaN, return argument.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 10
 *   Rating: 2
 */
unsigned float_neg(unsigned uf) {
	/* We recognize NaN by checking if the exp is all 1s and the frac field is not
	 * all 0s. If not NaN we simply toggle the sign bit
	 */
	int exp = (uf>>23) & 0xff;
	int frac = uf & 0x7fffff;
	if ((exp == 0xff) && frac) {
		return uf;
	}
	return uf ^ (1<<31);
}
/* 
 * float_i2f - Return bit-level equivalent of expression (float) x
 *   Result is returned as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point values.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_i2f(int x) {
	/* We construct the float as follows:
	 * 1. Sign: we simply extract the sign from int; if it is negative we transform
	 *	  x in positive
	 * 2. Exp: 127 (bias) + position of most significant bit of x (except sign bit)
	 *	  where the least signigicant bit is position 0; potentially + 1 if rounding
	 *	  happen (see below)
	 * 3. Frac: the (up to) 23 most significant bits of x (after the sign bit and
	 *    the initial 0s, and dropping the leading 1); potentially padded by 0
	 *    to the right to get 23 bits; potentially + 1 if rounding happen (see below)
	 * 4. Rounding: if x has more than 23 significants bits (excluding sign bit) we
	 *	  need to round. There are three cases based on the pattern of the extra bits:
	 *			a. 0x..x -> round-down;
	 *			b. 1x1x. -> round-up: frac++ and if frac was all 1s, exp++ and frac = 0
	 *			c. 100.0 -> if last significant bit we keep is 0 round-donw, if 1 round-up
	 * 5. Corner cases are x = 0 (return x) and x = INT_MIN (return 0xcf000000)
	 */
	int res = 0;
	int n = 30; // n will hold position of most significative bit
	int exp = 127; // bias
	int frac = 0;
	int bits_lost = 0;
	int round = 0; // 1 round; 0 no round
	int round_up = 0; // 1 round-up; 0 round-donw
	if (!x) return x;
	if (x == 0x80000000) return 0xcf000000;
	if (x<0) {
		x = -x;
		res = 0x80000000;
	}
	while (!(x>>n)) --n;
	round = n > 23;
	exp += n;
	x <<= (31-n); // remove 0s
	frac = (x>>8) & 0x7fffff;
	res = res | (exp<<23) | frac;
	if (round) {
		bits_lost = x & 0xff;
		round_up = (bits_lost > 128) ||
			((bits_lost == 128) && (frac & 1));
		if (round_up) {
			++res;
		}
	} 
	return res;
}
/* 
 * float_twice - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_twice(unsigned uf) {
	/* We have the following cases:
	 * 1. exp is 0xff return uf (infinity or NaN)
	 * 2. exp is > 0 (normalized) -> increase exp by 1;
	 *		if exp == 0xff then frac = 0 (infniity)
	 * 3. exp is 0 (denormalized) -> if frac == 0 return uf (+0 or -0);
	 *		else shift frac left by 1; if MSB of frac was 1 then exp = 1
	 */
	unsigned sign = uf & 0x80000000;
	unsigned exp = (uf>>23) & 0xff;	
	unsigned frac = uf & 0x7fffff;
	if (exp == 0xff) return uf;
	if (exp > 0) {
		++exp;
		if (exp == 0xff) frac = 0;
	} else {
		if (frac == 0) return uf;
		if (frac>>22) exp = 1;
		frac <<= 1;
	}
	return sign | (exp<<23) | frac;
}

