#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/soundcard.h>
#include <sys/soundcard.h>
#include "common.h"
#include "oss_api.h"

int oss_open(char *device, int *fd)
{
	int dev_fd = -1;
	dev_fd = open(device, O_RDONLY | O_NONBLOCK);
	if (dev_fd < 0) {
		printf("Failed to open device %s\n", device);
		return -1;
	}
	*fd = dev_fd;
	
	return 0;
}

void oss_close(int fd)
{
	if (fd >= 0) {
		close(fd);
	}
}

int oss_test_device(int fd)
{
	int ret = 0;

	int caps;
	ret = ioctl(fd, SNDCTL_DSP_GETCAPS, &caps);
	if (ret < 0) {
		printf("Failed to get DSP Capability\n");
		return -1;
	}
	printf("\tDSP Capability: %08x\n", caps);

	int format_mask;
	ret = ioctl(fd, SNDCTL_DSP_GETFMTS, &format_mask);
	if (ret < 0) {
		printf("Failed to get DSP Format\n");
		return -1;
	}
	printf("\tDSP Format Mask: %08x\n", format_mask);
	
	return 0;
}

int oss_set_format(int fd, unsigned int fragmentsize, unsigned int format,
	int channels, int samplerate)
{
	int ret = 0;
	ret = ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &fragmentsize);
	if (ret < 0) {
		printf("Failed to set dsp fragment\n");
		return -1;
	}

	ret = ioctl(fd, SNDCTL_DSP_SETFMT, &format);
	if (ret < 0) {
		printf("Failed to set dsp format\n");
		return -1;
	}

	ret = ioctl(fd, SNDCTL_DSP_CHANNELS, &channels);
	if (ret < 0) {
		printf("Failed to set dsp channels\n");
		return -1;
	}

	ret = ioctl(fd, SNDCTL_DSP_SPEED, &samplerate);
	if (ret < 0) {
		printf("Failed to set dsp rate\n");
		return -1;
	}
	
	return 0;
}

int oss_get_max_buffer_size(int fd, int *size)
{
	int max_buffer_size = 0;
	int ret = 0;
	
	ret = ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &max_buffer_size);
	if (ret < 0) {
		printf("failed to get dsp block size\n");
		return -1;
	}

	*size = max_buffer_size;

	return 0;
}

int oss_trigger(int fd)
{
	int trigger = 0;
	int ret = 0;

	trigger = PCM_ENABLE_INPUT;
	ret = ioctl(fd, SNDCTL_DSP_SETTRIGGER, &trigger);
	if (ret) {
		printf("Failed to trigger sound device\n");
		return -1;
	}
	
	return 0;
}

int oss_untrigger(int fd)
{
	int trigger = 0;
	int ret = 0;

	trigger = ~PCM_ENABLE_INPUT;
	ret = ioctl(fd, SNDCTL_DSP_SETTRIGGER, &trigger);
	if (ret) {
		printf("Failed to un-trigger sound device\n");
		return -1;
	}

	return 0;
}

int oss_getframe(int fd, void *buf, int buf_size, int *length)
{
	int ret = 0;
	ret = read(fd, buf, buf_size);
	if (ret < 0) {
		if (errno != EAGAIN) {
			printf("Failed to read sound device %d\n", errno);
		}
		return -1;
	}
	*length = ret;

	return 0;
}
