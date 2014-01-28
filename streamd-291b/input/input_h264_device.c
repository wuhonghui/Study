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

#include <event2/event.h>

#include "common.h"
#include "input_h264_device.h"
#include "media_source.h"
#include "v4l2_api.h"
#include "sonix_xu_ctrls.h"
#include "nalu.h"
#include "slog.h"

#define MAX_BUFFER_FRAME 3

#define H264_SIZE_HD				((1280<<16)|720)
#define H264_SIZE_VGA				((640<<16)|480)
#define H264_SIZE_QVGA				((320<<16)|240)
#define H264_SIZE_QQVGA				((160<<16)|112)

struct H264Format *gH264fmt = NULL;

// TODO: set h264 format and put nalus into different framebuffer queue.
// TODO: set multistream format and see what's going on when multi-streaming.
// TODO: Iframe interval?

static struct video_device {
	char *name;
	int fd;

	int width;
	int height;
	__u32 pixelformat;
	int fps;

	int h264_iframe_set;
	unsigned int nframe;

	int isstreaming;

	struct v4l2_buffer mmap_buffer[MAX_BUFFER_FRAME];
	struct v4l2_buffer user_buffer;

	struct media_source *h264_source_hd;
	struct media_source *h264_source_vga;
	struct media_source *h264_source_qvga;
	struct media_source *h264_source_qqvga;

	struct event *ev;
	struct event_base *base;

	struct timeval interval;
	struct timeval begin;
} h264_device;

static void input_h264_device_getframe_cb(evutil_socket_t fd, short events, void *arg);

int input_h264_device_init(struct event_base *base)
{
	int ret = 0;
	int i = 0;

	memset(&h264_device, 0, sizeof(h264_device));
	h264_device.fd = -1;

	h264_device.name = strdup("/dev/video1");
	h264_device.width = 1280;
	h264_device.height = 720;
	h264_device.pixelformat = V4L2_PIX_FMT_H264;
	h264_device.fps = 30;
	h264_device.h264_iframe_set = 20;
	h264_device.base = base;

	if (video_open(h264_device.name, &h264_device.fd))
		goto failed;

	//if (video_test_device(h264_device.fd)) {
	//	goto failed;
	//}

	// TODO:
	if (video_set_format(h264_device.fd, h264_device.width, h264_device.height, h264_device.pixelformat)) {
		/* skip for h264 */
		SLOG(SLOG_INFO, "Failed to set h264 format(V4L2).");
	}

	if (video_set_fps(h264_device.fd, h264_device.fps)) {
		goto failed;
	}

	if (video_request_buffer(h264_device.fd, MAX_BUFFER_FRAME)) {
		goto failed;
	}

	if (video_mmap_driver_buffers(h264_device.fd, h264_device.mmap_buffer, MAX_BUFFER_FRAME)) {
		goto failed;
	}
		
	for (i = 0; i < MAX_BUFFER_FRAME; i++) {
		if (video_queue_buffer(h264_device.fd, i)) {
			goto failed;
		}
	}

	memcpy(&h264_device.user_buffer, h264_device.mmap_buffer, sizeof(struct v4l2_buffer));
	h264_device.user_buffer.m.userptr = (unsigned long)malloc(h264_device.user_buffer.length);
	h264_device.user_buffer.bytesused = 0;

	if (video_stream_on(h264_device.fd)) {
		goto failed;
	}
	h264_device.isstreaming = 1;

	h264_device.ev = event_new(base, h264_device.fd, 0, input_h264_device_getframe_cb, NULL);
	if (!h264_device.ev) {
		goto failed;
	}
	gettimeofday(&h264_device.begin, NULL);
	h264_device.interval.tv_sec = 0;
	h264_device.interval.tv_usec = 6 * 1000; /* should test when multistream is enabled */

	struct timeval after = {0, 200 * 1000};
	event_add(h264_device.ev, &after);

	h264_device.h264_source_hd = server_media_source_find("LIVE-H264-HD");
	if (!h264_device.h264_source_hd) {
		h264_device.h264_source_hd = media_source_new("LIVE-H264-HD", 20);
		if (!h264_device.h264_source_hd) {
			goto failed;
		}
		server_media_source_add(h264_device.h264_source_hd);
	}

	h264_device.h264_source_vga = server_media_source_find("LIVE-H264-VGA");
	if (!h264_device.h264_source_vga) {
		h264_device.h264_source_vga = media_source_new("LIVE-H264-VGA", 20);
		if (!h264_device.h264_source_vga) {
			goto failed;
		}
		server_media_source_add(h264_device.h264_source_vga);
	}

	h264_device.h264_source_qvga = server_media_source_find("LIVE-H264-QVGA");
	if (!h264_device.h264_source_qvga) {
		h264_device.h264_source_qvga = media_source_new("LIVE-H264-QVGA", 20);
		if (!h264_device.h264_source_qvga) {
			goto failed;
		}
		server_media_source_add(h264_device.h264_source_qvga);
	}
	
	h264_device.h264_source_qqvga = server_media_source_find("LIVE-H264-QQVGA");
	if (!h264_device.h264_source_qqvga) {
		h264_device.h264_source_qqvga = media_source_new("LIVE-H264-QQVGA", 20);
		if (!h264_device.h264_source_qqvga) {
			goto failed;
		}
		server_media_source_add(h264_device.h264_source_qqvga);
	}

	SLOG(SLOG_INFO, "Init H264 video device %s", h264_device.name);

	return 0;
failed:
	input_h264_device_close();
	return -1;
}

