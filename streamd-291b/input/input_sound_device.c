#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <errno.h>
#include <linux/soundcard.h>

#include "common.h"
#include "input_sound_device.h"
#include "event2/event.h"
#include "oss_api.h"
#include "slog.h"
#include "media_source.h"

struct {
	char *name;
	int fd;

	unsigned int fragmentsize;
	unsigned int format;
	int channels;
	unsigned samplerate;

	int isstreaming;

	void *buf;
	int buf_size;
	int buf_len;

	struct media_source *sound_source_pcm;

	struct event *ev;
	struct event_base *base;

	struct timeval interval;
	struct timeval begin;
} sound_device;

static void input_sound_device_getframe_cb(evutil_socket_t fd, short events, void *arg);

int input_sound_device_init(struct event_base *base)
{
	int ret = 0;
	int i = 0;

	memset(&sound_device, 0, sizeof(sound_device));
	sound_device.fd = -1;

	sound_device.name = strdup("/dev/dsp");
	sound_device.fragmentsize = 0x7fff000b;
	sound_device.format = AFMT_U8;
	sound_device.channels = 1;
	sound_device.samplerate = 8000;
	sound_device.base = base;

	if (oss_open(sound_device.name, &sound_device.fd))
		goto failed;

	if (oss_test_device(sound_device.fd)) {
		goto failed;
	}
	
	if (oss_set_format(sound_device.fd, sound_device.fragmentsize, sound_device.format, sound_device.channels, sound_device.samplerate)) {
		goto failed;
	}

	if (oss_get_max_buffer_size(sound_device.fd, &sound_device.buf_size)) {
		goto failed;
	}

	sound_device.buf = (void *)malloc(sound_device.buf_size);
	if (!sound_device.buf) {
		goto failed;
	}

	if (oss_trigger(sound_device.fd)) {
		goto failed;
	}
	sound_device.isstreaming = 1;

	sound_device.ev = event_new(base, sound_device.fd, 0, input_sound_device_getframe_cb, NULL);
	if (!sound_device.ev) {
		goto failed;
	}
	gettimeofday(&sound_device.begin, NULL);
	sound_device.interval.tv_sec = 0;
	sound_device.interval.tv_usec = 40 * 1000; 

	struct timeval after = {0, 200 * 1000};
	event_add(sound_device.ev, &after);

	sound_device.sound_source_pcm = server_media_source_find("LIVE-SOUND-PCM");
	if (!sound_device.sound_source_pcm) {
		sound_device.sound_source_pcm = media_source_new("LIVE-SOUND-PCM", 2);
		if (!sound_device.sound_source_pcm) {
			goto failed;
		}
		server_media_source_add(sound_device.sound_source_pcm);
	}
	
	SLOG(SLOG_INFO, "Init sound device %s", sound_device.name);

	return 0;
failed:
	SLOG(SLOG_ERROR, "Failed to init sound device");
	
	input_sound_device_close();
	
	return -1;
}

int input_sound_device_close()
{
	SLOG(SLOG_INFO, "Close sound device %s", sound_device.name);
	
	if (sound_device.fd >= 0) {
		/* keep media source in server after device closed */
		if (sound_device.ev)
			event_free(sound_device.ev);
		if (sound_device.isstreaming) {
			oss_untrigger(sound_device.fd);
			sound_device.isstreaming = 0;
		}
		if (sound_device.buf) {
			free(sound_device.buf);
		}
		oss_close(sound_device.fd);
		if (sound_device.name) {
			free(sound_device.name);
		}
		memset(&sound_device, 0, sizeof(sound_device));
		sound_device.fd = -1;
	}
	
	return 0;
}

static void input_sound_device_getframe_cb(evutil_socket_t fd, short events, void *arg)
{
	int ret = 0;

	if (oss_getframe(sound_device.fd, sound_device.buf, sound_device.buf_size, &sound_device.buf_len)) {
		goto schedule;
	}
	struct timeval timestamp;
	gettimeofday(&timestamp, NULL);

	struct frame_buf *buf = NULL;
	buf = frame_buf_new();
	if (!buf) {
		goto schedule;
	}
	
	if (frame_buf_set_data(buf, (void *)sound_device.buf,
		sound_device.buf_len, &timestamp)) {
		frame_buf_free(buf);
		goto schedule;
	}

	if (frame_buf_queue_put(sound_device.sound_source_pcm->queue, buf)) {
		frame_buf_free(buf);
		goto schedule;
	}
	
schedule:
	event_add(sound_device.ev, &(sound_device.interval));
}
