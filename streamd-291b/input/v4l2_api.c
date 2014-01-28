#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "videodev2.h"

int video_open(char *device, int *fd)
{
	int device_fd = -1;
	device_fd = open(device, O_RDWR | O_NONBLOCK);
	if (device_fd < 0) {
		printf("Failed to open device %s, %d\n", device, errno);
		return -1;
	}

	struct v4l2_capability cap;
	memset(&cap, 0, sizeof(struct v4l2_capability));
	if (ioctl(device_fd, VIDIOC_QUERYCAP, &cap) < 0) {
		printf("Failed to query device %s's capability, %d\n", device, errno);
		close(device_fd);
		return -1;
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		printf("Failed to open device, %s is not a capature device\n", device);
		close(device_fd);
		return -1;
	}

	*fd = device_fd;

	return 0;
}

void video_close(int fd)
{
	if (fd >= 0)
		close(fd);
}

int video_test_device(int fd)
{
	int ret = 0;

	if (fd < 0) {
		return -1;
	}

	struct v4l2_capability cap;
	if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
		printf("Failed to query device capability, %d\n", errno);
		return -1;
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		printf("Device is not a capature device\n");
		return -1;
	}

	printf("Device Driver: %s\n", cap.driver);
	printf("Device Card: %s\n", cap.card);
	printf("Device Bus: %s\n", cap.bus_info);
	printf("Device Version: %x\n", cap.version);
	printf("Device Capability: %x\n", cap.capabilities);

	/* enum format */
	struct v4l2_fmtdesc fmtdesc;
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmtdesc.index = 0;
	printf("\nEnum Device Available Format: \n");
	while (1) {
		ret = ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc);
		if (ret == 0) {
			printf("\tFormat: %d\n", fmtdesc.index);
			printf("\tfmtdesc.flags: %x\n", fmtdesc.flags);
			printf("\tfmtdesc.description: %s\n", fmtdesc.description);
			printf("\tfmtdesc.pixelformat: %c%c%c%c\n\n",
				(fmtdesc.pixelformat) & 0xFF,
				(fmtdesc.pixelformat >> 8) & 0xFF,
				(fmtdesc.pixelformat >> 16) & 0xFF,
				(fmtdesc.pixelformat >> 24) & 0xFF);
			fmtdesc.index++;
		} else {
			break;
		}
	};

	/* get device video format */
	struct v4l2_format format;
	memset(&format, 0, sizeof(format));
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	printf("\nCurrent Format: \n");
	ret = ioctl(fd, VIDIOC_G_FMT, &format);
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
	} else {
		printf("\tFailed to get device video format\n");
		return -1;
	}

	/* enum input */
	struct v4l2_input input;
	input.index = 0;
	printf("\nEnum Available Input: \n");
	while (1) {
		ret = ioctl(fd, VIDIOC_ENUMINPUT, &input);
		if (ret == 0) {
			printf("\tInput: %d\n", input.index);
			printf("\tInput.name: %s\n", input.name);
			printf("\tInput.type: %x\n", input.type);
			printf("\tInput.audioset: %x\n", input.audioset);
			printf("\tInput.tuner: %x\n", input.tuner);
			printf("\tInput.status: %x\n", input.status);
			printf("\tInput.capabilities: %x\n\n", input.capabilities);

			input.index++;
		} else {
			break;
		}
	};

	int index = 0;
	ret = ioctl(fd, VIDIOC_G_INPUT, &index);
	if (ret == 0) {
		printf("Current Input: %d\n", index);
	} else {
		printf("Failed to get current input\n");
		return -1;
	}

	struct v4l2_standard standard;
	ret = ioctl(fd, VIDIOC_G_STD, &standard);
	if (ret == 0) {
		printf("Standard.index: %d\n", standard.index);
		printf("Standard.name: %s\n", standard.name);
	}

	struct v4l2_queryctrl ctrl;
	memset(&ctrl, 0, sizeof(ctrl));
	printf("\nAvailable User Ctrl: \n");
	for (ctrl.id = V4L2_CID_BASE; ctrl.id < V4L2_CID_LASTP1; ctrl.id++) {
		ret = ioctl(fd, VIDIOC_QUERYCTRL, &ctrl);
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
					ret = ioctl(fd, VIDIOC_QUERYMENU, &menu);
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

	printf("\nAvailable Private Ctrl: \n");
	for (ctrl.id = V4L2_CID_PRIVATE_BASE; ; ctrl.id++) {
		ret = ioctl(fd, VIDIOC_QUERYCTRL, &ctrl);
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
					ret = ioctl(fd, VIDIOC_QUERYMENU, &menu);
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

	printf("\nAvailable Extend Ctrl: \n");
	ctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;
	while(1) {
		ret = ioctl(fd, VIDIOC_QUERYCTRL, &ctrl);
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
	printf("\nCrop Capability:\n");
	ret = ioctl(fd, VIDIOC_CROPCAP, &cropcap);
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
	printf("\nCurrent Crop:\n");
	ret = ioctl(fd, VIDIOC_G_CROP, &crop);
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
	ret = ioctl(fd, VIDIOC_G_PARM, &streamparm);
	printf("\nCurrent Stream Parm:\n");
	if (ret == 0) {
		printf("\tstreamparm.parm.capature.capability: %x\n", streamparm.parm.capture.capability);
		printf("\tstreamparm.parm.capature.capturemode: %x\n", streamparm.parm.capture.capturemode);
		printf("\tstreamparm.parm.capature.timeperframe.numerator: %d\n", streamparm.parm.capture.timeperframe.numerator);
		printf("\tstreamparm.parm.capature.timeperframe.denominator: %d\n", streamparm.parm.capture.timeperframe.denominator);
		printf("\tstreamparm.parm.capature.extendedmode: %d\n", streamparm.parm.capture.extendedmode);
		printf("\tstreamparm.parm.capature.readbuffers: %d\n", streamparm.parm.capture.readbuffers);
	}

	return 0;
}

/*
	V4L2_PIX_FMT_MJPEG
 */

int video_set_format(int fd, int width, int height, __u32 pixelformat)
{
	struct v4l2_format fmt;
	memset(&fmt, 0, sizeof(struct v4l2_format));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = width;
	fmt.fmt.pix.height = height;
	fmt.fmt.pix.pixelformat = pixelformat;

	if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
		printf("Failed to set format %dx%d, %c%c%c%c\n", 
			fmt.fmt.pix.width,
			fmt.fmt.pix.height,
			(fmt.fmt.pix.pixelformat) & 0xFF,
			(fmt.fmt.pix.pixelformat >> 8) & 0xFF,
			(fmt.fmt.pix.pixelformat >> 16) & 0xFF,
			(fmt.fmt.pix.pixelformat >> 24) & 0xFF);
		return -1;
	}

	return 0;
}

int video_set_fps(int fd, int fps)
{
	struct v4l2_streamparm parm;
	memset(&parm, 0, sizeof(struct v4l2_streamparm));
	parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	parm.parm.capture.timeperframe.numerator = 1;
	parm.parm.capture.timeperframe.denominator = fps;
	if (ioctl(fd, VIDIOC_S_PARM, &parm) < 0) {
		printf("Failed to set video fps to %d\n", fps);
		return -1;
	}
	
	return 0;
}

int video_request_buffer(int fd, int n_buffers)
{
	struct v4l2_requestbuffers req;
	memset(&req, 0, sizeof(struct v4l2_requestbuffers));
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	req.count = n_buffers;
	if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
		printf("Failed to request %d frame buffers\n", n_buffers);
		return -1;
	}
	
	return 0;
}

