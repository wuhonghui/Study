#ifndef frame_buf_H
#define frame_buf_H

#include <stdint.h>

struct frame_buf {
	void *data;
	uint32_t size;
	struct timeval timestamp;

	struct frame_buf *prev;
	struct frame_buf *next;
	/* may extend */
};

struct frame_buf * frame_buf_new();
void frame_buf_free(struct frame_buf *buf);
int frame_buf_set_data(struct frame_buf *buf, void *data, int size, struct timeval *tv);
	
#endif
