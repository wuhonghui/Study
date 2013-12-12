#ifndef input_sound_H
#define input_sound_H

#ifdef __cplusplus
extern "C" {
#endif

#include "event2/event.h"
#include "receiver.h"

int sound_init();

int sound_close();

struct generic_receiver * sound_add_receiver(const char *identity,
	int (*cb)(void *data, size_t len, void *arg),
	void *arg);

void sound_remove_receiver(struct generic_receiver *receiver);

int sound_start(struct event_base *base);

int sound_stop();

enum DEVICE_STATUS sound_device_status(void);

#ifdef __cplusplus
}
#endif

#endif
