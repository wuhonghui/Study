#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "heap_sort.h"

int main(int argc, char *argv[])
{
	int a[1] = {2};
	heap_sort(a, 0);
	heap_sort(a, 1);
	assert(a[0] == 2);

	int b[2] = {-1,-2};
	heap_sort(b, 2);
	assert(b[0] == -2);
	assert(b[1] == -1);

	int c[12] = {1, 3, 4, 5,6,2,9,8,7,11,10,0};
	heap_sort(c, 12);
	assert(c[10] == 10);
	assert(c[11] == 11);
	assert(c[0] == 0);
	assert(c[4] == 4);

	return 0;
}
