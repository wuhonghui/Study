#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "min_edit_distance.h"

int main(int argc, char *argv[])
{
    assert(min_edit_distance("", "sssssss") == 7);
    assert(min_edit_distance("ffffsss", "sssssss") == 8);
    assert(min_edit_distance("ssfsdfsss", "sssssss") == 4);
    return 0;
}
