#include "algorithm-common.h"
#include "top_k.h"

static int adjust_mid(int *array, int left, int right)
{
	while (left < right) {
		while (left < right && array[right] > array[left]) {
			right--;
		}
		if (left < right) {
			swap(array + left, array + right);
		}

		while (left < right && array[left] < array[right]) {
			left++;
		}
		if (left < right) {
			swap(array + left, array + right);
		}
	}

	return left;
}

int top_k(int *array, int size, int k)
{
	int left = 0;
	int right = size - 1;
	int pos = size - k;

	int cur = adjust_mid(array, left, right);
	while (cur != pos) {
		if (cur > pos) {
			right = cur - 1;
			cur = adjust_mid(array, left, right);
		}else if (cur < pos) {
			left = cur + 1;
			cur = adjust_mid(array, left, right);
		}
	}

	return array[cur];
}