int input_h264_device_close()
{
	SLOG(SLOG_INFO, "Close H264 video device %s", h264_device.name);
	
	if (h264_device.fd >= 0) {
		/* keep media source in server after device closed */
		if (h264_device.ev)
			event_free(h264_device.ev);
		if (h264_device.isstreaming) {
			video_stream_off(h264_device.fd);
			h264_device.isstreaming = 0;
		}
		video_unmmap_driver_buffers(h264_device.fd, h264_device.mmap_buffer, MAX_BUFFER_FRAME);
		if (h264_device.user_buffer.m.userptr)
			free((unsigned char *)h264_device.user_buffer.m.userptr);
		video_close(h264_device.fd);
		if (h264_device.name) {
			free(h264_device.name);
		}
		memset(&h264_device, 0, sizeof(h264_device));
		h264_device.fd = -1;
	}
	return 0;
}

static void input_h264_device_getframe_cb(evutil_socket_t fd, short events, void *arg)
{
	int ret = 0;

	if((h264_device.nframe % h264_device.h264_iframe_set) == 0)	{
		XU_H264_Set_IFRAME(h264_device.fd);
	}

	if (video_getframe(h264_device.fd, h264_device.mmap_buffer, &h264_device.user_buffer)) {
		goto schedule;
	}

	struct frame_buf *buf = NULL;
	buf = frame_buf_new();
	if (!buf) {
		goto schedule;
	}
	if (frame_buf_set_data(buf, (void *)h264_device.user_buffer.m.userptr,
		h264_device.user_buffer.bytesused,
		&(h264_device.user_buffer.timestamp))) {
		frame_buf_free(buf);
		goto schedule;
	}

	unsigned char *nalu_data = (unsigned char *)h264_device.user_buffer.m.userptr;
	unsigned int   nalu_size = h264_device.user_buffer.bytesused;
	static unsigned int width;
	static unsigned int height;
	static unsigned int resolution;
	
	h264_decode_seq_parameter_set(nalu_data+4, nalu_size, &width, &height);
	resolution = ((width << 16) | height);

	// SLOG(SLOG_DEBUG, "H264 resolution %d x %d, size %u", width, height, nalu_size);

	if (resolution == H264_SIZE_HD) {
		if (frame_buf_queue_put(h264_device.h264_source_hd->queue, buf)) {
			frame_buf_free(buf);
			goto schedule;
		}
	} else if (resolution == H264_SIZE_VGA) {
		if (frame_buf_queue_put(h264_device.h264_source_vga->queue, buf)) {
			frame_buf_free(buf);
			goto schedule;
		}
	} else if (resolution == H264_SIZE_QVGA) {
		if (frame_buf_queue_put(h264_device.h264_source_qvga->queue, buf)) {
			frame_buf_free(buf);
			goto schedule;
		}
	} else if (resolution == H264_SIZE_QQVGA) {
		if (frame_buf_queue_put(h264_device.h264_source_qqvga->queue, buf)) {
			frame_buf_free(buf);
			goto schedule;
		}
	} else {
		SLOG(SLOG_ERROR, "unknow h264 resolution %d x %d", width, height);
		frame_buf_free(buf);
		goto schedule;
	}

	h264_device.nframe++;
	
schedule:
	event_add(h264_device.ev, &(h264_device.interval));
}


