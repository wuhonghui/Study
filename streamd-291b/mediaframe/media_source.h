#ifndef media_source_H
#define media_source_H

#include "frame_buf_queue.h"
#include "media_receiver.h"

struct media_source {
	char *name;
	
	struct frame_buf_queue *queue;
	struct media_receiver *receiver;

	struct media_source *prev;
	struct media_source *next;
};

struct media_source * media_source_new(char *name, int max_frame_buf);

void media_source_free(struct media_source *source);

int media_source_add_receiver(struct media_source *source, struct media_receiver *receiver);
int media_source_del_receiver(struct media_receiver *receiver);

int server_media_source_add(struct media_source *source);
int server_media_source_del(struct media_source *source);
int server_media_source_init();
int server_media_source_destroy();
struct media_source *server_media_source_find(char *name);

#endif
