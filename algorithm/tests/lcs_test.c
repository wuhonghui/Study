#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "lcs.h"

int 
main(int argc, char *argv[])
{
	char *common = NULL;
	int common_len = 0;

	common_len = longest_common_substring("xckksoas", "soxckkkas", &common);
	assert(strcmp(common, "xckk") == 0);

	common_len = longest_common_substring("ABAB", "BABA", &common);
	assert(strcmp(common, "BAB") == 0 || strcmp(common, "ABA") == 0);
	
	common_len = longest_common_substring("uiffdiffaaaccccccc", "fdddfaaaccciiidcac", &common);
	assert(strcmp(common, "faaaccc") == 0);

	common_len = longest_common_sequence("iueeeadf", "ieafueedf", &common);
	assert(strcmp(common, "iueedf") == 0 || strcmp(common, "ieeedf") == 0);

	common_len = longest_common_sequence("1234567890", "q1t2b3jjjj45ii67psdfafjlkj8;kzjxocijvp9;alskdjfk0", &common);
	assert(strcmp(common, "1234567890") == 0);

	return 0;
}
