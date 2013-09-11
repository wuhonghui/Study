#include <assert.h>
#include "max_sub_array.h"

int main(int argc, char *argv[])
{
	int a[9] = {-2,3,1,-1,4,-3,2,-1,2};
	assert(7 == max_sub_array(a, 9));

	int b[10] = {1,-2,3,10,-4,7,2,-5, 6};
	assert(19 == max_sub_array(b,9));

	return 0;
}