int video_query_buffer(int fd, struct v4l2_buffer *buffer, int i)
{
	memset(buffer, 0, sizeof(struct v4l2_buffer));
	buffer->index = i;
	buffer->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buffer->memory = V4L2_MEMORY_MMAP;
	if (ioctl(fd, VIDIOC_QUERYBUF, buffer) < 0) {
		printf("Failed to query frame buffer at index %d, %d\n", i, errno);
		return -1;
	}

	return 0;
}

int video_dequeue_buffer(int fd, struct v4l2_buffer *buffer)
{
	buffer->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buffer->memory = V4L2_MEMORY_MMAP;
	buffer->bytesused = 0;

	if (ioctl(fd, VIDIOC_DQBUF, buffer)) {
		buffer->bytesused = 0;
		if (errno == EAGAIN) {
			return 0;
		} else {
			printf("Failed to dequeue frame buffer %d\n", errno);
			return -1;
		}
	}
	
	return 0;
}

int video_queue_buffer(int fd, int i)
{
	struct v4l2_buffer buffer;
	memset(&buffer, 0, sizeof(struct v4l2_buffer));
	buffer.index = i;
	buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buffer.memory = V4L2_MEMORY_MMAP;
	if (ioctl(fd, VIDIOC_QBUF, &buffer) < 0) {
		if (errno == EBUSY) {
			return 0;
		} else {
			printf("Failed to queue frame buffer at index %d, %d\n", i, errno);
			return -1;
		}
	}

	return 0;
}

