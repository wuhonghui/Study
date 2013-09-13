#include <string.h>
#include "algorithm-common.h"
#include "string_rotate.h"

static void swap_char(char *a, char *b)
{
	if (a == b)
		return;

	*a ^= *b;
	*b ^= *a;
	*a ^= *b;
}

//right: step 1
int string_rotate(char *str, int step)
{
	if (!str)
		return -1;
	
	int start = 0;
	int end = 0;
	int len = strlen(str);
	
	step = step % len;
	step = (step < 0 ? step + len : step);

	start = 0;
	end = len - step - 1;
	while (start != end && start < end) {
		swap_char(str + start, str + end);
		start++;
		end--;
	}

	start = len - step;
	end = len - 1;
	while (start != end && start < end) {
		swap_char(str + start, str + end);
		start++;
		end--;
	}

	start = 0;
	end = len - 1;
	while (start != end && start < end) {
		swap_char(str + start, str + end);
		start++;
		end--;
	}

	return 0;
}
