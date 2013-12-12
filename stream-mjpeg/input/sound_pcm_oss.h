#ifndef sound_pcm_oss_H
#define sound_pcm_oss_H

#ifdef __cplusplus
extern "C" {
#endif

struct snd_pcm_oss_config;
struct snd_pcm_oss_device;

struct snd_pcm_oss_device *snd_pcm_oss_device_new();

void snd_pcm_oss_device_free(struct snd_pcm_oss_device *device);

int snd_pcm_oss_device_init(struct snd_pcm_oss_device *device,
	const char *name, struct snd_pcm_oss_config *config);

int snd_pcm_oss_device_close(struct snd_pcm_oss_device *device);

int snd_pcm_oss_device_trigger(struct snd_pcm_oss_device * device);
int snd_pcm_oss_device_untrigger(struct snd_pcm_oss_device * device);

int snd_pcm_oss_device_getfd(struct snd_pcm_oss_device * device);

int snd_pcm_oss_device_next_fragment(struct snd_pcm_oss_device * device);

int snd_pcm_oss_device_isstreaming(struct snd_pcm_oss_device *device);

struct buffer * snd_pcm_oss_device_getfrag(struct snd_pcm_oss_device *device);

int snd_pcm_oss_device_test(struct snd_pcm_oss_device *device);



struct snd_pcm_oss_config* snd_pcm_oss_config_new();

int snd_pcm_oss_config_init(struct snd_pcm_oss_config *config,
	int format, int samplebits, int samplerate, int channels, int fragmentsize);

void snd_pcm_oss_config_free(struct snd_pcm_oss_config *config);

#ifdef __cplusplus
}
#endif

#endif
