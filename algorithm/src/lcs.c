#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "lcs.h"

static void show_matrix(int **M, int m, int n)
{
	int i = 0;
	int j = 0;
	for (i = 0; i < m; i++) {
		for (j = 0; j < n; j++) {
			printf("%2d ", M[i][j]);
		}
		printf("\n");
	}
	printf("\n");
}

int longest_common_substring(char *a, char *b, char **c)
{
	int i = 0;
	int j = 0;
	int len_a = 0;
	int len_b = 0;
	int *M = NULL;
	int max = 0;
	char *max_end = NULL;

	assert(a != NULL && b != NULL);
	
	len_a = strlen(a);
	len_b = strlen(b);
	M = (int *)malloc((len_a + 1) * sizeof(int *));

	/* init */
	for (i = 0; i <= len_a; ++i) {
		M[i] = 0;
	}

	/* */
	for (i = 1; i <= len_b; i++) {
		for (j = len_a; j >= 1; j--) {
			if (b[i-1] == a[j-1]) {
				M[j] = M[j-1] + 1;
				if (M[j] > max) {
					max = M[j];
					max_end = a + j - 1;
				}
			} else {
				M[j] = 0;
			}
		}
	}

	free(M);

	*c = strndup(max_end - max + 1, max);

	return max;
}

#define DIAG (0)
#define UP (1)
#define LEFT (-1)
int longest_common_sequence(char *a, char *b, char **c)
{
	int **M = NULL;
	int **D = NULL;
	int lena;
	int lenb;
	int i, j;
	char *result = NULL;
	int result_i;

	assert(a && b);
	lena = strlen(a);
	lenb = strlen(b);

	M = (int **)malloc(sizeof(int) * (lena + 1));
	for (i = 0; i <= lena; i++) {
		M[i] = (int *)malloc(sizeof(int) * (lenb + 1));
	}
	for (i = 0; i <= lena; i++) {
		for (j = 0; j <= lenb; j++) {
			M[i][j] = 0;
		}
	}

	D = (int **)malloc(sizeof(int) * (lena + 1));
	for (i = 0; i <= lena; i++) {
		D[i] = (int *)malloc(sizeof(int) * (lenb + 1));
	}
	for (i = 0; i <= lena; i++) {
		for (j = 0; j <= lenb; j++) {
			D[i][j] = 0;
		}
	}

	for (i = 1; i <= lena; i++) {
		for (j = 1; j <= lenb; j++) {
			if (a[i-1] == b[j-1]) {
				M[i][j] = M[i-1][j-1] + 1;
				D[i][j] = DIAG;
			} else if (M[i-1][j] > M[i][j-1]) {
				M[i][j] = M[i-1][j];
				D[i][j] = UP;
			} else {
				M[i][j] = M[i][j-1];
				D[i][j] = LEFT;
			}
		}
	}

	show_matrix(M, lena+1, lenb+1);
	show_matrix(D, lena+1, lenb+1);

	result = malloc(sizeof(char) * lena + 1);
	
	i = lena;
	j = lenb;
	result_i = 0;
	while (i > 0 && j > 0) {
		if (D[i][j] == DIAG) {
			result[result_i++] = a[i-1];
			i--;
			j--;
		} else if (D[i][j] == LEFT) {
			j--;
		} else {
			i--;
		}
	}
	result[result_i] = '\0';

	char *begin = result;
	char *end = result + result_i - 1;
	while (begin < end) {
		*begin ^= *end;
		*end   ^= *begin;
		*begin ^= *end;
		begin++;
		end--;
	}

	*c = strdup(result);

	free(result);
	for (i = 0; i <= lena; i++) {
		free(D[i]);
	}
	free(D);
	for (i = 0; i <= lena; i++) {
		free(M[i]);
	}
	free(M);

	return result_i - 1;
}
#undef DIAG
#undef UP
#undef LEFT
