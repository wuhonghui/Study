#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include "common.h"
#include "input_video.h"
#include "v4l2_uvc.h"
#include "event2/event.h"
#include "utils/timing.h"
#include "receiver.h"

struct {
	struct uvc_video_device *device;
	struct uvc_video_config *config;
	struct event *ev;
	struct event_base *base;

	struct generic_receiver *receiver;
	uint32_t cbcount;
	uint32_t lastcbcout;

	struct timeval interval;
	struct timeval begin;

	int send_kb_per_second;
	int bitrate;

	uint32_t connection_now;
	uint32_t connection_max;
}video;

#ifdef ENABLE_TIMING
static struct timing_helper *video_ioctl = NULL;
static struct timing_helper *video_send = NULL;
#endif

int video_init(char *dev, int width, int height, int fps, int max_connection)
{
	int ret = 0;

	memset(&video, 0, sizeof(video));

	video.device = uvc_video_device_new();
	video.config = uvc_video_config_new();
	if (!video.device || !video.config) {
		goto failed;
	}
	
	video.connection_max = max_connection;

	uvc_video_config_init(video.config,
		width,
		height,
		fps);

	ret = uvc_video_device_init(video.device, dev, video.config);
	if (ret) {
		printf("failed to init uvc video device\n");
		goto failed;
	}

	/* test device capability */
	/* uvc_video_device_test(video.device); */

	receiver_identity_init();

	return 0;
failed:
	if (video.device) {
		uvc_video_device_free(video.device);
		video.device = NULL;
	}
	if (video.config) {
		uvc_video_config_free(video.config);
		video.config = NULL;
	}
	return -1;
}

int video_close()
{
	if (video.device) {
		uvc_video_device_close(video.device);
		uvc_video_device_free(video.device);
		video.device = NULL;
	}
	if (video.config) {
		uvc_video_config_free(video.config);
		video.config = NULL;
	}
	return 0;
}

int video_reconfig()
{
	int ret = 0;

	dbprintf("video device reconfig\n");

	ret = uvc_video_device_streamoff(video.device);
	if (ret) {
		return -1;
	}

	uvc_video_device_close(video.device);

	/* take a cup of coffee */
	usleep(500*1000);
	usleep(500*1000);
	usleep(500*1000);

	ret = uvc_video_device_init(video.device, "/dev/video0", video.config);
	if (ret) {
		dbprintf("failed to init uvc video device\n");
		return -1;
	}

	ret = uvc_video_device_streamon(video.device);
	if (ret) {
		dbprintf("failed to streamon uvc video device\n");
		return -1;
	}

	dbprintf("video device reconfig finished\n");

	return 0;
}

#ifdef ENABLE_TIMING
static void show_receiver(struct generic_receiver *receiver)
{
    struct tm *nowtm = NULL;
    char startbuf[TIMEBUF_LEN];
	char endbuf[TIMEBUF_LEN];
    struct timeval end;
    float fps;
    float totaltime;

	/* short connection, not enough statistics data */
	if (receiver->cbcount < 20)
		return;

    gettimeofday(&end, NULL);
    timeval_to_str(&(receiver->begin), startbuf, TIMEBUF_LEN);
    timeval_to_str(&(end), endbuf, TIMEBUF_LEN);

    timeval_sub(&end, &receiver->begin, &end);
	totaltime = timeval_to_float(&end);

    fps = ((float)receiver->cbcount) / totaltime;

	printf("    identity    : %s\n", receiver->identity);
    printf("    start time  : %s\n", startbuf);
    printf("    now time    : %s\n", endbuf);
	printf("    elapse      : %2.06f\n", totaltime);
	printf("    total frame : %d\n", receiver->cbcount);
    printf("    fps         : %2.2f\n", fps);
}
#endif

struct generic_receiver * video_add_receiver(const char *identity,
	int (*cb)(void *data, size_t len, void *arg),
	void *arg)
{
	struct generic_receiver *receiver = NULL;

	if (!video.device) {
		return NULL;
	}

	/* check if corresponding receiver is alread in list */
	receiver = video.receiver;
	while (receiver != NULL) {
		if (receiver->arg == arg) {
			dbprintf("receiver %p for arg %p is already in list\n", receiver, arg);
			return receiver;
		}
		receiver = receiver->next;
	}

	/* check if too many connections(identity null means not check) */
	if (identity != NULL) {
		if (video.connection_now >= video.connection_max) {
			printf("video have %d receivers now, reject new connection.\n", video.connection_now);
			return NULL;
		}
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
	receiver->next = video.receiver;
	video.receiver = receiver;

	/* add to database(filesystem) */
	if (receiver->identity) {
		receiver_identity_add(receiver->identity);
		/* connection statistics */
		video.connection_now++;
	}

	return receiver;
}

void video_remove_receiver(struct generic_receiver *receiver)
{
	struct generic_receiver *prev = video.receiver;

#ifdef ENABLE_TIMING
	show_receiver(receiver);
#endif

	if (prev) {
		if (prev == receiver) {
			video.receiver = receiver->next;
			if (receiver->identity) {
				receiver_identity_del(receiver->identity);
				free(receiver->identity);
			}
			free(receiver);
			video.connection_now--;
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
				video.connection_now--;
			}
		}
	}
}

