#ifndef sound_pcm_oss_H
#define sound_pcm_oss_H

#ifdef __cplusplus
extern "C" {
#endif

int oss_open(char *device, int *fd);
void oss_close(int fd);
int oss_test_device(int fd);
int oss_set_format(int fd, unsigned int fragmentsize, unsigned int format,
	int channels, int samplerate);
int oss_get_max_buffer_size(int fd, int *size);
int oss_trigger(int fd);
int oss_untrigger(int fd);
int oss_getframe(int fd, void *buf, int buf_size, int *length);

#ifdef __cplusplus
}
#endif

#endif
