#include <string.h>
#include <stdlib.h>
#include "kmp.h"

int kmp(char *sub, char *src)
{
    int index = 0;
    int i, j;
    int sub_len, src_len;
    int *next = NULL;

    sub_len = strlen(sub);
    src_len = strlen(src);

    next = (int *)malloc((sub_len + 1) * sizeof(int));
    next[0] = -1;
    i = 0;
    j = -1;
    while (i < sub_len) {
        while (j > -1 && sub[i] != sub[j]) 
            j = next[j];
        i++;
        j++;
        if (sub[i] == sub[j])
            next[i] = next[j];
        else
            next[i] = j;
    }
    next[i] = 1;

    i = j = 0;
    index = 0;
    while (i < src_len && j < sub_len) {
        if (j == -1 ||src[i] == sub[j]) {
            i++;
            j++;
        } else {
            j = next[j];
        }
    }

    if (j == sub_len) {
        index = i - sub_len;
    } else {
        index = -1;
    }

    free(next);

    return index;
}