static void video_cb(evutil_socket_t fd, short events, void *arg)
{
	void *data = NULL;
	size_t size;
	int ret = 0;
	struct generic_receiver *receiver = NULL;
	struct generic_receiver *receiver_next = NULL;

#ifdef ENABLE_TIMING
	timing_helper_begin(video_ioctl);
#endif

	ret = uvc_video_device_nextframe(video.device);

#ifdef ENABLE_TIMING
	timing_helper_end(video_ioctl);
#endif
	if (ret < 0) {
		if (ret == ERR_DEV) {
			video_reconfig();
		}
		goto out;
	}

	struct buffer *framebuffer = uvc_video_device_getframe(video.device);

	/* update config each second */
	if (video.cbcount - video.lastcbcout > uvc_video_config_get_fps(video.config)) {
		video.lastcbcout = video.cbcount;
		video.bitrate = video.send_kb_per_second;
		video.send_kb_per_second = 0;

		/*
		int fps = 0;
		uvc_video_config_get_details(video.config, NULL, NULL, &fps);
		dbprintf("fps:%d, bitrate:%d Kbps\n", fps, video.bitrate);
		*/

		receiver = video.receiver;
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
	video.send_kb_per_second += (framebuffer->size >> 7);	/* kbits */

#ifdef ENABLE_TIMING
	timing_helper_begin(video_send);
#endif

	video.cbcount++;

	receiver = video.receiver;
	while(receiver != NULL) {
		receiver_next = receiver->next;
		receiver->cb(framebuffer->data, framebuffer->size, receiver->arg);
		receiver->cbcount++;
		receiver = receiver_next;
	}

#ifdef ENABLE_TIMING
	timing_helper_end(video_send);
#endif

out:
	event_add(video.ev, &(video.interval));
}


int video_start(struct event_base *base)
{
	int ret = 0;

	if (!video.device) {
		return -1;
	}

	if (!base) {
		return -1;
	}

	ret = uvc_video_device_streamon(video.device);
	if (ret) {
		return -1;
	}

	video.base = base;
	video.ev = event_new(base, uvc_video_device_getfd(video.device), 0, video_cb, NULL);
	if (!video.ev) {
		printf("Could not create video frame event\n");
		return -1;
	}
	gettimeofday(&video.begin, NULL);
	video.interval.tv_sec = 0;
	video.interval.tv_usec = 25 * 1000;

	struct timeval after = {0, 200 * 1000};
	event_add(video.ev, &after);

#ifdef ENABLE_TIMING
	video_ioctl = timing_helper_new("Video GetNextFrame");
	video_send = timing_helper_new("Video Send");
#endif

	return 0;
}

int video_stop()
{
	int ret = 0;
	struct generic_receiver *receiver = NULL;
	struct generic_receiver *receiver_next = NULL;

	if (!video.device) {
		return -1;
	}

	if (video.ev) {
		event_free(video.ev);
	}

	receiver = video.receiver;
	while(receiver != NULL) {
		receiver_next = receiver->next;
		video_remove_receiver(receiver);
		receiver = receiver_next;
	}

	ret = uvc_video_device_streamoff(video.device);
	if (ret) {
		return -1;
	}

#ifdef ENABLE_TIMING
	timing_helper_free(video_ioctl);
	timing_helper_free(video_send);
#endif

	struct timeval end;
	float totaltime;
	float fps;

	gettimeofday(&end, NULL);
	timeval_sub(&end, &(video.begin), &end);
	totaltime = timeval_to_float(&end);
	fps = ((float)video.cbcount) / totaltime;

	printf("Current video connections %d\n", video.connection_now);
	printf("Device running time: %.06f seconds\n", totaltime);
	printf("Statictis: %d frame, avg %2.2f fps\n", video.cbcount, fps);

	return 0;
}

enum DEVICE_STATUS video_device_status(void)
{
	if (!video.device)
		return DEVICE_DOWN;

	if (uvc_video_device_isstreaming(video.device))
		return DEVICE_RUN_STREAMING;
	else
		return DEVICE_RUN_NOT_STREAMING;
}

unsigned int video_device_get_cbcount()
{
	return video.cbcount;
}
