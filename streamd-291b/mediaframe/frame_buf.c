#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "videodev2.h"
#include "frame_buf.h"

struct frame_buf * frame_buf_new(int bufsize)
{
	struct frame_buf *buf = NULL;
	buf = (struct frame_buf *)calloc(1, sizeof(struct frame_buf));
	if (!buf) {
		return NULL;
	}
	buf->prev = buf;
	buf->next = buf;

	return buf;
}

void frame_buf_free(struct frame_buf *buf)
{
	if (buf) {
		buf->prev->next = buf->next;
		buf->next->prev = buf->prev;

		if (buf->data)
			free(buf->data);

		free(buf);
	}
}

int frame_buf_set_data(struct frame_buf *buf, void *data, int size, struct timeval *tv)
{
	if (!data || !buf) 
		return -1;	
	
	if (buf->data) {
		free(buf->data);
		buf->data = NULL;
	}
		
	buf->data = malloc(size);
	if (!buf->data)
		return -1;
	
	memcpy(buf->data, data, size);
	buf->size = size;

	if (tv) {
		buf->timestamp = *tv;
	}
	
	return 0;
}
