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
#include "input_mjpeg_device.h"
#include "media_source.h"
#include "v4l2_api.h"
#include "sonix_xu_ctrls.h"
#include "slog.h"

#define MAX_BUFFER_FRAME 3

#define H264_SIZE_HD				((1280<<16)|720)
#define H264_SIZE_VGA				((640<<16)|480)
#define H264_SIZE_QVGA				((320<<16)|240)
#define H264_SIZE_QQVGA				((160<<16)|112)

/* use different structure in different device */
static struct video_device {
	char *name;
	int fd;

	int width;
	int height;
	__u32 pixelformat;
	int fps;

	int skip_frame;
	int nframe;
	
	int isstreaming;

	struct v4l2_buffer mmap_buffer[MAX_BUFFER_FRAME];
	struct v4l2_buffer user_buffer;

	struct media_source *mjpeg_source_hd;     /* 1280x720 */
	struct media_source *mjpeg_source_vga;	  /* 640x480 */
	struct media_source *mjpeg_source_qvga;   /* 320x240 */
	struct media_source *mjpeg_source_qqvga;  /* 160x112 */

	struct event *ev;
	struct event_base *base;

	struct timeval interval;
	struct timeval begin;
} mjpeg_device;

static void input_mjpeg_device_getframe_cb(evutil_socket_t fd, short events, void *arg);

int input_mjpeg_device_init(struct event_base *base)
{
	int ret = 0;
	int i = 0;

	memset(&mjpeg_device, 0, sizeof(mjpeg_device));
	mjpeg_device.fd = -1;

	mjpeg_device.name = strdup("/dev/video0");
	mjpeg_device.width = 640;
	mjpeg_device.height = 480;
	mjpeg_device.pixelformat = V4L2_PIX_FMT_MJPEG;
	mjpeg_device.fps = 20;
	mjpeg_device.skip_frame = 6;
	mjpeg_device.base = base;

	if (video_open(mjpeg_device.name, &mjpeg_device.fd))
		goto failed;

	//if (video_test_device(mjpeg_device.fd)) {
	//	goto failed;
	//}

	if (video_set_format(mjpeg_device.fd, mjpeg_device.width, mjpeg_device.height, mjpeg_device.pixelformat)) {
		goto failed;
	}

	if (video_set_fps(mjpeg_device.fd, mjpeg_device.fps)) {
		goto failed;
	}

	if (video_request_buffer(mjpeg_device.fd, MAX_BUFFER_FRAME)) {
		goto failed;
	}

	if (video_mmap_driver_buffers(mjpeg_device.fd, mjpeg_device.mmap_buffer, MAX_BUFFER_FRAME)) {
		goto failed;
	}
		
	for (i = 0; i < MAX_BUFFER_FRAME; i++) {
		if (video_queue_buffer(mjpeg_device.fd, i)) {
			goto failed;
		}
	}

	memcpy(&mjpeg_device.user_buffer, mjpeg_device.mmap_buffer, sizeof(struct v4l2_buffer));
	mjpeg_device.user_buffer.m.userptr = (unsigned long)malloc(mjpeg_device.user_buffer.length);
	mjpeg_device.user_buffer.bytesused = 0;

	if (video_stream_on(mjpeg_device.fd)) {
		goto failed;
	}
	mjpeg_device.isstreaming = 1;

	mjpeg_device.ev = event_new(base, mjpeg_device.fd, 0, input_mjpeg_device_getframe_cb, NULL);
	if (!mjpeg_device.ev) {
		goto failed;
	}
	gettimeofday(&mjpeg_device.begin, NULL);
	mjpeg_device.interval.tv_sec = 0;
	mjpeg_device.interval.tv_usec = 20 * 1000; /* should test when multistream is enabled */

	struct timeval after = {0, 200 * 1000};
	event_add(mjpeg_device.ev, &after);

	mjpeg_device.mjpeg_source_hd = server_media_source_find("LIVE-MJPEG-HD");
	if (!mjpeg_device.mjpeg_source_hd) {
		mjpeg_device.mjpeg_source_hd = media_source_new("LIVE-MJPEG-HD", 2);
		if (!mjpeg_device.mjpeg_source_hd) {
			goto failed;
		}
		server_media_source_add(mjpeg_device.mjpeg_source_hd);
	}

	mjpeg_device.mjpeg_source_vga = server_media_source_find("LIVE-MJPEG-VGA");
	if (!mjpeg_device.mjpeg_source_vga) {
		mjpeg_device.mjpeg_source_vga = media_source_new("LIVE-MJPEG-VGA", 2);
		if (!mjpeg_device.mjpeg_source_vga) {
			goto failed;
		}
		server_media_source_add(mjpeg_device.mjpeg_source_vga);
	}

	mjpeg_device.mjpeg_source_qvga = server_media_source_find("LIVE-MJPEG-QVGA");
	if (!mjpeg_device.mjpeg_source_qvga) {
		mjpeg_device.mjpeg_source_qvga = media_source_new("LIVE-MJPEG-QVGA", 2);
		if (!mjpeg_device.mjpeg_source_qvga) {
			goto failed;
		}
		server_media_source_add(mjpeg_device.mjpeg_source_qvga);
	}
	
	mjpeg_device.mjpeg_source_qqvga = server_media_source_find("LIVE-MJPEG-QQVGA");
	if (!mjpeg_device.mjpeg_source_qqvga) {
		mjpeg_device.mjpeg_source_qqvga = media_source_new("LIVE-MJPEG-QQVGA", 2);
		if (!mjpeg_device.mjpeg_source_qqvga) {
			goto failed;
		}
		server_media_source_add(mjpeg_device.mjpeg_source_qqvga);
	}

	SLOG(SLOG_INFO, "Init mjpeg video device %s", mjpeg_device.name);

	return 0;
failed:
	SLOG(SLOG_ERROR, "Failed to init mjpeg video device");
	
	input_mjpeg_device_close();
	
	return -1;
}

