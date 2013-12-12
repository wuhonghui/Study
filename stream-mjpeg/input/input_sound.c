#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <errno.h>
#include "common.h"
#include <linux/soundcard.h>
#include "input_sound.h"
#include "event2/event.h"
#include "utils/timing.h"
#include "sound_pcm_oss.h"

struct {
	struct snd_pcm_oss_device *device;
	struct snd_pcm_oss_config *config;
	struct event *ev;
	struct event_base *base;

	struct generic_receiver *receiver;
	uint32_t cbcount;
	uint32_t lastcbcount;

	struct timeval interval;
	struct timeval begin;

	uint32_t connection_now;
	uint32_t connection_max;
}sound;


#ifdef ENABLE_TIMING
static struct timing_helper *sound_read = NULL;
static struct timing_helper *sound_send = NULL;
#endif

int sound_init()
{
	int ret = 0;

	memset(&sound, 0, sizeof(sound));

	sound.device = snd_pcm_oss_device_new();
	sound.config = snd_pcm_oss_config_new();
	sound.connection_max = 16 + 1;

	if (!sound.device || !sound.config) {
		goto failed;
	}

	snd_pcm_oss_config_init(sound.config, AFMT_U8, 8, 8000, 1, 0x7fff000b);

	ret = snd_pcm_oss_device_init(sound.device, "/dev/dsp", sound.config);

	if (ret) {
		printf("failed to init sound device\n");
		goto failed;
	}

	return 0;
failed:
	if (sound.device) {
		snd_pcm_oss_device_free(sound.device);
		sound.device = NULL;
	}
	if (sound.config) {
		snd_pcm_oss_config_free(sound.config);
		sound.config = NULL;
	}
	return -1;
}

int sound_close()
{
	if (sound.device) {
		snd_pcm_oss_device_close(sound.device);
		snd_pcm_oss_device_free(sound.device);
		sound.device = NULL;
	}
	if (sound.config) {
		snd_pcm_oss_config_free(sound.config);
		sound.config = NULL;
	}
	return 0;
}


struct generic_receiver * sound_add_receiver(const char *identity,
	int (*cb)(void *data, size_t len, void *arg),
	void *arg)
{
	struct generic_receiver *receiver = NULL;

	if (!sound.device) {
		return NULL;
	}

	/* check if corresponding receiver is alread in list */
	receiver = sound.receiver;
	while (receiver != NULL) {
		if (receiver->arg == arg) {
			dbprintf("receiver %p for arg %p is already in list\n", receiver, arg);
			return receiver;
		}
		receiver = receiver->next;
	}

	/* check if too many connections */
	if (sound.connection_now == sound.connection_max) {
		printf("sound have %d receivers now, reject new.\n", sound.connection_now);
		return NULL;
	}

	/* alloc receiver */
	receiver = (struct generic_receiver *)malloc(sizeof(struct generic_receiver));
	if (!receiver)
		return NULL;

	/* init */
	receiver->identity = NULL;
	if (identity)
		receiver->identity = strdup(identity);
	receiver->cb = cb;
	receiver->arg = arg;
	receiver->next = NULL;
	receiver->cbcount = 0;
	gettimeofday(&(receiver->begin), NULL);

	/* receiver list */
	receiver->next = sound.receiver;
	sound.receiver = receiver;

	/* connection statistics */
	sound.connection_now++;

	/* add to database(filesystem) */
	if (receiver->identity) {
		receiver_identity_add(receiver->identity);
	}

	return receiver;
}


void sound_remove_receiver(struct generic_receiver *receiver)
{
	if (!receiver)
		return;

	struct generic_receiver *prev = sound.receiver;

	//show_receiver(receiver);

	if (prev) {
		if (prev == receiver) {
			sound.receiver = receiver->next;
			if (receiver->identity) {
				receiver_identity_del(receiver->identity);
				free(receiver->identity);
			}
			free(receiver);
			sound.connection_now--;
		} else {
			while(prev->next && prev->next != receiver) {
				prev = prev->next;
			}
			if (prev->next) {
				prev->next = receiver->next;
				if (receiver->identity) {
					receiver_identity_del(receiver->identity);
					free(receiver->identity);
				}
				free(receiver);
				sound.connection_now--;
			}
		}
	}
}

