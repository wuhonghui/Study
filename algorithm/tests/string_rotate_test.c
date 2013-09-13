#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "string_rotate.h"

int main(int argc, char *argv[])
{
	char a[] = "abcdefg";
	string_rotate(a, 0);
	assert(0 == strcmp(a, "abcdefg"));

	string_rotate(a, 3);
	assert(0 == strcmp(a, "efgabcd"));

	string_rotate(a, -3);
	assert(0 == strcmp(a, "abcdefg"));

	string_rotate(a, 10);
	assert(0 == strcmp(a, "efgabcd"));

	string_rotate(a, -10);
	assert(0 == strcmp(a, "abcdefg"));

	return 0;
}
