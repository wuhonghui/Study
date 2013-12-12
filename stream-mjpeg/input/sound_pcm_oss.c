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
#include "sound_pcm_oss.h"
#include "utils/timing.h"

struct snd_pcm_oss_config{
	int format;
	int samplebits;
	int samplerate;
	int channels;
	int fragmentsize;
};

struct snd_pcm_oss_device{
	int fd;
	char name[24];

	int isstreaming;
	struct buffer fragment;
	int max_buffer_size;

	struct snd_pcm_oss_config *config;
};

struct snd_pcm_oss_device * snd_pcm_oss_device_new()
{
	struct snd_pcm_oss_device *device = NULL;
	device = (struct snd_pcm_oss_device *)malloc(sizeof(struct snd_pcm_oss_device));
	if (!device) {
		printf("Could not create sound pcm device\n");
		return NULL;
	}

	memset(device, 0, sizeof(struct snd_pcm_oss_device));
	return device;
}

void snd_pcm_oss_device_free(struct snd_pcm_oss_device *device)
{
	if (device)
		free(device);
}

int snd_pcm_oss_device_init(struct snd_pcm_oss_device *device, const char *name, struct snd_pcm_oss_config *config)
{
	int ret = 0;

	if (!device || !config)
		return -1;

	if (!name) {
		strcpy(device->name, "/dev/dsp");
	} else {
		strcpy(device->name, name);
	}

	/* open device */
	device->fd = open(device->name, O_RDONLY | O_NONBLOCK);
	if (device->fd < 0) {
		printf("failed to open device %s\n", device->name);
		return -1;
	}

	/* set fragment */
	if (config->fragmentsize == 0) {
		config->fragmentsize = 0x7fff000b;
	}
	ret = ioctl(device->fd, SNDCTL_DSP_SETFRAGMENT, &(config->fragmentsize));
	if (ret < 0) {
		printf("failed to set dsp fragment\n");
		goto failed;
	}

	/* set format */
	if (config->format == 0) {
		config->format = AFMT_U8;
	}
	ret = ioctl(device->fd, SNDCTL_DSP_SETFMT, &(config->format));
	if (ret < 0) {
		printf("failed to set dsp format\n");
		goto failed;
	}

	/* set channels */
	if (config->channels == 0) {
		config->channels = 1;
	}
	ret = ioctl(device->fd, SNDCTL_DSP_CHANNELS, &(config->channels));
	if (ret < 0) {
		printf("failed to set dsp channels\n");
		goto failed;
	}

	/* set sample rate */
	if (config->samplerate == 0) {
		config->samplerate = 8000;
	}
	ret = ioctl(device->fd, SNDCTL_DSP_SPEED, &(config->samplerate));
	if (ret < 0) {
		printf("failed to set dsp rate\n");
		goto failed;
	}

	/* get block size and malloc sound buffer */
	ret = ioctl(device->fd, SNDCTL_DSP_GETBLKSIZE, &(device->max_buffer_size));
	if (ret < 0) {
		printf("failed to get dsp block size\n");
		goto failed;
	}

	dbprintf("SNDCTL_DSP_GETBLKSIZE is %d\n", device->max_buffer_size);

	device->fragment.data = malloc(device->max_buffer_size);
	device->fragment.size = 0;
	if (!device->fragment.data) {
		printf("failed to malloc sound fragment buffer\n");
		goto failed;
	}

	int trigger;
	trigger = ~PCM_ENABLE_INPUT;
	ret = ioctl(device->fd, SNDCTL_DSP_SETTRIGGER, &trigger);
	if (ret) {
		printf("failed to un-trigger sound device\n");
		goto failed;
	}
	device->isstreaming = 0;

	device->config = config;

	return 0;

failed:
	device->isstreaming = 0;

	if (device->fragment.data) {
		free(device->fragment.data);
		device->fragment.data = NULL;
	}

	if (device->fd) {
		close(device->fd);
		device->fd = 0;
	}

	return -1;
}


int snd_pcm_oss_device_close(struct snd_pcm_oss_device *device)
{
	if (device->isstreaming) {
		snd_pcm_oss_device_untrigger(device);
	}

	if (device->fragment.data) {
		free(device->fragment.data);
		device->fragment.data = NULL;
	}

	if (device->fd) {
		close(device->fd);
		device->fd = 0;
	}

	device->config = NULL;

	dbprintf("close sound device\n");

	return 0;
}


