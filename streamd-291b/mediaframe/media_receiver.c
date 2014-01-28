#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "media_receiver.h"

struct media_receiver * media_receiver_new()
{
	struct media_receiver *receiver = NULL;

	receiver = (struct media_receiver *)calloc(1, sizeof(struct media_receiver));
	if (!receiver)
		return NULL;

	receiver->next = receiver;
	receiver->prev = receiver;

	return receiver;
}

void media_receiver_free(struct media_receiver *receiver)
{
	if (receiver) {
		receiver->next->prev = receiver->prev;
		receiver->prev->next = receiver->next;
		receiver->next = receiver;
		receiver->prev = receiver;

		free(receiver);
	}
}

int media_receiver_set_callback(struct media_receiver *receiver,
		int (*new_frame_buf_cb)(struct frame_buf *buf, void *arg), 
		void *arg)
{
	if (!receiver || !new_frame_buf_cb)
		return -1;
	receiver->new_frame_buf_cb = new_frame_buf_cb;
	receiver->arg = arg;

	return 0;
}
