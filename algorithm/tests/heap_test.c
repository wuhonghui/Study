#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "heap.h"

int main(int argc, char *argv[])
{
	struct heap *h = NULL;
	int i = 0;
	int j = 0;

	h = heap_new(50, 0);
	for (i = 50; i > 0; i--) {
		heap_add(h, i);
	}
	while(1) {
		if (heap_delete(h, 0, &j)) {
			break;
		}
		i++;
		assert(j == i);
	}
	heap_free(h);

	h = heap_new(5000, 1);
	for (i = 0; i < 5000; i++) {
		heap_add(h, i);
	}
	while(1) {
		if (heap_delete(h, 0, &j)) {
			break;
		}
		i--;
		assert(j == i);
	}
	heap_free(h);

	return 0;
}
