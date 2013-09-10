#include "algorithm-common.h"
#include "bubble_sort.h"

void
bubble_sort(int *array, int start, int end)
{
	int i, j;

	i = start;
	j = end;

	for (i = start; i < end - 1; i++) {
		for (j = end - 1; j > i; j--) {
			if (*(array + j) < *(array + j - 1)) {
				swap(array + j, array + j - 1);
			}
		}
	}
}
