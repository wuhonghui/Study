#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "top_k.h"

int 
main(int argc, char *argv[])
{
	int a[10] = {5,6,8,7,9,0,4,1,2,3};
	assert(7 == top_k(a, 10, 3));
	assert(9 == top_k(a, 10, 1));
	assert(0 == top_k(a, 10, 10));

	int b[1] = {2};
	assert(2 == top_k(b, 1, 1));

	int c[20] = {99, 22, 66, 88,11, 32, 21, 89, 1111, 6666, 808};
	assert(11 == top_k(c, 11, 11));
	assert(6666 == top_k(c, 11, 1));
	return 0;
}
