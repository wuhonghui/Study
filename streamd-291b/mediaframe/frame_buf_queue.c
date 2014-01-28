#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "frame_buf.h"
#include "frame_buf_queue.h"

struct frame_buf_queue * frame_buf_queue_new(int max_buf)
{
	struct frame_buf_queue *queue = NULL;
	queue = (struct frame_buf_queue *)calloc(1, sizeof(struct frame_buf_queue));
	if (!queue)
		return NULL;

	queue->max_buf = max_buf;

	return queue;
}

void frame_buf_queue_free(struct frame_buf_queue *queue)
{
	if (queue) {
		if (queue->last_buf) {
			frame_buf_queue_drain_buf(queue);
		}

		free(queue);
	}
}

int frame_buf_queue_put(struct frame_buf_queue *queue, struct frame_buf *buf)
{
	if (!queue || !buf) {
		return -1;
	}

	if (queue->n_buf >= queue->max_buf) {
		frame_buf_free(queue->last_buf->next);
		queue->n_buf--;
	}
	
	if (queue->last_buf) {
		buf->next = queue->last_buf->next;
		buf->prev = queue->last_buf;
		buf->next->prev = buf;
		queue->last_buf->next = buf;
	}

	queue->last_buf = buf;
	queue->n_buf++;

	if (queue->new_frame_buf_cb) {
		// deal ret value ?
		queue->new_frame_buf_cb(queue->last_buf, queue->arg);
	}

	return 0;
}

int frame_buf_queue_drain_buf(struct frame_buf_queue *queue)
{
	if (!queue) 
		return -1;

	if (queue->last_buf) {
		while (queue->last_buf->next != queue->last_buf) {
			frame_buf_free(queue->last_buf->next);
		}
		frame_buf_free(queue->last_buf);
	}
	
	queue->n_buf = 0;
	queue->last_buf = NULL;

	return 0;
}

int frame_buf_queue_set_cb(struct frame_buf_queue *queue,
	int (*new_frame_buf_cb)(struct frame_buf *last_buf, void *arg), void *arg)
{
	if (queue) {
		queue->new_frame_buf_cb = new_frame_buf_cb;
		queue->arg = arg;
	}

	return 0;
}

