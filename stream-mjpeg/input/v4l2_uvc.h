#ifndef v4l2_uvc_H
#define v4l2_uvc_H
#ifdef __cplusplus
extern "C" {
#endif

#include "videodev2.h"

struct uvc_video_config;
struct uvc_video_device;


struct uvc_video_device * uvc_video_device_new();
void uvc_video_device_free(struct uvc_video_device *device);

int uvc_video_device_init(struct uvc_video_device *device, const char *name,
	struct uvc_video_config *config);
int uvc_video_device_close(struct uvc_video_device *device);

int uvc_video_device_streamon(struct uvc_video_device *device);
int uvc_video_device_streamoff(struct uvc_video_device *device);

int uvc_video_device_setparm_fps(struct uvc_video_device *device, int fps);

int uvc_video_device_nextframe(struct uvc_video_device *device);

int uvc_video_device_getfd(struct uvc_video_device *device);

struct buffer * uvc_video_device_getframe(struct uvc_video_device *device);

int uvc_video_device_isstreaming(struct uvc_video_device *device);

int uvc_video_device_test(struct uvc_video_device *device);


struct uvc_video_config * uvc_video_config_new();

int uvc_video_config_init(struct uvc_video_config *config,
	int width, int height, int fps);

void uvc_video_config_free(struct uvc_video_config * config);

int uvc_video_config_get_details(struct uvc_video_config * config,
	int *width, int *height, int *fps);

int uvc_video_config_get_fps(struct uvc_video_config *config);


#ifdef __cplusplus
}
#endif
#endif

