#include "algorithm-common.h"
#include "max_sub_array.h"

int max_sub_array(int *array, int size)
{
	int i = 0;
	int sum = 0;
	int maxsum = 0;

	for (i = 0; i < size; i++) {
		sum += array[i];

		if (sum < 0) {
			sum = 0;
		}
		
		maxsum = sum > maxsum? sum : maxsum;
	}

	return maxsum;
}