int video_stream_on(int fd)
{
	int type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
		printf("Failed to enable video capature(stream on)\n");
		return -1;
	}
	return 0;
}

int video_stream_off(int fd)
{
	int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (ioctl(fd, VIDIOC_STREAMOFF, &type) < 0) {
		printf("Failed to disable video capature(stream off)\n");
		return -1;
	}
	return 0;
}

int video_mmap_driver_buffers(int fd, struct v4l2_buffer *mmap_buffer, int n_buffers)
{
	int i = 0;
	struct v4l2_buffer buffer;
	
	for (i = 0; i < n_buffers; i++) {
		if (video_query_buffer(fd, &buffer, i)) {
			return -1;
		}

		mmap_buffer[i].m.userptr = (unsigned long)mmap(NULL,buffer.length,
				PROT_READ, MAP_SHARED, fd, buffer.m.offset);
		if (mmap_buffer[i].m.userptr == 0) {
			printf("Failed to mmap memory at frame index %d\n", i);
			return -1;
		}
		mmap_buffer[i].length = buffer.length;
	}

	return 0;
}

int video_getframe(int fd, struct v4l2_buffer *mmap_buffer,struct v4l2_buffer *user_buffer)
{
	struct v4l2_buffer ioctl_buf;
	ioctl_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl_buf.memory = V4L2_MEMORY_MMAP;
	ioctl_buf.bytesused = 0;
	
	if (ioctl(fd, VIDIOC_DQBUF, &ioctl_buf)) {
		if (errno != EAGAIN) {
			printf("Failed to dequeue buffer %d\n", errno);
		}
		return -1;
	}
	
	if (ioctl_buf.bytesused > 0) {
		user_buffer->timestamp = ioctl_buf.timestamp;
		user_buffer->bytesused = ioctl_buf.bytesused;
		user_buffer->reserved = ioctl_buf.reserved;
		memmove((void *)user_buffer->m.userptr, (void *)mmap_buffer[ioctl_buf.index].m.userptr, 
			ioctl_buf.bytesused);
	}

	if (ioctl(fd, VIDIOC_QBUF, &ioctl_buf) < 0) {
		printf("Failed to queue buffer at index %d, %d\n", ioctl_buf.index, errno);
		return -1;
	}
	
	return 0;
}

void video_unmmap_driver_buffers(int fd, struct v4l2_buffer *mmap_buffer, int n_buffers)
{
	int i = 0;
	for (i = 0; i < n_buffers; ++i) {
		if (0 != mmap_buffer[i].m.userptr) {
			munmap((void *)mmap_buffer[i].m.userptr, mmap_buffer[i].length);
			mmap_buffer[i].m.userptr = 0;
			mmap_buffer[i].length = 0;
		}
	}
}
