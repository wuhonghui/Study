#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include "common.h"
#include "v4l2_uvc.h"
#include "utils/timing.h"

#define frame_buffer_count 4

struct resolution{
	int width;
	int height;
};

struct uvc_video_config{
	struct resolution resolution;
	int fps;
};

struct uvc_video_device{
	int fd;
	char name[24];

	int isstreaming;
	struct buffer frame;

	struct buffer frame_buffer[frame_buffer_count];
	struct v4l2_buffer v4l2_buffer;
	int	max_buffer_size;

	struct uvc_video_config *config;
};

struct uvc_video_device * uvc_video_device_new()
{
	struct uvc_video_device *device = NULL;
	device = (struct uvc_video_device *)malloc(sizeof(struct uvc_video_device));
	if (device) {
		memset(device, 0, sizeof(struct uvc_video_device));
	} else {
		printf("Could not create new uvc_video_device\n");
		return NULL;
	}

	return device;
}

void uvc_video_device_free(struct uvc_video_device *device)
{
	if (device) {
		free(device);
	}
}

int uvc_video_device_init(struct uvc_video_device *device, const char *name,struct uvc_video_config *config)
{
	int ret = 0;

	if (!device || !config)
		return -1;

	if (!name) {
		strcpy(device->name, "/dev/video0");
	} else {
		strcpy(device->name, name);
	}

	/* open device */
	device->fd = open(device->name, O_RDWR | O_NONBLOCK);
	if (device->fd < 0) {
		printf("failed to open device %s\n", device->name);
		return -1;
	}

	/* query capability*/
	struct v4l2_capability caps;
	memset(&caps, 0, sizeof(struct v4l2_capability));
	if (ioctl(device->fd, VIDIOC_QUERYCAP, &caps) < 0) {
		printf("failed to querycap\n");
		goto error;
	}
	if (!(caps.capabilities & V4L2_CAP_VIDEO_CAPTURE)
		|| !(caps.capabilities & V4L2_CAP_STREAMING)) {
		printf("not a CAPTURE or STREAMING device\n");
		goto error;
	}

	/* set image format */
	struct v4l2_format fmt;
	memset(&fmt, 0, sizeof(struct v4l2_format));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = config->resolution.width;
	fmt.fmt.pix.height = config->resolution.height;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;

	if (ioctl(device->fd, VIDIOC_S_FMT, &fmt) < 0) {
		printf("failed to set format\n");
		goto error;
	}

	if (uvc_video_device_setparm_fps(device, config->fps)) {
		goto error;
	}

	/* request buffers */
	struct v4l2_requestbuffers req;
	memset(&req, 0, sizeof(struct v4l2_requestbuffers));
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	req.count = frame_buffer_count;
	if (ioctl(device->fd, VIDIOC_REQBUFS, &req) < 0) {
		printf("failed to request frame buffers\n");
		goto error;
	}

	/* mmap buffers */
	int i = 0;
	for (i = 0; i < frame_buffer_count; ++i) {
		memset(&device->v4l2_buffer, 0, sizeof(struct v4l2_buffer));
		device->v4l2_buffer.index = i;
		device->v4l2_buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		device->v4l2_buffer.memory = V4L2_MEMORY_MMAP;
		if (ioctl(device->fd, VIDIOC_QUERYBUF, &(device->v4l2_buffer)) < 0) {
			printf("failed to query frame buffer %d\n", i);
			goto error;
		}
		device->frame_buffer[i].data = (unsigned char *)mmap(NULL, device->v4l2_buffer.length,
				PROT_READ, MAP_SHARED, device->fd, device->v4l2_buffer.m.offset);
		if (device->frame_buffer[i].data == NULL) {
			printf("failed to mmap memory\n");
			goto error;
		}
		device->frame_buffer[i].size = device->v4l2_buffer.length;
	}

	/* malloc frame buffer to store each frame read from driver */
	device->max_buffer_size = device->frame_buffer[0].size;
	if (!device->frame.data) {
		printf("frame buffer size(max): %d\n", device->max_buffer_size);
		device->frame.data = (unsigned char *)malloc(device->max_buffer_size);
		if (!device->frame.data) {
			printf("Could not create frame for device\n");
			goto error;
		}
		device->frame.size = 0;
	}

	device->config = config;

	dbprintf("init video device\n");

	return 0;
error:
	device->isstreaming = 0;

	for (i = 0; i <= frame_buffer_count; ++i) {
		if (NULL != device->frame_buffer[i].data) {
			munmap(device->frame_buffer[i].data, device->frame_buffer[i].size);
			device->frame_buffer[i].data = NULL;
			device->frame_buffer[i].size = 0;
		}
	}

	if (device->frame.data) {
		free(device->frame.data);
		device->frame.data = NULL;
		device->frame.size = 0;
	}

	if (device->fd > 0) {
		close(device->fd);
		device->fd = 0;
	}

	return -1;
}


