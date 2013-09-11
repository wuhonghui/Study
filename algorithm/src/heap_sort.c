#include <stdlib.h>
#include <string.h>
#include "algorithm-common.h"
#include "heap_sort.h"

static void heap_sort_array_adjust(int *array, int start, int end)
{
	int i = 0;
	for (i = start;
		((i << 1) + 1) < end;
		) {
		int left = (i << 1) + 1;
		int right = (i << 1) + 2;
		int next = i;

		if (array[left] > array[i]) {
			next = left;
		}
		if (right < end && array[right] > array[left]) {
			next = right;
		}
		if (next != i) {
			swap(array + next, array + i);
			i = next;
		}else {
			break;
		}
	}
}

int heap_sort(int *array, int size)
{
	int i = 0;
	for (i = (size - 2) / 2; i >= 0; i--) {
		heap_sort_array_adjust(array, i, size);
	}

	for (i = 1; i < size; i++) {
		swap(array, array + size - i);
		heap_sort_array_adjust(array, 0, size - i);
	}

	return 0;
}
