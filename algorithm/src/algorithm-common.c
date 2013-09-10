#include "algorithm-common.h"

void 
print_int_array(int *array, int size)
{
	int i = 0;
	for(i = 0; i < size; i++) {
		printf("%d ", *(array + i));
	}
	printf("\n");
}

void 
swap(int *a, int *b)
{
	*a ^= *b;
	*b ^= *a;
	*a ^= *b;
}

