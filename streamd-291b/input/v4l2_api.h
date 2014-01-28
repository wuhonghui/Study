#ifndef v4l2_api_H
#define v4l2_api_H

#include "videodev2.h"

#ifndef V4L2_PIX_FMT_MJPEG
#define V4L2_PIX_FMT_MJPEG	v4l2_fourcc('M','J','P','G')	/* Motion-JPEG */
#endif
#ifndef V4L2_PIX_FMT_H264
#define V4L2_PIX_FMT_H264 	v4l2_fourcc('H','2','6','4') 	/* H264 */
#endif
#ifndef V4L2_PIX_FMT_MP2T
#define V4L2_PIX_FMT_MP2T 	v4l2_fourcc('M','P','2','T') 	/* MPEG-2 TS */
#endif
#ifndef V4L2_PIX_FMT_MPEG
#define V4L2_PIX_FMT_MPEG	v4l2_fourcc('M', 'P', 'E', 'G') /* MPEG-1/2/4    */
#endif

int video_open(char *device, int *fd);
void video_close(int fd);
int video_test_device(int fd);
int video_set_format(int fd, int width, int height, __u32 pixelformat);
int video_set_fps(int fd, int fps);
int video_request_buffer(int fd, int n_buffers);
int video_query_buffer(int fd, struct v4l2_buffer *buffer, int i);
int video_dequeue_buffer(int fd, struct v4l2_buffer *buffer);
int video_queue_buffer(int fd, int i);
int video_stream_on(int fd);
int video_stream_off(int fd);
int video_mmap_driver_buffers(int fd, struct v4l2_buffer *mmap_buffer, int n_buffers);
int video_getframe(int fd, struct v4l2_buffer *mmap_buffer,struct v4l2_buffer *user_buffer);
void video_unmmap_driver_buffers(int fd, struct v4l2_buffer *mmap_buffer, int n_buffers);

#endif
