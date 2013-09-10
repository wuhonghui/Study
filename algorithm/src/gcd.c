#include "gcd.h"
#include "algorithm-common.h"

int 
gcd(int a, int b)
{
	while (b != 0) {
		if (a > b)
			a = a % b;
		
		swap(&a, &b);
	}

	return a;
}
