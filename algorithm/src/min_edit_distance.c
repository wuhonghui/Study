#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "min_edit_distance.h"

static void show(int **M, int m, int n)
{
    int i = 0, j = 0;
    for (i = 0; i < m; i++) {
        for (j = 0; j < n; j++) {
           printf("%d ", M[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

int min_edit_distance(char *a, char *b)
{
    if (a == NULL) {
        if (b == NULL)
            return 0;
        else 
            return strlen(b);
    }
    if (b == NULL) {
        if (a == NULL)
            return 0;
        else
            return strlen(a);
    }

    int lena, lenb;
    int **distance = NULL;
    int i,j,k;

    lena = strlen(a);
    lenb = strlen(b);

    distance = (int **)malloc(sizeof(int) * (lena + 1));
    for (i = 0; i <= lena; i++) {
        distance[i] = malloc(sizeof(int) * (lenb + 1));
    }

    for (i = 0; i <= lena; i++) {
        distance[i][0] = i;
    }
    for (i = 0; i <= lenb; i++) {
        distance[0][i] = i;
    }

    for (i = 1; i <= lena; i++) {
        for (j = 1; j <= lenb; j++) {
            if (a[i-1] == b[j-1]) {
                distance[i][j] = distance[i-1][j-1];
            }else {
                distance[i][j] = distance[i-1][j-1] + 2;
            }
            if (distance[i-1][j] + 1 < distance[i][j])
                distance[i][j] = distance[i-1][j] + 1;
            if (distance[i][j-1] + 1 < distance[i][j])
                distance[i][j] = distance[i][j-1] + 1;
        }
    }

    //show(distance, lena+1, lenb+1);

    k = distance[lena][lenb];

    for (i = 0; i <= lena; i++) {
        free(distance[i]);
    }
    free(distance);

    return k;
}