static void sound_cb(evutil_socket_t fd, short events, void *arg)
{
	int ret = 0;
	struct generic_receiver *receiver = NULL;
	struct generic_receiver *receiver_next = NULL;

#ifdef ENABLE_TIMING
	timing_helper_begin(sound_read);
#endif
	ret = snd_pcm_oss_device_next_fragment(sound.device);
	if (ret < 0) {
		goto out;
	}
#ifdef ENABLE_TIMING
	timing_helper_end(sound_read);
#endif

	struct buffer *fragment = snd_pcm_oss_device_getfrag(sound.device);

#ifdef ENABLE_TIMING
	timing_helper_begin(sound_send);
#endif

	if (sound.cbcount - sound.lastcbcount > 8) {
		sound.lastcbcount = sound.cbcount;

		receiver = sound.receiver;
		while(receiver != NULL) {
			receiver_next = receiver->next;
			if (receiver->identity) {
				if (!receiver_identity_is_exist(receiver->identity)) {
					receiver->cb(NULL, 0, receiver->arg);
				}
			}
			receiver = receiver_next;
		}
	}

	sound.cbcount++;
	receiver = sound.receiver;
	while(receiver != NULL) {
		receiver_next = receiver->next;
		receiver->cbcount++;
		ret = receiver->cb(fragment->data, fragment->size, receiver->arg);
		receiver = receiver_next;
	}

#ifdef ENABLE_TIMING
	timing_helper_end(sound_send);
#endif

out:
	event_add(sound.ev, &(sound.interval));
}

int sound_start(struct event_base *base)
{
	int ret = 0;

	if (!sound.device) {
		return -1;
	}

	if (!base) {
		return -1;
	}

	ret = snd_pcm_oss_device_trigger(sound.device);
	if (ret) {
		return -1;
	}

	sound.ev = event_new(base, snd_pcm_oss_device_getfd(sound.device), 0, sound_cb, NULL);
	gettimeofday(&(sound.begin), NULL);
	sound.interval.tv_sec = 0;
	sound.interval.tv_usec = 125*1000;

	struct timeval after = {0, 200 * 1000};
	event_add(sound.ev, &after);

#ifdef ENABLE_TIMING
	sound_read= timing_helper_new("Sound Read");
	sound_send = timing_helper_new("Sound Send");
#endif

	return 0;
}

int sound_stop()
{
	int ret = 0;
	struct generic_receiver *receiver = NULL;
	struct generic_receiver *receiver_next = NULL;

	if (!sound.device) {
		return -1;
	}

	if (sound.ev) {
		event_free(sound.ev);
	}

	receiver = sound.receiver;
	while(receiver != NULL) {
		receiver_next = receiver->next;
		sound_remove_receiver(receiver);
		receiver = receiver_next;
	}

	ret = snd_pcm_oss_device_untrigger(sound.device);
	if (ret) {
		return -1;
	}

#ifdef ENABLE_TIMING
	timing_helper_free(sound_read);
	timing_helper_free(sound_send);
#endif

	struct timeval end;
	float totaltime;
	float fps;

	gettimeofday(&end, NULL);
	timeval_sub(&end, &(sound.begin), &end);
	totaltime = timeval_to_float(&end);
	fps = ((float)sound.cbcount) / totaltime;

	printf("Current sound connections %d\n", sound.connection_now);
	printf("Device running time: %.06f seconds\n", totaltime);
	printf("Statistis: %d frame, avg %2.2f fps\n", sound.cbcount, fps);

	return 0;
}

enum DEVICE_STATUS sound_device_status(void)
{
	if (!sound.device)
		return DEVICE_DOWN;

	if (snd_pcm_oss_device_isstreaming(sound.device))
		return DEVICE_RUN_STREAMING;
	else
		return DEVICE_RUN_NOT_STREAMING;
}