int snd_pcm_oss_device_trigger(struct snd_pcm_oss_device * device)
{
	int trigger = 0;
	int ret = 0;

	trigger = PCM_ENABLE_INPUT;
	ret = ioctl(device->fd, SNDCTL_DSP_SETTRIGGER, &trigger);
	if (ret) {
		printf("failed to trigger sound device\n");
		return -1;
	}

	device->isstreaming = 1;

	return 0;
}

int snd_pcm_oss_device_untrigger(struct snd_pcm_oss_device * device)
{
	int trigger = 0;
	int ret = 0;

	device->isstreaming = 0;

	trigger = ~PCM_ENABLE_INPUT;
	ret = ioctl(device->fd, SNDCTL_DSP_SETTRIGGER, &trigger);
	if (ret) {
		printf("failed to un-trigger sound device\n");
		return -1;
	}

	return 0;
}

int snd_pcm_oss_device_getfd(struct snd_pcm_oss_device * device)
{
	if (!device)
		return -1;

	return device->fd;
}


int snd_pcm_oss_device_next_fragment(struct snd_pcm_oss_device * device)
{
	if (!device || device->fd <= 0 || !device->fragment.data) {
		printf("unexpected exception in next fragment\n");
		return -1;
	}

	if (!device->isstreaming) {
		printf("sound device is not streaming\n");
		return -1;
	}

	int ret = 0;
	static int eagain_count = 0;
	ret = read(device->fd, device->fragment.data, device->max_buffer_size);
	if (ret < 0) {
		if (errno != EAGAIN) {
			printf("failed to read sound device %d\n", errno);
		} else {
			if (eagain_count++ > 80) {
				printf("sound device return too many EAGAIN\n");
				eagain_count = 0;
				/* re-trigger capture */
				snd_pcm_oss_device_untrigger(device);
				snd_pcm_oss_device_trigger(device);
			}
		}
		return -1;
	}
	eagain_count = 0;

	device->fragment.size = ret;

#if 0
	struct timeval tv;
	gettimeofday(&tv, NULL);
	printf("%u.%u read %d sound data.\n", tv.tv_sec, tv.tv_usec, sound->sound_buffer_size);
#endif

	return 0;
}

struct buffer * snd_pcm_oss_device_getfrag(struct snd_pcm_oss_device *device)
{
	return &(device->fragment);
}

int snd_pcm_oss_device_isstreaming(struct snd_pcm_oss_device *device)
{
	return device->isstreaming;
}

int snd_pcm_oss_device_test(struct snd_pcm_oss_device *device)
{
	printf("Start Test Sound Device...\n");

	int ret = 0;

	int caps;
	ret = ioctl(device->fd, SNDCTL_DSP_GETCAPS, &caps);
	if (ret == 0) {
		printf("\tDSP Capability: %08x\n", caps);
	}

	int format_mask;
	ret = ioctl(device->fd, SNDCTL_DSP_GETFMTS, &format_mask);
	if (ret == 0) {
		printf("\tDSP Format Mask: %08x\n", format_mask);
	}

	printf("Stop Test Sound Device...\n");
	return -1;
}

/**************config functions*****************/

struct snd_pcm_oss_config* snd_pcm_oss_config_new()
{
	struct snd_pcm_oss_config *config = NULL;

	config = (struct snd_pcm_oss_config *)malloc(sizeof(struct snd_pcm_oss_config));
	if (!config) {
		printf("Could not create new sound pcm config\n");
		return NULL;
	}
	memset(config, 0, sizeof(struct snd_pcm_oss_config));

	return config;
}


int snd_pcm_oss_config_init(struct snd_pcm_oss_config *config,
	int format, int samplebits, int samplerate, int channels, int fragmentsize)
{
	if (!config)
		return -1;

	config->format = format;
	config->samplebits = samplebits;
	config->samplerate = samplerate;
	config->channels = channels;
	config->fragmentsize = fragmentsize;
	return 0;
}

void snd_pcm_oss_config_free(struct snd_pcm_oss_config *config)
{
	if (config)
		free(config);
}

