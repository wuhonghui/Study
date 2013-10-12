#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "kmp.h"

int main(int argc, char *argv[])
{
	char *sub;
	int ret;

    assert(0 == kmp("abc", "abcdddd"));
    assert(9 == kmp("ui88", "aui8xxxuiui88"));

	sub = "ababb";
	ret = kmp(sub, "gcatcgcagagagtatacagtacg");
	assert(ret == -1);

	sub = "aaaa";
	ret = kmp(sub, "aaabaaabaaabdcaa");
	assert(ret == -1);

	sub = "fsss";
	ret = kmp(sub, "fssffffssfsfsfsfsfsfsfsss");
	assert(ret = 21);

	sub = "xckxcxcckxck";
	ret = kmp(sub, "soxckkkaxckxcxcckxcks");
	assert(ret == 8);
    return 0;
}
