#ifndef media_receiver_H
#define media_receiver_H

#include "frame_buf.h"

struct media_receiver {
	int (*new_frame_buf_cb)(struct frame_buf *buf, void *arg);
	void *arg;

	struct media_receiver *prev;
	struct media_receiver *next;
};

struct media_receiver * media_receiver_new();

void media_receiver_free(struct media_receiver *receiver);

int media_receiver_set_callback(struct media_receiver *receiver,
		int (*new_frame_buf_cb)(struct frame_buf *buf, void *arg), 
		void *arg);

#endif
