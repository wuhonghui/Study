#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "algorithm-common.h"
#include "merge_sort.h"

static int merge(int *array, int begin, int mid, int end)
{
	int k = 0;
	int i = begin;
	int j = mid + 1;
	int *newarray = (int *)malloc(sizeof(int) * (end - begin + 1));
	if (!newarray)
		return -1;

	while (i <= mid && j <= end) {
		if (array[i] < array[j])
			newarray[k++] = array[i++];
		else
			newarray[k++] = array[j++];
	}

	while (i <= mid)
		newarray[k++] = array[i++];
	while (j <= end)
		newarray[k++] = array[j++];

	k = 0;
	while (begin <= end)
		array[begin++] = newarray[k++];

	free(newarray);

	return 0;
}

int merge_sort(int *array, int begin, int end)
{
	if (!array)
		return -1;

	if (begin < end) {
		int mid = (begin + end) / 2;
		if (mid > begin) {
			merge_sort(array, begin, mid);
			merge_sort(array, mid + 1, end);
		}
		merge(array, begin, mid, end);
	}

	return 0;
}
