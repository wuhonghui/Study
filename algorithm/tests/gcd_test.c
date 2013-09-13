#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "gcd.h"

int 
main(int argc, char *argv[])
{
	assert(3 == gcd(3,6));
	assert(10 == gcd(40, 90));

	return 0;
}
