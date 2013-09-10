#include <stdio.h>
#include <assert.h>
#include "bubble_sort.h"

int 
main(int argc, char *argv[])
{
	int a[1] = {2};
	bubble_sort(a, 0, 1);
	assert(a[0] == 2);

	int b[2] = {-1,-2};
	bubble_sort(b, 0, 2);
	assert(b[0] == -2);
	assert(b[1] == -1);

	int c[12] = {1, 3, 4, 5,6,2,9,8,7,11,10,0};
	bubble_sort(c, 0, 12);
	assert(c[10] == 10);
	assert(c[11] == 11);
	assert(c[0] == 0);
	assert(c[4] == 4);

	return 0;
}