int input_mjpeg_device_close()
{
	SLOG(SLOG_INFO, "Close mjpeg video device %s", mjpeg_device.name);
	
	if (mjpeg_device.fd >= 0) {
		/* keep media source in server after device closed */
		if (mjpeg_device.ev)
			event_free(mjpeg_device.ev);
		if (mjpeg_device.isstreaming) {
			video_stream_off(mjpeg_device.fd);
			mjpeg_device.isstreaming = 0;
		}
		video_unmmap_driver_buffers(mjpeg_device.fd, mjpeg_device.mmap_buffer, MAX_BUFFER_FRAME);
		if (mjpeg_device.user_buffer.m.userptr)
			free((unsigned char *)mjpeg_device.user_buffer.m.userptr);
		video_close(mjpeg_device.fd);
		if (mjpeg_device.name) {
			free(mjpeg_device.name);
		}
		memset(&mjpeg_device, 0, sizeof(mjpeg_device));
		mjpeg_device.fd = -1;
	}
	
	return 0;
}

static void input_mjpeg_device_getframe_cb(evutil_socket_t fd, short events, void *arg)
{
	int ret = 0;

	if (video_getframe(mjpeg_device.fd, mjpeg_device.mmap_buffer, &mjpeg_device.user_buffer)) {
		goto schedule;
	}
	
	if (mjpeg_device.skip_frame > 0) {
		mjpeg_device.skip_frame--;
		goto schedule;
	}

	struct frame_buf *buf = NULL;
	buf = frame_buf_new();
	if (!buf) {
		goto schedule;
	}
	if (frame_buf_set_data(buf, (void *)mjpeg_device.user_buffer.m.userptr,
		mjpeg_device.user_buffer.bytesused,
		&(mjpeg_device.user_buffer.timestamp))) {
		frame_buf_free(buf);
		goto schedule;
	}
	
	int resolution = mjpeg_device.user_buffer.reserved;
	if (resolution == 0) {
		unsigned char *jpeg_data = (unsigned char *)mjpeg_device.user_buffer.m.userptr;
		resolution = ((jpeg_data[9] << 24) |
			(jpeg_data[10] << 16) | 
			(jpeg_data[7] << 8) |
			(jpeg_data[8] << 0));
	}

	// SLOG(SLOG_DEBUG, "mjpeg resolution %d x %d",  (resolution & 0xFFFF0000) >> 16, (resolution & 0xFFFF));
	
	if (resolution == H264_SIZE_HD) {
		if (frame_buf_queue_put(mjpeg_device.mjpeg_source_hd->queue, buf)) {
			frame_buf_free(buf);
			goto schedule;
		}
	} else if (resolution == H264_SIZE_VGA) {
		if (frame_buf_queue_put(mjpeg_device.mjpeg_source_vga->queue, buf)) {
			frame_buf_free(buf);
			goto schedule;
		}
	} else if (resolution == H264_SIZE_QVGA) {
		if (frame_buf_queue_put(mjpeg_device.mjpeg_source_qvga->queue, buf)) {
			frame_buf_free(buf);
			goto schedule;
		}
	} else if (resolution == H264_SIZE_QQVGA) {
		if (frame_buf_queue_put(mjpeg_device.mjpeg_source_qqvga->queue, buf)) {
			frame_buf_free(buf);
			goto schedule;
		}
	} else {
		SLOG(SLOG_ERROR, "unknow mjpeg resolution %d x %d",  (resolution & 0xFFFF0000) >> 16, 
			(resolution & 0xFFFF));
		frame_buf_free(buf);
		goto schedule;
	}

	mjpeg_device.nframe++;
	
schedule:
	event_add(mjpeg_device.ev, &(mjpeg_device.interval));
}


