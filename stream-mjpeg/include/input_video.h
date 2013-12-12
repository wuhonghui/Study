#ifndef input_video_H
#define input_video_H

#ifdef __cplusplus
extern "C" {
#endif

#include "event2/event.h"
#include "receiver.h"

int video_init();
int video_close();

int video_start(struct event_base *base);
int video_stop();

struct generic_receiver * video_add_receiver(const char *identity,
	int (*cb)(void *data, size_t len, void *arg),
	void *arg);

void video_remove_receiver(struct generic_receiver *receiver);

enum DEVICE_STATUS video_device_status(void);

unsigned int video_device_get_cbcount();

#ifdef __cplusplus
}
#endif


#endif
