#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "frame_buf.h"
#include "frame_buf_queue.h"
#include "media_receiver.h"
#include "media_source.h"
#include "slog.h"

static struct media_source *server_source = NULL;

int media_source_on_new_frame_buf(struct frame_buf *last_buf, void *arg)
{
	struct media_source *source = (struct media_source *)arg;
	struct media_receiver *receiver = NULL;
	struct media_receiver *receiver_next = NULL;

	if (!source) {
		return 0;
	}
	receiver = source->receiver->next;

	while (receiver != source->receiver) {
		receiver_next = receiver->next;
		if (receiver->new_frame_buf_cb) {
			// deal with ret?
			receiver->new_frame_buf_cb(last_buf, receiver->arg);
		}
		receiver = receiver_next;
	}
	
	return 0;
}

struct media_source * media_source_new(char *name, int max_frame_buf)
{
	struct media_source *source = NULL;
	source = (struct media_source *)calloc(1, sizeof(struct media_source));
	if (!source) {
		return NULL;
	}

	if (max_frame_buf > 0) {
		source->queue = frame_buf_queue_new(max_frame_buf);
		if (!source->queue) {
			goto failed;
		}
		
		frame_buf_queue_set_cb(source->queue, media_source_on_new_frame_buf, source);
		
		source->receiver = media_receiver_new();
		if (!source->receiver) {
			goto failed;
		}
	}
	
	if (name) {
		source->name = strdup(name);
		if (!source->name) {
			goto failed;
		}
	}
	source->prev = source;
	source->next = source;

	return source;
	
failed:
	media_source_free(source);
	return NULL;
}

void media_source_free(struct media_source *source)
{
	if (source) {
		if (source->queue)
			frame_buf_queue_free(source->queue);
		if (source->receiver) {
			while (source->receiver->next != source->receiver) {
				media_source_del_receiver(source->receiver->next);
			}
			media_receiver_free(source->receiver->next);
		}
		if (source->name)
			free(source->name);
		source->prev->next = source->next;
		source->next->prev = source->prev;
		free(source);
	}
}

int media_source_add_receiver(struct media_source *source, struct media_receiver *receiver)
{
	if (!source || !receiver)
		return -1;
	
	receiver->next = source->receiver->next;
	receiver->prev = source->receiver;
	receiver->next->prev = receiver;
	source->receiver->next = receiver;

	return 0;
}

int media_source_del_receiver(struct media_receiver *receiver)
{
	if (!receiver)
		return -1;
	
	receiver->next->prev = receiver->prev;
	receiver->prev->next = receiver->next;
	receiver->prev = receiver;
	receiver->next = receiver;
	
	return 0;
}

void media_source_list_add(struct media_source *head, struct media_source *source)
{
	source->next = head->next;
	source->prev = head;
	source->next->prev = source;
	head->next = source;
}

void media_source_list_del(struct media_source *source)
{
	source->prev->next = source->next;
	source->next->prev = source->prev;
	source->next = source;
	source->prev = source;
}





int server_media_source_add(struct media_source *source)
{
	if (server_source == NULL)
		server_media_source_init();
	
	media_source_list_add(server_source, source);
	
	return 0;
}

int server_media_source_del(struct media_source *source)
{
	media_source_list_del(source);
	
	return 0;
}

struct media_source *server_media_source_find(char *name)
{
	struct media_source *source = NULL;

	if (server_source) {
		source = server_source->next;
		while (source != server_source) {
			if (strcmp(source->name, name) == 0) {
				return source;
			}
			source = source->next;
		}
	}

	return NULL;
}

int server_media_source_init()
{
	if (server_source == NULL) {
		server_source = media_source_new("server source head", 0);
		if (!server_source) {
			return -1;
		}
	}
	return 0;
}

int server_media_source_destroy()
{
	if (server_source) {
		while (server_source->next != server_source) {
			media_source_free(server_source->next);
		}
		media_source_free(server_source);
	}

	return 0;
}

