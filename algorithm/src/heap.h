#ifndef heap_H
#define heap_H

#ifdef __cplusplus
extern "C" {
#endif

#define MIN_HEAP 0x0
#define MAX_HEAP 0x1

struct heap;

int heap_add(struct heap *h, int value);
int heap_delete(struct heap *h, int index, int *value);
struct heap * heap_new(int max_size, int flag);
void heap_free(struct heap *h);

#ifdef __cplusplus
extern "C" {
#endif

#endif