int uvc_video_device_close(struct uvc_video_device *device)
{
	if (device->isstreaming) {
		uvc_video_device_streamoff(device);
	}

	int i;
	for (i = 0; i < frame_buffer_count; ++i) {
		if (NULL != device->frame_buffer[i].data) {
			munmap(device->frame_buffer[i].data, device->frame_buffer[i].size);
			device->frame_buffer[i].data = NULL;
			device->frame_buffer[i].size = 0;
		}
	}

	if (device->frame.data) {
		free(device->frame.data);
		device->frame.data = NULL;
		device->frame.size = 0;
	}

	if (device->fd > 0) {
		close(device->fd);
		device->fd = 0;
	}

	device->config = NULL;

	dbprintf("close video device\n");

	return 0;
}

int uvc_video_device_streamon(struct uvc_video_device *device)
{
	if (!device || device->fd < 0)
		return -1;

	if (device->isstreaming)
		return 0;

	int i = 0;
	for (i = 0; i < frame_buffer_count; ++i) {
		memset(&device->v4l2_buffer, 0, sizeof(struct v4l2_buffer));
		device->v4l2_buffer.index = i;
		device->v4l2_buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		device->v4l2_buffer.memory = V4L2_MEMORY_MMAP;
		if (ioctl(device->fd, VIDIOC_QBUF, &(device->v4l2_buffer)) < 0) {
			if (errno == EBUSY) {
				continue;	/* video buffer already enabled */
			} else {
				printf("failed to queue buffer %d\n", errno);
			}
		}
	}

	int type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(device->fd, VIDIOC_STREAMON, &type) < 0) {
		printf("failed to enable capature\n");
		return -1;
	}
	device->isstreaming = 1;

	dbprintf("set video stream on\n");

	return 0;
}

int uvc_video_device_streamoff(struct uvc_video_device *device)
{
	if (!device || device->fd < 0)
		return -1;

	if (!device->isstreaming)
		return 0;

	device->isstreaming = 0;

	int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(device->fd, VIDIOC_STREAMOFF, &type) < 0) {
		printf("failed to disable capature\n");
		return -1;
	}

	dbprintf("set video stream off\n");

	return 0;
}

int uvc_video_device_setparm_fps(struct uvc_video_device *device, int fps)
{
	if (!device || device->fd <= 0)
		return -1;

	/* set stream parm */
	struct v4l2_streamparm parm;
	memset(&parm, 0, sizeof(struct v4l2_streamparm));
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = fps;
	if (ioctl(device->fd, VIDIOC_S_PARM, &parm) < 0) {
		printf("failed to set streaming parm\n");
		return -1;
	}

	if (device->config)
		device->config->fps = fps;

	dbprintf("set video fps %d\n", fps);

	return 0;
}

int uvc_video_device_getfd(struct uvc_video_device *device)
{
	return device->fd;
}

int uvc_video_device_nextframe(struct uvc_video_device *device)
{
	if (!device || device->fd < 0)
		return -1;

	if (!device->isstreaming)
		return -1;

	int ret = 0;
	static int eagain_count = 0;
	static int data_corrupt_count = 0;
	static int incomplete_count = 0;
	struct v4l2_buffer *vbuf = &(device->v4l2_buffer);

	vbuf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	vbuf->memory = V4L2_MEMORY_MMAP;

	ret = ioctl(device->fd, VIDIOC_DQBUF, vbuf);
	if (ret < 0) {
		if (errno != EAGAIN) {
			printf("failed to get buffer %d\n", errno);
		} else {
			if (eagain_count++ > 200) {
				printf("video device return too many EAGAIN\n");
				eagain_count = 0;
				ret = ERR_DEV;
			}
		}
		/* no frame buffer is dequeued, return here */
		return ret;
	}

	/* avoid crash, this may happens when streamon on after streamoff. */
	if (vbuf->bytesused > 0x200) {
		unsigned char *imgdata = device->frame_buffer[vbuf->index].data;

		if (imgdata[0] == 0xFF && imgdata[1] == 0xD8) {
			memmove(device->frame.data, imgdata, vbuf->bytesused);
			device->frame.size = vbuf->bytesused;
		} else {
			printf("data corrupted, size %d\n", vbuf->bytesused);
			ret = -1;
			if (data_corrupt_count++ > 200) {
				printf("video device data corrupted too many\n");
				data_corrupt_count = 0;
				ret = ERR_DEV;
			}
			goto queue;
		}

		/* (DEBUG) save to jpg file in sequence */
		/* savejpg(device->frame.data, device->frame.size); */
	} else {
		printf("image incomplete %d\n", vbuf->bytesused);
		ret = -1;
		if (incomplete_count++ > 200) {
			printf("video device data incomplete too many\n");
			incomplete_count = 0;
			ret = ERR_DEV;
		}
		goto queue;
	}

	/* reset counter if got a correct image */
	eagain_count = 0;
	data_corrupt_count = 0;
	incomplete_count = 0;

queue:
	if (ioctl(device->fd, VIDIOC_QBUF, vbuf) < 0) {
		printf("failed to queue buf\n");
		return -1;
	}

	return ret;
}

