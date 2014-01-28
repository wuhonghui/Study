#ifndef frame_buf_queue_H
#define frame_buf_queue_H

#include "frame_buf.h"
#include "media_receiver.h"

struct frame_buf_queue * frame_buf_queue_new(int max_buf);
void frame_buf_queue_free(struct frame_buf_queue *queue);
int frame_buf_queue_put(struct frame_buf_queue *queue, struct frame_buf *buf);
int frame_buf_queue_drain_buf(struct frame_buf_queue *queue);
int frame_buf_queue_set_cb(struct frame_buf_queue *queue,
	int (*new_frame_buf_cb)(struct frame_buf *last_buf, void *arg), void *arg);

// with multi-thread support?
struct frame_buf_queue {
	struct frame_buf *last_buf;
	int n_buf;
	int max_buf;

	int (*new_frame_buf_cb)(struct frame_buf *last_buf, void *arg);
	void *arg;
};

#endif
