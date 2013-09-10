#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "gcd.h"

int 
main(int argc, char *argv[])
{
	if (argc < 4)
		return -1;

	assert(atoi(argv[3]) == gcd(atoi(argv[1]), atoi(argv[2])));

	return 0;
}