struct buffer * uvc_video_device_getframe(struct uvc_video_device *device)
{
      return &(device->frame);
}


int uvc_video_device_isstreaming(struct uvc_video_device *device)
{
	return device->isstreaming;
}

int uvc_video_device_test(struct uvc_video_device *device)
{
	if (!device)
		return -1;

	if (device->fd < 0)
		return -1;

	int ret = 0;

	struct v4l2_capability caps;
	memset(&caps, 0, sizeof(struct v4l2_capability));
	ret = ioctl(device->fd, VIDIOC_QUERYCAP, &caps);
	if (ret == 0) {
		printf("Device Driver: %s\n", caps.driver);
		printf("Device Card: %s\n", caps.card);
		printf("Device Bus: %s\n", caps.bus_info);
		printf("Device Version: %x\n", caps.version);
		printf("Device Capability: %x\n", caps.capabilities);
	}

	/* enum format */
	struct v4l2_fmtdesc fmtdesc;
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmtdesc.index = 0;
	printf("Enum Available Format: \n");
	while (1) {
		ret = ioctl(device->fd, VIDIOC_ENUM_FMT, &fmtdesc);
		if (ret == 0) {
			printf("  Format: %d\n", fmtdesc.index);
			printf("  fmtdesc.flags: %x\n", fmtdesc.flags);
			printf("  fmtdesc.description: %s\n", fmtdesc.description);
			printf("  fmtdesc.pixelformat: %c%c%c%c\n",
				(fmtdesc.pixelformat) & 0xFF,
				(fmtdesc.pixelformat >> 8) & 0xFF,
				(fmtdesc.pixelformat >> 16) & 0xFF,
				(fmtdesc.pixelformat >> 24) & 0xFF);
			fmtdesc.index++;
		} else {
			break;
		}
	};

	struct v4l2_format format;
	memset(&format, 0, sizeof(format));
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	printf("Current Format: \n");
	ret = ioctl(device->fd, VIDIOC_G_FMT, &format);
	if (ret == 0) {
		printf("\tformat.fmt.pix.width: %d\n", format.fmt.pix.width);
		printf("\tformat.fmt.pix.height: %d\n", format.fmt.pix.height);
		printf("\tformat.fmt.pix.pixelformat: %c%c%c%c\n",
				(format.fmt.pix.pixelformat) & 0xFF,
				(format.fmt.pix.pixelformat >> 8) & 0xFF,
				(format.fmt.pix.pixelformat >> 16) & 0xFF,
				(format.fmt.pix.pixelformat >> 24) & 0xFF);
		printf("\tformat.fmt.pix.field: %d\n", format.fmt.pix.field);
		printf("\tformat.fmt.pix.bytesperline: %d\n", format.fmt.pix.bytesperline);
		printf("\tformat.fmt.pix.sizeimage: %d\n", format.fmt.pix.sizeimage);
		printf("\tformat.fmt.pix.colorspace: %d\n", format.fmt.pix.colorspace);
	}

	/* enum input */
	struct v4l2_input input;
	input.index = 0;
	printf("Enum Available Input: \n");
	while (1) {
		ret = ioctl(device->fd, VIDIOC_ENUMINPUT, &input);
		if (ret == 0) {
			printf("  Input: %d\n", input.index);
			printf("  Input.name: %s\n", input.name);
			printf("  Input.type: %x\n", input.type);
			printf("  Input.audioset: %x\n", input.audioset);
			printf("  Input.tuner: %x\n", input.tuner);
			printf("  Input.status: %x\n", input.status);
			printf("  Input.capabilities: %x\n", input.capabilities);

			input.index++;
		} else {
			break;
		}
	};

	int index = 0;
	ret = ioctl(device->fd, VIDIOC_G_INPUT, &index);
	if (ret == 0) {
		printf("Current Input: %d\n", index);
	}

	struct v4l2_standard standard;
	ret = ioctl(device->fd, VIDIOC_G_STD, &standard);
	if (ret == 0) {
		printf("Standard.index: %d\n", standard.index);
		printf("Standard.name: %s\n", standard.name);
	}

	struct v4l2_queryctrl ctrl;
	memset(&ctrl, 0, sizeof(ctrl));
	printf("Available User Ctrl: \n");
	for (ctrl.id = V4L2_CID_BASE; ctrl.id < V4L2_CID_LASTP1; ctrl.id++) {
		ret = ioctl(device->fd, VIDIOC_QUERYCTRL, &ctrl);
		if (ret == 0) {
			if (ctrl.flags & V4L2_CTRL_FLAG_DISABLED)
				continue;
			printf("\tctrl.id: %x\n", ctrl.id);
			printf("\tctrl.name: %s\n", ctrl.name);
			printf("\tctrl.type: %d\n", ctrl.type);
			printf("\tctrl.value: %d ~ %d\n", ctrl.minimum, ctrl.maximum);
			printf("\tctrl.value: step %d, default %d\n", ctrl.step, ctrl.default_value);
			printf("\tctrl.flags: %x\n", ctrl.flags);

			if (ctrl.type == V4L2_CTRL_TYPE_MENU) {
				printf ("\tMenu items:\n");
				struct v4l2_querymenu menu;
				memset(&menu, 0, sizeof(menu));
				menu.id = ctrl.id;
				for (menu.index = ctrl.minimum; menu.index <= ctrl.maximum; menu.index++) {
					ret = ioctl(device->fd, VIDIOC_QUERYMENU, &menu);
					if (ret == 0) {
						printf("\t\tmenu.name: %s\n", menu.name);
					}
				}
			}
			printf("\n");
		} else {
			if (errno == EINVAL) {
				continue;
			} else {
				printf("unknow error on ioctl VIDIOC_QUERYCTRL\n");
			}
		}
	}

	printf("Available Private Ctrl: \n");
	for (ctrl.id = V4L2_CID_PRIVATE_BASE; ; ctrl.id++) {
		ret = ioctl(device->fd, VIDIOC_QUERYCTRL, &ctrl);
		if (ret == 0) {
			if (ctrl.flags & V4L2_CTRL_FLAG_DISABLED)
				continue;
			printf("\tctrl.id: %x\n", ctrl.id);
			printf("\tctrl.name: %s\n", ctrl.name);
			printf("\tctrl.type: %d\n", ctrl.type);
			printf("\tctrl.value: %d ~ %d\n", ctrl.minimum, ctrl.maximum);
			printf("\tctrl.value: step %d, default %d\n", ctrl.step, ctrl.default_value);
			printf("\tctrl.flags: %x\n", ctrl.flags);

			if (ctrl.type == V4L2_CTRL_TYPE_MENU) {
				printf ("\tMenu items:\n");
				struct v4l2_querymenu menu;
				memset(&menu, 0, sizeof(menu));
				menu.id = ctrl.id;
				for (menu.index = ctrl.minimum; menu.index <= ctrl.maximum; menu.index++) {
					ret = ioctl(device->fd, VIDIOC_QUERYMENU, &menu);
					if (ret == 0) {
						printf("\t\tmenu.name: %s\n", menu.name);
					}
				}
			}
			printf("\n");
		} else {
			if (errno == EINVAL) {
				break;
			} else {
				printf("unknow error on ioctl VIDIOC_QUERYCTRL\n");
			}
		}
	}

	printf("Available Extend Ctrl: \n");
	ctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;
	while(1) {
		ret = ioctl(device->fd, VIDIOC_QUERYCTRL, &ctrl);
		if (ret == 0) {
			if (ctrl.flags & V4L2_CTRL_FLAG_DISABLED)
				continue;
			ctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;

			printf("\tctrl.id: %x\n", ctrl.id);
			printf("\tctrl.name: %s\n", ctrl.name);
			printf("\tctrl.type: %d\n", ctrl.type);
			printf("\tctrl.value: %d ~ %d\n", ctrl.minimum, ctrl.maximum);
			printf("\tctrl.value: step %d, default %d\n", ctrl.step, ctrl.default_value);
			printf("\tctrl.flags: %x\n", ctrl.flags);
			printf("\n");
		} else {
			if (errno == EINVAL) {
				break;
			} else {
				printf("unknow error on ioctl VIDIOC_QUERYCTRL\n");
			}
		}
	}

	struct v4l2_cropcap cropcap;
	memset(&cropcap, 0, sizeof(cropcap));
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	printf("Crop Capability:\n");
	ret = ioctl(device->fd, VIDIOC_CROPCAP, &cropcap);
	if (ret == 0) {
		printf("\tcropcap.bound.left: %d\n", cropcap.bounds.left);
		printf("\tcropcap.bound.top: %d\n", cropcap.bounds.top);
		printf("\tcropcap.bound.width: %d\n", cropcap.bounds.width);
		printf("\tcropcap.bound.height: %d\n", cropcap.bounds.height);

		printf("\tcropcap.defrect.left: %d\n", cropcap.defrect.left);
		printf("\tcropcap.defrect.top: %d\n", cropcap.defrect.top);
		printf("\tcropcap.defrect.width: %d\n", cropcap.defrect.width);
		printf("\tcropcap.defrect.height: %d\n", cropcap.defrect.height);
		printf("\tcropcap.pixelaspect.numerator: %d\n", cropcap.pixelaspect.numerator);
		printf("\tcropcap.pixelaspect.denominator: %d\n", cropcap.pixelaspect.denominator);
	} else {
		printf("Cropping not supported.\n");
	}

	struct v4l2_crop crop;
	memset(&crop, 0, sizeof(crop));
	crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	printf("Current Crop:\n");
	ret = ioctl(device->fd, VIDIOC_G_CROP, &crop);
	if (ret == 0) {
		printf("\tcrop.c.left: %d\n", crop.c.left);
		printf("\tcrop.c.top: %d\n", crop.c.top);
		printf("\tcrop.c.width: %d\n", crop.c.width);
		printf("\tcrop.c.height: %d\n", crop.c.height);
	} else {
		printf("Cropping not supported.\n");
	}

	struct v4l2_streamparm streamparm;
	memset(&streamparm, 0, sizeof(streamparm));
	streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(device->fd, VIDIOC_G_PARM, &streamparm);
	printf("Current Stream Parm:\n");
	if (ret == 0) {
		printf("\tstreamparm.parm.capature.capability: %d\n", streamparm.parm.capture.capability);
		printf("\tstreamparm.parm.capature.capturemode: %x\n", streamparm.parm.capture.capturemode);
		printf("\tstreamparm.parm.capature.timeperframe.numerator: %d\n", streamparm.parm.capture.timeperframe.numerator);
		printf("\tstreamparm.parm.capature.timeperframe.denominator: %d\n", streamparm.parm.capture.timeperframe.denominator);
		printf("\tstreamparm.parm.capature.extendedmode: %d\n", streamparm.parm.capture.extendedmode);
		printf("\tstreamparm.parm.capature.readbuffers: %d\n", streamparm.parm.capture.readbuffers);
	}

	return 0;
}

/*/////////////////////////////////////////////////////////////////////////
 config functions
/////////////////////////////////////////////////////////////////////////*/

struct uvc_video_config * uvc_video_config_new()
{
	struct uvc_video_config * config = (struct uvc_video_config *)malloc(sizeof(struct uvc_video_config));
	if (config) {
		return config;
	} else {
		printf("Could not create new uvc_video_config\n");
		return NULL;
	}

	return config;
}

int uvc_video_config_init(struct uvc_video_config *config,
	int width, int height, int fps)
{
	if (!config)
		return -1;

	config->fps = fps;
	config->resolution.width = width;
	config->resolution.height = height;

	return 0;
}

void uvc_video_config_free(struct uvc_video_config * config)
{
	if (config)
		free(config);
}

int uvc_video_config_get_details(struct uvc_video_config * config,
	int *width, int *height, int *fps)
{
	if (!config)
		return -1;

	if (width) {
		*width = config->resolution.width;
	}

	if (height) {
		*height = config->resolution.height;
	}

	if (fps) {
		*fps = config->fps;
	}

	return 0;
}

int uvc_video_config_get_fps(struct uvc_video_config *config)
{
	return config->fps;
}
