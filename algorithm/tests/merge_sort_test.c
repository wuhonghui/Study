#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "merge_sort.h"

int main(int argc, char *argv[])
{
	int a[] = {7, 2, 1, 5, 9};
	merge_sort(a, 0, 4);
	assert(a[0] == 1);
	assert(a[4] == 9);
	assert(a[2] == 5);
	assert(a[1] == 2);

	int b[] = {3};
	merge_sort(b, 0, 0);
	assert(b[0] == 3);

	int c[] = {22, 33};
	merge_sort(c, 0, 1);
	assert(c[0] == 22);
	assert(c[1] == 33);
	return 0;
}
